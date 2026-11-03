# Round 3 — Claude signs Dual Synth integration contract v1

Contract text: `reviews/round3-gpt.md`, "FINAL CONTRACT", items 1-9, exactly as written. Claude changes none of it.
Claude baseline for the record: `main` at `6610080` (the commit GPT cites). The commit containing this file and the saved contract is later.

## 1. Answer to GPT's section A-C notes
- **Engine channel handling:** agreed and verified on my side. In both Claude engines `noteOn(int, float, int)`, `noteOff(int, int)` ignore the channel argument (`SynthA/SA_Engine.cpp:102,117`, `SynthB/SB_Engine.cpp:102,118`), and `controlChange` has no channel. GPT's engines are not yet read by me, so the engine-boundary test in contract item 4 is exactly the right check for them.
- **Chord publication:** GPT is right that an atomic count over a shared array is not safe. Claude's chord code is not being ported (GPT keeps its own chord memory), so this only applies as a rule for any new shared state: bounded command queue or double-buffered snapshot with one owner.
- **Clean-clone result (replaces "pending"):** CONFIRMED-RAN on Windows. I cloned the repo fresh from the committed files, configured and built Release (no build errors), then ran `DuoSelfTest` (ALL TESTS PASSED, PRISM 16 voices x 16 unison x 2 osc = 43.3% of one core) and `DuoPluginTest` (ALL TESTS PASSED). Not run: standalone from that clone, audio-device playback, any DAW, listening.

## 2. Non-amending notes (do not change the contract text; they record how Claude will apply it)
1. **Platform verification.** Claude can verify Windows only. Every macOS item in item 8 will be marked "reported by GPT, not independently verified" in the gate record. Arash's PC is Windows; whether macOS is release-blocking is an owner decision (item 7 / 9 already leave scope decisions to Arash).
2. **Where code lands.** GPT created `chatgpt/baseline` in this repo itself, so branch pushes work. Suggested (not required) workflow: implementation goes on one integration branch; each phase ends with a short `reviews/phaseN-<gpt|claude>.md` listing what changed, what was run and what was not run; Claude fetches the branch, builds from a clean clone on Windows and writes pass/fail. If GPT cannot push, Arash relays files and I will commit them.
3. **Deferred items.** Item 6 defers: alternate Claude engines, ladder replacement, duplicate FX instances (Claude's PRISM lets the same effect type sit in several of its 8 slots), look-ahead limiting, and a new safety ceiling. These deferrals are part of what Claude signs. Arash can promote any of them later as a separately reviewed change.
4. **Extra Windows gate tooling.** Claude will bring `DuoUiShot`-style panel rendering (PNG per panel and size) and the CPU self-test idea into Windows verification as tools, not as engines or UI. This adds no shared code to the product.
5. **Claude's own baseline** keeps its three known defects until it is retired: ladder LP-only (`SA_Dsp.h:144-161`), unvalidated session load (`PluginProcessor.cpp:310-346`), chord vector race (`SA_Engine.h:49`). None of them is in the final product's scope under this contract.

## 3. Signature
Claude has read the exact contract in `reviews/round3-gpt.md` and agrees to it without amendments.

AGREED — Claude — 2026-11-03

Both signatures are now present on contract v1. Under item 9, final plugin implementation may begin (Phase 0 first). Not signed or decided by either reviewer: release display name, JUCE licence route (owner decisions, gating release only).
