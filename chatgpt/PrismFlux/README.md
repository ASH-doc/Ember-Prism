# Prism Flux — Synth B

A native C++17 wavetable synthesizer with a crisp, airy, edgy sound, a matching JUCE interface in standalone and VST3, and dedicated **Euphoria** and **Space** macros. Version 1.0.0, prepared 3 November 2026.

## Start on Mac

The `Mac` folder contains real, compiled **Apple Silicon (arm64)** binaries, targeting macOS 11 or later:

- `Prism Flux.app`: standalone instrument.
- `Prism Flux.vst3`: instrument plugin for an Apple Silicon VST3 host.
- `Install-Mac.command`: copies both into your user application/plugin folders. It asks before replacing an existing installation.

You can run the app from this folder or copy it into `~/Applications`. Copy the complete VST3 bundle into `~/Library/Audio/Plug-Ins/VST3`, then rescan plugins in your DAW. In standalone, open the JUCE **Options** menu to choose the output device, sample rate, buffer size, and MIDI inputs. Click the on-screen keys to play; the focused keyboard also accepts JUCE's computer-key mappings.

These locally built binaries are ad-hoc signed, not Apple Developer ID signed or notarized. If macOS blocks opening a downloaded copy, use the specific app's Open Anyway control in System Settings after reviewing the package. There is no need to disable Gatekeeper globally. Intel Macs and DAWs running in Intel/Rosetta mode need a rebuild; see below.

The actual VST3 binary passed discovery, instantiation, MIDI/audio rendering, state recall, and editor creation in an independent native host. Desktop device playback and commercial DAW compatibility still need a hands-on check on your system. See `docs/VALIDATION.md` for the precise test scope.

## What is inside

- 16-note polyphony; mono/legato and glide; two wavetable oscillators with up to 16 unison lanes each, sub and colored noise.
- 30 original tables in six families, 64 frames per table, 2048-sample base frames, 10 mip levels, cubic sample and smooth frame interpolation.
- Two warp stages per oscillator: sync, bend +/−, mirror, asymmetry, FM, AM, ring, PWM, quantize and flip.
- Two filters with nine modes and four routing options.
- Three AHDSR envelopes, four drawable LFOs, 24 modulation routes, eight assignable/renameable macros, Euphoria and Space, velocity, key, wheel, pressure, random and sample/hold.
- Arpeggiator and 16-step pitched sequencer with gate/velocity editing, swing, latch, host/internal clock, and a randomizer with six section locks.
- Ten reorderable effects: chorus, flanger, phaser, delay, reverb/shimmer, distortion, EQ, compressor, filter and widener.
- 116 original preset recipes plus Init, category/tag search, JSON patches with embedded imported wavetables, host automation/state, MIDI learn, undo, snapshot/swap, panic, spectrum and voice/unison displays.
- The same resizable native 16:9 interface in both formats. `Preview` contains images rendered from the real editor and a 42-second audio demonstration.

## Play and edit

Start with a preset, lower your monitoring level, and play MIDI or the on-screen keyboard. **Euphoria** adds oscillator spread and movement, level lift and airy EQ. **Space** expands enabled delay/reverb/widener modules, including shimmer feedback. Both are additional modulation sources. Bypassed effects stay bypassed.

Drag a colored source chip onto a control to add modulation. Click its colored modulation ring to inspect the route; the Matrix tab provides source, destination, depth, bipolar and curve editing for all 24 routes. All 519 parameters are exposed in **All controls** and to the DAW. Advanced oscillator/filter/FX options are behind **Edit** or **…**.

Draw an LFO shape with the mouse; right-click the drawing for sine, triangle, ramp, square or random shapes. In ARP/SEQ, the upper lane sets pitch, the aqua lane velocity and the pink lane gate. The ARP + and SEQ + buttons expose additional settings and the arp's own step lanes. Drag an effect's title/grip to reorder it. Double-click a macro label to rename it. Right-click a control label for MIDI learn, reset and modulation removal. Use **Snapshot**, then **Swap**, to compare two patches; these exchange snapshots, not continuous scene morphing.

For WAV import, choose entire-file single cycle or a strip with 512, 1024 or 2048 samples per cycle. Imported frames are resampled to 2048 samples, normalized/DC-corrected and mipmapped. Strips use at most 64 complete frames. An imported table is embedded in saved JSON; original files need not stay on disk. This is a wavetable importer, not a sampler or automatic pitch/cycle detector.

## Build Windows

Windows binaries are **not precompiled in this package**. Source, JUCE and build files are included.

1. Install Visual Studio 2022 / Build Tools with **Desktop development with C++**, a Windows SDK, and CMake 3.22 or later. Alternatively, run `Build-Windows.bat -InstallTools` from a terminal to use winget to install the build tools. Tool installation may require administrator confirmation.
2. Run `Build-Windows.bat`. It builds x64 Release and runs the core, native and actual-VST3 checks.
3. Find `Prism Flux.exe` and the complete `Prism Flux.vst3` bundle under `dist/Windows`.
4. Run `scripts/install-windows-vst3.ps1` to copy the plugin to the current user's VST3 folder. If needed, add `%LOCALAPPDATA%\Programs\Common\VST3` to the DAW scan paths.

JUCE is bundled in `third_party/JUCE`; a separate JUCE installation, Projucer or UI library download is unnecessary. The Windows scripts and workflow have not been executed on Windows in this session.

## Rebuild Mac

Install Xcode Command Line Tools and CMake 3.22+. From this project folder:

```sh
./scripts/build-macos.sh
# Universal Apple Silicon + Intel:
PRISM_ARCHS='arm64;x86_64' ./scripts/build-macos.sh
```

The script builds, tests and ad-hoc signs new binaries in `dist/Mac`. Use `PRISM_CMAKE=/absolute/path/to/cmake` if CMake is not on PATH. Universal and Intel builds are provided as build options; the included prebuilt binaries are arm64 only.

For portable DSP-only validation without JUCE:

```sh
cmake -S . -B build/core -DPRISM_NATIVE=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/core --config Release
ctest --test-dir build/core -C Release --output-on-failure
```

## Source and handoff

`Source` holds the independent Synth B DSP, schema, patch handling and native UI. `Tests` holds the regression suites and independent VST3 host. `Presets/factory.json` contains the complete bank; `scripts/generate-bank.py` regenerates it with Python 3 using `docs/parameters.json`. Python is not required to build or run the supplied project.

Read `docs/FEATURE-COVERAGE.md`, `docs/KNOWN-LIMITATIONS.md`, `docs/HANDOFF.md`, and `docs/VALIDATION.md`. The requested native C++ app/plugin replaces the PDF's browser implementation and phase-by-phase chat workflow. No browser build or combined two-synth mixer is included.

The authored source is provided under the MIT license in `LICENSE`. The combined JUCE binary is distributed using JUCE's AGPLv3 option and the bundled SDK's open-source terms; the full corresponding source and build scripts are included. Third-party components retain their own licenses. See `THIRD-PARTY-NOTICES.md` and `third_party/JUCE/LICENSE.md` before redistributing a modified build.
