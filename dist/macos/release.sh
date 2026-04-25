#!/usr/bin/env bash
# Build, sign, notarize, and package FlameRobin.app for macOS distribution.
#
# Prerequisites (one-time setup):
#   1. brew install cmake wxwidgets dylibbundler
#   2. Firebird installed (creates /Library/Frameworks/Firebird.framework)
#      (Firebird is treated as an external runtime dependency; users must
#       install Firebird separately to use FlameRobin.)
#   3. Developer ID Application certificate in your login keychain.
#      Verify with: security find-identity -v -p codesigning
#      The script auto-detects a single "Developer ID Application: ..."
#      identity. If you have more than one, set SIGN_IDENTITY explicitly.
#   4. Notarization credentials stored in keychain. From an Apple ID with an
#      app-specific password (https://account.apple.com → App-Specific Passwords):
#        xcrun notarytool store-credentials FlameRobinNotary \
#            --apple-id "you@example.com" \
#            --team-id  "ABCDEFGHIJ" \
#            --password "abcd-efgh-ijkl-mnop"
#
# Usage:
#   dist/macos/release.sh              # full release: build + sign + notarize + staple + dmg
#   dist/macos/release.sh --skip-notarize   # sign only, skip notary submission (faster, for testing)
#
# Override defaults via env vars:
#   SIGN_IDENTITY    Codesigning identity. Default: auto-detect the single
#                    "Developer ID Application: ..." in your keychain.
#   NOTARY_PROFILE   Keychain profile name created above (default: FlameRobinNotary)
#   BUILD_DIR        Build directory (default: build-release)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_ROOT"

NOTARY_PROFILE="${NOTARY_PROFILE:-FlameRobinNotary}"
BUILD_DIR="${BUILD_DIR:-build-release}"
ENTITLEMENTS="$REPO_ROOT/dist/macos/entitlements.plist"

SKIP_NOTARIZE=0
if [[ "${1:-}" == "--skip-notarize" ]]; then
    SKIP_NOTARIZE=1
fi

