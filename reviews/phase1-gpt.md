# Phase 1 follow-up — GPT

Base: 1fb8de0 on integration/dual-synth.

- Ember and Prism scope CC120/123 to the MIDI channel, including held keys, sustain and generated notes. Global Panic still clears both engines. Mono note-off checks the voice channel and can resume a held note on another channel.
- Performance recall validates both patches and prepares Prism's embedded asset on the calling thread. Publication waits for any active host block to finish. An audio callback arriving while publication is pending clears its output and MIDI without waiting. The next processed block sees both engines and the host settings together. A recall may briefly interrupt audio. This applies to whole-performance and DAW state recall through DualProcessor.
- Added engine-level channel isolation and a deterministic test that calls processBlock during publication. No UI code, presets, or parameter IDs changed.

Mac arm64 Release: standalone and VST3 built. HostIntegration and ActualVST3 passed, including sample-exact bypass, MIDI slicing and state checks. Windows, DAW use, and standalone device playback are pending for this follow-up.
