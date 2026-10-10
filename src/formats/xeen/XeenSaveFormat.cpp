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
	c.maxStatSkills.astrologer = in.boolean(); c.maxStatSkills.bodybuilder = in.boolean();
	c.maxStatSkills.prayerMaster = in.boolean(); c.maxStatSkills.prestidigitation = in.boolean();
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
	validateMap(s.camera.mapId);
	const bool cityCamera =
		s.camera.mapId == XeenMapIdentity(28) && xeenIndoorCoordinate(s.camera.x,s.camera.y);
	require((cityCamera || (s.camera.x >= 0 && s.camera.x <= 15 && s.camera.y >= 0 && s.camera.y <= 15)) &&
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
	require(s.barriers.size()<=1024,"too many barrier cells");
	for(unsigned n=0;n<s.barriers.size();++n) {
		const auto &v=s.barriers[n];
		require(v.tile && v.tile.side==XeenSide::Clouds && v.mask && v.mask<=15,"invalid barrier identity/mask");
		if(n) {const auto &a=s.barriers[n-1];require(a.tile<v.tile || (a.tile==v.tile && a.cell<v.cell),"noncanonical barrier order");}
		for(unsigned d=0;d<4;++d)require((v.mask&(1u<<d))?
			(v.walls[d]==1 || v.walls[d]==3 || v.walls[d]==6 || v.walls[d]==9 || v.walls[d]==13):v.walls[d]==0,"invalid barrier wall");
	}

	const auto &j = *s.journey;
	require(s.barriers.empty() || j.vertigoActors.has_value(),"barrier overrides require retained city actors");
	require(j.entry == XeenEncounterEntry::Journey,
		"unsupported Journey domain/schema/content");
	require(j.serviceEconomy.has_value(), "Journey economy presence mismatch");
	if (j.serviceEconomy) xeenValidateCurrentServiceEconomy(*j.serviceEconomy);
	require(j.regionalRecovery.has_value(), "Journey recovery presence mismatch");
	require(j.context.has_value(), "missing Journey context");
	// Stock may be depleted on any canonical day, including after year rollover.
	// All dates use the current stock validator, including depletion.
	require(j.context->profile == XeenBehaviorProfile::WorldOfXeenClouds &&
		(j.context->difficulty == XeenDifficulty::Adventurer || j.context->difficulty == XeenDifficulty::Warrior),
		"invalid Journey context enum");
	{
		require(xeenRegionalContext(*j.context), "invalid Journey calendar context");
		require(s.camera.mapId != XeenMapIdentity(28) || cityCamera, "camera outside Ironworks city domain");
		require(j.vertigoActors || s.camera.mapId == XeenMapIdentity(23), "absent city requires mainland camera");
	}
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
	if(j.treasure) {
		xeenValidateMonsterTreasure(*j.treasure);
		auto sources = j.treasure->pendingMask;
		for (const auto &entries : {j.treasure->weapons, j.treasure->armor})
			for (const auto &entry : entries) if (entry.item.id) sources |= 1u << entry.source;
		for(unsigned i=0;i<12;++i) if(sources & (1u<<i)) {
			require(i<j.actors.size(),"Monster treasure source missing");
			const auto &a=j.actors[i];
			require(a.id==XeenMonsterIdentity{23,i} && a.accounted && a.lifecycle==XeenActorLifecycle::Defeated &&
				a.status==XeenActorStatus::Physical && !a.hp && !a.activated && a.x==-128 && a.y==-128,"Noncanonical monster treasure source");
		}
	}
	validateMap(j.initializedMap);
	require(j.originalActorCount >= 1 && j.originalActorCount <= 107 &&
		j.actors.size() == j.originalActorCount, "invalid Journey actor counts");
	for (std::size_t i = 0; i < j.actors.size(); ++i) {
		const auto &a = j.actors[i];
		require(j.initializedMap==XeenMapIdentity(23) && a.id==XeenMonsterIdentity{23,i}, "invalid regional actor identity/count");
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
	require(s.camera.mapId != XeenMapIdentity(28) || j.vertigoActors.has_value(), "city camera requires city actors");

	for (const auto &id:s.disabledObjects) require(id.mapId!=XeenMapIdentity(28), "city object overlay is unsupported");
	for (const auto &id:s.disabledEvents) require(id.mapId!=XeenMapIdentity(28) ||
		j.vertigoActors.has_value(), "city Event overlay requires retained city actors");

	if (j.vertigoActors) {
		require(j.vertigoActors->size() <= 107, "invalid city actor count");
		require(j.cityOriginalActorCount<=j.vertigoActors->size(),"city save loses original actor records");
		for (std::size_t i=0; i<j.vertigoActors->size(); ++i) {
			const auto &a=(*j.vertigoActors)[i];
			if(i<j.cityOriginalActorCount) require(a.spawnedType==-1,"Original city type must bind to MOB");
			else require(a.spawnedType==(a.lifecycle==XeenActorLifecycle::Unresolved?-1:0),"Invalid script-slot MON type");
			require(a.id == XeenMonsterIdentity{28,i} && a.x >= -128 && a.x <= 31 && a.y >= -128 && a.y <= 31 &&
				a.hp >= 0 && a.hp <= 65535 && static_cast<unsigned>(a.lifecycle) <= 3 &&
				a.status == XeenActorStatus::Physical, "invalid city actor wire");
			if(a.lifecycle==XeenActorLifecycle::Present)
				require(a.hp>0 && !a.accounted && xeenIndoorCoordinate(a.x,a.y),"invalid live city actor");
			else if(a.lifecycle==XeenActorLifecycle::Defeated)
				require(!a.hp && !a.activated && a.accounted && a.x==-128 && a.y==-128,"invalid defeated city actor");
			else if(a.lifecycle==XeenActorLifecycle::Unresolved)
				require(!a.hp && !a.activated && !a.accounted && !a.x && !a.y,"invalid unresolved city slot");
			else require(!a.accounted && !a.activated,"invalid disabled city actor");
		}
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
	writeIdentities(out, s.disabledObjects);
	writeIdentities(out, s.disabledEvents);
	out.u16(static_cast<std::uint16_t>(s.barriers.size()));
	for(const auto &v:s.barriers) {
		out.map(v.tile);out.u8(v.cell);out.u8(v.mask);out.u16(v.originalWord);out.u8(v.originalAttributes);
		for(auto wall:v.walls)out.u8(wall);out.u8(v.unlocked);
	}

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
	out.u8(0); out.u16(j.initializedMap.number);
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

	out.u8(30);
	for(const auto &r:j.supplements) {
		const auto &v=*r.inputs.resistances;out.u8(r.owner);
		out.u8(v.coldPermanent);out.u8(v.coldTemporary);out.u8(v.electricalPermanent);out.u8(v.electricalTemporary);
		out.u8(v.firePermanent);out.u8(v.fireTemporary);out.u8(v.energyPermanent);out.u8(v.energyTemporary);out.u8(v.magicPermanent);out.u8(v.magicTemporary);
	}
	const auto &v=*j.treasure;out.u32(v.gold);out.u32(v.gems);out.u32(v.pendingMask);out.u32(v.pendingGold);
	unsigned weapons=0,armor=0;for(const auto &r:v.weapons) weapons+=r.item.id!=0;for(const auto &r:v.armor) armor+=r.item.id!=0;
	out.u8(weapons);out.u8(armor);
	for(unsigned category=0;category<2;++category) for(const auto &r:category?v.armor:v.weapons) if(r.item.id) {
		out.u8(r.source);out.u8(r.item.material);out.u8(r.item.id);out.u8(r.item.state);out.u8(r.item.frame);
	}
	out.u8(j.regionalRecovery->worldFlag16);

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
	out.u8(j.vertigoActors.has_value());
	if (j.vertigoActors) {
		out.u16(j.cityOriginalActorCount);out.u16(static_cast<std::uint16_t>(j.vertigoActors->size()));
		for (const auto &a:*j.vertigoActors) {
			out.u8(0);out.u16(28);out.u32(static_cast<std::uint32_t>(a.id.recordIndex));
			out.i16(static_cast<std::int16_t>(a.x));out.i16(static_cast<std::int16_t>(a.y));out.i32(a.hp);
			out.u8(a.activated);out.u8(static_cast<std::uint8_t>(a.lifecycle));out.u8(static_cast<std::uint8_t>(a.status));out.u8(a.accounted);
			out.i16(a.spawnedType);
		}
	}

	out.u8(2);out.u8(4);out.u8(4);out.u8(9);
	for (const auto &side:j.serviceEconomy->wares.records)
		for (const auto &shop:side) for (const auto &category:shop) for (const auto &item:category) {
			out.u8(item.material);out.u8(item.id);out.u8(item.state);out.u8(item.frame);
		}
	out.u32(j.serviceEconomy->bank.gold);out.u32(j.serviceEconomy->bank.gems);
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
	s.disabledObjects = readIdentities<XeenObjectIdentity>(in, kMaximumObjects);
	s.disabledEvents = readIdentities<XeenEventIdentity>(in, kMaximumEvents);
	const auto barrierCount=in.u16();require(barrierCount<=1024,"too many barrier cells");
	for(unsigned n=0;n<barrierCount;++n) {
		XeenBarrierOverride v;v.tile=in.map();v.cell=in.u8();v.mask=in.u8();v.originalWord=in.u16();v.originalAttributes=in.u8();
		for(auto &wall:v.walls)wall=in.u8();v.unlocked=in.boolean();s.barriers.push_back(v);
	}

	const auto suffixSize = in.remaining();
	XeenSaveJourney j;
	require(in.u8() == 3, "invalid v4 domain");
	j.schema = in.u16(); j.content = in.u16();
	if (j.schema != kJourneySchema || j.content != kJourneyContent) throw unsupportedPair(j.schema, j.content);
	// Bound allocation before parsing; the exact dynamic extent is verified
	// after the actor and treasure counts are decoded.
	require(suffixSize >= 4099+19+1 && suffixSize <= 4099+19*107+4+21*107+5*12+21, "Journey schema-9 size mismatch");
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
	require(in.u8() == 0, "invalid Journey map side"); j.initializedMap = {XeenSide::Clouds, in.u16()};
	j.originalActorCount = in.u16();
	const auto count = in.u16();
	require(count >= 1 && count <= 107 && count <= in.remaining()/19 && count == j.originalActorCount, "invalid Journey live record count");
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

	require(in.u8()==30,"Invalid resistance count");
	for(unsigned owner=0;owner<30;++owner) {
		require(in.u8()==owner,"Invalid resistance owner order");
		j.supplements[owner].inputs.resistances=XeenCombatResistances{in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8(),in.u8()};
	}
	XeenMonsterTreasure v;v.gold=in.u32();v.gems=in.u32();v.pendingMask=in.u32();v.pendingGold=in.u32();
	const unsigned weapons=in.u8(),armor=in.u8();
	require(weapons<=10 && armor<=10 && weapons+armor<=12,"Invalid treasure extent");
	for(unsigned category=0;category<2;++category) for(unsigned i=0;i<(category?armor:weapons);++i) {
		auto &r=(category?v.armor:v.weapons)[i];r.source=in.u8();r.item={in.u8(),in.u8(),in.u8(),in.u8()};
		require(r.item.id!=0,"Empty wire treasure");
	}
	j.treasure=v;
	j.regionalRecovery=XeenRegionalRecoveryState{in.boolean()};

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
	if (in.boolean()) {
		j.cityOriginalActorCount=in.u16();
		const unsigned cityCount=in.u16();require(cityCount<=107 && cityCount>=j.cityOriginalActorCount,"Invalid city runtime count");
		std::vector<XeenSaveJourneyActor> city;city.reserve(cityCount);
		for (unsigned i=0;i<cityCount;++i) {
			XeenSaveJourneyActor a;
			require(in.u8()==0 && in.u16()==28 && in.u32()==i,"Invalid city actor identity");
			a.id={28,i};a.x=in.i16();a.y=in.i16();a.hp=in.i32();a.activated=in.boolean();
			const auto life=in.u8(),status=in.u8();
			require(life<=3 && status==0,"Invalid city lifecycle/status");
			a.lifecycle=static_cast<XeenActorLifecycle>(life);a.status=XeenActorStatus::Physical;a.accounted=in.boolean();
			a.spawnedType=in.i16();city.push_back(a);
		}
		j.vertigoActors=std::move(city);
	}
	const unsigned n=weapons+armor;
	const unsigned expected=2753+19*count+1164+182+(j.vertigoActors ? 4+21*j.vertigoActors->size() : 0);

	require(in.u8()==2 && in.u8()==4 && in.u8()==4 && in.u8()==9,"Invalid merchant stock shape");
	XeenServiceEconomy economy;
	for (auto &side:economy.wares.records)
		for (auto &shop:side) for (auto &category:shop) for (auto &item:category)
			item={in.u8(),in.u8(),in.u8(),in.u8()};
	economy.bank.gold=in.u32();economy.bank.gems=in.u32();
	j.serviceEconomy=std::move(economy);
	const auto nameLength=in.u8();
	require(nameLength<=20 && nameLength<=in.remaining(), "invalid save name length");
	if(nameLength) {
		s.name.emplace(reinterpret_cast<const char *>(bytes.data()+in.position),nameLength);
		in.position+=nameLength;
		require(validName(*s.name), "invalid save name");
	}
	require(suffixSize==expected+5u*n+1+nameLength,"Invalid city/economy suffix length");

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
