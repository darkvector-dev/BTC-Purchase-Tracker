# Release 1.1.0

- Run both workflows from the same reviewed commit or tag. Their test steps must pass.
- Confirm that the v1.0.0 database opens without migration or data changes.
- Test every search mode in Italian and English, both alone and combined with the year filter.
- Confirm that search results recalculate totals, averages, the price chart and the monthly chart.
- Confirm that clearing the search restores every purchase for the selected year.
- Test the resulting Windows portable ZIP and Linux AppImage.
- Confirm both SHA-256 files against their corresponding download.
- Create a draft GitHub Release and attach only these normal user downloads:
  - `BTC-Purchase-Tracker-1.1.0-Windows-x64.zip`
  - `BTC-Purchase-Tracker-1.1.0-Windows-x64.zip.sha256`
  - `BTC-Purchase-Tracker-1.1.0-x86_64.AppImage`
  - `BTC-Purchase-Tracker-1.1.0-x86_64.AppImage.sha256`
- Preserve the Windows Qt Base and Qt SVG source archives emitted by Actions;
  publish them separately with the release or provide their exact source links.
- Confirm that the Windows `BUILD-INFO.txt` lists Qt Base and Qt SVG and that the
  AppImage contains `usr/share/doc/btc-purchase-tracker/BUILD-INFO.txt`.
- Add screenshots made with demonstration data and concise release notes.
- Publish the release. GitHub Actions artifacts expire after their retention period.
