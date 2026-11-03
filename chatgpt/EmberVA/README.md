# Ember VA

A native C++ virtual-analog instrument for Arash. One JUCE editor and one stereo synth engine are used by the standalone app and the VST3 plugin. This is the native adaptation of the supplied Synth A roadmap, with dedicated **Warmth** and **Saturation** knobs added beside four assignable macros.

## Start on this Mac

The supplied macOS binaries are **Apple Silicon / arm64**, targeting macOS 11 or later. They do not require JUCE, CMake or a compiler at runtime.

1. Open `Mac/Ember VA.app`.
2. Use the app's **Options → Audio/MIDI Settings** to choose your audio output and enable your MIDI keyboard. Start with 48 kHz and a 256-sample buffer.
3. Play the on-screen keyboard, or click it and use the computer keyboard (`A W S E D F T G Y H U J K`). External MIDI is supported.
4. Select a patch at the top. Start with **Copper Horizon**, **Brass Droplet**, **Copper Foundation**, or **Walnut Sky**.

For VST3, copy the **whole** `Mac/Ember VA.vst3` bundle into `~/Library/Audio/Plug-Ins/VST3/`, then rescan plugins in your DAW. `Mac/Install-Mac.command` performs that copy and places the app in `~/Applications/`. The installer asks before replacing an existing copy. The binaries are locally ad-hoc signed, not Apple Developer ID notarized.

Logic Pro uses Audio Units, not VST3. Use the standalone app or a VST3-capable host such as Ableton Live, REAPER, Cubase, Bitwig or Studio One. An AU is not included in this delivery.

## Play and edit

- Knobs: drag vertically; Shift-drag for finer movement; double-click to reset; wheel to adjust; click the numeric readout to type. Values accept `k` for kHz and `ms` for milliseconds where applicable.
- Four assignable macros: **Open, Width, Echo, Bite**. Double-click a macro label to rename it. Use multiple modulation rows with the same macro source to control several destinations.
- **Warmth** adds asymmetric coloration and a gentle darker balance. **Saturation** increases pre-filter nonlinear drive, with level compensation. They are separate automatable parameters.
- The gate is drawable: drag across its 16 bars to set levels. Shift-click or right-click a step to toggle its tie marker.
- `…` opens detailed controls for a section. **ALL CONTROLS** provides a searchable view of all 264 parameters. The main face has no tabs; detailed controls live in this separate window.
- Modulation amounts are normalized-range offsets from -1 to +1. A small amber dot identifies a parameter targeted by a route. Use the destination dropdown to select any registered parameter.
- Right-click a parameter **label** for MIDI learn, remove mapping, or reset. Move your hardware CC after selecting Learn. CC 64, 120 and 123 remain reserved for sustain and note safety.
- Hold up to four notes and press **LEARN CHORD** to store their relative intervals. Chord memory operates in Poly mode; use **PERFORM** to edit intervals or turn it off.
- **A / B** swaps the current patch with a comparison snapshot. Its initial comparison is Init. **UNDO** undoes parameter edits. **MUTATE** makes small continuous changes while leaving tuning selectors, routing selectors and master level alone.
- **SAVE / LOAD** use readable JSON patches. MIDI learn assignments and macro names are included. DAW project state stores the same format. Factory patches remain unchanged.
- **PANIC** immediately clears notes and effect tails. MIDI CC123 releases notes; CC120 silences/reset-clears the engine.
- Host tempo is used when available and **Follow host** is on. The standalone uses the internal BPM control. Detailed clock and synchronized rates are in **CLOCK +** and each section's controls.

## Sound architecture

