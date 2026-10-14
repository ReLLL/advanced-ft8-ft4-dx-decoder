# Third-party notices

## SDR++Brown and MSHV

The plugin integrates with [SDR++Brown](https://github.com/sannysanoff/SDRPlusPlusBrown), pinned at `679f48deee5299a199d15d30e6e785fac6054896`. Its FT8/FT4 decoder contains adapted MSHV code and upstream author notices. GPL terms apply. The staging script applies FFT plan lifetime fixes, without changing decoder sensitivity or algorithms. Release assets include the staged corresponding source and upstream license notices.

## Mapbox Earcut

`third_party/earcut/earcut.hpp` is the unmodified header from [Mapbox Earcut](https://github.com/mapbox/earcut.hpp), version 3.2.4, commit `177bd66b5338ab66f4e9ce8f3cca418b9463d809`. Its ISC license is included in `third_party/earcut/LICENSE`.

## Map data and providers

The bundled world country geometry is derived from [Natural Earth](https://www.naturalearthdata.com/about/terms-of-use/) public-domain data. Online maps are selectable Esri World Street Map and Humanitarian OpenStreetMap. Visible map credits link to the providers and contributors. Online tile caching is bounded and respects response cache directives; no bulk download is implemented.

See [OpenStreetMap attribution](https://www.openstreetmap.org/copyright), [OpenStreetMap France usage](https://www.openstreetmap.fr/usage/) and [Esri credits](https://doc.arcgis.com/en/arcgis-online/reference/display-copyrights.htm). Provider availability and terms can change.

## Runtime and system components

The host provides SDR++ core, Volk and FFTW runtime libraries. SQLite, curl, AppKit, AudioToolbox and OpenGL are used through the macOS/host environment. These libraries are not redistributed as plugin-owned binary dependencies.

## GridTracker compatibility

The receive-only UDP encoder follows [WSJT-X NetworkMessage schema 2](https://github.com/WSJTX/wsjtx/blob/master/Network/NetworkMessage.hpp). GridTracker and WSJT-X are independent projects; this plugin is not affiliated with them.

## Amateur Radio Country Files

The standard SDR++ build bundles CTY.DAT country-prefix data maintained by Jim Reisert, AD1C. The snapshot matches the data used by the pinned Brown host. Format and source: https://www.country-files.com/cty-dat-format/ . Redistribution permission is reproduced in `resources/cty-LICENSE.txt`. Country identity is inferred from prefixes and exact-call exceptions; these records do not locate an individual transmitter.

## Standard SDR++ host headers

The standard build uses headers from SDR++ commit `762444d3406d68fbc9d4dff8cc237d382793a6e3` under GPL-3.0. The required source and upstream license are included in the corresponding-source archive. It links to the user's separately installed standard SDR++ runtime, not Brown's host libraries.
