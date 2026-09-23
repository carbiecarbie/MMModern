#ifndef MMODERN_TESTS_XEEN_M35_CLI_WITNESS_H
#define MMODERN_TESTS_XEEN_M35_CLI_WITNESS_H
#include <functional>
#include <filesystem>
#include "app/XeenGameplayServices.h"
namespace mmodern {
int runM35CliWitness(const XeenGameplayServices &original,
	const std::optional<std::filesystem::path> &target, bool resume,
	std::optional<std::uint32_t> seed, std::optional<std::uint16_t> contract,
	const std::function<int(const XeenGameplayServices &)> &launch);
}
#endif
