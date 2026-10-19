"""Bundle matching MSYS2 runtime sources for a staged Windows plugin ZIP."""
import argparse
import concurrent.futures
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import urllib.request
import zipfile


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--binary-cache", type=Path, required=True)
    parser.add_argument("--cache", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    with zipfile.ZipFile(args.package) as archive:
        packages = json.loads(archive.read("runtime-packages.json"))
        needed = {Path(n).name for n in archive.namelist()
                  if n.startswith("AdvancedDx/AdvancedDx/") and n.endswith(".dll")}
    owners = []
    for name, package in packages.items():
        path = args.binary_cache / package["filename"]
        if digest(path) != package["sha256"]:
            raise ValueError("Binary package checksum mismatch: " + path.name)
        files = subprocess.check_output(["tar", "-tf", str(path)], text=True).splitlines()
        found = sorted(needed.intersection(Path(f).name for f in files if "/bin/" in f))
        if not found:
            continue
        raw = subprocess.check_output(["tar", "-xOf", str(path), ".PKGINFO"], text=True)
        info = dict(line.split(" = ", 1) for line in raw.splitlines() if " = " in line)
        source = info["pkgbase"] + "-" + info["pkgver"] + ".src.tar.zst"
        owners.append({"package": name, "version": info["pkgver"], "dlls": found,
                       "source": source, "binary_sha256": package["sha256"]})
        needed.difference_update(found)
    if needed:
        raise ValueError("No source owner for runtime DLLs: " + ", ".join(sorted(needed)))
    args.cache.mkdir(parents=True, exist_ok=True)

    def fetch(name):
        path = args.cache / name
        url = "https://repo.msys2.org/mingw/sources/" + name
        if not path.exists():
            temporary = path.with_suffix(".partial")
            with urllib.request.urlopen(url, timeout=120) as response, temporary.open("wb") as output:
                while block := response.read(1024 * 1024):
                    output.write(block)
            temporary.replace(path)
        return {"filename": name, "url": url, "sha256": digest(path), "bytes": path.stat().st_size}

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        sources = list(pool.map(fetch, sorted({owner["source"] for owner in owners})))
    # Bound decompression separately from downloads: compiler/Qt source bundles
    # expand substantially and should not compete for memory in parallel.
    for source in sources:
        path = args.cache / source["filename"]
        files = subprocess.check_output(["tar", "-tf", str(path)], text=True).splitlines()
        if not any(Path(f).name == "PKGBUILD" for f in files):
            raise ValueError("Source archive has no build recipe: " + path.name)
    metadata = args.cache / "runtime-sources.json"
    metadata.write_text(json.dumps({"owners": owners, "sources": sources}, indent=2) + "\n")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    temporary = args.output.with_suffix(".partial")
    with tarfile.open(temporary, "w") as bundle:
        for path in [metadata, *[args.cache / source["filename"] for source in sources]]:
            info = bundle.gettarinfo(str(path), arcname="windows-runtime-source/" + path.name)
            info.uid = info.gid = 0
            info.uname = info.gname = ""
            info.pax_headers = {}
            with path.open("rb") as stream:
                bundle.addfile(info, stream)
    temporary.replace(args.output)
    print(json.dumps({"runtime_source_archives": len(sources), "runtime_packages": len(owners),
                      "sha256": digest(args.output), "bytes": args.output.stat().st_size}))


if __name__ == "__main__":
    main()
