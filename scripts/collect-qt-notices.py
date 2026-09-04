"""Copy license/attribution documents without extracting archive paths."""
import sys
import tarfile
from pathlib import Path, PurePosixPath

archive, destination = map(Path, sys.argv[1:])
count = 0
with tarfile.open(archive) as source:
    for member in source:
        path = PurePosixPath(member.name)
        if path.is_absolute() or ".." in path.parts or not member.isfile():
            continue
        name = path.name.lower()
        if not (name.startswith(("license", "licence", "copying", "copyright"))
                or name == "qt_attribution.json" or "LICENSES" in path.parts):
            continue
        target = destination.joinpath(*path.parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        with source.extractfile(member) as data:
            target.write_bytes(data.read())
        count += 1
if count == 0:
    raise SystemExit("No Qt license notices found")
print(f"Collected {count} Qt source license/attribution documents")
