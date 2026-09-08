#include "games/xeen/XeenCharacterRules.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace mmodern {
namespace {

// Original constants: devtools/create_mm/create_xeen/constants.cpp.
constexpr std::array<int, 10> kBaseHpByClass = {
	10, 8, 7, 5, 4, 8, 7, 12, 6, 9
};
constexpr std::array<int, 5> kRaceHpBonuses = {0, -2, 1, -1, 2};
constexpr std::array<std::array<int, 2>, 5> kRaceSpBonuses = {{
	{{ 0,  0}},
	{{ 2,  0}},
	{{-1, -1}},
	{{ 1,  1}},
	{{-2, -2}}
}};
constexpr std::array<int, 10> kAgeRanges = {
	1, 6, 11, 18, 36, 51, 76, 101, 201, 65535
};
constexpr std::array<std::array<int, 10>, 2> kAgeAdjustments = {{
	{{-250, -50, -20, -10, 0, -2, -5, -10, -20, -50}},
	{{-250, -50, -20, -10, 0,  2,  5,  10,  20,  50}}
}};
constexpr std::array<int, 24> kStatValues = {
	3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 25, 30,
	35, 40, 50, 75, 100, 125, 150, 175, 200, 225, 250, 65535
};
constexpr std::array<int, 24> kStatBonuses = {
	-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6,
	7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20
};
constexpr std::array<int, 10> kAttributeCategories = {
	9, 17, 25, 33, 39, 45, 50, 56, 61, 72
};
constexpr std::array<int, 72> kAttributeBonuses = {
	2, 3, 5, 8, 12, 17, 23, 30, 38, 47,
	2, 3, 5, 8, 12, 17, 23, 30,
	2, 3, 5, 8, 12, 17, 23, 30,
	2, 3, 5, 8, 12, 17, 23, 30,
	3, 5, 10, 15, 20, 30,
	5, 10, 15, 20, 25, 30,
	4, 6, 10, 20, 50,
	4, 8, 12, 16, 20, 25,
	2, 4, 6, 10, 16,
	4, 6, 8, 10, 12, 14, 16, 18, 20, 25
};

enum class DerivedAttribute {
	Intellect = 1,
	Personality = 2,
	Endurance = 3
};

constexpr int kHpBonusCategory = 7;
constexpr int kSpBonusCategory = 8;
constexpr std::uint8_t kCursedOrBrokenMask = 0xc0;

std::size_t enumIndex(XeenRace race) {
	return static_cast<std::size_t>(race);
}

std::size_t enumIndex(XeenCharacterClass characterClass) {
	return static_cast<std::size_t>(characterClass);
}

std::uint8_t condition(const XeenCharacter &character, XeenCondition value) {
	return character.conditions[static_cast<std::size_t>(value)];
}

template<bool Checked> int add(int a, int b) {
	if constexpr (Checked) {
		const auto result = static_cast<std::int64_t>(a) + b;
		if (result < std::numeric_limits<int>::min() || result > std::numeric_limits<int>::max())
			throw std::invalid_argument("character rule addition exceeds the integer domain");
		return static_cast<int>(result);
	} else {
		return a + b;
	}
}

template<bool Checked> int multiply(int a, int b) {
	if constexpr (Checked) {
		const auto result = static_cast<std::int64_t>(a) * b;
		if (result < std::numeric_limits<int>::min() || result > std::numeric_limits<int>::max())
			throw std::invalid_argument("character rule product exceeds the integer domain");
		return static_cast<int>(result);
	} else {
		return a * b;
	}
}

template<bool Checked>
int currentLevel(const XeenCharacter &character) {
	return std::max(add<Checked>(character.permanentLevel, character.temporaryLevel), 0);
}

template<bool Checked>
int effectiveAge(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	// The original subtracts unsigned years, caps the result, then adds temp age.
	const std::uint32_t baseAge = std::min(context.currentYear - character.birthYear, 254u);
	return add<Checked>(static_cast<int>(baseAge), character.temporaryAge);
}

template<bool Checked>
int ageAdjustment(const XeenCharacter &character,
		const XeenCharacterRulesContext &context, bool mental) {
	const int age = effectiveAge<Checked>(character, context);
	std::size_t index = 0;
	while (index + 1 < kAgeRanges.size() && kAgeRanges[index] <= age)
		++index;
	return kAgeAdjustments[mental ? 1 : 0][index];
}

int conditionModifier(const XeenCharacter &character, DerivedAttribute attribute) {
	if (condition(character, XeenCondition::Dead) ||
			condition(character, XeenCondition::Stoned) ||
			condition(character, XeenCondition::Eradicated))
		return 0;

	int result = 0;
	if (attribute == DerivedAttribute::Intellect || attribute == DerivedAttribute::Personality) {
		result -= condition(character, XeenCondition::Insane);
		result -= condition(character, XeenCondition::Diseased);
	} else {
		result -= condition(character, XeenCondition::Diseased);
	}
	result -= condition(character, XeenCondition::HeartBroken);
	result -= condition(character, XeenCondition::InLove);
	result -= condition(character, XeenCondition::Weak);
	result -= condition(character, XeenCondition::Drunk);
	return result;
}

template<std::size_t N>
int itemBonusFrom(const std::array<XeenItem, N> &items, int category) {
	int result = 0;
	for (const XeenItem &item : items) {
		if (item.frame == 0 || (item.state & kCursedOrBrokenMask) != 0 ||
				item.material < 59 || item.material > 130 || category == 3)
			continue;

		const int materialIndex = item.material - 59;
		std::size_t attributeCategory = 0;
		while (kAttributeCategories[attributeCategory] < materialIndex)
			++attributeCategory;
		int effectiveCategory = static_cast<int>(attributeCategory);
		// Xeen has no equipment category for Endurance.
		if (effectiveCategory > 2)
			++effectiveCategory;
		if (effectiveCategory == category)
			result += kAttributeBonuses[static_cast<std::size_t>(materialIndex)];
	}
	return result;
}

int itemBonus(const XeenCharacter &character, int category) {
	return itemBonusFrom(character.weapons, category) +
		itemBonusFrom(character.armor, category) +
		itemBonusFrom(character.accessories, category);
}

int statBonus(int value) {
	std::size_t index = 0;
	while (index + 1 < kStatValues.size() && kStatValues[index] <= value)
		++index;
	return kStatBonuses[index];
}

const XeenAttributeValue &attributeValue(const XeenCharacter &character,
		DerivedAttribute attribute) {
	switch (attribute) {
	case DerivedAttribute::Intellect:
		return character.intellect;
	case DerivedAttribute::Personality:
		return character.personality;
	case DerivedAttribute::Endurance:
		return character.endurance;
	}
	return character.endurance;
}

template<bool Checked>
int effectiveAttribute(const XeenCharacter &character,
		const XeenCharacterRulesContext &context, DerivedAttribute attribute) {
	const XeenAttributeValue &value = attributeValue(character, attribute);
	const bool mental = attribute != DerivedAttribute::Endurance;
	const int equipment = attribute == DerivedAttribute::Endurance ? 0 :
		itemBonus(character, static_cast<int>(attribute));
	int result = add<Checked>(value.permanent, value.temporary);
	result = add<Checked>(result, ageAdjustment<Checked>(character, context, mental));
	result = add<Checked>(result, equipment);
	return std::max(add<Checked>(result, conditionModifier(character, attribute)), 0);
}

template<bool Checked>
int spPass(const XeenCharacter &character, const XeenCharacterRulesContext &context,
		DerivedAttribute attribute, bool hasRelevantSkill) {
	const int effective = effectiveAttribute<Checked>(character, context, attribute);
	const std::size_t race = enumIndex(character.race);
	const std::size_t racialColumn = attribute == DerivedAttribute::Intellect ? 0 : 1;
	int base = statBonus(effective) + 3 + kRaceSpBonuses[race][racialColumn];
	if (hasRelevantSkill)
		base += 2;
	base = std::max(base, 1);
	int result = multiply<Checked>(base, currentLevel<Checked>(character));
	if (character.characterClass != XeenCharacterClass::Sorcerer &&
			character.characterClass != XeenCharacterClass::Cleric &&
			character.characterClass != XeenCharacterClass::Druid)
		result /= 2;
	return result;
}

template<bool Checked>
int maximumHp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	int hpPerLevel = kBaseHpByClass[enumIndex(character.characterClass)] +
		statBonus(effectiveAttribute<Checked>(character, context, DerivedAttribute::Endurance)) +
		kRaceHpBonuses[enumIndex(character.race)] +
		(character.maxStatSkills.bodybuilder ? 1 : 0);
	hpPerLevel = std::max(hpPerLevel, 1);
	return std::max(add<Checked>(multiply<Checked>(hpPerLevel, currentLevel<Checked>(character)),
		itemBonus(character, kHpBonusCategory)), 0);
}

