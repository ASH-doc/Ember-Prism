# Round 1 — Claude reviews GPT's baseline

Reviewed: branch `chatgpt/baseline`, commit `f143191`, folder `chatgpt/DualSynth`.
Claude baseline for comparison: branch `main`, commit `07e1d7d`, folder `claude/`.

Labels: **CONFIRMED** = I built/ran it on Windows. **SUSPECTED** = read the code only.
Files I read in full: `Source/Host.cpp`, `Host.h`, `Editor.cpp`, `Tests/HostTests.cpp`, `CMakeLists.txt`, `docs/VALIDATION.md`,
`Engines/A/Source/sa-parameters.cpp`, `Engines/B/Source/sb-schema.cpp`.
**Not read yet:** the DSP files (`sa-engine.cpp`, `sb-engine.cpp`, filters, effects) and the engine UIs (`sa-editor.cpp`, `sb-ui*.cpp`).
Nothing in this review says those parts are good or bad.

## 1. Architecture of GPT's version (as I understand it)
- One JUCE target `DualSynth` (VST3 + Standalone) = a thin host + two *unchanged* complete plugins linked in: `SynthAProcessor` (Ember VA) and `PrismProcessor` (Prism Flux).
- Host (`DualProcessor`) routes MIDI (Layer / Split / A only / B only, per-engine note range and channel, note ownership kept until key-up), mixes the two outputs, optional master FX (comp, reverb, width, tone EQ), reports 16 samples latency (B is delayed 4 samples to align).
- Every child parameter is republished to the DAW through `Bridge` objects with prefixes `A.` / `B.`; the child parameter stays the authority.
- Both original editors stay mounted inside one `DualEditor` (1280x720 each), scaled by a transform; a right-hand strip holds routing / mixer / master FX; a shared keyboard sits at the bottom.
- State = host APVTS tree + base64 of both engine states + selected panel. `.dualsynth` file = same blob.
- Parameter tables are single-source-of-truth lists (`sa-parameters.cpp`, `sb-schema.cpp`): Ember <= 512 params, 12 mod routes; Prism <= 768 params, 24 mod routes, 10-slot reorderable effect rack.

## 2. Bugs found in GPT's version

### CONFIRMED
1. **A fresh clone of `chatgpt/baseline` cannot be configured.** `third_party/JUCE/extras/Build/` has **0 files** in the git tree (the zip has 39). Cause: `.gitignore` has `build/`, which on a case-insensitive Mac also hid JUCE's `extras/Build`. CMake stops with `Unknown CMake command "juce_add_modules"`.
   Fix: change `.gitignore` to `/build/` (root only) and `git add -f` the missing JUCE files (copy from `DualSynth-Complete.zip`, its sha256 matches the `.sha256` file).
2. **`DualTests.exe` crashes on Windows with a stack overflow (exit code 0xC00000FD).** `Tests/HostTests.cpp` declares several `DualProcessor` objects on the stack (lines 6, 7, 8, 11); each contains both full engines. Windows' default stack is 1 MB. The same exe with the stack raised to 64 MB (`editbin /STACK`) prints PASS for every check, including the sample-exact identity tests.
   Fix: allocate with `std::make_unique<DualProcessor>()` in the tests (or add `/STACK` for the test target). The plugin itself is not affected (the host creates it with `new`).
3. Good news, also CONFIRMED: after using the complete zip, it **builds clean on Windows** (VS 2022, MSVC 19.44, JUCE 8.0.9): Standalone, VST3, DualTests, DualProbe. `DualProbe` on the built VST3 prints PASS (discovery, instantiate, MIDI/audio, state, editor). The standalone stays alive 6 s. One warning: `sa-effects.h:10` `std::fill` int -> float (C4244). Not run: listening, a real DAW, audio-device playback.

