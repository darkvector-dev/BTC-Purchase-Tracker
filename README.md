# BTC Purchase Tracker

**BTC Purchase Tracker** is a simple, local desktop application for recording Bitcoin purchases over time.

It is designed to work without accounts, cloud services or wallet connections: your purchase history is stored in a local SQLite database under your control.

**Version:** 1.1.1<br>
**Platforms:** Windows x64 · Linux x86_64 · macOS Apple Silicon (M1 and later)<br>
**Interface languages:** Italian · English  
**Currencies:** EUR · USD

## Changes in 1.1.1

- Fixed reimport of exported CSVs: the `TOTALI` / `TOTALS` summary is skipped without an error or double-counting, in Italian/English and EUR/USD.
- Fixed database backup creation and replacement on Windows by releasing temporary file handles before removal.
- Added a GitHub Actions build and DMG packaging for macOS Apple Silicon (M1 and later).
- Made the search shortcut hint follow the platform (`Cmd+F` on macOS).
- Aligned manual purchase entry/editing with CSV validation: dates must fall between Bitcoin’s genesis block on **3 January 2009** and **today**, inclusive.
- Added CSV regression tests and enabled the existing test suite in Windows and macOS builds.

## Features

- Record purchase date, site / exchange, amount spent, BTC, satoshi and TX / transaction ID
- Add, edit and delete purchases
- View total fiat spent, total BTC purchased and total satoshi
- Calculate the average purchase price per BTC
- Calculate the average monthly DCA spending for the selected period
- Filter purchases and totals by year
- View the purchase-price trend on a chronological chart
- Hover over the chart to see purchase dates, amounts spent and prices per BTC
- Inspect all purchases on the same date and their daily spending total
- View monthly spending on a vertical bar chart
- Sort purchase data directly from the table
- Search purchases by date, exchange, amount, BTC / satoshi or TX / transaction ID
- Combine live search with the year filter and recalculate visible totals, averages and charts
- Click summary values to copy them to the clipboard
- Import purchases from CSV with validation and duplicate detection
- Export the complete purchase history to CSV
- Export a purchase report to PDF
- Create manual SQLite database backups
- Automatically maintain a CSV safety copy next to the database
- Choose where the database is stored on first launch
- Move the active database to another folder while retaining the previous copy as a safety backup
- Switch the interface between Italian and English
- Export a diagnostic log for troubleshooting
- Optional project support through a Lightning BOLT12 Offer

## Local and offline by design

BTC Purchase Tracker does not require:

- an account
- a cloud service
- a wallet connection
- private keys or seed phrases

The application stores its data locally in:

`btc-purchase-tracker.sqlite`

On first launch, you choose the folder where the database will be stored.

## EUR and USD databases

When creating a new database, BTC Purchase Tracker asks you to choose either **EUR** or **USD**.

The selected currency is permanently associated with that database and cannot be changed later. The application does **not** perform currency conversion.

If you want to track purchases made in a different currency, use a separate database.

## Data precision

Financial and Bitcoin values are stored using integer units:

- fiat amounts are stored as integer cents
- Bitcoin amounts are stored as integer satoshi

BTC values shown in the interface are calculated from satoshi. Bitcoin is not stored using floating-point values.

## Purchase overview

The main window provides:

- total fiat spent
- total BTC purchased
- total satoshi
- average purchase price per BTC
- year-based filtering
- chronological purchase-price chart
- sortable purchase table

Purchase dates must be **3 January 2009 or later** (Bitcoin’s genesis block)
and **no later than the current date on your computer**, with both endpoints
included. Earlier dates and future dates are rejected when saving a purchase
or importing a CSV. The upper limit follows the current day; it is not a fixed
release date. Existing database records are not automatically rewritten.

Each purchase can contain:

- date
- site / exchange
- fiat amount
- BTC
- satoshi
- TX / transaction ID

BTC and satoshi remain mathematically consistent when entering or importing data.

## Search and filters

The year filter can be combined with a live search across all purchase fields or
limited to date, exchange, amount, BTC / satoshi or TX / transaction ID.

