#!/usr/bin/env python3
"""Stage the x64 SDR# adapter and its isolated native workers. Never bundle SDR#."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
WINDOWS_OS_IMPORTS = {"authz", "mpr", "opengl32"}
SYSTEM = set("advapi32 bcrypt cfgmgr32 comctl32 comdlg32 crypt32 cryptbase cryptsp d3d9 dnsapi dwmapi dxgi gdi32 gdiplus imm32 iphlpapi kernel32 msvcrt ncrypt netapi32 normaliz ntdll ole32 oleaut32 powrprof psapi rpcrt4 secur32 setupapi shell32 shlwapi sspicli user32 userenv ucrtbase version winhttp winmm winspool wldap32 ws2_32 wtsapi32".split())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--adapter", type=Path, required=True)
    parser.add_argument("--runtime", type=Path, required=True, help="MSYS2 UCRT64 prefix")
    parser.add_argument("--objdump", required=True)
    parser.add_argument("--runtime-manifest", type=Path, required=True, help="Verified MSYS2 package versions and archive hashes")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        raise SystemExit("Output already exists; choose a new owned staging folder")
    output.mkdir(parents=True)
    plugin = output / "AdvancedDx"
    plugin.mkdir()
    native = plugin / "AdvancedDx"
    native.mkdir()
    shutil.copy2(args.adapter, plugin / "SDRSharp.AdvancedDx.dll")
    libraries = {p.name.lower(): p for p in (args.runtime / "bin").glob("*.dll")}
    pending = []
    for name in ("dx_sdrsharp", "dx_decoder", "dx_modes_decoder", "dx_wspr_decoder", "dx_js8_decoder"):
        source = args.build / "windows" / (name + ".exe")
        shutil.copy2(source, native / source.name)
        pending.append(native / source.name)
    copied = set()
    while pending:
        binary = pending.pop()
        imports = subprocess.check_output([args.objdump, "-p", str(binary)], text=True, encoding="utf-8", errors="replace")
        for name in re.findall(r"DLL Name:\s*(\S+)", imports):
            key = name.lower()
            if key.startswith(("api-ms-", "ext-ms-")) or key.removesuffix(".dll") in SYSTEM | WINDOWS_OS_IMPORTS:
                continue
            if key in copied:
                continue
            if key not in libraries:
                if os.name == "nt" and (Path(os.environ["SystemRoot"]) / "System32" / name).is_file():
                    continue
                raise RuntimeError(f"Missing runtime dependency: {name} required by {binary.name}")
            source = libraries[key]
            shutil.copy2(source, native / source.name)
            copied.add(key)
            pending.append(native / source.name)
    for name in ("world.geojson", "cty.dat", "cty-LICENSE.txt"):
        shutil.copy2(ROOT / "resources" / name, native / name)
    shutil.copytree(ROOT / "resources/windows-icons", native / "icons")
    notices = output / "Licenses"
    notices.mkdir()
    for name in ("LICENSE", "CREDITS.md", "THIRD_PARTY_NOTICES.md"):
        shutil.copy2(ROOT / name, notices / name)
    shutil.copytree(args.runtime / "share/licenses", notices / "MSYS2-runtime")
    shutil.copy2(args.runtime_manifest, output / "runtime-packages.json")
    shutil.copy2(ROOT / "windows/source-pin.json", output / "source-pin.json")
    for name in ("Install.cmd", "Install.ps1", "README.txt"):
        shutil.copy2(ROOT / "windows/installer" / name, output / name)
    private_paths = {str(Path.home()), os.environ.get("USERPROFILE", "")}
    private_markers = set()
    for prefix in private_paths:
        if len(prefix) < 4:
            continue
        for variant in (prefix, prefix.replace("\\", "/"), prefix.replace("/", "\\")):
            private_markers.update((variant.lower().encode("utf-8"), variant.lower().encode("utf-16le")))
    for binary in native.iterdir():
        if binary.suffix.lower() in (".exe", ".dll"):
            data = binary.read_bytes().lower()
            if any(marker in data for marker in private_markers):
                raise RuntimeError(f"Private build path embedded in {binary.name}; fix compiler path mapping before packaging")
    adapter_data = (plugin / "SDRSharp.AdvancedDx.dll").read_bytes().lower()
    if any(marker in adapter_data for marker in private_markers):
        raise RuntimeError("Private build path embedded in the managed adapter")
    files = {str(p.relative_to(output)).replace(os.sep, "/"): hashlib.sha256(p.read_bytes()).hexdigest()
             for p in sorted(output.rglob("*")) if p.is_file()}
    (output / "manifest.json").write_text(json.dumps({"platform": "windows-x64", "host": "SDR# .NET 9", "files": files}, indent=2), encoding="utf-8")
    archive = output.parent / (output.name + ".zip")
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as handle:
        for file in sorted(output.rglob("*")):
            if file.is_file():
                handle.write(file, str(file.relative_to(output)))
    print(json.dumps({"archive": str(archive), "sha256": hashlib.sha256(archive.read_bytes()).hexdigest(), "runtime_dlls": len(copied)}))


if __name__ == "__main__":
    main()
