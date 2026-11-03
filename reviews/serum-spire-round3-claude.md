# Serum / Spire import — Claude's evidence for round 3 of 5

2026-11-03, Arash's Windows machine. Aggregate numbers only: no third-party preset, parameter-name list, asset or render is in the repo. Raw output stays in the local scratchpad.

Tools added (code only): `Tools/SourceHost/SourceHost.cpp` (target `DualSourceHost`) hosts an installed VST3 and prints the plug-in's own parameter text; `Tools/SourceSurvey/Survey.cpp` (target `DualSurvey`) prints aggregate counts over a local library.

## 1. Can the Windows reference host render the originals?

| Plug-in | Loads in a headless JUCE host | Renders a note | Own parameter text | Notes |
|---|---|---|---|---|
| Serum 1 (1.3.5.7) | yes | yes (energy 805, peak 0.33) | **yes**, 2,397 parameters with real units | editor opens |
| Serum 2 (2.0.16) | yes (instrument and FX plug-ins) | yes (energy 675, peak 0.28) | 2,622 parameters listed | not yet checked for text quality |
| Spire 1.5 (1.5.15.0) | **no: access violation inside `Spire.vst3` while loading** (fault offset 0x88277), also when started from its own folder | – | – | Fallback: matched renders from FL Studio, or another loading route (not tried) |

