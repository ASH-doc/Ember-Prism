# Validation — 2 November 2026

## Build and binary checks

- Native macOS **arm64 Release** standalone app and VST3 built with Apple Clang 21, CMake 4.4.4 and bundled JUCE 8.0.9.
- Both final Mac bundles pass `codesign --verify --deep --strict` after ad-hoc signing.
- An independent JUCE VST3 host discovers and loads the **packaged** plugin, sends MIDI, checks finite/bounded stereo output, saves/restores host state and creates/destroys its actual editor. PASS.
- Windows and Intel Mac build configurations are supplied but were not executed.
- The standalone's normal desktop launch/audio-device path could not be verified: macOS Launch Services returned -10827 and direct execution aborted in `_RegisterApplication` before synth initialization. The computer-use service also failed to obtain a Finder window. The common processor/editor is covered by the tests below; this is not a substitute for the remaining desktop launch test.

## Engine regression

PASS: silence before notes, note generation, complete envelope release, 16-voice limit with stealing, panic clears tails, sustain pedal retention/release, mono legato return to held note, arp chord release, A4 zero-crossing frequency (440 ±1 Hz), all four filter models × all three routings × all four responses at extreme settings, finite/bounded output, unique parameter IDs, and all 264 modulation destinations under full-depth test input.

## State and preset checks

PASS: 115 factory entries (114 + Init), every patch validates and renders through the actual JUCE processor, parameter state survives JSON/DAW state round trips within 0.00001 of normalized range, metadata survives recall, invalid patch is rejected without changing the current patch.

Exact JSON text is not used as an equality criterion: floating-point host normalization may change the least significant digits while preserving the same setting.

## Interface checks

The compiled shared editor was created/destroyed and rendered at 1000 × 562, 1280 × 720 and 1600 × 900. Screenshots were inspected; compressed/ellipsized number readouts were corrected. Main face, six macro knobs, all 12 matrix rows, gate, effects, keys and detailed controls are native JUCE components. Desktop interaction could not be tested with the unavailable computer-use window service.

## Offline CPU benchmark

48 kHz output, fixed 2x internal DSP, 256-sample blocks, all three oscillators active, 16 held notes. Time to synthesize 1.003 seconds on this session's Apple Silicon machine; this is single-core offline render cost, not a measured live audio-device dropout threshold.

| Unison per oscillator | Internal oscillator lanes | Render time | One-core equivalent |
|---|---:|---:|---:|
| 1 | 48 | 0.228 s | 22.8% |
| 4 | 192 | 0.283 s | 28.2% |
| 7 | 336 | 0.333 s | 33.2% |

These figures are configuration-specific. Audio drivers, other plugins, higher rates, heavy modulation and triangle harmonic summation can change cost. The benchmark does not establish where glitches start on another laptop.

## Audio demo

`Preview/Ember-VA-audio-demo.wav` is a 36-second, 48 kHz, stereo, 24-bit offline render from the actual processor. Each six-second segment uses one factory recipe: Copper Horizon, Brass Droplet, Warehouse Lanterns, Copper Foundation, Walnut Sky and Copper Clockwork. No external mastering, samples or normalizer were applied.

Producer audition, physical MIDI/audio hardware playback, long sessions in a chosen DAW, high-register alias measurements, and calibrated self-oscillation tuning remain outstanding. See `KNOWN-ISSUES.md`; no claim of listening approval or all-host certification is made.

Raw logs are included alongside this report.
