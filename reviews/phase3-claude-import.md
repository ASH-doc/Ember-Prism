# Phase 3 — Claude EMBER presets imported; GPT's PRISM import measured (Claude)

Arash chose: **GPT's PRISM import + Claude's EMBER import.** Built on GPT's `72fd432` (`Map Claude Prism presets and wavetables into a compatibility bank`). Claude's own PRISM conversion and its table port (branch `claude/phase3-import`, `436a18e`) are **not** part of the integration; they stay in that local commit history only.
Environment of every result: Windows 11, MSVC 19.44, JUCE 8.0.9 vendored, Release x64. **Nothing was listened to. macOS, DAW: not run.**

## What is in the integration from Claude
- **EMBER:** 65 Claude presets (all but Init) converted to Synth A patches, appended after the 115 factory patches (bank = 180). Names end in `[C]` / `[C~]` / `[C?]` (close / approximate / different, see below), category `Claude <original category>`. Each patch carries a `claudeImport` record (map version, status, the distances, a note for every setting that could not be carried over, including the Claude values of any dropped modulation slot). A test confirms the 115 factory patches are byte-identical.
- **Mapping tool** (`Tools/ClaudeImport/ClaudeImport.cpp`, target `DualClaudeImport`): versioned map of parameter IDs, units, enum indexes, modulation routes (with depth converted from the Claude engine's scaling) and effects; level-matches each result against the Claude render and measures how close it sounds; also measures GPT's PRISM bank the same way (it never rewrites that bank). Inputs/outputs and how to re-run: `Tools/ClaudeImport/README.md`. The exporter that produced `claude-export.json` is in `Tools/ClaudeImport/claude-side/`.
- **Report:** `docs/claude-import-report.md` (every preset: status, score, spectrum / level / attack / release differences, gain trim, notes).

## How close are they (objective only)
Each preset was rendered by the Claude engine and by the ChatGPT engine (C3, velocity 0.8, 1 s hold), loudness-matched by the master level, and compared on a 31-band spectrum, loudness, attack time and release. For EMBER the tool also tried the filter cutoff up to one octave either way and kept the closest spectrum (45 of 127 presets in the earlier run moved; each move is in the patch notes).
| bank | close (score <= 4) | approximate (<= 9) | different (> 9) | total |
|---|---|---|---|---|
| EMBER (Claude's conversion) | 9 | 37 | 19 | 65 |
| PRISM (GPT's conversion, only level matched, including Init) | 6 | 43 | 14 | 63 |
For comparison, Claude's own PRISM conversion (not used) scored 7 / 41 / 14, so the two independent PRISM mappings are about equally close by this measure. "Close" means a small spectral and loudness difference on one note, not identical; the engines filter and band-limit differently. **Arash's ear decides which presets to keep.** The `[C?]` presets are the most likely to need hand tuning.

## Evidence (all RAN, Windows)
`DualTests` (default stack) PASS on GPT's tip plus this change: catalog (810 baseline, 18 added incl. GPT's Claude table selectors, 0 missing / changed), pre-Phase-2 golden render (difference exactly 0), GPT's PRISM import tests, and the new `claudeEmberImportTests` (bank size 115 + 65, factory patches unchanged, every imported EMBER patch loads under its stated name and makes finite, non-silent sound; 0 silent of 65).

## Decisions and limits for GPT / Arash
1. EMBER unison: Claude has one global unison; it is applied to all three oscillators. EMBER has no Pitch Bend modulation source and no amplitude-modulation destination; none of the 65 presets needed either.
2. Pitch modulation larger than +-100 cents cannot be expressed (fine-tune range); noted where it happened.
3. EMBER filter envelope / velocity depth were rescaled by the two engines' formulas (Claude env x5 octaves vs ChatGPT x6; velocity x2 vs x3). Pump release and the chord memory (session data in the Claude project) are not carried over.
4. Only C3 at velocity 0.8 was measured; other notes, velocities and long modulation movement were not compared. Only the filter cutoff was auto-calibrated.
5. Clean-clone build and macOS: not run by Claude.
