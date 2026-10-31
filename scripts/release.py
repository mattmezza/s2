#!/usr/bin/env python3
"""Validate, tag, push and publish a GitHub release: make release name=v0.1.0."""
import hashlib
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent

def run(*args, capture=False):
    result = subprocess.run(args, cwd=ROOT, check=True, text=True,
                            stdout=subprocess.PIPE if capture else None)
    return result.stdout.strip() if capture else None

if len(sys.argv) != 2 or not re.fullmatch(r'v\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?', sys.argv[1]):
    raise SystemExit('Usage: make release name=v0.1.0')
tag = sys.argv[1]
version = tag[1:]
if run('git', 'status', '--porcelain', capture=True):
    raise SystemExit('Commit all changes before releasing.')
if f'VERSION = {version}' not in (ROOT / 'config.mk').read_text().splitlines():
    raise SystemExit(f'Set VERSION = {version} in config.mk and commit it first.')
if f'## {tag}' not in (ROOT / 'CHANGELOG.md').read_text():
    raise SystemExit(f'Add {tag} release notes to CHANGELOG.md and commit them first.')
run('gh', 'auth', 'status')
run('make', '-j4')
run('make', 'test-text', 'test-blur')
for test in ('exit-codes.py', 'about.py', 'connectors.py', 'smoke.py'):
    run('python3', f'tests/{test}')
run('make', 'site')
run('python3', 'scripts/dist.py', '--version', version)
archive = ROOT / 'dist' / f's2-{version}.tar.gz'
checksum = archive.with_name('SHA256SUMS')
checksum.write_text(f'{hashlib.sha256(archive.read_bytes()).hexdigest()}  {archive.name}\n')
notes = (ROOT / 'CHANGELOG.md').read_text().split(f'## {tag}', 1)[1].split('\n## ', 1)[0].strip()
notes_path = ROOT / 'dist/release-notes.md'
notes_path.write_text(notes + '\n\nBuild instructions and runtime dependencies are in README.md. This source archive includes the pinned CUI dependency for offline builds.\n')
head = run('git', 'rev-parse', 'HEAD', capture=True)
if run('git', 'tag', '--list', tag, capture=True):
    if run('git', 'rev-list', '-n1', tag, capture=True) != head:
        raise SystemExit(f'{tag} already points to a different commit; refusing to move it.')
else:
    run('git', 'tag', '-a', tag, '-m', f's2 {tag}')
run('git', 'push', 'origin', 'HEAD', f'refs/tags/{tag}')
run('gh', 'release', 'create', tag, str(archive), str(checksum), '--verify-tag', '--title', f's2 {tag}', '--notes-file', str(notes_path))
