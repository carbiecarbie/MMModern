#ifndef MMODERN_XEEN_SERVICE_DAY_H
#define MMODERN_XEEN_SERVICE_DAY_H
#include "games/xeen/XeenMerchantGeneration.h"
#include "games/xeen/XeenItemCatalog.h"
namespace mmodern {
enum class XeenScriptServiceCharge : std::uint16_t { OneDay=1440, TemplePaid=2880 };
// The reference changed-day/charge predicate is arithmetic only. This grants
// no admission to multi-day calls, other modes or calendar advancement.
inline bool xeenServiceDayRegenerates(unsigned oldDay,unsigned destinationDay,std::uint64_t charge) noexcept {
	return oldDay!=destinationDay && (destinationDay%10==1 || charge>1440);
}
class XeenServiceDayCandidate {
public:
	XeenServiceDayCandidate(const XeenGameplayContext &,const XeenServiceEconomy &,
		const XeenJourneyRandomState &,
		XeenScriptServiceCharge charge=XeenScriptServiceCharge::OneDay, bool deferBankInterest=false, bool deferGeneration=false);
	bool service(unsigned budget=64,const std::function<void()> &check={},
		const std::function<void()> &afterStock={});
	bool complete() const noexcept { return completed && !generationDeferred; }
	bool reservationReady() const noexcept { return completed; }
	bool triggered() const noexcept { return regenerating; }
	const XeenGameplayContext &beforeContext() const noexcept { return originalContext; }
	const XeenServiceEconomy &beforeEconomy() const noexcept { return originalEconomy; }
	const XeenJourneyRandomState &beforeRandom() const noexcept { return originalRandom; }
	const XeenGameplayContext &context() const noexcept { return endingContext; }
	const XeenServiceEconomy &economy() const noexcept { return endingEconomy; }
	XeenJourneyRandomState continuation() const { return generationDeferred ? originalRandom : random.continuation(); }
	// A service visit reserves the successor without drawing future merchant RNG.
	bool pendingGeneration() const noexcept { return generationDeferred; }
	static constexpr std::uint64_t kDeferredRandomReserve=160*8*2;
	static bool randomRoom(const XeenJourneyRandomState &cursor) noexcept {
		return cursor.count<=UINT64_MAX-kDeferredRandomReserve;
	}
	XeenServiceDayCandidate rebindRandom(const XeenJourneyRandomState &) const;
	void validateComplete() const;
	void validateReservation() const;
	// Narrow relation for an already complete Smith departure. It
	// preserves the existing reservation while copying/validating a replacement;
	// the Flow supplies and atomically publishes its checked identity increment.
	XeenServiceDayCandidate rebindPurchase(const XeenServiceEconomy &after,
		XeenInventoryCategory,std::size_t slot,const XeenItem &expected) const;
	// Copy an already generated triggering reservation; otherwise start a fresh
	// private generation from the same live entry preimage.
	XeenServiceDayCandidate upgradeTemplePaid() const;
    XeenServiceDayCandidate addTemplePaidDay() const;
    XeenServiceDayCandidate rebindContext(const XeenGameplayContext &) const;
    std::uint16_t minutes() const noexcept {return static_cast<std::uint16_t>(charge);}
	// Bank menus update only balances; keep the reserved stock and RNG. Interest
	// is resolved once when departure closes the menu to further transactions.
	XeenServiceDayCandidate rebindBank(const XeenServiceEconomy &) const;
	XeenServiceDayCandidate settleBank() const;
private:
	XeenGameplayContext originalContext,endingContext;
	XeenServiceEconomy originalEconomy,endingEconomy;
	XeenJourneyRandomState originalRandom;
	XeenCombatRandom random;
	XeenMerchantStockCandidate stock;
	XeenScriptServiceCharge charge;
	bool regenerating=false,completed=false;
	bool bankInterestDeferred=false, generationDeferred=false, generationStarted=false;
};
}
#endif
