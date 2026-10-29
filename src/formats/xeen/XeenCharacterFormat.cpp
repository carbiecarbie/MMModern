#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenSaveSnapshot.h"
#include "formats/xeen/XeenGameplayContextFormat.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace mmodern {
namespace {

constexpr std::size_t kNameOffset = 0;
constexpr std::size_t kNameSize = 16;
constexpr std::size_t kSexOffset = 16;
constexpr std::size_t kRaceOffset = 17;
constexpr std::size_t kClassOffset = 19;
constexpr std::size_t kPermanentIntellectOffset = 22;
constexpr std::size_t kTemporaryIntellectOffset = 23;
constexpr std::size_t kPermanentPersonalityOffset = 24;
constexpr std::size_t kTemporaryPersonalityOffset = 25;
constexpr std::size_t kPermanentEnduranceOffset = 26;
constexpr std::size_t kTemporaryEnduranceOffset = 27;
constexpr std::size_t kPermanentLevelOffset = 35;
constexpr std::size_t kTemporaryLevelOffset = 36;
constexpr std::size_t kTemporaryAgeOffset = 38;
constexpr std::size_t kHasSpellsOffset = 163;
constexpr std::size_t kLearnedSpellsOffset = 121;
constexpr std::size_t kWeaponsOffset = 166;
constexpr std::size_t kArmorOffset = 202;
constexpr std::size_t kAccessoriesOffset = 238;
constexpr std::size_t kMiscellaneousOffset = 274;
constexpr std::size_t kSerializedItemSize = 4;
constexpr std::size_t kItemMaterialOffset = 0;
constexpr std::size_t kItemIdOffset = 1;
constexpr std::size_t kItemStateOffset = 2;
constexpr std::size_t kItemFrameOffset = 3;
constexpr std::size_t kConditionsOffset = 323;
constexpr std::size_t kCurrentHpOffset = 342;
constexpr std::size_t kCurrentSpOffset = 344;
constexpr std::size_t kBirthYearOffset = 346;

std::int16_t readSint16LE(const std::uint8_t *data) {
	const auto value = static_cast<std::uint16_t>(data[0]) |
		(static_cast<std::uint16_t>(data[1]) << 8);
	return static_cast<std::int16_t>(value);
}

std::uint16_t readUint16LE(const std::uint8_t *data) {
	return static_cast<std::uint16_t>(data[0]) |
		(static_cast<std::uint16_t>(data[1]) << 8);
}

template<std::size_t N>
void readItems(const std::uint8_t *record, std::size_t blockOffset,
		std::array<XeenItem, N> &items) {
	for (std::size_t i = 0; i < items.size(); ++i) {
		const std::uint8_t *item = record + blockOffset + i * kSerializedItemSize;
		items[i].material = item[kItemMaterialOffset];
		items[i].id = item[kItemIdOffset];
		items[i].state = item[kItemStateOffset];
		items[i].frame = item[kItemFrameOffset];
	}
}

std::string readBoundedName(const std::uint8_t *data) {
	std::size_t length = 0;
	while (length < kNameSize && data[length] != 0)
		++length;
	std::string result;
	result.reserve(length);
	for (std::size_t i = 0; i < length; ++i)
		result.push_back(static_cast<char>(data[i]));
	return result;
}

} // namespace

XeenCombatInputs XeenCharacterFormat::parseCombatInputs(const std::vector<std::uint8_t> &bytes, std::size_t owner,
		bool includeLuck, bool includeResistances, bool includePoison) {
	if (bytes.size() != XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize || owner >= XeenRoster::kCharacterCount)
		throw std::invalid_argument("combat CHR requires exactly thirty complete 354-byte records and a valid owner");
	const auto *p = bytes.data() + owner * XeenCharacter::kSerializedSize;
	XeenCombatInputs result{{p[20],p[21]}, {p[28],p[29]}, {p[30],p[31]}, p[34],
		std::uint32_t(p[348]) | (std::uint32_t(p[349])<<8) | (std::uint32_t(p[350])<<16) | (std::uint32_t(p[351])<<24)};
	if (includeLuck) result.luck = XeenAttributeValue{p[32],p[33]};
	if (includeResistances) result.resistances = XeenCombatResistances{p[313],p[314],p[315],p[316],p[311],p[312],p[319],p[320],p[321],p[322]};
	if (includePoison) result.poisonResistance = XeenAttributeValue{p[317],p[318]};
	return result;
}

