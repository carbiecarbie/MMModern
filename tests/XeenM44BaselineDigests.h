#ifndef MMODERN_TESTS_M44_BASELINE_DIGESTS_H
#define MMODERN_TESTS_M44_BASELINE_DIGESTS_H
// SHA-256 of the final saved state of four original-data process scenarios,
// recorded at the baseline revisions below. The hash is over the raw .mmsave
// bytes; never commit the .mmsave files (they contain original-data bytes).
//
// CTest runs them as xeen_m44_scenario_mainland, xeen_m44_scenario_services,
// xeen_m44_scenario_temple and xeen_m44_scenario_roundtrip
// (tests/XeenM44ScenarioTests.cpp, label `process`): each generates the final
// save in a temporary directory, compares its SHA-256 with the table below, then
// reloads it, saves again and requires identical bytes. The roundtrip scenario
// also resumes from its checkpoints A and B and requires identical finals.
//
// Audit with tools/m44-digest-audit.ps1: Preserve the current build outside the
// repository before rebuilding, then Compare against the candidate build.
// See tools/AGENTS.md for commands, prerequisites, reports and exit codes.
// The tool verifies these hashes but never edits them; replacements still need
// the maintainer's explicit approval of the complete byte/trace/field audit.
//
// The M44 witness drives the production application through presented-frame
// input and saves with the native F9 key at the end ("final" checkpoint);
// mmodern_m44_baseline_restart additionally reloads that save, saves again and
// compares bytes. M50 Part A, based on the step-3 checkpoint below:
// maintainer approved the byte/trace audit and replacements on 2026-10-05.
// Reason: save v5 format-only changes; zero gameplay divergence with old inputs.
// Version, payload length and CRC change; new fields add an empty barrier count
// and a spawnedType per retained city actor. All common payload bytes match.
// Mainland/services/Temple gain 2/94/106 bytes; their complete M49 traces match
// for 7,855/36,316/30,800 lines, including members, actors, RNG and presentation.
// The pre-change executables reproduce all old hashes; each new final save
// reloads and re-saves byte exactly. Local evidence (not committed):
// build-m44/m50-evidence/audit/approval-report.txt and per-route byte/trace JSON.
// Mainland old: d70e358e232c9a4587137630bc54b73ce22bc41d226c3c46fc5685aac02c681d
// Mainland new: aa76a877a74183799490f17b184b281cfa027f3459353b384ea97cd245eee095
// Services old: 6cf28764f2d261858fef8e7bae04806213e7a8580e0adda0d922ad996b90df42
// Services new: 8fc12cb33c7f85a973f6231f1a4c9ab90153ff84f0d63040941f54f3ffa16ae4
// Temple old: b621a290e10b91028419b58e05297d56b916ce42ed0b62dec60ff98ddd59f75e
// Temple new: d7dd4f5ebf5b4bd6632b1fd50b88a5d2c3430f559fc5202e9ff733734a690b4d
//
// M51 Part A: maintainer approved the audit and replacements on 2026-10-06.
// Reason: save v6 format-only changes; food and fire/energy/magic resistances
// added, with zero gameplay divergence in these scenarios. Each save gains
// 182 bytes (2 food + 180 resistance bytes); version, payload length and CRC
// change. Every retained field and all three complete traces are identical.
// Independent format-only projections match; native reload/re-save is exact.
// Local evidence (not committed): build-m44/m51-evidence/approval-report.txt.
// Mainland old: aa76a877a74183799490f17b184b281cfa027f3459353b384ea97cd245eee095
// Mainland new: aae06cee1c21accbdf1869d894b469878ea435f0616ee995738ef800ed97e6a9
// Services old: 8fc12cb33c7f85a973f6231f1a4c9ab90153ff84f0d63040941f54f3ffa16ae4
// Services new: 9a5ca4d79747fe15e70e0c805ebf1c04d06be2141fb531c5fa4b557f916687e1
// Temple old: d7dd4f5ebf5b4bd6632b1fd50b88a5d2c3430f559fc5202e9ff733734a690b4d
// Temple new: 1e231689a02bef951e0e39d370de39ceff674d24ca5c33fbb5f3ea85d05945bd
// M53 Part B: maintainer approved the complete CD byte/trace audit on
// 2026-10-09, before these replacements. Same inputs/seeds and full traces
// (7,855 / 36,316 / 30,800 lines), with zero RNG/time/reward/route divergence.
// Save format remains v6/schema9/content14. Logical encrypted CC size+CRC
// signatures change; Mainland Phirna disabled records 125..135 -> 131..141
// retain identical logical addresses/opcodes/operands after six preceding
// PlayCD insertions. CD Vertigo removes protection/self-disable at
// (15,0,South,4/5), removing Temple's seven-byte {28,764} overlay.
// All other bytes match; payload lengths/CRCs were explicitly recomputed.
// Ten saves/checkpoints reconstructed from only these deltas equal the CD
// candidates byte for byte. Full per-byte and record mapping/raw evidence:
// build-rel/m53-evidence/part-b/digest-audit.txt and digest-audit.json.
// M54 Part A: maintainer approved the v7 format-only audit on 2026-10-09.
// Preserved baseline executables reproduced all three M53 CD hashes.
// Every common gameplay byte and decoded field matches, as do complete traces
// (7,855 / 36,316 / 30,800 lines). All ten checkpoints append only a zero
// absent-name byte; version, payload length and CRC change. Schema 9/content 14
// remain unchanged. First divergence: envelope offset 8. Each final v7 save
// reloads and re-saves exactly. Full byte mapping, CRCs, hashes and evidence:
// build-rel/m54-evidence/digest-audit.json (local, never committed saves).
// M55 Part B: maintainer approved the v8 audit and digest replacement on 2026-10-10.
// Preserved Part A executables reproduced all three v7 hashes. Complete traces
// match for 7,855 / 36,316 / 30,800 lines; every common byte and decoded field
// matches across all ten checkpoints. Each save adds 60 bytes: quickOption and
// currentSpell for all 30 owners, initialized from CHR and retaining accepted
// spell selections. Only envelope version, payload length and CRC also change;
// schema 9/content 14 remain unchanged. Native v8 reload/re-save is exact for
// all three final saves. Full hashes, byte mappings, settings and CRCs:
// build-rel/m55b-evidence/digest-audit.json (local, never committed saves).
// M57 Part A: maintainer approved the v9 audit and digest replacement on 2026-10-10.
// Baseline v8 executables were preserved outside the repository before rebuilding
// and reproduced all three v8 hashes. Complete traces match for
// 7,855 / 36,316 / 30,800 lines; every common byte and decoded field matches
// across all ten checkpoints. Each save adds exactly 3,875 bytes: the awards
// block (1 + 30 * 129 bytes), initialized independently from CHR, and the
// four-byte empty object-override count. Only envelope version, payload length
// and CRC also change; schema 9/content 14 remain unchanged. Independent
// v8-to-v9 projections match, and native v9 reload/re-save is exact for all
// three final saves. No unexplained gameplay, time or RNG differences.
// Full old/new hashes, byte mappings, checkpoint CRCs and validation evidence:
// build-rel/m57a-evidence/digest-audit.json (local, never committed saves).
// M58: maintainer approved the v10 audit and digest replacement on 2026-10-11.
// Clean baseline ba1b41676e0b6c585e5c0b542c445254814a27b8 executables were
// preserved outside the repository before source changes and reproduced all
// three v9 hashes. Complete traces match for 7,855 / 36,316 / 30,800 lines;
// all common decoded fields match across all ten checkpoints, with none removed.
// Each character replaces four skill-presence bytes with all 18 CHR skill bytes:
// 14 * 30 = 420 added bytes. The exploration block adds a two-byte count and
// 35 bytes per physical tile (side, map number and 256-bit mask): 37 bytes for
// Mainland, 107 for Services/Temple. Total growth is 457 / 527 / 527 bytes.
// Only envelope version, payload length and CRC also change; schema 9/content 14
// remain unchanged. Every checkpoint matches the independent v9-to-v10 byte
// projection using CHR skills and masks derived from preserved traces/original
// DAT stepped bits. Native v10 final reload/re-save is exact in all scenarios.
// No unexplained gameplay, time or RNG differences. Full hashes, fields,
// byte alignments and CRCs: build-rel/m58-digest-comparison-01/digest-audit.json.
// Independent byte checks: build-rel/m58-audit-byte-projection.log.
// Evidence stays local; never commit original-data saves.
// Mainland old: 686cc0b27465446b666eecc50c129d611fbda62471eacdc22869a4510116e10f
// Services old: 818c81cec3f060333d0276d2cec4a1e4250205d12127e4405a5dd2679af61f1e
// Temple old: 96b806b150568bc1947ddfc9b27b5ca6e97270a3a7742d8224df24be491278ea
// M59 Part A: maintainer approved the v11 audit and digest replacement on 2026-10-11.
// Clean baseline 7e44908955f6c3b1c356b1e464b737411820587a executables were
// preserved outside the repository before source changes and reproduced all
// three v10 hashes. Complete traces and all common decoded fields are identical
// across all ten checkpoints. Each save grows by exactly 128 bytes: the single
// flag-16 byte becomes 128 world-flag bytes (+127, flag 16 preserved), followed
// later by a zero absent-head-cache byte (+1). Only envelope version 10 -> 11,
// payload length and CRC also change; schema 9/content 14 remain unchanged.
// Every checkpoint matches the independent byte projection, and native v11
// final reload/re-save is exact in all three scenarios. No unexplained changes.
// Preserved baseline: D:/M44-audits/m59-part-a-20261011.
// Local audit: build-rel/m59-digest-comparison/digest-audit.json.
// Independent projection: build-rel/m59-digest-comparison/byte-explanation.json.
// Evidence stays local; never commit original-data saves.
// Mainland old: d296ae3f83cdacb2c2a44828904c89f7f952a5ecf350ffa6f24d09b991a8124a
// Services old: e6dff2020eb0b675643c70923e51c2927e55df525c7d396e66d60f80f2ac7730
// Temple old: 8200689f224f00a977d5df6360a4b2c4d36f65375da6511a155a4975e526722e
// M61 Part C: maintainer approved the v12 audit and replacements on 2026-10-13.
// Clean baseline d5067c44be3ff07e03df90eac4082456a497e225 executables were
// preserved before code changes and reproduced all three v11 hashes.
// Complete traces match for 7,855 / 35,980 / 30,711 lines, and every common
// decoded snapshot field is identical across all ten checkpoints; none removed.
// Each save adds one zero Misc-count byte after the unchanged weapon/armor
// records. The 51 added decoded fields are Misc array size 10 and five zero
// fields per slot (source, material, id, state, frame). Only envelope version
// 11 -> 12, payload length (+1) and CRC also change; schema 9/content 14 remain.
// Every checkpoint matches the independent byte projection; native final
// reload/re-save is exact for all three scenarios. No unexplained differences.
// Preserved baseline: D:/M44-audits/m61c-d5067c44-preserve.
// Local audit: build-rel/m61c-digest-comparison/digest-audit.json.
// Independent projection and every checkpoint's length/CRC deltas:
// build-rel/m61c-digest-comparison/format-explanation.txt and checkpoint-deltas.tsv.
// Evidence stays local; never commit original-data saves.
// Mainland old: d8c361f620b7bb1f5c1d6afb7da88cdbee8fcfb5f82c75a1795acd690ef20d01
// Services old: a08fe83f6ab5267743355cf2138750034a166599cd808fdd6d334faca50989bc
// Temple old: 91b6af7f310ae55f72063add8542c5f1c3d02655e7ba50f2a887860f788c8fb1
// M69 step A1: maintainer approved the fourth scenario's fresh digest on
// 2026-10-15 (no earlier build to compare). "vertigo-f3-roundtrip", save v12:
// a prepared Journey kills the Orc its start view wakes, enters Vertigo, kills
// a Slime and the Slimes a bashed grate releases, searches a crate
// (checkpoint A), returns to F3 (checkpoint B), wounds a second Orc and runs,
// enters and leaves Vertigo, then kills the wounded Orc (final). The
// uninterrupted run and the runs resumed from A and from B end in identical
// finals; native reload/re-save is exact. The other three digests were
// unchanged by A1 (zero-difference audit against 00e84bf). Preserved baseline
// and audits: D:/M44-audits/m69a1-00e84bf, build-rel/m69a1-digest-comparison-2.
// Evidence stays local; never commit original-data saves.
// M69 step A2 (decision 8): maintainer approved the audit and replacements on
// 2026-10-15. Preserved A1 baseline D:/M44-audits/m69a1-a2-baseline reproduced
// all four A1 digests. Every map and game load now rebuilds monsters at MON HP,
// asleep (DOS static: map load 0x0f542 zeroes the array at 0x0f5c7 and refills
// HP at 0x0faa8), so saves project live monsters as MON HP and not woken.
// Mainland/services: traces identical; only the projected activation fields
// (and services' wounded survivor HP 3 -> 25) change. Temple: three trace lines
// show stranded Orcs #10/#11 asleep after the route's return to F3; nothing
// else differs. Roundtrip: Orc #11's flag differs from the first return to F3
// until the next entrance view, and full divergence starts at the second return
// (wounded Orc healed, three more RNG draws); A2 resumes from A and B equal the
// uninterrupted final. Native reload/re-save is exact in all four.
// Local audit: build-rel/m69a2-digest-comparison/digest-audit.json.
// Mainland old: 605fb7ef58148d6ad955faf0951c500dd8f3cc61d079d07710fbf78a9e8a4c27
// Services old: 417bd35216544d12265a116c8750d22953e12161cfeed075fb685a80e3a5d068
// Temple old: 0bd5d8cd9075e42ba3f9681d2c6322a0c23cc81d518654c8c8c48b57b68d0f32
// Roundtrip old: cfdcec346d6584a479e3ded6960ba61c0e79c4cedf52a7e563bbc675c02ba5d8
// M69 step B (save v13): maintainer approved the format-only audit and the
// replacements on 2026-10-15. Preserved A2 baseline D:/M44-audits/m69b-417a1c0
// reproduced all four A2 digests. Complete traces are identical in all four
// scenarios (roundtrip resumes from A and B equal the uninterrupted final);
// every common per-map field is equal across all 14 checkpoints. The per-map
// view loses only each monster's HP and wake fields (not saved, decision 8)
// and gains the generic word/attribute of the two Vertigo grate cells, whose
// faces, mask and unlocked bit are equal. The one changed decoded field,
// roundtrip pendingMask 32 -> 1, indexes the new source list
// {Clouds 23, record 5}. Every byte delta equals the removed v12 blocks
// (actor tables, flat overlay lists) minus the added v13 source list and
// per-map table; saves shrink by 281..968 bytes. Native reload/re-save is
// exact in all four. Local audit: build-rel/digest-comparison-m69b.
// Evidence stays local; never commit original-data saves.
// Mainland old: c453cb54597e04fa2d2bd9fe9c02f951d3438eded5e66e1b1a90c5ec6ce10c62
// Services old: 7d66ab9efe5e6b52f58360c446bcc35f61e1beba602c44c3bd404a71bec69e7c
// Temple old: 8725ee250ffeab5e2a015daebf2caf81af54a95b8d0048b75242ef8d49157503
// Roundtrip old: 9347bfebf93974e807aeca0d641778bf690b3e2d71ded50278bd3b687b6c51b0
// M70 Part A: maintainer approved the audit and replacements on 2026-10-15.
// Preserved foundation baseline D:/M44-audits/m70a-bf49eb3 reproduced all four
// digests. Cause: the Vertigo exit reset's Spawns of slots 50 and 51 are past the
// city's 46 MOB records and stay inert, as in DOS (count byte 0x9c59, which Spawn
// never changes; the .mob writer 0x266e3..0x2671c and map load 0x0f5be stop at
// it), so no script slots 46-51 exist. Temple and roundtrip leave Vertigo
// through that exit: their traces differ only by the six removed ACTOR 46-51
// lines per city actor dump (first at lines 29,450 and 14,301); the filtered
// old traces, including roundtrip resumes A and B, equal the new ones. Only
// map 28's monster table shrinks from 52 to 46 slots (42 bytes); checkpoints
// before the exit are byte-identical. Mainland and services are unchanged.
// Native reload/re-save is exact in all four. Local audit:
// build-rel/m70a-f-digest-comparison/digest-audit.json.
// Evidence stays local; never commit original-data saves.
// Temple old: 63807e54ffb79b4f34cfb5a7d3015425a4571f5cceb0a7311bfbba2c44de4947
// Roundtrip old: 5ec76e2426b856f3cb94e78c4c1a88742b7ab1991abbce035e6ed4c747d55d91
// M71: maintainer approved the complete audit and all four replacements on
// 2026-10-16. DOS makeItem corrections change category/subcategory bounds,
// level-7 selector consumption, weapon effectiveness and Misc charges; level-1
// drops now share makeItem. Ordinary gameplay route repairs preserve paid Fix,
// Training, Heal and resurrection. All 4,713 changed checkpoint fields are
// explained; no fields/checkpoints were added or removed, save v13/schema9/
// content14 and lengths are unchanged. Native final reload/re-save is exact
// in all four scenarios; Roundtrip resumes A and B equal the uninterrupted final.
// Verified Preserve: D:/M44-audits/makeitem-e035c5c.
// Local audit: build-rel/m71-digest-comparison-1/digest-audit.json and
// field-explanations.tsv; approval/repair report: build-rel/m71-report.md.
// Evidence stays local; never commit original-data saves.
// Temple Heal price (maintenance): maintainer approved the audit and the
// services replacement on 2026-10-16. The Heal price follows DOS XEEN.DAT
// 0x1c9ce: HP is compared unsigned (0x1cadb), so the services learner,
// resurrected from Dead at negative HP with level 3, no longer pays the 30-gold
// HP term. The only differences: 137 trace TREASURE lines (210 -> 240,
// 120 -> 150) and the gold field of both checkpoints (120 -> 150), with CRC.
// Mainland, temple and roundtrip are unchanged; per-map views are equal and
// native reload/re-save is exact. Verified Preserve:
// D:/M44-audits/temple-59855e8. Local audit:
// build-rel/digest-comparison-temple/digest-audit.json; report:
// build-rel/temple-price-report.md. Evidence stays local.
// Services old: f902b8f63391083bf592416b27653064d58ba3a56a6a9e1d3faf35a3780ae10f
// M72: maintainer approved all four replacements on 2026-10-17 after the
// complete byte/trace/field audit against preserved e21f398 executables.
// Save v14 adds an absent optional tip-state byte per retained map; schema9/
// content14 stay unchanged. Manual Space now charges time and services the
// original monster opportunity before arrival; repeated paid Temple actions
// add cumulative farewell days. These explain all 45 changed and 38 added
// decoded fields across 14 checkpoints, including mainland contact/damage/RNG.
// An independent v13 byte projection reconstructs every candidate exactly.
// Native reload/re-save is exact in all four scenarios; both roundtrip resumes
// equal the uninterrupted final. No unexplained differences remain.
// Preserve: D:/M44-audits/m72-e21f398. Local evidence (never commit saves):
// build-rel/digest-comparison-m72-e21f398/{digest-audit.json,
// field-explanations.tsv,byte-projection.json}; build-rel/m72-report.md.
// Mainland old: 4e0a9cfffd793ea50532a31b916c7aa126f34429697e2a0267cd763bb5a9cb76
// Services old: fad236725369d0d418c89c80dd2eb34693ed6cd036b0068254773cfcdd0ba826
// Temple old: 7ee0e06f95e230647a14b7f4489cbe166f0a9c9f711b509030efb7d9bb52ccd8
// Roundtrip old: cdceb44457f4a569afb05129992bc1e9705c20cb9e92b0880a28c16bcf15fb32
#include <array>
#include <cstdint>
namespace m44_baseline {
inline constexpr const char *revision = "M72 (save v14), baseline e21f398";
struct Scenario { const char *name; std::uint32_t seed; const char *sha256; };
inline constexpr std::array<Scenario,4> scenarios{{
    {"mainland-combat-myra-phirna",3626689381u,"be95a00f4b9d977770bd63aa4f5bdee066c26f0c2c269a7d5df9cfd777c694d6"},
    {"vertigo-buy-repair-training",7u,"b92a66716745852863813c7a8ecac579335444d8bb6353cd00c199c7b5ea92a1"},
    {"temple-recovery",3626689381u,"f2b207207a0a35d92f57b85af24b4b85c1d4625499cfde10017696601c075190"},
    {"vertigo-f3-roundtrip",3626689381u,"6b6396f2254cae6928fafefcdec9dfcf928d21a36244b35ad3bda858844898cd"}
}};
}
#endif

