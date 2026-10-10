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
// Regenerate by hand (Windows, ucrt64 on PATH for SDL2.dll; env SDL_VIDEODRIVER=dummy,
// SDL_RENDER_DRIVER=software; <game> is the original installation, <out> a
// scratch directory). Build with
//   cmake --build <build> --target mmodern_m44_baseline_witness mmodern_m43_cli_witness
// then run from <out>, hashing the file named in the last column:
//   MMODERN_M44_SCENARIO=mainland mmodern_m44_baseline_witness --journey-region
//       --combat-seed 3626689381 <game> --save-file <out>/mainland.mmsave
//                                                -> mainland.mmsave-final.mmsave
//   MMODERN_M44_SCENARIO=services mmodern_m44_baseline_witness --journey-region
//       --combat-seed 7 <game> --save-file <out>/services-paid.mmsave
//                                                -> services-paid.mmsave-final.mmsave
//   mmodern_m43_cli_witness --journey-region --combat-seed 3626689381 <game>
//       --save-file <out>/temple.mmsave          -> temple.mmsave
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
#include <array>
#include <cstdint>
namespace m44_baseline {
inline constexpr const char *revision = "9d51e42e237a2f54ef2c672a4450c88d4daa2034 + M55 Part B v8";
struct Scenario { const char *name; std::uint32_t seed; const char *sha256; };
inline constexpr std::array<Scenario,3> scenarios{{
    {"mainland-combat-myra-phirna",3626689381u,"2e556705d235b82c550b64570a799be612c16800862c86c7a6e43edb1c087cc7"},
    {"vertigo-buy-repair-training",7u,"03b4e5f7ea7c43f0128e5675fd24d9d5e559e9d1d5a7e12e15351352a3f8997b"},
    {"temple-recovery",3626689381u,"6f730c7f09160c7b6ee309c5042f6120ea82501da8c874e628fb8bb39a100d1f"}
}};
}
#endif

