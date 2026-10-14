# Installation

Read when: installing, upgrading, removing or restoring the plugin.

## Native installer

Requirements: Apple Silicon and macOS 15+. Supported hosts are **SDR++ 1.2.0, May 28, 2024 build**, at `/Applications/SDR++.app`, and **SDR++Brown 1.2.1, August 4, 2026 build**, at `/Applications/SDR++Brown.app`. Both were tested on macOS 26. The selected host supplies its matching runtime libraries. Standard SDR++ does not require Brown. Python, Homebrew and developer tools are not required for the native installer.

1. Install either supported radio application in Applications.
2. Open the release DMG, or extract the ZIP, and open **Install Advanced FT8-FT4 DX Decoder**.
3. The installer detects an installed host. If both are installed, select **SDR++** or **SDR++Brown**. Confirm the prefilled **Home locator**, or enter your own if known. It can also be set later.
4. Click **Install**. Missing first-run host settings are prepared automatically. A running host is asked to quit normally; the installer waits for its settings and never force-quits it.
5. Click **Open SDR++** or **Open SDR++Brown**, then open the plugin from the sidebar.

## Home location

The installer's optional **Home locator** field accepts the receiver's Maidenhead grid. Brown accepts four or six characters; standard SDR++ accepts four, six or eight. The installer saves it in the appropriate host setting. It preserves an existing locator if the field is left empty.

To change it later, use **Source > My Grid** in Brown's left sidebar, below **Operator Callsign**. Brown displays coordinates beside a valid grid and **Invalid** otherwise. For standard SDR++, use the plugin's **Settings > General > Maidenhead grid**. Standard SDR++ also has an optional receiver callsign in that settings page for the GridTracker context.

Use your actual receiving location. Distances and bearings use the center of the Maidenhead area, so a longer locator usually gives a closer estimate. A callsign-derived country does not provide an exact station position. With no valid home locator, standard SDR++ still decodes and maps received station grids, but does not invent a home marker, distance or bearing.

Installation is per user and requires no administrator password. Default profiles: `~/Library/Application Support/sdrpp-brown/` for Brown and `~/Library/Application Support/sdrpp/` for standard SDR++. The installer copies the plugin, helper and world geometry into `plugins/dx-notebook/` and registers the module. It verifies payload checksums and the selected host core library before writing; a different build is rejected rather than risking an ABI mismatch.

Settings and reception logs stay in place. A timestamped backup of the original configuration and previous plugin goes under `dx-notebook-backups/`. Copied files are verified before replacement, configuration replacement is atomic, and a handled installation failure restores the previous plugin/configuration. The installer also checks that the selected host has not restarted or changed the configuration before committing changes. Do not launch the selected host during installation.

## Custom profile or upgrade

Choose **Choose profile...** and select the selected host's profile folder containing `config.json`. Upgrade by running the same installer; it saves another backup and preserves existing preferences and logs, including a disabled plugin's enabled/disabled setting. The host begins with clear visible views on its next launch unless saved-history restoration is enabled.

For advanced users who already have Python 3, the previous command remains available in the extracted package:

```sh
python3 scripts/install.py --build bin --profile '/path/to/brown/profile'
```

Omit `--profile` for Brown's normal profile. For standard SDR++, use `python3 scripts/install.py --host sdrpp --build bin-sdrpp`; the matching host manifest and country data are included. Python is needed only for these optional commands and source-build tooling.

## Remove or restore

Quit the selected host. Copy a chosen backup's `config.json` to the profile's `config.json`. If the backup contains `plugin/`, copy its files back into `plugins/dx-notebook/` to restore the previous plugin build. The installer's **Show backup** button opens the backup for that installation.

For a first installation, restoring the original configuration removes the registration. The unused plugin directory can be moved to Trash. Reception history and CSV logs are stored separately; remove them only if you intend to erase those logs.

## Troubleshooting

- **First-run setup needed:** open the selected host once and return to the installer. Select a custom profile if you normally use one.
- **Unsupported host build:** use the specified host build or rebuild the plugin and installer against your matching host ABI. An installer cannot make an incompatible binary compatible.
- **The radio application could not quit:** quit it normally and click Retry; it may be waiting for a dialog. The installer never force-quits it.
- **File integrity check failed:** download and extract the complete package again.
- **No plugin window:** check the sidebar/module registration and matching host build.
- **No messages:** start the SDR, check modes and eligible dial frequencies, wait for complete UTC slots, and check clock synchronization and audio levels.
- **Unknown distance:** the station has no recent received grid. Country lookup alone cannot establish distance.
- **Missing tiles:** check connectivity or select the offline map. Viewed tiles remain available within cache limits.
- **CSV error:** choose a writable path and compatible CSV file; the logger refuses unrelated file headers.

Use GitHub Issues for reproducible problems. Include versions and steps; review logs and screenshots for private information before sharing.
