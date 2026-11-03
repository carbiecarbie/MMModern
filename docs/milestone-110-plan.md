# Milestone 110 plan - Russian language layer over the CD edition

**Tier A. Status: approved by the maintainer (2026-11-03).**
[Later goal 7](roadmap.md#later-goals) of the roadmap, started by the
maintainer on 2026-11-03 in parallel with M109. Baseline: `0494175`, equal to
`origin/main`; branch `m110-russian` in its own worktree and build directory.
Tier A because it changes how every archive resource and every program text
is read (ownership/architecture), and possibly the save format (decision 11).
Revised after the independent plan review of 2026-11-03; the maintainer
approved the revised plan, decision 10 included, the same day. Amended
after Part C's evidence (decisions 3 and 11, Part C's settled rules); the
maintainer approved the amendment on 2026-11-03.

## Goal and scope

A player who owns the World of Xeen CD and also installs the Russian fan
translation of old-games.ru (Clouds of Xeen RUS and Darkside of Xeen RUS 1.2)
can choose Russian. The text of the archives and of the programs, and the
font, then come from the translation; maps, rules, timing, music and recorded
speech stay those of the CD. English stays the default and does not change.

Out of scope:
- other languages and other Russian translations; unknown versions of the
  translation are refused with a clear message;
- a Russian floppy edition, with its own program and behavior;
- translating project text (launcher, diagnostics, "not supported yet"
  notices, CLI output), which stays English;
- distributing the translation: MMModern never includes, copies or modifies
  its files.

## Approved deviation

