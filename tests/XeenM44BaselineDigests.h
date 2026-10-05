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
#include <array>
#include <cstdint>
namespace m44_baseline {
inline constexpr const char *revision = "26390b3516e646d68d548f04692f86f0bb60d771 + M50 Part A steps 4-5";
struct Scenario { const char *name; std::uint32_t seed; const char *sha256; };
inline constexpr std::array<Scenario,3> scenarios{{
    {"mainland-combat-myra-phirna",3626689381u,"aa76a877a74183799490f17b184b281cfa027f3459353b384ea97cd245eee095"},
    {"vertigo-buy-repair-training",7u,"8fc12cb33c7f85a973f6231f1a4c9ab90153ff84f0d63040941f54f3ffa16ae4"},
    {"temple-recovery",3626689381u,"d7dd4f5ebf5b4bd6632b1fd50b88a5d2c3430f559fc5202e9ff733734a690b4d"}
}};
}
#endif

