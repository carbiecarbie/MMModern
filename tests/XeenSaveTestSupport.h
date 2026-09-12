#ifndef MMODERN_TESTS_XEEN_SAVE_SUPPORT_H
#define MMODERN_TESTS_XEEN_SAVE_SUPPORT_H

#include "XeenPartySnapshotTestSupport.h"
#include "formats/xeen/XeenSaveFormat.h"

#include <zlib.h>
#include <algorithm>
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

inline bool sameInputs(const XeenCombatInputs &a, const XeenCombatInputs &b) {
	return a.might.permanent == b.might.permanent && a.might.temporary == b.might.temporary &&
		a.speed.permanent == b.speed.permanent && a.speed.temporary == b.speed.temporary &&
		a.accuracy.permanent == b.accuracy.permanent && a.accuracy.temporary == b.accuracy.temporary &&
		a.temporaryAc == b.temporaryAc && a.experience == b.experience;
}

inline bool sameCompleted(const std::optional<XeenSaveCompletedEncounter> &a,
		const std::optional<XeenSaveCompletedEncounter> &b) {
	if (bool(a) != bool(b)) return false;
	if (!a) return true;
	if (a->entry != b->entry || a->victory != b->victory ||
		a->accountingConsumed != b->accountingConsumed || !(a->monster == b->monster) ||
		!(a->context == b->context)) return false;
	for (std::size_t i = 0; i < a->supplements.size(); ++i)
		if (a->supplements[i].owner != b->supplements[i].owner ||
			!sameInputs(a->supplements[i].inputs, b->supplements[i].inputs)) return false;
	return true;
}

