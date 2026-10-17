"""Official-encoder JS8 waveforms; all audio and scratch files are disposable."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import random
import array
import time
import wave


def read_exact(stream, length):
    result = bytearray()
    while len(result) < length:
        block = stream.read(length - len(result))
        if not block:
            raise RuntimeError("JS8 helper exited")
        result += block
    return result


def request(worker, number, period, pcm, utc):
    data = json.dumps({"id": number, "mode": "JS8", "period_ms": period * 1000,
                       "samples": len(pcm) // 2, "utc_ms": utc * 1000,
                       "frequency": 7078000}).encode()
    worker.stdin.write(struct.pack("<I", len(data)) + data + pcm)
    worker.stdin.flush()
    size, = struct.unpack("<I", read_exact(worker.stdout, 4))
    assert 0 < size <= 1024 * 1024
    result = json.loads(read_exact(worker.stdout, size))
    assert result["id"] == number
    return result["results"]


def main():
    helper, reference = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix="dx-js8-reference-") as folder:
        worker = subprocess.Popen([helper], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=folder)
        try:
            number = 0
            case = 0
            elapsed = []
            rss_kb = 0
            for period in (6, 10, 15, 30):
                for call, text in (("K1ABC", "CQ CQ CQ FN42"), ("K1ABC/P", "W9XYZ HELLO THIS IS A PORTABLE STATION TESTING JS8 RECEPTION")):
                    encoded = subprocess.check_output([reference, str(period), call, "FN42", text], cwd=folder)
                    size, = struct.unpack("<I", encoded[:4]);metadata = json.loads(encoded[4:4 + size]);pcm = encoded[4 + size:]
                    block = period * 12000 * 2;assert len(pcm) == block * len(metadata["frames"])
                    rows = []
                    case += 1
                    utc = 1800000000 + case * 3000
                    for i in range(len(metadata["frames"])):
                        number += 1
                        started = time.monotonic()
                        rows += request(worker, number, period, pcm[i * block:(i + 1) * block], utc + i * period)
                        elapsed.append(time.monotonic() - started)
                    rss_kb = max(rss_kb, int(subprocess.check_output(["ps", "-o", "rss=", "-p", str(worker.pid)], text=True).strip()))
                    assert rows, (period, call, "no decodes")
                    complete = [row for row in rows if row["message_complete"]]
                    assert complete and call in complete[-1]["message"], (period, call, rows)
                    assert ("FN42" if text.startswith("CQ") else "HELLO THIS IS A PORTABLE STATION TESTING JS8 RECEPTION") in complete[-1]["message"], (period, rows)
                    assert abs(complete[-1]["audio_hz"] - 1500) < 5
                    print("PASS JS8", period, "s", call, "frames", len(metadata["frames"]), flush=True)
                number += 1
                assert not request(worker, number, period, bytes(period * 12000 * 2), utc + len(metadata["frames"]) * period)
                randomizer = random.Random(2026 + period)
                noise = array.array("h", (round(randomizer.gauss(0, 2000)) for _ in range(period * 12000)))
                number += 1
                assert not request(worker, number, period, noise.tobytes(), utc + (len(metadata["frames"]) + 1) * period)
            assert not list(Path(folder).iterdir()), "Decoder wrote unexpected files"
            recordings = Path(__file__).resolve().parents[1] / "third_party/js8call/media/tests"
            for index, (filename, period, minimum) in enumerate((("A_1_4.wav", 15, 4), ("E_2_1.wav", 30, 1))):
                with wave.open(str(recordings / filename)) as recording:
                    assert recording.getframerate() == 12000 and recording.getsampwidth() == 2 and recording.getnchannels() == 1
                    pcm = recording.readframes(recording.getnframes())
                number += 1
                rows = request(worker, number, period, pcm, 1800100020 + index * 300)
                assert len(rows) >= minimum, (filename, rows)
                if filename.startswith("E"):
                    assert "KN4CRD: TEST" in rows[0]["message"] and not rows[0]["message_complete"]
                print("PASS upstream recording", filename, "receptions", len(rows), flush=True)
            print(f"JS8 fixture decoding: max {max(elapsed):.2f} s per window; RSS {rss_kb / 1024:.1f} MiB", flush=True)
        finally:
            worker.stdin.close()
            try:
                worker.wait(timeout=10)
            except subprocess.TimeoutExpired:
                worker.kill();worker.wait();raise
            errors = worker.stderr.read().decode(errors="replace")
            assert worker.returncode == 0, errors


if __name__ == "__main__":
    main()