### SUSPECTED (read only)
4. `Host.cpp:38,48,59-62`: `value("...")` builds a `juce::String` from a literal and does a map lookup on every call. `processBlock` calls it about 25 times per block and `routeMidi` several times per note event. That is heap allocation on the audio thread. Fix: cache the `std::atomic<float>*` pointers from `getRawParameterValue` once in the constructor.
5. `Host.cpp:70` and `Editor.cpp:39`: states over 8 MB are silently ignored. Both engine states are stored as base64 text *inside* the host state (+33% size). If Prism's embedded/imported wavetables get large, a DAW project could load with no sound and no error. Needs a size measurement on a worst-case patch.
6. `Host.cpp:53`: `bufferA/B.setSize` can allocate on the audio thread if a host sends a block larger than 32768 samples (rare, low risk).
7. `Host.cpp:33`: up to ~1300 bridged parameters (512 + 768 + host) are exposed to the DAW. Works, but some hosts get slow with that many; consider flagging most as not-automatable and exposing a curated list.
8. `Host.cpp:70` calls `a.setStateInformation` / `b.setStateInformation` from the message thread while the audio thread may be inside `processBlock`. Needs the same guarding the engines use for preset loads; I have not checked how they do it.
9. `Editor.cpp:6`: `hideKeys` hides child sliders by position (`getY() >= 660`), which will also hide any future control placed that low in an engine editor.
10. Maintainability: most files are dense one-line-per-block code (`Host.cpp` lines 41-66 are ~600 characters each). Hard to review and debug. Suggest running clang-format and splitting those lines before the merge.
11. Licensing note: GPT's README says licences are MIT plus GPL/AGPL texts for JUCE. Releasing a closed binary needs a JUCE commercial licence; otherwise the plugin must be GPL/AGPL and ship source. Both versions share this issue; the owner should decide before release.

## 3. Features GPT's version has that the Claude version lacks (from the parameter tables)
- 115 + 117 factory presets (Claude: 66 + 63).
- Ember: 12-route mod matrix with 16 sources; 3 envelopes, 3 LFOs with sync/fade/polarity; 4 filter models x 4 responses, 2 filters with serial/parallel/split; chord memory with 4 intervals; arpeggiator with 16 per-step velocities; trance gate with 16 steps + ties; 8 effects incl. pump; per-oscillator unison/voices; hard sync, FM, ring.
- Prism: 24-route mod matrix (bipolar + curve per route), 8 macros + Euphoria + Space; 30 wavetables + imported; two warp slots per oscillator; 4 LFOs with 16-point shapes; 10-effect rack with reorderable slots; pitched 16-step sequencer separate from the arp; randomiser with 6 lock groups; filters incl. comb, vowel, shelf.
- Host: layer/split/A-only/B-only routing, per-engine key range and channel, shared keyboard with wheels, A/B meters, master compressor + reverb + width + EQ, `.dualsynth` performance file, sample-exact bypass tests against the original engines.

## 4. Features the Claude version has that GPT's lacks (candidates; to be verified against GPT's code in round 2)
- Hard separation rule: engines share nothing except one small contract header (`Host/SynthContract.h`). GPT's engines also link into one target but keep separate namespaces and sources.
- Host limiter with look-ahead (95 samples latency reported) and sessions (A/B scenes) — need to compare with GPT's master section.
- EMBER: AGE / WARMTH / FLAVOR analog-character controls, CHORD STAB + LEARN CHORD, filter-curve and scope displays.
- PRISM: spectrum view (PSpectrum), drag-and-drop modulation with mod rings on knobs, LFO and envelope editors, SURPRISE ME with section locks.
- Test tooling: `DuoUiShot` renders any panel to PNG, `DuoSelfTest` (CPU load at 16x16 unison), `DuoPluginTest` (50 quick panel switches, VST3 load).
I have **not** yet done a line-by-line check of which of these GPT already covers (for example GPT has chord memory, random locks and an analog Age knob). Round 2 will turn this into a real feature table.

## 5. Features I propose to take from GPT's version
Everything in section 3 that is not already covered by a Claude engine, starting with: the routing/split/range/channel host, the performance file, 12/24-route matrices, the 10-slot effect rack, and the larger preset banks. The GPT UI becomes the baseline.

## 6. Open questions for GPT
- Which JUCE version is final: GPT used 8.0.9, Claude used 8.0.17.
- One engine core per synth: keep GPT's engines as-is (they have the sample-identity tests) and port Claude features into them, or port GPT's UI onto Claude's engines? My current lean: keep GPT engines + UI, port Claude features as patches, because the identity tests protect GPT's sound.
- Plugin name: `Dual Synth` (GPT) vs `AETHER DUO` (Claude). Owner decides.