XeenCharacter::XeenLearnedSpells XeenCharacterFormat::parseLearnedSpells(
		const std::vector<std::uint8_t> &bytes, std::size_t owner) {
	if (bytes.size() != XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize ||
			owner >= XeenRoster::kCharacterCount)
		throw std::invalid_argument("learned CHR requires thirty complete records and a valid owner");
	XeenCharacter::XeenLearnedSpells flags{};
	const auto *record = bytes.data() + owner * XeenCharacter::kSerializedSize;
	std::copy_n(record + kLearnedSpellsOffset, flags.size(), flags.begin());
	return flags;
}

XeenPurse XeenCharacterFormat::parseMonsterPurse(const std::vector<std::uint8_t> &bytes) {
	if (bytes.size() < 646) throw std::invalid_argument("maze.pty truncated before carried purse");
	const auto u32 = [&](std::size_t offset) {
		return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset+1]) << 8) |
			(std::uint32_t(bytes[offset+2]) << 16) | (std::uint32_t(bytes[offset+3]) << 24);
	};
	XeenPurse result;
	result.gold = u32(638); result.gems = u32(642);
	return result;
}

XeenBankBalances XeenCharacterFormat::parseBankBalances(const std::vector<std::uint8_t> &bytes) {
	// Clouds Party::synchronize layout: fixed record extent, with two LE32 bank fields.
	constexpr std::size_t kCloudsPartyBytes=812, kBankGold=646, kBankGems=650;
	static_assert(kBankGems+sizeof(std::uint32_t)<=kCloudsPartyBytes);
	if (bytes.size()!=kCloudsPartyBytes)
		throw std::invalid_argument("Invalid Clouds maze.pty field layout");
	const auto u32 = [&](std::size_t offset) {
		return std::uint32_t(bytes[offset]) | (std::uint32_t(bytes[offset+1])<<8) |
			(std::uint32_t(bytes[offset+2])<<16) | (std::uint32_t(bytes[offset+3])<<24);
	};
	XeenBankBalances bank{u32(kBankGold),u32(kBankGems)};
	return bank;
}

XeenRoster XeenCharacterFormat::parseRoster(const std::vector<std::uint8_t> &bytes) {
	const std::size_t expectedSize = XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize;
	if (bytes.size() != expectedSize) {
		throw std::runtime_error("maze.chr has " + std::to_string(bytes.size()) +
			" bytes; expected: " + std::to_string(expectedSize));
	}

	XeenRoster roster;
	for (std::size_t rosterId = 0; rosterId < XeenRoster::kCharacterCount; ++rosterId) {
		const std::uint8_t *record = bytes.data() + rosterId * XeenCharacter::kSerializedSize;
		XeenCharacter &character = roster.at(rosterId);
		character.rosterId = static_cast<std::uint8_t>(rosterId);
		character.name = readBoundedName(record + kNameOffset);
		character.sex = static_cast<XeenSex>(record[kSexOffset]);
		character.race = static_cast<XeenRace>(record[kRaceOffset]);
		character.characterClass = static_cast<XeenCharacterClass>(record[kClassOffset]);
		character.intellect.permanent = record[kPermanentIntellectOffset];
		character.intellect.temporary = record[kTemporaryIntellectOffset];
		character.personality.permanent = record[kPermanentPersonalityOffset];
		character.personality.temporary = record[kTemporaryPersonalityOffset];
		character.endurance.permanent = record[kPermanentEnduranceOffset];
		character.endurance.temporary = record[kTemporaryEnduranceOffset];
		character.permanentLevel = record[kPermanentLevelOffset];
		character.temporaryLevel = record[kTemporaryLevelOffset];
		character.temporaryAge = record[kTemporaryAgeOffset];
		character.hasSpells = record[kHasSpellsOffset] != 0;
		readItems(record, kWeaponsOffset, character.weapons);
		readItems(record, kArmorOffset, character.armor);
		readItems(record, kAccessoriesOffset, character.accessories);
		readItems(record, kMiscellaneousOffset, character.miscellaneous);
		std::copy_n(record + kConditionsOffset, character.conditions.size(),
			character.conditions.begin());
		character.currentHp = readSint16LE(record + kCurrentHpOffset);
		character.currentSp = readSint16LE(record + kCurrentSpOffset);
		character.birthYear = readUint16LE(record + kBirthYearOffset);
		std::copy_n(record + 39, character.skills.size(), character.skills.begin());
		for (unsigned i = 0; i < 64; ++i) {
			const auto award = record[57+i];
			character.awards[i] = i == 9 ? award : award & 15;
			character.awards[i+64] = i == 9 ? 0 : award >> 4;
		}
		character.birthDay = record[37];
		character.xeenSide=record[18];character.lloydMap=record[160];
		character.lloydX=record[161];character.lloydY=record[162];character.lloydSide=record[310];
		character.currentSpell=record[164];character.quickOption=record[165];
		character.townUnknown=record[339];character.savedMaze=readUint16LE(record+340);
		character.adventuringSpell=record[352];character.combatSpell=record[353];

	}
	return roster;
}

