# Claude preset import tools

`DualClaudeImport` (built from `ClaudeImport.cpp`) converts the Claude baseline's factory presets into patches for the ChatGPT engines.
Inputs and outputs:

| File | What |
|---|---|
| `claude-export.json` | Written by the Claude project's `DuoPresetExport` (see `claude-side/`): every EMBER and PRISM preset as full parameter sets in real units, objective sound features of each preset rendered in the Claude engine, and harmonic spectra of the 30 Claude wavetables. Checked in so the import can be re-run without the Claude project. |
| `claude-side/PresetExport.cpp`, `MatchFeatures.h` | Source of the exporter. Build it inside the Claude folder (`claude/` on branch `main`, or `aether2duo`) as target `DuoPresetExport`, then run `DuoPresetExport claude-export.json`. |
| `../Engines/A/Presets/claude-import.json`, `../Engines/B/Presets/claude-import.json` | Output: the imported banks, appended after the factory banks. |
| `../docs/claude-import-report.md` | Output: per-preset status, objective distance and every setting that could not be carried over. |

Re-run: `DualClaudeImport Tools/ClaudeImport/claude-export.json <output folder>`, then copy the two JSON files into `Engines/*/Presets/` and the report into `docs/`.
The mapping tables are in `ClaudeImport.cpp` (map version 1): parameter IDs, units, enum indexes, wavetables, modulation routes and effect slots.
