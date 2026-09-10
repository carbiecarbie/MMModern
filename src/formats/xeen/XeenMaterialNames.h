#ifndef MMODERN_FORMATS_XEEN_XEEN_MATERIAL_NAMES_H
#define MMODERN_FORMATS_XEEN_XEEN_MATERIAL_NAMES_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mmodern {

enum class XeenMaterialAvailability {
	Ready,
	Missing,
	Malformed,
	ReadError
};

struct XeenMaterialNameParseResult {
	XeenMaterialAvailability availability = XeenMaterialAvailability::Malformed;
	std::array<std::string, 131> names{};
	std::string diagnostic;

	bool ready() const { return availability == XeenMaterialAvailability::Ready; }
};

XeenMaterialNameParseResult parseXeenMaterialNames(
	const std::vector<std::uint8_t> &bytes);

} // namespace mmodern

#endif
