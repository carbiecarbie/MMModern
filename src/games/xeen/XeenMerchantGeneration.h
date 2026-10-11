#ifndef MMODERN_XEEN_MERCHANT_GENERATION_H
#define MMODERN_XEEN_MERCHANT_GENERATION_H
#include "games/xeen/XeenServiceEconomy.h"
#include "games/xeen/XeenItemGeneration.h"
#include <optional>
namespace mmodern {
// The merchant entry point shares the reason-0 generator with Event treasure.
using XeenMerchantItemCandidate = XeenItemCandidate;
class XeenMerchantStockCandidate {
public:
	XeenMerchantStockCandidate()=default;
	bool service(XeenConsequenceDraw &);
	bool complete() const noexcept { return side==2; }
	const XeenMerchantWares &wares() const noexcept { return generated; }
	unsigned generatedItems() const noexcept { return totalItems; }
	unsigned discardedItems() const noexcept { return discarded; }
private:
	XeenMerchantWares generated;
	std::array<unsigned,4> counts{};
	unsigned side=0,shop=0,band=0,ordinal=0,totalItems=0,discarded=0;
	std::optional<XeenMerchantItemCandidate> item;
};
}
#endif
