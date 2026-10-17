#include "formats/xeen/XeenSaveFormat.h"

#include <zlib.h>

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mmodern {
namespace {

constexpr std::array<std::uint8_t, 8> kMagic{'M', 'M', 'M', 'S', 'A', 'V', 'E', 0};
static_assert(std::numeric_limits<int>::digits == 31, "save requires 32-bit character integers");
static_assert(XeenSaveFormat::kMaximumSize <= std::numeric_limits<uInt>::max(),
	"save CRC input must fit zlib's length type");

void require(bool value, const char *message) {
	if (!value) throw std::runtime_error(std::string("MMModern save: ") + message);
}

// Darkside identities are representable; the Clouds-only restore rejects them.
void validateMap(XeenMapIdentity id) {
	require((id.side == XeenSide::Clouds || id.side == XeenSide::Darkside) && id.number >= 1 && id.number <= 9999,
		"invalid map identity");
}

// MOB and Spawn use signed-byte positions, including original (100,100)
// parking slots. Resource-bound restore separately validates live geometry.
bool monsterCoordinate(int value) { return value >= -128 && value <= 127; }
bool signedByte(int value) { return value >= -128 && value <= 127; }

void validateMonsters(const XeenSaveMonsterTable &table) {
	require(table.slots.size() <= XeenSaveFormat::kMaximumMonsters && table.originalCount <= table.slots.size(),
		"invalid monster table counts");
	for (std::size_t n = 0; n < table.slots.size(); ++n) {
		const auto &a = table.slots[n];
		require(monsterCoordinate(a.x) && monsterCoordinate(a.y) && static_cast<unsigned>(a.lifecycle) <= 3 &&
			(a.status == XeenActorStatus::Physical || a.status == XeenActorStatus::Unsupported),
			"monster value outside wire bounds");
		// A MOB record binds its own type (unresolved when its table entry is
		// absent). Older builds also wrote script slots with their spawned type;
		// restoration drops them because DOS never keeps slots past the MOB count.
		const bool script = n >= table.originalCount;
		if (!script) require(a.spawnedType == -1, "original monster type must bind to MOB");
		else require(a.spawnedType == -1 ? a.lifecycle == XeenActorLifecycle::Unresolved : a.spawnedType >= 0 &&
			a.lifecycle != XeenActorLifecycle::Unresolved, "invalid script-slot MON type");
		switch (a.lifecycle) {
		case XeenActorLifecycle::Present:
			require(!a.accounted && a.x >= 0 && a.y >= 0, "invalid live monster"); break;
		case XeenActorLifecycle::Defeated:
			require(a.accounted && a.x == -128 && a.y == -128, "invalid defeated monster"); break;
		case XeenActorLifecycle::Unresolved:
			require(!a.accounted && (!script || (!a.x && !a.y)), "invalid unresolved monster slot"); break;
		case XeenActorLifecycle::Disabled:
			require(!a.accounted, "invalid disabled monster"); break;
		}
	}
}

template<class Override>
void validateRecords(const std::vector<Override> &values, const char *message) {
	require(values.size() <= XeenSaveFormat::kMaximumOverrides, message);
	for (std::size_t n = 1; n < values.size(); ++n) require(values[n - 1].record < values[n].record, message);
}

void validateMapStates(const std::vector<XeenSaveMapState> &maps) {
	require(maps.size() <= XeenSaveFormat::kMaximumMaps, "too many map states");
	for (std::size_t i = 0; i < maps.size(); ++i) {
		const auto &m = maps[i];
		validateMap(m.id);
		require(i == 0 || maps[i - 1].id < m.id, "map states must be strictly ordered");
		require(!m.empty(), "noncanonical empty map state");
		if (m.monsters) validateMonsters(*m.monsters);
		validateRecords(m.objects, "invalid or duplicate object override");
		for (const auto &v : m.objects)
			require(signedByte(v.x) && signedByte(v.y) && (!v.flipped || v.x != -128 || v.y != -128),
				"invalid object override value");
		validateRecords(m.wallItems, "invalid or duplicate wall-item override");
		for (const auto &v : m.wallItems) require(signedByte(v.x) && signedByte(v.y), "invalid wall-item override value");
		require(m.cells.size() <= XeenSaveFormat::kMaximumCells, "too many cell overrides");
		for (std::size_t n = 0; n < m.cells.size(); ++n) {
			const auto &v = m.cells[n];
			require(n == 0 || m.cells[n - 1].cell < v.cell, "cell overrides must be strictly ordered");
			require(v.word != v.originalWord || v.attributes != v.originalAttributes, "noncanonical unchanged cell override");
		}
		validateRecords(m.events, "invalid or duplicate Event override");
	}
}

// Leading referenced source slots; the saved form has no gap after them.
unsigned treasureSourceCount(const XeenMonsterTreasure &t) {
	unsigned count = 0;
	while (count < t.sources.size() && (t.referencedSources() & (1u << count))) ++count;
	return count;
}

const XeenSaveMonster *findMonster(const std::vector<XeenSaveMapState> &maps, XeenMonsterIdentity id) {
	for (const auto &m : maps)
		if (m.id == id.mapId && m.monsters && id.recordIndex < m.monsters->slots.size())
			return &m.monsters->slots[id.recordIndex];
	return nullptr;
}

std::uint32_t checksum(const std::uint8_t *data, std::size_t size) {
	return static_cast<std::uint32_t>(crc32(crc32(0L, Z_NULL, 0), data, static_cast<uInt>(size)));
}

// Private, fixed-format byte operations; not a general serialization layer.
struct Writer {
	std::vector<std::uint8_t> bytes;
	void integer(std::uint64_t value, unsigned width) {
		require(bytes.size() <= XeenSaveFormat::kMaximumSize - width, "save is oversized");
		for (unsigned i = 0; i < width; ++i)
			bytes.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
	}
	void u8(std::uint8_t value) { integer(value, 1); }
	void u16(std::uint16_t value) { integer(value, 2); }
	void u32(std::uint32_t value) { integer(value, 4); }
	void u64(std::uint64_t value) { integer(value, 8); }
	void i16(std::int16_t value) { u16(static_cast<std::uint16_t>(value)); }
	void i32(int value) { u32(static_cast<std::uint32_t>(value)); }
	void map(XeenMapIdentity id) { u8(static_cast<std::uint8_t>(id.side)); u16(id.number); }
	void fingerprint(XeenArchiveFingerprint value) { u64(value.size); u32(value.crc32); }
};

struct Reader {
	const std::vector<std::uint8_t> &bytes;
	std::size_t position = 0;
	std::size_t remaining() const { return bytes.size() - position; }
	std::uint64_t integer(unsigned width) {
		require(width <= remaining(), "truncated input");
		std::uint64_t value = 0;
		for (unsigned i = 0; i < width; ++i)
			value |= static_cast<std::uint64_t>(bytes[position++]) << (8 * i);
		return value;
	}
	std::uint8_t u8() { return static_cast<std::uint8_t>(integer(1)); }
	std::uint16_t u16() { return static_cast<std::uint16_t>(integer(2)); }
	std::uint32_t u32() { return static_cast<std::uint32_t>(integer(4)); }
	std::uint64_t u64() { return integer(8); }
	std::int16_t i16() {
		const auto value = u16();
		return static_cast<std::int16_t>(value <= 0x7fff ? value : static_cast<int>(value) - 0x10000);
	}
	int i32() {
		const auto value = u32();
		return static_cast<int>(value <= 0x7fffffffU ? static_cast<std::int64_t>(value) :
			static_cast<std::int64_t>(value) - 0x100000000LL);
	}
	bool boolean() {
		const auto value = u8();
		require(value <= 1, "invalid boolean byte");
		return value != 0;
	}
	XeenMapIdentity map() {
		const auto side = static_cast<XeenSide>(u8());
		return {side, u16()};
	}
	XeenArchiveFingerprint fingerprint() {
		const auto size = u64();
		return {size, u32()};
	}
};

void writeCharacter(Writer &out, const XeenCharacter &c) {
	out.u8(c.rosterId);
	out.u8(static_cast<std::uint8_t>(c.name.size()));
	for (unsigned char byte : c.name) out.u8(byte);
	out.u8(static_cast<std::uint8_t>(c.sex));
	out.u8(static_cast<std::uint8_t>(c.race));
	out.u8(static_cast<std::uint8_t>(c.characterClass));
	for (const auto a : {c.intellect, c.personality, c.endurance}) {
		out.i32(a.permanent); out.i32(a.temporary);
	}
	out.i32(c.permanentLevel); out.i32(c.temporaryLevel); out.i32(c.temporaryAge);
	for (const auto value:c.skills) out.u8(value);
	out.u8(c.hasSpells);
	for (const auto *items : {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous})
		for (const auto item : *items) {
			out.u8(item.material); out.u8(item.id); out.u8(item.state); out.u8(item.frame);
		}
	out.i16(c.currentHp); out.i16(c.currentSp);
	for (const auto value : c.conditions) out.u8(value);
	out.u16(c.birthYear);
	out.u8(c.quickOption);out.u8(c.currentSpell);
}

XeenCharacter readCharacter(Reader &in) {
	XeenCharacter c;
	c.rosterId = in.u8();
	const auto length = in.u8();
	require(length <= 16 && length <= in.remaining(), "invalid character name length");
	c.name.assign(reinterpret_cast<const char *>(in.bytes.data() + in.position), length);
	in.position += length;
	c.sex = static_cast<XeenSex>(in.u8());
	c.race = static_cast<XeenRace>(in.u8());
	c.characterClass = static_cast<XeenCharacterClass>(in.u8());
	for (auto *a : {&c.intellect, &c.personality, &c.endurance}) {
		a->permanent = in.i32(); a->temporary = in.i32();
	}
	c.permanentLevel = in.i32(); c.temporaryLevel = in.i32(); c.temporaryAge = in.i32();
	for (auto &value:c.skills) value=in.u8();
	c.hasSpells = in.boolean();
	for (auto *items : {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous})
		for (auto &item : *items) {
			item.material = in.u8();
			item.id = in.u8();
			item.state = in.u8(); item.frame = in.u8();
		}

	c.currentHp = in.i16(); c.currentSp = in.i16();
	for (auto &value : c.conditions) value = in.u8();
	c.birthYear = in.u16();
	c.quickOption=in.u8();c.currentSpell=in.u8();
	return c;
}

void writeMapStates(Writer &out, const std::vector<XeenSaveMapState> &maps) {
	out.u16(static_cast<std::uint16_t>(maps.size()));
	for (const auto &m : maps) {
		out.map(m.id);
		out.u8(m.monsters.has_value());
		if (m.monsters) {
			out.u16(m.monsters->originalCount); out.u16(static_cast<std::uint16_t>(m.monsters->slots.size()));
			for (const auto &a : m.monsters->slots) {
				out.u8(static_cast<std::uint8_t>(a.x)); out.u8(static_cast<std::uint8_t>(a.y));
				out.u8(static_cast<std::uint8_t>(a.lifecycle)); out.u8(a.status == XeenActorStatus::Physical ? 0 : 1);
				out.u8(a.accounted); out.i16(a.spawnedType);
			}
		}
		out.u16(static_cast<std::uint16_t>(m.objects.size()));
		for (const auto &v : m.objects) {
			out.u16(v.record); out.u8(static_cast<std::uint8_t>(v.x)); out.u8(static_cast<std::uint8_t>(v.y)); out.u8(v.flipped);
		}
		out.u16(static_cast<std::uint16_t>(m.wallItems.size()));
		for (const auto &v : m.wallItems) {
			out.u16(v.record); out.u8(static_cast<std::uint8_t>(v.x)); out.u8(static_cast<std::uint8_t>(v.y));
		}
		out.u16(static_cast<std::uint16_t>(m.cells.size()));
		for (const auto &v : m.cells) {
			out.u8(v.cell); out.u16(v.word); out.u8(v.attributes); out.u16(v.originalWord); out.u8(v.originalAttributes);
		}
		out.u16(static_cast<std::uint16_t>(m.events.size()));
		for (const auto &v : m.events) { out.u16(v.record); out.u8(v.opcode); }
		out.u8(m.explored.has_value());
		if (m.explored) for (const auto byte : *m.explored) out.u8(byte);
		out.u8(m.tavernTips.has_value());
		if (m.tavernTips) out.u8(*m.tavernTips);
	}
}

int signedWireByte(std::uint8_t value) { return value < 128 ? int(value) : int(value) - 256; }

// Every count is bounded by the bytes that remain before anything is allocated.
std::vector<XeenSaveMapState> readMapStates(Reader &in) {
	const auto count = in.u16();
	require(count <= XeenSaveFormat::kMaximumMaps && count <= in.remaining() / 14, "invalid map state count");
	std::vector<XeenSaveMapState> maps(count);
	for (auto &m : maps) {
		m.id = in.map();
		if (in.boolean()) {
			XeenSaveMonsterTable table;
			table.originalCount = in.u16();
			const auto slots = in.u16();
			require(slots <= XeenSaveFormat::kMaximumMonsters && slots <= in.remaining() / 7, "invalid monster count");
			table.slots.resize(slots);
			for (auto &a : table.slots) {
				a.x = signedWireByte(in.u8()); a.y = signedWireByte(in.u8());
				const auto life = in.u8(), status = in.u8();
				require(life <= 3 && status <= 1, "invalid monster lifecycle/status");
				a.lifecycle = static_cast<XeenActorLifecycle>(life);
				a.status = status == 0 ? XeenActorStatus::Physical : XeenActorStatus::Unsupported;
				a.accounted = in.boolean(); a.spawnedType = in.i16();
			}
			m.monsters = std::move(table);
		}
		const auto objects = in.u16();
		require(objects <= in.remaining() / 5, "invalid object override count");
		m.objects.resize(objects);
		for (auto &v : m.objects) {
			v.record = in.u16(); v.x = signedWireByte(in.u8()); v.y = signedWireByte(in.u8()); v.flipped = in.boolean();
		}
		const auto wallItems = in.u16();
		require(wallItems <= in.remaining() / 4, "invalid wall-item override count");
		m.wallItems.resize(wallItems);
		for (auto &v : m.wallItems) { v.record = in.u16(); v.x = signedWireByte(in.u8()); v.y = signedWireByte(in.u8()); }
		const auto cells = in.u16();
		require(cells <= XeenSaveFormat::kMaximumCells && cells <= in.remaining() / 7, "invalid cell override count");
		m.cells.resize(cells);
		for (auto &v : m.cells) {
			v.cell = in.u8(); v.word = in.u16(); v.attributes = in.u8(); v.originalWord = in.u16(); v.originalAttributes = in.u8();
		}
		const auto events = in.u16();
		require(events <= in.remaining() / 3, "invalid Event override count");
		m.events.resize(events);
		for (auto &v : m.events) { v.record = in.u16(); v.opcode = in.u8(); }
		if (in.boolean()) {
			std::array<std::uint8_t, 32> bits{};
			for (auto &byte : bits) byte = in.u8();
			m.explored = bits;
		}
		if (in.boolean()) m.tavernTips = in.u8();
	}
	return maps;
}

// Bytes of one decoded map state, for the computed payload extent.
std::size_t mapStateBytes(const XeenSaveMapState &m) {
	return 3 + 1 + (m.monsters ? 4 + 7 * m.monsters->slots.size() : 0) + 2 + 5 * m.objects.size() +
		2 + 4 * m.wallItems.size() + 2 + 7 * m.cells.size() + 2 + 3 * m.events.size() + 1 + (m.explored ? 32 : 0) +
		1 + (m.tavernTips ? 1 : 0);
}

XeenUnsupportedSave unsupportedPair(std::uint16_t schema, std::uint16_t content) {
	return XeenUnsupportedSave(((schema >= 1 && schema <= 8 && schema == content) ||
		(schema == 8 && (content == 9 || content == 10)) ||
		(schema == XeenSaveFormat::kJourneySchema && content >= 11 && content < XeenSaveFormat::kJourneyContent)) ?
		XeenUnsupportedSave::Kind::Older : XeenUnsupportedSave::Kind::Newer);
}

} // namespace

void XeenSaveFormat::validate(const XeenSaveSnapshot &s) {
	require(!s.name || validName(*s.name), "invalid save name");
	if (!s.journey) throw XeenUnsupportedSave(XeenUnsupportedSave::Kind::Older);
	if (s.journey->schema != kJourneySchema || s.journey->content != kJourneyContent) throw unsupportedPair(s.journey->schema, s.journey->content);
	// Map-agnostic: the camera fits the indoor capacity; restore binds it to the
	// camera map's geometry.
	validateMap(s.camera.mapId);
	require(xeenIndoorCoordinate(s.camera.x, s.camera.y) && static_cast<unsigned>(s.camera.direction) <= 3,
		"invalid committed camera");
	require(s.activeRosterIds.size() <= XeenParty::kMaximumVisibleMembers, "too many active members");
	for (const auto id : s.activeRosterIds)
		require(id < XeenRoster::kCharacterCount, "invalid active roster identity");
	for (std::size_t i = 0; i < s.characters.size(); ++i) {
		const auto &c = s.characters[i];
		for(unsigned award=0;award<128;++award)
			require(award==9 || c.awards[award]<=15,"award count exceeds CHR representation");
		require(c.rosterId == i, "character identity differs from roster slot");
		require(c.name.size() <= 16 && c.name.find('\0') == std::string::npos, "invalid character name");
	}
	if(s.headCache)require(s.headCache->mapId==s.camera.mapId,"head cache must belong to the primary map");
	validateMapStates(s.maps);

	const auto &j = *s.journey;
	require(j.entry == XeenEncounterEntry::Journey,
		"unsupported Journey domain/schema/content");
	require(j.serviceEconomy.has_value(), "Journey economy presence mismatch");
	if (j.serviceEconomy) xeenValidateCurrentServiceEconomy(*j.serviceEconomy);
	require(j.worldFlags.has_value(), "Journey recovery presence mismatch");
	require(j.context.has_value(), "missing Journey context");
	// Stock may be depleted on any canonical day, including after year rollover.
	// All dates use the current stock validator, including depletion.
	require(j.context->profile == XeenBehaviorProfile::WorldOfXeenClouds &&
		(j.context->difficulty == XeenDifficulty::Adventurer || j.context->difficulty == XeenDifficulty::Warrior),
		"invalid Journey context enum");
	require(xeenRegionalContext(*j.context), "invalid Journey calendar context");
	for (std::size_t i = 0; i < j.supplements.size(); ++i) {
		const auto &r = j.supplements[i];
		require(r.inputs.luck.has_value(), "Journey Luck presence mismatch");
		require(r.inputs.resistances.has_value(), "Journey resistance presence mismatch");
		require(r.inputs.poisonResistance.has_value(), "Journey poison presence mismatch");
		if (r.inputs.poisonResistance) for (int v:{r.inputs.poisonResistance->permanent,r.inputs.poisonResistance->temporary}) require(v>=0 && v<=255,"Journey poison input outside byte range");
		if (r.inputs.luck) for (int v : {r.inputs.luck->permanent,r.inputs.luck->temporary}) require(v >= 0 && v <= 255, "Journey Luck outside byte range");
		require(r.owner == i, "invalid Journey supplemental owner sequence");
		for (int v : {r.inputs.might.permanent, r.inputs.might.temporary, r.inputs.speed.permanent,
			r.inputs.speed.temporary, r.inputs.accuracy.permanent, r.inputs.accuracy.temporary, r.inputs.temporaryAc})
			require(v >= 0 && v <= 255, "Journey supplement outside byte range");
	}
	require(j.skeletonSeed == 0 && j.random && j.random->algorithm == 1 && j.random->state != 0, "invalid Journey random representation");
	require(j.treasure.has_value(),"Journey treasure presence mismatch");
	xeenValidateMonsterTreasure(*j.treasure);
	// The saved form: pending bits and stored items refer to source slots
	// 0..n-1 exactly, ordered by identity, each a defeated, accounted slot of
	// its own map.
	const auto &sources = j.treasure->sources;
	const auto used = j.treasure->referencedSources();
	const auto count = treasureSourceCount(*j.treasure);
	require(used == (1u << count) - 1, "Monster treasure sources differ from their references");
	for (std::size_t i = 0; i < count; ++i) {
		validateMap(sources[i].mapId);
		require(sources[i].recordIndex < kMaximumMonsters && (i == 0 || sources[i - 1] < sources[i]),
			"Monster treasure sources must be strictly ordered");
		const auto *a = findMonster(s.maps, sources[i]);
		require(a && a->lifecycle == XeenActorLifecycle::Defeated && a->accounted, "Noncanonical monster treasure source");
	}

	for (const auto &character : s.characters) {
		require(character.learnedSpells.has_value(), "Journey learned-spell presence mismatch");
		require(character.quickOption<=3 && (character.currentSpell<39 || character.currentSpell==255), "invalid Quick Fight setting");
	}

}

std::vector<std::uint8_t> XeenSaveFormat::encode(const XeenSaveSnapshot &s) {
	validate(s);
	require(!s.name || validName(*s.name), "invalid save name");
	const auto version = kJourneyVersion;
	Writer out;
	out.bytes.resize(kHeaderSize);
	out.fingerprint(s.resources.clouds);
	out.u8(s.resources.darkside.has_value());
	out.fingerprint(s.resources.darkside.value_or(XeenArchiveFingerprint{}));
	out.map(s.camera.mapId);
	out.u8(static_cast<std::uint8_t>(s.camera.x)); out.u8(static_cast<std::uint8_t>(s.camera.y));
	out.u8(static_cast<std::uint8_t>(s.camera.direction));
	out.u8(static_cast<std::uint8_t>(s.activeRosterIds.size()));
	for (const auto id : s.activeRosterIds) out.u8(id);
	out.u8(static_cast<std::uint8_t>(s.characters.size()));
	for (const auto &c : s.characters) writeCharacter(out, c);
	for (const auto count : s.questItems) out.u32(count);
	for (const bool value : s.questFlags) out.u8(value);
	for (const bool value : s.gameFlags) out.u8(value);

	const auto &j = *s.journey;
	out.u8(3); out.u16(j.schema); out.u16(j.content); out.u8(1);
	const auto &c = *j.context;
	out.u8(0); out.u8(c.difficulty == XeenDifficulty::Adventurer ? 0 : 1);
	out.u16(c.ctr24); out.u16(c.day); out.u16(c.year); out.u16(c.minutes);
	for (auto v : c.effects) out.u8(v);
	for (auto v : c.lightAndResistances) out.u16(v);
	out.u8(c.rested); out.u8(c.newDay); out.u16(s.food); out.u8(30);
	for (const auto &r : j.supplements) {
		out.u8(r.owner);
		for (int v : {r.inputs.might.permanent, r.inputs.might.temporary, r.inputs.speed.permanent,
			r.inputs.speed.temporary, r.inputs.accuracy.permanent, r.inputs.accuracy.temporary, r.inputs.temporaryAc}) out.i32(v);
		out.u32(r.inputs.experience);
		out.i32(r.inputs.luck->permanent); out.i32(r.inputs.luck->temporary);
	}
	 { out.u8(j.random->algorithm); out.u32(j.random->state); out.u64(j.random->count); }

	out.u8(30);
	for(const auto &r:j.supplements) {
		const auto &v=*r.inputs.resistances;out.u8(r.owner);
		out.u8(v.coldPermanent);out.u8(v.coldTemporary);out.u8(v.electricalPermanent);out.u8(v.electricalTemporary);
		out.u8(v.firePermanent);out.u8(v.fireTemporary);out.u8(v.energyPermanent);out.u8(v.energyTemporary);out.u8(v.magicPermanent);out.u8(v.magicTemporary);
	}
	const auto &v=*j.treasure;out.u32(v.gold);out.u32(v.gems);out.u32(v.pendingMask);out.u32(v.pendingGold);
	// v13: the monster identities that pending bits and item sources index.
	const auto sourceCount=treasureSourceCount(v);out.u8(static_cast<std::uint8_t>(sourceCount));
	for(unsigned i=0;i<sourceCount;++i) {out.map(v.sources[i].mapId);out.u16(static_cast<std::uint16_t>(v.sources[i].recordIndex));}
	unsigned weapons=0,armor=0;for(const auto &r:v.weapons) weapons+=r.item.id!=0;for(const auto &r:v.armor) armor+=r.item.id!=0;
	out.u8(weapons);out.u8(armor);
	for(unsigned category=0;category<2;++category) for(const auto &r:category?v.armor:v.weapons) if(r.item.id) {
		out.u8(r.source);out.u8(r.item.material);out.u8(r.item.id);out.u8(r.item.state);out.u8(r.item.frame);
	}
	unsigned miscellaneous=0;for(const auto &r:v.miscellaneous) miscellaneous+=r.item.id!=0;
	out.u8(miscellaneous);
	for(const auto &r:v.miscellaneous) if(r.item.id) {
		out.u8(r.source);out.u8(r.item.material);out.u8(r.item.id);out.u8(r.item.state);out.u8(r.item.frame);
	}
	for (bool flag:j.worldFlags->values) out.u8(flag);

	out.u8(30);
	for (std::size_t owner=0; owner<s.characters.size(); ++owner) {
		out.u8(static_cast<std::uint8_t>(owner));
		for (const auto flag:*s.characters[owner].learnedSpells) out.u8(flag);
	}

	out.u8(30);
	for (unsigned owner=0; owner<30; ++owner) {
		const auto &v=*j.supplements[owner].inputs.poisonResistance;
		out.u8(static_cast<std::uint8_t>(owner));out.u8(static_cast<std::uint8_t>(v.permanent));out.u8(static_cast<std::uint8_t>(v.temporary));
	}

	out.u8(2);out.u8(4);out.u8(4);out.u8(9);
	for (const auto &side:j.serviceEconomy->wares.records)
		for (const auto &shop:side) for (const auto &category:shop) for (const auto &item:category) {
			out.u8(item.material);out.u8(item.id);out.u8(item.state);out.u8(item.frame);
		}
	out.u32(j.serviceEconomy->bank.gold);out.u32(j.serviceEconomy->bank.gems);
	out.u8(30);
	for(unsigned owner=0;owner<30;++owner) {
		out.u8(owner);for(const auto value:s.characters[owner].awards)out.u8(value);
	}
	// v13: every retained map's state, strictly ordered by identity.
	writeMapStates(out, s.maps);
	// v11: bounded primary-map cache overrides; original HED remains resource driven.
 out.u8(s.headCache.has_value());
 if(s.headCache) {
  out.map(s.headCache->mapId);
  unsigned count=0;for(const auto &head:s.headCache->overrides)count+=head.has_value();
  out.u16(count);
  for(unsigned i=0;i<256;++i)if(const auto &head=s.headCache->overrides[i]) {out.u8(i);out.u8(head->left);out.u8(head->right);}
 }
 // Zero is the explicit absent-name encoding for developer loose saves.
	out.u8(s.name ? static_cast<std::uint8_t>(s.name->size()) : 0);
	if (s.name) for (unsigned char byte : *s.name) out.u8(byte);

	Writer header;
	for (const auto byte : kMagic) header.u8(byte);
	header.u16(version); header.u8(0); header.u8(0);
	header.u32(static_cast<std::uint32_t>(out.bytes.size() - kHeaderSize));
	header.u32(checksum(out.bytes.data() + kHeaderSize, out.bytes.size() - kHeaderSize));
	std::copy(header.bytes.begin(), header.bytes.end(), out.bytes.begin());
	return std::move(out.bytes);
}

XeenSaveSnapshot XeenSaveFormat::decode(const std::vector<std::uint8_t> &bytes) {
	require(bytes.size() <= kMaximumSize, "save is oversized");
	Reader in{bytes};
	for (const auto byte : kMagic) require(in.u8() == byte, "unrecognized format");
	const auto version = in.u16();
	require(in.u8() == 0, "unsupported game side");
	require(in.u8() == 0, "nonzero reserved byte");
	const auto length = in.u32();
	const auto crc = in.u32();
	require(length == in.remaining(), "payload length does not match file length");
	require(crc == checksum(bytes.data() + kHeaderSize, length), "payload checksum mismatch");
	if (version != kJourneyVersion)
		throw XeenUnsupportedSave(version >= 1 && version < kJourneyVersion ? XeenUnsupportedSave::Kind::Older : XeenUnsupportedSave::Kind::Newer);
	XeenSaveSnapshot s;
	s.resources.clouds = in.fingerprint();
	const bool hasDarkside = in.boolean();
	const auto darkside = in.fingerprint();
	if (hasDarkside) s.resources.darkside = darkside;
	else require(darkside.size == 0 && darkside.crc32 == 0, "absent archive has nonzero signature");
	s.camera.mapId = in.map();
	s.camera.x = in.u8(); s.camera.y = in.u8();
	s.camera.direction = static_cast<XeenDirection>(in.u8());
	const auto members = in.u8();
	require(members <= XeenParty::kMaximumVisibleMembers, "too many active members");
	for (unsigned i = 0; i < members; ++i) s.activeRosterIds.push_back(in.u8());
	require(in.u8() == s.characters.size(), "incorrect roster count");
	for (auto &c : s.characters) c = readCharacter(in);
	for (auto &count : s.questItems) count = in.u32();
	for (auto &value : s.questFlags) value = in.boolean();
	for (auto &value : s.gameFlags) value = in.boolean();

	// Fixed Journey bytes: domain/schema/content/context marker 6, context and
	// food 36, supplements 1230, RNG 13, resistances 331, treasure counts 20,
	// world flags 128, learned spells 1201, poison 91, economy 1164, awards
	// 3871, map count 2, head-cache flag 1 and name length 1.
	constexpr std::size_t kFixedJourneyBytes = 8095;
	const auto suffixSize = in.remaining();
	XeenSaveJourney j;
	require(in.u8() == 3, "invalid v4 domain");
	j.schema = in.u16(); j.content = in.u16();
	if (j.schema != kJourneySchema || j.content != kJourneyContent) throw unsupportedPair(j.schema, j.content);
	require(suffixSize >= kFixedJourneyBytes, "Journey schema-9 size mismatch");
	require(in.u8() == 1, "missing Journey context");
	XeenGameplayContext c;
	require(in.u8() == 0, "invalid Journey profile");
	const auto difficulty = in.u8(); require(difficulty <= 1, "invalid Journey difficulty");
	c.difficulty = difficulty == 0 ? XeenDifficulty::Adventurer : XeenDifficulty::Warrior;
	c.ctr24 = in.u16(); c.day = in.u16(); c.year = in.u16(); c.minutes = in.u16();
	for (auto &v : c.effects) v = in.u8();
	for (auto &v : c.lightAndResistances) v = in.u16();
	c.rested = in.boolean(); c.newDay = in.boolean(); j.context = c; s.food=in.u16();
	require(in.u8() == 30, "invalid Journey supplement count");
	for (unsigned i = 0; i < 30; ++i) {
		auto &r = j.supplements[i]; r.owner = in.u8();
		require(r.owner == i, "invalid Journey supplemental owner sequence");
		r.inputs.might.permanent = in.i32(); r.inputs.might.temporary = in.i32();
		r.inputs.speed.permanent = in.i32(); r.inputs.speed.temporary = in.i32();
		r.inputs.accuracy.permanent = in.i32(); r.inputs.accuracy.temporary = in.i32();
		r.inputs.temporaryAc = in.i32(); r.inputs.experience = in.u32();
		r.inputs.luck = XeenAttributeValue{in.i32(),in.i32()};
	}
	 { XeenJourneyRandomState r; r.algorithm=in.u8(); r.state=in.u32(); r.count=in.u64(); j.random=r; }

	require(in.u8()==30,"Invalid resistance count");
	for(unsigned owner=0;owner<30;++owner) {
		require(in.u8()==owner,"Invalid resistance owner order");
		j.supplements[owner].inputs.resistances=XeenCombatResistances{in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8()};
	}
	XeenMonsterTreasure v;v.gold=in.u32();v.gems=in.u32();v.pendingMask=in.u32();v.pendingGold=in.u32();
	const unsigned sourceCount=in.u8();
	require(sourceCount<=kMaximumTreasureSources,"too many monster treasure sources");
	for(unsigned i=0;i<sourceCount;++i) {
		const auto mapId=in.map();
		v.sources[i]={mapId,in.u16()};
	}
	const unsigned weapons=in.u8(),armor=in.u8();
	require(weapons<=10 && armor<=10 && weapons+armor<=12,"Invalid treasure extent");
	for(unsigned category=0;category<2;++category) for(unsigned i=0;i<(category?armor:weapons);++i) {
		auto &r=(category?v.armor:v.weapons)[i];r.source=in.u8();r.item={in.u8(),in.u8(),in.u8(),in.u8()};
		require(r.item.id!=0,"Empty wire treasure");
	}
	const unsigned miscellaneous=in.u8();
	require(miscellaneous<=10 && weapons+armor+miscellaneous<=12,"Invalid treasure extent");
	for(unsigned i=0;i<miscellaneous;++i) {
		auto &r=v.miscellaneous[i];r.source=in.u8();r.item={in.u8(),in.u8(),in.u8(),in.u8()};
		require(r.item.id!=0,"Empty wire treasure");
	}
	j.treasure=v;
	j.worldFlags.emplace();
	for (auto &flag:j.worldFlags->values) flag=in.boolean();

	require(in.u8()==30,"Invalid learned-spell owner count");
	for (unsigned owner=0; owner<30; ++owner) {
		require(in.u8()==owner,"Invalid learned-spell owner order");
		XeenCharacter::XeenLearnedSpells book{};
		for (auto &flag:book) flag=in.u8();
		s.characters[owner].learnedSpells=book;
	}

	require(in.u8()==30,"Invalid poison owner count");
	for (unsigned owner=0; owner<30; ++owner) {
		require(in.u8()==owner,"Invalid poison owner order");
		j.supplements[owner].inputs.poisonResistance=XeenAttributeValue{in.u8(),in.u8()};
	}

	require(in.u8()==2 && in.u8()==4 && in.u8()==4 && in.u8()==9,"Invalid merchant stock shape");
	XeenServiceEconomy economy;
	for (auto &side:economy.wares.records)
		for (auto &shop:side) for (auto &category:shop) for (auto &item:category)
			item={in.u8(),in.u8(),in.u8(),in.u8()};
	economy.bank.gold=in.u32();economy.bank.gems=in.u32();
	j.serviceEconomy=std::move(economy);
	require(in.u8()==30,"invalid award owner count");
	for(unsigned owner=0;owner<30;++owner) {
		require(in.u8()==owner,"invalid award owner sequence");
		for(auto &value:s.characters[owner].awards)value=in.u8();
	}
	s.maps=readMapStates(in);
	unsigned headSize=0;
 if(in.boolean()) {
  s.headCache=XeenHeadCache{in.map(),{}};const unsigned headCount=in.u16();
  require(headCount<=256 && headCount<=in.remaining()/3,"invalid head override count");
  int previous=-1;
  for(unsigned i=0;i<headCount;++i) {const auto cell=in.u8();require(cell>previous,"unordered head overrides");previous=cell;s.headCache->overrides[cell]=XeenHeadIndicators{in.u8(),in.u8()};}
  headSize=5+3*headCount;
 }
 const auto nameLength=in.u8();
	require(nameLength<=20 && nameLength<=in.remaining(), "invalid save name length");
	if(nameLength) {
		s.name.emplace(reinterpret_cast<const char *>(bytes.data()+in.position),nameLength);
		in.position+=nameLength;
		require(validName(*s.name), "invalid save name");
	}
	// The payload extent is computed from the decoded counts.
	std::size_t expected=kFixedJourneyBytes+5*sourceCount+5*(weapons+armor+miscellaneous)+headSize+nameLength;
	for(const auto &m:s.maps)expected+=mapStateBytes(m);
	require(suffixSize==expected,"Invalid Journey suffix length");

	s.journey = std::move(j);

	require(in.remaining() == 0, "trailing payload data");
	validate(s);
	return s;
}

bool XeenSaveFormat::validName(std::string_view name) noexcept {
	return !name.empty() && name.front()!=' ' && name.size()<=20 && std::all_of(name.begin(),name.end(),[](unsigned char c) {
		return c>=0x20 && c<=0x7e;
	});
}

XeenArchiveFingerprint XeenSaveFormat::fingerprint(std::istream &stream) {
	require(stream.good(), "archive stream is not readable");
	std::array<char, 65536> buffer{};
	XeenArchiveFingerprint result;
	uLong crc = crc32(0L, Z_NULL, 0);
	for (;;) {
		try { stream.read(buffer.data(), buffer.size()); }
		catch (const std::ios_base::failure &) {
			if (stream.bad() || !stream.eof()) throw;
		}
		require(!stream.bad() && (!stream.fail() || stream.eof()), "archive read failed");
		const auto count = static_cast<std::uint64_t>(stream.gcount());
		require(count <= std::numeric_limits<std::uint64_t>::max() - result.size,
			"archive size overflow");
		result.size += count;
		crc = crc32(crc, reinterpret_cast<const Bytef *>(buffer.data()), static_cast<uInt>(count));
		if (stream.eof()) break;
	}
	result.crc32 = static_cast<std::uint32_t>(crc);
	return result;
}

} // namespace mmodern
