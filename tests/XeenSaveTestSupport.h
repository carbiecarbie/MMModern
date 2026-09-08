#ifndef MMODERN_TESTS_XEEN_SAVE_SUPPORT_H
#define MMODERN_TESTS_XEEN_SAVE_SUPPORT_H

#include "XeenPartySnapshotTestSupport.h"
#include "formats/xeen/XeenSaveFormat.h"

#include <zlib.h>
#include <limits>

namespace save_test {
using namespace mmodern;
using Bytes = std::vector<std::uint8_t>;

inline void check(bool value, const char *message) {
	if (!value) throw std::runtime_error(message);
}

template<class Function>
void rejects(Function function, const std::string &message = {}) {
	try { function(); }
	catch (const std::exception &error) {
		if (!message.empty() && std::string(error.what()).find(message) == std::string::npos)
			throw std::runtime_error("wrong rejection: " + std::string(error.what()));
		return;
	}
	throw std::runtime_error("expected rejection did not occur");
}

inline bool sameCamera(const XeenCamera &a, const XeenCamera &b) {
	return a.mapId == b.mapId && a.x == b.x && a.y == b.y && a.direction == b.direction;
}

inline void sameSnapshot(const XeenSaveSnapshot &a, const XeenSaveSnapshot &b) {
	check(a.resources == b.resources && sameCamera(a.camera, b.camera), "signature/camera changed");
	check(a.activeRosterIds == b.activeRosterIds, "membership order changed");
	for (std::size_t i = 0; i < a.characters.size(); ++i)
		remove_test::checkSameCharacter(a.characters[i], b.characters[i]);
	check(a.questItems == b.questItems && a.questFlags == b.questFlags && a.gameFlags == b.gameFlags,
		"independent counters/flags changed");
	check(a.disabledObjects == b.disabledObjects && a.disabledEvents == b.disabledEvents,
		"independent world identities changed");
}

inline XeenSaveSnapshot sample() {
	XeenSaveSnapshot s;
	s.resources = {{123456, 0x12345678U}, XeenArchiveFingerprint{654321, 0x87654321U}};
	s.camera = {23, 8, 2, XeenDirection::North};
	s.activeRosterIds = {18, 0, 18, 23, 1, 6};
	for (std::size_t i = 0; i < s.characters.size(); ++i) {
		auto &c = s.characters[i];
		c.name = i == 29 ? "" : "Member" + std::to_string(i);
		c.sex = static_cast<XeenSex>(i * 7);
		c.race = static_cast<XeenRace>(i % 5);
		c.characterClass = static_cast<XeenCharacterClass>(i % 10);
		c.intellect = {static_cast<int>(i) * 10000 + 1, -static_cast<int>(i)};
		c.personality = {static_cast<int>(i) * 10000 + 2, -static_cast<int>(i) - 1};
		c.endurance = {static_cast<int>(i) * 10000 + 3, -static_cast<int>(i) - 2};
		c.permanentLevel = static_cast<int>(i) + 1;
		c.temporaryLevel = static_cast<int>(i % 3) - 1;
		c.temporaryAge = -3;
		c.birthYear = static_cast<std::uint16_t>(580 + i);
		c.currentHp = static_cast<std::int16_t>(static_cast<int>(i) * 3 - 10);
		c.currentSp = -static_cast<std::int16_t>(i);
		c.maxStatSkills = {bool(i & 1), bool(i & 2), bool(i & 4), bool(i & 8)};
		c.hasSpells = i % 2 == 0;
		for (std::size_t j = 0; j < c.conditions.size(); ++j)
			c.conditions[j] = static_cast<std::uint8_t>(i * 11 + j);
		unsigned category = 0;
		for (auto *items : {&c.weapons, &c.armor, &c.accessories}) {
			for (std::size_t j = 0; j < items->size(); ++j)
				(*items)[j] = {static_cast<std::uint8_t>(i + j + category),
					static_cast<std::uint8_t>(i + j + category + 37),
					static_cast<std::uint8_t>(i + j + category + 73)};
			category += 9;
		}
	}
	s.characters[0].name = std::string(16, static_cast<char>(0xe1));
	s.characters[29].race = static_cast<XeenRace>(255);
	s.characters[29].characterClass = static_cast<XeenCharacterClass>(255);
	for (std::size_t i = 0; i < s.questItems.size(); ++i) s.questItems[i] = static_cast<std::uint32_t>(i * 259);
	s.questItems.back() = std::numeric_limits<std::uint32_t>::max();
	for (std::size_t i = 0; i < s.questFlags.size(); ++i) s.questFlags[i] = i % 3 == 1;
	for (std::size_t i = 0; i < s.gameFlags.size(); ++i) s.gameFlags[i] = i % 5 == 2;
	s.disabledObjects = {{1, 0}, {2, 1}, {9999, 0xffffffffU}};
	s.disabledEvents = {{2, 0}, {3, 1}, {9999, 0xffffffffU}};
	return s;
}

inline void put32(Bytes &bytes, std::size_t offset, std::uint32_t value) {
	for (unsigned i = 0; i < 4; ++i) bytes.at(offset + i) = static_cast<std::uint8_t>(value >> (i * 8));
}

inline void fixEnvelope(Bytes &bytes) {
	check(bytes.size() >= 20, "test envelope is incomplete");
	put32(bytes, 12, static_cast<std::uint32_t>(bytes.size() - 20));
	put32(bytes, 16, static_cast<std::uint32_t>(crc32(crc32(0L, Z_NULL, 0),
		bytes.data() + 20, static_cast<uInt>(bytes.size() - 20))));
}

} // namespace save_test
#endif
