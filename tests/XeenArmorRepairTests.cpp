#include "games/xeen/XeenArmorRepair.h"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool value, const char *message) {
	if (!value) throw std::runtime_error(message);
}
bool sameItem(const XeenItem &a, const XeenItem &b) {
	return a.material==b.material && a.id==b.id && a.state==b.state && a.frame==b.frame;
}
void unchanged(const XeenArmorRepairCandidate &c, const XeenItem &item, std::uint32_t gold) {
	check(sameItem(c.before,item) && sameItem(c.after,item), "refusal/quote changed raw item");
	check(c.goldBefore==gold && c.goldAfter==gold, "refusal/quote changed purse");
}
void pricesAndFunds() {
	// Independent literal expectations from the accepted M38 price table.
	constexpr unsigned prices[2][13]={{2,10,20,40,60,100,200,10,6,4,25,20,10},
		{1,2,5,10,15,25,50,2,1,1,6,5,2}};
	for (unsigned material=0; material<2; ++material) for (unsigned id=1; id<=13; ++id) {
		const XeenItem item{static_cast<std::uint8_t>(material ? 38 : 0),static_cast<std::uint8_t>(id),128,9};
		const auto price=prices[material][id-1];
		for (const std::uint32_t gold : {0u,price-1,price,price+1,0x7fffffffU,0x80000000U,0xffffffffU}) {
			const auto quote=xeenQuoteArmorRepair(XeenInventoryCategory::Armor,item,gold);
			check(quote.outcome==XeenArmorRepairOutcome::Quoted && quote.price==price,"wrong exact quote or funds tested before confirmation");
			unchanged(quote,item,gold);
			const auto repair=xeenPrepareArmorRepair(item,gold);
			check(repair.price==price && sameItem(repair.before,item) && repair.goldBefore==gold,"repair changed quote/preimage");
			if (gold<price) {
				check(repair.outcome==XeenArmorRepairOutcome::InsufficientGold,"short funds accepted");
				unchanged(repair,item,gold);
			} else {
				check(repair.outcome==XeenArmorRepairOutcome::Repaired && repair.goldAfter==gold-price,"full-u32/exact payment wrong");
				auto expected=item; expected.state=0;
				check(sameItem(repair.after,expected),"repair changed nonbroken bytes");
				const auto repeat=xeenPrepareArmorRepair(repair.after,repair.goldAfter);
				check(repeat.outcome==XeenArmorRepairOutcome::Intact,"repeat repair accepted");
				unchanged(repeat,repair.after,repair.goldAfter);
			}
			check(item.state==128,"pure rule mutated caller item");
		}
	}
}
void eligibilityAndRawBytes() {
	for (unsigned material=0; material<256; ++material) for (unsigned id=0; id<256; ++id)
		for (unsigned state : {0u,127u,128u,255u}) {
			const XeenItem item{static_cast<std::uint8_t>(material),static_cast<std::uint8_t>(id),static_cast<std::uint8_t>(state),255};
			const auto expected=id==0 ? XeenArmorRepairOutcome::Empty :
				(id>13 || (material!=0 && material!=38)) ? XeenArmorRepairOutcome::Unsupported :
				state<128 ? XeenArmorRepairOutcome::Intact : XeenArmorRepairOutcome::Quoted;
			const auto quote=xeenQuoteArmorRepair(XeenInventoryCategory::Armor,item,0);
			check(quote.outcome==expected,"category/empty/domain/broken refusal order wrong");
			unchanged(quote,item,0);
			if (expected!=XeenArmorRepairOutcome::Quoted) {
				check(quote.price==0,"refused record received a price");
				const auto repair=xeenPrepareArmorRepair(item,0);
				check(repair.outcome==expected,"funds refusal masked record refusal");
				unchanged(repair,item,0);
			}
		}
	for (unsigned category=0; category<256; ++category) {
		if (category==static_cast<unsigned>(XeenInventoryCategory::Armor)) continue;
		for (const XeenItem item : {XeenItem{255,0,255,255}, XeenItem{0,1,128,9}, XeenItem{0,1,0,9}}) {
			const auto quote=xeenQuoteArmorRepair(static_cast<XeenInventoryCategory>(category),item,0xffffffffU);
			check(quote.outcome==XeenArmorRepairOutcome::Unsupported && quote.price==0,"wrong category admitted or checked too late");
			unchanged(quote,item,0xffffffffU);
		}
	}
	for (unsigned material : {0u,38u}) for (unsigned id=1; id<=13; ++id)
		for (unsigned lower=0; lower<128; ++lower) for (unsigned frame=0; frame<256; ++frame) {
			const XeenItem item{static_cast<std::uint8_t>(material),static_cast<std::uint8_t>(id),
				static_cast<std::uint8_t>(128+lower),static_cast<std::uint8_t>(frame)};
			const auto repaired=xeenPrepareArmorRepair(item,1000);
			auto expected=item; expected.state=static_cast<std::uint8_t>(lower);
			check(repaired.outcome==XeenArmorRepairOutcome::Repaired && sameItem(repaired.before,item) &&
				sameItem(repaired.after,expected),"repair normalized curse/counter/frame or other raw bytes");
		}
}
void departure() {
	XeenGameplayContext canonical;
	canonical.day=8;canonical.year=610;canonical.minutes=584;canonical.ctr24=17;
	for (unsigned day : {8u,9u}) for (unsigned minute : {300u,584u,1259u}) for (unsigned ctr : {0u,17u,23u}) {
		auto before=canonical;before.day=day;before.minutes=minute;before.ctr24=ctr;
		const auto original=before;
		const auto after=xeenPrepareSmithDeparture(before,9);
		auto expected=before;expected.day=day+1;
		check(after && *after==expected && before==original,"departure changed fields other than day or mutated input");
	}
	for (unsigned content=0; content<12; ++content) if (content!=9)
		check(!xeenPrepareSmithDeparture(canonical,content),"legacy/unknown content received smith day jump");
	for (unsigned day : {0u,1u,7u,10u,11u,99u,65535u}) {
		auto before=canonical;before.day=day;
		check(!xeenPrepareSmithDeparture(before,9),"unsupported day admitted");
	}
	for (unsigned mode=0; mode<24; ++mode) {
		auto before=canonical;
		if (mode==0) before.year=609;
		else if (mode==1) before.year=611;
		else if (mode==2) before.minutes=299;
		else if (mode==3) before.minutes=1260;
		else if (mode==4) before.ctr24=24;
		else if (mode==5) before.rested=true;
		else if (mode==6) before.newDay=true;
		else if (mode==7) before.difficulty=XeenDifficulty::Warrior;
		else if (mode==8) before.profile=static_cast<XeenBehaviorProfile>(255);
		else if (mode<18) before.effects[mode-9]=1;
		else before.lightAndResistances[mode-18]=1;
		const auto original=before;
		check(!xeenPrepareSmithDeparture(before,9) && before==original,"invalid context accepted or modified");
	}
}
}
int main() {
	try {
		pricesAndFunds();eligibilityAndRawBytes();departure();
		std::cout << "M38 pure armor repair and bounded departure rules passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';return 1;
	}
}
