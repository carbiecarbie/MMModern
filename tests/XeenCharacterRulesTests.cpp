#include "games/xeen/XeenCharacterRules.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

using namespace mmodern;

namespace {

constexpr XeenCharacterRulesContext kContext{1000};

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

void checkEqual(int actual, int expected, const char *message) {
	if (actual != expected)
		throw std::runtime_error(message);
}

XeenCharacter basicCharacter() {
	XeenCharacter character;
	character.race = XeenRace::Human;
	character.characterClass = XeenCharacterClass::Knight;
	character.intellect.permanent = 11;
	character.personality.permanent = 11;
	character.endurance.permanent = 11;
	character.permanentLevel = 1;
	character.birthYear = 982; // Age 18 in kContext: no age adjustment.
	return character;
}

void equip(XeenItem &item, std::uint8_t material,
		std::uint8_t state = 0, std::uint8_t frame = 1) {
	item.material = material;
	item.state = state;
	item.frame = frame;
}

void testHpClassesRacesSkillsAndLevel() {
	constexpr std::array<XeenCharacterClass, 10> classes = {
		XeenCharacterClass::Knight, XeenCharacterClass::Paladin,
		XeenCharacterClass::Archer, XeenCharacterClass::Cleric,
		XeenCharacterClass::Sorcerer, XeenCharacterClass::Robber,
		XeenCharacterClass::Ninja, XeenCharacterClass::Barbarian,
		XeenCharacterClass::Druid, XeenCharacterClass::Ranger
	};
	constexpr std::array<int, 10> expectedBaseHp = {10, 8, 7, 5, 4, 8, 7, 12, 6, 9};
	for (std::size_t i = 0; i < classes.size(); ++i) {
		XeenCharacter character = basicCharacter();
		character.characterClass = classes[i];
		checkEqual(XeenCharacterRules::maxHp(character, kContext), expectedBaseHp[i],
			"HP base by class");
	}

	constexpr std::array<XeenRace, 5> races = {
		XeenRace::Human, XeenRace::Elf, XeenRace::Dwarf,
		XeenRace::Gnome, XeenRace::HalfOrc
	};
	constexpr std::array<int, 5> raceHp = {0, -2, 1, -1, 2};
	for (std::size_t i = 0; i < races.size(); ++i) {
		XeenCharacter character = basicCharacter();
		character.race = races[i];
		checkEqual(XeenCharacterRules::maxHp(character, kContext), 10 + raceHp[i],
			"racial HP bonus");
	}

	XeenCharacter character = basicCharacter();
	character.maxStatSkills.bodybuilder = true;
	character.permanentLevel = 3;
	checkEqual(XeenCharacterRules::maxHp(character, kContext), 33,
		"Bodybuilder must add one HP per level");

	character = basicCharacter();
	character.permanentLevel = 2;
	character.temporaryLevel = 3;
	checkEqual(XeenCharacterRules::maxHp(character, kContext), 50,
		"permanent and temporary level must add");
	character.permanentLevel = 1;
	character.temporaryLevel = -2;
	checkEqual(static_cast<int>(character.currentLevel()), 0,
		"negative resulting level must clamp to zero");
	checkEqual(XeenCharacterRules::maxHp(character, kContext), 0,
		"zero current level HP");

	character = basicCharacter();
	character.characterClass = XeenCharacterClass::Sorcerer;
	character.race = XeenRace::Elf;
	character.endurance.permanent = 0;
	character.permanentLevel = 2;
	checkEqual(XeenCharacterRules::maxHp(character, kContext), 2,
		"HP per level minimum must be one");
}

void testStatBonusBoundaries() {
	constexpr std::array<int, 23> boundaries = {
		3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 25, 30,
		35, 40, 50, 75, 100, 125, 150, 175, 200, 225, 250
	};
	constexpr std::array<int, 24> bonuses = {
		-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6,
		7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20
	};
	for (std::size_t i = 0; i < boundaries.size(); ++i) {
		XeenCharacter character = basicCharacter();
		character.characterClass = XeenCharacterClass::Barbarian;
		character.endurance.permanent = boundaries[i] - 1;
		checkEqual(XeenCharacterRules::maxHp(character, kContext) - 12, bonuses[i],
			"stat bonus immediately below threshold");
		character.endurance.permanent = boundaries[i];
		checkEqual(XeenCharacterRules::maxHp(character, kContext) - 12, bonuses[i + 1],
			"stat bonus at threshold must use next band");
	}
	XeenCharacter character = basicCharacter();
	character.characterClass = XeenCharacterClass::Barbarian;
	character.endurance.permanent = 70000;
	checkEqual(XeenCharacterRules::maxHp(character, kContext), 32,
		"top stat bonus must remain twenty");
}

void testAgeAndTemporaryAttributes() {
	struct AgeCase { int age; int physical; int mental; };
	constexpr std::array<AgeCase, 19> cases = {{
		{0, -250, -250}, {1, -50, -50}, {5, -50, -50},
		{6, -20, -20}, {10, -20, -20}, {11, -10, -10},
		{17, -10, -10}, {18, 0, 0}, {35, 0, 0},
		{36, -2, 2}, {50, -2, 2}, {51, -5, 5},
		{75, -5, 5}, {76, -10, 10}, {100, -10, 10},
		{101, -20, 20}, {200, -20, 20}, {201, -50, 50},
		{254, -50, 50}
	}};
	for (const AgeCase &entry : cases) {
		XeenCharacter character = basicCharacter();
		character.birthYear = static_cast<std::uint16_t>(kContext.currentYear - entry.age);
		character.endurance.permanent = 300;
		character.intellect.permanent = 300;
		character.personality.permanent = 300;
		checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext),
			300 + entry.physical, "physical age boundary");
		checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext),
			300 + entry.mental, "mental age boundary for Intellect");
		checkEqual(XeenCharacterRules::effectivePersonality(character, kContext),
			300 + entry.mental, "mental age boundary for Personality");
	}

	XeenCharacter character = basicCharacter();
	character.temporaryAge = 18; // Base age 18 becomes 36.
	character.endurance.permanent = 100;
	character.intellect.permanent = 100;
	checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext), 98,
		"temporary age physical adjustment");
	checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext), 102,
		"temporary age mental adjustment");

	character = basicCharacter();
	character.birthYear = 0; // Base age is capped to 254.
	character.endurance.permanent = 100;
	checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext), 50,
		"base age cap");

	character = basicCharacter();
	character.endurance.permanent = 10;
	character.endurance.temporary = -20;
	checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext), 0,
		"effective attribute minimum");
	character.intellect.temporary = 7;
	character.personality.temporary = -3;
	checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext), 18,
		"positive temporary Intellect");
	checkEqual(XeenCharacterRules::effectivePersonality(character, kContext), 8,
		"negative temporary Personality");
}

