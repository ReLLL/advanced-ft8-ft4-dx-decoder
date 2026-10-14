# User guide

Read when: setting up reception, understanding timestamps or distances, or configuring maps, logs and GridTracker.

## Start receiving

Set your own home Maidenhead grid in Brown. Distances and bearings originate at its center. Start the SDR and place a supported FT8 or FT4 dial inside the captured RF range. The plugin creates its own receiving channels for eligible presets; the ordinary Radio module can keep its selected mode. Check **Receiving**, level in dBFS, slot progress and decoded messages. **Time correction** follows queued/skipped counts on the same row, including zero. Brown shows the manual Source > Time correction offset applied to capture timestamps; standard SDR++ uses the Mac UTC clock with no plugin offset. This value is separate from a received signal's DT. Additional dial frequencies can be added in Settings > Reception.

FT8 slots last 15 seconds; FT4 slots last 7.5 seconds. A complete slot is needed after tuning or starting. Keep the computer clock synchronized. SNR is the decoder's estimate measured at this receiver; dBFS is the input audio level, a different measurement.

## Statistics and saved history

Countries and stations count unique received identities. Country names come from the transmitting callsign through Brown's local `cty.dat` DXCC database. Exact callsign exceptions take priority, then the longest matching prefix; portable operating prefixes can override a home callsign. FT8/FT4 does not transmit a separate country field. This identifies a callsign's DXCC entity and cannot establish exact transmitter coordinates. **Farthest** includes the country in parentheses. **UPTIME** shows active decoding time for this session, excluding stopped time. **Last 5 min** counts distinct recent callsigns; **Latest heard** shows the latest five distinct calls, country, approximate distance and SNR.

**Use saved history for stats** starts unchecked. Off counts only receptions received since this session started or session statistics were reset. On uses all retained database receptions, including imported history. The worker calculates the new values in the background and the previous values remain visible until ready. Uptime remains session-based. This preference is saved; turn it off to return to session statistics. It does not restore table/map entries or first-heard times.

**Use saved history for everything** also starts unchecked. Enable it to restore retained receptions into statistics, map markers and table rows together. First-heard columns use the first retained reception timestamps. Loading runs in the background; filters and table sorting still apply. Both time windows show All history while enabled, and uptime remains this session's active decoding time. The preference is saved, so enabling it also restores history after restart. Turning it off returns to session views and the previous statistics preference. Both checkboxes are available above the map/table and in Settings > History. Live decoding continues with either option checked: new receptions update the table, map and statistics and are saved to SQLite history and the CSV log.

## Window and reception views

Closing the decoder window hides its interface. Reception, uptime, logging, alerts, GridTracker forwarding and snapshots continue while the sidebar switch is enabled, the SDR is running and reception is not paused. The sidebar shows Receiving in background. Open brings the window to the front, refreshes current filters and preserves its size and position. The sidebar switch disables reception; closing the window does not reset statistics or data.

The table area has three tabs with shared time, search and reception filters:

- Unique stations keeps one latest matching row per sender. Its default sort is newest Station first heard.
- All messages shows each retained reception, including repeated messages and senders, newest UTC first. Equal timestamps use arrival order. It adds UTC date/time and Source to the decoder fields; the count column is hidden because each row is one reception. Its green/gold values and nine-second arrival highlight match Unique stations.
- Countries heard groups the latest matching reception of each distinct sender by country. It shows station count, mean distance, mean SNR, the latest five distinct senders and time since the last reception. Unknown distances are excluded from the distance mean. SNR is the arithmetic mean of receiver estimates in dB, not combined signal power. Repeated messages do not add weight. Unresolved sender hashes are excluded from station averages. The tooltip states how many senders supplied distance evidence.

Each tab remembers its sort, column order, widths and visibility. Countries heard initially sorts latest reception first. Exports follow the active tab. Clear and its confirmation still apply to the table/map views and optional statistics reset, across all tabs.

## Waterfall corner notifications

Settings > General > Corner notifications on waterfall starts on and saves its preference. The corner remains visible when the decoder window is closed. Every new reception gets its own row, including repeated messages and senders: flag, country, sender, approximate distance, receiver SNR and the full quoted message in italics. Countries use green, senders blue and distances orange; SNR and messages use neutral text. New rows glow in and out for two seconds. Newest arrivals appear first. Rows expire after 10 seconds by default; Notification duration in General settings allows 2-120 seconds. There is no four-row limit.

