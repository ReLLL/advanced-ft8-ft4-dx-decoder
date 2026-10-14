# Changelog

## 1.0.25

- Add a native macOS installer for both SDR++ and SDR++Brown, with automatic host selection, optional home locator, automatic first-run preparation, safe host quit, custom-profile selection, backup access and Open host. End users do not need Python, Terminal, Homebrew, developer tools or administrator access.
- Verify payload hashes and the selected host core before writing. Stage replacements, preserve settings/logs, keep timestamped backups and recover from handled write failures. Refuse configuration changes or a restarted host during installation.
- Include the installer automatically in release ZIP and DMG packaging, with Developer ID/notarization options and verification gates. Include installer source in the corresponding-source archive.
- Add a separate standard SDR++ build using standard APIs and ImGui hooks, UTC capture timing, independent location settings, bundled country-prefix data and separate reception storage. Keep Brown's native integration intact.
- Verify live FT8 reception in standard SDR++ 1.2.0 (May 28, 2024 build), including countries, distances, map, tables and notifications. Add installer safety checks for both hosts. Document Brown's Source > My Grid and standard SDR++'s plugin location setting.

## 1.0.24

- Show each new reception in the waterfall corner, including repeats. Expire notifications after an adjustable 10 seconds by default while preserving history and the farthest row. Keep the two-second arrival glow. No four-row cap.
- Show both trophy and country flag on the farthest row. Color only its Farthest label gold; use regular field colors for its data.
- Add flags before country names in all tables. Reformat station details with colored fields and a reception-history table.
- Double-click a Countries heard row to open all matching receptions with sender, target, signal, distance, grid, message and source. Apply current filters and time/history scope; update asynchronously.
- Rename Approx. km to Distance. Add a saved kilometers/miles preference, default kilometers, across distance displays and filters. Preserve canonical kilometer logs and exports.
- Test notification expiry, repeats, ignored receptions, startup/history behavior, country detail filtering/paging and distance conversion.

## 1.0.23

- Make window close/open explicitly independent of receiver enablement. Show Receiving in background in the sidebar; reopening refreshes data and focuses the existing window without resetting its size or position.
- Add Unique stations, All messages and Countries heard tabs with shared filters. Keep repeats in All messages, newest first; show UTC and source alongside decoder fields. Save each table's layout and sort.
- Add country station counts, mean distance/SNR, latest five distinct senders and last-heard age. Average each sender's latest matching reception once; omit unknown distances. Country exports follow the selected tab.
- Add Corner notifications on waterfall, initially on: every heard country, native flags, distinct field colors and full italic messages. Changed rows pulse for two seconds; a fixed gold trophy row shows the farthest reception. Long lists and messages scroll without tuning the waterfall.
- Add country aggregation, repeat ordering, native flag/trophy and pulse regressions. Use unique recovery-test directories to avoid stale files when process IDs are reused.

## 1.0.22

- Keep country cells green for stations from the three newest countries as their messages update; repeats do not promote older countries. Gold retains priority.
- Rename Call to Sender. Show Target and Mod as the final two default columns; Mod displays the latest FT8/FT4 mode.
- Save column drag order, widths, visibility and sort in plugin settings across restarts. Every column can move.
- Add numeric ranges for all signal, distance, frequency, count and age columns, alongside Sender/Target/Mod and other text filters. Apply ranges to displayed rows and refresh age filters as time passes. Filter controls scroll within the screen.

## 1.0.21

- Show one table row per callsign across bands, modes and dial frequencies. Message, SNR and reception fields use the latest matching decode; equal timestamps use arrival order. Keep every reception in the logs and station details.
- Enable Ignore transmitter for unresolved and malformed identities. With no transmitter identity, confirm the exact message to delete and ignore; preserve unrelated unidentified messages. Persist both ignore types and retry CSV cleanup after restart. Preserve quoted identifiers and messages during log cleanup.
- Add Look up addressed station to the shared table/map menu, with Google, QRZ and HamQTH choices for the target callsign.

## 1.0.20

- Limit green country markers to the three most recently discovered countries. Repeats and additional stations from an already heard country do not promote it; the fourth new country replaces the oldest marker. Gold keeps priority.
- Add Use saved history for everything above the views and in Settings > History. Restore retained statistics, map markers, table rows and first-heard times in the background, with existing filters and sorting. Starts off; saves the preference. Uptime stays session-based.
- Clear with Reset stats returns to session mode and zeroed totals. Clearing without Reset stats hides selected views without removing retained records. Update the legend, tooltips and help.

## 1.0.19

- Map markers and table rows share the same callsign lookup, copying, favorite and ignore menu.
- Ignore transmitter asks for confirmation, removes all of that callsign's receptions from views, statistics, saved history and known reception CSV logs, and blocks future receptions. Cleanup runs in the background and retries failed log writes after restart. Unignore allows future receptions; it cannot restore deleted history.
- Grouped station rows display the latest received message, with arrival order breaking equal UTC timestamps. First-heard timestamps retain their documented meaning.
- Verify RK9UE's NO35 distance from KM69 against the ARRL grid calculator: approximately 4,296 km. Expose station coordinates and the home grid in map tooltips. Recalculate saved distance and bearing values in the background when the home grid changes.

## 1.0.18

First public release of Advanced FT8/FT4 DX Decoder for the tested macOS Apple Silicon SDR++Brown build.

- Native FT8/FT4 decoding, reception statistics, map, sortable message table, help and organized settings.
- Clear visible views and fresh session statistics at startup, with persistent SQLite and CSV logs.
- Optional saved-history statistics, initially off, calculated in the background.
- Station first heard before Msg age, newest station first by default; right-aligned age/first-heard values.
- Germany country-name normalization, cached-distance asterisk before the value, table color legend and right-edge Close details button.
- Visible map/table splitter, country highlights, day/night overlay, bounded tile caching and dark-blue animated dotted exchange arcs.
- Last five distinct station messages in map hover, callsign website lookups, quiet alert defaults and individual sounds.
- Optional direct GridTracker UDP feed, initially off, with local defaults ready.
- Decoder helper isolation, FFT plan lifetime corrections, persistent CSV recovery and test coverage for session/history switching.