void testConditions() {
	constexpr std::array<XeenCondition, 5> shared = {
		XeenCondition::Diseased, XeenCondition::HeartBroken,
		XeenCondition::InLove, XeenCondition::Weak, XeenCondition::Drunk
	};
	for (XeenCondition value : shared) {
		XeenCharacter character = basicCharacter();
		character.endurance.permanent = 100;
		character.intellect.permanent = 100;
		character.personality.permanent = 100;
		character.conditions[static_cast<std::size_t>(value)] = 3;
		checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext), 97,
			"shared condition on Endurance");
		checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext), 97,
			"shared condition on Intellect");
		checkEqual(XeenCharacterRules::effectivePersonality(character, kContext), 97,
			"shared condition on Personality");
	}

	XeenCharacter character = basicCharacter();
	character.endurance.permanent = 100;
	character.intellect.permanent = 100;
	character.personality.permanent = 100;
	character.conditions[static_cast<std::size_t>(XeenCondition::Insane)] = 4;
	checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext), 100,
		"Insane must not affect Endurance");
	checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext), 96,
		"Insane must affect Intellect");
	checkEqual(XeenCharacterRules::effectivePersonality(character, kContext), 96,
		"Insane must affect Personality");

	character.conditions.fill(0);
	character.conditions[static_cast<std::size_t>(XeenCondition::Diseased)] = 2;
	character.conditions[static_cast<std::size_t>(XeenCondition::HeartBroken)] = 3;
	character.conditions[static_cast<std::size_t>(XeenCondition::InLove)] = 4;
	character.conditions[static_cast<std::size_t>(XeenCondition::Weak)] = 5;
	character.conditions[static_cast<std::size_t>(XeenCondition::Drunk)] = 6;
	character.conditions[static_cast<std::size_t>(XeenCondition::Insane)] = 7;
	checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext), 80,
		"accumulated physical conditions");
	checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext), 73,
		"accumulated mental conditions");

	constexpr std::array<XeenCondition, 3> fatal = {
		XeenCondition::Dead, XeenCondition::Stoned, XeenCondition::Eradicated
	};
	for (XeenCondition value : fatal) {
		character.conditions[static_cast<std::size_t>(value)] = 1;
		checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext), 100,
			"fatal condition must cancel the entire condition modifier");
		checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext), 100,
			"fatal condition mental short circuit");
		character.conditions[static_cast<std::size_t>(value)] = 0;
	}

	character.conditions.fill(0);
	character.conditions[static_cast<std::size_t>(XeenCondition::Poisoned)] = 9;
	checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext), 100,
		"Poisoned is irrelevant to HP/SP attributes");
}

