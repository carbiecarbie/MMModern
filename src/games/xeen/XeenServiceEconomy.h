#ifndef MMODERN_XEEN_SERVICE_ECONOMY_H
#define MMODERN_XEEN_SERVICE_ECONOMY_H
#include "games/xeen/XeenCharacter.h"
#include <array>
#include <cstdint>
namespace mmodern {
// Fixed physical storage: numeric side, shop, category, physical item slot.
// Item scalar writes join the party's existing observation range immediately.
struct XeenMerchantWares {
	using Shop = std::array<XeenItemCategory,4>;
	using Side = std::array<Shop,4>;
	std::array<Side,2> records{};
	Side &operator[](std::size_t side) { return records.at(side); }
	const Side &operator[](std::size_t side) const { return records.at(side); }
	auto begin() noexcept { return records.begin(); } auto end() noexcept { return records.end(); }
	auto begin() const noexcept { return records.begin(); } auto end() const noexcept { return records.end(); }
	friend bool operator==(const XeenMerchantWares &a,const XeenMerchantWares &b) noexcept {
		for(unsigned s=0;s<2;++s) for(unsigned p=0;p<4;++p) for(unsigned c=0;c<4;++c) for(unsigned i=0;i<9;++i) {
			const auto &x=a[s][p][c][i],&y=b[s][p][c][i];
			if(x.material!=y.material || x.id!=y.id || x.state!=y.state || x.frame!=y.frame) return false;
		}
		return true;
	}
	friend bool operator!=(const XeenMerchantWares &a,const XeenMerchantWares &b) noexcept { return !(a==b); }
};
struct XeenBankBalances {
	XeenMutable<std::uint32_t> gold=0,gems=0;
	friend bool operator==(const XeenBankBalances &a,const XeenBankBalances &b) noexcept { return a.gold==b.gold && a.gems==b.gems; }
	friend bool operator!=(const XeenBankBalances &a,const XeenBankBalances &b) noexcept { return !(a==b); }
};
struct XeenServiceEconomy {
	XeenMerchantWares wares;
	XeenBankBalances bank;
	friend bool operator==(const XeenServiceEconomy &a,const XeenServiceEconomy &b) noexcept { return a.wares==b.wares && a.bank==b.bank; }
	friend bool operator!=(const XeenServiceEconomy &a,const XeenServiceEconomy &b) noexcept { return !(a==b); }
};
bool xeenPossibleMerchantItem(unsigned level,unsigned category,const XeenItem &) noexcept;
void xeenValidateMerchantWares(const XeenMerchantWares &);
inline void xeenValidateServiceEconomy(const XeenServiceEconomy &v) { xeenValidateMerchantWares(v.wares); }
// Current-state admission is content-specific. Complete generation remains a
// separate, stricter contract for fresh initialization and every restock.
// Content 13 proves existence of a permitted generated predecessor and plain
// W/A deletions; it does not establish historical purchases or payment.
void xeenValidateCurrentServiceEconomy(const XeenServiceEconomy &,std::uint16_t content);
std::uint32_t xeenBankInterest(std::uint32_t balance) noexcept;
XeenBankBalances xeenPrepareBankInterest(const XeenBankBalances &) noexcept;
}
#endif
