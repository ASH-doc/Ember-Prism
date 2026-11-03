param([string]$PluginPath = (Join-Path $PSScriptRoot "Ember VA.vst3"))
$ErrorActionPreference = "Stop"
if (-not (Test-Path $PluginPath)) { throw "Pass -PluginPath pointing to the complete Ember VA.vst3 folder." }
$Destination = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3"
New-Item -ItemType Directory -Force $Destination | Out-Null
Copy-Item $PluginPath $Destination -Recurse -Force
Write-Host "Installed in $Destination. Rescan VST3 plugins in your DAW."
