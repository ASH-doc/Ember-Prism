# Windows verification of the combined revision (Claude)

Tested revision: the tree of `b6b7c53` (a message-only amend of the tested `554e40e`) = GPT's `72fd432` (Claude PRISM compatibility bank) + GPT's `f3cbe88` (`design/curated-sound-banks`, Signal Studies and grouped browsers) + Claude's EMBER import (`c86ab18`) + the merge fixes below. Windows 11, MSVC 19.44, JUCE 8.0.9, Release x64.

## Results (all RAN)
| Check | Result |
|---|---|
| Release build: standalone, VST3, `DualTests`, `DualProbe`, `DualCatalog`, `DualClaudeImport` | builds, no errors |
| `DualTests` (HostIntegration) on the default stack | PASS |
| `DualProbe` (ActualVST3) on the built VST3 | PASS |
| Standalone launch | alive after 6 s (no audio-device playback test) |
| Parameter catalog | 810 baseline, 18 added, 0 missing, 0 changed (the 18 = 16 Phase 2 + GPT's two Claude table selectors) |
| Pre-Phase-2 golden master-effects render | largest difference exactly 0 |
| Signal Studies | 8 patches per synth, all 16 load and play; the automated phrase-correlation check finds no near-identical renders (max correlation 0.26) |
| Original preset indexes | EMBER factory patches 0-114 byte-identical (test); PRISM original bank preserved (GPT's tests) |
| Claude EMBER import | 65 patches load under their stated names and make finite, non-silent sound |
| Claude PRISM compatibility bank | 63 patches load and play (GPT's test) |
| Browser: All sounds groups by sound, search, Previous/Next | covered by GPT's tests (grouped menus keep every preset; tag search returns the original preset ID) plus new tests, below |
| Browser: Claude banks | EMBER "Claude Import" filter shows exactly 65; PRISM "Claude Compatibility" filter shows exactly 63; every item belongs to the bank; Next / Previous wrap inside the filtered list and visit every item once; search "bass" inside the bank returns only matching items (EMBER 11, PRISM 9) |

## Merge conflicts and what was done
1. `CMakeLists.txt`: both sides edited the binary-data lines. Kept the union (EmberBank with factory + Signal Studies, a separate EmberClaudeBank, PrismBank with factory + Signal Studies + Claude Prism).
2. `Engines/A/Source/sa-processor.cpp`: both sides appended presets. Order is now factory (115), Claude imports (65), Signal Studies (8, last). Signal Studies must stay last: GPT's browser test locates its patches by counting back from the end of the bank.
3. `Tests/HostTests.cpp`: both sides edited the test call list. Kept GPT's list and added Claude's tests.
4. Browser change by Claude: `Source/PresetBrowser.h` got a "Claude Import" bank filter for the imported EMBER presets (their category is `Claude <category>`, so under All sounds they already group by sound: Bass, Leads, Pads, ...). Prism keeps GPT's Claude Compatibility filter unchanged.

## Not verified
- The browser pop-up menus were not looked at on screen; the checks above exercise the menu model (items, ids, filters), not rendering or mouse use.
- Listening, DAW, macOS, clean-clone build: not run by Claude. Signal Studies and the Claude banks were not auditioned.
- The graphite sidebar and both synth panels were not changed by the merge; the earlier panel screenshots were not regenerated for this revision.
