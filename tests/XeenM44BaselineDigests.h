#ifndef MMODERN_TESTS_M44_BASELINE_DIGESTS_H
#define MMODERN_TESTS_M44_BASELINE_DIGESTS_H
// SHA-256 of the final saved state of three original-data process scenarios,
// recorded at the baseline revision below. The hash is over the raw .mmsave
// bytes; never commit the .mmsave files (they contain original-data bytes).
//
// CTest runs them as xeen_m44_scenario_mainland, xeen_m44_scenario_services and
// xeen_m44_scenario_temple (tests/XeenM44ScenarioTests.cpp, label `process`): each
// generates the final save in a temporary directory, compares its SHA-256 with
// the table below, then reloads it, saves again and requires identical bytes.
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
#include <array>
#include <cstdint>
namespace m44_baseline {
inline constexpr const char *revision = "7e44908955f6c3b1c356b1e464b737411820587a + M59 Part A v11";
struct Scenario { const char *name; std::uint32_t seed; const char *sha256; };
inline constexpr std::array<Scenario,3> scenarios{{
    {"mainland-combat-myra-phirna",3626689381u,"d8c361f620b7bb1f5c1d6afb7da88cdbee8fcfb5f82c75a1795acd690ef20d01"},
    {"vertigo-buy-repair-training",7u,"a08fe83f6ab5267743355cf2138750034a166599cd808fdd6d334faca50989bc"},
    {"temple-recovery",3626689381u,"91b6af7f310ae55f72063add8542c5f1c3d02655e7ba50f2a887860f788c8fb1"}
}};
}
#endif