inline void sameSnapshot(const XeenSaveSnapshot &a, const XeenSaveSnapshot &b) {
	check(a.resources == b.resources && sameCamera(a.camera, b.camera), "signature/camera changed");
	check(a.itemState == b.itemState, "item presence changed");
	check(a.activeRosterIds == b.activeRosterIds, "membership order changed");
	for (std::size_t i = 0; i < a.characters.size(); ++i)
		remove_test::checkSameCharacter(a.characters[i], b.characters[i]);
	check(a.questItems == b.questItems && a.questFlags == b.questFlags && a.gameFlags == b.gameFlags,
		"independent counters/flags changed");
	check(a.disabledObjects == b.disabledObjects && a.disabledEvents == b.disabledEvents,
		"independent world identities changed");
	check(sameCompleted(a.completedEncounter, b.completedEncounter),
		"completed encounter extension changed");
	check(bool(a.journey)==bool(b.journey),"Journey presence changed");
	if(a.journey) {
		const auto &x=*a.journey,&y=*b.journey;
		check(x.entry==y.entry&&x.schema==y.schema&&x.contract==y.contract&&x.context==y.context&&
			x.skeletonSeed==y.skeletonSeed&&x.initializedMap==y.initializedMap&&x.originalActorCount==y.originalActorCount&&
			x.actors.size()==y.actors.size(),"Journey header values changed");
		for(unsigned i=0;i<30;++i) check(x.supplements[i].owner==y.supplements[i].owner&&
			sameInputs(x.supplements[i].inputs,y.supplements[i].inputs),"Journey supplements changed");
		for(std::size_t i=0;i<x.actors.size();++i) {const auto &p=x.actors[i],&q=y.actors[i];
			check(p.id==q.id&&p.x==q.x&&p.y==q.y&&p.hp==q.hp&&p.activated==q.activated&&
				p.lifecycle==q.lifecycle&&p.status==q.status&&p.accounted==q.accounted,"Journey actor record changed");
		}
	}
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
		for (auto *items : {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous}) {
			for (std::size_t j = 0; j < items->size(); ++j)
				(*items)[j] = {static_cast<std::uint8_t>(i + j + category),
					static_cast<std::uint8_t>(j % 3 == 1 ? 0 : i + j + category + 111),
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

inline XeenSaveSnapshot completedSample() {
	auto s = sample();
	s.camera = {20, 14, 2, XeenDirection::East};
	s.activeRosterIds.assign(kXeenCombatOwners.begin(), kXeenCombatOwners.end());
	XeenSaveCompletedEncounter completed;
	completed.context = {XeenBehaviorProfile::WorldOfXeenClouds, XeenDifficulty::Adventurer,
		23, 1, 610, 959, {}, {}, false, false};
	constexpr std::array<std::uint8_t, 6> owners{0, 1, 6, 11, 14, 18};
	for (std::size_t i = 0; i < owners.size(); ++i) {
		auto &r = completed.supplements[i]; r.owner = owners[i];
		r.inputs.might = {int(i * 7), int(i * 7 + 1)};
		r.inputs.speed = {int(i * 7 + 2), int(i * 7 + 3)};
		r.inputs.accuracy = {int(i * 7 + 4), int(i * 7 + 5)};
		r.inputs.temporaryAc = int(i * 7 + 6);
		r.inputs.experience = i == 0 ? 0U : 0x10203040U + static_cast<std::uint32_t>(i * 0x01010101U);
	}
	s.completedEncounter = completed;
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

// Independent checksum repair for explicitly constructed legacy/wire fixtures.
inline void fixIndependentEnvelope(Bytes &bytes) {
	put32(bytes, 12, static_cast<std::uint32_t>(bytes.size() - 20));
	std::uint32_t crc = 0xffffffffU;
	for (std::size_t i = 20; i < bytes.size(); ++i) {
		crc ^= bytes[i];
		for (int bit = 0; bit < 8; ++bit)
			crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0);
	}
	put32(bytes, 16, crc ^ 0xffffffffU);
}

// Explicit old-layout construction, independent of the production encoder.
// Three membership bytes and thirty three-byte names extend the minimal v1.
inline Bytes nonzeroLegacy() {
	Bytes bytes(4957 + 3 + 30 * 3, 0);
	const Bytes prefix{'M','M','M','S','A','V','E',0,1,0,0,0};
	std::copy(prefix.begin(), prefix.end(), bytes.begin());
	bytes[20] = 99; bytes[28] = 88; // Synthetic archive signature; dark absent.
	bytes[46] = 1; bytes[48] = 1; bytes[49] = 1;
	bytes[51] = 3; bytes[52] = 18; bytes[53] = 0; bytes[54] = 18; bytes[55] = 30;
	for (unsigned i = 0; i < 30; ++i) {
		const auto base = 56 + 152 * i; // 149 fixed bytes plus name contents.
		bytes[base] = i; bytes[base + 1] = 3;
		bytes[base + 2] = 'R'; bytes[base + 3] = '0' + i / 10; bytes[base + 4] = '0' + i % 10;
		bytes[base + 32] = 1; // Permanent level, after name and attributes.
		for (unsigned category = 0; category < 3; ++category)
			for (unsigned slot = 0; slot < 9; ++slot) {
				const auto item = base + 49 + category * 27 + slot * 3;
				bytes[item] = 140 + i + category * 9 + slot;
				bytes[item + 1] = 33 + category * 29 + slot;
				bytes[item + 2] = 2 + i;
			}
		bytes[base + 130] = 40 + i; bytes[base + 132] = 5 + i;
		bytes[base + 150] = 0x50; bytes[base + 151] = 2; // Year 592.
	}
	const auto afterRoster = 56 + 152 * 30;
	put32(bytes, afterRoster + 17 * 4, 3);
	bytes[afterRoster + 140 + 2] = 1;
	bytes[afterRoster + 170 + 7] = 1;
	fixIndependentEnvelope(bytes);
	return bytes;
}

inline void distinctiveInitialItems(XeenRoster &roster) {
	for (unsigned i = 0; i < 30; ++i) {
		auto &c = roster.at(i);
		unsigned category = 0;
		for (auto *items : {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous}) {
			for (unsigned slot = 0; slot < 9; ++slot)
				(*items)[slot] = {static_cast<std::uint8_t>(200 + category),
					static_cast<std::uint8_t>(slot == 1 ? 0 : 1 + i * 7 + category * 17 + slot),
					static_cast<std::uint8_t>(255 - slot), static_cast<std::uint8_t>(230 + category)};
			++category;
		}
	}
}

inline XeenSaveSnapshot expectedLegacy(const XeenSaveSnapshot &legacy, const XeenRoster &initial) {
	check(legacy.itemState == XeenSaveItemState::LegacyV1MissingFields, "fixture was not decoded as v1");
	check(legacy.activeRosterIds == std::vector<std::uint8_t>({18, 0, 18}) &&
		legacy.questItems[17] == 3 && legacy.questFlags[2] && legacy.gameFlags[7],
		"independent v1 membership/quest/flag layout differs");
	auto expected = legacy;
	expected.itemState = XeenSaveItemState::Complete;
	for (unsigned i = 0; i < 30; ++i) {
		auto &c = expected.characters[i];
		check(c.name == "R" + std::string(i < 10 ? "0" : "") + std::to_string(i) &&
			c.permanentLevel == 1 && c.currentHp == 40 + i && c.currentSp == 5 + i && c.birthYear == 592,
			"independent v1 fields around item block differ");
		const auto &defaults = initial.at(i);
		const XeenItemCategory *sources[]{&defaults.weapons, &defaults.armor, &defaults.accessories};
		XeenItemCategory *targets[]{&c.weapons, &c.armor, &c.accessories};
		for (unsigned category = 0; category < 3; ++category)
			for (unsigned slot = 0; slot < 9; ++slot) {
				auto &item = (*targets[category])[slot];
				check(item.material == 140 + i + category * 9 + slot &&
					item.state == 33 + category * 29 + slot && item.frame == 2 + i && item.id == 0,
					"independent v1 triple layout differs");
				item.id = (*sources[category])[slot].id;
			}
		c.miscellaneous = defaults.miscellaneous;
	}
	return expected;
}

} // namespace save_test
#endif
