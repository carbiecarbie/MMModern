#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/XeenArmorRepair.h"
#include "games/xeen/XeenEquipmentPurchase.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
namespace {
bool serviceDepartureRegenerates(unsigned oldDay,unsigned endingDay,std::uint16_t minutes) noexcept {
    // DOS addTime 0x263e7..2640b compares the original charge as a signed
    // word (jle at 0x263f9), independently of unsigned minute normalization.
    const int signedMinutes=minutes<32768?int(minutes):int(minutes)-65536;
    return oldDay!=endingDay && (endingDay%10==1 || signedMinutes>1440);
}
}
XeenServiceDayCandidate::XeenServiceDayCandidate(const XeenGameplayContext &context,
		const XeenServiceEconomy &economy,const XeenJourneyRandomState &cursor,
		XeenScriptServiceCharge serviceCharge,bool deferBankInterest,bool deferGeneration):
	originalContext(context),endingContext(context),originalEconomy(economy),endingEconomy(economy),
	originalRandom(cursor),random(cursor),charge(serviceCharge),bankInterestDeferred(deferBankInterest) {

	const auto successor=xeenPrepareServiceDeparture(context,static_cast<std::uint16_t>(charge));
	if(!successor)throw std::invalid_argument("Unsupported script service context");
	xeenValidateCurrentServiceEconomy(economy);endingContext=*successor;
	regenerating=serviceDepartureRegenerates(originalContext.day,endingContext.day,static_cast<std::uint16_t>(charge));
	generationDeferred=regenerating && deferGeneration;
	// 160 items, at most eight accepted requests each, spans <=101. A rejected
	// nonzero xorshift32 raw (<101) has a successor >100: at most two raws/request.
	if(generationDeferred && !randomRoom(cursor))throw std::overflow_error("Deferred service-day random capacity exhausted");
	completed=!regenerating || generationDeferred;
}
bool XeenServiceDayCandidate::service(unsigned budget,const std::function<void()> &check,
		const std::function<void()> &afterStock) {
	if(check)check();
	if(completed && !generationDeferred)return true;
	generationStarted=true;
	XeenConsequenceDraw draw{random,std::min(budget,64u),check};
	if(!stock.service(draw))return false;
	xeenValidateMerchantWares(stock.wares());
	if(check)check();
	try {if(afterStock)afterStock();}catch(...) {if(check)check();throw;}
	if(check)check();
	endingEconomy.wares=stock.wares();
	endingEconomy.bank=bankInterestDeferred?originalEconomy.bank:xeenPrepareBankInterest(originalEconomy.bank);
	generationDeferred=false;completed=true;return true;
}
XeenServiceDayCandidate XeenServiceDayCandidate::rebindPurchase(const XeenServiceEconomy &after,
		XeenInventoryCategory category,std::size_t slot,const XeenItem &expected) const {
	if(!completed)throw std::invalid_argument("Equipment Buy requires a complete purchase departure");
	validateReservation();
	xeenValidateEquipmentPurchaseEconomyDelta(originalEconomy,after,category,slot,expected);
	xeenValidateCurrentServiceEconomy(after);
	// Copy the completed random/generation state; never construct/service another
	// generation or recalculate interest. Triggered ending stock remains exact.
	auto replacement=*this;
	replacement.originalEconomy=after;
	if(!regenerating || generationDeferred)replacement.endingEconomy=after;
	replacement.validateReservation();
	return replacement;
}
XeenServiceDayCandidate XeenServiceDayCandidate::upgradeTemplePaid() const {
	if(charge!=XeenScriptServiceCharge::OneDay || !completed)
		throw std::invalid_argument("Temple upgrade requires a complete one-day reservation");
	validateReservation();
	auto paid=xeenPrepareTemplePaidDeparture(originalContext);
	if(!paid)throw std::invalid_argument("Temple two-day departure exceeds the supported year");
	if(!regenerating)return {originalContext,originalEconomy,originalRandom,
		XeenScriptServiceCharge::TemplePaid,false,true};
	auto replacement=*this;
	replacement.charge=XeenScriptServiceCharge::TemplePaid;
	replacement.endingContext=*paid;
	replacement.validateReservation();
	return replacement;
}
XeenServiceDayCandidate XeenServiceDayCandidate::addTemplePaidDay() const {
    validateReservation();
    if(generationStarted || (regenerating && !generationDeferred))
        throw std::logic_error("Temple payment followed departure generation");
    // DOS 0x1ae68 and 0x1d78e add into one unsigned word, including wrap.
    const auto next=static_cast<XeenScriptServiceCharge>(static_cast<std::uint16_t>(minutes()+1440));
    return {originalContext,originalEconomy,originalRandom,next,false,true};
}
XeenServiceDayCandidate XeenServiceDayCandidate::rebindContext(const XeenGameplayContext &after) const {
    validateReservation();
    auto expected=originalContext;expected.effects=after.effects;expected.lightAndResistances=after.lightAndResistances;
    if(!(expected==after) || generationStarted || (regenerating && !generationDeferred))
        throw std::logic_error("Service effect rebind changed calendar or followed generation");
    return {after,originalEconomy,originalRandom,charge,bankInterestDeferred,true};
}
void XeenServiceDayCandidate::validateComplete() const {
	validateReservation();
	if(generationDeferred)throw std::invalid_argument("Service-day generation is deferred");
}
void XeenServiceDayCandidate::validateReservation() const {
	if(!completed)throw std::invalid_argument("Service-day departure is incomplete");
	const auto expected=xeenPrepareServiceDeparture(originalContext,static_cast<std::uint16_t>(charge));
	if(!expected || !(endingContext==*expected) ||
		regenerating!=serviceDepartureRegenerates(originalContext.day,endingContext.day,static_cast<std::uint16_t>(charge)))
		throw std::invalid_argument("Service-day context successor changed");
	xeenValidateCurrentServiceEconomy(originalEconomy);
	if(generationDeferred) {
		if(endingEconomy!=originalEconomy || !regenerating)
			throw std::invalid_argument("Deferred service-day reservation changed");
		return;
	}
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
		if(endingEconomy.bank.gold!=(bankInterestDeferred?std::uint32_t(originalEconomy.bank.gold):interest(originalEconomy.bank.gold)) ||
			endingEconomy.bank.gems!=(bankInterestDeferred?std::uint32_t(originalEconomy.bank.gems):interest(originalEconomy.bank.gems)) ||
			ending.algorithm!=originalRandom.algorithm || !ending.state || ending.count<=originalRandom.count)
			throw std::invalid_argument("Triggering service-day successor changed");
	}
}
XeenServiceDayCandidate XeenServiceDayCandidate::rebindBank(const XeenServiceEconomy &after) const {
	validateReservation();
	if(!bankInterestDeferred || after.wares!=originalEconomy.wares)
		throw std::logic_error("Bank transfer changed stock or followed settlement");
	auto next=*this;
	next.originalEconomy.bank=after.bank;next.endingEconomy.bank=after.bank;
	next.validateReservation();return next;
}
XeenServiceDayCandidate XeenServiceDayCandidate::settleBank() const {
	validateReservation();auto next=*this;
	if(next.bankInterestDeferred) {
		if(regenerating && !generationDeferred)next.endingEconomy.bank=xeenPrepareBankInterest(originalEconomy.bank);
		next.bankInterestDeferred=false;
	}
	next.validateReservation();return next;
}
XeenServiceDayCandidate XeenServiceDayCandidate::rebindRandom(const XeenJourneyRandomState &cursor) const {
	validateReservation();
	if(generationStarted || (regenerating && !generationDeferred))
		throw std::logic_error("Service animation followed retained day generation");
	if(generationDeferred && !randomRoom(cursor))throw std::overflow_error("Service animation would consume reserved day RNG");
	auto next=*this;next.originalRandom=cursor;next.random=XeenCombatRandom(cursor);
	next.validateReservation();return next;
}

}
