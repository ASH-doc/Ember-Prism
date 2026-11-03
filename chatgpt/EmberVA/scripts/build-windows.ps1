param([switch]$InstallTools)
$ErrorActionPreference = "Stop"
$Project = Split-Path -Parent $PSScriptRoot
if ($InstallTools) {
  winget install --exact --id Kitware.CMake --accept-package-agreements --accept-source-agreements
  winget install --exact --id Microsoft.VisualStudio.2022.BuildTools --override "--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended" --accept-package-agreements --accept-source-agreements
  Write-Host "If tools were newly installed, reopen PowerShell before building."
}
$CMake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $CMake) {
  $Candidate = Join-Path $env:ProgramFiles "CMake\bin\cmake.exe"
  if (Test-Path $Candidate) { $CMake = $Candidate } else { throw "CMake missing. Run scripts/build-windows.ps1 -InstallTools, then reopen PowerShell." }
}
if ($CMake -is [System.Management.Automation.CommandInfo]) { $CMake = $CMake.Source }
$CTest = Join-Path (Split-Path -Parent $CMake) "ctest.exe"
$Build = Join-Path $Project "build-windows"
& $CMake -S $Project -B $Build -G "Visual Studio 17 2022" -A x64
if ($LASTEXITCODE -ne 0) { throw "Configure failed. Install Visual Studio 2022 Desktop development with C++ and Windows SDK." }
& $CMake --build $Build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed" }
& $CTest --test-dir $Build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
$Artifacts = Join-Path $Build "EmberVA_artefacts\Release"
$Probe = Join-Path $Build "EmberPluginProbe_artefacts\Release\EmberPluginProbe.exe"
& $Probe (Join-Path $Artifacts "VST3\Ember VA.vst3")
if ($LASTEXITCODE -ne 0) { throw "VST3 load check failed" }
$Release = Join-Path $Project "release-windows"
New-Item -ItemType Directory -Force $Release | Out-Null
Copy-Item (Join-Path $Artifacts "Standalone\Ember VA.exe") $Release -Force
Copy-Item (Join-Path $Artifacts "VST3\Ember VA.vst3") $Release -Recurse -Force
Copy-Item (Join-Path $Project "scripts\install-windows-vst3.ps1") $Release -Force
Write-Host "Built and tested: $Release"
