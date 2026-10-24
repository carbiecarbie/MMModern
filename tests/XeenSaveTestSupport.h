#ifndef MMODERN_TESTS_XEEN_SAVE_SUPPORT_H
#define MMODERN_TESTS_XEEN_SAVE_SUPPORT_H

#include "XeenPartySnapshotTestSupport.h"
#include "formats/xeen/XeenSaveFormat.h"

#include <zlib.h>
#include <algorithm>
#include <limits>

namespace save_test {
inline std::uint16_t legacyJourneySchema(unsigned content) {
    return static_cast<std::uint16_t>(content<=8 ? content : content<=10 ? 8 : 9);
}

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
		a.temporaryAc == b.temporaryAc && a.experience == b.experience && bool(a.luck)==bool(b.luck) && (!a.luck || (a.luck->permanent==b.luck->permanent && a.luck->temporary==b.luck->temporary)) && a.resistances == b.resistances &&
		bool(a.poisonResistance)==bool(b.poisonResistance) && (!a.poisonResistance ||
		(a.poisonResistance->permanent==b.poisonResistance->permanent && a.poisonResistance->temporary==b.poisonResistance->temporary));
}

inline void sameSnapshot(const XeenSaveSnapshot &a, const XeenSaveSnapshot &b) {
    check(a.insideInn==b.insideInn,"Inn state");
	check(a.name == b.name, "save name changed");
	check(a.resources == b.resources && sameCamera(a.camera, b.camera), "signature/camera changed");
	check(a.food==b.food,"food changed");
 check(a.soundOptions==b.soundOptions,"sound options changed");
	check(a.activeRosterIds == b.activeRosterIds, "membership order changed");
	check(a.maps==b.maps,"per-map state changed");
	check(a.headCache==b.headCache,"head cache changed");
	for(unsigned owner=0;owner<30;++owner)check(a.characters[owner].awards==b.characters[owner].awards,"award counts changed");
	for (std::size_t i = 0; i < a.characters.size(); ++i)
		remove_test::checkSameCharacter(a.characters[i], b.characters[i]);
	check(a.questItems == b.questItems && a.questFlags == b.questFlags && a.gameFlags == b.gameFlags,
		"independent counters/flags changed");
	check(bool(a.journey)==bool(b.journey),"Journey presence changed");
	if(a.journey) {
		const auto &x=*a.journey,&y=*b.journey;
		check(x.entry==y.entry&&x.schema==y.schema&&x.content==y.content&&x.context==y.context&&
			x.skeletonSeed==y.skeletonSeed && x.random==y.random && x.purse==y.purse &&
			x.worldFlags==y.worldFlags && x.serviceEconomy==y.serviceEconomy,"Journey header/economy values changed");
		for(unsigned i=0;i<30;++i) check(x.supplements[i].owner==y.supplements[i].owner&&
			sameInputs(x.supplements[i].inputs,y.supplements[i].inputs),"Journey supplements changed");
	}
}

// A map's monster table of default slots (Present at (0,0), MOB-bound).
inline XeenSaveMonsterTable monsterTable(unsigned count) {
	XeenSaveMonsterTable table;table.originalCount=static_cast<std::uint16_t>(count);table.slots.resize(count);
	return table;
}

// Flat views of the v13 per-map table, in identity order, for tests that
// address overlays across maps. Setters keep the table free of empty entries
// and may forge duplicates, which the encoder must reject.
inline void prune(XeenSaveSnapshot &s) {
	s.maps.erase(std::remove_if(s.maps.begin(),s.maps.end(),[](const XeenSaveMapState &m){return m.empty();}),s.maps.end());
}
inline bool removedEntry(const XeenSaveObjectOverride &v) { return v.x==-128 && v.y==-128; }
inline std::vector<XeenObjectIdentity> removedObjects(const XeenSaveSnapshot &s) {
	std::vector<XeenObjectIdentity> ids;
	for(const auto &m:s.maps)for(const auto &v:m.objects)if(removedEntry(v))ids.push_back({m.id,v.record});
	return ids;
}
inline std::vector<XeenObjectOverride> movedObjects(const XeenSaveSnapshot &s) {
	std::vector<XeenObjectOverride> values;
	for(const auto &m:s.maps)for(const auto &v:m.objects)if(!removedEntry(v))values.push_back({{m.id,v.record},v.x,v.y,v.flipped});
	return values;
}
inline std::vector<XeenEventIdentity> disabledEvents(const XeenSaveSnapshot &s) {
	std::vector<XeenEventIdentity> ids;
	for(const auto &m:s.maps)for(const auto &v:m.events)if(!v.opcode)ids.push_back({m.id,v.record});
	return ids;
}
inline std::vector<XeenExploredTile> exploredTiles(const XeenSaveSnapshot &s) {
	std::vector<XeenExploredTile> tiles;
	for(const auto &m:s.maps)if(m.explored)tiles.push_back({m.id,*m.explored});
	return tiles;
}
inline void addRemovedObject(XeenSaveSnapshot &s,XeenObjectIdentity id) {
	auto &objects=s.map(id.mapId).objects;
	const auto at=std::upper_bound(objects.begin(),objects.end(),id.recordIndex,[](std::size_t r,const XeenSaveObjectOverride &v){return r<v.record;});
	objects.insert(at,{static_cast<std::uint16_t>(id.recordIndex),-128,-128,false});
}
inline void addDisabledEvent(XeenSaveSnapshot &s,XeenEventIdentity id) {
	auto &events=s.map(id.mapId).events;
	const auto at=std::upper_bound(events.begin(),events.end(),id.recordIndex,[](std::size_t r,const XeenSaveEventOverride &v){return r<v.record;});
	events.insert(at,{static_cast<std::uint16_t>(id.recordIndex),0});
}
inline void setRemovedObjects(XeenSaveSnapshot &s,const std::vector<XeenObjectIdentity> &ids) {
	for(auto &m:s.maps)m.objects.erase(std::remove_if(m.objects.begin(),m.objects.end(),removedEntry),m.objects.end());
	for(const auto id:ids)addRemovedObject(s,id);
	prune(s);
}
inline void setDisabledEvents(XeenSaveSnapshot &s,const std::vector<XeenEventIdentity> &ids) {
	for(auto &m:s.maps)m.events.erase(std::remove_if(m.events.begin(),m.events.end(),[](const XeenSaveEventOverride &v){return !v.opcode;}),m.events.end());
	for(const auto id:ids)addDisabledEvent(s,id);
	prune(s);
}
inline void clearExplored(XeenSaveSnapshot &s) {
	for(auto &m:s.maps)m.explored.reset();
	prune(s);
}