The farthest reception in the selected statistics scope stays below the notification list. Its row includes a trophy and country flag. Only the word Farthest is gold; the remaining fields use the usual colors. Flags use ISO region codes from the bundled Natural Earth data and existing DXCC outline aliases; unmapped entities use a neutral flag. A flag identifies the displayed region, not an operator's nationality.

Long reception lists scroll vertically; long full messages scroll horizontally and are also available on hover. Corner scrolling does not tune the waterfall. Expiry removes only notifications; table, map, statistics and persistent logs retain the reception. Saved history can restore the farthest record but does not replay old receptions as notifications. Clear with Reset stats clears the notification feed too.

## Flags, details and distance units

Country names in Unique stations, All messages, Countries heard and the details history have flags before them. Existing green/gold country highlights remain unchanged.

Station details show separate colors for sender, country, distance, received signal, mode/grid and message. The close button stays at the right. Reception history is a table with UTC, sender, target, country, grid, SNR, Distance, mode/band, full message and source. Right-click anywhere on a details row for the same station popup as the main table. It searches the sender on Google, QRZ or HamQTH; Look up addressed station offers the same providers for the target. The menu retains the clicked reception while new rows arrive.

Double-click a Countries heard row to open all its matching messages in the details pane, newest first, including repeats. The current table filters, time window and saved-history choice apply. New matching receptions update this pane in the background. Older messages loads the next 500 receptions.

Settings > General > Distance units offers Kilometers (km), the default, and Miles (mi). The choice is remembered. Distance columns, country averages, farthest statistics, latest-heard summaries, map tips, details, alerts and waterfall notifications use the selected unit. Distance filters show and accept that unit; changing units preserves their physical limits. Sorting uses the same underlying distances. CSV/JSON exports and reception logs keep their explicitly named kilometer fields for compatibility. Distances remain approximate received-grid-center short paths in either unit.

## Table timestamps and sorting

| Column | Meaning |
| --- | --- |
| Station first heard | First reception of any message from that callsign during the visible session, across bands and modes. |
| Msg age | Time since the latest received message. Repeated receptions refresh it. |
| Msg first heard | First reception of this exact message from this callsign during the visible session. Repeats retain it. |

Age values through 600 seconds show elapsed seconds followed by ago, such as `30s ago` or `600s ago`. Older values show the original UTC date and time, such as `2026-10-14 14:30:00 UTC`. The same rule applies to both reception tables, Countries heard > Last heard and map age tooltips. Unknown times show `--`; future timestamps show `0s ago`. Hover still shows the original UTC date/time. Sorting always uses the underlying timestamp; age filters continue to use elapsed seconds regardless of the displayed format. With saved history for everything enabled, the first-heard times use retained history rather than this session. **Station first heard** appears before **Msg age**. All three align right. The default sort places newly heard stations first, using their first-heard timestamps. Click any column header to change the sort. Drag headers to reorder columns and drag their edges to resize. Right-click a header to show or hide columns. Order, widths, visibility and sorting are saved across restarts; horizontal scrolling reveals extra columns. Sender identifies the transmitter. Target shows the addressed callsign, or -- for a general call or no target. Mod shows FT8 or FT4 for the latest matching reception. Target and Mod initially appear last, in that order.

The table shows **one row per callsign** across all matching bands, modes and dial frequencies. Its message, SNR, band, mode, distance and other reception fields come from the latest matching decode. Rows without a transmitter identity group by exact message text. Individual receptions remain in saved logs and station details. Right-click a row for lookup, copying, watch-list and other station actions. Selecting a row opens details; **Close details** sits at the right edge.

The Message cell updates with the newest matching reception from that station. If timestamps tie, the last arrival wins. Filters apply before selecting the latest row: filtering to 40m shows that station's latest 40m reception. Two receptions on nearby FT4 dial frequencies appear in one station row. All individual receptions remain logged.

## Colors and distance evidence

- Green country: any station from one of the three most recently discovered countries. Discovery order follows each country's first arrival in the statistics session; repeats and new stations from an already heard country do not promote it. A fourth new country returns the oldest marker to normal text. With saved history for everything enabled, discovery uses all retained receptions. Every displayed station row from those three countries stays green as messages update. Gold takes precedence for the farthest station.
- Bold gold country and distance: the farthest station in the selected statistics scope. Gold takes precedence over green.
- Fading green row: just received message, fading over nine seconds.
- Light-green age and map halo: the latest reception batch, until a newer batch arrives.
- Callsign and grid text remain neutral. Teal SNR indicates at least -10 dB; this is a display threshold, not a calibrated quality grade.

