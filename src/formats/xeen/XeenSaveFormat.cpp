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

void validateMap(XeenMapIdentity id) {
	require(id.side == XeenSide::Clouds && id.number >= 1 && id.number <= 9999,
		"invalid Clouds map identity");
}

template<class Identity>
void validateIdentities(const std::vector<Identity> &ids, std::size_t limit) {
	require(ids.size() <= limit, "too many disabled identities");
	for (std::size_t i = 0; i < ids.size(); ++i) {
		validateMap(ids[i].mapId);
		require(ids[i].recordIndex <= std::numeric_limits<std::uint32_t>::max(),
			"original record index exceeds save representation");
		require(i == 0 || ids[i - 1] < ids[i], "disabled identities must be strictly ordered");
	}
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
	out.u8(c.maxStatSkills.astrologer); out.u8(c.maxStatSkills.bodybuilder);
	out.u8(c.maxStatSkills.prayerMaster); out.u8(c.maxStatSkills.prestidigitation);
	out.u8(c.hasSpells);
	for (const auto *items : {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous})
		for (const auto item : *items) {
			out.u8(item.material); out.u8(item.id); out.u8(item.state); out.u8(item.frame);
		}
	out.i16(c.currentHp); out.i16(c.currentSp);
	for (const auto value : c.conditions) out.u8(value);
	out.u16(c.birthYear);
}

XeenCharacter readCharacter(Reader &in, std::uint16_t version) {
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
	c.maxStatSkills.astrologer = in.boolean(); c.maxStatSkills.bodybuilder = in.boolean();
	c.maxStatSkills.prayerMaster = in.boolean(); c.maxStatSkills.prestidigitation = in.boolean();
	c.hasSpells = in.boolean();
	for (auto *items : {&c.weapons, &c.armor, &c.accessories})
		for (auto &item : *items) {
			item.material = in.u8();
			if (version >= 2) item.id = in.u8();
			item.state = in.u8(); item.frame = in.u8();
		}
	if (version >= 2)
		for (auto &item : c.miscellaneous) {
			item.material = in.u8(); item.id = in.u8();
			item.state = in.u8(); item.frame = in.u8();
		}
	c.currentHp = in.i16(); c.currentSp = in.i16();
	for (auto &value : c.conditions) value = in.u8();
	c.birthYear = in.u16();
	return c;
}

template<class Identity>
void writeIdentities(Writer &out, const std::vector<Identity> &ids) {
	out.u32(static_cast<std::uint32_t>(ids.size()));
	for (const auto id : ids) {
		out.map(id.mapId); out.u32(static_cast<std::uint32_t>(id.recordIndex));
	}
}

template<class Identity>
std::vector<Identity> readIdentities(Reader &in, std::size_t limit) {
	const auto count = in.u32();
	require(count <= limit && count <= in.remaining() / 7, "invalid disabled-identity count");
	std::vector<Identity> ids;
	ids.reserve(count);
	for (std::uint32_t i = 0; i < count; ++i) {
		const auto mapId = in.map();
		const auto index = in.u32();
		require(index <= std::numeric_limits<std::size_t>::max(), "record index cannot be represented");
		ids.push_back({mapId, static_cast<std::size_t>(index)});
	}
	return ids;
}

} // namespace

