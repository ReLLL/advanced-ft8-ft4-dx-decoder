"""Package the plugin and matching source without profile data or build intermediates."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PUBLIC_FILES = ("README.md", "LICENSE", "THIRD_PARTY_NOTICES.md", "CHANGELOG.md", "CMakeLists.txt", ".gitignore")
SOURCE_DIRS = ("src", "scripts", "tests", "docs", "third_party/earcut")


def anonymous_owner(info):
    info.uid = info.gid = 0
    info.uname = info.gname = ""
    info.pax_headers = {}
    return info


def source_files():
    for name in PUBLIC_FILES:
        yield ROOT / name
    yield ROOT / "resources/world.geojson"
    for name in SOURCE_DIRS:
        for path in sorted((ROOT / name).rglob("*")):
            if path.is_file() and "__pycache__" not in path.parts:
                yield path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build-release")
    args = parser.parse_args()
    version = re.search(r"VERSION (\d+\.\d+\.\d+)", (ROOT / "CMakeLists.txt").read_text()).group(1)
    name = "advanced-ft8-ft4-dx-decoder-" + version
    output = ROOT / "dist"
    output.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="dx-release-") as temporary:
        package = Path(temporary) / name
        for relative in ("README.md", "LICENSE", "THIRD_PARTY_NOTICES.md", "CHANGELOG.md", "scripts/install.py", "resources/world.geojson", "third_party/earcut/LICENSE", "docs/INSTALL.md", "docs/USER_GUIDE.md"):
            target = package / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, target)
        for binary in ("dx_notebook.dylib", "dx_decoder"):
            target = package / "bin" / binary
            target.parent.mkdir(exist_ok=True)
            shutil.copyfile(args.build / binary, target)
            target.chmod(0o755)
            subprocess.run(["strip", "-S", str(target)], check=True)
            libraries = subprocess.check_output(["otool", "-L", str(target)], text=True)
            if "/opt/homebrew/" in libraries or "/Users/" in libraries:
                raise ValueError("Nonportable binary dependency")
            strings = subprocess.check_output(["strings", str(target)])
            if b"/Users/" in strings:
                raise ValueError("Local user path in release binary")
            subprocess.run(["codesign", "--force", "--sign", "-", str(target)], check=True)
            subprocess.run(["codesign", "--verify", "--strict", str(target)], check=True)
        archive = output / (name + "-macos-arm64.zip")
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as bundle:
            for path in sorted(package.rglob("*")):
                if path.is_file():
                    bundle.write(path, path.relative_to(package.parent))
    source = output / (name + "-corresponding-source.tar.gz")
    with tarfile.open(source, "w:gz") as bundle:
        for path in source_files():
            bundle.add(path, arcname=Path(name) / path.relative_to(ROOT), filter=anonymous_owner)
        staged = ROOT / "build"
        for relative in ("license", "source-pin.txt", "core", "decoder_modules/ft8_decoder", "decoder_modules/radio"):
            bundle.add(staged / relative, arcname=Path(name) / "build" / relative, filter=anonymous_owner)
    checksums = output / (name + "-SHA256SUMS.txt")
    checksums.write_text("".join(hashlib.sha256(path.read_bytes()).hexdigest() + "  " + path.name + "\n" for path in (archive, source)))
    print("Release package, corresponding source and SHA-256 checksums:")
    for path in (archive, source, checksums):
        print(path.name)


if __name__ == "__main__":
    main()
