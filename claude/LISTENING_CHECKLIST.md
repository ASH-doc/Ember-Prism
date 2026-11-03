# Listening checklist (I could not listen — please check these by ear)

Open the standalone app or load the VST3 on a MIDI track. Start with the master drawer closed and your monitors at a normal level.

## EMBER (Synth A) — warm / Moog
1. **Moog Sub Bass** (Basses): play C1–C2. Should be round, heavy and stable. Raise **Reso** slowly: bass should thin a little (that is the real ladder behaviour), never crackle.
2. **Ladder resonance:** init patch, hold a note, sweep **Filter 1 Cutoff** with **Reso** at ~0.8. Smooth, no zipper noise. At Reso = 1 the filter should sing a clean sine on its own pitch.
3. **WARMTH 0 → 1** on any pad: it should get fuller and rounder (even harmonics), not louder or harsher.
4. **AGE 0 → 1** on **Hypersaw Anthem**: at 0 clean and steady, at 1 gently wobbling and gritty but still playable.
5. **Acid Squelch** (Basses, mono): fast filter snap, glide between notes, no clicks.
6. **Gated Supersaw / Pumping Chord Stab:** tight to the BPM; change BPM and it stays in time.
7. Switch **Filter 1 Model** through the four types: ladder = warm and heavy, multimode = smooth, screamer = gritty/aggressive, state-variable = punchy.

## PRISM (Synth B) — crystal clear
1. **Glass Pluck / Crystal Pluck:** bright and clean, no fizz or grit; a long delay/reverb tail should stay smooth.
2. **Position sweep:** init patch, hold a note, drag **Position** slowly through each table family (try Saw-Square, Glass Peaks, Vowel AEIOU). Should morph smoothly, no steps or zipper noise.
3. **High notes:** play C6–C8 on the plain saw table. Should be clean (measured alias floor is about 60 dB below the note).
4. **Supersaw Prism / Wide Wavetable Lead:** wide, bright and stable; press mono in your DAW — it should not collapse.
5. **Drag-and-drop:** drag the **LFO1** chip onto **Position** (OSC 1). The coloured bar appears and the sound should scan.
6. **Warp modes:** step through Sync, Bend, Mirror, FM… at moderate amounts; each should sound clearly different.
7. **Formant Lead / Vowel Morph:** tonal, vocal-like movement.

## Host
1. **Layer:** both synths sound together; the two **meters** move; **M**ute one — it goes silent and, when un-muted, nothing is hanging.
2. **Split:** low notes EMBER only, high notes PRISM only; **A<B** swaps. Change the split note while holding a key — no stuck notes.
3. **Channel:** a MIDI keyboard on channel 2 plays PRISM only.
4. **Master drawer:** glue compressor with a drum-heavy layer: gentle “glue” at 2:1, gain-reduction meter moves, no pumping on low notes. Reverb: smooth, no metallic ringing; try Low cut at 400 Hz.
5. **Limiter:** raise both strip volumes to +6 dB and hit a big chord — it must never get harsh or clip.
6. **PANIC** while holding a huge chord: instant silence.
7. **Save → change things → Load** a session: everything comes back, including PRISM’s drawn LFO shapes.

Tell me what feels wrong (“EMBER bass too thin”, “PRISM pluck too sharp”, …) and I will tune the specific parameter or DSP stage.
