#include "formats/xeen/XeenCharacterFormat.h"

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
constexpr std::size_t kAstrologerOffset = 41;
constexpr std::size_t kBodybuilderOffset = 42;
constexpr std::size_t kPrayerMasterOffset = 51;
constexpr std::size_t kPrestidigitationOffset = 52;
constexpr std::size_t kHasSpellsOffset = 163;
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

XeenCombatInputs XeenCharacterFormat::parseCombatInputs(const std::vector<std::uint8_t> &bytes, std::size_t owner, bool includeLuck) {
	if (bytes.size() != XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize || owner >= XeenRoster::kCharacterCount)
		throw std::invalid_argument("combat CHR requires exactly thirty complete 354-byte records and a valid owner");
	const auto *p = bytes.data() + owner * XeenCharacter::kSerializedSize;
	XeenCombatInputs result{{p[20],p[21]}, {p[28],p[29]}, {p[30],p[31]}, p[34],
		std::uint32_t(p[348]) | (std::uint32_t(p[349])<<8) | (std::uint32_t(p[350])<<16) | (std::uint32_t(p[351])<<24)};
	if (includeLuck) result.luck = XeenAttributeValue{p[32],p[33]};
	return result;
}

XeenRoster XeenCharacterFormat::parseRoster(const std::vector<std::uint8_t> &bytes) {
	const std::size_t expectedSize = XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize;
	if (bytes.size() != expectedSize) {
		throw std::runtime_error("maze.chr possui " + std::to_string(bytes.size()) +
			" bytes; esperado: " + std::to_string(expectedSize));
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
		character.maxStatSkills.astrologer = record[kAstrologerOffset] != 0;
		character.maxStatSkills.bodybuilder = record[kBodybuilderOffset] != 0;
		character.maxStatSkills.prayerMaster = record[kPrayerMasterOffset] != 0;
		character.maxStatSkills.prestidigitation = record[kPrestidigitationOffset] != 0;
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
	}
	return roster;
}

XeenCharacterFormat::PartyHeader XeenCharacterFormat::parsePartyHeader(
		const std::vector<std::uint8_t> &bytes) {
	constexpr std::size_t kRequiredSize = 2 + XeenParty::kSerializedMemberSlots;
	if (bytes.size() < kRequiredSize)
		throw std::runtime_error("maze.pty truncado antes da lista da Party");

	PartyHeader result;
	result.firstCount = bytes[0];
	result.effectiveCount = bytes[1];
	for (std::size_t i = 0; i < result.rosterIds.size(); ++i) {
		const auto value = bytes[2 + i];
		result.rosterIds[i] = value == 0xff ? -1 : value;
	}
	return result;
}

} // namespace mmodern
