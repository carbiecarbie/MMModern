#include "games/xeen/XeenMerchantGeneration.h"
#include "games/xeen/XeenMerchantTables.h"
#include <algorithm>
namespace mmodern {
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
