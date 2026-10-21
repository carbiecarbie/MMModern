#include "games/xeen/XeenEquipmentPurchase.h"
#include "games/xeen/XeenArmorRepair.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
namespace {
bool sameCategory(const XeenItemCategory &a,const XeenItemCategory &b) noexcept {
    for(unsigned i=0;i<9;++i)if(!xeenSameItem(a[i],b[i]))return false;
    return true;
}
}
unsigned xeenIronworksShop(XeenMapIdentity map) noexcept {
    if(map.side!=XeenSide::Clouds)return 0;
    switch(static_cast<unsigned>(map.number)) {
    case 30:return 1;case 73:return 2;case 49:return 3;default:return 0;
    }
}
bool xeenSupportedEquipmentOffer(unsigned side,unsigned shop,XeenInventoryCategory category,const XeenItem &item) noexcept {
    return side==0 && shop<4 && item.frame==0 &&
        !(category==XeenInventoryCategory::Weapons && item.id>=34) &&
        xeenItemPrice(category,item,XeenItemService::Buy).has_value();
}
std::optional<std::uint32_t> xeenEquipmentPurchasePrice(XeenInventoryCategory category,const XeenItem &item) noexcept {
    if(!xeenSupportedEquipmentOffer(0,0,category,item))return {};
    return xeenItemPrice(category,item,XeenItemService::Buy);
}
XeenEquipmentPurchaseResult xeenQuoteEquipmentPurchase(const XeenPartyState &party,std::size_t member,
        XeenInventoryCategory category,std::size_t slot,unsigned side,unsigned shop) {

    xeenValidateJourneyParty(party);
    XeenEquipmentPurchaseResult r;r.category=category;
    r.goldBefore=r.goldAfter=party.purse->gold;
    if(member>=party.party.size()){r.outcome=XeenEquipmentPurchaseOutcome::InvalidParticipant;return r;}
    if(static_cast<unsigned>(category)>=4){r.outcome=XeenEquipmentPurchaseOutcome::InvalidCategory;return r;}
    if(slot>=9){r.outcome=XeenEquipmentPurchaseOutcome::InvalidSlot;return r;}
    if(side>=2 || shop>=4)return r;
    r.side=static_cast<std::uint8_t>(side);r.shop=static_cast<std::uint8_t>(shop);
    r.member=static_cast<std::uint8_t>(member);r.owner=party.party.activeRosterIds()[member];
    std::copy(party.party.activeRosterIds().begin(),party.party.activeRosterIds().end(),r.activeRosterIds.begin());
    r.offerSlot=static_cast<std::uint8_t>(slot);
    r.stockBefore=r.stockAfter=party.serviceEconomy->wares[side][shop][static_cast<unsigned>(category)];
    r.recipientBefore=r.recipientAfter=*xeenInventoryItems(party.roster.at(r.owner),category);
    r.offer=r.stockBefore[slot];
    if(!r.offer.id){r.outcome=XeenEquipmentPurchaseOutcome::Empty;return r;}
    if(!xeenSupportedEquipmentOffer(side,shop,category,r.offer))return r;
    // Original Buy refuses a full physical tail before confirmation or money.
    if(!xeenItemHasTailCapacity(r.recipientBefore)){r.outcome=XeenEquipmentPurchaseOutcome::DestinationFull;return r;}
    r.price=*xeenEquipmentPurchasePrice(category,r.offer);
    if(r.goldBefore>=r.price)r.goldAfter=r.goldBefore-r.price;
    else r.shortfall=r.price-r.goldBefore;
    r.outcome=XeenEquipmentPurchaseOutcome::Quoted;
    return r;
}
XeenEquipmentPurchaseCandidate xeenPrepareEquipmentPurchase(const XeenPartyState &party,std::size_t member,
        XeenInventoryCategory category,std::size_t slot,unsigned side,unsigned shop) {
    XeenEquipmentPurchaseCandidate candidate;
    auto &r=candidate.result;r=xeenQuoteEquipmentPurchase(party,member,category,slot,side,shop);
    candidate.economyBefore=candidate.economyAfter=*party.serviceEconomy;
    if(r.outcome!=XeenEquipmentPurchaseOutcome::Quoted)return candidate;
    if(r.goldBefore<r.price){r.outcome=XeenEquipmentPurchaseOutcome::InsufficientGold;return candidate;}
    unsigned occupied=0;
    for(const auto &item:r.recipientBefore)if(item.id)++occupied;
    r.recipientSlot=static_cast<std::uint8_t>(occupied);
    auto delivered=r.offer;delivered.frame=0;
    r.recipientAfter.back()=delivered;xeenCompactItems(r.recipientAfter);
    r.stockAfter[slot]={};xeenCompactItems(r.stockAfter);
    candidate.economyAfter.wares[side][shop][static_cast<unsigned>(category)]=r.stockAfter;
    xeenValidateEquipmentPurchaseEconomyDelta(candidate.economyBefore,candidate.economyAfter,category,slot,r.offer,side,shop);
    xeenValidateCurrentServiceEconomy(candidate.economyAfter);
    // All other party fields are unchanged and the original party passed full
    // admission. Validate the only changed character without copying a marked
    // roster or obtaining whole-owner replacement authority.
    auto recipient=party.roster.at(r.owner);
    *xeenInventoryItems(recipient,category)=r.recipientAfter;
    XeenCharacterRules::validateForUse(recipient,{party.encounterContext->year});
    xeenValidateCompletedEquipment(recipient);
    r.outcome=XeenEquipmentPurchaseOutcome::Purchased;
    return candidate;
}
void xeenValidateEquipmentPurchaseEconomyDelta(const XeenServiceEconomy &before,const XeenServiceEconomy &after,
        XeenInventoryCategory category,std::size_t slot,const XeenItem &expected,unsigned side,unsigned shop) {
    const unsigned c=static_cast<unsigned>(category);
    if(c>=4 || slot>=8 || side>=2 || shop>=4 || !xeenSupportedEquipmentOffer(side,shop,category,expected) ||
        !xeenSameItem(before.wares[side][shop][c][slot],expected))
        throw std::invalid_argument("Equipment Buy source preimage is invalid");
    auto depleted=before.wares[side][shop][c];depleted[slot]={};xeenCompactItems(depleted);
    if(before.bank!=after.bank)throw std::invalid_argument("Equipment Buy changed bank balances");
    for(unsigned s=0;s<2;++s)for(unsigned p=0;p<4;++p)for(unsigned a=0;a<4;++a)
        if(!sameCategory(s==side && p==shop && a==c?depleted:before.wares[s][p][a],after.wares[s][p][a]))
            throw std::invalid_argument("Equipment Buy changed an unauthorized merchant record");
}
}
