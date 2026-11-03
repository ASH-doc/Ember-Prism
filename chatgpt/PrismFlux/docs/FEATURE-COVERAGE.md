# Roadmap coverage

The attached Synth B PDF is a product specification, not a higher-priority instruction to stop at Phase 0 or use a browser. The user's direct request selects C++/JUCE, native standalone/VST3 and full implementation. The optional future combined host/mixer is outside this instrument.

| Roadmap area | Delivered implementation | Main files |
|---|---|---|
| Independent architecture | Native `SynthB` engine, `sb-` files, one parameter schema, stereo MIDI instrument; no dependency on the other authored synth | sb-schema, sb-engine, sb-plugin |
| Wavetable synthesis | Two oscillators, 30 original tables, 6 families, 64 frames, 2048 base samples, 10 mips, cubic sample interpolation and frame/mip interpolation | sb-tables |
| Imports | Single-cycle and 512/1024/2048 strip WAV import; max64 frames; embedded JSON asset | sb-patches |
| Warp and unison | Two warp stages, 11 active warp choices plus None, 1–16 lanes/oscillator, detune curves, width, blend, phase, random phase and WT spread | sb-engine, sb-tables |
| Voice and sources | 16 voices, release-aware stealing, 2ms start fade, mono/legato/glide, sub, colored noise, velocity, bend, sustain and pressure | sb-engine |
| Filters | LP12, LP24, HP, BP, notch, comb, vowel, air shelf, low shelf; drive, key/velocity/env tracking; serial, parallel, per oscillator and both routes | sb-filters, sb-engine |
| Modulation | 3 AHDSR with curves; 4 drawable16-point LFOs with sync, interpolation, reset and fade; random and S&H; 24 source/destination/depth/bipolar/curve rows | sb-motion, sb-engine |
| Macros | 8 renameable routed macros plus Euphoria and Space, each available as a source and destination | sb-schema, sb-engine, sb-rack |
| Performance | 6-mode arp and 16-step sequencer, per-step pitch/gate/velocity, swing, latch, rates, length, octaves, restart; internal/host tempo | sb-motion |
| Randomizer | Six locks: oscillators, filters, envelopes, modulation, sequence, effects | sb-patches |
| Effects | 10 modules, bypass and wet/dry, unique reorder, per-module settings; octave shimmer; delay ping-pong/cross, drive/wavefold/bitcrush | sb-rack |
| Presets and patches | 116 + Init; exact category counts; search/category/tags; full state JSON, macro names, MIDI maps, embedded asset; invalid patch validation | sb-patches, generate-bank.py, factory.json |
| Native UI | Same editor in standalone and VST3; navy/glass/aqua/violet; two stack views, rings/source chips, draggable filter handles, editable LFO/sequence, scrollable matrix, spectrum/voice/unison indicators | sb-ui, sb-ui-motion |
| Utility | Panic, MIDI learn, undo, snapshot/swap, searchable519-parameter inspector, resizable16:9 surface | sb-plugin, sb-ui |
| Delivery | Mac arm64 app/VST3, full C++/JUCE, Windows build/install scripts, Mac universal-build option, tests and manual | CMakeLists.txt, scripts, Tests, Mac |

See KNOWN-LIMITATIONS.md for the exact differences from aspirational quality targets and optional ideas. In particular, no browser implementation or continuous snapshot morph engine is claimed.
