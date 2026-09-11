#ifndef MMODERN_XEEN_GAMEPLAY_CONTEXT_H
#define MMODERN_XEEN_GAMEPLAY_CONTEXT_H
#include <array>
#include <cstdint>

namespace mmodern {
enum class XeenBehaviorProfile { WorldOfXeenClouds };
enum class XeenDifficulty : std::uint8_t { Adventurer, Warrior };

struct XeenGameplayContext {
	XeenBehaviorProfile profile = XeenBehaviorProfile::WorldOfXeenClouds;
	XeenDifficulty difficulty = XeenDifficulty::Adventurer;
	std::uint16_t ctr24 = 0, day = 0, year = 0, minutes = 0;
	std::array<std::uint8_t, 9> effects{};
	std::array<std::uint16_t, 6> lightAndResistances{};
	bool rested = false, newDay = false;
	friend bool operator==(const XeenGameplayContext &a, const XeenGameplayContext &b) {
		return a.profile == b.profile && a.difficulty == b.difficulty && a.ctr24 == b.ctr24 &&
			a.day == b.day && a.year == b.year && a.minutes == b.minutes && a.effects == b.effects &&
			a.lightAndResistances == b.lightAndResistances && a.rested == b.rested && a.newDay == b.newDay;
	}
};
}
#endif
