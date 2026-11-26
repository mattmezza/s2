"""Collect the exact Alpine library sources and static archives used by the linker."""

from concurrent.futures import ThreadPoolExecutor
from hashlib import sha512
from pathlib import Path
import re
import shutil
import subprocess
import sys


def download(url, destination, *, optional=False):
    result = subprocess.run(
        ["curl", "-fsSL", "--retry", "3", "-o", destination, url],
        stderr=subprocess.DEVNULL if optional else None,
        check=not optional,
    )
    return result.returncode == 0


def collect_sources(package, sources, alpine_version):
    origin, version, commit = (package[field] for field in ("o", "V", "c"))
    directory = sources / f"{origin}-{version}"
    directory.mkdir()
    base = f"https://raw.githubusercontent.com/alpinelinux/aports/{commit}"
    recipe = directory / "APKBUILD"
    for repository in ("main", "community"):
        recipe_url = f"{base}/{repository}/{origin}"
        if download(f"{recipe_url}/APKBUILD", recipe, optional=True):
            break
    else:
        raise RuntimeError(f"No Alpine recipe found for {origin} at {commit}")

    manifest = re.search(r'^sha512sums="([^"]+)"', recipe.read_text(), re.MULTILINE)
    if not manifest:
        raise RuntimeError(f"No source checksums in {recipe_url}/APKBUILD")
    for line in manifest[1].strip().splitlines():
        digest, name = line.split()
        target = directory / name
        if not download(f"{recipe_url}/{name}", target, optional=True):
            download(f"https://distfiles.alpinelinux.org/distfiles/v{alpine_version}/{name}", target)
        with target.open("rb") as source:
            checksum = sha512()
            for chunk in iter(lambda: source.read(1024 * 1024), b""):
                checksum.update(chunk)
        if checksum.hexdigest() != digest:
            raise RuntimeError(f"Checksum mismatch for {target}")


def main(link_map, destination):
    libraries = destination / "libraries"
    sources = destination / "sources"
    libraries.mkdir(parents=True)
    sources.mkdir()
    archives = sorted({Path(path).resolve() for path in
                       re.findall(r"^LOAD (.+\.a)$", link_map.read_text(), re.MULTILINE)})
    if not archives:
        raise RuntimeError("No static archives found in the linker map")
    for archive in archives:
        shutil.copyfile(archive, libraries / archive.name)

    # CUI and the locally built dependencies have sources outside the Alpine package database.
    packaged = [archive for archive in archives
                if archive.is_relative_to("/usr/lib") or archive.is_relative_to("/lib")]
    owners = subprocess.check_output(["apk", "info", "-Wq", *packaged], text=True).splitlines()
    installed = Path("/lib/apk/db/installed").read_text().split("\n\n")
    by_origin = {}
    for record in installed:
        fields = dict(line.split(":", 1) for line in record.splitlines())
        if fields.get("P") in owners:
            by_origin[fields["o"]] = fields
    alpine_version = ".".join(Path("/etc/alpine-release").read_text().split(".")[:2])
    with ThreadPoolExecutor(max_workers=8) as pool:
        jobs = [pool.submit(collect_sources, fields, sources, alpine_version)
                for fields in by_origin.values()]
        for job in jobs:
            job.result()


if __name__ == "__main__":
    main(Path(sys.argv[1]), Path(sys.argv[2]))
