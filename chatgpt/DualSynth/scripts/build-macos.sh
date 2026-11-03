#!/bin/bash
# Dual Synth: native Mac build. For universal: DUAL_ARCHS='arm64;x86_64' ./scripts/build-macos.sh
set -euo pipefail
project_root="$(cd "$(dirname "$0")/.." && pwd)"
cmake_exe="${DUAL_CMAKE:-$(command -v cmake || true)}"
if [[ -z "$cmake_exe" ]]; then
  echo 'Install CMake 3.22+ and Xcode Command Line Tools, then rerun. You may set DUAL_CMAKE to the full cmake path.' >&2
  exit 1
fi
ctest_exe="$(dirname "$cmake_exe")/ctest"
architecture="${DUAL_ARCHS:-$(uname -m)}"
build_root="$project_root/build/macos"
"$cmake_exe" -S "$project_root" -B "$build_root" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_OSX_ARCHITECTURES=$architecture" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
"$cmake_exe" --build "$build_root" --parallel "${DUAL_JOBS:-4}"
"$ctest_exe" --test-dir "$build_root" -C Release --output-on-failure
mkdir -p "$project_root/dist/Mac"
for item in 'Standalone/Dual Synth.app' 'VST3/Dual Synth.vst3'; do
  /usr/bin/ditto "$build_root/DualSynth_artefacts/Release/$item" "$project_root/dist/Mac/$(basename "$item")"
  /usr/bin/codesign --force --deep --sign - "$project_root/dist/Mac/$(basename "$item")"
done
printf 'Built files: %s/dist/Mac\n' "$project_root"
