"""Stage a pinned JS8Call decoder/parser subset without changing the upstream checkout."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

PIN = "a7ff1be0b389d287fdc56e2ea0d06962aa68127d"
ROOT = Path(__file__).resolve().parents[1]
FILES = ("JS8.cpp", "JS8.hpp", "commons.h", "varicode.cpp", "varicode.h",
         "decodedtext.cpp", "decodedtext.h", "jsc.cpp", "jsc.h", "jsc_map.cpp",
         "jsc_list.cpp", "COPYING", "vendor/CRCpp/CRC.h", "media/tests/README",
         "media/tests/A_1_4.wav", "media/tests/E_2_1.wav")


def stage(source):
    if subprocess.check_output(["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip() != PIN:
        raise ValueError("JS8Call revision does not match the reviewed source")
    paths = [source / name for name in FILES] + sorted((source / "vendor/Eigen").rglob("*"))
    paths = [path for path in paths if path.is_file()]
    destination = ROOT / "third_party/js8call"
    with tempfile.TemporaryDirectory(prefix="dx-js8-source-") as temporary:
        pending = Path(temporary)
        manifest = {}
        for path in paths:
            name = path.relative_to(source)
            target = pending / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)
            manifest[str(name)] = hashlib.sha256(target.read_bytes()).hexdigest()
        for name in manifest:
            target = destination / name
            if target.exists() and target.read_bytes() != (pending / name).read_bytes():
                raise ValueError("Refusing to overwrite a modified staged source: " + name)
        for name in manifest:
            target = destination / name
            target.parent.mkdir(parents=True, exist_ok=True)
            if not target.exists():
                shutil.copyfile(pending / name, target)
        target = destination / "source-pin.json"
        contents = json.dumps({"repository": "https://github.com/js8call/js8call", "commit": PIN, "files": manifest}, indent=2) + "\n"
        if not target.exists() or target.read_text() != contents:
            target.write_text(contents)


def verify():
    source = ROOT / "third_party/js8call"
    manifest = json.loads((source / "source-pin.json").read_text())
    if manifest["commit"] != PIN:
        raise ValueError("Unexpected JS8Call source revision")
    for name, digest in manifest["files"].items():
        path = source / name
        if path.is_symlink() or not path.resolve().is_relative_to(source.resolve()) or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
            raise ValueError("JS8Call source integrity check failed: " + name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    if args.verify:
        verify()
    elif args.source:
        stage(args.source.resolve())
    else:
        parser.error("Choose --source or --verify")
