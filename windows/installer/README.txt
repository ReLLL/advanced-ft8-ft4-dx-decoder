Kondor FT8 / FT4 / WSPR / FST4 / FST4W / JS8 / Q65 Advanced Decoder
For SDR# (.NET 9, x64)
Developer: ReLLL - https://github.com/ReLLL/kondor-weak-signal-modes-decoder

1. Extract this entire ZIP. Close SDR#.
2. Open Install.cmd and select SDRSharp.dotnet9.exe in your SDR# folder.
3. Open SDR#. Find Kondor Advanced Decoder in the plugins panel.

Manual installation (without Install.cmd):
Close SDR#. Copy this ZIP's top-level AdvancedDx folder into Plugins beside
SDRSharp.dotnet9.exe. Keep the nested folder structure and all accompanying files:
Plugins\AdvancedDx\SDRSharp.AdvancedDx.dll
Plugins\AdvancedDx\AdvancedDx\dx_sdrsharp.exe
If SDRSharp.config sets core.pluginsDirectory, use that directory instead.
For upgrades, move the old AdvancedDx folder to a backup outside Plugins first.
Reopen SDR# and enable the decoder. No Plugins.xml entry is required.

Keep the full AdvancedDx folder together. Closing the decoder window keeps it
receiving in the background while enabled. The Open button brings it back.
FT8, FT4 and WSPR start enabled; other modes are optional in settings.
Tune the visible spectrum to a supported dial frequency and start the radio.

Set your home grid in Settings for distances and bearings. A Maidenhead grid
is a short location code such as KM69. Find your location on a locator map
(https://www.levinecentral.com/ham/grid_square.php), then copy its grid code.

Preferences and reception history are stored separately from SDR#:
%LOCALAPPDATA%\Advanced DX Decoder\SDRSharp
Updates preserve that folder. Uninstall by closing SDR# and removing only
Plugins\AdvancedDx. Previous plugin binaries are backed up in AdvancedDxBackups
beside SDR#. SDR# and its native DSP libraries are supplied by Airspy, not this
package. See Licenses for third-party acknowledgements and license terms.

This package targets the x64 .NET 9 SDR# host. Older/.NET 8 and x86 SDR# builds
need a matching adapter and are not covered by this target.

We need feedback, reception reports and feature ideas:
X / Twitter: https://x.com/FoxOnTheRunTr (@FoxOnTheRunTr)
GitHub: https://github.com/ReLLL/kondor-weak-signal-modes-decoder/issues