template<bool Checked>
int maximumSp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	if (!character.hasSpells)
		return 0;

	int result = 0;
	if (character.characterClass == XeenCharacterClass::Sorcerer ||
			character.characterClass == XeenCharacterClass::Archer) {
		result = spPass<Checked>(character, context, DerivedAttribute::Intellect,
			character.maxStatSkills.prestidigitation);
	} else if (character.characterClass == XeenCharacterClass::Druid ||
			character.characterClass == XeenCharacterClass::Ranger) {
		const int personality = spPass<Checked>(character, context, DerivedAttribute::Personality,
			character.maxStatSkills.astrologer);
		const int intellect = spPass<Checked>(character, context, DerivedAttribute::Intellect,
			character.maxStatSkills.astrologer);
		result = add<Checked>(personality, intellect) / 2;
	} else {
		// This also preserves the original fallback for anomalous spell-capable records.
		result = spPass<Checked>(character, context, DerivedAttribute::Personality,
			character.maxStatSkills.prayerMaster);
	}

	return std::max(add<Checked>(result, itemBonus(character, kSpBonusCategory)), 0);
}

} // namespace

void XeenCharacterRules::validateForUse(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	if (enumIndex(character.race) >= kRaceHpBonuses.size() ||
			enumIndex(character.characterClass) >= kBaseHpByClass.size())
		throw std::invalid_argument("active character has an unsupported race or class");
	static_cast<void>(currentLevel<true>(character));
	for (const auto attribute : {DerivedAttribute::Intellect, DerivedAttribute::Personality,
			DerivedAttribute::Endurance})
		static_cast<void>(effectiveAttribute<true>(character, context, attribute));
	static_cast<void>(maximumHp<true>(character, context));
	static_cast<void>(maximumSp<true>(character, context));
}

int XeenCharacterRules::effectiveEndurance(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return effectiveAttribute<false>(character, context, DerivedAttribute::Endurance);
}

int XeenCharacterRules::effectiveIntellect(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return effectiveAttribute<false>(character, context, DerivedAttribute::Intellect);
}

int XeenCharacterRules::effectivePersonality(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return effectiveAttribute<false>(character, context, DerivedAttribute::Personality);
}

int XeenCharacterRules::maxHp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return maximumHp<false>(character, context);
}

int XeenCharacterRules::maxSp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return maximumSp<false>(character, context);
}

} // namespace mmodern
