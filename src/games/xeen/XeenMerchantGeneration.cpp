#include "games/xeen/XeenMerchantGeneration.h"
#include "games/xeen/XeenMerchantTables.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
XeenMerchantItemCandidate::XeenMerchantItemCandidate(unsigned value):level(value) {
	if(level<1 || level>6)throw std::invalid_argument("Unsupported merchant item level");
}
bool XeenMerchantItemCandidate::service(XeenConsequenceDraw &draw) {
	draw.remaining=std::min(draw.remaining,64u);
	while(step!=Step::Done && draw.remaining) switch(step) {
	case Step::Category: { const auto n=draw.draw(0,100);if(n) { categoryRoll=*n;step=Step::Subcategory; } break; }
	case Step::Subcategory: { const auto n=draw.draw(0,level==6?80:100);if(n) { subcategory=*n;step=Step::Resolve; } break; }
	case Step::Resolve:
		if(categoryRoll<=(level==1?40:35)) {
			selectedCategory=0;lo=subcategory<=30?1:subcategory<=60?7:subcategory<=85?18:30;
			hi=subcategory<=30?6:subcategory<=60?17:subcategory<=85?29:33;step=Step::Id;
		} else if(level==1) { selectedCategory=categoryRoll<=85?1:3;lo=1;hi=selectedCategory==1?7:9;step=Step::Id; }
		else if(categoryRoll<=60) {
			selectedCategory=1;
			if(subcategory>70) { generated.id=8;step=Step::Enchantment; }else {lo=1;hi=7;step=Step::Id;}
		} else {
			step=Step::Enchantment;
			if(subcategory<=10) {selectedCategory=1;generated.id=9;}
			else if(subcategory<=20) {selectedCategory=1;generated.id=13;}
			else if(subcategory<=35) {selectedCategory=2;generated.id=1;}
			else if(subcategory<=45) {selectedCategory=1;generated.id=10;}
			else if(subcategory<=55) {selectedCategory=1;lo=11;hi=12;step=Step::Id;}
			else if(subcategory<=65) {selectedCategory=2;generated.id=2;}
			else if(subcategory<=75) {selectedCategory=2;lo=3;hi=7;step=Step::Id;}
			else if(subcategory<=80) {selectedCategory=2;lo=8;hi=10;step=Step::Id;}
			else {selectedCategory=3;lo=1;hi=9;step=Step::Id;}
		}
		break;
	case Step::Id: { const auto n=draw.draw(lo,hi);if(n) { generated.id=static_cast<std::uint8_t>(*n);step=Step::Enchantment; }break; }
	case Step::Enchantment: {
		const auto n=draw.draw(1,100);if(!n)break;
		if(selectedCategory==3) { generated.material=generated.id;step=Step::Special; }
		else if(level==1)step=Step::Done;
		else {
			enchantment=selectedCategory==2?(*n<=20?Enchantment::Material:*n<=60?Enchantment::Element:Enchantment::Attribute):
				(*n<=70?Enchantment::Material:*n<=98?Enchantment::Element:Enchantment::Attribute);
			step=Step::Selector;
		}
		break;
	}
	case Step::Selector: {
		const auto n=draw.draw(1,100);if(!n)break;
		merchant::Interval interval{};
		if(enchantment==Enchantment::Material) { const unsigned row=*n<=70?0:1;interval=merchant::material[row][level-2];offset=36+(row?9:0); }
		else if(enchantment==Enchantment::Element) { unsigned row=0;while(*n>merchant::elementThresholds[row])++row;interval=merchant::elements[row][level-2];offset=merchant::elementOffsets[row]; }
		else { unsigned row=0;while(*n>merchant::attributeThresholds[row])++row;interval=merchant::attributes[row][level-2];offset=58+merchant::attributeOffsets[row]; }
		lo=interval.lo;hi=interval.hi;step=Step::Value;break;
	}
	case Step::Value: { const auto n=draw.draw(lo,hi);if(n) { generated.material=static_cast<std::uint8_t>(*n+offset);step=selectedCategory==0?Step::Effectiveness:Step::Done; }break; }
	case Step::Effectiveness: { const auto n=draw.draw(0,20);if(n)step=*n==10?Step::EffectivenessValue:Step::Done;break; }
	case Step::EffectivenessValue: { const auto n=draw.draw(1,6);if(n) { generated.state=static_cast<std::uint8_t>(*n);step=Step::Done; }break; }
	case Step::Special: { const auto interval=merchant::specials[level-1];const auto n=draw.draw(interval.lo,interval.hi);if(n) {generated.id=static_cast<std::uint8_t>(*n);step=Step::Charges;}break; }
	case Step::Charges: { const auto n=draw.draw(1,8);if(n) {generated.state=static_cast<std::uint8_t>(*n);step=Step::Done;}break; }
	case Step::Done: break;
	}
	return complete();
}
bool XeenMerchantStockCandidate::service(XeenConsequenceDraw &draw) {
	draw.remaining=std::min(draw.remaining,64u);
	while(!complete()) {
		if(!item) {
			while(ordinal==merchant::counts[side][band][shop]) {
				ordinal=0;
				if(++band==4) { band=0;counts.fill(0);if(++shop==4) {shop=0;if(++side==2)return true;} }
			}
			item.emplace(merchant::level(side,shop,band));
		}
		if(!item->service(draw))return false;
		const auto category=item->category();
		if(counts[category]<8)generated[side][shop][category][counts[category]++]=item->item();else ++discarded;
		++ordinal;++totalItems;item.reset();
	}
	return true;
}
}
