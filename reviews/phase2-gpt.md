# Phase 2 review and graphite integration

Reviewed `327dad3b574521569c79c28602d3152134bd6b09` on `integration/dual-synth` after reading `reviews/phase2-claude.md`. The graphite work uses that exact revision as its parent.

## Contract deviations

1. **Accept FET 76 as the extended compressor.** Arash requested the 76-style compressor. The fixed knee, output trim, parallel blend and switchable 90 Hz detector filter satisfy that direction. The needle and ratio selection stay. Treat the sound as a software approximation pending listening.
2. **Accept deferring independent reverb decay for this phase.** Freeverb Size couples decay and feedback behavior. An independently adjustable decay would require a deliberate reverb change, not a second knob for the same coefficient. Keep legacy Size and record independent decay as deferred.
3. **Accept FET 76 for new instances and Glue for pre-Phase-2 recalls.** Master FX still starts bypassed, and explicit compressor choices must survive recall. Verify the pre-Phase-2 render with the checked-in golden fixture.

## Diff findings

- **Confirmed by source inspection.** `fet.hpf` was exposed to the host and read by DSP, but no attachment bound it to a panel control in the original Phase 2 editor. The graphite version adds the 90 Hz SC switch for the existing ID. See `Source/Editor.cpp:36–41` and `Source/Host.cpp:173–174` at 327dad3.
- **Suspected audible click on automation.** The reverb send changes its delay tap immediately and switches the low-cut filter in or out near 20.5 Hz. Test under live automation and crossfade delay taps and filter engagement if the discontinuity is audible. See `Source/MasterDsp.h`, ReverbInputStage.
- **Confirmed by source inspection, audio impact unverified.** The gain-reduction meter receives the wet path's reduction while Master FX is bypassed. The wet path still computes before the final blend. The meter can therefore show compression on an inaudible path. Decide whether to label it as detector preview or show zero when bypassed. See `Source/Host.cpp:170–189` and `Source/Editor.cpp:80` at 327dad3.
- **Suspected transition artifact.** Switching compressor models selects one path immediately. Their envelopes do not advance together and may resume with old detector history. Test an in-flight model switch under sustained audio before deciding on a reset or crossfade. See `Source/Host.cpp:170–180` at 327dad3.

## Graphite change list

- Painted a black Control Bay nameplate, charcoal chassis with recessed sections, numbered headers, and separate mixer-strip divider in `Editor.cpp`.
- Restyled existing knobs, toggles, ratio buttons and stereo bars in `EditorWidgets.h`. Preserved the cream analog needle and all COMP/REVERB/TONE controls. Kept the two synth editors intact.
- Added one switch component bound to existing `fet.hpf`. Moved Master FX slightly right to align with the Master Glue header. All other control bounds and attachments stay in place.
- No new parameter IDs. The 16 Phase 2 IDs and all 810 baseline IDs are retained. No DSP changes in the graphite commit.
- Recomputed packaged hashes in `docs/ENGINE-INTEGRITY.json` against the current engine sources. Original hashes remain the comparison baseline.

## Mac verification

- Clean arm64 Release configure and full build passed on exact commit `327dad3`. DualProbe passed on the built VST3. The exact DualTests run stopped at the catalog comparison with 15 continuous numeric text differences, including 0 versus 0.0000018 for the master default. No IDs were missing.
- The graphite follow-up keeps the original catalog and golden fixture. Its catalog check tolerates small float-normalization differences only for continuous defaults and midpoints. IDs, names, metadata, range endpoints, and choices retain exact comparisons. Both Mac suites passed after this correction. Catalog result: 810 original, 16 added by Phase 2, zero missing or changed. Golden audio maximum absolute error was 4.84288e-7, within the existing 1e-4 tolerance. The existing bypass test also reproduced 12.7181 dB of reported GR with Master FX off.
- Standalone built. LaunchServices returned -10827 and direct execution aborted at macOS application registration before synth startup. Device playback remains unverified in this session.
- Native screenshots were inspected for Ember, Prism, and the three master tabs. Windows verification of the graphite follow-up is for Claude.