void testEquipmentAttributeRules() {
	constexpr std::array<int, 8> attributeBonuses = {2, 3, 5, 8, 12, 17, 23, 30};
	for (std::size_t i = 0; i < attributeBonuses.size(); ++i) {
		XeenCharacter character = basicCharacter();
		equip(character.weapons[0], static_cast<std::uint8_t>(69 + i));
		checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext),
			11 + attributeBonuses[i], "Intellect equipment table");
		character = basicCharacter();
		equip(character.weapons[0], static_cast<std::uint8_t>(77 + i));
		checkEqual(XeenCharacterRules::effectivePersonality(character, kContext),
			11 + attributeBonuses[i], "Personality equipment table");
	}

	XeenCharacter character = basicCharacter();
	equip(character.weapons[0], 69, 0, 0);
	equip(character.weapons[1], 69, 0x40);
	equip(character.weapons[2], 69, 0x80);
	equip(character.weapons[3], 58);
	equip(character.weapons[4], 131);
	checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext), 11,
		"unequipped, cursed, broken and invalid materials must not contribute");

	character = basicCharacter();
	equip(character.weapons[0], 69, 0x3f); // Low state bits do not invalidate it.
	equip(character.armor[0], 70);
	equip(character.accessories[0], 71);
	checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext), 21,
		"weapon, armor and accessory bonuses must accumulate");
	// ID zero never gated the existing modifier rules. Misc records add no effects.
	const auto hp = XeenCharacterRules::maxHp(character, kContext);
	const auto sp = XeenCharacterRules::maxSp(character, kContext);
	character.weapons[0].id = 255; character.armor[0].id = 37; character.accessories[0].id = 1;
	character.miscellaneous.fill({69, 255, 0, 1});
	checkEqual(XeenCharacterRules::effectiveIntellect(character, kContext), 21, "IDs or misc added intellect effects");
	checkEqual(XeenCharacterRules::maxHp(character, kContext), hp, "IDs or misc added HP effects");
	checkEqual(XeenCharacterRules::maxSp(character, kContext), sp, "IDs or misc added SP effects");
	character.miscellaneous.fill({110, 37, 0, 1});
	checkEqual(XeenCharacterRules::maxSp(character, kContext), sp, "misc added direct SP effects");
	character.miscellaneous.fill({105, 37, 0, 1});
	checkEqual(XeenCharacterRules::maxHp(character, kContext), hp, "misc added direct HP effects");

	character = basicCharacter();
	character.endurance.permanent = 50;
	equip(character.weapons[0], 69);
	equip(character.armor[0], 77);
	equip(character.accessories[0], 85);
	checkEqual(XeenCharacterRules::effectiveEndurance(character, kContext), 50,
		"equipment must never add Endurance");

	static_assert(XeenCharacter::kEquipmentSlotsPerCategory * 3 == 27,
		"rules intentionally see only weapons, armor and accessories");
}

void testDirectEquipmentBonuses() {
	constexpr std::array<int, 5> hpBonuses = {4, 6, 10, 20, 50};
	for (std::size_t i = 0; i < hpBonuses.size(); ++i) {
		XeenCharacter character = basicCharacter();
		equip(character.weapons[0], static_cast<std::uint8_t>(105 + i));
		checkEqual(XeenCharacterRules::maxHp(character, kContext), 10 + hpBonuses[i],
			"direct HP equipment table");
	}

	XeenCharacter character = basicCharacter();
	character.permanentLevel = 2;
	equip(character.weapons[0], 105);
	checkEqual(XeenCharacterRules::maxHp(character, kContext), 24,
		"direct HP bonus must be added after level multiplication");

	constexpr std::array<int, 6> spBonuses = {4, 8, 12, 16, 20, 25};
	for (std::size_t i = 0; i < spBonuses.size(); ++i) {
		character = basicCharacter();
		character.characterClass = XeenCharacterClass::Sorcerer;
		character.hasSpells = true;
		equip(character.weapons[0], static_cast<std::uint8_t>(110 + i));
		checkEqual(XeenCharacterRules::maxSp(character, kContext), 3 + spBonuses[i],
			"direct SP equipment table");
	}

	character = basicCharacter();
	character.characterClass = XeenCharacterClass::Sorcerer;
	character.hasSpells = true;
	character.permanentLevel = 2;
	equip(character.weapons[0], 110);
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 10,
		"direct SP bonus must be added after level multiplication");
}