Distances are approximate great-circle distances between Maidenhead grid centers. A four-character grid represents an area, so its center is not a surveyed transmitter location. No tilde is shown. `*` before a distance means the same callsign's most recent directly received grid was reused, for up to six hours. Repeated nongrid messages never renew the evidence time. No recent grid means unknown distance.

Another decoder may use a country or regional reference point and report a different distance. This plugin uses received grid evidence. Germany is displayed under that short name throughout the interface and new logs.

Hover over home to see the configured Maidenhead grid and its center coordinates; station hover shows its received grid center coordinates. Changing the home grid recalculates saved table distances, bearings and farthest statistics in the background. Existing CSV entries keep the values recorded at reception time.

For example, KM69 (39.5 N, 33.0 E) to NO35 (55.5 N, 87.0 E) is 4,296.4 km on the mean-radius sphere. The [ARRL grid-distance calculator](https://contests.arrl.org/griddistancecalc.php) gives 4,296.5 km using a slightly different Earth radius. These are geographic short-path distances; the radio signal's actual ionospheric path is not measured.

## Map and communication arcs

Drag the visible divider to give more space to the map or table. Drag the map to pan and scroll over it to zoom. Settings > Map selects the basemap. The default **Street by Esri (English)** provides the street/terrain style used by GridTracker's Esri option. Humanitarian OpenStreetMap and the bundled offline map remain available.

The house marks home. Country labels on the offline/OpenStreetMap maps use small muted text and disappear if the label does not fit inside the country outline. Provider labels baked into tiles follow that provider's style. Optional heard-country shading starts faint green and has a color/opacity selector. Several DXCC entities can share one outline.

Night shading follows the solar terminator. The boundary indicates the day/night geometry; it does not predict reception conditions.

Dark-blue curved moving dots connect an addressed transmitter to the station it calls. General calls create no arc. There is one arc per station pair, dial and mode, with the latest reply setting its direction. Both stations need received grids, which may be cached. Set **Latest exchanges** and **Animate paths** in Map settings. The curved display is a visual guide, not a measured ionospheric ray path.

Selecting a station highlights its marker without drawing a path from home.

Station hover shows age, distance, band, receiver SNR and the latest five distinct messages on that frequency/mode. Each line has its latest UTC time followed by quoted text. Repeats refresh that message's time without using another row. The history loads in the background.

## Map caching

Visible online tiles download in the background with four parallel requests and reused connections. Previously viewed tiles are cached in memory and in a bounded 256 MB disk cache. Parent/child tile fallbacks keep useful map detail visible during zoom changes. Downloads outside the view are cancelled.

The cache respects provider expiry and no-store directives. Stale cached tiles remain usable during network failure. It is not an all-zoom offline download: only visited tiles are cached, with eviction as the limit is reached. The bundled world map works without a network. Provider credits stay visible on the map.

Right-click a station marker or table row for the same station menu: Google, QRZ, HamQTH, copying, favorites and Ignore transmitter. **Look up addressed station** opens the same three providers for the target callsign in an exchange message. General calls and unresolved target identifiers do not offer a callsign lookup. When nearby entries share a marker, the menu applies to the latest reception represented by that marker.

**Ignore transmitter** opens a confirmation naming the displayed transmitter identity and log paths. It accepts valid, unresolved and malformed identities. Confirming removes matching receptions on every band and mode from the table, map, statistics, SQLite history and known reception CSV logs, and blocks matching future receptions. An unresolved hash is matched as the displayed identifier; it may be reused by different stations. If a row has no transmitter identity, the confirmation instead names its exact message: repetitions of that text are removed and ignored across bands and modes, while other unidentified messages are kept. Message text alone cannot identify a sender. Uptime is retained. Cancel leaves data unchanged. **Unignore all** in the station menu or Settings > History allows future receptions; deleted records stay deleted.

Log cleanup runs in the background. A failed file write remains visible and retries, including after restart. Cleanup covers the current/default reception log and prior log locations recorded by this version. It does not edit other decoders' input logs, exported snapshots or unmanaged CSV copies.

## Filters, clear and logs

Band, FT8/FT4 and message-type controls sit above both views. CQ/general call includes CQ and QRZ. Two-way exchange means a message addressed to another station. Filters provide Sender, Target, Mod, prefix, country, continent, grid, band, dial frequency, message text, protocol meaning and exclusions. Column value ranges add optional minimum/maximum limits for SNR, distance, dial MHz, audio Hz, signed DT, bearing, reception count, Msg age and both first-heard ages. Age limits use seconds ago and update with time. Unknown values fail an enabled range; limits are inclusive. Text, band, mode and existing reception filters select matching receptions before choosing the latest row. Column value ranges then test the displayed row, so they never substitute an older matching decode. Reception count counts matching receptions; first-heard times retain their session/history scope. Alerts use reception values and omit station-count/age ranges. Link map/table filters when desired.

**Clear** asks for confirmation. **Reset stats** is the first option and starts checked. Select table, map or both. Clearing hides prior rows and starts new visible first-heard times; it keeps the reception database and CSV. Reset stats turns off both saved-history options, so totals return to zero. Without Reset stats, earlier rows are hidden only in selected views and saved history still contributes to enabled history statistics. Turn Use saved history for everything off and on to restore all retained rows. Brown restart starts empty views by default; enabling history for everything restores retained rows instead. Settings > History also has individual restore buttons.

Settings > History selects the CSV location and maximum megabytes. At the limit, the oldest complete CSV records are trimmed; Clear does not trim it. CSV supports commas, quotes and embedded line breaks. It includes UTC date/time, reception ID, identities, country, grid evidence, band/mode, frequencies, SNR, DT, distance, bearing and full message/raw text. The retained SQLite database is separate from the CSV size limit.

## GridTracker and alerts

In GridTracker, configure a local WSJT-X UDP listener on port 2237. In Settings > Integrations, enable the feed using the filled-in `127.0.0.1:2237` values. The plugin sends receive-only heartbeat, status and decode packets, with one identity per dial/mode. It never transmits. A recent GridTracker reply confirms communication; sent packet counts alone do not prove delivery.

Recent live receptions can replay when enabled or on GridTracker's request. The default window is 30 minutes, limited to 5000 entries within the current UTC day. Imported history and recording playback are excluded. Stop a separate helper feed if it supplies the same receptions, to avoid duplicate sources. [GridTracker setup reference](https://docs.gridtracker.org/latest/Appendices/Appendix-B-Configuring-WSJT-X-and-JTDX-for-GridTracker.html).

Sound alerts start off. Settings > Alerts selects events and individual macOS sounds. Preview plays only when clicked. Enabled alerts can sound with the decoder window closed; a cooldown limits repeated alerts.

## DX activity bearing

The top statistic is labeled **DX ACTIVITY BEARING**.

The statistic takes short-path bearings from your home grid to the farthest recent unique stations and calculates their equal-weight circular mean. The reception window is always the last 15 minutes, independent of the map/table time window or older saved history. Five farthest stations are used by default; the station count is adjustable in Reception settings. Map filters and eligible capture bands/modes apply. A normalized resultant below 0.5 displays mixed directions instead of one unreliable bearing.

This measures heard geographic activity. Antenna pattern, transmit power, band, sampling, long-path propagation and ionospheric conditions can all affect it. It is not a measured arrival azimuth or proof that a particular propagation direction is open.

## Data and privacy

The default data directories are `~/Library/Application Support/SDR++Brown DX Notebook/` for Brown and `~/Library/Application Support/SDR++ DX Notebook/` for standard SDR++. The SQLite history, CSV and tile cache stay local. Online maps contact their tile providers; callsign lookups open the selected website in your default browser. GridTracker UDP is off until enabled. There is no account, product analytics or automatic log upload.

## SDR++ and SDR++Brown

Both hosts are supported through separate matching binaries. Use the native installer to select the detected application; each host keeps a separate profile, reception database and CSV log. Brown's integration remains native. Standard SDR++ uses standard host APIs and does not require Brown to be installed.

Set the home locator in the installer, in Brown's **Source > My Grid**, or in standard SDR++'s plugin **Settings > General > Maidenhead grid**. Without a valid home locator, the standard build leaves the home marker, distances and bearings unknown while station-grid mapping and decoding continue. Four- and six-character locators work in both; the standard build also accepts eight.

Standard SDR++ timestamps live capture against the Mac's UTC clock. Keep the computer clock synchronized. Its file source lacks Brown's recorded-stream timestamp API, so replay timestamps describe when the samples were processed rather than their original recording date.

## Color based on SNR

The checkbox beside **Use saved history for everything** starts off and is remembered. When enabled, each map circle uses the latest receiver SNR represented by that marker: red at -30 dB, yellow at -10 dB and green at +10 dB. Colors interpolate continuously; values outside this fixed range use its endpoint colors. The map shows a gradient legend, and tooltips retain the exact numeric SNR. Latest-reception halos and gold selected-station outlines stay separate. This is a visual scale for the decoder's received SNR estimate, not a calibrated propagation or signal-quality grade. Country shading and table colors are unchanged.
