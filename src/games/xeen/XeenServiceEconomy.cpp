#include "games/xeen/XeenServiceEconomy.h"
#include "games/xeen/XeenMerchantTables.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
namespace {
bool within(unsigned value,merchant::Interval range,unsigned offset=0) noexcept { return value>=range.lo+offset && value<=range.hi+offset; }
bool empty(const XeenItem &i) noexcept { return !i.material && !i.id && !i.state && !i.frame; }
bool materialPossible(unsigned level,unsigned value) noexcept {
	if(level==1)return value==0;
	const auto column=level-2;
	if(within(value,merchant::material[0][column],36) || within(value,merchant::material[1][column],45))return true;
	for(unsigned row=0;row<6;++row)if(within(value,merchant::elements[row][column],merchant::elementOffsets[row]))return true;
	for(unsigned row=0;row<10;++row)if(within(value,merchant::attributes[row][column],58+merchant::attributeOffsets[row]))return true;
	return false;
}
constexpr unsigned index(unsigned w,unsigned a,unsigned x,unsigned m) noexcept { return ((w*9+a)*9+x)*9+m; }
}
bool xeenPossibleMerchantItem(unsigned level,unsigned category,const XeenItem &item) noexcept {
	if(level<1 || level>6 || category>3 || item.frame || !item.id)return false;
	if(category==3) {
		return level<6 && item.material>=1 && item.material<=9 && item.state>=1 && item.state<=8 &&
			within(item.id,merchant::specials[level-1]);
	}
	if(category==2 && level==1)return false;
	const unsigned maximum=category==0?(level==6?29:33):category==1?(level==1?7:13):10;
	if(item.id>maximum || (category==0?(item.state>6 || (level==1 && item.state)):bool(item.state)))return false;
	return materialPossible(level,item.material);
}
void xeenValidateMerchantWares(const XeenMerchantWares &wares) {
	const auto require=[](bool value) { if(!value)throw std::invalid_argument("Invalid generated merchant wares"); };
	// Two fixed sets of 9^4 consumed-prefix states. No draw, allocation,
	// history search or serialized generation metadata is involved.
	for(unsigned side=0;side<2;++side)for(unsigned shop=0;shop<4;++shop) {
		std::array<unsigned,4> lengths{};
		for(unsigned category=0;category<4;++category) {
			bool tail=false;
			for(unsigned slot=0;slot<9;++slot) {
				const auto &item=wares[side][shop][category][slot];
				if(!item.id) { require(empty(item));tail=true;continue; }
				require(!tail && slot<8 && !item.frame && !(item.state&0xc0));++lengths[category];
			}
		}
		std::array<bool,6561> current{},next{};current[0]=true;
		for(unsigned band=0;band<4;++band)for(unsigned call=0;call<merchant::counts[side][band][shop];++call) {
			next.fill(false);const auto level=merchant::level(side,shop,band);
			for(unsigned w=0;w<=lengths[0];++w)for(unsigned a=0;a<=lengths[1];++a)
			for(unsigned x=0;x<=lengths[2];++x)for(unsigned m=0;m<=lengths[3];++m) {
				std::array<unsigned,4> consumed{{w,a,x,m}};
				if(!current[index(w,a,x,m)])continue;
				for(unsigned category=0;category<4;++category) {
					const unsigned count=consumed[category];
					if(count==8) { if((category!=2 || level>1) && (category!=3 || level<6))next[index(w,a,x,m)]=true; }
					else if(count<lengths[category] && xeenPossibleMerchantItem(level,category,wares[side][shop][category][count])) {
						++consumed[category];next[index(consumed[0],consumed[1],consumed[2],consumed[3])]=true;--consumed[category];
					}
				}
			}
			current=next;
		}
		require(current[index(lengths[0],lengths[1],lengths[2],lengths[3])]);
	}
}
std::uint32_t xeenBankInterest(std::uint32_t balance) noexcept { return static_cast<std::uint32_t>(std::uint64_t(balance)+balance/100); }
XeenBankBalances xeenPrepareBankInterest(const XeenBankBalances &before) noexcept {
	XeenBankBalances after;after.gold=xeenBankInterest(before.gold);after.gems=xeenBankInterest(before.gems);return after;
}
}
