# Integration report — AETHER DUO

## 1. What was built, and one decision you should know about

The roadmaps describe two Web Audio synths plus a web host. Your last message asked me to design both from those descriptions, integrate them, and **translate the result into a real C++ Windows app + VST3**. The synths did not exist yet (the folder only held the roadmaps, the unrelated AETHER plugin and a single web page), so I built the instruments **directly in C++/JUCE** and skipped a throw-away web version. The roadmap’s “host contract” (`noteOn`, `noteOff`, `pitchBend`, `controlChange`, `aftertouch`, `allNotesOff`, `panic`, tempo, patch/params) became a small C++ interface, `Source/Host/SynthContract.h`; the host only talks to the synths through it.

**Sound goals (as requested):** EMBER = very harmonic, analog, Moog-style warmth; PRISM = crystal-clear, modern, digital.
* EMBER: zero-delay-feedback **Moog ladder** with `tanh` saturation in every stage and partial resonance bass-loss compensation; per-voice analog drift + tolerance (AGE); even-harmonic WARMTH stage; polyBLEP oscillators with a deliberately analog (not ultra-clean) top end.
* PRISM: band-limited wavetables (octave mip-maps, 8× oversampled top harmonic, cubic interpolation), clean TPT filters, clean effects.

## 2. Measured results (offline, `DuoSelfTest` / `DuoPluginTest` — all passing)

| Check | Result |
|---|---|
| Tuning, both synths | A4 = 439.5 Hz measured (FFT bin size 1.5 Hz) |
| EMBER Moog ladder slope | 32.8 dB/octave above cutoff (24 dB filter + the saw’s 6 dB/oct + knee) |
| EMBER Multimode | 20.1 dB/oct (12 dB filter + 6) |
| EMBER ladder self-oscillation | pure tone at 439.5 Hz for cutoff 440 Hz, key-tracked |
| EMBER WARMTH | 2nd harmonic of a sine rises from −43.5 dB to −24.7 dB (+18.8 dB) |
| EMBER alias floor (saw) | −34 dB at C7, −28 dB at C8 — **analog-gritty on purpose; PRISM is cleaner** |
| PRISM alias floor (wavetable saw) | **−65 dB at C7, −59 dB at C8** |
| All 30 wavetables × 3 positions | finite, audible, bounded across C1–C8 |
| All 10 warp modes at max with 16-voice unison; all 8 filter types at max resonance + drive; all 10 rack effects | finite, bounded |
| All presets (65 EMBER + 62 PRISM + Init each) | finite, audible, peak ≤ 0.70 at the synth output |
| Host: layer / mute / solo | audible, silent when muted, solo isolates |
| Host: three routing modes | split (incl. swap), channel (incl. omni) route notes exactly |
| Stuck notes | muting releases, routing-mode change releases everything, panic is instant; no voices left afterwards |
| Limiter | +21 dB overdrive stays ≤ −0.3 dBFS (peak 0.87 / 0.966 max) |
| Sessions | save → change → load restores every parameter and the drawn LFO shapes; JSON is identical after the round trip; a wrong file is rejected and changes nothing |
| DAW state | `getStateInformation`/`setStateInformation` round trip exact |
| 50 quick panel switches | same component count, audio never stops, parameters unchanged |
| **Real VST3 file** loaded like a DAW | loads, stereo instrument, MIDI → audio (peak 0.40), saves 21 kB state, editor opens; 418 parameters; 95 samples latency (limiter look-ahead, reported to the DAW) |
| Standalone app | starts and keeps running (smoke test) |

**CPU** (one core, 48 kHz, Release build, offline render):
* 12-note chord on both synths at once (EMBER hypersaw ×7 ×2 osc + chorus/delay/reverb; PRISM 8-voice unison ×2 osc + warp + 3 FX): **25 %**.
* PRISM alone, 16 voices × 2 osc: 21 % (4 unison), 29 % (8), 36 % (12), **44 % (16 unison)**. Glitches in real time should only start well beyond that; I did not push to the glitch point.

## 3. Golden-rule separation audit (grep-verified)

* `Source/SynthA` mentions nothing of PRISM (0 hits) and `Source/SynthB` nothing of EMBER (0 hits); neither panel calls the other synth’s parameters, engine or presets (0 hits).
* The only shared include is `Host/SynthContract.h` (a pure interface). No shared DSP, helper, preset, effect or UI code. Same-named files (`Dsp.h`, `Fx.h`, …) are separate implementations in separate namespaces (`sa`, `sb`) and separate parameter prefixes (`sa_…`, `sb_…`).
* Host DSP (`HostDsp.h`, `HostEngine.*`) mentions neither synth by name, has no UI code, and its compressor/reverb/limiter are separate, original implementations (EMBER and PRISM each have their own reverb as well).
* The panels include `PluginProcessor.h` (needed for their parameters), which transitively compiles both engines’ headers; that is compile-time only, no call crosses over.

## 4. Where I cut scope (honest list)

* **Presets:** 65 EMBER + 62 PRISM (+ Init each), not 114 / 116. Hand-tuned by parameter reasoning; I could not listen, so they need your ears (see the checklist).
* **EMBER:** 8 matrix slots (not 12); no filter FM, no oversampling inside filters (heavy drive can alias a little); unison is shared by oscillators 1–2; no “Condition” switch (AGE does the job), MIDI learn, Mutate/Dice, A/B compare, pitch “Brake”; LFOs have no fade-in/key-sync/polarity; envelope curve is fixed (analog-style exponential).
* **PRISM:** no single-cycle WAV import and no table drawing/spectral editor (the 30 tables are generated by code); 10 matrix slots; 8 FX slots (not 10) reordered with **< >** buttons (not drag); the reverb’s “Air” control brightens the tail but there is **no pitch-shifted shimmer**; FX/warp amounts have no per-assignment curve/bipolar toggle; no hold stage on the envelopes; no light theme/snapshots.
* **Host:** the master drawer is a fixed 400 px panel; no per-strip insert FX (by design); the standalone’s audio/MIDI device selection is JUCE’s own **Options** dialog.
* **Web items from the first brief** (Web MIDI selector, localStorage autosave, `window.SynthA`, single-file HTML build, 16:9 mount/unmount): replaced by their native equivalents — JUCE MIDI/audio settings, the DAW project or the standalone’s saved settings, the C++ contract, and scaling the whole window with a fixed aspect ratio. Not applicable: AudioWorklets and `file://` limits.

## 5. Assumptions I made without asking

1. Natively built, no web prototype (see §1).
2. Names: AETHER DUO / EMBER / PRISM.
3. Master volume tops out at 0 dB (so the −0.3 dB limiter ceiling is never exceeded); synth outputs carry about −6 dB of headroom before the host.
4. CHANNEL mode defaults: A = ch 1, B = ch 2 (0 = omni).
5. Windows x64 only, Release build, static runtime (no redistributable needed).

## 6. Not run / not verified

* **Listening:** nobody listened to this build; all quality claims are measurements, not judgements.
* **Real audio hardware / MIDI keyboard / a DAW:** not run (the VST3 was loaded and rendered through JUCE’s plugin host in a test, not inside Cubase/Ableton/FL).
* Drag-and-drop with a mouse and every individual control: the UI was rendered to PNG (`screenshots/`) and its state/logic tested, but I did not click through it by hand.
* macOS/Linux builds, Standard VST3 validator (Steinberg `validator`), and long-duration stability.
