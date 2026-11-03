# Phase 3 start — Prism Pitch Bend

The Windows-verified Graphite Control Bay commit 50c3f9e was fast-forwarded onto integration/dual-synth. Mac arm64 Release HostIntegration and ActualVST3 passed after the merge.

Prism now has Pitch Bend at source index 24, after the original 24 choices (including None). The existing Matrix and the new BEND drag chip use the same route. Modulation uses the smoothed bend for the voice's MIDI channel and honors route depth, curve and polarity. Bipolar maps the signed wheel range. Unipolar maps the same range to 0 through 1. The existing pitch bend range still governs oscillator tuning.

No parameter IDs were added or renamed. Existing stored source indices still select the same sources. The 24 existing Prism source selectors now have 25 choices, so automation stored by a DAW as normalized values can resolve to different choices. Legacy automated source-selector lanes need DAW verification. Saved performances and patches store the source index.

Mac arm64 Release build, HostIntegration and ActualVST3 passed again after the feature. The tests check the exact expected catalog extension, both bend directions, channel isolation, unipolar mapping, performance recall and existing bypass/golden renders. Windows verification and preset/table mapping remain open.
