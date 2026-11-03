# Installs AETHER DUO.vst3 for all users (needs the admin prompt). Right-click -> Run with PowerShell.
$src = Join-Path $PSScriptRoot 'AETHER DUO.vst3'
$dst = 'C:\Program Files\Common Files\VST3'
if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Start-Process powershell -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""; exit
}
New-Item -ItemType Directory -Force $dst | Out-Null
Copy-Item $src $dst -Recurse -Force
Write-Host "Installed to $dst. Rescan plugins in your DAW; it shows up as an instrument named AETHER DUO."
Read-Host 'Press Enter to close'
