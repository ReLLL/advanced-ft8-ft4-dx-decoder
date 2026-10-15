# Changelog

## 1.0.33 (unreleased development)

- Add optional WSPR, FST4, FST4W and Q65 reception using pinned official WSJT-X decoder sources, with mode-specific periods and Q65 tone spacing in Reception settings. New modes start off; FT8/FT4 keep their existing controls and decoder.
- Use a separate bounded worker for additional modes, complete UTC captures, cancellation on retuning/stopping, bounded temporary files and bundled macOS 15-compatible Fortran runtimes. Q65 decodes individual sequences without cross-sequence averaging.
- Extend shared mode/Beacon filters, reception history, CSV logging and details, including reported beacon transmitter power separately from receiver SNR. Keep FT8/FT4 map circles; add squares, triangles, diamonds and hexagons for the new modes.
- Credit WSJT-X, the protocol designers and decoder authors in README, CREDITS.md and third-party notices; include credits in release downloads and the installer.

## 1.0.32

- Enable Color based on SNR by default for new or unset preferences. The checkbox remains optional and saves the selected preference.

## 1.0.31

- Check history size and count every 30 minutes, at startup and on policy changes. Default to 100,000 receptions and gradually trim to 95,000; custom limits retain 95%. Reception and CSV protection remain unchanged.

## 1.0.30

- Maintain latest sender/country summaries transactionally and reuse session statistics between arrivals. Read the latest messages through the time index.
- Limit SQLite history to 50,000 receptions by default, with resumable background deletion to 90% of the limit. Settings shows the retained date range and supports changing or disabling the limit. CSV rotation remains separate.
- Check the stored count and database size once a minute, at startup and on a policy change. Run short cleanup batches during continuous decoding, protect the CSV watermark, and update long-session stats incrementally as records expire.
- Preserve reception IDs, CSV catch-up boundaries and later arrivals during automatic cleanup. Reuse freed pages and use incremental compaction where enabled.
- Add isolated large-history benchmarks and regression coverage for cleanup rollback, restart, summaries and ID continuity.

## 1.0.29

- Extend sender and target callsign-hash matching from three minutes to twelve minutes. Accept only one distinct matching callsign; retain unresolved identifiers when candidates are missing or ambiguous. Exclude future receptions and candidates older than twelve minutes.
- Add isolated tests for the twelve-minute boundary, all three hash sizes, sender/target matching, collisions and expiry. All displayed station distances continue to use the receiver's home grid, independently of station-to-station communication arcs.

## 1.0.28

- Combine time, band, mode, message type, search and advanced filters in one responsive bar for the map and all table tabs. Retain the previous table selection on upgrade; wrap controls only when necessary and add Reset for both views.
- Limit exchange arcs to the host's visible RF spectrum, including zoom and pan. Hide stale arcs immediately when the viewport changes and choose recent exchanges from the new range.
- Show each map-tooltip message's receiver SNR after its text. Color time and message separately, and use the continuous red/yellow/green signal scale for SNR in map message history and details.
- Add a horizontal splitter above details and remember its height independently of the map/table split.
- Add the compass direction after the degree bearing in station details, using the short-path bearing from the home location.
- Add a compact compass beside DX ACTIVITY BEARING within the existing two-row statistic height. Its red/white needle follows the estimated bearing; mixed or unknown directions show no needle.
- Display reception dates in the computer's time zone throughout tables, details, map tooltips and copied rows. Include the UTC offset; elapsed ages and chronological sorting keep their original behavior.
- Interpret custom filters and saved-message cleanup dates as local times. Respect 23-hour and 25-hour calendar days; reject invalid or ambiguous clock-change times before cleanup.
- Preserve existing custom-range instants when upgrading or changing the computer's time zone. Add local date/time to CSV exports; keep reception log timestamps and GridTracker messages in UTC.

## 1.0.27

- Separate Ignore transmitter and Delete transmitter in every station context menu. Each confirmation offers one unchecked checkbox for the other operation. Ignore alone keeps saved receptions; Delete alone permits future receptions.
- Add Settings > Saved messages with independent, confirmed, one-off cleanup actions: before/after/between UTC dates, selected country, keep only selected country, missing distance, and missing grid. Preview database matches asynchronously; clean known CSV logs and SQLite, update views/statistics and retain receptions added after the preview.
- Compact SQLite after deletion in the background. Migrate reception IDs to an explicit integer primary key before VACUUM; preserve CSV/import watermarks and resume failed log cleanup after restart.
- Enable time-window selection with saved history, add clearly named Last hour/6 hours/24 hours/7 days/month presets and an All button to cancel the time filter independently in each view. Last month means a rolling 30 days.
- Use Date in map tooltips for UTC timestamps and Age for seconds ago.
- Require isolated test profiles and removal of test receptions after testing in project instructions; clean owned test files when test engines close.

## 1.0.26

- Show ages through 600 seconds as seconds followed by ago; older values show UTC date/time. Apply the same formatter to Unique stations, All messages, Countries heard and map age tooltips. Keep timestamp sorting and numeric age filters unchanged.
- Rename the top direction statistic to DX ACTIVITY BEARING and fix its reception window at the last 15 minutes, independently of map/table history windows. Keep the station count adjustable.
- Show Time correction on the queued/skipped row, including zero. Brown uses the actual applied capture-time offset; standard SDR++ reports no plugin offset.

- Remove installer restrictions on exact host versions/core fingerprints; retain host identity, required-library, payload and backup checks.
- Suggest an empty home grid with macOS Location Services, showing accuracy and asking the user to confirm. Keep existing/manual grids, support denied/unavailable location, and add a locator-map button and beginner README instructions.

- Add Color based on SNR beside the history checkbox, default off and remembered. Map-circle fills use a continuous red/yellow/green dB scale with a legend, separate selection outlines and latest-reception halos.
- Add sender and target callsign lookup to the details-row popup; retain the clicked reception while live messages update.
- Fix saved-history restoration during reception: rolling refreshes no longer discard completed loads. Rank compact reception IDs and index first-heard queries to reduce work. Keep live reception and logging active while history is enabled.
- Hide ignored transmitters immediately from map/table/details while the worker deletes their saved receptions and known CSV logs; preserve confirmation and retry failed log cleanup.
- Remove the home-to-selected-station gold path. Keep dark-blue communication arcs between addressed stations.

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
