#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/XeenArmorRepair.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
XeenServiceDayCandidate::XeenServiceDayCandidate(const XeenGameplayContext &context,
		const XeenServiceEconomy &economy,const XeenJourneyRandomState &cursor,std::uint16_t content):
	originalContext(context),endingContext(context),originalEconomy(economy),endingEconomy(economy),
	originalRandom(cursor),random(cursor) {
	if(content!=11)throw std::invalid_argument("Service economy requires Journey content 11");
	const auto successor=xeenPrepareSmithDeparture(context,content);
	if(!successor)throw std::invalid_argument("Unsupported one-day script service context");
	xeenValidateServiceEconomy(economy);endingContext=*successor;
	regenerating=xeenServiceDayRegenerates(originalContext.day,endingContext.day,1440);
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
}
