# Advanced FT8/FT4 DX Decoder

**FT8 and FT4 decoding, a DX map, and reception logging inside SDR++Brown for macOS.** See the stations you hear, their countries, receiver SNR, approximate distances and recent exchanges without opening a separate mapper.

Created by [ReLLL](https://github.com/ReLLL). Free software under GPL-3.0-or-later.

## Download and install

[Download the latest release](https://github.com/ReLLL/advanced-ft8-ft4-dx-decoder/releases/latest). The prebuilt package is for **Apple Silicon Macs running macOS 15 or later**, with **SDR++Brown 1.2.1, August 4, 2026 build** installed at `/Applications/SDR++Brown.app`. Tested on macOS 26. Other Brown builds may require recompilation against their matching ABI. This is a Brown plugin; it requires the host application.

1. Install [SDR++Brown](https://sdrpp-brown.san.systems/), open it once, then quit it.
2. Download and extract the `macos-arm64.zip` release package.
3. Open Terminal in the extracted folder and run:

   ```sh
   python3 scripts/install.py --build bin
   ```

4. Start Brown. Open **Advanced FT8/FT4 DX Decoder** from the sidebar.
5. Set your home Maidenhead grid in Brown, start your SDR, and tune a capture covering an FT8 or FT4 dial frequency. Eligible presets show **Receiving**.

Python 3 is needed for installation. The installer backs up Brown's configuration and any previous plugin, then installs outside Brown's signed application. It preserves the existing decoder and map helper. [Installation and removal](docs/INSTALL.md).

## What you can see

- **At a glance:** unique countries, unique stations, farthest distance with country, active decoding uptime, stations heard in the last five minutes, and the latest five calls with country, distance and receiver SNR.
- **Flexible workspace:** resizable window, visible map/table splitter, sortable and resizable columns, map-only and table-only views. Newest **Station first heard** comes first by default; repeats retain the original first-heard time.
- **Message detail:** callsign, country, grid, band, FT8/FT4 mode, SNR, approximate distance, message text, Msg age, Msg first heard and Station first heard. Hover reveals UTC timestamps; right-click opens Google, QRZ or HamQTH.
- **DX map:** Esri street map by default, Humanitarian OpenStreetMap or bundled offline world map. Country names, day/night shading, home marker, optional faint country highlights, and dark-blue animated dotted arcs between addressed stations with received location evidence.
- **Focused reception:** separate or linked band/mode filters, CQ/general-call and two-way message filters, search, exclusions and time windows.
- **Clear visual feedback:** nine-second reception highlight, latest-batch map halos and green age values, green newly heard countries, and bold gold for the farthest station's country and distance.
- **Persistent logs:** UTF-8 CSV with UTC dates, callsigns, SNR, frequency, distance and message details. Commas, quotes and line breaks are escaped. Choose the file location and size limit in Settings.
- **Optional GridTracker feed:** direct local UDP using WSJT-X packets, initially off, with `127.0.0.1:2237` ready to enable. No separate bridge is required for this feed.
- **Quiet defaults:** alert sounds, map marker counts and saved-history statistics start off. Individual alert sounds are selectable.

Brown starts with empty visible views and fresh session statistics. **Use saved history for stats** optionally includes the retained reception database. Switching it runs in the background; uptime always measures this session's active decoding. Clear asks for confirmation and preserves saved logs.

## Read the numbers correctly

Distance uses the great-circle path between your home grid center and a received station grid center. `*` before a distance marks recent cached grid evidence. A missing grid stays unknown; a callsign prefix identifies a DXCC entity rather than an exact transmitter location.

**DX activity bearing** summarizes short-path bearings from home to the farthest recent stations. It describes observed reception activity, not measured arrival direction or a propagation forecast. An addressed message and an arc do not prove a completed contact.

[User guide](docs/USER_GUIDE.md) explains the columns, colors, settings, map cache, scientific limits and GridTracker setup. The same essentials are available in the plugin's **Help** dialog.

## Build and contribute

See [building and releasing](docs/RELEASING.md). The decoder uses pinned SDR++Brown/MSHV code, with FFT plan lifetime fixes. History uses SQLite, native dialogs and sound use AppKit, and polygon triangulation uses Mapbox Earcut. [Third-party notices](THIRD_PARTY_NOTICES.md).

Report bugs through [GitHub Issues](https://github.com/ReLLL/advanced-ft8-ft4-dx-decoder/issues). Include plugin version, Brown build, macOS version, reproduction steps and relevant messages. Review logs and screenshots for private information before sharing.
