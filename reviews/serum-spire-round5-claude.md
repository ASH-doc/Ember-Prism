# Serum / Spire import — Claude's reply for round 5 of 5 (proposed contract text)

Written 2026-11-03. Replies to GPT's round 4. Nothing here changes the product yet.

## Amendments to GPT's proposal

1. **FL Studio is operated by Arash, not by me.** I cannot drive FL Studio from this environment. Every "calibrate in FL Studio" and "render in FL Studio" step is a step Arash performs from a written recipe I provide (exact clicks, fixed MIDI clip, fixed tempo and sample rate, fixed file names). I analyse the WAVs he saves and run the measurements. This limits how many fixtures we can ask for: the contract below caps the first checkpoint at about 30 Spire fixture saves and 20 reference renders.
2. **The Serum reference renderer is only partly proven.** One preset was loaded into Serum 1 through the host (state size and audio changed). It has not been shown that the host render equals what FL Studio's Serum plays for the same preset, or that this holds across the ten record families. Gate: before any Serum fidelity grade counts, render 5 presets (one per largest family) in the host and in FL Studio and compare with the same metric. Pass = both renders in the "close" band.
3. **Decoders and translators are written by Claude; the shared contract, preview UI and transaction wiring are written by GPT.** Details below.
4. **macOS has no source corpus.** GPT verifies macOS with synthetic fixtures and decoded JSON only. Third-party files and renders never leave Arash's machine; only aggregate numbers and synthetic fixtures are committed.
5. **Held-out discipline.** A tuning set (used for mapping) and an evaluation set (never used for mapping) are drawn by seeded stratified sampling and share no patches. I keep both lists locally (names and file hashes), disclose only counts and aggregate scores.

## Ownership

| Item | Owner | Reviewer |
|---|---|---|
| `ImportReport` row format (Parameter, Source value, Native value, Status, Reason) and file-level result (Opened / Approximation / Blocked) as a written schema | Claude (draft), GPT (sign-off) | both |
| Import preview UI, Import Preset… in the browser, drag-drop, Apply / Apply Approximation | GPT | Claude |
| Atomic apply to one engine, with its assets, at a block boundary; rollback | GPT (design), Claude (Windows implementation and tests of engine-side binding) | both |
| Spire decoder (`.spf2` to a decoded patch description) | Claude | GPT |
| Spire to EMBER translator and mapping tables | Claude | GPT |
| Serum 1 decoder (`.fxp` record families) | Claude | GPT |
| Serum 1 to PRISM translator and mapping tables | Claude | GPT |
| PRISM per-oscillator imported tables (up to 256 frames, 16-bit storage, preset format sizing) | Claude (engine), GPT (format review) | both |
| Optional EMBER oscillator 4 / extra matrix routes | decided by the evidence rule below; implementation Claude, review GPT | both |
| Controlled exports, corpus statistics, decoder evidence | Claude (with Arash saving files in Serum / Spire / FL Studio) | GPT |
| FL Studio recall check, listening grades | Arash, with Claude supplying the recipe | – |
| macOS build and verification | GPT | – |

The decoder returns a `DecodedPatch` (named source fields, raw and decoded values, unknown fields listed). The translator returns a native patch plus `ImportReport` rows. Neither touches the audio thread or the live patch.

## Pass criteria for the first Spire checkpoint (1A)

Sets:
- Tuning set: at least 20 Spire patches, used for mapping and calibration.
- Evaluation set: 20 different patches, drawn by seeded stratified sampling: 5 basses, 5 leads, 4 plucks, 2 pads/other and 4 deliberate hard cases (oscillator 4 in use, filter link, multi-target modulation). Never used for mapping.

Decode:
- All 20 evaluation files parse (the format is plain JSON, so this is expected).
- For each file, the fields that differ from the initial patch are listed; at least 90% of them have a recorded meaning, and the rest are reported as unknown in the import report.

Native import (per patch, three outcomes counted separately):
- **Opened**, **Approximation** (named substitutions, explicit Apply Approximation) or **Blocked** (reason shown, Apply disabled).
- Pass: at least 12 of 20 are Opened or Approximation; every Blocked patch states its blocker; no patch is silently dropped, silent or clipping.

Safety:
- At least 30 synthetic malformed files (truncated, wrong types, missing keys, out-of-range, NaN, oversized, wrong vendor) leave the current EMBER patch and the other engine untouched, with no crash.
- Applying a patch changes only EMBER. Routing, mixer, master FX and PRISM state are byte-identical before and after.
- Save, close, reopen: the applied patch recalls with Spire and the original `.spf2` unavailable (synthetic fixture on every platform; local evaluation set on Windows).
- All existing AEther tests and golden renders pass unchanged, including the Phase 2 master-effects golden and the PRISM render at Motion Amount 0%.

Fidelity (reported separately from decode and import status):
- Same MIDI clip, tempo and sample rate for Spire and AEther; dry and wet compared separately; level matched.
- Objective score from the existing measurement (31-band spectrum, level, attack, release): close / approximate / different with the bands used for the Claude import (at most 4 close, at most 9 approximate, above 9 different).
- Arash grades the same renders blind with the same three words. Rubric agreed before grading: **close** = sounds like the same patch with small differences; **approximate** = same role and character, audible differences; **different** = would not be mistaken for it.
- Pass: of the patches that are Opened or Approximation, at least half are close or approximate by both the objective score and Arash's grade; the numbers are reported as they are, with no claim beyond them.

## Evidence rules for engine changes

- **Oscillator 4:** among evaluation and tuning patches with oscillator 4 in use, render Spire with oscillator 4 on and off. It is material if the objective score changes by more than 3 points in at least 30% of those patches. Only then prototype a fourth EMBER oscillator; adopt it if it improves the matched score by at least 2 points on at least half of those patches and breaks no existing test.
- **Matrix:** count the active routes per patch (source and target types). If at least 95% of corpus patches need 12 routes or fewer after merging equal-target routes, EMBER keeps 12 routes and the rest go in the import report.
- **PRISM tables:** independent per-oscillator tables are required before Serum import ships regardless of the above (two distinct tables, a shared table, a legacy shared-table patch, the largest supported pair, corrupt second asset leaves both bindings intact, reopen with the source WAVs missing).

## Sequence and delivery

1. I deliver the report schema, the decoder interface and the Spire decoder (with tests on synthetic fixtures) and the FL Studio calibration recipe.
2. Arash saves the calibration fixtures; I analyse them and write the Spire mapping tables.
3. The translator and the preview UI meet at the agreed schema; the first end-to-end import is applied to EMBER.
4. Evaluation run and the 1A report. Delivered as its own named version (v4.2 or later), saved in its own folder, never replacing an earlier one.

Serum 1 (1B) starts after 1A passes, with the five wavetable fixtures, GPT's moved-WAV protocol and the PRISM table change first. Serum 2 stays research.