XeenCharacterFormat::PartyHeader XeenCharacterFormat::parsePartyHeader(
		const std::vector<std::uint8_t> &bytes) {
	constexpr std::size_t kRequiredSize = 2 + XeenParty::kSerializedMemberSlots;
	if (bytes.size() < kRequiredSize)
		throw std::runtime_error("maze.pty truncated before party member list");

	PartyHeader result;
	result.firstCount = bytes[0];
	result.effectiveCount = bytes[1];
	for (std::size_t i = 0; i < result.rosterIds.size(); ++i) {
		const auto value = bytes[2 + i];
		result.rosterIds[i] = value == 0xff ? -1 : value;
	}
	return result;
}

XeenCamera XeenCharacterFormat::parsePartyLocation(const std::vector<std::uint8_t> &bytes) {
	if (bytes.size()<18 || bytes[10]>3 || bytes[11]>=32 || bytes[12]>=32 || !bytes[13])
		throw std::invalid_argument("Invalid or truncated PTY location");
	return {bytes[13],bytes[11],bytes[12],static_cast<XeenDirection>(bytes[10])};
}

void XeenCharacterFormat::writeDosRoster(std::vector<std::uint8_t> &bytes, const XeenSaveSnapshot &s) {
	if (bytes.size() != 30 * XeenCharacter::kSerializedSize || !s.journey)
		throw std::invalid_argument("DOS CHR writer requires thirty records and a Journey");
	for (std::size_t i = 0; i < 30; ++i) {
		auto *p = bytes.data() + i * XeenCharacter::kSerializedSize;
		const auto &c = s.characters[i]; const auto &a = s.journey->supplements[i].inputs;
		const auto word = [&](unsigned off, std::uint16_t v) { p[off]=v; p[off+1]=v>>8; };
		const auto attribute = [&](unsigned off, const XeenAttributeValue &v) {p[off]=v.permanent; p[off+1]=v.temporary;};
		if (c.name.size() > 16 || !a.luck || !a.resistances || !a.poisonResistance || !c.learnedSpells)
			throw std::invalid_argument("Incomplete DOS CHR state");
		for(int value:{c.intellect.permanent,c.intellect.temporary,c.personality.permanent,c.personality.temporary,
			c.endurance.permanent,c.endurance.temporary,c.permanentLevel,c.temporaryLevel,c.temporaryAge})
			if(value<0 || value>255) throw std::invalid_argument("DOS CHR byte overflow at roster "+std::to_string(i));
		if(c.awards[73]) throw std::invalid_argument("DOS CHR has no independent award slot 73");
		std::copy(c.name.begin(), c.name.end(), p);
		if (c.name.size() < 16) p[c.name.size()] = 0; // Remaining padding keeps New Game bytes.
		p[16]=static_cast<std::uint8_t>(c.sex); p[17]=static_cast<std::uint8_t>(c.race);
		p[18]=c.xeenSide; p[19]=static_cast<std::uint8_t>(c.characterClass);
		attribute(20,a.might); attribute(22,c.intellect); attribute(24,c.personality); attribute(26,c.endurance);
		attribute(28,a.speed); attribute(30,a.accuracy); attribute(32,*a.luck);
		p[34]=a.temporaryAc; p[35]=c.permanentLevel; p[36]=c.temporaryLevel; p[37]=c.birthDay; p[38]=c.temporaryAge;
		std::copy(c.skills.begin(),c.skills.end(),p+39);
		for(unsigned j=0;j<64;++j) p[57+j]=j==9 ? c.awards[j] : c.awards[j] | (c.awards[j+64]<<4);
		std::copy(c.learnedSpells->begin(),c.learnedSpells->end(),p+121);
		p[160]=c.lloydMap; p[161]=c.lloydX; p[162]=c.lloydY; p[163]=c.hasSpells;
		p[164]=c.currentSpell; p[165]=c.quickOption;
		unsigned off=166;
		for(const auto *category : {&c.weapons,&c.armor,&c.accessories,&c.miscellaneous}) for(const auto &item:*category) {
			p[off++]=item.material; p[off++]=item.id; p[off++]=item.state; p[off++]=item.frame;
		}
		p[310]=c.lloydSide;
		const auto &r=*a.resistances;
		p[311]=r.firePermanent; p[312]=r.fireTemporary;
		// Follow the existing reader's byte mapping; DOS elemental labels are not re-inferred.
		p[313]=r.coldPermanent; p[314]=r.coldTemporary; p[315]=r.electricalPermanent; p[316]=r.electricalTemporary;
		attribute(317,*a.poisonResistance); p[319]=r.energyPermanent; p[320]=r.energyTemporary;
		p[321]=r.magicPermanent; p[322]=r.magicTemporary;
		std::copy(c.conditions.begin(),c.conditions.end(),p+323);
		p[339]=c.townUnknown; word(340,c.savedMaze); word(342,c.currentHp); word(344,c.currentSp); word(346,c.birthYear);
		const std::uint32_t xp=a.experience; for(unsigned j=0;j<4;++j) p[348+j]=xp>>(j*8);
		p[352]=c.adventuringSpell; p[353]=c.combatSpell;
	}
}