- 16 polyphonic voices; mono and last-note-priority legato; glide, bend, pressure, wheel, velocity curves and sustain.
- Three oscillators per voice: saw, PWM pulse, triangle, sine and seven-saw hypersaw. Each supports 1–8 unison voices, detune, stereo spread, center/side blend, tuning, phase settings and four original coloration choices.
- Oscillator 2 hard sync, ring modulation, light oscillator FM, sub sine/square, white/pink noise, slow pitch drift and voice tolerance.
- Two independently enabled filters: Ember 24 ladder-style low-pass, Silk 12 multimode, Snarl 12 nonlinear resonant, Forge state-variable. Serial, parallel and source-split routing, drive, key tracking, velocity, filter envelope and oscillator FM.
- Three curved ADSRs, three LFOs with six shapes, key restart, fade, polarity and synchronized rates; 12 modulation routes; four assignable macros plus Warmth/Saturation.
- Six arp patterns, 1–4 octaves, swing, hold, gate length and a 16-step velocity pattern. A separate 16-step level/tie trance gate has depth, smoothing, shape and restart controls.
- Eight private effects: tube/tape/hard drive, chorus, flanger, phaser, dark stereo/ping-pong delay, algorithmic plate/hall-style reverb, low/high tone shelves and synchronized pump.
- 2x internal DSP and a 63-tap output downsampling filter; DC blocking, continuous-parameter smoothing, bounded feedback and a soft output safety ceiling near -6 dBFS. Reported latency: 16 host samples.

The character is original. These are efficient analog-inspired models, not replicas of any commercial instrument's circuitry or sounds. Read `docs/KNOWN-ISSUES.md` for the limits of the current build and `docs/VALIDATION.md` for what was actually tested.

## Windows build

Windows binaries are **not precompiled** in this Mac delivery. Complete build sources and the pinned JUCE dependency are included.

Requirements: Windows 10/11 x64, Visual Studio 2022 Build Tools with **Desktop development with C++** and a Windows SDK, plus CMake 3.22 or later.

```powershell
# Optional: install missing build tools through winget, then reopen PowerShell.
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1 -InstallTools

# Build, test the engine and processor, load-test the VST3, then collect binaries.
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1
```

You can also double-click `Build-Windows.bat` after installing the tools. Results go to `release-windows/Ember VA.exe` and `release-windows/Ember VA.vst3`. Run the supplied VST3 install script or copy the complete bundle into a DAW-supported VST3 location. Windows runtime/host compatibility has not been tested here.

Manual build:

```powershell
cmake -S . -B build-windows -G "Visual Studio 17 2022" -A x64
cmake --build build-windows --config Release --parallel
ctest --test-dir build-windows -C Release --output-on-failure
```

## Build on macOS

Install Apple's Command Line Tools (`xcode-select --install`) and CMake. The project includes JUCE 8.0.9 at commit `f72bad64d29715216226685810c5196bd0d79d77`. No Projucer, npm, web server or external font is used.

```sh
bash scripts/build-macos.sh
```

The script requests a universal arm64+x86_64 build. Set `EMBER_ARCHS=arm64` to build only Apple Silicon. The supplied prebuilt package is arm64 only; Intel builds are a build option, not a tested deliverable.

If `third_party/JUCE` is absent, CMake fetches the exact pinned commit. With the bundled JUCE present, dependency download is unnecessary. CMake's configure helper needs local executable/build permission.

## Source map

| Files | Responsibility |
|---|---|
| `sa-parameters.h/.cpp` | Single parameter schema, ranges, choices and IDs |
| `sa-dsp.h` | Sources, envelopes, TPT filter primitives and decimator |
| `sa-engine.h/.cpp` | Voice allocation, MIDI, routing, modulation and render loop |
| `sa-sequencer.h/.cpp` | Arpeggiator, held-note storage and trance gate |
| `sa-effects.h/.cpp` | Private stereo effect chain |
| `sa-processor.h/.cpp` | JUCE host wrapper, automation, keyboard, MIDI learn |
| `sa-patches.cpp` | Validated JSON, factory bank access, state, A/B, mutation |
| `sa-editor.h/.cpp` | Shared native face, controls, scope and full parameter inspector |
| `Presets/factory.json` | 114 original recipes plus Init, embedded in the binaries |
| `Tests/` | Engine, actual processor and independent VST3-loader tests |
| `scripts/` | Build/install helpers and reproducible preset-bank generator |
| `docs/` | Parameter schema, preset list, handoff, coverage and validation |
| `.github/workflows/build.yml` | Optional Windows/macOS CI; not published or run remotely |

The synth core is independent of JUCE and can be built alone:

```sh
cmake -S . -B build-core -DEMBER_BUILD_PLUGIN=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

See `THIRD-PARTY-NOTICES.md` and bundled license files for dependency terms. Source is included alongside the binaries.