void XeenSaveFormat::validate(const XeenSaveSnapshot &s) {
	require(!(s.completedEncounter && s.journey), "mutually exclusive save domains");
	validateMap(s.camera.mapId);
	require(s.camera.x >= 0 && s.camera.x <= 15 && s.camera.y >= 0 && s.camera.y <= 15 &&
		static_cast<unsigned>(s.camera.direction) <= 3, "invalid committed camera");
	require(s.activeRosterIds.size() <= XeenParty::kMaximumVisibleMembers, "too many active members");
	for (const auto id : s.activeRosterIds)
		require(id < XeenRoster::kCharacterCount, "invalid active roster identity");
	for (std::size_t i = 0; i < s.characters.size(); ++i) {
		const auto &c = s.characters[i];
		require(c.rosterId == i, "character identity differs from roster slot");
		require(c.name.size() <= 16 && c.name.find('\0') == std::string::npos, "invalid character name");
	}
	validateIdentities(s.disabledObjects, kMaximumObjects);
	validateIdentities(s.disabledEvents, kMaximumEvents);
	if (s.journey) {
		const auto &j = *s.journey;
		require(s.itemState == XeenSaveItemState::Complete, "Journey requires complete item fields");
		require(j.entry == XeenEncounterEntry::Journey && j.schema == 1 && j.contract == 1,
			"unsupported Journey domain/schema/contract");
		require(j.context.has_value(), "missing Journey context");
		require(j.context->profile == XeenBehaviorProfile::WorldOfXeenClouds &&
			(j.context->difficulty == XeenDifficulty::Adventurer || j.context->difficulty == XeenDifficulty::Warrior),
			"invalid Journey context enum");
		for (std::size_t i = 0; i < j.supplements.size(); ++i) {
			const auto &r = j.supplements[i];
			require(r.owner == i, "invalid Journey supplemental owner sequence");
			for (int v : {r.inputs.might.permanent, r.inputs.might.temporary, r.inputs.speed.permanent,
				r.inputs.speed.temporary, r.inputs.accuracy.permanent, r.inputs.accuracy.temporary, r.inputs.temporaryAc})
				require(v >= 0 && v <= 255, "Journey supplement outside byte range");
		}
		require(j.skeletonSeed != 0, "zero Journey seed");
		validateMap(j.initializedMap);
		require(j.originalActorCount >= 1 && j.originalActorCount <= 107 &&
			j.actors.size() == 1, "invalid Journey actor counts");
		for (std::size_t i = 0; i < j.actors.size(); ++i) {
			const auto &a = j.actors[i];
			validateMap(a.id.mapId);
			require(a.id.recordIndex <= std::numeric_limits<std::uint32_t>::max() &&
				(i == 0 || j.actors[i-1].id < a.id), "invalid Journey identity order/range");
			require(a.x >= -128 && a.x <= 31 && a.y >= -128 && a.y <= 31 && a.hp >= 0 && a.hp <= 65535,
				"Journey live value outside wire bounds");
			require(a.lifecycle == XeenActorLifecycle::Present || a.lifecycle == XeenActorLifecycle::Disabled ||
				a.lifecycle == XeenActorLifecycle::Unresolved || a.lifecycle == XeenActorLifecycle::Defeated,
				"invalid Journey lifecycle");
			require(a.status == XeenActorStatus::Physical || a.status == XeenActorStatus::Unsupported, "invalid Journey status");
		}
	}
	if (s.completedEncounter) {
		const auto &e = *s.completedEncounter;
		require(s.itemState == XeenSaveItemState::Complete, "completed encounter requires complete item fields");
		require(e.entry == XeenEncounterEntry::Diagnostic27 && e.victory && e.accountingConsumed,
			"invalid completed encounter discriminator");
		require(e.monster == XeenMonsterIdentity{{XeenSide::Clouds, 20}, 5},
			"invalid completed monster identity");
		require(s.camera.mapId == XeenMapIdentity{XeenSide::Clouds, 20} && s.camera.x >= 13 && s.camera.x <= 14 &&
			s.camera.y >= 1 && s.camera.y <= 2, "completed encounter camera is outside the admitted envelope");
		require(s.activeRosterIds == std::vector<std::uint8_t>(kXeenCombatOwners.begin(), kXeenCombatOwners.end()),
			"completed encounter membership differs from Diagnostic27");
		const auto &c = e.context;
		require(c.profile == XeenBehaviorProfile::WorldOfXeenClouds && c.difficulty == XeenDifficulty::Adventurer &&
			c.ctr24 < 24 && c.day == 1 && c.year == 610 && c.minutes >= 491 && c.minutes <= 959 &&
			c.effects == std::array<std::uint8_t, 9>{} && c.lightAndResistances == std::array<std::uint16_t, 6>{} &&
			!c.rested && !c.newDay, "invalid completed encounter context");
		constexpr std::array<std::uint8_t, 6> owners{0, 1, 6, 11, 14, 18};
		for (std::size_t i = 0; i < owners.size(); ++i) {
			const auto &record = e.supplements[i];
			require(record.owner == owners[i], "invalid completed supplemental owner sequence");
			for (const int value : {record.inputs.might.permanent, record.inputs.might.temporary,
				record.inputs.speed.permanent, record.inputs.speed.temporary,
				record.inputs.accuracy.permanent, record.inputs.accuracy.temporary, record.inputs.temporaryAc})
				require(value >= 0 && value <= 255, "completed supplemental input outside byte-origin range");
		}
	}
}

