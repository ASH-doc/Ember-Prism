# AETHER DUO — Claude version (hand-off for cross-review)

Two-synth instrument, JUCE 8.0.17, VST3 + Standalone, C++20.
Build: `build.ps1` (CMake + VS2022). Tests: `DuoSelfTest`, `DuoPluginTest`. UI renders: `DuoUiShot` (writes PNGs).

## Architecture rules (please keep or argue against)
1. **Total separation** between the two synths. Only `Source/Host/SynthContract.h` and the host are shared.
   No shared DSP, UI helpers or presets. UI helper classes are duplicated per synth on purpose.
2. **EMBER** = Synth A, namespace `sa` / `emberui`, parameter IDs `sa_*`. Warm subtractive virtual-analog.
3. **PRISM** = Synth B, namespace `sb` / `prismui`, parameter IDs `sb_*`. Clean wavetable.
4. Host owns the mixer, master, panel switching, session/DAW state.
5. Parameters through APVTS. UI is plain JUCE components, no external UI libraries.

## Where things are
| Area | Path |
|---|---|
| Contract between host and synths | `Source/Host/SynthContract.h` |
| Host editor (switcher, bars) | `Source/ui/HostEditor.cpp` |
| EMBER engine / UI | `Source/SynthA/` , `Source/SynthA/ui/SA_Panel.cpp` |
| PRISM engine / UI | `Source/SynthB/` , `Source/SynthB/ui/SB_Panel.cpp` |
| Tests | `Tests/` |

## What was verified (and what was not)
Verified by running: full build, DuoSelfTest, DuoPluginTest (panel switching, sessions, DAW state, VST3 load),
standalone starts and survives 5 s, UI screenshots rendered (`screenshots/`).
**Not run:** manual click-through, real-mouse drag and drop, use inside a DAW, listening tests.

## Honest gaps the reviewer should look for
- Mockup-style features that do not exist in the Claude engines: Import WAV, Mutate, A/B, Undo, Snapshot, Swap,
  12 EMBER mod routes, ten named macros, per-oscillator unison in EMBER.
- UI is Claude's design (wood/amber EMBER, cyan/violet PRISM). The owner prefers the GPT design — the merge
  should keep the GPT look and bring in the Claude features.
