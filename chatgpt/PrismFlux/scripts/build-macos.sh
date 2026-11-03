#!/bin/bash
# Prism Flux: native Mac build. For universal: PRISM_ARCHS='arm64;x86_64' ./scripts/build-macos.sh
set -euo pipefail
project_root="$(cd "$(dirname "$0")/.." && pwd)"
cmake_exe="${PRISM_CMAKE:-$(command -v cmake || true)}"
if [[ -z "$cmake_exe" ]]; then
  echo 'Install CMake 3.22+ and Xcode Command Line Tools, then rerun. You may set PRISM_CMAKE to the full cmake path.' >&2
  exit 1
fi
ctest_exe="$(dirname "$cmake_exe")/ctest"
architecture="${PRISM_ARCHS:-$(uname -m)}"
build_root="$project_root/build/macos"
"$cmake_exe" -S "$project_root" -B "$build_root" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_OSX_ARCHITECTURES=$architecture" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
"$cmake_exe" --build "$build_root" --parallel "${PRISM_JOBS:-4}"
"$ctest_exe" --test-dir "$build_root" -C Release --output-on-failure
mkdir -p "$project_root/dist/Mac"
for item in 'Standalone/Prism Flux.app' 'VST3/Prism Flux.vst3'; do
  /usr/bin/ditto "$build_root/PrismFlux_artefacts/Release/$item" "$project_root/dist/Mac/$(basename "$item")"
  /usr/bin/codesign --force --deep --sign - "$project_root/dist/Mac/$(basename "$item")"
done
printf 'Built files: %s/dist/Mac\n' "$project_root"