No Russian CD edition exists. The layer combines the CD edition with a
translation made for the floppy editions, so recorded speech (Crodo's
narration, the intros and endings) stays English while the screen is Russian.
Approved by the maintainer on 2026-11-03; recorded in
[original quirks](original-quirks.md#approved-deviations) and
[ScummVM differences](scummvm-differences.md) when Part A lands. The pinned
ScummVM supports only the Russian Clouds of Xeen, as its own floppy game.

## Current state and evidence (2026-11-02/03)

Line references are to the baseline, not to the uncommitted spike. Local
copies and scripts are outside the repository in
`D:/Projetos/MModern/ru-translation` (`scripts/cc_diff.py`,
`scripts/cc_overlay.py`, `scripts/cd_extract.py`, `scripts/program_probe.py`).
- **Translation.** The Russian Clouds `xeen.cc` is the one the pinned
  ScummVM detects as the "Russian fan-translation"
  (`engines/mm/detection_tables.h:241`). The Darkside translation is not in
  the reference; its program opens `xeen.cc`, `dark.cc` and `intro.cc`, so
  the two form a Russian World of Xeen.
- **Archives.** The English CD and floppy archives are nearly identical
  (XEEN 25, DARK 55, INTRO 119 differing entries, the last the talkie
  extras). The translation changes 105, 379 and 7 floppy entries; 21 and 39
  of those were also changed by the CD. These are byte differences only:
  they do not show which fields are text. Both archives' `2a0c` New Game
  blocks are among the conflicts, and the bridge builds indexed initial-state
  containers from them (`ScummVmXeenBridge.cpp:302-336`).
- **Font.** The Russian `fnt` is 8704 bytes: the English 128 glyphs plus a
  second bank of 128 (66 used) for bytes `0x80-0xff`, in the layout of the
  pinned `engines/mm/xeen/font.cpp:48-57`. `XeenFontFormat.cpp:27,46` masks
  every byte to 7 bits, and the text renderer masks before testing controls
  (`XeenTextRenderer.cpp` `parseText`, `renderOriginal`, `render`,
  `textWidth`; `XeenDosText::validateControls`): `0xa0` (`а`) is a space,
  `0x8a` a newline, `0x83` an alignment and `0x8c` a color command today.
- **Programs.** Both Russian programs are unpacked, with cp866 text.
  `scripts/program_probe.py` measures, against the CD `WORLD/XEEN.DAT`, 21%
  of sampled 24-byte code chunks found unchanged and 60% of format-string
  skeletons aligned in order. This is reconnaissance; it establishes neither
  field identity nor coverage. `XeenDosText` holds 275 fields and 4+2 button
  layouts at fixed CD offsets, checked through relocations, segment bases,
  pointer tables, exact extents and argument signatures
  (`XeenDosText.cpp:358-443`); consumers sit in 15 files (221 `scalar`, 47
  `table`, 6 `buttons` lines). `XeenCutsceneData` reads captions and score
  text, and also mouth timing, animation coordinates, sound cues and actor
  data, from `WORLD/XEEN.EXE` at fixed offsets behind a CRC check.
- **Items.** Item names come from the reference's English `CONSTANTS_7`
  blob at build time (`cmake/MMModernItemCatalog.cmake`), not from the
  program; material names come from `mae.xen` in `DARK.CC`.
- **Code.** Condition names (`XeenCharacter.cpp:55-72`, shown in combat by
  `XeenEncounterFlow.cpp:594`) and the combat roster words (`:586-597`) are
  game text written in code.
- **Arguments.** English `X_FOUND_Y` takes two arguments
  (`XeenDialogView.cpp:181`); the reference's Russian string takes three,
  with the gendered verb (`ru_constants.h:1641`). Presentation records carry
  names but not the recipient's sex.
- **Input and saves.** `XeenTextInput::type` accepts only `0x20-0x7f` with a
  font advance; save names must be `0x20-0x7e`
  (`XeenSaveFormat.cpp:619-623`) and enter the formatted slot list directly
  (`XeenTitleFlow.cpp:210`). Saves are v25, Journey schema 9, content 14,
  with no language; their fingerprint hashes the raw CD `xeen.cc` and
  `dark.cc` (`XeenSaveFile.cpp:218-227`), independently of the bridge.
- **Other readers.** `ScummVmRuntime.cpp:30-57` registers the raw archive
  files with `SearchMan`; no production caller is known to open them through
  it. Metadata limits check the CD index size (`ScummVmXeenBridge.cpp:240`),
  and the sprite cache key has no language (`:275-279`).
- **Settings.** `settings.ini` is parsed strictly (`XeenWonStore.cpp:30-43`):
  a new key needs the parser, the writer and its completeness rule.
- **Reference.** The pinned reference has 29 Russian-only sites, listed
  under decision 7.
- **Part 0a spike (2026-11-03).** With the translated entries substituted
  in `CheckedInstalledArchive::createReadStreamForMember`, a font that
  selects the two-bank layout by its size, and the renderer's masking made
  to keep bytes `0x80-0xff` with that font, the CD install draws the Russian
  `aaze0028.txt` Vertigo texts and the Russian `back.raw` logo, and the
  English pass with the English font is unchanged. This shows only that
  substitution at that point reaches these reads and that the two-bank font
  draws; it does not settle conflicts, other formats, the `SearchMan` path or
  a game session.
- **Part 0b calibration (2026-11-03).** Scripts `field_map.py`,
  `field_align.py` and `exe_probe.py`; results in `ru-translation/out`. The
  Russian `xeen.dat` data segment is `0x46e2` (file `0x4c290`), its
  `xeen.exe` one `0x19af` (file `0x1c6d0`), matched by rank of relocated
  segment words. The gate passed: no required mapping proved unresolvable
  and no evidence contradicted the plan. Findings:
  - **Callers locate most fields.** Matching each CD reference by its
    surrounding code, with the Russian string required to start a string
    and to keep the English argument signature and leading controls, maps
    157 of the 275 `XeenDosText` fields; a read sample of 31 is right, and
    one false hit (an immediate into the middle of a string) was removed by
    the string-start rule. The other 118 are mostly pointer tables and
    strings reached through data pointers or changed code: per-field work in
    Part B. Aligning the data segments by order is not reliable (it
    contradicts the callers on 10 fields: the translators moved strings).
  - **Tables and buttons.** Text tables map through their callers (for
    example `STAT_NAMES`: Сила, Интеллект, Сила духа...). The title and
    chooser button tables keep the CD positions at the same offsets with
    Cyrillic hotkey bytes (title `н з с д` for S, L, C, O); difficulty moved
    with its callers; the control panel has 7 buttons instead of the CD's 9,
    without the CD-only text and speech toggles; the Other Options keys are
    `с о а` for D, C, V.
  - **`XEEN.EXE` text.** The Russian program holds the cutscene labels
    ("Король Барлок", "Крестьяне"), the score pages and formats ("Нажмите
    клавишу!") and the World ending captions. It has no `cd1...eg2` talkie
    ending resources, so the CD ending sequence keeps its own structure.
  - **Grammar.** The sampled Russian strings avoid inflection rather than
    computing it: "взломал(а) замок", "не смог(ла)", abbreviated "Зол"/"Смц"
    in the found-treasure message, "за %lu золота". The reference's gendered
    and plural tables are for its own Russian Clouds, not this program.
  - **Names live in the archives.** Map names (`xeen0028.txt` Вертиго,
    `dark0029.txt` Каслвью), `xeen.mon`, `dark.mon`, `spells.xen`,
    `mae.xen` and both mirror lists are translated in the Russian `dark.cc`,
    so World of Xeen needs no Clouds name tables from a program. The
    monster files also carry statistics (decision 3's structured merge), and
    mirror answers are Russian words.

## Design decisions

1. **Language is a host setting, not an edition.** The CD stays the only
   edition. A `settings.ini` key selects Russian and names the translation's
   folder; the launcher offers both, and a developer option does the same.
   Without a valid translation Russian cannot be selected and English plays.
   The language is fixed for the lifetime of a bridge: changing it builds new
   archives and caches (in practice, a new session).
2. **Identified translation.** The three archives and the Darkside programs
   are recognized by size and checksum. Any other file is refused with a
   message that names it.
3. **A language layer, proven by format.** The repository keeps, per
   archive, the ids of the substituted entries (metadata only), each with its
   resource format. Every format is classified before it is substituted:
   text and translated pictures are taken from the translation; any format
   that also carries gameplay data (New Game containers, Event scripts, maps,
   monster records) keeps the CD's gameplay fields and record identities and
   takes only its text, through a structured merge; anything unclassified
   stays CD. Cutscene timing, coordinates and sound tables always stay CD,
   with one Russian-only exception from the Russian programs: in every
   scene that uses the caption strip the strip runs during speech and the
   scene waits for it, as Part C lists (maintainer, 2026-11-03: intros,
   endings and the `xeen.dat` entrance scenes, Sphinx, Golem/Dungeon,
   Reaper/Tower and Dwarf, which carry the same patch). English cutscene timing stays exactly
   the CD's.
   A test proves, per substituted entry, that the non-text content equals
   the CD's, which is what makes this a language layer.
4. **One substitution point.** Installed-archive reads go through
   `CheckedInstalledArchive::createReadStreamForMember`; New Game blocks and
   metadata reads use the substituted bytes, and limits check the
   substituted size. The raw `SearchMan` registration in `ScummVmRuntime`
   either reads through the same layer or refuses to open an archive member
   while Russian is active, so no path can bypass it silently.
5. **Two-bank font and 8-bit text.** With Russian active, bytes `0x80-0xff`
   are characters drawn from the second bank, and only `0x20` is a space, as
   the pinned reference does; every masking site above follows the language.
   English drawing and parsing do not change.
6. **Russian program text uses the same names.** `XeenDosText` and the text
   fields of `XeenCutsceneData` gain a Russian layout: the same field and
   button names, with offsets into the Russian Darkside programs, found by
   calibrated static analysis. Russian item names come from the Russian
   program by the same means, so the build-time English catalog stays English
   only. Each English field ends in one of three states: mapped, confirmed
   absent from the translation (CD-only text, which stays English), or not
   located. Only "confirmed absent" may fall back to English; "not located"
   fails the completeness test, which keeps later additions cheap and honest.
7. **A formatting contract for Russian text.** When a Russian field's
   argument signature differs from the English one (for example the
   gendered `X_FOUND_Y`), Part B adds the presentation arguments its callers
   need, such as the recipient's sex, with that field. The signature of every
   Russian field is part of its layout and checked as the English ones are.
8. **Russian rules come from the Russian program.** Russian-only behavior is
   reproduced from the Russian program and, for Clouds, checked against the
   reference's Russian sites. Nothing is invented where the program shows no
   difference. The pinned reference shows candidates to look for:
   - three-form plurals of days and food, two forms of gold, and gendered
     verbs ("born", "found", "picked/unable to pick the lock");
   - item descriptions with the condition words first and the name before
     the material (`engines/mm/xeen/item.cpp:358`);
   - Cyrillic entry in text prompts (F11 on, F12 off, JCUKEN positions to
     cp866 bytes) and command hotkeys on the Cyrillic initials;
   - the Bank's gold and gem buttons swapped to match the Russian sprites;
   - for Clouds, Russian map, monster, spell and material names and mirror
     answers, which the reference keeps as tables because the Clouds program
     holds them; in World of Xeen they come from the translated archives
     (Part 0b), so no such tables are needed;
   - the Clouds intro subtitles split at each period.
   F11/F12, the Bank swap and the subtitle split stay candidates until the
   Russian program shows them. The reference's own oddities (its gold plural
   ignoring 11-14, Shift breaking Cyrillic entry) are not copied unless the
   Russian program shows them. Part 0b found the sampled Russian strings
   avoiding inflection ("взломал(а)", abbreviations), so most plural and
   gender candidates are expected not to apply; each is settled by the
   program, not assumed.
   Input follows the Russian program's bytes: its command hotkeys and button
   keys are cp866 Cyrillic letters, and the mirror answers are Russian words,
   so Russian play needs a keyboard mapping from physical keys to cp866 for
   hotkeys and prompts alike. Part C settles from evidence how the DOS
   Russian programs received Cyrillic input (the Clouds translation ships a
   keyboard driver, `KSR.COM`; the Darkside one has none), and the layout
   and switch that follow from it. The Russian control panel follows the
   Russian program: it has seven buttons and no Text and Speech toggles,
   whose row holds its Mr Wizard button, so those toggles are absent in
   Russian and speech cannot be switched off from the panel there (amended
   by the maintainer, 2026-11-03, after Part B found the Russian panel).
9. **Game text in code moves to the programs.** Condition names, the combat
   roster words and any other player-visible game text MMModern still writes
   in code are read from the program, in both languages.
10. **Names are literal text in both languages.** A player-entered name
    (save slot, character) keeps its stored bytes and is never read as
    formatting: the renderer receives it as literal characters, so `0x8a`
    in a name cannot start a new line in English. In English, a byte
    `0x80-0xff` in a name draws the English glyph of its low 7 bits; in
    Russian, the Cyrillic glyph. This cross-language display has no
    original counterpart and is part of the approved deviation, subject to
    the maintainer's confirmation (given 2026-11-03).
11. **Saves.** The Russian program accepts Cyrillic save names: its
    save-name prompt (`0xb7b1`) uses the shared text entry, whose character
    test the translators widened from `0x7f` to `0xff` (`0x1dc86`; CD
    `0x1e123`), and its keyboard types only Cyrillic letters, digits and
    symbols. So save v26 becomes the single current format for both
    languages (approved by the maintainer, 2026-11-03): a save name is 1-20 bytes, does not start with a space, and
    each byte is `0x20-0x7e` or `0x80-0xff` (cp866 in Russian). The layout,
    Journey schema 9 and content 14 do not change; v25 saves are rejected
    with the existing older-format message, as the 0.x policy says. The M44
    digests change once, after a byte audit the maintainer approves
    (envelope version only). The other consumers handle bytes
    `0x80-0xff`: the title slot list and the panel draw the name
    literally, Save As entry admits it, the `Resumed` console line and
    `mmodern_save_dump` keep the bytes. The DOS export writes its own ASCII
    `--name`, not the save's name, and stays as it is; the archive
    fingerprint hashes only the archives. English entry keeps the CD's `0x20-0x7f`, so English play never
    creates such a name; a Russian-named save loads in English and draws as
    decision 10 says. English and Russian games share one save folder and
    load in either language. Gameplay identities stay unchanged, as
    decision 3's test proves.

## Parts

- **Part 0a - rendering spike (done).** See the evidence above; its code is
  throwaway.
- **Part 0b - calibration gate for program text (passed 2026-11-03).**
  Results under the evidence above. Before bulk mapping,
  map a representative sample by calibrated analysis, each traced to its
  callers with offsets in both programs: `XeenDosText` scalars, a pointer
  table and a button layout; `XeenCutsceneData` captions and score text; a
  grammatical variant (a plural or gendered string). Settle whether the
  Russian Darkside program holds the Clouds names (maps, monsters, spells,
  materials, mirror answers) or the Russian Clouds program must supply them.
  The gate stops the milestone on a required mapping it cannot resolve or on
  contradictory evidence; it reports, and the plan is amended.
- **Part A - language layer.** Setting, launcher choice, identification, the
  substitution point and the `SearchMan` path, the classified id lists with
  every substituted format (the 60 conflicts and the New Game containers
  included), the structured merges, the two-bank font and 8-bit text, literal
  names, the quirks and differences entries.
- **Part B - program text.** The Russian layout of every `XeenDosText` and
  `XeenCutsceneData` text field and button, with its argument signature and
  the presentation arguments it needs; Russian item names; the completeness
  test; game text moved out of code.
- **Part C - Russian rules and input.** Decision 8, the keyboard mapping
  for Cyrillic hotkeys and prompts, mirror answers, and decision 11's save
  names. Also, from the Part A review (maintainer, 2026-11-03):
  - the Russian monster names in `xeen.mon`/`dark.mon` contain `\n` (for
    example "Сточный\nохотник"): find in the Russian program how it lays these
    names out in the combat rows and in `MONSTER_DETAILS`, and reproduce that;
  - in the non-original `render()` path, `parseText` turns a literal `0xa0`
    into a space, which becomes a word-break point
    (`XeenTextRenderer.cpp:263`): a literal must never be a break point, as in
    `renderOriginal`.

  Settled from the Russian programs before implementation (2026-11-03;
  offsets in the Part C evidence notes, recorded in the quirks when
  implemented):
  - **Keyboard.** The Russian program reads keys with the CD's own INT 9
    handler and one scancode table whose letter rows are lowercase ЙЦУКЕН
    cp866 (`0x44176`). There is no second table and no switch, so Latin
    letters cannot be typed; F11 and F12 do nothing. Shift uppercases
    Cyrillic and leaves other keys unchanged; Ctrl gives the symbols the CD
    gives with Shift; Ctrl+Alt+Del no longer quits. MMModern maps physical
    key positions (scancodes, independent of the host layout) to these
    bytes in Russian, for hotkeys and text entry alike, and takes Russian
    text from key events, not from host text input. `KSR.COM` and the
    F11/F12 switch belong to the Russian Clouds program only (calibrated:
    its F11/F12 switch at `0x5f76c`), which World of Xeen does not use.
    Consequences that change behavior: the H key gives `0xe0` (`р`), which
    is also the CD's Ins code; Shift+digit stays a digit; Ctrl+Left/Right
    become plain turns, because the Ctrl codes are gone; in text entry
    Left (`0xf0`) deletes, Right (`0xf1`) types `ё` and F1-F10 type
    `0xc9-0xd2`, so a save name can hold such bytes, which v26 admits. The
    `XEEN.EXE` presentations keep the CD's keyboard table and Ctrl rule
    (`0x1928f`), so their keys, Ctrl-Q included, are unchanged in Russian.
  - **Hotkeys.** Every Russian key comes from the program's 26 button
    tables, its static main-screen, combat and bank tables, the keys it
    writes into the item tables at run time, and its switch and compare
    sites, by index against the CD's. Keys the translators left Latin (an
    `E` exit in the Inn and Item to Gold's `G`) stay mouse-only in Russian,
    as in the program (maintainer, 2026-11-03); where there is no button,
    the path is unreachable, and another key still serves it (the Inn's
    Escape, Space and `ы`). The reverse case follows the program too: in
    Create's attribute exchange the buttons keep Latin values while the
    dispatch expects Cyrillic, so the keys work and clicking a row does
    nothing. Strafing moves from Ctrl+Left/Right to the `х` and `ъ` keys.
  - **Control panel and keyboard-only keys.** The Russian panel dispatch
    (`0x1ea78`, 23 entries at `cs:19eb`) gives: `а` Save, `о` and `и` Save
    As, `з` Load, `в` Quit, `е` and `к` Efx, `м` Music, `п` Mr Wizard; `ж` is
    the clock gate's G stage, `б` its B stage and `0xb1` its R stage. The O
    stage has no entry and `0xb1` cannot be typed, so the hidden panel clock
    cannot be opened in Russian: `ж` alone does nothing visible and `б`
    after `ж` closes the panel, as the CD's B without O does. The CD's W
    (Wait) test stays `W` in the Russian program (`0xcff0`), so W-Wait
    cannot be typed; Space keeps its path. These are reported, not
    replaced by invented keys.
  - **Kept Russian-program defects** (maintainer, 2026-11-03): the hidden
    panel clock cannot be opened; W-Wait cannot be typed (Space still
    works); in Create's attribute exchange clicking a row does nothing and
    only the keys work; the Latin `E` (Inn) and `G` (Item to Gold) keys are
    mouse-only. Part C records each as a kept original error in
    `original-quirks.md`.
  - **Text entry and answers.** Entry accepts `0x20-0xff` in Russian; word
    capitalization never changes Cyrillic. Mirror and riddle answers compare
    Cyrillic exactly (the program's case folding covers `a-z` only), and the
    Russian mirror words are capitalized, so Shift matters.
  - **Items.** Weapons, armor and accessories read name, broken, cursed,
    then material and suffix; miscellaneous items keep the CD order. The
    words are "сломано" and "проклято" with a leading space, and "of" is a
    space. `mae.xen` becomes a translated entry (entry zero is a space):
    decision 3's per-entry non-text proof and the material parser test
    cover it in the Russian data test.
  - **Not in this program:** plurals and gender (Part B), the Bank's
    gold/gem swap (its keys and dispatch keep the CD order) and the
    subtitle split at periods. If the translated `bank2.icn` frames turn
    out to show gems before gold, the program's order still stays and the
    mismatch is recorded as a quirk.
  - **Caption strip, every scene** (maintainer, 2026-11-03: the rule covers
    every scene that uses the strip, intros and endings alike; decision 3).
    The strip still scrolls 40 characters with the CD cadence, but the
    Russian programs run it whenever its position is at least 1, with
    speech and Efx too (`XEEN.EXE` helper `0x112ce`, CD gate "digital device
    below 2"; `xeen.dat` entrance renderer `0xd826`, CD gate "Efx off or
    digital device below 2"). Rules, per scene in the evidence table:
    - With speech, every strip wait (29 in `XEEN.EXE`, four entrance
      scenes in `xeen.dat`) ends when the speech has ended and the strip
      has wrapped; that is the Clouds intro, the Clouds ending (Alamar and
      the king speeches), the Darkside intro speeches and the Darkside
      ending speeches.
    - Without speech, the waits stay the CD's, and so do the device gates
      beside them (Alamar plays one clip, the king's skip, the room's
      0xdc-tick hold), except: the Clouds intro runs all 14 narration slots,
      without the CD's forced step 20; its slots 0-12 end on speech idle
      and slot 13 waits for the wrap (also with speech).
    - In both cases: a scene that clears the position to 0 keeps the strip
      off (later queen clips, pharoh9a); the Release creature caption starts
      at position 1 (CD 0); the Clouds ending's Alamar start marks the strip
      timer; the entrance scenes start their cursor at 1 (CD 0) and keep the
      CD's text-mode phase rules.
    - English keeps the CD gates and timing exactly.
  - **N3.** The program draws the `\n` in monster names as a line break in
    both places; it spaces the combat rows 18 pixels apart (y 2/20/38) and
    moves their three hit rows to y 10, 28 and 46, 18 high.
- **Release notes.** The M110 release notes say that going back to an older
  build rejects the six-key `settings.ini` (decision 1) and so loses the
  won-ending flags, because release and development builds share the save
  folder (maintainer, 2026-11-03).

## Risks

- Part B is the largest cost. Part 0b bounds it; isolated fields that cannot
  be identified are as blocking as whole areas that differ in code.
- The Darkside translation changed the program "to support Cyrillic"; such
  changes may include layout (window sizes, positions) that is code.
- Russian strings are longer than the CD's fixed field extents; the Russian
  layout gives its own extents, and drawing follows the Russian program.
- A structured merge may meet a format whose text and data cannot be
  separated safely; that entry stays CD and is listed.
- Only the maintainer can judge play feel; Russian text quality needs a
  reader of Russian (the issue's author offered to help).

## Acceptance

The complete CTest suite passes with and without the Russian material.
English behavior is unchanged; English tests change only where a deliberate
version-policy assertion changes (decision 11), and where decision 1's
six-key `settings.ini` changes the expected settings bytes in
`tests/XeenEndingTests.cpp`, a deliberate settings-format change (maintainer,
2026-11-03). Automated evidence, with
Russian-data tests registered through `mmodern_data_test()` under a new
material variable:
- per substituted entry, its non-text content equals the CD's, with
  format-level tests for representative conflicts and the New Game
  containers;
- English frames and texts equal before and after the layer exists;
- save/load round trips across languages, with Russian and English names,
  and the English slot list and character displays with Cyrillic names;
- settings preserved; invalid, partial or unknown translations refused;
- the completeness test of decision 6 and the argument signatures of
  decision 7;
- grammatical boundaries (1, 2-4, 5-20, 21, 11-14) for every plural and
  both sexes for every gendered form;
- intro, ending and score frames in Russian, rendered and inspected by the
  agent.
One final maintainer session covers only what the above cannot: Cyrillic
entry feel (Shift capitals, a Cyrillic save name), hotkeys and the Bank by
mouse and keyboard together, a mirror answer with capitals, clipping in a
full session and the audiovisual presentation of an intro with English
speech under the scrolling Russian strip; the guide says why for each
item. If the maintainer wants, a Russian reader checks the text. Independent
reviews of the plan and of the implementation, as Tier A requires.
