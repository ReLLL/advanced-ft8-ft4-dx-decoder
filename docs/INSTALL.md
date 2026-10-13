# Installation

Read when: installing, upgrading, removing or restoring the plugin.

## Prebuilt package

Requirements: Apple Silicon, macOS 15+, Python 3, and the matching SDR++Brown 1.2.1 August 4, 2026 build at `/Applications/SDR++Brown.app`. Tested on macOS 26. The plugin links to Brown's bundled libraries; Homebrew is not needed to run the prebuilt package.

1. Open Brown once so it creates its normal profile. Quit Brown before installation.
2. Extract the release ZIP. In Terminal, change to the extracted folder.
3. Run `python3 scripts/install.py --build bin`.
4. Start Brown and open **Advanced FT8/FT4 DX Decoder** from its sidebar.

The default profile is `~/Library/Application Support/sdrpp-brown/`. Installation copies the plugin, decoder helper and world geometry into `plugins/dx-notebook/` beneath that profile, then registers the module. A timestamped backup of the configuration and previous plugin is saved under `dx-notebook-backups/`.

The package contains ad-hoc signed plugin/helper binaries. It is not a notarized standalone application. macOS can require approval for downloaded executable code. Build from source if your security policy requires locally built binaries. The installer preserves Brown's signed bundle and its existing decoder/helper configuration.

## Custom profile or upgrade

Quit Brown, then run:

```sh
python3 scripts/install.py --build bin --profile '/path/to/brown/profile'
```

Upgrade with the same command. Preferences and reception logs are retained; each installation creates a new backup. Brown begins with clear visible views on its next launch.

## Remove or restore

Quit Brown. To restore the previous configuration, copy the chosen backup's `config.json` to the profile's `config.json`. If that backup contains `plugin/`, copy its three files back into `plugins/dx-notebook/` to restore the previous plugin build.

For a first installation, restoring the original configuration removes the registration. The now-unused plugin directory can be moved to Trash. Saved reception history and CSV remain separately in the data directory; remove them only if you intend to erase the logs.

## Troubleshooting

- **Installer says Brown is running:** quit the application and retry; Brown writes its configuration on exit.
- **No plugin window:** check the sidebar/module registration and ensure the host build matches the release ABI.
- **No messages:** start the SDR, check enabled modes and eligible dial frequencies, wait for complete UTC slots, and verify clock synchronization and audio levels.
- **Unknown distance:** the station has no recent received grid. Country lookup alone cannot establish distance.
- **Missing tiles:** check connectivity or select the offline map. Previously viewed cached tiles remain available within cache limits.
- **CSV error:** choose a writable path and a compatible CSV file; the logger refuses to overwrite an unrelated file header.

Use GitHub Issues for reproducible problems. Include versions and steps; do not upload personal profile folders or reception logs without reviewing their contents.
