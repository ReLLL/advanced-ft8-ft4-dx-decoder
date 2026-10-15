"""Run official-encoder fixtures through the isolated production helper protocol."""
import json
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def main():
    helper, generator = (Path(value).resolve() for value in sys.argv[1:])
    cases = [("FST4", 1, period, 0) for period in (15, 30, 60, 120, 300, 900, 1800)]
    cases += [("FST4W", 2, period, 0) for period in (120, 300, 900, 1800)]
    cases += [("Q65", 3, period, submode) for period, maximum in ((15, 2), (30, 3), (60, 4), (120, 4), (300, 4)) for submode in range(maximum + 1)]
    metrics = []
    with tempfile.TemporaryDirectory(prefix="dx-mode-fixture-") as temporary:
        root = Path(temporary)
        for index, (mode, number, period, submode) in enumerate(cases):
            with tempfile.TemporaryDirectory(dir=root) as scratch:
                audio = Path(scratch) / "fixture.pcm"
                subprocess.run([str(generator), str(number), str(period), str(submode), str(audio)], cwd=scratch, check=True, timeout=30, capture_output=True)
                pcm = audio.read_bytes()
                assert len(pcm) == period * 24000
                request = json.dumps({"id": index, "utc_ms": 1792051200000, "mode": mode, "period_ms": period * 1000, "submode": submode, "frequency": 14074000, "samples": len(pcm) // 2}).encode()
                started = time.monotonic()
                response = subprocess.run([str(helper)], input=struct.pack("<I", len(request)) + request + pcm, cwd=scratch, capture_output=True, timeout=120, check=True)
                length, = struct.unpack("<I", response.stdout[:4])
                assert len(response.stdout) == length + 4
                decoded = json.loads(response.stdout[4:])
                assert not decoded["error"], decoded
                assert any("K1ABC" in row["message"] for row in decoded["results"]), (mode, period, submode, decoded)
                assert all(0 <= row["audio_hz"] <= 4800 for row in decoded["results"])
                metrics.append({"mode": mode, "period": period, "submode": submode, "seconds": round(time.monotonic() - started, 3)})
                print(json.dumps(metrics[-1]), flush=True)
    assert not root.exists()
    print(json.dumps({"fixtures": len(metrics), "max_seconds": max(row["seconds"] for row in metrics), "test_audio_and_logs_removed": True}), flush=True)


if __name__ == "__main__":
    main()
