# Release 1.1.1

- Run all three workflows from the same reviewed commit or tag. Their test steps must pass.
- Confirm that the v1.0.0 database opens without migration or data changes.
- Test every search mode in Italian and English, both alone and combined with the year filter.
- Confirm that search results recalculate totals, averages, the price chart and the monthly chart.
- Confirm that clearing the search restores every purchase for the selected year.
- Test the resulting Windows portable ZIP and Linux AppImage.
- Build the Apple Silicon DMG and test it on the iMac M3 before publishing.
  Check first launch/Gatekeeper, icon, Retina layout, Cmd+F, and IT/EN labels.
  Add/edit/delete demo purchases, export CSV/PDF, import CSV, create and restore
  a SQLite backup, close and reopen the installed app. No local Qt should be required.
- Reimport an exported CSV in IT/EN and EUR/USD: TOTALI/TOTALS must not produce
  an error or become a purchase. Nonempty TXIDs must still be deduplicated.
- Confirm all SHA-256 files against their corresponding download.
- Create a draft GitHub Release and attach only these normal user downloads:
  - `BTC-Purchase-Tracker-1.1.1-Windows-x64.zip`
  - `BTC-Purchase-Tracker-1.1.1-Windows-x64.zip.sha256`
  - `BTC-Purchase-Tracker-1.1.1-x86_64.AppImage`
  - `BTC-Purchase-Tracker-1.1.1-x86_64.AppImage.sha256`
  - `BTC-Purchase-Tracker-1.1.1-macOS-arm64.dmg`
  - `BTC-Purchase-Tracker-1.1.1-macOS-arm64.dmg.sha256`
- Preserve the Windows and macOS Qt Base and Qt SVG source archives emitted by Actions;
  publish them separately with the release or provide their exact source links.
- Confirm that the Windows `BUILD-INFO.txt` lists Qt Base and Qt SVG and that the
  AppImage contains `usr/share/doc/btc-purchase-tracker/BUILD-INFO.txt`.
- Add screenshots made with demonstration data and concise release notes.
- Publish the release. GitHub Actions artifacts expire after their retention period.
