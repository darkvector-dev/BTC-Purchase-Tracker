# Third-party components

BTC Purchase Tracker uses dynamically linked Qt 6 libraries (including Widgets,
Core, GUI and SQL) and the SQLite database driver. The application's own license
is GPL-3.0-or-later; third-party components retain their respective licenses.

## Qt

Qt is copyright The Qt Company Ltd. and other contributors. The essential Qt
libraries are available under LGPLv3 or GPL terms, with separate terms for some
embedded third-party code. This project uses the open-source Qt distribution.
Copies of GPLv3 and LGPLv3 are included in `licenses/`.

- Licensing: https://doc.qt.io/qt-6/licensing.html
- Third-party code: https://doc.qt.io/qt-6/licenses-used-in-qt.html
- Source archives: https://download.qt.io/archive/qt/
- Obligations: https://www.qt.io/development/open-source-lgpl-obligations

Recipients may modify or replace LGPL libraries and reverse engineer for debugging
those modifications as permitted by their licenses. Rebuild instructions for the
application are in README.md. Use compatible replacement shared libraries; an
AppImage can be extracted with `--appimage-extract` for inspection and rebuilding.

## SQLite

SQLite is in the public domain: https://www.sqlite.org/copyright.html
It may be provided by Qt's bundled copy or by the Linux runtime. The actual
version and applicable notices must be checked in each generated package.

## Build-specific information

Windows packages include Qt Base source notices collected from the source archive
matching the installed Qt version, plus a build information file. The matching
Qt Base source archive is emitted as a separate build artifact for publication.
Microsoft runtime files retain Microsoft's terms; they are not covered by the
application's GPL license.

Linux packages include Ubuntu copyright files and a package/version inventory.
This inventory intentionally includes build dependencies as well as runtime
packages; it does not assert that every listed package is inside the AppImage.

## Release maintainer check

Before publishing binaries, inspect the actual DLLs/shared libraries and preserve
all applicable notices, including dependencies embedded in Qt. Make the complete
corresponding sources for covered distributed components available with the
release under their applicable licenses. For Ubuntu libraries, retain the exact
Ubuntu source package version and patches, not just the upstream Qt sources.
Third-party website links and generic license texts alone do not complete that
source distribution step. See RELEASE_CHECKLIST.md in the repository.
