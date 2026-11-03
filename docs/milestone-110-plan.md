# Milestone 110 plan - Russian language layer over the CD edition

**Tier A. Status: completed and accepted by the maintainer (2026-11-03)**
after the play-test of the checks that need no Russian (typing and a
Cyrillic save name, the Bank by mouse and keyboard, combat and Quick Fight
Next, the intro with speech under the strip, English unchanged); reading
the Russian text in play is left to a Russian speaker. Plan `087883d`,
Part A `7c55280`, Part B `efd7d3a`, Part C `d2d687e`.
[Later goal 7](roadmap.md#later-goals) of the roadmap, run in parallel with
M109. Save v26, Journey schema 9 and content 14.

## Goal and scope

A player who owns the World of Xeen CD and also installs the old-games.ru
fan translation (Clouds of Xeen RUS and Darkside of Xeen RUS 1.2) can choose
Russian. The text and pictures of the archives, the program text and the
font then come from the translation; maps, rules, statistics, timing, music
and recorded speech stay those of the CD. English stays the default and does
not change. Other languages, other translations, a Russian floppy edition,
translated project text and distributing the translation are out of scope.

## Key decisions

- **Approved deviation.** No Russian CD edition exists, so recorded speech
  stays English under Russian text
  ([quirks](original-quirks.md#dev-russian-layer)). Player-entered names are
  drawn literally in both languages; in English a byte `0x80-0xff` draws the
  glyph of its low 7 bits.
- **Host setting, identified translation.** `language` and
  `russian_translation` in `settings.ini` (the launcher's Language window,
  or `--russian <folder>`) select it. The three archives and the Darkside
  programs are recognized by size and checksum; any other file is refused,
  naming it, and English plays.
- **A language layer, proven by format.** One substitution point,
  `CheckedInstalledArchive::createReadStreamForMember`, serves every archive
  read, and the raw `SearchMan` path cannot bypass it. The repository keeps
  only the ids and formats of the substituted entries. Formats that also
  carry gameplay data take only their text through a structured merge, and
  a test proves per entry that the non-text content equals the CD's.
- **Russian program text and rules from the Russian programs.** By calibrated
  static analysis of the translation's `xeen.dat` and `xeen.exe`: every
  `XeenDosText` field and button, the cutscene captions and score text, item
  names, and the game text MMModern had written in code (now read from the
  program in both languages). Nothing is invented where the program shows
  no difference: the Russian program does not inflect, and has no Bank
  gold/gem swap or subtitle split.
- **Keyboard.** The Russian program's one lowercase ЙЦУКЕН table by
  physical key, Shift capitals, Ctrl symbols, no F11/F12 switch, strafe on
  х/ъ, every hotkey from its tables ([keyboard](original-quirks.md#ru-keyboard),
  [controls](controls.md)). The program's key defects are kept
  ([defects](original-quirks.md#ru-kept-key-defects)); clicking Quick Fight
  Next works, an approved deviation
  ([Next](original-quirks.md#dev-ru-quick-fight-next)).
- **Caption strip.** In every scene that uses it, intros, endings and four
  entrance scenes, the strip runs during speech and the scene waits for it,
  as in the Russian programs ([strip](original-quirks.md#ru-caption-strip));
  English timing is the CD's.
- **Saves v26.** The Russian program types Cyrillic save names, so the one
  current format admits name bytes `0x20-0x7e` and `0x80-0xff`; v25 is
  rejected with the older-format message, and the M44 digests changed only
  in the envelope version after the maintainer's byte audit. Games in either
  language share the save folder and load in either.

## Results

- With Russian, the game plays the translated texts, pictures and two-bank
  font, the Russian control panel (seven buttons, no Text or Speech toggle),
  credits page, item descriptions
  ([order](original-quirks.md#ru-item-order)), two-line monster names in
  combat ([rows](original-quirks.md#ru-combat-rows)), Cyrillic entry, mirror
  answers compared exactly, and the intros and endings with the Russian strip.
- Program text census: 275 fields mapped, 4 confirmed absent from the
  translation (CD-only text that stays English), none unlocated
  ([program text](original-quirks.md#ru-program-text)).
- Tests: the layer's per-entry proofs, structured merges and refusals; the
  Russian program text census and signatures; Russian rules, keys and
  captions; save round trips with Russian and English names; Russian frames
  of the intros, endings and score pages rendered and inspected. English
  frames and texts unchanged. Complete suite 416/416 with the Russian
  material and 413/413 without.
- Independent reviews of the plan, its Part C amendment and every part's
  implementation; every finding fixed and its recheck approved.
- **Play-test** (2026-11-03): typing and a Cyrillic save name reloaded from
  the title, the Bank by mouse and keyboard, combat targeting and Quick
  Fight Next, the Darkside intro with speech under the strip, and English
  unchanged with v25 rejected, all passed.
- **Open:** a Russian reader has not yet checked the text in play.
- **Release notes:** going back to an older build rejects the six-key
  `settings.ini` and so loses the won-ending flags, because release and
  development builds share the save folder (maintainer, 2026-11-03).
