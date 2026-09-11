#include "formats/xeen/XeenGameplayContextFormat.h"
#include <algorithm>
#include <stdexcept>

namespace mmodern {
XeenGameplayContext XeenGameplayContextFormat::parse(const std::vector<std::uint8_t> &pty) {
	if (pty.size() < 659) throw std::invalid_argument("truncated encounter PTY context");
	auto word = [&pty](std::size_t p) {
		return static_cast<std::uint16_t>(pty[p] | (std::uint16_t(pty[p + 1]) << 8));
	};
	XeenGameplayContext c;
	if (pty[27] > 1 || pty[658] > 1) throw std::invalid_argument("invalid PTY difficulty/rested");
	c.difficulty = static_cast<XeenDifficulty>(pty[27]);
	c.ctr24 = word(610); c.day = word(612); c.year = word(614); c.minutes = word(616);
	if (c.ctr24 >= 24 || c.day >= 100 || c.minutes >= 1440)
		throw std::invalid_argument("invalid PTY time context");
	std::copy_n(pty.begin() + 18, 9, c.effects.begin());
	for (std::size_t i = 0; i < c.lightAndResistances.size(); ++i)
		c.lightAndResistances[i] = word(620 + 2 * i);
	c.rested = pty[658] != 0;
	c.newDay = c.minutes < 300;
	return c;
}
}
