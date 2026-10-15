"""Stage the pinned, unmodified WSJT-X receive-mode dependency closure."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from stage_wspr import PIN, ROOT

FORTRAN = ('lib/77bit/packjt77.f90', 'lib/ana64.f90', 'lib/blanker.f90', 'lib/ccf2.f90', 'lib/chkcall.f90', 'lib/crc.f90', 'lib/db.f90', 'lib/deg2grid.f90', 'lib/determ.f90', 'lib/fftw3mod.f90', 'lib/fmtmsg.f90', 'lib/four2a.f90', 'lib/fst4/decode240_101.f90', 'lib/fst4/decode240_74.f90', 'lib/fst4/encode240_101.f90', 'lib/fst4/encode240_74.f90', 'lib/fst4/fastosd240_74.f90', 'lib/fst4/fst4_baseline.f90', 'lib/fst4/gen_fst4wave.f90', 'lib/fst4/genfst4.f90', 'lib/fst4/get_crc24.f90', 'lib/fst4/get_fst4_bitmetrics.f90', 'lib/fst4/osd240_101.f90', 'lib/fst4_decode.f90', 'lib/ft2/gfsk_pulse.f90', 'lib/ft8/decode174_91.f90', 'lib/ft8/encode174_91_nocrc.f90', 'lib/ft8/ft8_downsample.f90', 'lib/ft8/ft8apset.f90', 'lib/ft8/ft8b.f90', 'lib/ft8/gen_ft8wave.f90', 'lib/ft8/get_crc14.f90', 'lib/ft8/osd174_91.f90', 'lib/ft8/subtractft8.f90', 'lib/ft8/sync8d.f90', 'lib/ft8/twkfreq1.f90', 'lib/grid2deg.f90', 'lib/indexx.f90', 'lib/packjt.f90', 'lib/pctile.f90', 'lib/peakup.f90', 'lib/platanh.f90', 'lib/polyfit.f90', 'lib/prog_args.f90', 'lib/q65_decode.f90', 'lib/qra/q65/genq65.f90', 'lib/qra/q65/q65.f90', 'lib/qra/q65/q65_ap.f90', 'lib/qra/q65/q65_loops.f90', 'lib/qra/q65/q65_set_list.f90', 'lib/qra/q65/q65_set_list2.f90', 'lib/sec0.f90', 'lib/shell.f90', 'lib/smo121.f90', 'lib/spec64.f90', 'lib/timer_module.f90', 'lib/twkfreq.f90', 'lib/types.f90')
FORTRAN += ("lib/ft8/genft8.f90", "lib/ft8/encode174_91.f90")
CODECS = tuple("lib/qra/q65/" + name for name in ("npfwht.c", "pdmath.c", "qracodes.c", "normrnd.c", "qra15_65_64_irr_e23.c", "q65.c", "q65_subs.c"))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    args = parser.parse_args()
    listing = subprocess.check_output(["git", "-C", str(args.source), "ls-tree", "-r", "--name-only", PIN], text=True).splitlines()
    names = set(FORTRAN) | set(CODECS) | {"lib/crc10.cpp", "lib/crc13.cpp", "lib/crc14.cpp", "COPYING", "doc/common/license.adoc"}
    names.update(name for name in listing if name.startswith("lib/qra/q65/") and name.endswith(".h"))
    payload = {}
    while names:
        name = names.pop()
        if name in payload:
            continue
        data = subprocess.check_output(["git", "-C", str(args.source), "show", PIN + ":" + name])
        payload[name] = data
        for include in re.findall(r"^\s*(?:include\s*|#\s*include\s*)['\"]([^'\"]+)['\"]", data.decode(errors="replace"), re.I | re.M):
            choices = [str(Path(name).parent / include), "lib/" + include, "lib/fst4/" + include]
            match = next((value for value in choices if value in listing), None)
            if match is None:
                raise ValueError("Missing pinned include: " + include)
            names.add(match)
    destination = ROOT / "third_party/wsjtx"
    for name, data in payload.items():
        path = destination / name
        if path.exists() and (path.is_symlink() or path.read_bytes() != data):
            raise ValueError("Differing staged source: " + name)
    for name, data in payload.items():
        path = destination / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    manifest = {"repository": "https://github.com/WSJTX/wsjtx", "commit": PIN,
                "fortran": FORTRAN, "codecs": CODECS,
                "files": {name: hashlib.sha256(data).hexdigest() for name, data in sorted(payload.items())}}
    (destination / "mode-source-pin.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (destination / "mode-sources.cmake").write_text("set(DX_MODE_FORTRAN\n" + "".join(' \"${WSJT_SOURCE}/' + name + '\"\n' for name in FORTRAN) + ")\n")
    print("Pinned native mode sources staged:", len(payload))

if __name__ == "__main__":
    main()
