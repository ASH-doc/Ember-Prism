# Spire 1.5 calibration findings (first session, 2026-11-03)

Obtained by driving Spire inside FL Studio (computer use, Arash's machine) and reading the saved `.spf2` files. Only facts about the file format and Spire's own lists are recorded here. No third-party preset content. The calibration files themselves stay on Arash's machine (`%APPDATA%\RevealSound\Banks\AEther calibration`).

## How the fixtures are made
Spire's own library browser has a Save button. It writes a plain-text `.spf2` straight into `%APPDATA%\RevealSound\Banks\<folder>`, so each fixture can be read the moment it is saved. The INIT button loads Spire's initial patch. (If a typed file name has no `.spf2` the file is saved without an extension; type it.)

## Established
- **`*_mute` flags**: 1 = muted. Saved fixture S02 has all five effect MUTE buttons lit red and `chr_mute`, `del_mute`, `phs_mute`, `rev_mute`, `shp_mute` all changed 0 to 1 against the initial patch S01. The earlier library statistics therefore mean that 93 to 97% of presets leave each effect not muted; they say nothing about the wet level.
- **`eq_power`**: 1 = EQ on (switching the EQ on changed 0 to 1).
- **Initial patch (S01)**, the baseline for "fields that differ from default": 722 fields; oscillator 1 type Classic, wave Sin, mix 0.5, the other three oscillator mixes 0; no oscillator muted; filter 1 type `perfecto` mode LP4 cutoff 1.0 resonance 0; filter 2 off; effects not muted, all wet values 0; EQ off.
- **List choices are stored as index / 999 with index 0 first.** Confirmed against the screen: filter 1 type `perfecto` is list item 1 and stored as 0.001001; `Classic` and `Sin` are item 0 and stored as 0.
- **Spire's own displays are not in real units.** The manual says knobs run from 0 to 1000 (centre 500); the LCD shows such numbers. Hz, seconds and cents for cutoff, envelopes and so on are *not* available from Spire's display. They have to be calibrated from audio (render, measure) against the evaluation score, as was done for the Claude import. The planned "36 numbers" step of the recipe is dropped.

## List contents (item order as shown by Spire; index = position, first item 0)
- **Filter type** (7): off, perfecto, acido, infecto, scorpio, combo, shaper.
- **Filter mode** (for `perfecto`): LP4, BP4, HP4, Peak. Other filter types probably have different mode lists (not yet captured).
- **Oscillator type** (7): Classic, Noise, FM, AMSync, SawPWM, HardFM, Vowel.
- **Oscillator wave** (49, read column by column, top to bottom): Sin, Tri, TriSine, Para 1-4, Bass 1-4, Piano A1-A4, Violin 1-3, Oboe, Sitar, Bayan 1-2, Guitar 1-3, Organ 1-3, Brass 1-2, Vocal 1-6, Robo 1-3, Synth 1-3, Inter 1-2, FM 1-3, Alarm 1-2. Only index 0 (Sin) is confirmed against a saved file; the column order for the rest is an assumption to verify with one more fixture (choose Bass 1, expect 7/999 = 0.007007).

## Second session: more lists (items in the order Spire shows them; index counts from 0)
- **Unison voices** (9): 1 Voice to 9 Voices. **Unison interval** (18): 1 Octave, 2 Octave, 3 Octave, Major3rd, Minor3rd, Major7th, Dom7th, Min-Maj7th, Minor7th, Minor9th, Major9th, Dom9th, Half-Dim7th, Dim7th, Sus2, Sus4, Perfect5th, Perfect4th.
- **LFO form** (52): Sin, Tri, Sqr, SH, SG, then the oscillator waves from TriSine onward in the same order (TriSine, Para 1-4, Bass 1-4, Piano A1-A4, Violin 1-3, Oboe, Sitar, Bayan 1-2, Guitar 1-3, Organ 1-3, Brass 1-2, Vocal 1-6, Robo 1-3, Synth 1-3, Inter 1-2, FM 1-3, Alarm 1-2). **Confirmed:** Tri = 1 (saved fixture S01: lfo1_form = 1/999).
- **Tempo divisions** for LFO sync and arpeggiator rate (21), read column by column: 16/1, 8/1, 4/1, 2/1, 1/1, 3/4, 2/3, 1/2, 3/8, 1/3, 1/4, 3/16, 1/6, 1/8, 3/32, 1/12, 1/16, 3/64, 1/24, 1/32, 1/64. **Confirmed:** 1/16 = 16 (S01 rp_speed = 16/999). The LFO's list shows these only when SYNC is on; with SYNC off the LFO time box reads Free.
- **Arpeggiator mode** (10): Up, Down, Up/Down, Up/Down2, Down/Up, Down/Up2, Random, Ordered, Step, Chord. **Arpeggiator velocity** (5): Key, Hold, Step, Step+Key, Step+Hold.
- **Modulation source** (33 entries shown): off, Osc1-4, Env1-4, LFO1-4, Velocity, KeyFollow, Random Bipolar, Random Unipolar, PitchWheel, ModWheel, Aftertouch, FootPedal, Breath, Expression, Sustain, Stepper1, Stepper2, ArpStep, Mod1-4, Constant 10%, Constant 50%. **Index anchors confirmed from saved files:** Env1 = 5, LFO1 = 9, Velocity = 13, PitchWheel = 16, ModWheel = 17, Stepper1 = 23, Constant 50% = 32. Counting the visible entries does not reproduce 16 and 32, so one entry between Velocity and PitchWheel and one between Stepper1 and Constant 50% are not what they look like; the indexes of KeyFollow, the two Random entries, Stepper2, ArpStep, Mod1-4 and Constant 10% are therefore **not yet known** and need two more fixtures.
- **Modulation target**: a two-level menu (off, OscMixer, All Osc Pitch, Osc1-4, Filter, Env1-4, All LFO Rate, LFO1-4, Shaper, Phaser, Chorus, Delay, Reverb, MasterOut, Slot1-15, Arp, Stepper, Macro, Other, each with its own sub-list). Not yet mapped to indexes; the corpus shows which indexes matter (index 45 is the target of about 23% of presets), so the first fixtures will pick those.
- **Matrix layout**: 15 slots on 5 pages (3 slots per page), each slot has 2 sources and 4 targets with an amount fader each.

## Saving behaviour to remember
Spire writes edits into the currently loaded library preset when another preset action happens (S02 was overwritten by the later slot-source edits; its original mute/EQ state is also contained in it). Use Create with a new name to freeze each state, and read the file right after.

## Not captured yet
Step-modulator modes, the mode lists of the other filter types, the modulation target indexes, the order of the oscillator waves beyond Sin, oscillator 2 to 4 behaviour, and every continuous unit curve.
