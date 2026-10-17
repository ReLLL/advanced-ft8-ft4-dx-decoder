"""Prepare JS8 runtime provenance and retained notices beside the helper."""
import argparse
import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def stage(build, cache):
    runtime = json.loads((cache / "complete.json").read_text())
    source = cache / ("qtbase-everywhere-src-" + runtime["version"])
    js8 = ROOT / "third_party/js8call"
    pin = json.loads((js8 / "source-pin.json").read_text())
    js8_license = (js8 / "COPYING").read_text() + "\n\nEigen MPL-2.0\n\n" + (source / "LICENSES/MPL-2.0.txt").read_text()
    crc = (js8 / "vendor/CRCpp/CRC.h").read_text().split("*/", 1)[0] + "*/\n"
    qt_license = "\n\n".join(name.name + "\n\n" + name.read_text() for name in sorted((source / "LICENSES").glob("*.txt")))
    files = {"COPYING.JS8": js8_license + "\n\nCRC++\n\n" + crc,
             "COPYING.QT": qt_license,
             "js8-runtime.json": json.dumps({"js8_repository": pin["repository"], "js8_commit": pin["commit"], "qt": runtime}, indent=2) + "\n"}
    build.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".js8-payload-", dir=build) as temporary:
        for name, text in files.items():
            pending = Path(temporary) / name
            pending.write_text(text)
            target = build / name
            if target.is_symlink():
                raise ValueError("Payload destination is a symbolic link")
            if not target.exists() or target.read_bytes() != pending.read_bytes():
                pending.replace(target)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--cache", type=Path, required=True)
    args = parser.parse_args()
    stage(args.build.resolve(), args.cache.resolve())
