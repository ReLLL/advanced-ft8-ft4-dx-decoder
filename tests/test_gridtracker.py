"""Compare the native wire encoding with the existing, tested Python bridge."""
import pathlib
import subprocess
import sys
import tempfile
import struct
from types import SimpleNamespace

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "gridtracker-bridge"))
import protocol

with tempfile.TemporaryDirectory(prefix="dx-udp-wire-") as folder:
    packet = pathlib.Path(folder) / "decode.bin"
    subprocess.run([sys.argv[1], str(packet)], check=True)
    native = packet.read_bytes()
    row = SimpleNamespace(timestamp=1800000040, mode="FT8", snr=-17, dt=.35,
                          audio_hz=1245, message="CQ K1ABC FN42")
    reference = protocol.decode(row)
    def payload(data):
        magic, schema, kind, length = struct.unpack(">IIII", data[:16])
        assert (magic, schema, kind) == (0xADBCCBDA, 2, 2)
        return data[16 + length:]
    assert payload(native) == payload(reference)
    print("Native decode packet matches the proven bridge byte for byte")
