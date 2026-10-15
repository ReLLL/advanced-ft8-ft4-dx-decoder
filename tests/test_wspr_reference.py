"""Offline WSPR fixtures only; own and remove every generated reception and decoder log."""
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import json
import array
import math
import wave


def main():
    decoder, simulator = [str(Path(value).resolve()) for value in sys.argv[1:]]
    timings = []
    with tempfile.TemporaryDirectory(prefix="dx-wspr-fixture-") as temporary:
        folder = Path(temporary)
        for index, message in enumerate(("K1ABC FN42 33", "G4ABC IO91 23", "PJ4/K1ABC 30")):
            signal = folder / f"261015_{index * 2:04d}.c2"
            generated = subprocess.run([simulator, "-s", "-15", "-o", str(signal), message], cwd=folder,
                                       capture_output=True, timeout=10)
            # The upstream simulator returns 1 after successful generation.
            assert generated.returncode == 1 and signal.stat().st_size == 360026
            began = time.monotonic()
            result = subprocess.run([decoder, "-a", str(folder), str(signal)], cwd=folder,
                                    check=True, capture_output=True, text=True, timeout=20)
            elapsed = time.monotonic() - began
            assert message in result.stdout, result.stdout
            assert "<DecodeFinished>" in result.stdout
            timings.append(elapsed)
        # Exercise the real 12 kHz audio front end, beyond the complex baseband path.
        generated = subprocess.run([simulator, "-c", "K1ABC FN42 33"], cwd=folder,
                                   capture_output=True, text=True, timeout=10)
        assert generated.returncode == 1
        symbols = [int(value) for value in generated.stdout.split("Channel symbols:\n", 1)[1].split()]
        assert len(symbols) == 162 and all(0 <= symbol <= 3 for symbol in symbols)
        pcm = array.array("h", [0]) * 1440000
        phase = 0.0
        for index, symbol in enumerate(symbols):
            step = 2 * math.pi * (1500 + (symbol - 1.5) * 12000 / 8192) / 12000
            start = 24000 + index * 8192
            for offset in range(8192):
                pcm[start + offset] = round(4000 * math.sin(phase))
                phase += step
            phase %= 2 * math.pi
        if sys.byteorder != "little":
            pcm.byteswap()
        audio = folder / "261015_0010.wav"
        with wave.open(str(audio), "wb") as output:
            output.setnchannels(1)
            output.setsampwidth(2)
            output.setframerate(12000)
            output.writeframes(pcm.tobytes())
        began = time.monotonic()
        result = subprocess.run([decoder, "-a", str(folder), "-f", "14.0956", str(audio)], cwd=folder,
                                check=True, capture_output=True, text=True, timeout=20)
        assert "K1ABC FN42 33" in result.stdout and "14.097" in result.stdout, result.stdout
        timings.append(time.monotonic() - began)
        print(json.dumps({"known_messages": len(timings), "max_decode_seconds": max(timings),
                          "isolated": True, "recordings_and_logs_removed_on_exit": True}))
    assert not folder.exists()


if __name__ == "__main__":
    main()
