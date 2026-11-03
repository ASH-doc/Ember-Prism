# Phase 0 and Phase 1 — implementation notes (Claude)

Branch `integration/dual-synth`, created from `chatgpt/baseline` at `f143191`. Contract: `reviews/round3-gpt.md` on `main`, signed by GPT and Claude 2026-11-03.
Assignment note: contract item 7 gave integration to GPT. Arash (owner) asked Claude to do Phase 0 and Phase 1 himself. GPT should review these commits the same way Claude reviewed GPT's baseline.
Environment of every result below: Windows 11, VS 2022 Build Tools (MSVC 19.44), JUCE 8.0.9 as vendored, Release x64, build in the worktree. **macOS: not run.** **DAW, listening, audio-device playback: not run.**

## Phase 0 — build completeness and Windows test crash
| Item | Change | Result |
|---|---|---|
| Missing JUCE files | `.gitignore` anchored to `/build/ /dist/ /work/`; 42 files of `third_party/JUCE/extras/Build/` added (taken from `DualSynth-Complete.zip`, whose sha256 matches). A comparison of the repo tree against the whole zip found nothing else missing. | Configure + Release build succeed from a checkout of the branch (commit `928a039`). |
| `DualTests` stack overflow | Every `DualProcessor` fixture in `Tests/HostTests.cpp` now lives on the heap. | `DualTests.exe` with the default 1 MB stack: exit 0, all original checks pass, including the sample-exact comparisons. |
Only `chatgpt/DualSynth` was fixed. `chatgpt/EmberVA` and `chatgpt/PrismFlux` still have the same unanchored ignore rules.

## Phase 1 — host hardening (all in `chatgpt/DualSynth`)
| Contract item | Change | Evidence (all RAN) |
|---|---|---|
| Parameter snapshots | `Host.h/.cpp`: all 27 host parameters resolved once in the constructor (`cached[]`, `get(Id)`); `routeMidi` and each render slice read them once. No name lookups or `String` construction on the audio thread. | `routeMidi`: 0 heap allocations over 200 calls with 40 events each. `processBlock`: 0 heap allocations over 200 steady-state blocks with both engines holding notes (includes engine code). An allocation counter self-check proves the counter works. |
| Callback storage / oversize blocks | Buffers are allocated in the constructor and in `prepareToPlay`; the audio thread never resizes. A block longer than `sliceCapacity()` (32768 or the prepared size) is rendered in slices: MIDI offsets rebased per slice, new `SliceTransport` advances PPQ / samples / seconds by the samples already rendered. | A 40000-sample block with a note at sample 35000: audio starts on exactly sample 35000 (A) and 35004 (B, with its 4-sample alignment); output is **sample-identical** to the same engines rendering one 40000-sample block. Transport shift unit-tested (offset 0 unchanged; +1 s at 120 BPM = +2 PPQ). |
| MIDI ownership | A repeated note-on of the same channel and pitch first sends a note-off to the previous owner(s) at the same sample, then takes fresh ownership (it used to OR the owner masks). CC120 / CC123 clear only their own channel's ownership (they used to clear all 16 channels). | New tests: repeat releases first (order and sample position checked); same pitch on two channels is independent; CC123 / CC120 on one channel leave other channels owned; routing change leaves note-off going to the original engine. All pass. |
| State publication | `DualProcessor::loadPerformance` returns a `juce::Result`. It validates the host tree, both base64 payloads, size limits (8 MB total, 2 MB per engine) and **both engine patches** before changing anything (`SynthAProcessor::setPatch(var, commit)` and `PrismProcessor::restore(var, commit)` got a `commit` flag; `false` stops after their existing validation). Only then does it apply. `setStateInformation` (DAW recall) keeps the reason in `lastLoadError()`. The editor's Load button shows a warning with the reason instead of failing silently. | Truncated data, garbage, empty, 8 MB+1, bad base64, missing engine payload, valid A with invalid B, out-of-range A: each is rejected and the processor state is **byte-identical** afterwards. A good file loads completely. |
| Size budget | Measured with the largest supported Prism patch (64-frame embedded wavetable at the 800000-character limit). | Synth B state 713,875 bytes; whole performance 963,110 bytes. Limits are 2,097,152 per engine and 8,388,608 total. The "valid patch can exceed 8 MB" worry from Round 1 is **not supported by this measurement** (Synth A's state was not stressed separately). |
| Embedded-editor mode | `SynthAEditor::setEmbedded(bool)` and `PrismEditor::setEmbedded(bool)` (inline, in `sa-editor.h` and `sb-ui.h`) hide each panel's own keyboard and wheels. `Editor.cpp` calls them; the position-based `hideKeys` (hid any slider with y >= 660) is gone. | Editor snapshots at 1280 and 1600 px, A and B: lower 55% of the image (keyboard, wheels, mixer strip) matches the baseline renders (0 to 3 differing pixels out of roughly half a million; the baseline differs from itself by about 70 pixels of animation in the top rows). |
| Load during playback | `loadDuringPlaybackTest`: 704 `loadPerformance` calls on one thread while another renders 601 blocks with held notes. | No crash, no hang, all output finite. **This is a smoke test, not a race detector** (none available on this toolchain). |

Original `DualTests` checks (identity with the original engines at unity gain, B delayed 4 samples, state recall, automation bridge, master FX finite, mute) all still PASS after the host rewrite. `DualProbe` on the built VST3: PASS (discovery, instantiate, MIDI/audio, state, editor). Standalone `Dual Synth.exe` stayed running for 6 s.

## Files changed
Host: `Source/Host.h`, `Source/Host.cpp`, `Source/Editor.cpp`. Tests: `Tests/HostTests.cpp`.
Engine wrappers (no DSP, no parameters, no presets touched): `Engines/A/Source/sa-processor.h`, `sa-patches.cpp`, `sa-editor.h`; `Engines/B/Source/sb-plugin.h`, `sb-patches.cpp`, `sb-ui.h`.
**`docs/ENGINE-INTEGRITY.json` and the README sentence "copied unchanged" are now out of date** for those six wrapper files. GPT owns the doc update; the contract's "no parameter ID, range, default or choice index changes" is respected (no parameter file was edited).

## Not done / open (deliberately not claimed)
1. **Coherent publication at a safe boundary.** `loadPerformance` validates atomically, but the final apply still sets engine parameters one by one from the message thread while the audio thread may render. A block can see a mix of old and new values for microseconds. Fixing that means staging in the engines and swapping at a block boundary (engine-side change). Not done.
2. Slicing re-bases PPQ, samples and seconds only; `ppqPositionOfLastBarStart`, loop and time-signature fields are passed through unchanged.
3. Engine MIDI-channel behaviour is not yet tested at the engine boundary (contract item 4); GPT's engines keep per-channel wheel / bend / pressure / sustain arrays, which looks right, but no test exercises it.
4. The 1300 bridged automation parameters were not touched or profiled.
5. Phase 1 in a real DAW and by ear: not run. macOS: not run.
6. `chatgpt/EmberVA` and `chatgpt/PrismFlux` ignore rules not fixed.
