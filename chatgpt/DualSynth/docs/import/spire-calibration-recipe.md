# Spire calibration recipe — step 1 (for Arash, about 45 minutes)

> **Superseded 2026-11-03.** Arash enabled computer use, so Claude now makes these fixtures in FL Studio itself (see `spire-calibration-findings.md`). Part A is done, most of Part B is being captured from the screen, and Part C is dropped because Spire shows 0 to 1000 values rather than Hz or seconds. This recipe stays as the manual fallback.


Why: the Spire preset files are readable, but they store plain numbers like 0.62. To turn them into real sounds in EMBER I need to know what the numbers mean (which effects are on, what each list item is called, how a number becomes Hz or seconds). Spire is the only thing that can tell me. I can't run Spire myself (it crashes when I load it), so this step is done by you in FL Studio.

Everything you save stays on your computer. Nothing goes to GitHub.

Contract limit for the whole first checkpoint: about 30 saves and 20 renders. This step uses **6 saves, 7 screenshots and 36 numbers**. Render comparisons come later, with a separate recipe.

## Before you start
1. Make a folder on your Desktop called **Spire calibration**.
2. In FL Studio load **one fresh Spire** (a new instance, no preset chosen, nothing changed).
3. Tell me if anything below does not exist in your Spire (a button with a different name, no Save button, and so on). A screenshot is enough. Do not guess; skipping a step is better than a wrong step.

## Part A: three saves about the effects (10 minutes)
Spire has an effects section (chorus, delay, reverb, phaser, shaper, EQ). I need to learn whether a stored 1 means "on" or "off".

- **S01-default**: with the fresh Spire, do nothing at all and save the preset as `S01-default` into the Spire calibration folder.
- **S02-fx-on**: turn **every** effect ON (chorus, delay, reverb, phaser, shaper, EQ), so you can see each one lit or active. Save as `S02-fx-on`.
- **S03-fx-off**: turn **every** effect OFF. Save as `S03-fx-off`.

## Part B: seven screenshots of the lists (10 minutes)
Open each drop-down list so all its items are visible (scroll and take two screenshots if it does not fit) and take a screenshot. Name them as shown. Put them in the same folder.

1. `L1-osc-type`: oscillator 1 **type** list
2. `L2-osc-wave`: oscillator 1 **wave** list (with the default type)
3. `L3-filter-type`: filter 1 **type** list
4. `L4-filter-mode`: filter 1 **mode** list
5. `L5-lfo-form`: LFO 1 **form** (shape) list
6. `L6-arp-mode`: arpeggiator **mode** list
7. `L7-mod-sources`: the list of modulation **sources** in the first modulation slot

## Part C: three saves about numbers (25 minutes)
For these twelve controls I need Spire's own display of the value at three positions: all the way down, in the middle, all the way up. Use the same preset for all three saves: start from the fresh Spire.

The twelve controls:
1. Filter 1 **cutoff**
2. Filter 1 **resonance**
3. Envelope 1 **attack**
4. Envelope 1 **decay**
5. Envelope 1 **sustain**
6. Envelope 1 **release**
7. LFO 1 **rate** (with tempo sync **off** if there is a sync switch)
8. Oscillator 1 **fine tune**
9. Oscillator 1 **octave**
10. Oscillator 1 **unison detune**
11. Oscillator 1 **mix / volume** in the mixer
12. **Master volume**

- **S10-low**: turn all twelve all the way **down** (minimum). Write the number or text Spire shows for each (hover or drag on the knob; it usually shows the value). Save as `S10-low`.
- **S11-mid**: put all twelve exactly in the **middle** (double-click often resets; otherwise use the position marker). Write the shown values. Save as `S11-mid`.
- **S12-high**: all twelve all the way **up**. Write the shown values. Save as `S12-high`.

Write the numbers into `calibration-answers.txt` in the same folder, like this (one line per control, three values):

```
filter1 cutoff:    low = ____    mid = ____    high = ____
filter1 resonance: low = ____    mid = ____    high = ____
env1 attack:       low = ____    mid = ____    high = ____
env1 decay:        low = ____    mid = ____    high = ____
env1 sustain:      low = ____    mid = ____    high = ____
env1 release:      low = ____    mid = ____    high = ____
lfo1 rate:         low = ____    mid = ____    high = ____
osc1 fine:         low = ____    mid = ____    high = ____
osc1 octave:       low = ____    mid = ____    high = ____
osc1 unison det:   low = ____    mid = ____    high = ____
osc1 mix:          low = ____    mid = ____    high = ____
master volume:     low = ____    mid = ____    high = ____
```
Include the unit Spire shows (Hz, ms, s, dB, %, st, cents). If a control shows no number, write `no number shown`.

## When you are done
Tell me "calibration saved". I read the six files and your numbers, work out the meanings, write the Spire mapping table, and show you what I found before any of it is used. I will also tell you if a part did not work and what to repeat.

## What this does not do
- No sound comparison yet. That is the next recipe (reference renders with a fixed MIDI note), and it needs the translator first.
- It does not teach me every setting, only the ones that matter most for the first 20 presets. More fixtures can follow within the cap.
