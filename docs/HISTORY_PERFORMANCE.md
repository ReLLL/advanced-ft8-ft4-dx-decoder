---
read_when: Changing history queries, saved statistics, retention or database maintenance.
---

# Reception history performance

The default database limit is **100,000 receptions**, with cleanup to **95,000**. This keeps a bounded recent history for map/table searches. CSV has its own independent size policy. Settings > History shows the actual retained start date, cumulative automatic deletions and any retry error. After automatic deletion, the existing table legend also shows the retained start date.

## Measurements: October 15, 2026

Release builds on Apple Silicon, macOS 26.6.2, system SQLite 3.51.0. Brown continued receiving during the tests. Isolated fixtures repeated the shape of a real reception snapshot, with about 2,500 resolved senders and 84 countries. Synthetic records stayed outside normal profiles and logs. These are observed runs, with mixed cache conditions, rather than latency guarantees or a high-diversity capacity test.

| Operation | Previous, 1 million | Improved, 1 million | Improved, 5 million |
| --- | ---: | ---: | ---: |
| Saved statistics | 16,001 ms | 6 ms | 13 ms |
| Full-history session statistics | 13,879 ms | 3 ms | 3 ms |
| Latest message page | 5,720 ms | 8 ms | 10 ms |
| Unique-station page | 1,967 ms | 29 ms | 52 ms |
| Country filter | 463 ms | 14 ms | 445 ms |
| Broad CQ text search | 908 ms | 898 ms | 8,735 ms |

The previous 5-million-record saved-statistics query was stopped after exceeding 180 seconds. Time Profiler identified Store::stats with SQLite reads and sorting on its stacks. The improved 5-million fixture was about 5.5 GB and passed SQLite quick_check. Initial summary/index backfill is a one-time migration; one early 5-million migration took 91 seconds. Reopening the completed summaries took 2 ms. A 1-million migration took about 4.8 seconds.

Individual SQL timing understates the cost of updating both views. Complete Engine snapshots, including maps, tables, counts, statistics and paths, gave:

| Complete view refresh | 225,000 retained | 90,000 retained | 45,000 retained |
| --- | ---: | ---: | ---: |
| Unique stations | 183 ms | 122 ms | 109 ms |
| All messages | 95 ms | 79 ms | 74 ms |
| CQ text search | 730 ms | 326 ms | 187 ms |
| No-result text search | 401 ms | 207 ms | 91 ms |
| Country filter | 68 ms | 39 ms | 24 ms |
| Band and mode filter | 1,063 ms | 418 ms | 232 ms |
| Distance sort | 125 ms | 103 ms | 87 ms |
| SNR sort | 104 ms | 97 ms | 83 ms |
| Countries heard | 136 ms | 119 ms | 108 ms |

The initial 50,000/45,000 default followed these complete refresh measurements. The requested default is now 100,000/95,000, with checks every 30 minutes; the 90,000-record results are the nearest measured comparison, not an exact benchmark of the new limit. Earlier rows in this table precede the final literal-filter optimization. Raising the limit remains available, with a visible performance explanation.

## Integrity and maintenance

- Latest sender/country summaries live in SQLite and update through triggers in the reception transaction. First/latest timestamps and stable reception IDs are retained. Unknown-message grouping has its own summary.
- Session statistics consume newly committed IDs rather than rescanning every message. Automatic expiry removes only affected cached entries and refreshes their indexed first/latest/farthest references. This avoids rebuilding the session cache after every deletion batch during month-long runs. Manual deletion, distance/country changes and external database commits invalidate the relevant cache.
- Latest-message ordering uses the time index. First-heard lookups and oldest-record deletion explicitly use covering indexes. This avoids an observed planner choice that fetched an entire country's payload pages after each deletion.
- The threshold check runs every 30 minutes, at startup and on a policy change. It reads the persisted count and filesystem metadata. The schedule advances from the current monotonic time after sleep, without replaying missed checks. Automatic cleanup deletes at most 64 records per transaction, with at least 250 ms between maintenance attempts. Reception writes and CSV sync run first; decoder activity and queued queries cannot starve it. Only IDs already passed by the CSV append watermark are eligible, and IDs after the cleanup boundary are protected. Storage errors back off for 30 seconds. Temporary count overshoot is allowed.
- Record deletion, summary changes, deletion count, legacy-import floor and resumable progress commit together. Reception IDs never restart after trimming or VACUUM. A manual cleanup can finish the retention target early; automatic cleanup then stops.
- A 5-million fixture processed over 320,000 records in production-sized batches; the largest measured batch in those completed timing blocks was 33 ms. The process was stopped during cleanup. Reopening confirmed 4,635,712 retained records plus 364,288 committed deletions, matching the initial 5 million, and quick_check passed.
- Separate fixtures completed trims to 225,000, 90,000 and 45,000 and passed full integrity_check after each. Fixture preparation used batches of 256 for these complete trims; these timings are not the production batch limit.
- A continuous Store session completed 12 receive/cleanup cycles with 60,012 new receptions and week-spaced simulated timestamps. It returned to 45,000 records after each cycle, reconciled 66,308 deletions against the initial snapshot plus new arrivals, and passed integrity_check. Across 1,047 production-sized batches, the slowest deletion plus session-statistics update was 32 ms. This exercises repeated expiry within one session, rather than claiming a real three-month unattended run.
- Tests cover failed-transaction rollback, abrupt exit before commit, restart/resume, summary equivalence, later arrivals, CSV catch-up, ID continuity and overlapping manual deletion. Every synthetic fixture is removed after testing.
- Freed pages are reusable immediately. New databases use incremental auto-vacuum; existing databases gain it when compacted. Small incremental steps reclaim pages during idle periods. Existing databases are not rebuilt while receiving; one-off confirmed Saved messages cleanup still performs full compaction.

## Repeating the workload

Use dx_history_benchmark only with a SQLite online-backup copy in its owned temporary directory. The executable refuses other locations. The directory must be created with the advanced-dx-scale- prefix under the system temporary directory and contain the .benchmark-owned marker with advanced-dx-scale-v1. Never point it at a normal profile.

Commands: seed FILE COUNT, measure FILE, engine FILE, soak FILE, and trim FILE LIMIT [BATCH]. The seed ceiling is five million records and it preserves a 2 GB free-space reserve. Engine measurements disable forwarding, sound, legacy sources and automatic trimming, and keep CSV paths inside the fixture. The soak uses Store directly and performs no external logging or forwarding. Move only aggregate measurements into documentation; keep reception payloads and backups private. Stop all fixture writers before deleting owned temporary databases.

SQLite references: [transactional WAL](https://sqlite.org/wal.html), [incremental auto-vacuum](https://sqlite.org/pragma.html#pragma_auto_vacuum), [function argument caching](https://sqlite.org/c3ref/get_auxdata.html).