std::vector<std::uint8_t> XeenSaveFormat::encode(const XeenSaveSnapshot &s) {
	validate(s);
	require(s.itemState == XeenSaveItemState::Complete, "unresolved legacy item state cannot be encoded");
	const auto version = s.journey ? kJourneyVersion : s.completedEncounter ? kCompletedVersion : kOrdinaryVersion;
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
	writeIdentities(out, s.disabledObjects);
	writeIdentities(out, s.disabledEvents);
	if (s.completedEncounter) {
		const auto &e = *s.completedEncounter;
		out.u8(1);out.u8(2);out.u8(1);out.u8(1);
		out.map(e.monster.mapId);out.u32(static_cast<std::uint32_t>(e.monster.recordIndex));
		out.u8(static_cast<std::uint8_t>(e.context.profile));
		out.u8(static_cast<std::uint8_t>(e.context.difficulty));
		out.u16(e.context.ctr24);out.u16(e.context.day);out.u16(e.context.year);out.u16(e.context.minutes);
		for(const auto value:e.context.effects)out.u8(value);
		for(const auto value:e.context.lightAndResistances)out.u16(value);
		out.u8(e.context.rested);out.u8(e.context.newDay);out.u8(6);
		for(const auto &record:e.supplements) {
			out.u8(record.owner);
			for(const int value:{record.inputs.might.permanent,record.inputs.might.temporary,
				record.inputs.speed.permanent,record.inputs.speed.temporary,
				record.inputs.accuracy.permanent,record.inputs.accuracy.temporary,record.inputs.temporaryAc})out.i32(value);
			out.u32(record.inputs.experience);
		}
	}
	if (s.journey) {
		const auto &j = *s.journey;
		out.u8(3); out.u16(j.schema); out.u16(j.contract); out.u8(1);
		const auto &c = *j.context;
		out.u8(0); out.u8(c.difficulty == XeenDifficulty::Adventurer ? 0 : 1);
		out.u16(c.ctr24); out.u16(c.day); out.u16(c.year); out.u16(c.minutes);
		for (auto v : c.effects) out.u8(v);
		for (auto v : c.lightAndResistances) out.u16(v);
		out.u8(c.rested); out.u8(c.newDay); out.u8(30);
		for (const auto &r : j.supplements) {
			out.u8(r.owner);
			for (int v : {r.inputs.might.permanent, r.inputs.might.temporary, r.inputs.speed.permanent,
				r.inputs.speed.temporary, r.inputs.accuracy.permanent, r.inputs.accuracy.temporary, r.inputs.temporaryAc}) out.i32(v);
			out.u32(r.inputs.experience);
		}
		out.u32(j.skeletonSeed); out.u8(0); out.u16(j.initializedMap.number);
		out.u16(j.originalActorCount); out.u16(static_cast<std::uint16_t>(j.actors.size()));
		for (const auto &a : j.actors) {
			out.u8(0); out.u16(a.id.mapId.number); out.u32(static_cast<std::uint32_t>(a.id.recordIndex));
			out.i16(static_cast<std::int16_t>(a.x)); out.i16(static_cast<std::int16_t>(a.y)); out.i32(a.hp);
			out.u8(a.activated);
			switch (a.lifecycle) {
			case XeenActorLifecycle::Present: out.u8(0); break;
			case XeenActorLifecycle::Disabled: out.u8(1); break;
			case XeenActorLifecycle::Unresolved: out.u8(2); break;
			case XeenActorLifecycle::Defeated: out.u8(3); break;
			}
			out.u8(a.status == XeenActorStatus::Physical ? 0 : 1); out.u8(a.accounted);
		}
	}
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
	require(version == 1 || version == 2 || version == 3 || version == 4, "unsupported version");
	require(in.u8() == 0, "unsupported game side");
	require(in.u8() == 0, "nonzero reserved byte");
	const auto length = in.u32();
	const auto crc = in.u32();
	require(length == in.remaining(), "payload length does not match file length");
	require(crc == checksum(bytes.data() + kHeaderSize, length), "payload checksum mismatch");
	XeenSaveSnapshot s;
	s.itemState = version == 1 ? XeenSaveItemState::LegacyV1MissingFields : XeenSaveItemState::Complete;
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
	for (auto &c : s.characters) c = readCharacter(in, version);
	for (auto &count : s.questItems) count = in.u32();
	for (auto &value : s.questFlags) value = in.boolean();
	for (auto &value : s.gameFlags) value = in.boolean();
	s.disabledObjects = readIdentities<XeenObjectIdentity>(in, kMaximumObjects);
	s.disabledEvents = readIdentities<XeenEventIdentity>(in, kMaximumEvents);
	if(version==3) {
		XeenSaveCompletedEncounter e;
		require(in.remaining()==243,"invalid v3 extension size");
		require(in.u8()==1,"v3 extension is absent");
		require(in.u8()==2,"invalid v3 entry kind");e.entry=XeenEncounterEntry::Diagnostic27;
		require(in.u8()==1,"invalid v3 completion");e.victory=true;
		require(in.u8()==1,"invalid v3 accounting state");e.accountingConsumed=true;
		e.monster.mapId=in.map();const auto record=in.u32();
		require(record<=std::numeric_limits<std::size_t>::max(),"monster identity cannot be represented");
		e.monster.recordIndex=static_cast<std::size_t>(record);
		e.context.profile=static_cast<XeenBehaviorProfile>(in.u8());
		e.context.difficulty=static_cast<XeenDifficulty>(in.u8());
		e.context.ctr24=in.u16();e.context.day=in.u16();e.context.year=in.u16();e.context.minutes=in.u16();
		for(auto &value:e.context.effects)value=in.u8();
		for(auto &value:e.context.lightAndResistances)value=in.u16();
		e.context.rested=in.boolean();e.context.newDay=in.boolean();
		require(in.u8()==6,"invalid v3 supplemental owner count");
		for(auto &supplement:e.supplements) {
			supplement.owner=in.u8();
			supplement.inputs.might.permanent=in.i32();supplement.inputs.might.temporary=in.i32();
			supplement.inputs.speed.permanent=in.i32();supplement.inputs.speed.temporary=in.i32();
			supplement.inputs.accuracy.permanent=in.i32();supplement.inputs.accuracy.temporary=in.i32();
			supplement.inputs.temporaryAc=in.i32();supplement.inputs.experience=in.u32();
		}
		s.completedEncounter=std::move(e);
	}
	if (version == 4) {
		require(in.remaining() == 1060, "invalid v4 extension size");
		XeenSaveJourney j;
		require(in.u8() == 3, "invalid v4 domain");
		j.schema = in.u16(); j.contract = in.u16();
		require(j.schema == 1 && j.contract == 1, "unsupported Journey schema/contract");
		require(in.u8() == 1, "missing Journey context");
		XeenGameplayContext c;
		require(in.u8() == 0, "invalid Journey profile");
		const auto difficulty = in.u8(); require(difficulty <= 1, "invalid Journey difficulty");
		c.difficulty = difficulty == 0 ? XeenDifficulty::Adventurer : XeenDifficulty::Warrior;
		c.ctr24 = in.u16(); c.day = in.u16(); c.year = in.u16(); c.minutes = in.u16();
		for (auto &v : c.effects) v = in.u8();
		for (auto &v : c.lightAndResistances) v = in.u16();
		c.rested = in.boolean(); c.newDay = in.boolean(); j.context = c;
		require(in.u8() == 30, "invalid Journey supplement count");
		for (unsigned i = 0; i < 30; ++i) {
			auto &r = j.supplements[i]; r.owner = in.u8();
			require(r.owner == i, "invalid Journey supplemental owner sequence");
			r.inputs.might.permanent = in.i32(); r.inputs.might.temporary = in.i32();
			r.inputs.speed.permanent = in.i32(); r.inputs.speed.temporary = in.i32();
			r.inputs.accuracy.permanent = in.i32(); r.inputs.accuracy.temporary = in.i32();
			r.inputs.temporaryAc = in.i32(); r.inputs.experience = in.u32();
		}
		j.skeletonSeed = in.u32();
		require(in.u8() == 0, "invalid Journey map side"); j.initializedMap = {XeenSide::Clouds, in.u16()};
		j.originalActorCount = in.u16();
		const auto count = in.u16();
		require(count >= 1 && count <= 107 && count <= in.remaining()/19 && count == 1, "invalid Journey live record count");
		for (unsigned i = 0; i < count; ++i) {
			XeenSaveJourneyActor a;
			require(in.u8() == 0, "invalid Journey actor side"); a.id.mapId = {XeenSide::Clouds, in.u16()};
			const auto index = in.u32();
			require(index <= std::numeric_limits<std::size_t>::max(), "Journey identity cannot be represented");
			a.id.recordIndex = static_cast<std::size_t>(index);
			a.x = in.i16(); a.y = in.i16(); a.hp = in.i32(); a.activated = in.boolean();
			switch (in.u8()) {
			case 0: a.lifecycle = XeenActorLifecycle::Present; break;
			case 1: a.lifecycle = XeenActorLifecycle::Disabled; break;
			case 2: a.lifecycle = XeenActorLifecycle::Unresolved; break;
			case 3: a.lifecycle = XeenActorLifecycle::Defeated; break;
			default: require(false, "invalid Journey lifecycle");
			}
			const auto status = in.u8(); require(status <= 1, "invalid Journey status");
			a.status = status == 0 ? XeenActorStatus::Physical : XeenActorStatus::Unsupported;
			a.accounted = in.boolean(); j.actors.push_back(a);
		}
		s.journey = std::move(j);
	}
	require(in.remaining() == 0, "trailing payload data");
	validate(s);
	return s;
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
