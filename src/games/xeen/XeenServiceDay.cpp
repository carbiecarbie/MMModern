#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/XeenArmorRepair.h"
#include "games/xeen/XeenEquipmentPurchase.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
XeenServiceDayCandidate::XeenServiceDayCandidate(const XeenGameplayContext &context,
		const XeenServiceEconomy &economy,const XeenJourneyRandomState &cursor,std::uint16_t content,
		XeenScriptServiceCharge serviceCharge):
	originalContext(context),endingContext(context),originalEconomy(economy),endingEconomy(economy),
	originalRandom(cursor),random(cursor),content(content),charge(serviceCharge) {
	if(content<11 || content>14)throw std::invalid_argument("Service economy requires Journey content 11 through 14");
	const auto successor=charge==XeenScriptServiceCharge::OneDay ?
		xeenPrepareSmithDeparture(context,content) : xeenPrepareTemplePaidDeparture(context,content);
	if(!successor)throw std::invalid_argument("Unsupported script service context");
	xeenValidateCurrentServiceEconomy(economy,content);endingContext=*successor;
	regenerating=xeenServiceDayRegenerates(originalContext.day,endingContext.day,static_cast<std::uint16_t>(charge));
	completed=!regenerating;
}
bool XeenServiceDayCandidate::service(unsigned budget,const std::function<void()> &check,
		const std::function<void()> &afterStock) {
	if(check)check();
	if(completed)return true;
	XeenConsequenceDraw draw{random,std::min(budget,64u),check};
	if(!stock.service(draw))return false;
	xeenValidateMerchantWares(stock.wares());
	if(check)check();
	try {if(afterStock)afterStock();}catch(...) {if(check)check();throw;}
	if(check)check();
	endingEconomy.wares=stock.wares();endingEconomy.bank=xeenPrepareBankInterest(originalEconomy.bank);
	completed=true;return true;
}
XeenServiceDayCandidate XeenServiceDayCandidate::rebindPurchase(const XeenServiceEconomy &after,
		XeenInventoryCategory category,std::size_t slot,const XeenItem &expected) const {
	if((content!=13 && content!=14) || !completed)throw std::invalid_argument("Equipment Buy requires a complete purchase departure");
	validateComplete();
	xeenValidateEquipmentPurchaseEconomyDelta(originalEconomy,after,category,slot,expected);
	xeenValidateCurrentServiceEconomy(after,content);
	// Copy the completed random/generation state; never construct/service another
	// generation or recalculate interest. Triggered ending stock remains exact.
	auto replacement=*this;
	replacement.originalEconomy=after;
	if(!regenerating)replacement.endingEconomy=after;
	replacement.validateComplete();
	return replacement;
}
XeenServiceDayCandidate XeenServiceDayCandidate::upgradeTemplePaid() const {
	if(content!=14 || charge!=XeenScriptServiceCharge::OneDay || !completed)
		throw std::invalid_argument("Temple upgrade requires a complete one-day reservation");
	validateComplete();
	auto paid=xeenPrepareTemplePaidDeparture(originalContext,content);
	if(!paid)throw std::invalid_argument("Temple two-day departure exceeds the supported year");
	if(!regenerating)return {originalContext,originalEconomy,originalRandom,content,
		XeenScriptServiceCharge::TemplePaid};
	auto replacement=*this;
	replacement.charge=XeenScriptServiceCharge::TemplePaid;
	replacement.endingContext=*paid;
	replacement.validateComplete();
	return replacement;
}
void XeenServiceDayCandidate::validateComplete() const {
	if(!completed)throw std::invalid_argument("Service-day departure is incomplete");
	const auto expected=charge==XeenScriptServiceCharge::OneDay ?
		xeenPrepareSmithDeparture(originalContext,content) : xeenPrepareTemplePaidDeparture(originalContext,content);
	if(!expected || !(endingContext==*expected) ||
		regenerating!=xeenServiceDayRegenerates(originalContext.day,endingContext.day,static_cast<std::uint16_t>(charge)))
		throw std::invalid_argument("Service-day context successor changed");
	xeenValidateCurrentServiceEconomy(originalEconomy,content);
	const auto ending=random.continuation();
	if(!regenerating) {
		if(endingEconomy!=originalEconomy || ending!=originalRandom)
			throw std::invalid_argument("Nontriggering service-day successor changed");
	} else {
		xeenValidateServiceEconomy(endingEconomy);
		// Verify the prepared interest relation without executing another interest
		// operation (whose production call trace must remain exactly once).
		const auto interest=[](std::uint32_t value) noexcept {
			return static_cast<std::uint32_t>(std::uint64_t(value)+value/100u);
		};
		if(endingEconomy.bank.gold!=interest(originalEconomy.bank.gold) ||
			endingEconomy.bank.gems!=interest(originalEconomy.bank.gems) ||
			ending.algorithm!=originalRandom.algorithm || !ending.state || ending.count<=originalRandom.count)
			throw std::invalid_argument("Triggering service-day successor changed");
	}
}
}
