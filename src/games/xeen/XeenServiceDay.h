#ifndef MMODERN_XEEN_SERVICE_DAY_H
#define MMODERN_XEEN_SERVICE_DAY_H
#include "games/xeen/XeenMerchantGeneration.h"
namespace mmodern {
// The reference changed-day/charge predicate is arithmetic only. This grants
// no admission to multi-day calls, other modes or calendar advancement.
inline bool xeenServiceDayRegenerates(unsigned oldDay,unsigned destinationDay,std::uint64_t charge) noexcept {
	return oldDay!=destinationDay && (destinationDay%10==1 || charge>1440);
}
class XeenServiceDayCandidate {
public:
	XeenServiceDayCandidate(const XeenGameplayContext &,const XeenServiceEconomy &,
		const XeenJourneyRandomState &,std::uint16_t content=11);
	bool service(unsigned budget=64,const std::function<void()> &check={},
		const std::function<void()> &afterStock={});
	bool complete() const noexcept { return completed; }
	bool triggered() const noexcept { return regenerating; }
	const XeenGameplayContext &beforeContext() const noexcept { return originalContext; }
	const XeenServiceEconomy &beforeEconomy() const noexcept { return originalEconomy; }
	const XeenJourneyRandomState &beforeRandom() const noexcept { return originalRandom; }
	const XeenGameplayContext &context() const noexcept { return endingContext; }
	const XeenServiceEconomy &economy() const noexcept { return endingEconomy; }
	XeenJourneyRandomState continuation() const { return random.continuation(); }
private:
	XeenGameplayContext originalContext,endingContext;
	XeenServiceEconomy originalEconomy,endingEconomy;
	XeenJourneyRandomState originalRandom;
	XeenCombatRandom random;
	XeenMerchantStockCandidate stock;
	bool regenerating=false,completed=false;
};
}
#endif
