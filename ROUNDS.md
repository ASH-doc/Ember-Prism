# Ember-Prism — cross-review rounds (Claude <-> GPT)

Goal: ONE final plugin. Keep the GPT design/UI. Keep every feature from both versions. Fix each other's bugs.

Folders: `claude/` = Claude version, `gpt/` = GPT version. Each reviewer only edits the OTHER folder via a
written patch list in `reviews/` — nobody silently rewrites the other's code.

## Round 1 — Review
Each side reads the other's folder and writes `reviews/round1-<claude|gpt>.md`:
1. Architecture summary of the other version (what it is, in 10 lines).
2. Bugs found (file:line, what breaks, how to fix). Mark each CONFIRMED (ran it) or SUSPECTED (read only).
3. Features the other version has that mine lacks.
4. Features I will take from the other version.
Rule: never claim something works unless it was actually built/run. Write "not run" otherwise.

## Round 2 — Debug and merge plan
Each side answers the other's round-1 file in `reviews/round2-<claude|gpt>.md`:
1. Accept / reject each bug with a reason.
2. Proposed final architecture (module layout, parameter IDs, preset format, host/synth contract).
3. Feature table: feature | from which version | who implements it.
4. Conflicts that need Arash to decide.

## Round 3 — Handshake
Each side writes `reviews/round3-<claude|gpt>.md`, ending with a **FINAL CONTRACT** block:
- Final folder layout and build system
- Host/synth interface (one agreed header)
- Parameter ID list and naming
- UI: GPT look is the baseline; list of Claude features added and where they sit
- Test list that must pass before release (build, self test, plugin test, DAW load)
- Order of work and who owns which module
Both sides sign with: `AGREED — <Claude|GPT> — <date>`. Work on the final plugin starts only after both signatures exist.