log() { printf '\n\033[1;34m==> %s\033[0m\n' "$*"; }
fail() { printf '\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

# Auto-detect the local Developer ID if SIGN_IDENTITY isn't set explicitly.
# Pulls every "Developer ID Application: ..." line from the keychain; if
# exactly one exists, use it. Otherwise we ask the maintainer to choose.
# Uses a while-read loop instead of mapfile so this stays compatible with
# Apple's bundled bash 3.2.
if [[ -z "${SIGN_IDENTITY:-}" ]]; then
    devids=()
    while IFS= read -r line; do
        [[ -n "$line" ]] && devids+=("$line")
    done < <(
        security find-identity -v -p codesigning 2>/dev/null \
            | sed -nE 's/^[[:space:]]*[0-9]+\)[[:space:]]+[A-F0-9]{40}[[:space:]]+"(Developer ID Application:[^"]*)"$/\1/p'
    )
    if [[ ${#devids[@]} -eq 1 ]]; then
        SIGN_IDENTITY="${devids[0]}"
    elif [[ ${#devids[@]} -eq 0 ]]; then
        fail "No 'Developer ID Application' identity found in keychain. Install one from https://developer.apple.com or set SIGN_IDENTITY explicitly."
    else
        printf 'Multiple Developer ID identities found:\n' >&2
        printf '  %s\n' "${devids[@]}" >&2
        fail "Set SIGN_IDENTITY env var to the one you want to use."
    fi
fi

# ---- Prerequisite checks ----
log "Checking prerequisites"
command -v cmake >/dev/null || fail "cmake not found (brew install cmake)"
command -v xcodebuild >/dev/null || fail "xcodebuild not found (install Xcode command-line tools)"
command -v dylibbundler >/dev/null || fail "dylibbundler not found (brew install dylibbundler)"
[[ -d /Library/Frameworks/Firebird.framework ]] || fail "Firebird framework not found at /Library/Frameworks/Firebird.framework"
security find-identity -v -p codesigning | grep -qF "$SIGN_IDENTITY" \
    || fail "Signing identity not found in keychain: $SIGN_IDENTITY"
log "Using signing identity: $SIGN_IDENTITY"
if [[ $SKIP_NOTARIZE -eq 0 ]]; then
    xcrun notarytool history --keychain-profile "$NOTARY_PROFILE" >/dev/null 2>&1 \
        || fail "Notary keychain profile '$NOTARY_PROFILE' not found. See script header for setup."
fi

# ---- Build ----
log "Configuring CMake (Release, Xcode generator) in $BUILD_DIR"
rm -rf "$BUILD_DIR"
cmake -G Xcode -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$(brew --prefix)" \
    .

log "Building Release configuration"
xcodebuild -project "$BUILD_DIR/flamerobin.xcodeproj" \
    -target flamerobin \
    -configuration Release \
    build

APP_PATH="$BUILD_DIR/Release/flamerobin.app"
[[ -d "$APP_PATH" ]] || fail "Build did not produce $APP_PATH"
BIN_PATH="$APP_PATH/Contents/MacOS/flamerobin"

# ---- Bundle third-party dylibs ----
# The hardened runtime requires every loaded dylib to be signed by the same
# Team ID as the main binary (or be an Apple system library). Homebrew and
# Firebird dylibs are signed by their respective projects, not by us, so they
# fail validation. dylibbundler copies them into Contents/Frameworks/ and
# rewrites the binary's load commands to @rpath/... so we can re-sign them
# with our own Developer ID. The resulting .app is fully self-contained:
# users do not need Homebrew or a system Firebird install to launch it.
log "Bundling third-party dylibs into $APP_PATH/Contents/Frameworks"
mkdir -p "$APP_PATH/Contents/Frameworks"
dylibbundler --overwrite-dir --bundle-deps \
    --fix-file "$BIN_PATH" \
    --dest-dir "$APP_PATH/Contents/Frameworks/" \
    --install-path "@rpath/" \
    --search-path "$(brew --prefix)/lib" \
    --search-path /Library/Frameworks/Firebird.framework/Libraries \
    --search-path /Library/Frameworks/Firebird.framework/Resources/lib

# CMake/Xcode adds stray "@rpath/" LC_RPATH entries to the binary. They were
# inert before bundling (the loader never actually used them since dylibs were
# loaded by absolute path), but with bundled dylibs that resolve through
# @rpath, dyld walks the rpath list — and modern dyld fatally rejects duplicate
# LC_RPATH entries. Strip every "@rpath/" rpath, then add our real one.
while otool -l "$BIN_PATH" | grep -q 'path @rpath/ '; do
    install_name_tool -delete_rpath "@rpath/" "$BIN_PATH"
done
install_name_tool -add_rpath "@executable_path/../Frameworks" "$BIN_PATH" 2>/dev/null || true

# ---- Sign ----
# Sign bundled dylibs first (innermost-out), then the app bundle. --deep on
# the outer sign would also work but signing the dylibs explicitly is more
# predictable and Apple's recommended approach.
log "Signing bundled dylibs with hardened runtime + secure timestamp"
find "$APP_PATH/Contents/Frameworks" -type f \( -name "*.dylib" -o -name "*.so" \) -print0 \
    | xargs -0 -I {} codesign --force --options runtime \
        --sign "$SIGN_IDENTITY" --timestamp {}

log "Signing $APP_PATH with hardened runtime + secure timestamp + entitlements"
codesign --force --options runtime \
    --sign "$SIGN_IDENTITY" \
    --entitlements "$ENTITLEMENTS" \
    --timestamp \
    "$APP_PATH"

log "Verifying signature"
codesign --verify --deep --strict --verbose=2 "$APP_PATH"
codesign -dv --verbose=2 "$APP_PATH" 2>&1 | grep -E "Authority|TeamIdentifier|Timestamp|Runtime"

# ---- Package for notarization ----
DIST_DIR="$BUILD_DIR/dist"
mkdir -p "$DIST_DIR"
SUBMIT_ZIP="$DIST_DIR/flamerobin-submit.zip"

log "Zipping app for notary submission"
ditto -c -k --keepParent "$APP_PATH" "$SUBMIT_ZIP"

# ---- Notarize ----
if [[ $SKIP_NOTARIZE -eq 1 ]]; then
    log "Skipping notarization (--skip-notarize)"
else
    log "Submitting to Apple notary service (this may take a few minutes)"
    xcrun notarytool submit "$SUBMIT_ZIP" \
        --keychain-profile "$NOTARY_PROFILE" \
        --wait

    log "Stapling notarization ticket to app bundle"
    xcrun stapler staple "$APP_PATH"
    xcrun stapler validate "$APP_PATH"

    log "Final Gatekeeper assessment"
    spctl --assess --type execute --verbose "$APP_PATH"
fi

# ---- Final distribution archive ----
VERSION="$(grep -E '^#define FR_VERSION_' src/frversion.h \
    | awk '{print $3}' \
    | paste -sd. - 2>/dev/null || echo "unknown")"

DIST_ZIP="$DIST_DIR/FlameRobin-${VERSION}-macos-arm64.zip"
log "Creating distribution archive: $DIST_ZIP"
rm -f "$DIST_ZIP"
ditto -c -k --keepParent "$APP_PATH" "$DIST_ZIP"

log "Done"
printf '  App:     %s\n' "$APP_PATH"
printf '  Archive: %s\n' "$DIST_ZIP"
[[ $SKIP_NOTARIZE -eq 0 ]] && printf '  Status:  Signed + notarized + stapled — ready to ship\n' \
                           || printf '  Status:  Signed only (not notarized; users will see Gatekeeper warning)\n'
