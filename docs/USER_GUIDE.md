# User guide

Read when: setting up reception, understanding timestamps or distances, or configuring maps, logs and GridTracker.

## Start receiving

Set your own home Maidenhead grid in Brown. Distances and bearings originate at its center. Start the SDR and place a supported FT8 or FT4 dial inside the captured RF range. The plugin creates its own receiving channels for eligible presets; the ordinary Radio module can keep its selected mode. Check **Receiving**, level in dBFS, slot progress and decoded messages. Additional dial frequencies can be added in Settings > Reception.

FT8 slots last 15 seconds; FT4 slots last 7.5 seconds. A complete slot is needed after tuning or starting. Keep the computer clock synchronized. SNR is the decoder's estimate measured at this receiver; dBFS is the input audio level, a different measurement.

## Statistics and saved history

Countries and stations count unique received identities. Country names use Brown's callsign/DXCC tables; they cannot establish exact transmitter coordinates. **Farthest** includes the country in parentheses. **UPTIME** shows active decoding time for this session, excluding stopped time. **Last 5 min** counts distinct recent callsigns; **Latest heard** shows the latest five distinct calls, country, approximate distance and SNR.

**Use saved history for stats** starts unchecked. Off counts only receptions received since this session started or session statistics were reset. On uses all retained database receptions, including imported history. The worker calculates the new values in the background and the previous values remain visible until ready. Uptime remains session-based. This preference is saved; turn it off to return to session statistics. It does not restore table/map entries or first-heard times.

## Table timestamps and sorting

| Column | Meaning |
| --- | --- |
| Station first heard | First reception of any message from that callsign during the visible session, across bands and modes. |
| Msg age | Time since the latest received message. Repeated receptions refresh it. |
| Msg first heard | First reception of this exact message from this callsign during the visible session. Repeats retain it. |

These values show elapsed seconds; hover shows the original UTC date/time. **Station first heard** appears before **Msg age**. All three align right. The default sort places newly heard stations first, using their first-heard timestamps. Click any column header to change the sort. Columns can be resized, reordered and hidden; horizontal scrolling reveals extra columns.

**Every message** shows individual decodes. Otherwise the table groups each station/frequency/mode and shows its latest matching decode. Right-click a row for lookup, copying, watch-list and other station actions. Selecting a row opens details; **Close details** sits at the right edge.

## Colors and distance evidence

- Green country: the first reception from that country during the current statistics session.
- Bold gold country and distance: the farthest station in the selected statistics scope. Gold takes precedence over green.
- Fading green row: just received message, fading over nine seconds.
- Light-green age and map halo: the latest reception batch, until a newer batch arrives.
- Callsign and grid text remain neutral. Teal SNR indicates at least -10 dB; this is a display threshold, not a calibrated quality grade.

Distances are approximate great-circle distances between Maidenhead grid centers. A four-character grid represents an area, so its center is not a surveyed transmitter location. No tilde is shown. `*` before a distance means the same callsign's most recent directly received grid was reused, for up to six hours. Repeated nongrid messages never renew the evidence time. No recent grid means unknown distance.

Another decoder may use a country or regional reference point and report a different distance. This plugin uses received grid evidence. Germany is displayed under that short name throughout the interface and new logs.

## Map and communication arcs

Drag the visible divider to give more space to the map or table. Drag the map to pan and scroll over it to zoom. Settings > Map selects the basemap. The default **Street by Esri (English)** provides the street/terrain style used by GridTracker's Esri option. Humanitarian OpenStreetMap and the bundled offline map remain available.

The house marks home. Country labels on the offline/OpenStreetMap maps use small muted text and disappear if the label does not fit inside the country outline. Provider labels baked into tiles follow that provider's style. Optional heard-country shading starts faint green and has a color/opacity selector. Several DXCC entities can share one outline.

Night shading follows the solar terminator. The boundary indicates the day/night geometry; it does not predict reception conditions.

