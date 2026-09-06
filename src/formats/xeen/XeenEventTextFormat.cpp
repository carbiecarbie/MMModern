#include "formats/xeen/XeenEventTextFormat.h"

namespace mmodern {

std::vector<std::string> XeenEventTextFormat::parse(const std::vector<std::uint8_t> &bytes) {
	std::vector<std::string> strings;
	std::string current;
	for (const std::uint8_t byte : bytes) {
		if (byte == 0) {
			strings.push_back(current);
			current.clear();
		} else {
			current.push_back(static_cast<char>(byte));
		}
	}
	if (!current.empty())
		strings.push_back(current);
	return strings;
}

} // namespace mmodern
