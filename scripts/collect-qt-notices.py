"""Retain Qt license texts and files referenced by attribution metadata."""

import json
import posixpath
import re
import sys
import tarfile
from pathlib import Path, PurePosixPath


def collect(archive: Path, destination: Path) -> None:
    with tarfile.open(archive) as source:
        members = {
            member.name: member
            for member in source
            if member.isfile()
            and not PurePosixPath(member.name).is_absolute()
            and ".." not in PurePosixPath(member.name).parts
        }

        selected = set()

        for name, member in members.items():
            path = PurePosixPath(name)

            # Test/example documentation is not part of the deployed modules.
            if len(path.parts) > 1 and path.parts[1] in {"tests", "examples", "doc"}:
                continue

            if (
                "LICENSES" in path.parts
                or path.name == "qt_attribution.json"
                or re.fullmatch(
                    r"(LICENSE|LICENCE|COPYING|COPYRIGHT|NOTICE)([.\-_].*)?",
                    path.name,
                    re.IGNORECASE,
                )
            ):
                selected.add(name)

            if path.name != "qt_attribution.json":
                continue

            with source.extractfile(member) as stream:
                entries = json.loads(stream.read(), strict=False)

            if isinstance(entries, dict):
                entries = [entries]

            for entry in entries:
                license_files = entry.get("LicenseFile", [])
                if isinstance(license_files, str):
                    license_files = [license_files]

                for license_file in license_files:
                    candidates = [
                        posixpath.normpath(
                            str(path.parent / entry.get("Path", "") / license_file)
                        ),
                        posixpath.normpath(str(path.parent / license_file)),
                    ]
                    found = next(
                        (candidate for candidate in candidates if candidate in members),
                        None,
                    )
                    if found is None:
                        raise RuntimeError(
                            f"Missing license referenced by {name}: {license_file}"
                        )
                    selected.add(found)

        if not selected:
            raise RuntimeError("No license notices found in source archive")

        for name in sorted(selected):
            target = destination.joinpath(*PurePosixPath(name).parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            with source.extractfile(members[name]) as stream:
                target.write_bytes(stream.read())

    print(f"{archive.name}: retained {len(selected)} license/attribution files")


if __name__ == "__main__":
    collect(Path(sys.argv[1]), Path(sys.argv[2]))
