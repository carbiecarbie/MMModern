#include "formats/xeen/XeenEventFormat.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace mmodern {

std::vector<XeenEventRecord> XeenEventFormat::parse(
		const std::vector<std::uint8_t> &bytes) {
	// ScummVM reference: engines/mm/xeen/scripts.cpp,
	// MazeEvent::synchronize / MazeEvents::synchronize.
	std::vector<XeenEventRecord> records;
	std::size_t offset = 0;

	while (offset < bytes.size()) {
		const std::size_t recordOffset = offset;
		const std::uint8_t length = bytes[offset++];
		if (length < 5) {
			throw std::runtime_error("maze.evt no offset " +
				std::to_string(recordOffset) + ": comprimento menor que 5");
		}
		if (static_cast<std::size_t>(length) > bytes.size() - offset) {
			throw std::runtime_error("maze.evt no offset " +
				std::to_string(recordOffset) + ": registro truncado");
		}

		XeenEventRecord record;
		record.fileOffset = recordOffset;
		record.lengthField = length;
		record.x = bytes[offset++];
		record.y = bytes[offset++];
		record.direction = bytes[offset++];
		record.line = bytes[offset++];
		record.opcode = bytes[offset++];

		const std::size_t parameterCount = static_cast<std::size_t>(length) - 5;
		record.parameters.insert(record.parameters.end(), bytes.begin() + offset,
			bytes.begin() + offset + parameterCount);
		offset += parameterCount;
		records.push_back(std::move(record));
	}

	return records;
}

} // namespace mmodern
