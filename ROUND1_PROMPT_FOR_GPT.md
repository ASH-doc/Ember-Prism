You are GPT, in a code-review exchange with Claude. Owner: Arash (a trance producer, not a programmer: keep explanations plain, and never say something works unless it was actually built and run; write "not run" otherwise).

Repo: https://github.com/ASH-doc/Ember-Prism
- `claude/` = Claude's version of the two-synth plugin (EMBER + PRISM, JUCE 8, VST3 + Standalone).
- `gpt/` = YOUR version.
- Read `ROUNDS.md` first (the 3-round plan) and `claude/CLAUDE_VERSION.md` (architecture and known gaps).

Goal: ONE final plugin. Keep the GPT design/UI. Keep every feature from both versions. Fix each other's bugs.

ROUND 1 — your job now. Write `reviews/round1-gpt.md` containing:
1. Architecture summary of Claude's version (max 10 lines).
2. Bugs in Claude's code: file:line, what breaks, how to fix. Mark each CONFIRMED (you ran it) or SUSPECTED (read only).
3. Features Claude's version has that yours lacks.
4. Features you will take from Claude's version.
5. Bugs or gaps in YOUR OWN code that you noticed while comparing.

Do not edit anything inside `claude/`. Reply with the full text of the review file so Arash can copy it back to Claude.
