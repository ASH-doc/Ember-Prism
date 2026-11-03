# Evaluation notes — the objective score, reproducible

Contract v1 uses the measurement already used for the Claude-to-AEther import. Source: `Tools/ClaudeImport/MatchFeatures.h` (plain C++, same file in both old projects). Command-line check of two renders: `DualCompareWav reference.wav candidate.wav [holdSeconds]`.

## Rendering rules
- Same MIDI for reference and candidate: `Tests/fixtures/import/reference-note-60-2s-138bpm.mid` (MIDI note 60, velocity 100, held 2.0 s at 138 BPM, then released; FL Studio shows note 60 as C5).
- Same tempo (138 BPM) and sample rate (48 kHz), 24-bit WAV, export at least 4 seconds so the release tail is captured. No normalising, no limiter, no master effects of the DAW.
- Dry and wet are rendered and compared separately: dry = the synth's own effects off, wet = as the preset intends.
- Level matched listening is separate from the score: the score includes a level term, listening is done after matching loudness.

## What is measured (mono mix of left and right, held for `holdSeconds`)
- **Spectrum**: Hann-windowed FFT of 16,384 samples starting 0.2 s into the note; energy summed into 31 third-octave bands from 40 Hz to 16 kHz; each band in dB relative to the loudest band (floor 90 dB below).
- **Level**: RMS over the held part, in dB.
- **Attack**: with 5 ms windows over the held part, the first window whose RMS reaches 90% of the loudest window; reported in ms.
- **Release**: RMS 0.30–0.35 s after note-off minus RMS in the 50 ms before note-off, in dB.

## Distance between reference a and candidate b
```
spectrumDb = sum_i w_i * |a.band_i - b.band_i| / sum_i w_i     with w_i = 10^( max(a.band_i, b.band_i) / 20 )
levelDb    = | a.rmsDb - b.rmsDb |
attackMs   = | a.attackMs - b.attackMs |
releaseDb  = | max(-60, a.releaseDb) - max(-60, b.releaseDb) |
score      = spectrumDb + 0.5 * levelDb + 0.02 * min(attackMs, 200) + 0.2 * min(releaseDb, 20)
```
Bands used in the contract: **score <= 4 close, <= 9 approximate, > 9 different.** (Those are the bands used for the Claude import: 9 close, 37 approximate, 19 different for the 65 EMBER presets.)

## Listening rubric (agreed before grading)
- **close**: sounds like the same patch with small differences.
- **approximate**: same role and character, audible differences.
- **different**: would not be mistaken for it.
Arash grades blind (A/B in random order, level matched). The objective score and the listening grade are reported side by side, never merged.

## What is reported for each checkpoint, separately
1. Decoded fields: how many fields of each evaluation patch have a recorded meaning, how many are unknown.
2. Native import status: Opened / Approximation / Blocked counts, with blockers named.
3. Sound fidelity: objective score band and Arash's grade for the patches that are Opened or Approximation.

## Sets (contract v1)
- Tuning set, at least 20 patches, used for mapping; evaluation set, 20 different patches, never used for mapping. Drawn by seeded stratified sampling (5 basses, 5 leads, 4 plucks, 2 pads or other, 4 hard cases). The lists (names and file hashes) stay on Arash's machine; only counts and aggregate scores are committed.
