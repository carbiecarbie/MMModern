#ifndef MMODERN_GAMES_XEEN_XEEN_CHARACTER_H
#define MMODERN_GAMES_XEEN_XEEN_CHARACTER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include "games/xeen/XeenMutation.h"

namespace mmodern {

enum class XeenSex : std::uint8_t {
	Male = 0,
	Female = 1,
	YesPlease = 2
};

enum class XeenRace : std::uint8_t {
	Human = 0,
	Elf = 1,
	Dwarf = 2,
	Gnome = 3,
	HalfOrc = 4
};

enum class XeenCharacterClass : std::uint8_t {
	Knight = 0,
	Paladin = 1,
	Archer = 2,
	Cleric = 3,
	Sorcerer = 4,
	Robber = 5,
	Ninja = 6,
	Barbarian = 7,
	Druid = 8,
	Ranger = 9
};

enum class XeenCondition : std::uint8_t {
	Cursed = 0,
	HeartBroken = 1,
	Weak = 2,
	Poisoned = 3,
	Diseased = 4,
	Insane = 5,
	InLove = 6,
	Drunk = 7,
	Asleep = 8,
	Depressed = 9,
	Confused = 10,
	Paralyzed = 11,
	Unconscious = 12,
	Dead = 13,
	Stoned = 14,
	Eradicated = 15,
	Good = 16
};

struct XeenAttributeValue {
	XeenMutable<int> permanent = 0;
	XeenMutable<int> temporary = 0;
};

struct XeenMaxStatSkills {
	XeenMutable<bool> astrologer = false;
	XeenMutable<bool> bodybuilder = false;
	XeenMutable<bool> prayerMaster = false;
	XeenMutable<bool> prestidigitation = false;
};

// Stored original item bytes. ID zero is empty, but other bytes remain state.
struct XeenItem {
	XeenMutable<std::uint8_t> material = 0;
	XeenMutable<std::uint8_t> id = 0;
	XeenMutable<std::uint8_t> state = 0;
	XeenMutable<std::uint8_t> frame = 0;
};

// Fixed category size checks the helper boundary without accepting arbitrary spans.
using XeenItemCategory = std::array<XeenItem, 9>;
// Only the final slot determines capacity; earlier holes do not make room.
bool xeenItemHasTailCapacity(const XeenItemCategory &items);
// Explicit stable compaction clears empty-slot metadata. Never implicit in I/O.
void xeenCompactItems(XeenItemCategory &items);

struct XeenCharacter {
	static constexpr std::size_t kSerializedSize = 354;
	static constexpr std::size_t kConditionCount = 16;
	static constexpr std::size_t kEquipmentSlotsPerCategory = 9;
	static constexpr std::uint8_t kPortraitRosterCount = 24;
	using XeenLearnedSpells = XeenMutableArray<std::uint8_t, 39>;

	XeenMutable<std::uint8_t> rosterId = 0;
	XeenMutableString name;
	XeenMutable<XeenSex> sex = XeenSex::Male;
	XeenMutable<XeenRace> race = XeenRace::Human;
	XeenMutable<XeenCharacterClass> characterClass = XeenCharacterClass::Knight;
	XeenAttributeValue intellect;
	XeenAttributeValue personality;
	XeenAttributeValue endurance;
	XeenMutable<int> permanentLevel = 0;
	XeenMutable<int> temporaryLevel = 0;
	XeenMutable<int> temporaryAge = 0;
	XeenMaxStatSkills maxStatSkills;
	XeenMutable<bool> hasSpells = false;
	// Presence is distinct from an explicitly empty spellbook. Original flags are retained verbatim.
	XeenMutableOptional<XeenLearnedSpells> learnedSpells;
	XeenItemCategory weapons{};
	XeenItemCategory armor{};
	XeenItemCategory accessories{};
	XeenItemCategory miscellaneous{};
	XeenMutable<std::int16_t> currentHp = 0;
	XeenMutable<std::int16_t> currentSp = 0;
	XeenMutableArray<std::uint8_t, kConditionCount> conditions{};
	XeenMutable<std::uint16_t> birthYear = 0;

	unsigned currentLevel() const;
	XeenCondition worstCondition() const;
	bool canAct() const;
	std::optional<std::string> portraitResourceName() const;
};

const char *xeenClassName(XeenCharacterClass characterClass);
const char *xeenConditionName(XeenCondition condition);

} // namespace mmodern

#endif
