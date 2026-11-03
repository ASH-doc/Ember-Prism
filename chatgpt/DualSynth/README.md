# Dual Synth

Ember VA (Synth A) and Prism Flux (Synth B) inside one native C++/JUCE standalone instrument and VST3. A thin host provides shared MIDI, a shared keyboard, routing, a two-channel mixer and an optional master effects section.

## Start

The `Mac` folder contains the compiled Apple Silicon app and VST3. Run `Install-Mac.command`, or manually copy the app to `~/Applications` and the full VST3 bundle to `~/Library/Audio/Plug-Ins/VST3`. Rescan in an Apple Silicon DAW. In standalone, choose your audio and MIDI devices from the JUCE Options menu. The binaries target macOS11+, are ad-hoc signed, and are not notarized. Intel/Rosetta hosts require an Intel/universal rebuild.

## Performance controls

- **A / Ember VA** and **B / Prism Flux** switch the displayed native panel. Both instruments keep processing while hidden, muted, or receiving no new notes. Switching panels does not recall a preset or reset audio.
- **Layer** sends notes to both engines. **Split** sends notes below the split point to A, and the split note and above to B. **A only / B only** sends new notes to one engine.
- Each engine also has an inclusive low/high MIDI note range and a channel filter:0 means all channels. These intersect with the selected routing mode. Reversed low/high ranges receive no new notes.
- Key-up follows the engine(s) that received key-down, even if you change routing while holding a key. Controllers are sent to both engines so sustain releases, wheel and expression remain coherent. Notes retain their MIDI channel, pitch, velocity and sample offset. No transpose or velocity remapping is added.
- The shared on-screen keyboard and its pitch/mod wheels feed the same routing as an external MIDI controller. These GUI wheels send channel1. Original per-engine keyboard widgets are hidden in the host; synth sound controls stay intact.
- **A / B output dB** and **Mute** act after each synth's complete internal effects. Muting smoothly fades output, while the synth continues processing notes and releases.
- **Master** is the final output level. Default channel levels are−6dB each; master is0dB. There is no automatic normalization, limiter or saturation in the dry path. The output meter turns red above0dBFS; lower levels if needed.

## Master effects

**MASTER FX is OFF by default.** With it off, the original engine outputs are mixed without master color processing. At unity mixer/master gains, A output matches its original processor sample for sample; B matches its original output with a four-sample alignment delay. A reports16samples of latency and B12, so the host aligns B to A and reports16samples to the DAW. This is timing compensation, not a change to either synthesis algorithm.

The optional section processes only the summed stereo output: a stereo-linked compressor (threshold, ratio, attack, release), algorithmic stereo reverb (size, damping, mix), stereo width, and optional low/air tone EQ. Individual Comp/Verb/EQ switches let you choose the glue. Width1 and EQ0dB are neutral. Master bypass crossfades smoothly to avoid an abrupt discontinuity. Effects can intentionally change the final mixed sound when enabled; neither engine's own patch, DSP or effects are edited by these controls.

## Mixer, routing and master additions (Phase 2)

- **Mixer:** each strip has level, pan (centre is exactly unity), mute, **solo** and a stereo level bar; the output has its own stereo bar. Muted and solo-excluded engines keep processing.
- **Routing:** **Swap** puts A on the high keys and B on the low keys in Split mode. **TAP** sets tempo from your taps (average of up to 8, restarts after 2 s) on both engines, A or B (menu next to it) and turns their *Follow host* off.
- **Master compressor:** choose **FET 76** or **Glue** (the original compressor, unchanged). FET 76 is a fast peak compressor in the style of classic FET limiting amplifiers: Input drives the signal into a fixed threshold, Output trims, Attack 20-800 us, Release 50-1100 ms, ratio buttons 4 / 8 / 12 / 20 and ALL, parallel Mix and a 90 Hz sidechain filter. An analog-style needle meter shows gain reduction for both types. It adds no latency. New instances start on FET 76; performances saved before Phase 2 reload on Glue.
- **Reverb:** added **pre-delay** (0-100 ms) and **low cut** (20-600 Hz) on the reverb send; at their defaults the reverb is bit-for-bit what it was.
- The master section has three tabs: COMP, REVERB and TONE (EQ and width). **Master FX is still off by default.**
- FET 76 values were set by measurement, not by ear. See `reviews/phase2-claude.md` for the numbers and for what was not tested.

## Presets, automation and recall

Both synths retain their own original preset banks, patch import/export, macros and editors. `Save performance` saves both complete engine states, host routing/mixer/effects, MIDI maps, embedded Prism wavetable assets, and the selected panel as one `.dualsynth` file. Your DAW project stores the same complete state. Ordinary MIDI keys and active effect tails are not serialized.

All original synth parameters are published to the DAW under `A.` / `B.` prefixes through forwarding bridges. Their original parameters and ranges remain authoritative. Host mixer/routing/effects parameters are exposed separately. Child preset recalls and GUI gestures notify the corresponding parent automation parameter. Dense automation and CC maps should still be checked in your particular DAW.

## Build

JUCE8.0.9 and both full engine sources are bundled. No separate UI library installation is needed.

Mac, with CMake3.22+ and Xcode Command Line Tools:

```sh
./scripts/build-macos.sh
# Intel + Apple Silicon:
DUAL_ARCHS='arm64;x86_64' ./scripts/build-macos.sh
```

Windows: install CMake and Visual Studio2022 Desktop development with C++, then run `Build-Windows.bat`. `Build-Windows.bat -InstallTools` uses winget to install those tools if needed. Results go to `dist/Windows`; the PowerShell installer copies the VST3 into the current user's plugin folder. Windows and universal builds have not been executed here.

See `docs/VALIDATION.md` for exact verification scope and `docs/ENGINE-INTEGRITY.json` for source comparisons. Both engine DSP implementations and presets are copied unchanged from their delivered projects. Only wrapper binary-data include/namespace and factory entrypoint symbols are adapted to coexist in one link target. The original standalone packages are preserved separately.

Original authored files retain MIT licensing; JUCE and SDK components retain their applicable licenses. Full source, GNU license texts and dependency notices are included. See `LICENSE`, `licenses/`, and `third_party/JUCE/LICENSE.md`.
