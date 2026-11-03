# ImportReport and DecodedPatch — schema draft 1 (for GPT's review)

Contract: `reviews/serum-spire-contract-v1.md`. Code: `Source/Import/ImportTypes.h` (plain C++17, no JUCE types), decoder `Source/Import/SpireDecoder.h`.

```
preset file --decoder--> DecodedPatch --translator--> native patch + ImportReport --preview UI--> apply (one engine, atomic)
```

## DecodedPatch (decoder output)

| Field | Meaning |
|---|---|
| `format` | `SpireSpf2`, `Serum1Fxp`, `Serum2Preset` (only Spire is implemented) |
| `formatVersion` | from the file when it has one, otherwise empty |
| `name` | supplied by the caller (file name without extension) when the file has none; Spire files have none |
| `author`, `vendor`, `bank`, `notes`, `tags` | file metadata, length-limited |
| `fields[]` | `{ name, group, value, outOfRange }` in file order. `value` is the stored normalised number, untouched. `group` is the name prefix (`osc1`, `flt1`, `env3`, `slot12`, `stp1`, `global`...). |
| `warnings[]` | non-fatal findings: ignored icon image, values outside 0..1 that were kept as stored |

`DecodeResult { ok, blocker, patch }`: when `ok` is false the patch must not be used and `blocker` is a sentence for the preview. A decoder never touches the live patch and never runs on the audio thread. Limits: 2 MB file, 2000 fields, 64-character ASCII names, 16 nesting levels.

Spire-specific: list choices are stored as `index / 999` (`spireEnumIndex`, `spireOnEnumLattice`). What each index means is *not* in the decoder; it comes from the calibration (`docs/import/spire-calibration-recipe.md`) and lives in the translator's mapping tables.

## ImportReport (translator output, what the preview shows)

| Field | Meaning |
|---|---|
| `result` | `Opened`, `Approximation` or `Blocked` (file level) |
| `format`, `target` | source format and target engine (`Ember` or `Prism`) |
| `headline` | one line for the top of the preview, e.g. "Opens cleanly: 31 settings mapped" or "Playable with 4 approximations" |
| `blocker` | set when `result == Blocked`: unrecognised version, malformed data, unresolved required asset, or no minimum playable voice. Apply stays disabled. |
| `rows[]` | `{ parameter, sourceValue, nativeValue, status, reason }` |
| `mapped() / approximated() / omitted()` | counts for the summary |

Row meanings (agreed in the contract):
- **Mapped**: a verified native representation exists for the source setting. It does not say the patch sounds right.
- **Approximated**: a named substitution (for example "filter model with a comparable slope"). `reason` says which.
- **Omitted**: an active feature could not be carried over. `reason` says what will be audibly different where known.
- A row exists only for settings that are *active* in the source patch (a muted oscillator or an effect that is off gets no row).

`Approximation` results require the explicit **Apply Approximation** action. `Opened` may use plain **Apply**. `Blocked` disables both.

## Apply transaction (GPT's design, constraints from the contract)

- Targets one engine. Routing, mixer, master FX and the other engine's patch are outside the transaction and must be byte-identical before and after.
- The native patch and every required asset are validated first, then published together at an audio block boundary. Any failure leaves the current patch and bindings untouched.
- The saved native patch carries everything it needs; it must reopen without the source plug-in or file.

## Synthetic fixtures for the UI work (no third-party content)

- `Tests/fixtures/import/synthetic-spire-minimal.spf2`: 137 invented fields in the real Spire shape (4 oscillators, 2 filters, 4 envelopes, 4 LFOs, arp, globals), values made up.
- `Tests/ImportTests.cpp`: 35 malformed or hostile cases plus 3,000 randomly damaged copies; each is blocked with a stated reason, no crashes.
- Parsing coverage on the local library (Arash's machine, 2026-11-03): **72,084 of 72,084** Spire `.spf2` files decode (user banks plus factory), 0 blocked. Two files carry a huge value in the field `soft`; it is kept and flagged `outOfRange`. This is *reading* coverage only; playable translation coverage is a separate number and does not exist yet.
- A first `ImportReport` example for the UI (hand-written, not produced by a translator yet):

```json
{
  "result": "Approximation",
  "format": "Spire .spf2",
  "target": "Ember",
  "headline": "Playable with 2 approximations, 1 omission",
  "blocker": "",
  "rows": [
    { "parameter": "EMBER filter 1 cutoff", "sourceValue": "0.62 (stored)", "nativeValue": "3.1 kHz", "status": "Mapped", "reason": "" },
    { "parameter": "EMBER filter 1 model", "sourceValue": "list item 5", "nativeValue": "Forge SVF", "status": "Approximated", "reason": "Closest native filter with a comparable slope" },
    { "parameter": "Oscillator 4", "sourceValue": "mix 0.45", "nativeValue": "", "status": "Omitted", "reason": "EMBER has three oscillators; the fourth layer is not carried over" }
  ]
}
```

Please review the field names and the Opened / Approximation / Blocked wording. Anything you change, change in `ImportTypes.h` so the UI and the translators stay on one definition.