void testSpClassesSkillsAndRounding() {
	XeenCharacter character = basicCharacter();
	character.hasSpells = false;
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 0,
		"hasSpells false must return zero");
	equip(character.weapons[0], 115);
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 0,
		"hasSpells false must return before direct SP equipment");

	struct ClassCase { XeenCharacterClass characterClass; int expected; };
	constexpr std::array<ClassCase, 6> cases = {{
		{XeenCharacterClass::Sorcerer, 3}, {XeenCharacterClass::Cleric, 3},
		{XeenCharacterClass::Archer, 1}, {XeenCharacterClass::Paladin, 1},
		{XeenCharacterClass::Druid, 3}, {XeenCharacterClass::Ranger, 1}
	}};
	for (const ClassCase &entry : cases) {
		character = basicCharacter();
		character.characterClass = entry.characterClass;
		character.hasSpells = true;
		checkEqual(XeenCharacterRules::maxSp(character, kContext), entry.expected,
			"class SP rule");
	}

	character = basicCharacter();
	character.characterClass = XeenCharacterClass::Archer;
	character.hasSpells = true;
	character.intellect.permanent = 15; // (stat +2 + 3) / 2 = 2.
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 2,
		"integer division for half caster");

	character = basicCharacter();
	character.characterClass = XeenCharacterClass::Ranger;
	character.hasSpells = true;
	character.personality.permanent = 11; // Pass 3 -> half 1.
	character.intellect.permanent = 0;    // Pass clamps 1 -> half 0.
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 0,
		"Ranger must halve each pass before averaging");

	character.characterClass = XeenCharacterClass::Druid;
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 2,
		"Druid must average complete passes");

	character = basicCharacter();
	character.characterClass = XeenCharacterClass::Archer;
	character.hasSpells = true;
	character.intellect.permanent = 0;
	character.race = XeenRace::HalfOrc;
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 0,
		"SP pass minimum may become zero after half-class division");

	character = basicCharacter();
	character.characterClass = XeenCharacterClass::Knight;
	character.hasSpells = true;
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 1,
		"anomalous spell-capable class must use original fallback");
}

void testSpRacesAndSkills() {
	constexpr std::array<XeenRace, 5> races = {
		XeenRace::Human, XeenRace::Elf, XeenRace::Dwarf,
		XeenRace::Gnome, XeenRace::HalfOrc
	};
	constexpr std::array<int, 5> intellectExpected = {3, 5, 2, 4, 1};
	constexpr std::array<int, 5> personalityExpected = {3, 3, 2, 4, 1};
	for (std::size_t i = 0; i < races.size(); ++i) {
		XeenCharacter character = basicCharacter();
		character.race = races[i];
		character.hasSpells = true;
		character.characterClass = XeenCharacterClass::Sorcerer;
		checkEqual(XeenCharacterRules::maxSp(character, kContext), intellectExpected[i],
			"racial Intellect SP bonus");
		character.characterClass = XeenCharacterClass::Cleric;
		checkEqual(XeenCharacterRules::maxSp(character, kContext), personalityExpected[i],
			"racial Personality SP bonus");
	}

	XeenCharacter character = basicCharacter();
	character.hasSpells = true;
	character.characterClass = XeenCharacterClass::Sorcerer;
	character.maxStatSkills.prestidigitation = true;
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 5,
		"Prestidigitation bonus");
	character.characterClass = XeenCharacterClass::Cleric;
	character.maxStatSkills.prayerMaster = true;
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 5,
		"Prayer Master bonus");
	character.characterClass = XeenCharacterClass::Druid;
	character.maxStatSkills.astrologer = true;
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 5,
		"Astrologer Druid bonus on both passes");
	character.characterClass = XeenCharacterClass::Ranger;
	checkEqual(XeenCharacterRules::maxSp(character, kContext), 2,
		"Astrologer Ranger bonus before half-class division");

	XeenCharacter fromOne = basicCharacter();
	XeenCharacter fromTwo = basicCharacter();
	fromOne.characterClass = fromTwo.characterClass = XeenCharacterClass::Sorcerer;
	fromOne.hasSpells = fromTwo.hasSpells = true;
	fromOne.maxStatSkills.prestidigitation = std::uint8_t{1} != 0;
	fromTwo.maxStatSkills.prestidigitation = std::uint8_t{2} != 0;
	checkEqual(XeenCharacterRules::maxSp(fromOne, kContext),
		XeenCharacterRules::maxSp(fromTwo, kContext),
		"serialized skill values one and two must have identical boolean effect");
}

} // namespace

int main() {
	try {
		testHpClassesRacesSkillsAndLevel();
		testStatBonusBoundaries();
		testAgeAndTemporaryAttributes();
		testConditions();
		testEquipmentAttributeRules();
		testDirectEquipmentBonuses();
		testSpClassesSkillsAndRounding();
		testSpRacesAndSkills();
		std::cout << "Xeen character rules: synthetic HP/SP and derived attributes OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