void XeenCharacterFormat::writeDosParty(std::vector<std::uint8_t> &p, const XeenSaveSnapshot &s) {
	if (p.size()!=1530 || !s.journey || !s.journey->context || !s.journey->purse ||
		!s.journey->serviceEconomy || !s.journey->worldFlags || s.activeRosterIds.size()>8)
		throw std::invalid_argument("DOS PTY writer requires the live DARK layout and a complete Journey");
	for(auto count:s.questItems) if(count>255) throw std::invalid_argument("DOS quest item count exceeds one byte");
	const auto word=[&](unsigned off,std::uint16_t v) {p[off]=v; p[off+1]=v>>8;};
	const auto dword=[&](unsigned off,std::uint32_t v) {for(unsigned j=0;j<4;++j)p[off+j]=v>>(j*8);};
	const auto flags=[&](unsigned off,const auto &values) {
		for(unsigned j=0;j<values.size();++j) {const auto mask=0x80u>>(j%8); p[off+j/8]=(p[off+j/8]&~mask)|(values[j]?mask:0);}
	};
	p[0]=p[1]=s.activeRosterIds.size();
	std::copy(s.activeRosterIds.begin(),s.activeRosterIds.end(),p.begin()+2);
	p[10]=static_cast<std::uint8_t>(s.camera.direction); p[11]=s.camera.x; p[12]=s.camera.y; p[13]=s.camera.mapId.number;
	p[14]=s.soundOptions.efx; p[15]=s.soundOptions.music;
	// PTY16, prior map17 and the unused roster-ID suffix have no v22 home.
	const auto &j=*s.journey;
	XeenGameplayContextFormat::writeDos(p,*j.context);
	const auto &e=*j.serviceEconomy;
	for(unsigned side=0;side<2;++side) {
		unsigned off=side?832:28;
		for(unsigned cat=0;cat<4;++cat) for(unsigned item=0;item<9;++item) for(unsigned shop=0;shop<4;++shop) {
			const auto &v=e.wares[side][shop][cat][item];
			p[off++]=v.material; p[off++]=v.id; p[off++]=v.state; p[off++]=v.frame;
		}
	}
	word(604,j.cloudsCompleted); word(618,s.food);
	dword(638,j.purse->gold); dword(642,j.purse->gems); dword(646,e.bank.gold); dword(650,e.bank.gems); dword(654,j.playTimeSeconds);
	flags(659,s.gameFlags); flags(723,j.worldFlags->values); flags(739,s.questFlags);
	std::copy(s.questItems.begin(),s.questItems.end(),p.begin()+747);
	p[1528]=!s.soundOptions.text; p[1529]=s.soundOptions.speech;
}

} // namespace mmodern
