# Phase 4 — release gate record (contract item 8)

Revision under test: branch `integration/dual-synth`, commit `a600249c02748f6dfd336672cff0329e709b2509` (GPT's Phase 3, Signal Studies and graphite sidebar merged with Claude's EMBER import). Record written by Claude after the Windows clean-clone gate.

| Gate (contract item 8) | Windows | macOS | Owner | Status |
|---|---|---|---|---|
| Clean-clone Release build, standalone + VST3 | **PASS** — fresh `git clone --depth 1` of GitHub `a600249`, configured and built from scratch (MSVC 19.44, JUCE 8.0.9, Release x64); the 42 JUCE build files are present | PASS reported by GPT (arm64) | Claude / GPT | done |
| Host tests on the default stack | **PASS** (`DualTests`, from the clean clone) | PASS reported by GPT | Claude / GPT | done |
| Original-engine bypass comparison (sample-exact) | **PASS** | PASS reported by GPT | GPT test, Claude re-ran | done |
| VST3 discovery / render / state / editor probe | **PASS** (`DualProbe` on the clean-clone VST3) | PASS reported by GPT | Claude / GPT | done |
| Standalone launch | alive after 6 s | LaunchServices -10827, unverified (GPT) | Claude / GPT | **open on Mac** |
| Standalone **device playback** | not run | not run | Arash | **open** |
| Overlapping channels, repeated notes, sustain, bend, pressure, panic, CC120/123, routing changes, mute/solo with held notes | **PASS** (host and engine tests) | PASS reported | Claude / GPT | done |
| Callback allocations and slice boundaries | **PASS** (0 allocations in `routeMidi` and steady-state `processBlock`; 40000-sample block renders sample-identical to one block) | not run by Claude | Claude | done on Windows |
| Malformed and oversized sessions, atomic rejection | **PASS** | PASS reported | Claude | done |
| Legacy recall (pre-Phase-2 states) | **PASS** (state without the new controls loads on the baseline Glue compressor; golden master-effects render difference exactly 0) | PASS reported | Claude | done |
| Imported assets and preset mapping | **PASS** (Claude PRISM bank 63, Claude EMBER bank 65, Signal Studies 16: all load, finite, non-silent; factory presets unchanged) | PASS reported | GPT / Claude | done (objective only) |
| Editor switching under playback | **PASS** (identity test switches panels during playback) | PASS reported | GPT | done |
| Parameter catalog frozen | **PASS** (810 baseline, 18 added, 0 missing / changed) | PASS reported | Claude | done |
| **Real DAW** save / reopen / automation pass | not run | not run | **Arash** | **open** |
| **Matched listening** for new DSP and imported presets | not run | not run | **Arash** | **open** |
| Display name | — | — | **Arash** | **open** (working name "Dual Synth") |
| JUCE licence route (commercial vs open-source terms) | — | — | **Arash** | **open** (gates any distribution) |
| `docs/ENGINE-INTEGRITY.json` refresh after the bank merge | — | — | GPT | **open** (GPT reported it) |

## What Arash needs to do (nothing below has been done by Claude or GPT)
1. Install the Windows VST3 and standalone from the clean-clone build (folder `Desktop\Dual-Synth-Windows-a600249`) and load it in the DAW.
2. DAW pass: insert the instrument, play both synths in Layer and Split; save the project, close the DAW, reopen it; check the sound and the panel come back. Automate a few parameters (for example master level, a Prism source selector, FET input) and play the project back. Watch for dropouts, clicks or high CPU.
3. Listen: FET 76 (all five ratios, Master FX on and off, Glue for comparison); the 65 + 63 Claude presets (`[C]` first, then `[C~]`, then `[C?]`); Signal Studies. Report which presets to fix, keep or drop.
4. Decide the display name and the JUCE licence route.

## Known behaviour to be aware of during the DAW pass
- Loading a saved state while playing briefly outputs silence and drops incoming notes in that moment.
- Prism's modulation source list gained a 25th entry (Pitch Bend); a DAW that stored automation of those source selectors as a normalised value could map to a different source.
- An instance keeps at most 32 distinct imported wavetables (the Claude PRISM tables count against this).
- Compressor type switching (Glue / FET 76) is a hard switch, not a crossfade.

## Working name and install (added after the first Windows install)
Arash asked for the working name **AEther v4.0** and for the VST3 in `C:\Program Files\VST`. The plugin's registered name, the standalone window title, the editor title and `getName()` now read AEther v4.0 (`PRODUCT_NAME` in `CMakeLists.txt`). The bundle ID, manufacturer code and plugin code are unchanged, so the plugin identity for recall is the same as before; no project had been saved with the earlier `Dual Synth` build. Installed by Claude: `C:\Program Files\VST\AEther v4.0.vst3` (byte-identical to the tested build; Windows asked for administrator approval). The earlier per-user test copy `Dual Synth.vst3` was removed so only one plugin shows up. Re-tested on the renamed build: `DualTests` PASS, `DualProbe` PASS, standalone alive, plugin name AEther v4.0 in the module info. `scripts/*` and `Build-Windows.bat` still mention `Dual Synth.vst3`; they were not changed.

