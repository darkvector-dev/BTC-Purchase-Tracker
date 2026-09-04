"""Retain Ubuntu notices and exact installed/source package versions."""
import os
import shutil
import subprocess
import sys
from pathlib import Path

destination = Path(sys.argv[1])
fmt = "${binary:Package}\t${Version}\t${source:Package}\t${source:Version}\n"
inventory = subprocess.check_output(["dpkg-query", "-W", "-f=" + fmt], text=True)
(destination / "BUILD-INFO.txt").write_text(
    "Application: 1.0.0\nCommit: " + os.environ.get("GITHUB_SHA", "unknown")
    + "\nInstalled build environment (not a bundled-library list):\n"
    + "Binary package\tVersion\tSource package\tSource version\n" + inventory)
for row in inventory.splitlines():
    package = row.split("\t")[0].split(":")[0]
    notice = Path("/usr/share/doc") / package / "copyright"
    if notice.is_file():
        target = destination / "licenses" / "ubuntu" / (package + ".copyright")
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(notice, target)
# Debian copyright files can refer to these full texts.
shutil.copytree("/usr/share/common-licenses", destination / "licenses" / "common-licenses", dirs_exist_ok=True)
