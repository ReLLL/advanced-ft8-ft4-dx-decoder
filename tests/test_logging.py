import csv
import pathlib
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="dx-csv-check-") as folder:
    root = pathlib.Path(folder)
    fixture = root / "fixture.csv"
    subprocess.run([sys.argv[1], str(fixture)], check=True)
    with fixture.open(newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    assert len(rows) == 2
    assert rows[0]["UTC"] == "2027-01-15T08:00:00Z"
    assert rows[0]["Country"] == 'Country, "quoted"'
    assert rows[0]["Raw"] == 'first, line\r\nsecond "line"'
    assert rows[1]["Message"] == 'HELLO, "world"\r\nnext line'
    assert rows[0]["DTSeconds"] == "0.35"
    for name in ("limited.csv", "relocated.csv"):
        with (root / name).open(newline="", encoding="utf-8") as source:
            data = list(csv.DictReader(source))
        assert data and all(None not in row and len(row) == 28 for row in data)
        ids = [int(row["ReceptionId"]) for row in data]
        assert ids == sorted(set(ids)) and ids[-1] == 502
    print("Python CSV reader verified commas, quotes, CRLF and complete bounded records")
