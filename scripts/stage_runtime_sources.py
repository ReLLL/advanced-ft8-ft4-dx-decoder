"""Keep the bundled GCC runtime's complete source and build recipe beside release sources."""
import hashlib
from pathlib import Path
import urllib.request

SOURCES = {
    "gcc-14.3.0.tar.xz": ("https://ftp.gnu.org/gnu/gcc/gcc-14.3.0/gcc-14.3.0.tar.xz", "e0dc77297625631ac8e50fa92fffefe899a4eb702592da5c32ef04e2293aca3a"),
    "gcc-14.3.0-darwin.diff": ("https://raw.githubusercontent.com/Homebrew/formula-patches/473d292cbbdfd318341cb6d4bcdf8de47879bab7/gcc/gcc-14.3.0.diff", "b8611362ae43a5644ab908d6e4d9bfc90346a914c3ba851197086d54148b1289"),
}


def prepare(folder):
    folder.mkdir(parents=True, exist_ok=True)
    for name, (url, digest) in SOURCES.items():
        target = folder / name
        if not target.exists():
            pending = folder / (name + ".partial")
            with urllib.request.urlopen(url, timeout=120) as response, pending.open("wb") as output:
                while block := response.read(1024 * 1024):
                    output.write(block)
            pending.replace(target)
        with target.open("rb") as stream:
            if hashlib.file_digest(stream, "sha256").hexdigest() != digest:
                raise ValueError("Runtime source checksum mismatch: " + name)
    formula = folder / "gcc@14.rb"
    if not formula.exists():
        formula.write_bytes(urllib.request.urlopen("https://raw.githubusercontent.com/Homebrew/homebrew-core/fb9b5cc0b6f88cbd784aa327bfd13b0f7faa44f5/Formula/g/gcc@14.rb", timeout=30).read())


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    prepare(parser.parse_args().destination)
