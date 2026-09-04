# Third-party components

BTC Purchase Tracker uses dynamically linked Qt 6 libraries and the SQLite
database driver. The application is licensed GPL-3.0-or-later; third-party
components retain their respective licenses.

## Qt

Qt is copyright The Qt Company Ltd. and other contributors. This project uses
the open-source Qt distribution. Qt libraries are available under LGPLv3 or GPL
terms, while some embedded third-party components use their own licenses.
Copies of GPLv3 and LGPLv3 are included in `licenses/`.

- Licensing: https://doc.qt.io/qt-6/licensing.html
- Third-party code: https://doc.qt.io/qt-6/licenses-used-in-qt.html
- Source archives: https://download.qt.io/archive/qt/
- Open-source obligations: https://www.qt.io/development/open-source-lgpl-obligations

Recipients may modify or replace LGPL libraries and reverse engineer for
debugging those modifications as permitted by their licenses. Application
rebuild instructions are in README.md. An AppImage can be extracted with
`--appimage-extract` to inspect its contents.

## SQLite

SQLite is in the public domain: https://www.sqlite.org/copyright.html

## Windows package

The Windows build uses Qt Base and Qt SVG. Its package contains the notices
collected from the matching Qt source archives and a `BUILD-INFO.txt` file with
the exact Qt version and source filenames. The workflow also emits the complete
matching Qt Base and Qt SVG archives as separate maintainer artifacts.

The Microsoft Visual C++ runtime is not bundled. Users who do not already have
it can install the official x64 Redistributable linked from README.md.

## Linux AppImage

The AppImage contains copyright notices for the Ubuntu shared libraries actually
deployed inside it, their exact binary/source package versions and direct source
package pages. The information is stored under
`usr/share/doc/btc-purchase-tracker/` and is available after extraction.

The AppImage type-2 runtime identifies its own license, embedded dependencies,
source revision and source locations through the `--appimage-help` and
`--appimage-version` commands.
