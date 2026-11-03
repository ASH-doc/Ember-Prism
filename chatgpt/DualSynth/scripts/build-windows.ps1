# Dual Synth: build native Windows x64 standalone, VST3 and validation tools.
param([switch]$InstallTools, [int]$Jobs = 4)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program exited with code $LASTEXITCODE" }
}
if ($InstallTools) {
    if (-not (Get-Command winget -ErrorAction SilentlyContinue)) { throw 'Install Microsoft App Installer to obtain winget, or install CMake and Visual Studio 2022 C++ Build Tools manually.' }
    Invoke-Checked 'winget' @('install','--id','Kitware.CMake','--exact','--accept-package-agreements','--accept-source-agreements')
    Invoke-Checked 'winget' @('install','--id','Microsoft.VisualStudio.2022.BuildTools','--exact','--accept-package-agreements','--accept-source-agreements','--override','--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended')
    $env:Path = [Environment]::GetEnvironmentVariable('Path','Machine') + ';' + [Environment]::GetEnvironmentVariable('Path','User')
}
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$cmakeExe = if ($cmakeCommand) { $cmakeCommand.Source } else { Join-Path $env:ProgramFiles 'CMake\bin\cmake.exe' }
if (-not (Test-Path $cmakeExe)) { throw 'CMake is missing. Run Build-Windows.bat -InstallTools or install CMake 3.22+ and Visual Studio 2022 with Desktop development with C++.' }
$ctestExe = Join-Path (Split-Path $cmakeExe -Parent) 'ctest.exe'
$buildRoot = Join-Path $projectRoot 'build\windows-x64'
Invoke-Checked $cmakeExe @('-S',$projectRoot,'-B',$buildRoot,'-G','Visual Studio 17 2022','-A','x64')
Invoke-Checked $cmakeExe @('--build',$buildRoot,'--config','Release','--parallel',"$Jobs")
Invoke-Checked $ctestExe @('--test-dir',$buildRoot,'-C','Release','--output-on-failure')
$destination = Join-Path $projectRoot 'dist\Windows'
New-Item -ItemType Directory -Force -Path $destination | Out-Null
Copy-Item (Join-Path $buildRoot 'DualSynth_artefacts\Release\Standalone\Dual Synth.exe') $destination -Force
$vstTarget = Join-Path $destination 'Dual Synth.vst3'
if (Test-Path $vstTarget) { Remove-Item $vstTarget -Recurse -Force }
Copy-Item (Join-Path $buildRoot 'DualSynth_artefacts\Release\VST3\Dual Synth.vst3') $destination -Recurse
Write-Host "Ready: $destination"
