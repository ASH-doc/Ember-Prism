# AEther v4.9 handoff for Claude

GPT owns product design, interaction specifications, technical diagnosis and Mac verification. Claude owns source implementation going forward and the Windows build and verification. This branch contains the Mac design reference that Arash auditioned. Windows and Mac should use the same JUCE implementation.

## Git handoff

- Design branch: `design/aether-v4.9`. Tested source: `9f5c36a`. The two implementation commits are `6d97c25` (EMBER workbench and compressor meter) and `9f5c36a` (graphite routing, mixer and Mac build configuration).
- The two commits are based on `491c248` on `integration/dual-synth`. At handoff the integration branch still points there. The v4.9 design can be fast-forwarded into a fresh local Windows branch from that base.
- If your working branch has advanced, fetch both refs and inspect the diff before applying anything. Bring the two commits onto a fresh branch based on your current Windows integration, for example with `git cherry-pick -x 6d97c25 9f5c36a`. Review overlaps in `Source/Editor.cpp`, `Source/Host.cpp`, `Source/Host.h`, `CMakeLists.txt` and `Tests/HostTests.cpp`. The separate import preview branch `feature/import-preview` touches those same files. Preserve the Spire work when resolving overlaps. Keep the feature branches separate until the combined build is tested.
- Build Windows x64 Release with the existing CMake targets `DualSynth_Standalone`, `DualSynth_VST3`, `DualTests`, `DualImportTests` and `DualProbe`. Update the Windows package script's artifact names if you use it. It still looks for `Dual Synth.exe` and `Dual Synth.vst3`, while CMake now emits `AEther v4.9.exe` and `AEther v4.9.vst3`. Keep plug-in code `Du49` so v4.9 can coexist with v4.1.

## Design from v4.0 to v4.9

| Version | Design to carry forward |
| --- | --- |
| **v4.0** | ChatGPT EMBER and PRISM panels, shared keyboard and graphite Control Bay. Layer, Split, A/B Only, independent note ranges and MIDI channels, and per-engine mixer controls. Sidebar master tabs for FET 76 or Glue, Reverb, and Tone. Preset browser by sound, including Signal Studies and the compatibility banks. |
| **v4.1** | EMBER's three dots open a warm Arp & Gate board with 16 aligned velocity, gate and tie steps. The two clocks stay independent. PRISM has large wavetable and filter views, a Classic View switch and three tempo-based motion lanes. The window scale is remembered. At Motion Amount 0% the old PRISM render is preserved. |
| **v4.9** | EMBER opens on the Workbench. Three stepped waveform selectors show shape previews. The larger filter bank has a response guide, dedicated Filter 1/2 selectors and ribbed Cutoff, Resonance and Drive knobs. The 12-route patchboard uses colored cables and signed depth. Advanced View restores the full original EMBER panel. The graphite Control Bay adds mode buttons, a live color-coded note map, a draggable split marker, named notes and Omni labels, vertical gain faders and centered pan rails. The compressor meter now holds short reduction peaks and labels bypass, dry mix and zero reduction. |

The newer views bind the existing parameter IDs. Preserve the Arp & Gate board, EMBER Advanced View, PRISM Classic View, and graphite styling. The filter response display is a visual guide. The Spire decoder and mapping checkpoint are separate from the v4.9 design changes.

## Mac playback diagnosis

**Reported by Arash:** clicks and sudden noises on the Mac build. The exact preset, host or device conditions and timestamp are still open.

- **Controlled renders match.** On Apple Silicon at 48 kHz, all 23 paired v4.1 (`491c248`) and v4.9 (`9f5c36a`) processor renders had identical sample hashes. Cases covered EMBER alone, PRISM alone and Layer, 64/128/256/512-sample blocks, Master FX bypass, FET and Glue, and heavy PRISM unison. The default held-chord cases produced finite output, zero silent held blocks and zero callback deadline misses. These are processor tests, independent of the live Core Audio device and Logic scheduling.
- **Recall can drop sound and MIDI.** `Source/Host.cpp` sets `publicationPending` during performance recall. A callback that sees it, or fails the publication try-lock, clears the whole buffer and MIDI. A paced test at 48 kHz / 64 samples with 20 concurrent valid recalls reproduced 5–8 fully silent note-on blocks per run in each version. Reproduce this independently and redesign publication. Stage validation and asset preparation off the audio thread, then adopt a complete prepared state at a block boundary while retaining the block and its MIDI. Keep any transition smoothing bounded, and keep host parameter notifications off the audio thread. Keep pending-state and asset reclamation deterministic. Add an audio continuity test and a note-off ownership test under repeated loads.
- **Heavy PRISM can miss deadlines.** With 16 voices and two oscillators at 16 unison voices each, both versions missed callback budgets. In one v4.9 run at 48 kHz / 64 samples, p95 was 1.34 ms against a 1.33 ms budget. A buffer increase may help in use. Profile the real callback and keep DSP changes gated by golden renders. Repeat with the editor closed, Workbench open and the window scaled, to separate audio load from repaint contention. Include real MIDI and host tempo or transport changes.
- **Live cause remains unconfirmed.** A system log contains a Logic audio engine overload/reset, but it predates the v4.9 build and does not establish the cause of Arash's report. If normal playback still clicks without recall and at ordinary voice counts, capture the patch, routing, Master FX state, host, sample rate, buffer, device and timestamp. Align a short recording against block-boundary drop counters, callback deadline misses, transport jumps and clipping. Run in standalone and Logic with the same MIDI to isolate the device or host path.

## Compressor meter diagnosis and behavior

With Master FX and Compressor enabled at 48 kHz / 256 samples, the same four-note chord passed audio in both engines with **0 dB** measured gain reduction at the default FET Input of **+6 dB**. At **+24 dB** the maximum was **12.63 dB** EMBER-only, **4.06 dB** PRISM-only, and **12.29 dB** layered. Glue at a **−36 dB** threshold reached **6.06 dB** layered. The Mac host tests also observed the held meter value and zero reading at 100% dry or with compression off. An audible signal and a stationary needle are therefore expected below the detector threshold. Whether the user's needle moves during actual gain reduction in Logic or standalone is still open.

Implement a clear, separate **detector/input** indication and **gain reduction** indication. The input indication should light when audio reaches the detector. Keep the needle at zero when gain reduction is zero. Label Master FX bypass, compressor bypass, 0% wet, and below threshold or no input separately when those states can be measured. Preserve the FET drive and Glue threshold defaults. Capture peak telemetry without audio-thread UI work, and make a driven note test verify both a nonzero detector reading and visible needle travel. Repeat with dry mix, bypass, and a quiet patch. For a quick manual check enable Master FX, keep FET Mix above 0%, play a chord and raise FET Input while monitoring output level.

## Acceptance

1. Run Windows Release CTest and load the actual VST3 in FL Studio. Confirm the original Windows PRISM golden at 0%, active Motion at 100%, original parameter IDs, preset recall, and no unplanned sound changes.
2. Check both EMBER views, the arp overlay, PRISM Signal and Classic views, the split map and faders, and the compressor with and without gain reduction. Save and reopen a test project with automation.
3. Record click and overload results for standalone and FL Studio at 64/128/256/512 samples. Include concurrent state recall and high-unison PRISM in the regression cases. GPT will rerun on Mac and check Logic after the implementation lands.
