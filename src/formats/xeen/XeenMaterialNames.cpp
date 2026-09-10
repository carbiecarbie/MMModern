#include "formats/xeen/XeenMaterialNames.h"

#include <cstddef>
#include <string_view>
#include <utility>

namespace mmodern {
namespace {

constexpr std::size_t kMaximumBytes = 8192;
constexpr std::size_t kMaximumTokenBytes = 63;

XeenMaterialNameParseResult malformed(const char *diagnostic) {
	XeenMaterialNameParseResult result;
	result.diagnostic = diagnostic;
	return result;
}

bool trimByte(std::uint8_t byte) {
	return byte == ' ' || byte == '\t' || byte == '\r' || byte == '\n';
}

std::string sanitize(const std::uint8_t *data, std::size_t size) {
	std::size_t first = 0;
	while (first < size && trimByte(data[first]))
		++first;
	while (size > first && trimByte(data[size - 1]))
		--size;
	std::string result;
	result.reserve(size - first);
	for (std::size_t i = first; i < size; ++i) {
		const std::uint8_t byte = data[i];
		result.push_back(byte >= 32 && byte <= 126 ? static_cast<char>(byte) : '?');
	}
	return result;
}

} // namespace

XeenMaterialNameParseResult parseXeenMaterialNames(
		const std::vector<std::uint8_t> &bytes) {
	if (bytes.size() > kMaximumBytes)
		return malformed("mae.xen exceeds the 8,192-byte material-name limit");

	std::array<std::string, 131> candidate{};
	std::size_t position = 0;
	for (std::size_t index = 0; index < candidate.size(); ++index) {
		const std::size_t start = position;
		while (position < bytes.size() && bytes[position] != 0) {
			if (position - start == kMaximumTokenBytes)
				return malformed("mae.xen contains a material name longer than 63 bytes");
			++position;
		}
		if (position == bytes.size())
			return malformed("mae.xen is truncated before its 131st terminator");
		if (index == 0 && position != start)
			return malformed("mae.xen entry zero must be structurally empty");
		candidate[index] = sanitize(bytes.data() + start, position - start);
		++position;
	}
	if (position != bytes.size())
		return malformed("mae.xen has trailing data after entry 130");
	if (!candidate[0].empty())
		return malformed("mae.xen entry zero must be empty");
	for (std::size_t index = 1; index < candidate.size(); ++index) {
		if (candidate[index].empty())
			return malformed("mae.xen contains an empty required material name");
	}

	XeenMaterialNameParseResult result;
	result.availability = XeenMaterialAvailability::Ready;
	result.names = std::move(candidate);
	return result;
}

} // namespace mmodern
