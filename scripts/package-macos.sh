#!/usr/bin/env bash
# Run from the repository root after a Release build with Qt 6.8 on PATH.
set -euo pipefail

build_dir="${1:-build-macos}"
version="1.1.1"
app_name="BTC Purchase Tracker.app"
package_dir="dist/macos-package"
app="$package_dir/$app_name"
resources="$app/Contents/Resources"
qt_version="$(qmake -query QT_VERSION)"
[[ "$qt_version" =~ ^6\.8\.[0-9]+$ ]]
test -d "$build_dir/$app_name"
# Refuse to reuse a staging folder so stale plugins cannot enter a new package.
test ! -e "$package_dir"
mkdir -p "$package_dir"
ditto "$build_dir/$app_name" "$app"

mkdir -p "$resources/licenses"
cp LICENSE "$resources/LICENSE.txt"
cp README.md THIRD_PARTY.md "$resources/"
cp -R licenses/. "$resources/licenses/"
{
    echo "Application: $version"
    echo "Commit: ${GITHUB_SHA:-local}"
    echo "Qt: $qt_version"
    echo "Application architecture: arm64"
    echo "Minimum macOS: 12.0"
    echo "Signature: ad-hoc; no Developer ID or Apple notarization"
} > "$resources/BUILD-INFO.txt"

for module in qtbase qtsvg; do
    source_name="$module-everywhere-src-$qt_version.tar.xz"
    source_url="https://download.qt.io/archive/qt/6.8/$qt_version/submodules/$source_name"
    curl --fail --location --retry 3 "$source_url" -o "dist/$source_name"
    python3 scripts/collect-qt-notices.py "dist/$source_name" "$resources/licenses/$module"
    echo "Qt source: $source_name" >> "$resources/BUILD-INFO.txt"
    echo "Upstream: $source_url" >> "$resources/BUILD-INFO.txt"
done

# macdeployqt supplies Qt frameworks, the Cocoa platform and SQL drivers.
# An ad-hoc signature supports local ARM execution without publisher credentials.
macdeployqt "$app" -verbose=2 -codesign=-
test -f "$app/Contents/PlugIns/platforms/libqcocoa.dylib"
test -f "$app/Contents/PlugIns/sqldrivers/libqsqlite.dylib"
test "$(lipo -archs "$app/Contents/MacOS/BTC Purchase Tracker")" = "arm64"
codesign --verify --deep --strict --verbose=2 "$app"

ln -s /Applications "$package_dir/Applications"
cp README.md THIRD_PARTY.md "$package_dir/"
cp LICENSE "$package_dir/LICENSE.txt"
cp macos/LEGGIMI-MAC.txt "$package_dir/"
dmg="BTC-Purchase-Tracker-$version-macOS-arm64.dmg"
hdiutil create -volname "BTC Purchase Tracker" -srcfolder "$package_dir" \
    -format UDZO -ov "dist/$dmg"
hdiutil verify "dist/$dmg"
(cd dist && shasum -a 256 "$dmg" > "$dmg.sha256")
