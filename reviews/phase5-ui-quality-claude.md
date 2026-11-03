# Phase 5 — arp board, PRISM Signal view, Motion lanes, size and meter (Claude)

Branch `claude/ui-quality` (from `integration/dual-synth` 7ea5e00). Implements GPT's designs from `design/ember-arp-board` and `design/prism-signal-motion` (bcd2c60), and fixes two things Arash reported from FL Studio: the dead gain-reduction needle and a blurry, hard-to-size editor.

Everything below was built and tested on **Windows x64 (MSVC 19.44, JUCE 8.0.9)** with `DualTests`, `DualProbe` and a 6-second standalone launch. **Nothing was run in FL Studio.** The FL Studio checks GPT listed are still Arash's.

## What changed

### Gain-reduction needle
The needle was not broken. It reads zero while **Master FX** is off, and Master FX is off by default (baseline default, left alone). The meter now says "MASTER FX OFF / click to switch on" and one click turns Master FX on. Screenshot: `phase5-screenshots/host-master-fx-off.png`.

### Window size and sharpness
- Default is exactly 100% (1600 x 840), where text is hinted and sharpest. It used to open at 90% (1440 x 756), which scales every glyph.
- New size menu (90 / 100 / 125 / 150 / 200%) bottom right; free resizing now goes up to 3200 x 1680 (was 1920).
- The chosen width is stored with the project (`uiWidth` on the host state) and restored on reopen. A new instance opens at 100%.
- The UI is vector drawing, so it stays sharp at any size; the 125 / 150 / 200% settings make it larger without the smallest text becoming unreadable. Not changed: font sizes inside the two engine panels (8–10 px at 100%).

### EMBER — Arp & Gate board
- The three dots beside the compact ARP strip open an in-panel board (overlay on the EMBER panel; the host sidebar is untouched). Close button and Esc close it; focus returns to the three dots.
- Sections: 01 arpeggiator (`arp.on/mode/rate/hold/octaves/swing/gate`), 02 trance gate (`seq.gate.on/rate/depth/smooth/shape/restart`).
- One 16-column matrix for velocity (`arp.velocityN`), gate level (`seq.gate.stepN`), ties (`seq.gate.tieN`), grouped in fours. Drag to draw (interpolates across skipped steps), double-click resets a step, arrow keys move/change (Shift = fine), `V G T` pick a lane, Space toggles a tie.
- All edits go through `SynthAProcessor::setParam` (gesture, undo, automation, preset refresh). No new parameter IDs, no DSP change. Arp and gate keep their own clocks.
- Not done: the live step indicator (GPT noted it needs separate telemetry).

### PRISM — Signal view (default) and Classic view
- A toggle (`CLASSIC VIEW` / `SIGNAL VIEW`) next to the browser buttons. Signal view covers the macro row, source chips, envelope/LFO/matrix panel and spectrum; the rack and keyboard stay. **Trade-off:** macros and drag-to-modulate are in Classic view.
- Two oscillator cards with a 300 x 142 wavetable display each. It draws the actual selected table (factory, imported, Claude), labelled "wavetable preview". Drag or left/right arrows scan `osc.pos`.
- One 640 x 146 filter graph with a Filter 1 / Filter 2 selector (selector only changes which filter is edited; the other is drawn dim). X = cutoff (20 Hz–20 kHz, log), Y = resonance, one gesture per drag, arrow keys as keyboard alternative, bypass shown, labelled illustrative. Type, power, cutoff, resonance, drive, mix, key track, env, velocity, vowel, routing and balance are next to it.
- The compact filter view now shares the same response function (`filterResponse`) so both draw identical curves.

### PRISM — Motion Amount and three lanes
Done as ordinary engine parameters, **not** a separate payload as GPT proposed. Reason: patches, presets, host automation and recall already handle every engine parameter; a separate payload would have needed new save/load and validation paths.
- Appended after all existing parameters (no index moved): `motion.amount` (default **0**), and per lane `motion.laneN.dest / division / length / interp / depth / step1..step16` for N = 1..3. 64 parameters; host catalogue 810 baseline + 82 added, 0 missing, 0 changed.
- Destinations: Off, OSC 1/2 position, Filter 1/2 cutoff, Filter 1/2 resonance. Defaults: lane 1 → OSC 1 position, lane 2 → OSC 2 position, lane 3 → Filter 1 cutoff.
- Each lane loops over 2–16 steps on its own division (1/32 … 2 bars), Step or Glide, scaled by depth and Motion Amount, applied after base values and the mod matrix in the destination's own scaling and clamping.
- Lane position is a pure function of the beat clock (host beat while the host plays and Host sync is on, internal clock otherwise), so tempo changes and transport jumps need no stored state.
- Amount 0 skips the path entirely.
- The UI highlights the playing step per lane from an atomic step index written by the engine (no allocation on the audio thread).

## Test results (Windows, this build)
- `DualTests` PASS (all earlier tests plus the new ones below); `DualProbe` real VST3 PASS; standalone alive after 6 s.
- Old PRISM presets: 8 presets (spread over the factory, Claude and Signal Studies banks) rendered **before** Motion existed (`Tests/golden-prism-presets.bin`, written from the unchanged engine). With Motion Amount 0% the render now differs by **0**; with all three lanes rewritten but amount still 0% also **0**.
- Amount 100%: output changes (largest sample difference 0.218), all finite.
- Lane follows the beat: beat 0 vs beat 16 (whole loops later) difference 0; beat 0 vs beat 1 (one step) differs.
- Arp board opens/closes, arrow-key edits change only their own parameters, window size is stored and restored, new instance opens at 100%.
- Heap allocations: 0 in 200 steady-state `processBlock` calls and 0 in 200 `routeMidi` calls. Legacy master-effects golden: difference 0.

## Not tested / open
- **FL Studio**: editor scaling at the user's display scaling, three-dot opener, step editing by mouse, both filter banks by ear, factory/imported wavetables, host automation of the new parameters, project save/reopen, Motion across tempo/transport changes. None of this was run in a DAW.
- Mouse editing (drag on the step lanes, filter graph, wave scan) was not exercised by an automated test; only keyboard edits, parameter writes and screenshots were. The graphs were checked as rendered snapshots only.
- macOS: not built.
- Motion was only judged by numbers (render differences), not by ear.
- `docs/ENGINE-INTEGRITY.json` and the "DSP unchanged" sentences in `README.md` / `docs/VALIDATION.md` describe the pre-Motion engines; PRISM's `sb-engine.*` and `sb-schema.*` now differ by the Motion code above. The JSON was not regenerated.
- Engine font sizes were not enlarged; legibility depends on the chosen window size.

Screenshots: `reviews/phase5-screenshots/`.
