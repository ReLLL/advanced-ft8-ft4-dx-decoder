"""Own all synthetic reception stores and delete them after the pipeline test exits."""
from pathlib import Path
import array
import math
import subprocess
import sys
import tempfile
import sqlite3
import csv


def main():
    test, helper, generator, simulator = (Path(value).resolve() for value in sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix="dx-mode-pipeline-") as temporary:
        root = Path(temporary)
        for mode, number, period in (("FST4", 1, 15), ("FST4W", 2, 120), ("Q65", 3, 15)):
            subprocess.run([str(generator), str(number), str(period), "0", str(root / (mode + ".pcm"))], cwd=root, check=True, timeout=30, capture_output=True)
        generated = subprocess.run([str(simulator), "-c", "K1ABC FN42 33"], cwd=root, capture_output=True, text=True, timeout=10)
        assert generated.returncode == 1
        symbols = [int(value) for value in generated.stdout.split("Channel symbols:\n", 1)[1].split()]
        assert len(symbols) == 162
        pcm = array.array("h", [0]) * 1440000
        phase = 0.0
        for index, symbol in enumerate(symbols):
            step = 2 * math.pi * (1500 + (symbol - 1.5) * 12000 / 8192) / 12000
            for offset in range(8192):
                pcm[24000 + index * 8192 + offset] = round(4000 * math.sin(phase))
                phase += step
            phase %= 2 * math.pi
        if sys.byteorder != "little":
            pcm.byteswap()
        (root / "WSPR.pcm").write_bytes(pcm.tobytes())
        subprocess.run([str(test), str(helper), str(root)], cwd=root, check=True, timeout=240)
        with sqlite3.connect(root / "saved/history.sqlite3") as database:
            records = database.execute("SELECT rowid,mode,call FROM decodes ORDER BY rowid").fetchall()
        with (root / "saved/receptions.csv").open(newline="") as stream:
            logged = list(csv.DictReader(stream))
        assert len(records) == len(logged) == 4
        assert {row[1] for row in records} == {"FST4", "FST4W", "Q65", "WSPR"}
        assert all(row[2] == "K1ABC" for row in records)
    assert not root.exists()
    print("Synthetic audio, SQLite, CSV and decoder scratch removed")


if __name__ == "__main__":
    main()
