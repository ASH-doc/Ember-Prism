# Native handoff / resume pack

Ember VA is Synth A's standalone C++ engine and shared JUCE Standalone/VST3 editor. Original scope: warm subtractive analog character, not wavetable synthesis. The user's native C++ request superseded the PDF's browser-only implementation and stop-after-each-phase workflow.

Parameters: 264 dot-separated IDs; see `parameters.json`. Every range/default is defined in `sa-parameters.cpp`. The UI, APVTS, JSON patches and modulation matrix use that table. All synth files are local to this project. No shared host effects or cross-instrument dependencies exist.

Core API: `SynthA::Engine::prepare`, `setParameters`, `getParameters`, `midi`, `render`, `allNotesOff`, `panic`. `render` accepts sample-offset MIDI, optional host BPM and optional PPQ and produces one stereo output. Allocate/prepare outside the render callback. The core render path uses bounded arrays and preallocated delay lines. The fixed MIDI event capacity in the wrapper is 2048 events per block; overflow invokes panic rather than leaving stuck notes.

Patch API: `SynthAProcessor::getPatch/setPatch`, `getParam/setParam`, `presetCount/preset/loadPreset`. JSON format remains `dualsynth-patch`, version 1, engine A. Native values are physical units (Hz, seconds, semitones, dB), not normalized browser slider values. Enum values are numeric indices; case-insensitive enum names are accepted on import. All `params` values are canonical, including matrix and FX; descriptive `mod`/`fx` records are also exported. Sparse patch imports default missing values and can read separate mod/FX records. Invalid imports are rejected before mutation.

UI mount/unmount equivalent: create/destroy `SynthAEditor`; the processor and engine remain independent. The same editor class is used by both formats. State remains in the processor after closing the window. JUCE owns the standalone audio/MIDI device selection.

Next work should prioritize producer audition, high-register sync/FM spectral refinement, calibrated filter self-oscillation, and host/device stress testing. See `VALIDATION.md` for executed checks, `KNOWN-ISSUES.md` for limits, and `PRESET-LIST.md` for bank coverage.