Search results immediately update the visible table, totals, average purchase
price, monthly DCA average and charts. Press `Ctrl+F` on Windows/Linux or `Cmd+F` on macOS to focus the search field; clear the field to return to the complete view for the selected year.

Dates can be searched using a complete date, month/year or year. Amount searches
use the permanent EUR or USD currency of the active database. BTC values accept
either decimal notation or whole satoshi values.

## CSV import

BTC Purchase Tracker can import CSV files using common column names such as:

- `Data`, `Date`, `Data acquisto`
- `Sito`, `Site`, `Exchange`, `Piattaforma`
- `Euro`, `EUR`, `Euro spesi`
- `USD`, `Dollari`, `Dollars`, `USD spent`
- `BTC`, `Bitcoin`, `BTC on-chain`
- `Satoshi`, `Sats`
- `TX`, `TXID`, `ID transazione`

Supported separators include:

- semicolon (`;`)
- comma (`,`)
- tab

BTC **or** satoshi is sufficient. If both are present, they must represent the same amount.

Supported date formats include:

- `26/08/2026`
- `2026-08-26`
- `26-08-2026`

CSV dates use the same allowed range: **3 January 2009 through today, inclusive**.

Before importing, the application displays a summary of:

- valid rows
- duplicate TX / IDs
- rows containing errors

Valid rows are then inserted in a single SQLite transaction.

The `TOTALI` / `TOTALS` summary from the application’s CSV export is ignored
on import, independently of the interface language and database currency.
It is neither an extra purchase nor an invalid row. Actual malformed purchases
are still reported. Duplicate detection uses nonempty TX / IDs; purchases without
a TX / ID are not automatically deduplicated when importing the same file again.

## Automatic CSV safety copy

BTC Purchase Tracker automatically maintains:

`btc_purchase_tracker_autobackup.csv`

in the same folder as the active SQLite database.

The file is refreshed after relevant database operations, providing an additional human-readable copy of the purchase history.

For a complete application backup, the SQLite database itself remains the primary file to preserve.

## Diagnostic log

A diagnostic log can be exported from the **Info** menu when troubleshooting is required.

The exported log is limited to approximately **200 KB** and contains technical application events only. It does not include purchase amounts, BTC / satoshi values, transaction notes, TX / IDs or the Lightning BOLT12 Offer.

The user's home-directory path is masked when it appears in operating-system error messages.

## Downloads

Prebuilt packages are available from the GitHub **Releases** section.

### Windows x64

Download the Windows ZIP archive, extract it and run:

`BTC-Purchase-Tracker.exe`

The Windows release is portable and does not require a traditional installer.

### Windows SmartScreen warning

On Windows 10/11, Microsoft Defender SmartScreen may display a blue warning because the application is unsigned and has not yet built a recognized reputation. If you downloaded it from the official GitHub Releases page, click **More info** and then **Run anyway**. The application is not code-signed because trusted code-signing requires a verified publisher identity, which conflicts with the project's anonymous, privacy-focused nature. The source code is publicly available for review, and the published SHA-256 checksums can be used to verify that downloaded files match the official release.

