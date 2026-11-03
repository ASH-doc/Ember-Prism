# Developer handoff

Prism Flux 1.0.0 is the independent native Synth B instrument. Build root is this directory. C++17, CMake3.22+, JUCE8.0.9. All third-party source is local. Build both `PrismFlux_Standalone` and `PrismFlux_VST3`; run CTest. `PrismVST3Check` deliberately links JUCE host code without linking the synth engine, so its test loads the actual plugin bundle.

## Native contract

`SynthB::Instrument::prepare(sampleRate)` allocates processing history before playback. `parameters(State)` sets the full schema state; `render(left,right,frames,events,count,hostBpm,hostBeat,customWave)` produces stereo and consumes sorted sample-offset MIDI events. `message(Event)`, `releaseAll()` and `reset()` handle notes, controllers and panic. `Schema::get()` owns parameter definitions and index layout. `PrismProcessor` supplies JUCE automation, patch serialization and the editor. Reported oversampling latency is12 host samples; reported tail estimate is30 seconds.

The `Instrument` render path uses fixed/preallocated buffers. UI/file/table generation is outside it. The JUCE wrapper's keyboard/MIDI buffering and deferred CC handling should be profiled separately before making a blanket realtime-allocation guarantee. Imported assets are owned for the instance lifetime and handed to the engine through an atomic current pointer. One shared imported table slot is available to both oscillators. Original factory table storage is shared immutably within a process; oscillator, filter, voice, motion and effect state is per instance.

The former browser host methods map to native methods: init→prepareToPlay; output→stereo buses; mount/unmount→createEditor/destruction; MIDI methods→processBlock MIDI; get/setParam→read/write; get/setPatch→patch/restore; getPresetList/loadPreset→factoryPatch/selectFactory. No DOM, AudioContext or Web Audio adapter is included. A future dual-engine host can route identical MIDI to separate processor instances and mix their stereo output.

## State

Canonical JSON uses format `dualsynth-patch`, version1, engine`B`, and all519 **actual-value** parameters under `params` (Hz, milliseconds/seconds where defined, dB, enum indices). This is not a map of uniformly normalized0…1 values. `docs/parameters.json` is authoritative for ranges and choices. Patch metadata includes name, category, tags, eight macro labels, MIDI mapping, descriptive mod/fx and optional raw float32-LE base64 waveform. Complete canonical `params` takes precedence over descriptive mod/fx. Sparse mod records accept source names, destination IDs, amount, bipolar and curve; sparse FX records use module names and numeric `mix/a/b/c/mode/sync/division` values.

Patch validation finishes before parameters are changed; rejection does not partially apply invalid values. Audio takes atomics at each block, so applying a multi-parameter patch is not a sample-atomic transaction with concurrently running host automation. Recall requests a panic/reset and stores an asset pointer with release/acquire semantics. Structural changes are quantized at control boundaries.

To regenerate bank metadata: build `PrismCoreTests`, run it with `--schema` into `docs/parameters.json`, then run `python3 scripts/generate-bank.py`, then rebuild the plugin so its embedded bank updates. Keep parameter IDs stable for host recall. Metadata and state tests use normalized tolerance to account for float host conversions.

## File ownership

- sb-schema: metadata, ranges and enums.
- sb-tables: original spectra, FFT, mip storage and warp functions.
- sb-filters: digital filters, circular delay, DC guard, downsampler.
- sb-motion: envelopes, LFO, arp and sequence.
- sb-rack: ten effects.
- sb-engine: synthesis, voice allocation, modulation and audio rendering.
- sb-plugin / sb-patches: JUCE wrapper, MIDI, JSON and factory operations.
- sb-ui / sb-ui-motion: native appearance and interactions.

Do not replace Synth B with the other synth's oscillator, filter, effect or UI code. The only copied third-party dependency was JUCE; authored implementation is independent. Optional future work and unverified deployment areas are recorded in KNOWN-LIMITATIONS.md.
