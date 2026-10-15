"""Noise and malformed IPC checks, with no normal receiver files involved."""
from pathlib import Path
import array
import json
import random
import struct
import subprocess
import sys
import tempfile


def frame(header, pcm=b""):
    body = json.dumps(header).encode()
    return struct.pack("<I", len(body)) + body + pcm


def main():
    binary = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="dx-mode-noise-") as temporary:
        rng = random.Random(20261015)
        for mode, period in (("WSPR", 120), ("FST4", 15), ("FST4W", 120), ("Q65", 15)):
            pcm = array.array("h", (rng.randrange(-1000, 1001) for _ in range(period * 12000)))
            if sys.byteorder != "little":
                pcm.byteswap()
            request = {"id": 1, "utc_ms": 1792051200000, "mode": mode, "period_ms": period * 1000, "submode": 0, "frequency": 14095600, "samples": len(pcm)}
            result = subprocess.run([str(binary)], input=frame(request, pcm.tobytes()), cwd=temporary, capture_output=True, timeout=120, check=True)
            decoded = json.loads(result.stdout[4:])
            assert not decoded["results"] and not decoded["error"], (mode, decoded)
            print(mode, "fixed-seed noise: no decodes", flush=True)
        for mode, period, submode, count in (("unknown", 15000, 0, 180000), ("FST4W", 60000, 0, 720000), ("Q65", 0, 4, 180000), ("FST4", 15000, 0, -1)):
            result = subprocess.run([str(binary)], input=frame({"id": 2, "mode": mode, "period_ms": period, "submode": submode, "samples": count}), cwd=temporary, capture_output=True, timeout=5)
            assert result.returncode == 2
    assert not Path(temporary).exists()
    print("Invalid mode/period/spacing/count rejected; test files removed")


if __name__ == "__main__":
    main()