The archive does not bundle the Microsoft Visual C++ runtime. If the application does not start or Windows reports a missing runtime DLL, install the latest official **Microsoft Visual C++ Redistributable x64** from [Microsoft's download page](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170), then start the application again.

### Linux x86_64

Download the Linux AppImage from the release assets. If it is supplied inside a ZIP, extract it first.

If necessary, make it executable:

```bash
chmod +x BTC-Purchase-Tracker-*.AppImage
```

Then run it:

```bash
./BTC-Purchase-Tracker-*.AppImage
```

No system-wide installation is required.

### macOS Apple Silicon (M1 and later)

The macOS workflow builds a native **arm64** app for **macOS 12 Monterey or later**.
Intel Macs are not targeted.

Download the `.dmg`, open it, and drag **BTC Purchase Tracker** into
**Applications**. Eject the disk image and launch the app from Applications.
Qt and SQLite are included; no Homebrew, Qt installation or Rosetta is needed.
Choose a writable folder for your database, such as a dedicated folder in Documents.

This build is ad-hoc signed, without an Apple Developer ID or notarization.
If macOS blocks it because the developer cannot be verified, after attempting to
open it go to **System Settings → Privacy & Security → Open Anyway** and confirm,
provided you trust the source of the download. See
[Apple’s instructions](https://support.apple.com/en-us/102445).

## SHA-256 verification

Download `checksums.txt` alongside the release archives. Its entries must refer to the exact files published on the GitHub Releases page: the complete Windows/Linux ZIP files and, when available, the macOS DMG. The Mac workflow also provides a separate `.dmg.sha256` file.

On Linux, place `checksums.txt` and the downloaded ZIP file or files in the same folder and run:

```bash
sha256sum -c checksums.txt --ignore-missing
```

On Windows, calculate the hash of the downloaded ZIP in PowerShell using the corresponding command, then compare it with the value in `checksums.txt`:

```powershell
Get-FileHash .\BTC-Purchase-Tracker-1.1.1-Windows-x64.zip -Algorithm SHA256
Get-FileHash .\BTC-Purchase-Tracker-1.1.1-x86_64.zip -Algorithm SHA256
```

On macOS, put the DMG and its matching `.dmg.sha256` file in the same folder:

```bash
shasum -a 256 -c BTC-Purchase-Tracker-1.1.1-macOS-arm64.dmg.sha256
```

A matching checksum checks file integrity against that checksum; it is not a publisher signature.

## Building from source

BTC Purchase Tracker is written in **C++20** using **Qt 6 Widgets** and **Qt SQL / SQLite**.

### Arch Linux / EndeavourOS

Install the typical build dependencies:

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base
```

Build and run:

```bash
./build.sh
./build/btc-purchase-tracker
```

### AppImage

The repository includes an AppImage build script:

```bash
./build_appimage.sh
```

A GitHub Actions workflow is also included at:

`.github/workflows/appimage.yml`

### Windows

The repository includes a GitHub Actions workflow for generating a portable Windows x64 build:

`.github/workflows/windows.yml`

### macOS Apple Silicon

Run **Actions → Build macOS Apple Silicon → Run workflow** on GitHub.
The workflow is `.github/workflows/macos.yml`; it also runs on `v*` tags.
It uses the native Apple Silicon `macos-15` runner, Qt 6.8.3, and runs the tests
before packaging. Download the `BTC-Purchase-Tracker-1.1.1-macOS-arm64`
artifact and extract the DMG and its checksum from the artifact ZIP.
The dependency-source artifact is for maintainers.

To build locally, install Xcode command-line tools, CMake and the official Qt
6.8 desktop macOS kit, then put that kit’s `bin` directory on `PATH`:

```bash
cmake -S . -B build-macos -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(qmake -query QT_INSTALL_PREFIX)" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
cmake --build build-macos --parallel 3
ctest --test-dir build-macos --output-on-failure
bash scripts/package-macos.sh build-macos
```

Packaging needs internet access for the matching Qt source archives. Use a fresh
`dist/macos-package` staging directory for each package.
Implementation references: [Qt deployment](https://doc.qt.io/qt-6.8/macos-deployment.html),
[Qt macOS requirements](https://doc.qt.io/qt-6.8/macos.html),
[GitHub runners](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).

## License

BTC Purchase Tracker is free and open-source software licensed under the **GNU General Public License v3.0 or later (GPL-3.0-or-later)**. See [`LICENSE`](LICENSE) for the complete license text.

You may use, study, modify and redistribute the software under the terms of the GPL. If you distribute modified versions or other covered derivative works, the GPL requires the corresponding source code to remain available under the applicable GPL terms.

Copyright (c) 2026 BTC Purchase Tracker contributors.

Third-party components and licensing information are listed in [`THIRD_PARTY.md`](THIRD_PARTY.md).

The **BTC Purchase Tracker** name and logo identify the official project. The software license does not grant permission to present modified or unofficial builds as official releases of BTC Purchase Tracker.

---

**BTC Purchase Tracker** — keep track of your Bitcoin purchases locally, simply and precisely.
