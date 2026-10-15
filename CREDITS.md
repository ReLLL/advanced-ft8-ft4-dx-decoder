# Credits

Advanced FT8/FT4/WSPR DX Decoder is developed by [ReLLL](https://github.com/ReLLL). Its receiver integration, maps, reception history and interface build on substantial work by the amateur-radio and open-source communities.

## WSJT-X, protocols and decoder algorithms

Thank you to the [WSJT-X / WSJT development team](https://wsjtx.github.io/wsjtx/) and the designers and architects of the weak-signal protocols used by this project. The WSJT project was started by **Joe Taylor, K1JT**, in 2001. Its protocol design, signal processing, error-correcting codes and decoder implementation are upstream work, not inventions of this plugin.

We acknowledge **Joe Taylor K1JT, Steve Franke K9AN, Bill Somerville G4WJS, Nico Palermo IV3NWV**, and the other WSJT contributors named in the [upstream acknowledgements](https://github.com/WSJTX/wsjtx/blob/967c85a6119110788b0df3782cf796485b2cee14/doc/user_guide/en/acknowledgements.adoc) and [copyright statement](https://github.com/WSJTX/wsjtx/blob/967c85a6119110788b0df3782cf796485b2cee14/doc/common/license.adoc). This includes contributors to FT8, FT4, WSPR, FST4, FST4W and Q65, their message formats and their coding and decoding methods.

- **FT8 and FT4:** **Steve Franke K9AN and Joe Taylor K1JT**, with the WSJT development team. The [official project history](https://wsjtx.github.io/wsjtx/about.html) credits their FT8 design and Franke's work on the modes, codes and decoders, including FT4.
- **FST4 and FST4W:** **Steve Franke K9AN, Bill Somerville G4WJS and Joe Taylor K1JT**, the authors of the [official FST4/FST4W introduction](https://wsjt.sourceforge.io/FST4_Quick_Start.pdf), with the WSJT development team.
- **WSPR:** Joe Taylor K1JT and Steven Franke K9AN, as credited in the upstream reference decoder. Its Fano decoder also retains **Phil Karn KA9Q**'s notices. See the [WSPR decoder source](https://github.com/WSJTX/wsjtx/blob/967c85a6119110788b0df3782cf796485b2cee14/lib/wsprd/wsprd.c).
- **Q65 and Q-ary repeat-accumulate coding:** **Nico Palermo IV3NWV / Microtelecom Srl**, as credited in the [Q65 coding implementation](https://github.com/WSJTX/wsjtx/blob/967c85a6119110788b0df3782cf796485b2cee14/lib/qra/q65/q65.c), together with the WSJT-X integration contributors.
- **Protocol specifications:** the [WSJT-X protocol documentation](https://github.com/WSJTX/wsjtx/blob/967c85a6119110788b0df3782cf796485b2cee14/doc/user_guide/en/protocols.adoc) describes the modes and their technical parameters.

The staged WSJT-X sources retain their original notices and GPL license. Their pinned revision and file hashes are recorded under `third_party/wsjtx/`. Release 1.0.39 includes FT8, FT4, WSPR, FST4, FST4W and Q65 reception. The README and release notes describe available features.

## MSHV and radio hosts

- **[MSHV](https://sourceforge.net/projects/mshv/): Hrisimir Hristov, LZ2HV**, for the adapted C++ decoder implementation used through SDR++Brown, based on WSJT-family work. Original WSJT and MSHV author and license notices remain in the corresponding source.
- **[SDR++](https://github.com/AlexandreRouma/SDRPlusPlus): Alexandre Rouma and contributors**, for the host application and plugin APIs.
- **[SDR++Brown](https://github.com/sannysanoff/SDRPlusPlusBrown): sannysanoff and contributors**, for the Brown host, its integrations and the decoder source used by the FT8/FT4 build.

## Data, maps and supporting software

- **Jim Reisert, AD1C**, for [Amateur Radio Country Files](https://www.country-files.com/), used for callsign-prefix and exact-call country identification.
- **[Natural Earth](https://www.naturalearthdata.com/)** contributors, for the bundled world geometry.
- **Esri and its data contributors**, and **OpenStreetMap contributors / OpenStreetMap France**, for the selectable online basemaps. The map displays the active provider's attribution.
- **[Mapbox Earcut](https://github.com/mapbox/earcut.hpp)** contributors, for polygon triangulation.
- **FFTW: Matteo Frigo and Steven G. Johnson**, and the contributors to **SQLite, Dear ImGui, Volk, curl, GNU Compiler Collection** and the other libraries used by the plugin, its hosts and decoder dependencies.
- **[GridTracker](https://gridtracker.org/)** developers and community, for the independent mapping application with which the optional WSJT-X-compatible UDP feed interoperates.

Thank you to the operators who test reception, report bugs and suggest improvements. Project names and acknowledgements do not imply affiliation or endorsement. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and the retained upstream license files for redistribution terms; this credits file does not replace them.
