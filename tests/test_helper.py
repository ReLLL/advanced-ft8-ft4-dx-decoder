"""Framed memory decoder checks; generated local messages only."""
import json
from pathlib import Path
import select
import struct
import subprocess
import tempfile
import time
import os

ROOT = Path(__file__).resolve().parents[1]
BUILD = Path(os.environ.get("DX_TEST_BUILD", ROOT / "build-native"))


def read_exact(pipe, length, timeout=20):
    output = b""
    deadline = time.monotonic() + timeout
    while len(output) < length:
        ready, _, _ = select.select([pipe], [], [], max(0, deadline - time.monotonic()))
        if not ready:
            raise TimeoutError("Decoder timeout")
        chunk = pipe.read(length - len(output))
        if not chunk:
            raise RuntimeError("Decoder exited")
        output += chunk
    return output


def main():
    timings = []
    with tempfile.TemporaryDirectory(prefix="dx-fixture-") as temp:
        fixtures = {}
        for mode in ("FT8", "FT4"):
            path = Path(temp) / (mode + ".pcm")
            subprocess.run([BUILD / "dx_fixture", mode, path], check=True)
            fixtures[mode] = path.read_bytes()
        process = subprocess.Popen([BUILD / "dx_decoder"], stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, bufsize=0)
        try:
            for identity, mode in enumerate(("FT8", "FT4") * 5):
                header = json.dumps({"id": identity, "mode": mode, "utc_ms": 1800000000000,
                                     "samples": len(fixtures[mode]) // 2}).encode()
                started = time.monotonic()
                process.stdin.write(struct.pack("<I", len(header)) + header + fixtures[mode])
                length, = struct.unpack("<I", read_exact(process.stdout, 4))
                assert 0 < length < 1024 * 1024
                response = json.loads(read_exact(process.stdout, length))
                assert response["id"] == identity
                messages = [r["message"].strip() for r in response["results"]]
                assert "CQ K1ABC FN42" in messages, (mode, response)
                assert response["malformed"] == 0
                timings.append((mode, time.monotonic() - started))
            process.stdin.close()
            assert process.wait(timeout=5) == 0
        finally:
            if process.poll() is None:
                process.kill()
            process.wait()
        pairs = [timings[i][1] + timings[i + 1][1] for i in range(0, len(timings), 2)]
        report = {"jobs": len(timings), "single_helper": True,
                  "combined_max_seconds": max(pairs), "timings_seconds": timings}
        print(json.dumps(report, indent=2))
        assert max(pairs) < 7.5, "Combined decoding exceeds FT4 interval"
        bad = subprocess.run([BUILD / "dx_decoder"], input=struct.pack("<I", 5000),
                             capture_output=True, timeout=5)
        assert bad.returncode == 2


if __name__ == "__main__":
    main()
