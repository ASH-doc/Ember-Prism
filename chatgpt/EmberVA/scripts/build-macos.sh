#!/bin/bash
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build-macos"
# Default creates a universal Apple Silicon + Intel package; override EMBER_ARCHS=arm64 if desired.
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="${EMBER_ARCHS:-arm64;x86_64}" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build "$BUILD_DIR" --parallel 4
ctest --test-dir "$BUILD_DIR" --output-on-failure
"$BUILD_DIR/EmberPluginProbe_artefacts/Release/EmberPluginProbe" "$BUILD_DIR/EmberVA_artefacts/Release/VST3/Ember VA.vst3"
mkdir -p "$PROJECT_DIR/release-macos"
ditto "$BUILD_DIR/EmberVA_artefacts/Release/Standalone/Ember VA.app" "$PROJECT_DIR/release-macos/Ember VA.app"
ditto "$BUILD_DIR/EmberVA_artefacts/Release/VST3/Ember VA.vst3" "$PROJECT_DIR/release-macos/Ember VA.vst3"
codesign --force --deep --sign - "$PROJECT_DIR/release-macos/Ember VA.app"
codesign --force --deep --sign - "$PROJECT_DIR/release-macos/Ember VA.vst3"
echo "Built and tested: $PROJECT_DIR/release-macos"
