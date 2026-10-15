# Additional receive modes

Read when: extending capture, decoder workers, mode filtering, marker rendering or protocol-specific reception fields.

## Status and setup

Release 1.0.39 includes WSPR, FST4, FST4W and Q65 alongside FT8/FT4 in both host builds. FT8, FT4 and WSPR default on; main-window switches control all three. Other modes default off and can be enabled in Modes... or Settings > Reception. Saved enable choices take precedence. Sequence defaults to All; a single period remains selectable to reduce work. This update moves existing sequence settings to All once, preserving mode enable switches; later selections are remembered. For Q65, match the tone spacing used on air. FT8/FT4 retain their existing controls and decoder helper.

| Mode | Sequence lengths | Map shape | Message behavior |
| --- | --- | --- | --- |
| FT8 / FT4 | 15 / 7.5 seconds | Circle | Existing QSO formats |
| WSPR | 120 seconds | Square | Beacon, optional grid and transmitter power |
| FST4 | 15, 30, 60, 120, 300, 900, 1800 seconds | Triangle | QSO formats |
| FST4W | 120, 300, 900, 1800 seconds | Diamond | Beacon, optional grid and transmitter power |
| Q65 | 15, 30, 60, 120, 300 seconds | Hexagon | QSO formats |

Q65 offers A-C at 15 seconds, A-D at 30 seconds, and A-E for the longer sequences, matching the documented combinations. All supports A-C across all five periods; select an individual longer period for D/E. This build decodes individual sequences without cross-sequence averaging. All tries each period explicitly; it does not infer tone spacing. Common dial presets come from the pinned WSJT-X frequency list; custom dial frequencies are supported.

## Reception and limits

All modes share the map, Unique stations, All messages and Countries heard tabs, filtering, country lookup, home distances, notifications, saved-message management and persistent CSV/SQLite logging. The mode filter selects a protocol; Beacon selects WSPR/FST4W. Reported transmitter power appears in details in dBm/watts and is separate from receiver SNR. Beacon records have no addressed target and never create QSO arcs. Hashed or unresolved identities do not establish a country. Existing CSV columns remain compatible; full beacon text preserves power, while SQLite payloads also retain protocol metadata.

Shape identifies mode and SNR remains color. FT8/FT4 circle geometry, selection outlines and arrival halos are preserved. The shape legend appears only with additional-mode records. Distances and bearings always use the receiver's home grid. GridTracker forwarding currently remains FT8/FT4 only.

A capture begins on the next complete UTC boundary for each selected period. Progress follows the shortest period; longer periods collect in parallel and may need up to 30 minutes after their next boundary. Stopping, retuning, timestamp discontinuities and mode changes invalidate all partial captures. Retuning also cancels queued/in-flight additional-mode work. At most four additional channels and 96 MiB of capture audio are active. Each channel retains one rolling buffer of immutable blocks sized to its longest period. Jobs reference overlapping blocks rather than copying audio for every period. Helper IPC streams the exact window directly from those blocks, including clipped first/last blocks.

The independent worker queue holds at most 32 jobs referencing at most 96 MiB of unique audio, ordered by deadline. Count shared blocks once when enforcing the byte limit. Queue memory and capture memory may overlap; in-flight helper audio is additional. Expired or excess jobs increment the skipped count. These bounds accommodate coincident sequence completions without unbounded backlog; All can use more CPU and slow hardware can skip work. Additional modes cannot occupy the FT8/FT4 decoder worker. One additional decoder runs at a time, at reduced process priority, with a 120-second watchdog. Errors appear separately and later sequences can recover.

Capture tests cover every period together, shared block ownership, non-nested 120/300-second UTC boundaries, partial starts, timestamp jumps, resets and memory bounds. The production pipeline fixtures also pass captured audio through shared-block helper IPC, SQLite and CSV. Native ImGui interaction tests check mouse selection, local arrow-key selection and double-click details behavior.

Decoder processes use owned temporary directories. WSPR audio and diagnostic reception files are removed after each job; bounded hash/FFT state stays only for the worker's lifetime. Stopping a worker kills its process group and removes its scratch directory. Protocol changes cannot load synthetic tests into the receiver's normal history.

## Sources, build and packaging

Use the pinned official WSJT-X routines, revision 967c85a6119110788b0df3782cf796485b2cee14, with original notices intact. Source manifests under third_party/wsjtx record file hashes. The plugin adds native framing and receiver adapters. Build dependencies include a Fortran compiler and Boost headers; users need neither a compiler nor Python. See CREDITS.md and THIRD_PARTY_NOTICES.md for authors and licenses.

Normal builds enable DX_ADDITIONAL_MODES. Set it OFF for an FT8/FT4-only build. The two additional helpers and their runtime libraries form one verified installer payload. The staged GCC 14.3 runtimes come from the pinned Homebrew arm64_sequoia bottle, independently of the developer's newer compiler runtime. Staging verifies hashes and Mach-O minimum OS versions, changes copied library paths to portable relative names and replaces files atomically. No host application is modified or re-signed. macOS 15 is the build floor; macOS 26 is the test environment.

## Validation

Official-encoder fixtures exercise all seven FST4 periods, all four FST4W periods and all 22 supported Q65 period/spacing combinations through framed helper IPC. WSPR fixtures cover standard and portable callsigns and full two-minute real audio. The reception pipeline test also checks all four new modes through the independent worker, country/grid interpretation, home distance, shared map/table queries, SQLite and CSV. Failure tests cover queue bounds, stalled additional decoders, uninterrupted FT8 processing, retune cancellation and shutdown. Tests own and remove their synthetic audio, databases, CSV and decoder logs.

These checks establish functional decoding and integration, not a measured weak-signal sensitivity, false-decode rate, EME performance or month-long live soak result. Native regression tests, FT8/FT4 fixtures and C++ address/undefined-behavior checks also remain part of the gate. Fortran routines are unmodified upstream code and are not instrumented by the C++ sanitizers.

References: [WSJT-X protocols and operating guide](https://wsjtx.github.io/wsjtx/guide-full.html), [pinned WSJT-X sources](https://github.com/WSJTX/wsjtx/tree/967c85a6119110788b0df3782cf796485b2cee14).
