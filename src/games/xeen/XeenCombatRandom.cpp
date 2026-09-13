#include "games/xeen/XeenCombat.h"
#include <limits>
#include <stdexcept>
namespace mmodern {
namespace { void require(bool value,const char *message) { if (!value) throw std::invalid_argument(message); } }
XeenCombatRandom::XeenCombatRandom(std::uint32_t seed):value(seed) { require(seed!=0,"combat seed must be nonzero"); }
XeenCombatRandom::XeenCombatRandom(std::vector<Draw> values):tape(std::make_shared<const std::vector<Draw>>(std::move(values))) {}
XeenCombatRandom::XeenCombatRandom(XeenJourneyRandomState r):value(r.state),offset(r.count) { require(r.algorithm==1 && r.state, "Invalid combat continuation"); }
XeenJourneyRandomState XeenCombatRandom::continuation() const { require(!tape,"Diagnostic tape is not a durable continuation"); return {1,value,offset}; }
std::optional<std::uint32_t> XeenCombatRandom::draw(std::uint32_t lo,std::uint32_t hi) {
	require(lo<=hi && std::uint64_t(hi)-lo+1<=std::numeric_limits<std::uint32_t>::max(),"invalid random interval");
	std::uint32_t raw;
	if(offset==std::numeric_limits<std::uint64_t>::max())throw std::overflow_error("combat random cursor exhausted");
	if(tape) {
		require(offset<tape->size(),"combat random tape exhausted"); const auto d=(*tape)[offset++];
		require(d.lo==lo&&d.hi==hi,"combat random request differs from tape");
		if(!d.raw) { require(d.value>=lo&&d.value<=hi,"combat random value outside request"); return d.value; }
		raw=d.value;
	} else { value^=value<<13; value^=value>>17; value^=value<<5; raw=value; ++offset; }
	const std::uint32_t span=hi-lo+1,threshold=(0u-span)%span;
	if(raw<threshold) return {}; return lo+raw%span;
}

}