Serum 1 accepts the chunk of a `.fxp` as its state (wrapped in JUCE's VST3 state wrapper): its state grew from 8,483 to 44,810 bytes and the audio changed. **Reading parameter values back after a load did not change** (all stayed at default), so the real plug-in can serve as an audio oracle but not yet as a value oracle. Unresolved: whether a message loop or the editor is needed.

## 2. Counts, with denominators and unknowns

**Spire** (`.spf2`): 72,084 files (user banks 71,005 + factory 1,079), 186 distinct vendors, **0 unreadable**, 722 parameters in every file.
- Enumerations are stored as **index / 999** (values 0.001001, 0.002002, ... are indexes 1, 2, ...). Seen in every enum-like field I inspected (oscillator type/wave, filter type/mode, mod source/target, step modulator mode).
- Oscillator feeding the mixer (`mix_oscN` non-zero; not the same as audible): osc1 97.9%, osc2 81.8%, osc3 51.2%, **osc4 25.1%** (18,084 presets). EMBER has 3 oscillators.
- Muted oscillators (`oscN_mute` = 1): osc1 0.6%, osc2 1.8%, osc3 3.8%, osc4 5.7%.
- Arpeggiator on: 10.7%. Filter link on: 37.3%. Filter routing switch set: 5.1%.
- Mod-matrix slots in use (source non-zero): slot 1 about 50%, slot 5 about 7%, slot 15 about 0.6%.
- Envelope 4 attack non-zero: about 5%.
- Unknown: **effect polarity.** `chr_mute`, `del_mute`, `rev_mute`, `phs_mute`, `shp_mute` are 0 in 93–97% of files and 1 in 3–7%. If 1 means "muted", almost every preset has every effect on, which is implausible; if 1 means "on", chorus is on in 7.2%, delay 7.3%, reverb 5.8%, phaser 7.3%, shaper 3.2%. Needs a controlled export before any effect is mapped. Same question for `eq_power` (1 in 86.6%).
- Unknown: the meaning of every enum index (filter types, oscillator types/waves, modulation source and target lists) and the unit curves of all continuous values.

**Serum 1** (`.fxp`): 16,427 files, 16,406 readable, **21 not in the expected `CcnK/FPCh` form** (counted separately), 0 from another plug-in.
- The first zlib stream of the chunk inflates to one of **only 10 sizes**: 28,232 bytes (12,090 presets, 74%), 33,872 (1,439), 34,016 (670), 20,704 (609), 21,528 (536), 21,524 (476), 172,736 (418), 21,808 (145), 34,012 (20), 21,804 (3). So the parameter block is a fixed-layout record with a few versions.
- Files can be far larger than their inflated first stream (a 1.2 MB file inflates to 21,528 bytes), so **extra data follows the first stream**. Hypothesis, not tested: that is where custom wavetables live. Nothing about table counts can be claimed yet.
- Unknown: field order of the record, wavetable storage, effect section, all enum meanings.

**Serum 2**: still one file. Nothing counted.

## 3. First source-unit mappings verified

From Serum 1's own display text at five normalised positions (0, .25, .5, .75, 1) on the real plug-in. Fitted formulas agree with all five readings; **not yet checked against held-out values**.
- Envelope times (Atk/Hold/Dec/Rel): seconds = 32 * x^5 (0 s, 31 ms, 1.00 s, 7.59 s, 32 s).
- Filter cutoff: Hz = 8 * 2761^x (8, 59, 425, 3063, 22090 Hz).
- Osc fine tune: -100 to +100 cents linear. Pan: -50 to +50 linear. Octave: -4 to +4. Coarse pitch: -64 to +64.
- Unison voices: round(1 + 15 * x) (1, 5, 9, 12, 16).
- Resonance and mix: 0–100% linear. Envelope sustain: dB (-oo, -24.1, -12.0, -5.0, 0.0).
- Filter type is a named list (about 96 entries, e.g. "MG Low 6", "HP 12", "Flg -", "Scream BP"); the host gives each name, so the type table can be read out, not guessed.
For Spire only the index / 999 enum encoding is established; no unit conversion yet.

## 4. Gap audit: Spire fields against EMBER (from field names; enum meanings unknown)

| Area | Spire 1.5 | EMBER now | Gap |
|---|---|---|---|
| Oscillators | 4 (type, wave, two controls, wavetable mix, unison count/density/detune/octave/width, analog flag, phase, pan, key, mix-to) | 3 (5 waves, 4 characters, 8-voice unison, sync, FM, ring) | osc 4 used by 25% of presets; wavetable oscillator types unknown |
| Filters | 2 (type and mode lists, cutoff, resonance, link, routing, balance, key track) | 2 (4 models x 4 responses, drive, FM) | filter type list may include types EMBER lacks (comb, formant): not yet known |
| Envelopes | 4, with attack/decay/sustain/release, curves, extra slope stage, 2 targets each | 3 ADSR plus curve | 4th envelope, slope stage, direct targets |
| LFOs | 4 (form, morph, symmetry, phase, fade, sync, mono, 2 targets each) | 3 (6 shapes, sync, fade) | 4th LFO, morph, symmetry |
| Matrix | 15 slots, each 2 sources and 4 targets | 12 routes | partly covered; slots above 5 used by under 7% |
| Step modulators | 2 x 16 steps with curves | gate lane and arp only | no equivalent |
| Effects | chorus, delay, reverb, phaser, shaper, EQ, plus global boost/warm/soft | drive, chorus, flanger, phaser, delay, reverb, EQ, pump | broadly covered once polarity and units are known |

## 5. Telling two custom Serum tables apart (plan)

Cannot be answered from the corpus alone. Needed fixtures (made by Arash in Serum 1, saved as `.fxp`): (1) Init; (2) Init with one custom WAV in oscillator A; (3) the same WAV in A and a different WAV in B; (4) the same WAV in A and B; (5) fixture 2 with the WAV file moved away before saving. Compare the bytes after the first stream across these; then check with held-out presets, and use the real Serum to render each fixture. Distinguishing shared from separate tables needs fixtures 3 and 4; an unresolved asset needs fixture 5.

## 6. Proposed next steps
1. Controlled Spire exports for effect polarity, each enum list and the main unit curves (needs either a way to load Spire in the host or FL Studio renders).
2. Resolve the Serum 1 record layout using the 16,406-file corpus and the real plug-in as an audio oracle.
3. EMBER extension decisions after item 1, using the corpus counts above.
