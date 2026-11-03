# Install the completed Windows bundle for the current user. No admin access needed.
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$source = Join-Path $projectRoot 'dist\Windows\Prism Flux.vst3'
if (-not (Test-Path $source)) { throw 'Build Windows first with Build-Windows.bat.' }
$directory = Join-Path $env:LOCALAPPDATA 'Programs\Common\VST3'
$target = Join-Path $directory 'Prism Flux.vst3'
New-Item -ItemType Directory -Force -Path $directory | Out-Null
if (Test-Path $target) {
    if ((Read-Host 'Replace the installed Prism Flux VST3? Type YES') -cne 'YES') { exit }
    Remove-Item $target -Recurse -Force
}
Copy-Item $source $directory -Recurse
Write-Host "Installed: $target. Rescan this folder in your DAW."
