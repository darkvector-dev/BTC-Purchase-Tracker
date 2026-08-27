#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-appimage"
APPDIR="$ROOT/AppDir"
TOOLS="$ROOT/.tools"
mkdir -p "$TOOLS"

for cmd in cmake g++ wget; do command -v "$cmd" >/dev/null || { echo "Manca: $cmd"; exit 1; }; done

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "$BUILD" -j"$(nproc)"
rm -rf "$APPDIR"
DESTDIR="$APPDIR" cmake --install "$BUILD"

LINUXDEPLOY="$TOOLS/linuxdeploy-x86_64.AppImage"
QTPLUGIN="$TOOLS/linuxdeploy-plugin-qt-x86_64.AppImage"
[ -f "$LINUXDEPLOY" ] || wget -O "$LINUXDEPLOY" https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
[ -f "$QTPLUGIN" ] || wget -O "$QTPLUGIN" https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
chmod +x "$LINUXDEPLOY" "$QTPLUGIN"

export QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake || true)}"
if [ -z "$QMAKE" ]; then echo "qmake6/qmake non trovato: installa gli strumenti Qt 6 di sviluppo."; exit 1; fi

cd "$ROOT"
export APPIMAGE_EXTRACT_AND_RUN=1
"$LINUXDEPLOY" --appdir "$APPDIR" \
  --desktop-file "$ROOT/assets/btc-purchase-tracker.desktop" \
  --icon-file "$ROOT/assets/btc-purchase-tracker.svg"
"$QTPLUGIN" --appdir "$APPDIR"
"$LINUXDEPLOY" --appdir "$APPDIR" --output appimage

echo "AppImage creata nella cartella: $ROOT"
