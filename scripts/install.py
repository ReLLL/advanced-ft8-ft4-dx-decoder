"""Install Advanced FT8/FT4/WSPR DX Decoder outside the host's signed app, with reversible backups."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from mode_payload import mode_files, NATIVE_BINARIES

ROOT = Path(__file__).resolve().parents[1]
FILES = ("dx_notebook.dylib", "dx_decoder", "world.geojson")
INSTANCE_NAME = "Advanced FT8/FT4/WSPR DX Decoder"
PREVIOUS_NAMES = ("Advanced FT8/FT4 DX Decoder", "DX Notebook")
MODULE_NAME = "advanced_ft8_ft4_dx_decoder"


def install(build, profile, host="brown"):
    config = profile / "config.json"
    data = json.loads(config.read_text())
    instances = data.setdefault("moduleInstances", {})
    existing = instances.get(INSTANCE_NAME)
    previous = [instances[name] for name in PREVIOUS_NAMES if name in instances]
    legacy = next(iter(previous), None)
    if existing and existing.get("module") not in (MODULE_NAME, "dx_notebook"):
        raise ValueError("Advanced FT8/FT4/WSPR DX Decoder name belongs to another module")
    if any(value.get("module") not in (MODULE_NAME, "dx_notebook") for value in previous):
        raise ValueError("Legacy DX Notebook name belongs to another module")
    target = profile / "plugins/dx-notebook"
    sources = {"dx_notebook.dylib": build / "dx_notebook.dylib",
               "dx_decoder": build / "dx_decoder", "world.geojson": ROOT / "resources/world.geojson"}
    sources.update({name: build / name for name in mode_files(build)})
    if host == "sdrpp":
        manifest = json.loads((build / "dx_notebook.host.json").read_text())
        if manifest.get("host") != "sdrpp":
            raise ValueError("Standard SDR++ requires its matching plugin build")
        sources["cty.dat"] = ROOT / "resources/cty.dat"
    if not all(p.is_file() for p in sources.values()):
        raise ValueError("Build or map resource missing")
    modules = data.setdefault("modules", [])
    plugin = str(target / "dx_notebook.dylib")
    if plugin not in modules:
        modules.append(plugin)
    instances[INSTANCE_NAME] = dict(existing or legacy or {})
    instances[INSTANCE_NAME].update({"module": MODULE_NAME, "enabled": (existing or legacy or {}).get("enabled", True)})
    for name in PREVIOUS_NAMES:
        instances.pop(name, None)
    menu = data.setdefault("menuElements", [])
    for item in menu:
        if item.get("name") in PREVIOUS_NAMES:
            item["name"] = INSTANCE_NAME
    seen = False
    renamed_menu = []
    for item in menu:
        if item.get("name") == INSTANCE_NAME:
            if seen:
                continue
            seen = True
        renamed_menu.append(item)
    if not seen:
        renamed_menu.insert(0, {"name": INSTANCE_NAME, "open": True})
    data["menuElements"] = renamed_menu
    backup = profile / "dx-notebook-backups" / datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%d-%H%M%S-%fZ")
    backup.mkdir(parents=True)
    shutil.copy2(config, backup / "config.json")
    if target.exists():
        shutil.copytree(target, backup / "plugin")
    target.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".dx-install-", dir=profile) as temporary:
        stage = Path(temporary)
        for name, source in sources.items():
            shutil.copy2(source, stage / name)
            assert hashlib.sha256(source.read_bytes()).digest() == hashlib.sha256((stage / name).read_bytes()).digest()
            if name == "dx_decoder" or name in NATIVE_BINARIES:
                (stage / name).chmod(0o755)
        staged_config = stage / "config.json"
        staged_config.write_text(json.dumps(data, indent=4) + "\n")
        staged_config.chmod(config.stat().st_mode & 0o777)
        for name in sources:
            os.replace(stage / name, target / name)
        os.replace(staged_config, config)
    return backup


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build-native")
    parser.add_argument("--host", choices=("brown", "sdrpp"), default="brown")
    parser.add_argument("--profile", type=Path)
    args = parser.parse_args()
    profile = args.profile or Path.home() / "Library/Application Support" / ("sdrpp-brown" if args.host == "brown" else "sdrpp")
    pattern = r"SDR\+\+Brown.app/Contents/MacOS/sdrpp" if args.host == "brown" else r"SDR\+\+.app/Contents/MacOS/sdrpp"
    probe = subprocess.run(["pgrep", "-f", pattern], capture_output=True)
    if probe.returncode == 0:
        parser.error("Quit the selected radio application before installation; it saves settings on exit")
    if probe.returncode != 1:
        parser.error("Unable to verify the radio application's process state")
    print("Installed candidate. Legacy decoding and bridge preserved.")
    print("Backup:", install(args.build.resolve(), profile.resolve(), args.host))


if __name__ == "__main__":
    main()
