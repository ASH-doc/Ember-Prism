# Validation — 3 November 2026

Tested here: Release, arm64, Apple Clang, JUCE8.0.9, macOS27.2 build26B123. Deployment target11.0; compatibility with that oldest target was not tested on an actual macOS11 machine. Final native build completed successfully. Warnings are JUCE test-target macro redefinition and deprecated WAV writer API use, not build errors.

## Automated results

All three registered CTest suites passed:

1. **Core**: 519 unique parameter IDs; 30 tables with64frames and ten complete finite mip levels; audible frame variation; silent initialization; notes, release,16-voice stealing, panic clearing, sustain, legato return; nine filter types across four routes at extreme settings;12warp choices; arp/seq start/release; imported sine A4 frequency; every parameter as a modulation destination remains finite.
2. **Native**: all117patches validate, render nonzero bounded audio and retain parameter/metadata values through native state; malformed recall is rejected before application; locked randomizer preserves the oscillator section; modulation creation, FX reorder, duplicate rack recall, sparse descriptive modulation/FX import; four WAV frame modes, embedded table recall;44.1/48/96kHz and varied1/31/127/512/1024sample buffers; editor creation and snapshots.
3. **VST3Binary**: independent host loads the actual VST3 bundle, discovers the instrument, sends MIDI, checks finite nonzero stereo, saves/restores host state and creates/renders its editor. Repeated against the **copied, signed bundle in Mac/** after packaging.

Logs are in `docs/validation`. The final source is the source used for the delivered binaries. A manual visual inspection covered the native editor at1000/1280width and all lower editing tabs;1600width was also rendered automatically. Preview PNGs are real native component snapshots, not a web mockup.

The42second demo renders seven factory categories, six seconds each: Pluck, Lead, Bass, Pad, Arp, Key, Vocal. `Preview/audio-analysis.json` records levels. Signal statistics are not a subjective listening assessment.

## Performance measurement

Core test:16notes, two oscillators,48kHz,256sample blocks, approximately1.003seconds of offline audio; the engine includes the2x path and effect rack. Last isolated wall times:

| Unison lanes per oscillator | Wall time | Offline time / audio time |
|---|---:|---:|
|1|0.230s|22.9%|
|4|0.360s|35.9%|
|8|0.549s|54.7%|
|16|0.927s|92.4%|

Tables generated in0.302seconds, once per process. These are single-run local offline measurements, not guaranteed realtime CPU/load numbers or comparisons against another synthesizer. Maximum unison leaves little headroom on this test configuration.

## Binary and launch checks

Both supplied Mac bundles contain arm64 Mach-O executables. `codesign --verify --deep --strict` passed for the app and VST3. Dependencies resolve to macOS system libraries/frameworks; no local JUCE shared library is required. They are ad-hoc signed and not notarized.

**Standalone desktop launch was not verified.** LaunchServices returned -10827 (kLSNoExecutableErr) even though the executable and bundle metadata are present and valid. Direct execution aborted in macOS `_RegisterApplication` / `NSApplication` initialization, before synth initialization. The relevant stack is recorded in `validation/standalone-registration.txt`. This is consistent with restrictions on GUI startup in this session, but it is not proof that the app will launch successfully on every desktop. Native processor/editor validation does not substitute for audio-device playback testing.

No Windows compiler or Windows machine was available. Windows CMake configuration, PowerShell build/install scripts, batch entrypoint and optional GitHub workflow are supplied; a Windows build/run is not claimed. Mac shell scripts passed `bash -n`; the universal/Intel option has not been built here.

## Quick hands-on acceptance

- Open standalone, select your output and MIDI input, play presets and confirm stereo output without dropouts at a comfortable buffer.
- Insert the VST3 on a MIDI instrument track; play notes, automate Position/Euphoria/Space, save/reopen the project and compare state.
- Drag LFO1 to Position; adjust depth/curve in Matrix; confirm the ring and sound move. Draw an LFO and edit pitch/gate/velocity steps.
- Try WAV import with a known single cycle. Save JSON, load another preset, then reopen the JSON and verify the waveform returns.
- Reorder effects and check bypass. Raise Space with reverb enabled; release notes and check the decay. Use Panic to clear all sound.
- Compare high notes, extreme warps,16-note chords and high unison by ear. Reduce unison if the audio meter/dropouts indicate insufficient headroom.
