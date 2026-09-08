#include "games/xeen/XeenCharacter.h"

#include <algorithm>
#include <cstdio>

namespace mmodern {

bool xeenItemHasTailCapacity(const XeenItemCategory &items) {
	return items.back().id == 0;
}

void xeenCompactItems(XeenItemCategory &items) {
	std::size_t occupied = 0;
	for (std::size_t i = 0; i < items.size(); ++i)
		if (items[i].id != 0)
			items[occupied++] = items[i];
	std::fill(items.begin() + occupied, items.end(), XeenItem{});
}

unsigned XeenCharacter::currentLevel() const {
	return static_cast<unsigned>(std::max(permanentLevel + temporaryLevel, 0));
}

bool XeenCharacter::canAct() const {
	switch (worstCondition()) {
	case XeenCondition::Asleep:
	case XeenCondition::Paralyzed:
	case XeenCondition::Unconscious:
	case XeenCondition::Dead:
	case XeenCondition::Stoned:
	case XeenCondition::Eradicated:
		return false;
	default:
		return true;
	}
}

XeenCondition XeenCharacter::worstCondition() const {
	for (std::size_t i = conditions.size(); i > 0; --i) {
		if (conditions[i - 1] != 0)
			return static_cast<XeenCondition>(i - 1);
	}
	return XeenCondition::Good;
}

std::optional<std::string> XeenCharacter::portraitResourceName() const {
	if (rosterId >= kPortraitRosterCount)
		return std::nullopt;
	char nameBuffer[11];
	std::snprintf(nameBuffer, sizeof(nameBuffer), "char%02u.fac",
		static_cast<unsigned>(rosterId) + 1);
	return std::string(nameBuffer);
}

const char *xeenClassName(XeenCharacterClass characterClass) {
	static constexpr const char *kNames[] = {
		"Knight", "Paladin", "Archer", "Cleric", "Sorcerer",
		"Robber", "Ninja", "Barbarian", "Druid", "Ranger"
	};
	const auto index = static_cast<std::size_t>(characterClass);
	return index < sizeof(kNames) / sizeof(kNames[0]) ? kNames[index] : "Unknown";
}

const char *xeenConditionName(XeenCondition condition) {
	static constexpr const char *kNames[] = {
		"Cursed", "Heart Broken", "Weak", "Poisoned", "Diseased",
		"Insane", "In Love", "Drunk", "Asleep", "Depressed", "Confused",
		"Paralyzed", "Unconscious", "Dead", "Stone", "Eradicated", "Good"
	};
	const auto index = static_cast<std::size_t>(condition);
	return index < sizeof(kNames) / sizeof(kNames[0]) ? kNames[index] : "Unknown";
}

} // namespace mmodern
