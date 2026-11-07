# Spire to EMBER, checkpoint 1A: first measured results (2026-11-07)

Contract: `reviews/serum-spire-contract-v1.md`. Translator v0 (`Source/Import/SpireToEmber.cpp`), tool `DualSpireImport batch`. Sets drawn by `DualSurvey spire-select` (seed 20261107): 20 tuning presets (T01 to T20) and 20 held-out evaluation presets (E01 to E20) from Spire's factory presets. Preset names and files stay on Arash's machine; only the IDs and scores are recorded here.

## How it was measured
- Reference: each preset loaded in Spire 1.5 inside FL Studio 2025 and rendered as one held note (MIDI note 60, 4 beats at 130 BPM = 1.846 s, then release tail), WAV 32-bit float, no master effects. 40 renders.
- Candidate: the translated patch rendered in EMBER with the same note and hold.
- Score: the agreed objective score (`docs/import/evaluation-notes.md`; at most 4 close, at most 9 approximate, above 9 different).
- "Before" = default curves, loudness matched only. "After" = the tool also searched the filter cutoff (up to a few octaves) and the envelope time scale against the original's own render. **The "after" figure needs the original Spire render, so it only applies when the user can render the original; the "before" figure is what a plain import gives.**
- Not done: Arash's blind listening grades (contract requires them); dry and wet were not rendered separately; no mapping change was made after seeing the evaluation scores.

## Results
| | presets | Opened / Approximation / Blocked | close | approximate | different |
|---|---|---|---|---|---|
| Evaluation set (held out), after correction | 20 | 0 / 20 / 0 | 1 (E19, 2.5) | 3 (E02 8.5, E04 5.1, E10 6.6) | 16 |
| Tuning set, after correction | 20 | 0 / 20 / 0 | 0 | 2 (T02 4.8, T10 5.9) | 18 |
| Evaluation set, before correction | 20 | | 0 | 1 (E19 9.5) | 19 |

Median score, evaluation set: before 22.6, after 16.8. Best: E19 (2.5). Worst: E07 (44.6), E09 (30.1).

## Against the contract's pass criteria
- Decode: all 40 files parse. Met.
- Import status: at least 12 of 20 Opened or Approximation. Met (20 of 20 are Approximation; none is Opened because every preset has settings whose scale is not calibrated; none Blocked).
- Safety checks (34 malformed cases, random damage): met in the decoder tests; the apply transaction is GPT's and does not exist yet.
- **Fidelity: at least half of the playable presets close or approximate by both the objective score and Arash's grade. NOT met.** Objective: 4 of 20 on the held-out set (20%), and 5 of 40 over both sets. Arash's grades not collected.

## What the numbers say
- The default curves are far off: before correction the median score is above 20 and most presets are above 17.
- Correction helps but does not fix it, and it hits its limits: 12 of 40 presets reach the +6 dB master-volume ceiling (EMBER is still too quiet), and many cutoff shifts and time scales run to several octaves. That points to **systematic mapping errors, not just unit curves** (likely suspects: oscillator and unison loudness, the plain-wave substitutes for Spire's sampled waves, envelope time scale, filter response, Spire's analog drift and effects).
- Hard cases (oscillator 4, filter link, multi-target) are not clearly worse than ordinary ones in this sample (E19 is the best result, E17 and E20 are mid-range).

## Next steps (not started)
1. Take the first ten tuning presets apart by score component (spectrum, level, attack, release) to find which mapping dominates the error.
2. Calibrate level, waveform choice and envelope times from the oscillators and envelopes alone (one control at a time) before the filter.
3. Re-run the evaluation set once, unchanged, after the mapping changes, and report it as a new result next to this one.
4. Collect Arash's blind grades on a subset (A/B, level matched).