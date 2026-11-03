# Phase 3 — Claude Prism compatibility bank

Integrated on top of 0cfae41, including Claude's master FX follow-up. Source preset and wavetable recipes are from Claude's baseline at 07e1d7d14dce7551b4732e98fe63ee281a22a604.

In Prism, select **Claude Compatibility** in the category menu. All 63 Claude patches, including Init, follow the original 117 factory patches. Each keeps its source category as a searchable tag. The original factory bank file and preset indexes are preserved.

Use **Edit → Claude Table** on each oscillator to select a Claude recipe or return to the original Table control. The waveform view labels the active Claude table. A WAV import selects the existing Imported slot and clears that oscillator's Claude override. The 30 Claude waveform recipes use 32 frames and Prism's existing interpolation and mipmap path. The original oscillator table IDs, choices, normalization and Imported index are preserved. The only added IDs are `B.osc1.claude.table` and `B.osc2.claude.table`. They default to Use original table and are appended after existing parameters. Existing modulation destination indexes remain valid.

The compatibility bank uses explicit source/destination, units, enum, step, table and effect mappings. The raw source values and drawn LFO points are retained in `Engines/B/Presets/claude-prism-source.tsv`. The generated bank records the source commit and preset index. To regenerate from the DualSynth project folder, run:

    python3 scripts/map-claude-prism.py ../PrismFlux/docs/parameters.json

The mapped patches approximate the original sound. Envelope curves, modulation depth scaling, filter and effect algorithms, and timing differ between engines. Short step patterns are repeated across Prism's 16 steps. Combined AM/ring maps to AM, and Claude LFO sample-and-hold routes use Prism's shared sample-and-hold source. Effects map to Prism's unique modules. Duplicate enabled slots, if present in source data, are recorded and only the first instance is used. Listening and level matching against Claude's reference renders remain pending.

Mac arm64 Release HostIntegration and ActualVST3 passed. Tests check all 30 table identities and finite signal, all 63 preset loads and audible signal, original-bank index retention, performance recall, source catalog compatibility, and the original bypass and golden renders. Windows verification is pending. No graphite sidebar or host design files changed.
