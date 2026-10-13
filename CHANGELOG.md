# Changelog

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
