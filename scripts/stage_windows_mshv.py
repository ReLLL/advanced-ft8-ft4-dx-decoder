"""Stage the pinned decoder for MinGW without modifying either macOS build."""
import argparse
from pathlib import Path


def stage(source: Path, destination: Path):
    for file in source.rglob('*'):
        if not file.is_file():
            continue
        relative = file.relative_to(source)
        data = file.read_bytes()
        if relative.as_posix() == 'ft8_etc/mshv_support.h':
            text = data.decode()
            before = '#ifdef _WIN32\n#include <stdlib.h>'
            if before not in text or '#include <complex.h>' not in text:
                raise ValueError('Pinned MSHV Windows compatibility block changed')
            text = text.replace('#include <complex.h>', '#include <complex>\nusing std::isnan;\nusing std::isinf;')
            text = text.replace(before, '#ifdef _WIN32\n#include <windows.h>\n#endif\n\n#if defined(_WIN32) && !defined(__MINGW32__)\n#include <stdlib.h>', 1)
            text = text.replace('#else\n#include <unistd.h>\n#endif', '#else\n#include <unistd.h>\n#include <pthread.h>\n#endif', 1)
            data = text.encode()
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists() or target.read_bytes() != data:
            target.write_bytes(data)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--destination', type=Path, required=True)
    args = parser.parse_args()
    stage(args.source, args.destination)