Dark-blue curved moving dots connect an addressed transmitter to the station it calls. General calls create no arc. There is one arc per station pair, dial and mode, with the latest reply setting its direction. Both stations need received grids, which may be cached. Set **Latest exchanges** and **Animate paths** in Map settings. The curved display is a visual guide, not a measured ionospheric ray path. A gold line to a selected station is a distance guide from home.

Station hover shows age, distance, band, receiver SNR and the latest five distinct messages on that frequency/mode. Each line has its latest UTC time followed by quoted text. Repeats refresh that message's time without using another row. The history loads in the background.

## Map caching

Visible online tiles download in the background with four parallel requests and reused connections. Previously viewed tiles are cached in memory and in a bounded 256 MB disk cache. Parent/child tile fallbacks keep useful map detail visible during zoom changes. Downloads outside the view are cancelled.

The cache respects provider expiry and no-store directives. Stale cached tiles remain usable during network failure. It is not an all-zoom offline download: only visited tiles are cached, with eviction as the limit is reached. The bundled world map works without a network. Provider credits stay visible on the map.

## Filters, clear and logs

Band, FT8/FT4 and message-type controls sit above both views. CQ/general call includes CQ and QRZ. Two-way exchange means a message addressed to another station. Filters provide callsign, prefix, country, grid, frequency, distance, SNR and exclusion options. Link map/table filters when desired.

**Clear** asks for confirmation. **Reset stats** is the first option and starts checked. Select table, map or both. Clearing hides prior rows and starts new visible first-heard times; it keeps the reception database and CSV. If saved-history statistics are enabled, old history still contributes to those totals. Uncheck that option for a zeroed session count. Brown restart also starts empty views. Settings > History can explicitly restore retained rows.

Settings > History selects the CSV location and maximum megabytes. At the limit, the oldest complete CSV records are trimmed; Clear does not trim it. CSV supports commas, quotes and embedded line breaks. It includes UTC date/time, reception ID, identities, country, grid evidence, band/mode, frequencies, SNR, DT, distance, bearing and full message/raw text. The retained SQLite database is separate from the CSV size limit.

## GridTracker and alerts

In GridTracker, configure a local WSJT-X UDP listener on port 2237. In Settings > Integrations, enable the feed using the filled-in `127.0.0.1:2237` values. The plugin sends receive-only heartbeat, status and decode packets, with one identity per dial/mode. It never transmits. A recent GridTracker reply confirms communication; sent packet counts alone do not prove delivery.

Recent live receptions can replay when enabled or on GridTracker's request. The default window is 30 minutes, limited to 5000 entries within the current UTC day. Imported history and recording playback are excluded. Stop a separate helper feed if it supplies the same receptions, to avoid duplicate sources. [GridTracker setup reference](https://docs.gridtracker.org/latest/Appendices/Appendix-B-Configuring-WSJT-X-and-JTDX-for-GridTracker.html).

Sound alerts start off. Settings > Alerts selects events and individual macOS sounds. Preview plays only when clicked. Enabled alerts can sound with the decoder window closed; a cooldown limits repeated alerts.

## DX activity bearing

The statistic takes short-path bearings from your home grid to the farthest recent unique stations and calculates their equal-weight circular mean. Defaults: five stations within 15 minutes. Both are adjustable in Reception settings. Map filters and eligible capture bands/modes apply. A normalized resultant below 0.5 displays mixed directions instead of one unreliable bearing.

This measures heard geographic activity. Antenna pattern, transmit power, band, sampling, long-path propagation and ionospheric conditions can all affect it. It is not a measured arrival azimuth or proof that a particular propagation direction is open.

## Data and privacy

The default data directory is `~/Library/Application Support/SDR++Brown DX Notebook/`. The SQLite history, CSV and tile cache stay local. Online maps contact their tile providers; callsign lookups open the selected website in your default browser. GridTracker UDP is off until enabled. There is no account, product analytics or automatic log upload.
