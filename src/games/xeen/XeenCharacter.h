#ifndef MMODERN_GAMES_XEEN_XEEN_CHARACTER_H
#define MMODERN_GAMES_XEEN_XEEN_CHARACTER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

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
	int permanent = 0;
	int temporary = 0;
};

struct XeenMaxStatSkills {
	bool astrologer = false;
	bool bodybuilder = false;
	bool prayerMaster = false;
	bool prestidigitation = false;
};

// Minimal serialized item data needed by the future HP/SP rules. This is not
// an inventory model; item identity and miscellaneous items are deliberately absent.
struct XeenItemModifierSource {
	std::uint8_t material = 0;
	std::uint8_t state = 0;
	std::uint8_t frame = 0;
};

struct XeenCharacter {
	static constexpr std::size_t kSerializedSize = 354;
	static constexpr std::size_t kConditionCount = 16;
	static constexpr std::size_t kEquipmentSlotsPerCategory = 9;
	static constexpr std::uint8_t kPortraitRosterCount = 24;

	std::uint8_t rosterId = 0;
	std::string name;
	XeenSex sex = XeenSex::Male;
	XeenRace race = XeenRace::Human;
	XeenCharacterClass characterClass = XeenCharacterClass::Knight;
	XeenAttributeValue intellect;
	XeenAttributeValue personality;
	XeenAttributeValue endurance;
	int permanentLevel = 0;
	int temporaryLevel = 0;
	int temporaryAge = 0;
	XeenMaxStatSkills maxStatSkills;
	bool hasSpells = false;
	std::array<XeenItemModifierSource, kEquipmentSlotsPerCategory> weapons{};
	std::array<XeenItemModifierSource, kEquipmentSlotsPerCategory> armor{};
	std::array<XeenItemModifierSource, kEquipmentSlotsPerCategory> accessories{};
	std::int16_t currentHp = 0;
	std::int16_t currentSp = 0;
	std::array<std::uint8_t, kConditionCount> conditions{};
	std::uint16_t birthYear = 0;

	unsigned currentLevel() const;
	XeenCondition worstCondition() const;
	bool canAct() const;
	std::optional<std::string> portraitResourceName() const;
};

const char *xeenClassName(XeenCharacterClass characterClass);
const char *xeenConditionName(XeenCondition condition);

} // namespace mmodern

#endif
