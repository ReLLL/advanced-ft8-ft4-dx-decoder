# Advanced FT8/FT4/WSPR DX Decoder

## Test reception cleanup

- Use isolated profiles and reception files for synthetic, offline-recording and decoder tests.
- Delete test receptions after testing from every affected SQLite database, reception CSV/log file, table, map and saved statistics. Test data must not remain mixed with normal reception history.
- Record the exact test source, time range and reception keys before testing. Match that evidence during cleanup; do not delete genuine receptions based only on a callsign that looks like a test.
- Stop test writers before cleanup, verify the test records are gone from all affected stores, and restore the user's normal receiver settings. Rebuild saved summaries and vacuum databases after deletion while preserving reception IDs.
- Keep parent electro changes outside plugin commits and pushes, following the user's explicit repository scope.

## New-mode development

- Keep the published FT8/FT4 baseline intact while adding WSPR, FST4/FST4W and Q65. New decoders remain unavailable in the normal UI until capture, parsing, storage, failure recovery, timing and both-host tests pass. Prototype targets stay off by default and out of installers.
- Marker shape encodes mode; SNR remains color. Preserve existing FT8/FT4 circles, selection outlines and arrival halos. Show the shape legend only when additional modes are present. Beacon messages have no QSO target; never invent a communication arc.
- Do not reuse the FT8/FT4 helper queue for potentially long-running new decoders. Bound separate workers, captured audio, pending jobs and disk use. Retuning or stopping invalidates partial captures and averaging context. Keep test audio and reference-decoder logs in owned temporary folders.
- Preserve existing SQLite payloads and reception CSV compatibility. Transmitter power is distinct from received SNR. Public binaries must include compatible runtimes; the local Fortran prototype alone does not prove the existing macOS 15 support floor.

## Date and time

- Display dates and interpret user-entered date filters in the computer's time zone. Use the shared date formatter and parser; include the UTC offset in displayed timestamps.
- Keep stored epochs, reception CSV log timestamps and radio/GridTracker protocol times in UTC. Sorting uses underlying timestamps, independently of presentation. Respect daylight-saving day lengths and reject ambiguous or nonexistent local cleanup times.

## History performance and integrity

- Remove likely falses is reversible: apply the same country/grid check to existing and new receptions in map, tables, reception statistics and alerts; never delete or ignore records through this checkbox. Keep uptime as elapsed runtime. Cache filtered history and session summaries separately; expire only accepted records from each summary during bounded retention batches.
- Default retention: 100,000 receptions, cleanup to 95,000. Both thresholds are configurable in Settings > History > History DB; existing custom limits without a saved target keep the previous 95% target. Check count and file-size metadata every 30 minutes, at startup and on a policy change. At most 64 records per transaction, with at least 250 ms between maintenance steps. Run reception writes and CSV sync first; protect the CSV watermark and new arrivals. Do not require the decoder or query queue to become idle, which can starve month-long operation. Keep the retained date visible; saved statistics cover retained records. CSV has an independent limit.
- Automatic expiry must update session cache entries incrementally. Rebuilding the whole retained session after every batch causes repeated long pauses once a continuously running session reaches retention age.
- Reception changes, entity summaries, retention progress and stable IDs must commit together. Never reuse IDs after deletion or compaction. Preserve later arrivals outside the captured cleanup boundary and retry interrupted work.
- Keep covering indexes explicit for first/latest UTC lookups. Adding ID indexes caused SQLite to choose country-wide payload scans during deletion; the million-record fixtures exposed this. Measure complete Engine view updates, not only individual SQL queries, before raising retention defaults.
- Performance fixtures require a verified SQLite online backup and owned temporary paths. Delete synthetic databases and CSV files after stopping fixture writers. Keep original backups private and retain only aggregate performance results in project docs.
