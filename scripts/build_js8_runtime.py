"""Build a pinned static Qt Core, with bundled dependencies, for the isolated JS8 helper."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import urllib.request

VERSION = "6.8.2"
NAME = "qtbase-everywhere-src-" + VERSION
URL = "https://download.qt.io/archive/qt/6.8/6.8.2/submodules/" + NAME + ".tar.xz"
SHA = "012043ce6d411e6e8a91fdc4e05e6bedcfa10fcb1347d3c33908f7fdd10dfe05"


def prepare(root):
    root.mkdir(parents=True, exist_ok=True)
    archive = root / (NAME + ".tar.xz")
    if not archive.exists():
        pending = root / (NAME + ".partial")
        with urllib.request.urlopen(URL, timeout=120) as source, pending.open("wb") as target:
            while block := source.read(1024 * 1024):
                target.write(block)
        pending.replace(archive)
    with archive.open("rb") as stream:
        if hashlib.file_digest(stream, "sha256").hexdigest() != SHA:
            raise ValueError("Qt Core source checksum mismatch")
    prefix = root / "install"
    recipe = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    complete = root / "complete.json"
    if (prefix / "lib/cmake/Qt6/Qt6Config.cmake").exists() and complete.exists() and json.loads(complete.read_text()).get("recipe_sha256") == recipe:
        return prefix
    source = root / NAME
    if not source.exists():
        with tarfile.open(archive) as bundle:
            bundle.extractall(root, filter="data")
    build = root / "build"
    options = ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
               "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_SHARED_LIBS=OFF",
               "-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0", "-DCMAKE_INSTALL_PREFIX=/usr/local/lib/advanced-dx-js8",
               "-DQT_FEATURE_relocatable=ON",
               "-DQT_BUILD_EXAMPLES=OFF", "-DQT_BUILD_TESTS=OFF",
               "-DINPUT_doubleconversion=qt", "-DINPUT_pcre=qt", "-DINPUT_zlib=qt",
               # Qt 6.8's ARM yield intrinsic needs its declaration with newer Apple Clang.
               "-DCMAKE_C_FLAGS=-include arm_acle.h",
               "-DCMAKE_CXX_FLAGS=-include arm_acle.h",
               "-DCMAKE_OBJCXX_FLAGS=-include arm_acle.h -ffile-prefix-map=" + str(root) + "=.",
               "-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG -ffile-prefix-map=" + str(root) + "=."]
    for feature in ["gui", "network", "dbus", "sql", "testlib", "concurrent", "printsupport",
                    "opengl", "glib", "icu", "zstd", "libb2", "framework"]:
        options.append("-DFEATURE_" + feature + "=OFF")
    subprocess.run(options, check=True)
    subprocess.run(["cmake", "--build", str(build), "--parallel", "4"], check=True)
    subprocess.run(["cmake", "--install", str(build), "--prefix", str(prefix)], check=True)
    complete.write_text(json.dumps({"version": VERSION, "url": URL, "sha256": SHA, "recipe_sha256": recipe, "linkage": "static", "minimum_macos": "15.0"}, indent=2) + "\n")
    return prefix


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, default=Path(__file__).resolve().parents[1] / "build-js8-runtime")
    print(prepare(parser.parse_args().cache.resolve()))
