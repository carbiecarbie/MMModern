#include "games/xeen/XeenGameplayContext.h"
#include <limits>
#include <stdexcept>

namespace mmodern {
XeenTimePreparation xeenPrepareTime(const XeenGameplayContext &current, std::uint64_t charge) {
	if (current.day >= 100 || current.minutes >= 1440 || current.ctr24 >= 24 ||
		current.profile != XeenBehaviorProfile::WorldOfXeenClouds ||
		static_cast<unsigned>(current.difficulty) > 1)
		throw std::invalid_argument("Noncanonical gameplay calendar");
	XeenTimePreparation result;
	result.context = current;
	constexpr std::uint64_t daysPerYear = 100, minutesPerDay = 1440;
	const std::uint64_t start = (std::uint64_t(current.year)*daysPerYear+current.day)*minutesPerDay+current.minutes;
	const std::uint64_t limit = (std::uint64_t(std::numeric_limits<std::uint16_t>::max())+1)*daysPerYear*minutesPerDay;
	if (charge >= limit-start) throw std::overflow_error("Gameplay calendar year overflow");
	const auto end = start+charge;
	const auto crossings = [&](std::uint64_t period, std::uint64_t offset) {
		const auto through = [&](std::uint64_t t) { return t < offset ? std::uint64_t(0) : (t-offset)/period+1; };
		return through(end)-through(start);
	};
	result.processing480 = charge/480 + (current.minutes%480+charge%480)/480 != 0;
	result.midnights = end/minutesPerDay-start/minutesPerDay;
	result.yearRollovers = end/(minutesPerDay*daysPerYear)-start/(minutesPerDay*daysPerYear);
	result.dawns = crossings(minutesPerDay,300);
	result.dusks = crossings(minutesPerDay,1260);
	result.context.minutes = static_cast<std::uint16_t>(end%minutesPerDay);
	result.context.day = static_cast<std::uint16_t>((end/minutesPerDay)%daysPerYear);
	result.context.year = static_cast<std::uint16_t>(end/(minutesPerDay*daysPerYear));
	// Party::addTime compares ending day numbers, even after whole years.
	if (result.context.day != current.day) result.context.newDay = true;
	result.dailyProcessing = result.context.newDay && result.context.minutes >= 300;
	return result;
}

bool xeenRegionalContext(const XeenGameplayContext &c) noexcept {
	return c.profile == XeenBehaviorProfile::WorldOfXeenClouds && static_cast<unsigned>(c.difficulty)<=1 &&
		c.day < 100 && c.ctr24 < 24 && c.minutes < 1440 && c.effects[1]<=1 && c.effects[2]<=1 && c.effects[3]<=1 && c.effects[4]<=1;
}
}
