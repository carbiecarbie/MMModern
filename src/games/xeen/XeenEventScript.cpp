#include "games/xeen/XeenEventScript.h"

#include <stdexcept>
#include <utility>

namespace mmodern {
namespace {

std::uint8_t physicalDirectionByte(XeenDirection direction) {
	switch (direction) {
	case XeenDirection::North:
	case XeenDirection::East:
	case XeenDirection::South:
	case XeenDirection::West:
		return static_cast<std::uint8_t>(direction);
	}
	throw std::invalid_argument("direcao fisica de evento invalida");
}

bool sameKey(const XeenEventRecord &left, const XeenEventRecord &right) {
	return left.x == right.x && left.y == right.y &&
		left.direction == right.direction && left.line == right.line;
}

} // namespace

XeenEventScript::XeenEventScript(XeenEventFile eventFile) :
	_eventFile(std::move(eventFile)) {
}

const XeenEventRecord *XeenEventScript::findInstruction(std::uint8_t x,
		std::uint8_t y, XeenDirection direction, std::uint8_t line) const {
	const std::uint8_t directionByte = physicalDirectionByte(direction);
	for (const XeenEventRecord &record : _eventFile.records) {
		if (record.x == x && record.y == y && record.line == line &&
				(record.direction == directionByte ||
				 record.direction == kXeenEventDirectionAll))
			return &record;
	}
	return nullptr;
}

std::vector<XeenEventDuplicateKey> XeenEventScript::duplicateKeys() const {
	std::vector<XeenEventDuplicateKey> duplicates;
	for (std::size_t duplicateIndex = 1;
			duplicateIndex < _eventFile.records.size(); ++duplicateIndex) {
		const XeenEventRecord &duplicate = _eventFile.records[duplicateIndex];
		for (std::size_t firstIndex = 0; firstIndex < duplicateIndex; ++firstIndex) {
			const XeenEventRecord &first = _eventFile.records[firstIndex];
			if (sameKey(first, duplicate)) {
				duplicates.push_back({duplicate.x, duplicate.y, duplicate.direction,
					duplicate.line, first.fileOffset, duplicate.fileOffset});
				break;
			}
		}
	}
	return duplicates;
}

} // namespace mmodern
