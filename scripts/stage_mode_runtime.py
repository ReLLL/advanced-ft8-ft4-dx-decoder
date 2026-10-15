"""Stage verified GCC runtime copies compatible with the macOS 15 support floor."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
import urllib.request

SHA = "a506cdc6f124d98860376b407ea1ccd7efc26c17599dd36e8d84fe5a8d9174d7"
COMMIT = "fb9b5cc0b6f88cbd784aa327bfd13b0f7faa44f5"
LIBRARIES = ("libgfortran.5.dylib", "libquadmath.0.dylib", "libgcc_s.1.1.dylib")
LGPL_SHA = "32434afcc8666ba060e111d715bfdb6c2d5dd8a35fa4d3ab8ad67d8f850d2f2b"


def prepare(destination, cache):
    destination.mkdir(parents=True, exist_ok=True)
    cache.mkdir(parents=True, exist_ok=True)
    bottle = cache / "gcc14-sequoia.tar.gz"
    if not bottle.exists():
        token = json.load(urllib.request.urlopen("https://ghcr.io/token?service=ghcr.io&scope=repository:homebrew/core/gcc/14:pull", timeout=30))["token"]
        request = urllib.request.Request("https://ghcr.io/v2/homebrew/core/gcc/14/blobs/sha256:" + SHA, headers={"Authorization": "Bearer " + token})
        pending = cache / "gcc14-sequoia.partial"
        with urllib.request.urlopen(request, timeout=120) as response, pending.open("wb") as output:
            shutil.copyfileobj(response, output)
        del token, request
        pending.replace(bottle)
    with bottle.open("rb") as stream:
        if hashlib.file_digest(stream, "sha256").hexdigest() != SHA:
            raise ValueError("GCC runtime bottle checksum mismatch")
    names = {*LIBRARIES, "COPYING.RUNTIME"}
    found = set()
    with tarfile.open(bottle) as archive:
        for entry in archive:
            name = Path(entry.name).name
            if entry.isfile() and name in names:
                if name in found:
                    raise ValueError("Duplicate runtime file")
                (destination / name).write_bytes(archive.extractfile(entry).read())
                found.add(name)
    if found != names:
        raise ValueError("Incomplete runtime bottle")
    license_file = cache / "COPYING.LIB"
    if not license_file.exists():
        license_file.write_bytes(urllib.request.urlopen("https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-14.3.0/libquadmath/COPYING.LIB", timeout=30).read())
    if hashlib.sha256(license_file.read_bytes()).hexdigest() != LGPL_SHA:
        raise ValueError("Runtime license checksum mismatch")
    shutil.copyfile(license_file, destination / "COPYING.LIB")
    for name in LIBRARIES:
        library = destination / name
        version = subprocess.check_output(["otool", "-l", str(library)], text=True)
        minimum = re.findall(r"\bminos (\d+)\.(\d+)", version)
        if not minimum or any(tuple(map(int, value)) > (15, 0) for value in minimum):
            raise ValueError("Runtime needs newer macOS")
        library.chmod(0o755)
        subprocess.run(["install_name_tool", "-id", "@rpath/" + name, str(library)], check=True)
        dependencies = subprocess.check_output(["otool", "-L", str(library)], text=True).splitlines()[2:]
        for dependency in dependencies:
            path = dependency.strip().split(" (")[0]
            if Path(path).name in LIBRARIES:
                subprocess.run(["install_name_tool", "-change", path, "@loader_path/" + Path(path).name, str(library)], check=True)
        subprocess.run(["codesign", "--force", "--sign", "-", str(library)], check=True, capture_output=True)
    (destination / "mode-runtime.json").write_text(json.dumps({"formula_commit": COMMIT, "bottle_sha256": SHA, "bottle_tag": "arm64_sequoia", "libraries": LIBRARIES}, indent=2) + "\n")


def stage(destination, cache):
    destination.mkdir(parents=True, exist_ok=True)
    # Never truncate a library that a running test helper may have mapped.
    with tempfile.TemporaryDirectory(prefix=".mode-runtime-stage-", dir=destination) as temporary:
        pending = Path(temporary)
        prepare(pending, cache)
        for name in (*LIBRARIES, "COPYING.RUNTIME", "COPYING.LIB", "mode-runtime.json"):
            target = destination / name
            if target.is_symlink():
                raise ValueError("Runtime destination is a symbolic link")
            if not target.exists() or target.read_bytes() != (pending / name).read_bytes():
                (pending / name).replace(target)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", type=Path, required=True)
    parser.add_argument("--cache", type=Path, required=True)
    args = parser.parse_args()
    stage(args.destination.resolve(), args.cache.resolve())
