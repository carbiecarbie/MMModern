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
// compares bytes. Reproduced on the Stage A tree: all three digests match.
#include <array>
#include <cstdint>
namespace m44_baseline {
inline constexpr const char *revision = "a72a09916758c57b51a9a2590a9aac6eedb7c442";
struct Scenario { const char *name; std::uint32_t seed; const char *sha256; };
inline constexpr std::array<Scenario,3> scenarios{{
    {"mainland-combat-myra-phirna",3626689381u,"bfae8220526671fb046a939e06b13553a34631a9183bc74b24473ac03e1a26f3"},
    {"vertigo-buy-repair-training",7u,"b1462449b8d12953a26d6e4edfcb264353c71bbb965b64ce3dc8f4d14fb74402"},
    {"temple-recovery",3626689381u,"813cf5e7fcfa8d87ba90cfc8c5a6c4471488ffbcebd0c63440afde4098f90ac0"}
}};
}
#endif

