# Additional receive modes

Read when: extending capture, decoder workers, mode filtering, marker rendering or protocol-specific reception fields.

## Status and setup

Development build 1.0.33 adds WSPR, FST4, FST4W and Q65 to both host builds. The published 1.0.32 release supports FT8/FT4. Additional modes start off. Use Modes... or Settings > Reception to enable a mode, choose its sequence length and, for Q65, match the tone spacing used on air. FT8/FT4 retain their existing controls and decoder helper.

| Mode | Sequence lengths | Map shape | Message behavior |
| --- | --- | --- | --- |
| FT8 / FT4 | 15 / 7.5 seconds | Circle | Existing QSO formats |
| WSPR | 120 seconds | Square | Beacon, optional grid and transmitter power |
| FST4 | 15, 30, 60, 120, 300, 900, 1800 seconds | Triangle | QSO formats |
| FST4W | 120, 300, 900, 1800 seconds | Diamond | Beacon, optional grid and transmitter power |
| Q65 | 15, 30, 60, 120, 300 seconds | Hexagon | QSO formats |

Q65 offers A-C at 15 seconds, A-D at 30 seconds, and A-E for the longer sequences, matching the documented combinations. This build decodes individual sequences without cross-sequence averaging. Match the transmitting stations; the plugin does not automatically infer the sequence or spacing. Common dial presets come from the pinned WSJT-X frequency list; custom dial frequencies are supported.

## Reception and limits

All modes share the map, Unique stations, All messages and Countries heard tabs, filtering, country lookup, home distances, notifications, saved-message management and persistent CSV/SQLite logging. The mode filter selects a protocol; Beacon selects WSPR/FST4W. Reported transmitter power appears in details in dBm/watts and is separate from receiver SNR. Beacon records have no addressed target and never create QSO arcs. Hashed or unresolved identities do not establish a country. Existing CSV columns remain compatible; full beacon text preserves power, while SQLite payloads also retain protocol metadata.

Shape identifies mode and SNR remains color. FT8/FT4 circle geometry, selection outlines and arrival halos are preserved. The shape legend appears only with additional-mode records. Distances and bearings always use the receiver's home grid. GridTracker forwarding currently remains FT8/FT4 only.

A capture begins on the next complete UTC sequence boundary. Progress shows elapsed and total seconds. Stopping, retuning, timestamp discontinuities and mode changes invalidate partial captures. Retuning also cancels queued/in-flight additional-mode work. At most four additional channels and 96 MB of capture audio are active; the independent worker queue holds at most four jobs and 64 MB of audio. Expired or excess jobs increment the skipped count. Additional modes cannot occupy the FT8/FT4 decoder worker. One additional decoder runs at a time, at reduced process priority, with a 120-second watchdog. Errors appear separately and later sequences can recover.

Decoder processes use owned temporary directories. WSPR audio and diagnostic reception files are removed after each job; bounded hash/FFT state stays only for the worker's lifetime. Stopping a worker kills its process group and removes its scratch directory. Protocol changes cannot load synthetic tests into the receiver's normal history.

## Sources, build and packaging

Use the pinned official WSJT-X routines, revision 967c85a6119110788b0df3782cf796485b2cee14, with original notices intact. Source manifests under third_party/wsjtx record file hashes. The plugin adds native framing and receiver adapters. Build dependencies include a Fortran compiler and Boost headers; users need neither a compiler nor Python. See CREDITS.md and THIRD_PARTY_NOTICES.md for authors and licenses.

Normal builds enable DX_ADDITIONAL_MODES. Set it OFF for an FT8/FT4-only build. The two additional helpers and their runtime libraries form one verified installer payload. The staged GCC 14.3 runtimes come from the pinned Homebrew arm64_sequoia bottle, independently of the developer's newer compiler runtime. Staging verifies hashes and Mach-O minimum OS versions, changes copied library paths to portable relative names and replaces files atomically. No host application is modified or re-signed. macOS 15 is the build floor; macOS 26 is the test environment.

## Validation

Official-encoder fixtures exercise all seven FST4 periods, all four FST4W periods and all 22 supported Q65 period/spacing combinations through framed helper IPC. WSPR fixtures cover standard and portable callsigns and full two-minute real audio. The reception pipeline test also checks all four new modes through the independent worker, country/grid interpretation, home distance, shared map/table queries, SQLite and CSV. Failure tests cover queue bounds, stalled additional decoders, uninterrupted FT8 processing, retune cancellation and shutdown. Tests own and remove their synthetic audio, databases, CSV and decoder logs.

These checks establish functional decoding and integration, not a measured weak-signal sensitivity, false-decode rate, EME performance or month-long live soak result. Native regression tests, FT8/FT4 fixtures and C++ address/undefined-behavior checks also remain part of the gate. Fortran routines are unmodified upstream code and are not instrumented by the C++ sanitizers.

References: [WSJT-X protocols and operating guide](https://wsjtx.github.io/wsjtx/guide-full.html), [pinned WSJT-X sources](https://github.com/WSJTX/wsjtx/tree/967c85a6119110788b0df3782cf796485b2cee14).
