macOS Release Builds
====================

This directory contains the tooling to produce a signed, notarized
distributable `FlameRobin.app` for macOS. The output is a self-contained
`.app` bundle that runs on any Apple Silicon Mac (with Firebird installed
separately) — no Homebrew, no source tree, no Gatekeeper warning.

Quick start
-----------

```sh
dist/macos/release.sh
```

Output: `build-release/dist/FlameRobin-<version>-macos-arm64.zip`

For a faster sign-only test build (no Apple round-trip):

```sh
dist/macos/release.sh --skip-notarize
```

What the script does
--------------------

1. **Build** — clean Release configuration via CMake (Xcode generator) +
   `xcodebuild`.
2. **Bundle** — uses `dylibbundler` to copy every Homebrew dylib (wxWidgets
   and its transitive deps: libpng, libtiff, libjpeg, libwebp, etc.) into
   `flamerobin.app/Contents/Frameworks/`, and rewrites the binary's load
   commands to `@rpath/*.dylib`. Firebird's `libfbclient` is intentionally
   left as an external reference — users install Firebird themselves.
3. **Strip duplicate rpaths** — Xcode emits stray empty `@rpath/` LC_RPATH
   entries; modern dyld treats duplicates as a fatal error, so we delete
   them and add a single `@executable_path/../Frameworks` rpath.
4. **Sign** — every bundled dylib first, then the `.app` bundle, with
   hardened runtime + Apple secure timestamp. Uses your Developer ID.
5. **Notarize** — submits the zipped bundle to Apple's notary service via
   `xcrun notarytool`, waits for the result.
6. **Staple** — embeds the notarization ticket so Gatekeeper accepts the
   app offline.
7. **Package** — produces `FlameRobin-<version>-macos-arm64.zip`, ready to
   upload to a release page.

One-time setup
--------------

### 1. Build dependencies

```sh
brew install cmake wxwidgets dylibbundler
```

Firebird must also be installed (the installer from
https://www.firebirdsql.org/ creates `/Library/Frameworks/Firebird.framework`).

### 2. Developer ID certificate

A Developer ID Application certificate must be in your login keychain.
Verify with:

```sh
security find-identity -v -p codesigning
```

You should see a line like
`Developer ID Application: <Your Org> (<TEAMID>)`.
Without one, the script can produce a sign-only build (`--skip-notarize`)
that will run on your own machine but show Gatekeeper warnings on others.

### 3. Notarization credentials

Apple's notary service authenticates via an **app-specific password**
generated under the Apple ID associated with your Developer team.

Generate one (browser, ~30 seconds):

1. Sign in at https://account.apple.com
2. *Sign-in and Security* → *App-Specific Passwords* → *+*
3. Name it (e.g. `FlameRobin Notary`). Apple shows the password as
   `xxxx-xxxx-xxxx-xxxx` — copy it (it is only shown once).

Store it in your keychain under the profile name the script uses
(`FlameRobinNotary`):

```sh
xcrun notarytool store-credentials FlameRobinNotary \
    --apple-id "you@example.com" \
    --team-id  "ABCDEFGHIJ" \
    --password "xxxx-xxxx-xxxx-xxxx"
```

The password lives only in your local keychain — never in the repo or
shell history. The script references the profile name by string.

Configuration
-------------

Override defaults with environment variables:

| Variable         | Default                                                                  |
|------------------|--------------------------------------------------------------------------|
| `SIGN_IDENTITY`  | Auto-detect a single `Developer ID Application: ...` from your keychain. |
| `NOTARY_PROFILE` | `FlameRobinNotary`                                                       |
| `BUILD_DIR`      | `build-release`                                                          |

If you have multiple Developer ID certificates the script will list them
and ask you to set `SIGN_IDENTITY` explicitly:

```sh
SIGN_IDENTITY="Developer ID Application: Your Org (XXXXXXXXXX)" \
NOTARY_PROFILE=MyNotary \
dist/macos/release.sh
```

Verifying a release
-------------------

After a successful run:

```sh
# What identity signed it
codesign -dv --verbose=2 build-release/Release/flamerobin.app

# Will Gatekeeper open it?
spctl --assess --type execute --verbose build-release/Release/flamerobin.app
# expect: accepted — source=Notarized Developer ID

# Notarization ticket present?
xcrun stapler validate build-release/Release/flamerobin.app
```

To test on another Mac, copy the zip from `build-release/dist/`,
unzip, and double-click — no warnings, no install required (apart from
Firebird if the user wants to actually connect to a database).
