#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
BUILD="$HERE/build"
APPDIR="$HERE/AppDir"
TOOLS="$HERE/tools"

for cmd in cmake ninja curl; do
  command -v "$cmd" >/dev/null || { echo "Manca: $cmd"; exit 1; }
done

cmake -S "$HERE" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" --parallel

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/share/applications" "$APPDIR/usr/share/icons/hicolor/scalable/apps" "$TOOLS"
cp "$BUILD/btc-purchase-tracker" "$APPDIR/usr/bin/"
cp "$HERE/appimage/btc-purchase-tracker.desktop" "$APPDIR/usr/share/applications/"
cp "$HERE/resources/btc-purchase-tracker.svg" "$APPDIR/usr/share/icons/hicolor/scalable/apps/"

LD="$TOOLS/linuxdeploy-x86_64.AppImage"
QT="$TOOLS/linuxdeploy-plugin-qt-x86_64.AppImage"

if [ ! -f "$LD" ]; then
  curl -L --fail -o "$LD" https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fi
if [ ! -f "$QT" ]; then
  curl -L --fail -o "$QT" https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
fi
chmod +x "$LD" "$QT"

export VERSION="1.0.0"
export OUTPUT="$HERE/BTC-Purchase-Tracker-1.0.0-x86_64.AppImage"

# --appimage-extract-and-run evita di dipendere da FUSE sul sistema di build.
"$LD" --appimage-extract-and-run \
  --appdir "$APPDIR" \
  --executable "$APPDIR/usr/bin/btc-purchase-tracker" \
  --desktop-file "$APPDIR/usr/share/applications/btc-purchase-tracker.desktop" \
  --icon-file "$APPDIR/usr/share/icons/hicolor/scalable/apps/btc-purchase-tracker.svg" \
  --plugin qt \
  --output appimage

echo
echo "Creata: $OUTPUT"
