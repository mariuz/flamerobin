#!/usr/bin/env bash
# Build, sign, notarize, and package FlameRobin.app for macOS distribution.
#
# Prerequisites (one-time setup):
#   1. brew install cmake wxwidgets
#   2. Firebird installed (creates /Library/Frameworks/Firebird.framework)
#   3. Developer ID Application certificate in your login keychain
#      Verify: security find-identity -v -p codesigning
#   4. Notarization credentials stored in keychain. From an Apple ID with an
#      app-specific password (https://account.apple.com → App-Specific Passwords):
#        xcrun notarytool store-credentials FlameRobinNotary \
#            --apple-id "you@example.com" \
#            --team-id  "5CSH5U4F8F" \
#            --password "abcd-efgh-ijkl-mnop"
#
# Usage:
#   dist/macos/release.sh              # full release: build + sign + notarize + staple + dmg
#   dist/macos/release.sh --skip-notarize   # sign only, skip notary submission (faster, for testing)
#
# Override defaults via env vars:
#   SIGN_IDENTITY    Codesigning identity (default: Code Infinity Developer ID)
#   NOTARY_PROFILE   Keychain profile name created above (default: FlameRobinNotary)
#   BUILD_DIR        Build directory (default: build-release)

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$REPO_ROOT"

SIGN_IDENTITY="${SIGN_IDENTITY:-Developer ID Application: Code Infinity (Pty) Ltd (5CSH5U4F8F)}"
NOTARY_PROFILE="${NOTARY_PROFILE:-FlameRobinNotary}"
BUILD_DIR="${BUILD_DIR:-build-release}"

SKIP_NOTARIZE=0
if [[ "${1:-}" == "--skip-notarize" ]]; then
    SKIP_NOTARIZE=1
fi

log() { printf '\n\033[1;34m==> %s\033[0m\n' "$*"; }
fail() { printf '\033[1;31mERROR: %s\033[0m\n' "$*" >&2; exit 1; }

# ---- Prerequisite checks ----
log "Checking prerequisites"
command -v cmake >/dev/null || fail "cmake not found (brew install cmake)"
command -v xcodebuild >/dev/null || fail "xcodebuild not found (install Xcode command-line tools)"
[[ -d /Library/Frameworks/Firebird.framework ]] || fail "Firebird framework not found at /Library/Frameworks/Firebird.framework"
security find-identity -v -p codesigning | grep -q "$SIGN_IDENTITY" \
    || fail "Signing identity not found in keychain: $SIGN_IDENTITY"
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

# ---- Sign ----
log "Signing $APP_PATH with hardened runtime + secure timestamp"
codesign --force --deep --options runtime \
    --sign "$SIGN_IDENTITY" \
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
