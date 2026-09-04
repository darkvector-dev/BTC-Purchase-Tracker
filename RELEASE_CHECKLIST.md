# Release 1.0.0

- Build both workflows from the same reviewed commit/tag; preserve that commit.
- Test the resulting Windows portable ZIP and Linux AppImage.
- Inspect the packaged libraries, license notices and BUILD-INFO files.
- Publish corresponding dependency sources with the binaries: the Windows Qt Base
  source archive is produced by the workflow. Check any additional deployed
  libraries. For Linux, use the Ubuntu source packages (including distribution
  patches) matching the bundled libraries and the recorded versions; preserve
  AppImage runtime licensing/source information as applicable.
- This patch improves notice collection; it is not a completed audit of binaries
  that have not yet been built. Complete the preceding check before publication.
- Attach Windows ZIP + SHA-256 and Linux AppImage + SHA-256 to a draft release.
- Attach the source bundles and accompanying license/build information.
- Verify Linux SHA-256 from the download folder and compare Windows ZIP SHA-256.
- Add screenshots made with demonstration data and a short release description.
- Publish the release after the checks. Actions artifacts expire after 30 days.
