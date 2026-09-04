# BTC Purchase Tracker

**BTC Purchase Tracker** is a simple, local desktop application for recording Bitcoin purchases over time.

It is designed to work without accounts, cloud services or wallet connections: your purchase history is stored in a local SQLite database under your control.

**Version:** 1.0.0  
**Platforms:** Windows x64 · Linux x86_64  
**Interface languages:** Italian · English  
**Currencies:** EUR · USD

## Features

- Record purchase date, site / exchange, amount spent, BTC, satoshi and TX / transaction ID
- Add, edit and delete purchases
- View total fiat spent, total BTC purchased and total satoshi
- Calculate the average purchase price per BTC
- Calculate the average monthly DCA spending for the selected period
- Filter purchases and totals by year
- View the purchase-price trend on a chronological chart
- View monthly spending on a vertical bar chart
- Sort purchase data directly from the table
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

Each purchase can contain:

- date
- site / exchange
- fiat amount
- BTC
- satoshi
- TX / transaction ID

BTC and satoshi remain mathematically consistent when entering or importing data.

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

Before importing, the application displays a summary of:

- valid rows
- duplicate TX / IDs
- rows containing errors

Valid rows are then inserted in a single SQLite transaction.

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

Windows SmartScreen may display a warning for an unsigned application downloaded from the Internet.

### Linux x86_64

Download the Linux ZIP archive and extract the included AppImage.

If necessary, make it executable:

```bash
chmod +x BTC-Purchase-Tracker-*.AppImage
```

Then run it:

```bash
./BTC-Purchase-Tracker-*.AppImage
```

No system-wide installation is required.

## SHA-256 verification

Each release package includes the corresponding SHA-256 checksum file supplied with that build.

Use it to verify that the executable or AppImage has not been corrupted or altered after packaging.

On Linux, SHA-256 files can be checked with `sha256sum`. On Windows, hashes can be calculated with PowerShell using `Get-FileHash`.

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

## License

BTC Purchase Tracker is free and open-source software licensed under the **GNU General Public License v3.0 or later (GPL-3.0-or-later)**. See [`LICENSE`](LICENSE) for the complete license text.

You may use, study, modify and redistribute the software under the terms of the GPL. If you distribute modified versions or other covered derivative works, the GPL requires the corresponding source code to remain available under the applicable GPL terms.

Copyright (c) 2026 BTC Purchase Tracker contributors.

Third-party components and licensing information are listed in [`THIRD_PARTY.md`](THIRD_PARTY.md).

The **BTC Purchase Tracker** name and logo identify the official project. The software license does not grant permission to present modified or unofficial builds as official releases of BTC Purchase Tracker.

---

**BTC Purchase Tracker** — keep track of your Bitcoin purchases locally, simply and precisely.
