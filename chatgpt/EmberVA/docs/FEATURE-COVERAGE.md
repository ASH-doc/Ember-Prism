# Roadmap-to-native coverage

| Roadmap area | Native delivery |
|---|---|
| Polyphonic engine | 16 voices, three oscillators + sub + noise, stereo routing, soft voice-steal tail |
| Sources | PolyBLEP saw/pulse, harmonic triangle, sine, hypersaw, tune/phase, PWM, sync/ring/light FM |
| Unison and character | Up to eight per oscillator, seven-saw mode, nonlinear detune law, stereo/blend, drift/tolerance, Age |
| Requested macro additions | Dedicated Warmth and Saturation, independently automated and saved |
| Filters | Four models per filter, two filter stages, serial/parallel/split, key/env/velocity/drive/FM |
| Modulation | Three ADSRs with curves, three six-shape LFOs, sync/fade/polarity, 12 routes, four renamed/assigned macros |
| Trance tools | Six arp modes, octave/rate/swing/hold/length/velocity pattern, drawable level/tie gate, glide/modes/chord capture |
| Effects | Eight separate effect blocks with bypass/mix and detailed controls |
| UI | One JUCE face at 1280 × 720, proportional resizing, dark metal/walnut/amber, faders/knobs, scope, filter sketches, keys/wheels |
| Complete parameter reachability | Main performance face plus `…` / All Controls inspector; 264 registered parameters |
| Presets | Requested category counts total 114 plus Init; factory browser, category/search/arrows, JSON load/save |
| Host integration | Native AudioProcessor stereo instrument, VST3 automation, sample-offset MIDI, tempo/PPQ, host-state recall |
| Standalone | Same editor/processor via JUCE audio/MIDI device wrapper, built arm64 app bundle |
| Windows | Complete CMake project, pinned bundled JUCE, VS2022 build script, install helper, CI configuration; not built on this Mac |
| Audits | Engine regression, factory rendering, state recall, malformed patch rejection, VST3 binary-load probe, screenshots at three sizes |

Browser-specific files and the JavaScript global host contract were replaced by the C++ API described in `HANDOFF.md`. Optional brainstorm exclusions and sound-quality limits are explicit in `KNOWN-ISSUES.md`. “Implemented” does not mean ear-approved or certified across every host.
