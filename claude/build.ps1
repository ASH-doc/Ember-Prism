# Builds AETHER DUO (VST3 + Standalone) with CMake + Visual Studio Build Tools.
#   .\build.ps1                 -> Release build of the plugin + standalone + tests
#   .\build.ps1 -Target DuoSelfTest   -> build one target only
#   .\build.ps1 -Clean          -> wipe the build folder first
param([string]$Target = '', [switch]$Clean)

$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot

$cmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmake) { $cmake = 'C:\Program Files\CMake\bin\cmake.exe' }
if (-not (Test-Path $cmake)) { throw 'CMake not found. Install it with: winget install Kitware.CMake' }

if ($Clean -and (Test-Path build)) { Remove-Item build -Recurse -Force }

& $cmake -S . -B build -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE) { throw 'configure failed' }

if ($Target) { & $cmake --build build --config Release --parallel --target $Target }
else         { & $cmake --build build --config Release --parallel }
if ($LASTEXITCODE) { throw 'build failed' }

Write-Host ''
if (Test-Path 'build\AetherDuo_artefacts\Release\VST3') {
    Write-Host 'VST3      :' (Resolve-Path 'build\AetherDuo_artefacts\Release\VST3\AETHER DUO.vst3')
    Write-Host 'Standalone:' (Resolve-Path 'build\AetherDuo_artefacts\Release\Standalone\AETHER DUO.exe')
}
