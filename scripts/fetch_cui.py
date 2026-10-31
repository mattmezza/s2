#!/usr/bin/env python3
"""Download the pinned, checksum-verified CUI source on first build."""
import hashlib
import io
import json
from pathlib import Path
import shutil
import tarfile
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parent.parent
lock = json.loads((ROOT / 'cui.lock.json').read_text())
target = ROOT / 'vendor/cui'
stamp = target / '.s2-revision'
if stamp.exists() and stamp.read_text().strip() == lock['revision']:
    stamp.touch()
    raise SystemExit(0)
if target.exists():
    raise SystemExit('vendor/cui exists without the expected revision stamp; move it aside and run make deps')
print(f"Downloading CUI {lock['revision']}…", flush=True)
request = urllib.request.Request(lock['origin'], headers={'User-Agent': 's2-build'})
with urllib.request.urlopen(request, timeout=120) as response:
    archive = response.read()
if hashlib.sha256(archive).hexdigest() != lock['archive_sha256']:
    raise SystemExit('CUI archive checksum mismatch; refusing to extract')
target.parent.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix='.cui-', dir=target.parent) as staging:
    with tarfile.open(fileobj=io.BytesIO(archive), mode='r:gz') as tar:
        # Reject links and traversal before extraction, including on older Python.
        for member in tar.getmembers():
            if member.name.startswith('/') or '..' in Path(member.name).parts or not (member.isfile() or member.isdir()):
                raise SystemExit(f'Unsafe CUI archive member: {member.name}')
        tar.extractall(staging, **({'filter': 'data'} if hasattr(tarfile, 'data_filter') else {}))
    children = list(Path(staging).iterdir())
    if len(children) != 1 or not (children[0] / 'CMakeLists.txt').is_file():
        raise SystemExit('Unexpected CUI archive layout')
    (children[0] / '.s2-revision').write_text(lock['revision'] + '\n')
    shutil.move(str(children[0]), target)
print('CUI ready.')
