# Validation

Built Release for Apple Silicon using CMake, Apple Clang and the bundled JUCE8.0.9. The app and VST3 target macOS11+, are ad-hoc signed, and pass strict code-signature verification. The deployment target is not an assertion of testing on macOS11.

`HostIntegration` passes checks for layer and split boundaries, per-engine ranges and channels, MIDI offsets, control messages and held-note ownership during routing changes. It verifies that muting ramps to silence without stopping either instrument, and that unmuting reveals the still-playing voice. It also verifies full state recall and the host automation bridge.

The integration test compares each hosted instrument against an independent instance of its original processor with identical sample-offset MIDI. With the host mixer at0dB and master effects bypassed, the output is sample-identical for A and sample-identical for B after its four-sample alignment delay. The test changes the visible panel during playback. The source audit in `ENGINE-INTEGRITY.json` confirms34 matching files; only the two processor wrapper files differ, for their bank namespaces and factory symbols. Synthesis engines, internal effects, editor source and factory banks are unchanged.

The test also renders a layer with enabled master effects and checks finite, nonzero audio. Both editor views were rendered at1280 and1600pixels; previews are snapshots of the actual native editor. The same editor is used for both formats.

`ActualVST3` loads the real VST3 using an independent JUCE host, processes MIDI and stereo audio, saves/restores plugin state and creates its editor. It also passed against the final copied and signed package.

**Desktop standalone launch remains unverified.** LaunchServices returned -10827 in the execution session even though the arm64 executable is present and the bundle signature verifies. The VST3 and native editor tests do not verify audio-device playback from the standalone app. Windows build files are supplied; there was no Windows toolchain available to build or run them. The Intel/universal build option was not run.

Logs: `ctest.txt`, `native.txt`, `packaged-vst3.txt`, and `standalone-launch.txt`.
