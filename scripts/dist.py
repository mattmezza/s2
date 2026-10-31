#!/usr/bin/env python3
"""Create a source archive from tracked files, with the pinned CUI dependency."""
import argparse
import io
from pathlib import Path
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser()
parser.add_argument('--version', required=True)
args = parser.parse_args()
if not args.version or any(c not in '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-' for c in args.version):
    parser.error('invalid version')
name = f's2-{args.version}'
output = ROOT / 'dist' / f'{name}.tar.gz'
output.parent.mkdir(exist_ok=True)
subprocess.run(['python3', 'scripts/fetch_cui.py'], cwd=ROOT, check=True)
tracked = subprocess.check_output(['git', 'archive', '--format=tar', f'--prefix={name}/', 'HEAD'], cwd=ROOT)
with tarfile.open(output, 'w:gz') as archive:
    with tarfile.open(fileobj=io.BytesIO(tracked)) as source:
        for member in source:
            archive.addfile(member, source.extractfile(member) if member.isfile() else None)
    archive.add(ROOT / 'vendor/cui', arcname=f'{name}/vendor/cui', filter=lambda m: None if m.name == f'{name}/vendor/cui/build' or m.name.startswith(f'{name}/vendor/cui/build/') else m)
print(output)