inline XeenServiceEconomy literalMerchantEconomy() {
	// Independent possible output: all twenty calls choose Weapons; calls after
	// the eighth discard. First eight levels follow each shop's literal bands.
	XeenServiceEconomy e;
	const std::array<std::array<std::uint8_t,8>,8> materials{{
		{{0,0,0,0,0,0,0,0}},{{0,0,0,0,0,37,37,37}},
		{{0,0,0,0,0,37,37,37}},{{0,0,0,0,0,37,37,37}},
		{{0,0,0,0,0,0,0,0}},{{0,0,0,0,0,37,37,37}},
		{{40,40,40,40,40,41,41,41}},{{39,39,39,39,39,40,40,40}}
	}};
	for (unsigned shop=0;shop<8;++shop) for(unsigned slot=0;slot<8;++slot)
		e.wares[shop/4][shop%4][0][slot]={materials[shop][slot],1,0,0};
	e.bank.gold=0xfedcba98u;e.bank.gems=0xffffffffu;
	return e;
}

inline XeenSaveSnapshot currentWireSnapshot() {
 XeenSaveSnapshot s;s.camera={23,9,11,XeenDirection::West};s.journey.emplace();
 s.activeRosterIds={0,18,14,11,1,6};for(auto &c:s.characters)c.name="Member";
 auto &j=*s.journey;j.schema=XeenSaveFormat::kJourneySchema;j.content=XeenSaveFormat::kJourneyContent;j.context.emplace();
 j.context->day=8;j.context->year=610;j.context->minutes=577;
 for(unsigned i=0;i<30;++i){j.supplements[i].owner=i;auto &v=j.supplements[i].inputs;
  v.luck.emplace();v.resistances.emplace();v.poisonResistance.emplace();s.characters[i].learnedSpells.emplace();}
 j.random.emplace();s.map(23).monsters=monsterTable(19);
 j.purse.emplace();j.worldFlags.emplace();j.serviceEconomy=literalMerchantEconomy();return s;
}

inline XeenSaveSnapshot sample() {
	auto s = currentWireSnapshot();
	s.resources = {{123456, 0x12345678U}, XeenArchiveFingerprint{654321, 0x87654321U}};
	s.camera = {23, 8, 2, XeenDirection::North};
	s.activeRosterIds = {18, 0, 14, 23, 1, 6};
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
		for (unsigned skill=0;skill<c.skills.size();++skill) c.skills[skill]=static_cast<std::uint8_t>(i*19+skill);
		c.hasSpells = i % 2 == 0;
		c.quickOption=i%4;c.currentSpell=i%39;
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
	// Removed objects and MakeNothingHere records on unvisited maps.
	s.map(1).objects = {{0, -128, -128, false}};
	s.map(2).objects = {{1, -128, -128, false}};
	s.map(2).events = {{0, 0}};
	s.map(3).events = {{1, 0}};
	s.map(9999).objects = {{65535, -128, -128, false}};
	s.map(9999).events = {{65535, 0}};
	return s;
}

// Wire offset of the Journey domain byte: header, fingerprints, camera, member
// and roster counts, members, characters, quest counters/flags and game flags.
inline std::size_t journeyOffset(const XeenSaveSnapshot &s) {
	std::size_t n=20+25+1+6+2+s.activeRosterIds.size()+4*s.questItems.size()+s.questFlags.size()+s.gameFlags.size()+4;
	for(const auto &c:s.characters)n+=239+c.name.size();
	return n;
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

} // namespace save_test
#endif
