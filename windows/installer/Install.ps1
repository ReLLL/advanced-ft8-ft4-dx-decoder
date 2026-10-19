param([string]$HostPath, [switch]$Quiet)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
try {
    if (!$HostPath) {
        if ($Quiet) { throw 'Specify the SDR# application path.' }
        $picker = New-Object System.Windows.Forms.OpenFileDialog
        $picker.Title = 'Select your SDR# .NET 9 application'
        $picker.Filter = 'SDR# .NET 9|SDRSharp.dotnet9.exe'
        if ($picker.ShowDialog() -ne 'OK') { exit 0 }
        $HostPath = $picker.FileName
    }
    $hostExe = [IO.Path]::GetFullPath($HostPath)
    if ([IO.Path]::GetFileName($hostExe) -ne 'SDRSharp.dotnet9.exe' -or !(Test-Path -LiteralPath $hostExe -PathType Leaf)) { throw 'Select SDRSharp.dotnet9.exe from the official x64 distribution.' }
    $hostRoot = [IO.Path]::GetDirectoryName($hostExe)
    if (!(Test-Path (Join-Path $hostRoot 'shark.dll'))) { throw 'Select the application inside a complete SDR# x64 installation.' }
    $running = Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -and [IO.Path]::GetDirectoryName($_.ExecutablePath) -eq $hostRoot -and $_.Name -like 'SDRSharp*.exe' }
    if ($running) { throw 'Close this SDR# application, then run Install again.' }
    $manifest = Get-Content (Join-Path $PSScriptRoot 'manifest.json') -Raw | ConvertFrom-Json
    foreach ($entry in $manifest.files.PSObject.Properties) {
        $path = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot $entry.Name))
        if (!$path.StartsWith($PSScriptRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid package path.' }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $entry.Value) { throw ('Package damaged: ' + $entry.Name + '. Extract the complete ZIP again.') }
    }
    $plugins = Join-Path $hostRoot 'Plugins'
    $config = Join-Path $hostRoot 'SDRSharp.config'
    if (Test-Path -LiteralPath $config) {
        [xml]$settings = Get-Content -LiteralPath $config -Raw
        $configured = $settings.configuration.add | Where-Object { $_.key -eq 'core.pluginsDirectory' } | Select-Object -First 1
        if ($configured -and $configured.value) {
            $plugins = if ([IO.Path]::IsPathRooted($configured.value)) { [IO.Path]::GetFullPath($configured.value) } else { [IO.Path]::GetFullPath((Join-Path $hostRoot $configured.value)) }
        }
    }
    New-Item -ItemType Directory -Force -Path $plugins | Out-Null
    $target = Join-Path $plugins 'AdvancedDx'
    $stage = Join-Path $hostRoot ('AdvancedDx-stage-' + [guid]::NewGuid())
    $backup = $null
    try {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'AdvancedDx') -Destination $stage -Recurse
        if (Test-Path $target) {
            $backup = Join-Path $hostRoot ('AdvancedDxBackups/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
            New-Item -ItemType Directory -Force -Path ([IO.Path]::GetDirectoryName($backup)) | Out-Null
            [IO.Directory]::Move($target, $backup)
        }
        [IO.Directory]::Move($stage, $target)
    } catch {
        if ($backup -and !(Test-Path $target)) { [IO.Directory]::Move($backup, $target) }
        throw
    } finally {
        if (Test-Path $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
    }
    if ($Quiet) { Write-Output 'Installed. Reception history and preferences are preserved.' }
    else { [System.Windows.Forms.MessageBox]::Show('Installed. Open SDR# and look for Kondor Weak Signal Modes Decoder in its plugins panel. Set your home Maidenhead grid in the decoder settings for distances and bearings.', 'Kondor Weak Signal Modes Decoder') | Out-Null }
} catch {
    if ($Quiet) { Write-Error $_.Exception.Message -ErrorAction Continue }
    else { [System.Windows.Forms.MessageBox]::Show($_.Exception.Message, 'Installation could not finish', 'OK', 'Error') | Out-Null }
    exit 1
}
