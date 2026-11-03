# Windows verification of 240f29e (GPT's Phase 1 follow-up) — Claude

Commit: `240f29e` on `integration/dual-synth`. Environment: Windows 11, VS 2022 Build Tools (MSVC 19.44), JUCE 8.0.9 vendored, Release x64, built in a worktree checked out from the branch.

## Results (all RAN)
| Check | Result |
|---|---|
| Build `DualTests`, `DualSynth_Standalone`, `DualSynth_VST3`, `DualProbe` | Succeeds, no compile errors |
| `DualTests` on the default 1 MB stack | PASS, including GPT's new engine-channel and publication regressions and all earlier checks (sample-exact bypass, slicing, state rejection, allocation counters = 0) |
| `DualProbe` on the built VST3 | PASS (discovery, instantiate, MIDI/audio, finite output, state, editor) |
| Standalone launch | stayed running for 6 s |
Not run: DAW, listening, audio-device playback, macOS (GPT reports Mac arm64 passing).

## Code review of the diff (read the whole diff)
Looks correct (read, plus the passing tests):
- Lock order is consistent: `loadPerformance` takes `recallLock`, then `publicationLock`; `getStateInformation` takes only `recallLock`; `processBlock` only *tries* `publicationLock`. No inversion, and the audio thread never waits.
- `publicationPending` is set only inside the commit step, after validation, so a failed validation never blocks audio. The `ScopeGuard` is declared before the optional lock, so the lock is released first and the pending flag cleared second.
- CC120/CC123 now clear held keys, heldOrder, sustain, generated/arp notes and voices of the addressed channel only, in both engines. CC120 hard-stops voices, CC123 releases them. Mono "resume the newest held note" now looks across channels and only triggers when the released voice was on that channel.

Observations to decide on (not bugs in the tests, but behaviour GPT/Arash should know about):
1. **Dropped input during publication.** While a recall is publishing, `processBlock` returns silence and clears the incoming MIDI buffer. Note-ons arriving in that window are lost, not delayed. GPT documents "may briefly interrupt audio"; the lost-MIDI part is not mentioned. It lasts as long as one engine restore takes (milliseconds to tens of milliseconds, not measured).
2. **Skipped bookkeeping in that window.** The early return happens before `panic.exchange`, the on-screen keyboard processing and the bend/wheel exchange, so those are processed in the first block after publication, not lost.
3. `loadPerformance` holds `recallLock` for the whole recall, so a DAW state save during a recall waits for it. Fine on the message thread; worth remembering if any other thread ever calls `getStateInformation`.
4. The engine files `sa-engine.cpp`, `sa-sequencer.cpp`, `sb-engine.cpp`, `sb-motion.cpp/.h` now differ from the original standalone engines (they were "unchanged" in the baseline). `docs/ENGINE-INTEGRITY.json` should list them. I did not check whether the standalone EmberVA / PrismFlux projects should get the same CC120/123 fix.
5. My Phase 1 test `loadDuringPlaybackTest` now runs only 60 loads against ~40,000 audio blocks, because blocks during publication return almost instantly. It still passes, but its stress on the publication path is lower than before; a loop that keeps loading for a fixed time instead of a fixed block count would restore it. I have not changed it.

Phase 1 open items in my earlier notes: items 1 (coherent publication) and 3 (engine channel handling) are now addressed by this commit; items 2, 4, 5, 6 are unchanged.
