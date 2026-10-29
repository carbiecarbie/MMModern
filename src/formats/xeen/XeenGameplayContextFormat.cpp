#include "formats/xeen/XeenGameplayContextFormat.h"
#include <algorithm>
#include <stdexcept>

namespace mmodern {
void XeenGameplayContextFormat::writeDos(std::vector<std::uint8_t> &p, const XeenGameplayContext &c) {
	if(p.size()<659) throw std::invalid_argument("Truncated DOS PTY context output");
	const auto word=[&](unsigned off,std::uint16_t v){p[off]=v; p[off+1]=v>>8;};
	std::copy(c.effects.begin(),c.effects.end(),p.begin()+18); p[27]=static_cast<std::uint8_t>(c.difficulty);
	word(610,c.ctr24); word(612,c.day); word(614,c.year); word(616,c.minutes);
	for(unsigned i=0;i<6;++i) word(620+2*i,c.lightAndResistances[i]);
	p[658]=c.rested;
}
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
