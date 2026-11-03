You are GPT, in a code-review exchange with Claude. Owner: Arash (a trance producer, not a programmer: keep explanations plain, and never say something works unless it was actually built and run; write "not run" otherwise).

Repo: https://github.com/ASH-doc/Ember-Prism
- Branch `main`, folder `claude/` = Claude's version (EMBER + PRISM, JUCE 8.0.17, VST3 + Standalone). Baseline commit `07e1d7d`.
- Branch `chatgpt/baseline`, folder `chatgpt/DualSynth` = YOUR version. Baseline commit `f143191`.
- On branch `main` read `ROUNDS.md` (the 3-round plan), `claude/CLAUDE_VERSION.md` (architecture and known gaps), and `reviews/round1-claude.md` (Claude's review of YOUR code, already written).

Goal: ONE final plugin. Keep the GPT design/UI. Keep every feature from both versions. Fix each other's bugs.

ROUND 1 — your job now. Write `reviews/round1-gpt.md` containing:
1. Architecture summary of Claude's version (max 10 lines).
2. Bugs in Claude's code: file:line, what breaks, how to fix. Mark each CONFIRMED (you built/ran it) or SUSPECTED (read only).
3. Features Claude's version has that yours lacks (verify against Claude's source, do not guess).
4. Features you will take from Claude's version.
5. Bugs or gaps in YOUR OWN code that you noticed while comparing.
6. Your answer to each item in `reviews/round1-claude.md`: accept / reject / already fixed, with a reason. In particular check Claude's three CONFIRMED findings (missing JUCE extras/Build files in the git tree, DualTests stack overflow on Windows, clean Windows build) and say whether you agree.

Do not edit anything inside `claude/`. Reply with the full text of the review file so Arash can copy it back to Claude.
