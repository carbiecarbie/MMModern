#ifndef MMODERN_FORMATS_XEEN_XEEN_MATERIAL_NAMES_H
#define MMODERN_FORMATS_XEEN_XEEN_MATERIAL_NAMES_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "formats/xeen/XeenLanguage.h"

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

// Russian (M110 Part C): entry zero is one space and names are cp866, kept
// as bytes; English requires an empty entry zero and shows ASCII only.
XeenMaterialNameParseResult parseXeenMaterialNames(
	const std::vector<std::uint8_t> &bytes, XeenLanguage language = XeenLanguage::English);

} // namespace mmodern

#endif
