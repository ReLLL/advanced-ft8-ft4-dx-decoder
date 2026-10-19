"""Windows functional fixtures. Every synthetic reception stays in a disposable folder."""
import json
import array
import math
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def request(binary, mode, pcm, folder, period):
    header = json.dumps({"id": 1, "mode": mode, "utc_ms": 1800000000000,
                         "period_ms": period * 1000, "submode": 0,
                         "frequency": 14074000, "samples": len(pcm) // 2}).encode()
    started = time.monotonic()
    result = subprocess.run([str(binary)], input=struct.pack("<I", len(header)) + header + pcm,
                            cwd=folder, capture_output=True, timeout=20 if mode == "JS8" else 120, check=True)
    size, = struct.unpack("<I", result.stdout[:4])
    assert size == len(result.stdout) - 4
    decoded = json.loads(result.stdout[4:])
    assert not decoded.get("error"), decoded
    assert any("K1ABC" in row["message"] for row in decoded["results"]), (mode, decoded)
    print("PASS", mode, period, "seconds; decode", round(time.monotonic()-started,3), "s", flush=True)


def main():
    build = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="dx-windows-decoders-") as temp:
        folder = Path(temp)
        for mode, period in (("FT8", 15), ("FT4", 7.5)):
            pcm = folder / "fixture.pcm"
            subprocess.run([str(build / "dx_fixture.exe"), mode, str(pcm)], cwd=folder, check=True, timeout=30)
            request(build / "dx_decoder.exe", mode, pcm.read_bytes(), folder, period)
        for mode, number, period in (("FST4", 1, 15), ("FST4W", 2, 120), ("Q65", 3, 30)):
            pcm = folder / "fixture.pcm"
            subprocess.run([str(build / "dx_native_reference.exe"), str(number), str(period), "0", str(pcm)], cwd=folder, check=True, timeout=60)
            request(build / "dx_modes_decoder.exe", mode, pcm.read_bytes(), folder, period)
        signal = folder / "261019_1200.c2"
        simulator = subprocess.run([str(build / "dx_wspr_simulator.exe"), "-s", "-15", "-o", str(signal), "K1ABC FN42 33"], cwd=folder, capture_output=True, timeout=30)
        assert simulator.returncode == 1 and signal.exists()
        result = subprocess.run([str(build / "dx_wspr_decoder.exe"), "-a", str(folder), str(signal)], cwd=folder, check=True, capture_output=True, timeout=60)
        assert b"K1ABC FN42 33" in result.stdout, result.stdout
        print("PASS WSPR", flush=True)
        symbols = subprocess.run([str(build / "dx_wspr_simulator.exe"), "-c", "K1ABC FN42 33"], cwd=folder, capture_output=True, text=True, timeout=30)
        assert symbols.returncode == 1
        tones = [int(value) for value in symbols.stdout.split("Channel symbols:\n", 1)[1].split()]
        assert len(tones) == 162
        audio = array.array("h", [0]) * 1440000
        phase = 0.0
        for index, tone in enumerate(tones):
            step = 2 * math.pi * (1500 + (tone - 1.5) * 12000 / 8192) / 12000
            for offset in range(8192):
                audio[24000 + index * 8192 + offset] = round(4000 * math.sin(phase))
                phase += step
            phase %= 2 * math.pi
        if sys.byteorder != "little":
            audio.byteswap()
        request(build / "dx_modes_decoder.exe", "WSPR", audio.tobytes(), folder, 120)
        for period in (6, 10, 15, 30):
            encoded = subprocess.check_output([str(build / "dx_js8_reference.exe"), str(period), "K1ABC", "FN42", "CQ CQ CQ FN42"], cwd=folder, timeout=60)
            size, = struct.unpack("<I", encoded[:4])
            metadata = json.loads(encoded[4:4+size])
            assert len(metadata["frames"]) == 1
            request(build / "dx_js8_decoder.exe", "JS8", encoded[4+size:], folder, period)
    assert not folder.exists()
    print("All seven modes passed; synthetic audio and reception logs removed", flush=True)


if __name__ == "__main__":
    main()
