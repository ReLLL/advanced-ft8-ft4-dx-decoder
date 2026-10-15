"""Stage a pinned, unmodified WSJT-X WSPR reference decoder for isolated development."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

PIN = "967c85a6119110788b0df3782cf796485b2cee14"
ROOT = Path(__file__).resolve().parents[1]
FILES = ("COPYING", "lib/indexx.f90") + tuple("lib/wsprd/" + name for name in (
    "README", "wsprd.c", "wsprd_utils.c", "wsprd_utils.h", "wsprsim.c", "wsprsim_utils.c",
    "wsprsim_utils.h", "fano.c", "fano.h", "jelinek.c", "jelinek.h", "nhash.c", "nhash.h",
    "tab.c", "metric_tables.c", "osdwspr.f90"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    args = parser.parse_args()
    destination = ROOT / "third_party/wsjtx"
    payload = {name: subprocess.check_output(["git", "-C", str(args.source), "show", PIN + ":" + name]) for name in FILES}
    for name, data in payload.items():
        path = destination / name
        if path.exists() and (path.is_symlink() or path.read_bytes() != data):
            raise ValueError("Refusing to overwrite differing staged source: " + name)
    for name, data in payload.items():
        path = destination / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    manifest = {"repository": "https://github.com/WSJTX/wsjtx", "commit": PIN,
                "files": {name: hashlib.sha256(data).hexdigest() for name, data in payload.items()}}
    (destination / "source-pin.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("Pinned WSJT-X WSPR reference sources staged without changes")


if __name__ == "__main__":
    main()
