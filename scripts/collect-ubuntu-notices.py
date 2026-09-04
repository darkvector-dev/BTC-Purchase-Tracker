"""Collect Ubuntu notices for shared libraries already deployed in AppDir."""
import os
import re
import shutil
import subprocess
import sys
import urllib.parse
from pathlib import Path


def collect(appdir, destination):
    packages = set()
    libraries = set()
    for path in appdir.rglob('*'):
        if not path.is_file() or '.so' not in path.name:
            continue
        with path.open('rb') as stream:
            if stream.read(4) != b'\x7fELF':
                continue
        libraries.add(path.name)
    if not libraries:
        raise RuntimeError('No deployed shared libraries found; run linuxdeploy first')
    for name in sorted(libraries):
        result = subprocess.run(['dpkg-query', '-S', '*/' + name],
                                text=True, capture_output=True)
        owners = set()
        for row in result.stdout.splitlines():
            if ': ' not in row:
                continue
            owner, installed_path = row.split(': ', 1)
            if Path(installed_path).name == name and '/lib' in installed_path:
                owners.update(owner.split(', '))
        if not owners:
            raise RuntimeError('Cannot locate Ubuntu license owner for ' + name)
        packages.update(owners)
    destination.mkdir(parents=True, exist_ok=True)
    notices = destination / 'licenses' / 'ubuntu'
    notices.mkdir(parents=True, exist_ok=True)
    common_names = {'GPL-3', 'LGPL-3'}
    rows = []
    sources = set()
    fmt = '${binary:Package}\t${Version}\t${source:Package}\t${source:Version}\n'
    for package in sorted(packages):
        row = subprocess.check_output(
            ['dpkg-query', '-W', '-f=' + fmt, package], text=True
        )
        rows.append(row)
        fields = row.rstrip('\n').split('\t')
        sources.add((fields[2], fields[3]))
        notice = Path('/usr/share/doc') / package.split(':')[0] / 'copyright'
        if not notice.is_file():
            raise RuntimeError('Missing copyright notice: ' + package)
        text = notice.read_text(errors='replace')
        shutil.copyfile(notice, notices / (package.replace(':', '_') + '.copyright'))
        common_names.update(re.findall(r'/usr/share/common-licenses/([A-Za-z0-9.+-]+)', text))
    common = destination / 'licenses' / 'common-licenses'
    common.mkdir(parents=True, exist_ok=True)
    for name in sorted(common_names):
        source = Path('/usr/share/common-licenses') / name
        if source.is_file():
            shutil.copyfile(source, common / name)
    source_links = '\n'.join(
        'https://launchpad.net/ubuntu/+source/'
        + urllib.parse.quote(name, safe='')
        + '/'
        + urllib.parse.quote(version, safe='')
        for name, version in sorted(sources)
    )
    (destination / 'BUILD-INFO.txt').write_text(
        'Application: 1.0.0\nCommit: ' + os.environ.get('GITHUB_SHA', 'unknown')
        + '\nUbuntu packages owning deployed shared libraries (not the build environment):\n'
        + 'Binary package\tVersion\tSource package\tSource version\n'
        + ''.join(rows)
        + '\nExact Ubuntu source package pages:\n'
        + source_links
        + '\n')
    print(f'Collected notices for {len(packages)} packages owning {len(libraries)} deployed libraries')


if __name__ == '__main__':
    collect(Path(sys.argv[1]), Path(sys.argv[2]))
