#include "games/xeen/XeenEquipmentPurchase.h"
#include "games/xeen/XeenArmorRepair.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenCharacterRules.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
namespace {
// Numeric adaptation from ScummVM developers' GPL-3.0-or-later
// LangConstants::WEAPON_BASE_COSTS and ItemsDialog::calcItemCost, pinned at
// 6814ee9ba54582f5b5adcffab49efbbd8f589edd. See docs/dependencies.md for
// attribution and corresponding-source obligations. No modifier tables apply.
constexpr std::array<std::uint32_t,33> weaponCosts{{50,15,100,80,40,60,1,10,150,30,60,
    8,50,100,15,30,15,200,80,250,150,400,100,40,120,300,100,200,300,25,100,50,15}};
bool sameCategory(const XeenItemCategory &a,const XeenItemCategory &b) noexcept {
    for(unsigned i=0;i<9;++i)if(!xeenSameItem(a[i],b[i]))return false;
    return true;
}
}
bool xeenSupportedEquipmentOffer(unsigned side,unsigned shop,XeenInventoryCategory category,const XeenItem &item) noexcept {
    return side==0 && shop==0 && item.material==0 && item.state==0 && item.frame==0 && item.id>=1 &&
        ((category==XeenInventoryCategory::Weapons && item.id<=33) ||
        (category==XeenInventoryCategory::Armor && item.id<=13));
}
std::optional<std::uint32_t> xeenEquipmentPurchasePrice(XeenInventoryCategory category,const XeenItem &item) noexcept {
    if(!xeenSupportedEquipmentOffer(0,0,category,item))return {};
    const std::uint64_t base=category==XeenInventoryCategory::Weapons?weaponCosts[item.id-1]:kXeenArmorBaseCosts[item.id-1];
    return static_cast<std::uint32_t>(std::max<std::uint64_t>(1,base/1));
}
XeenEquipmentPurchaseResult xeenQuoteEquipmentPurchase(const XeenPartyState &party,std::size_t member,
        XeenInventoryCategory category,std::size_t slot,std::uint16_t content,unsigned side,unsigned shop) {
    if(content!=13)throw std::invalid_argument("Equipment Buy requires Journey content 13");
    xeenValidateJourneyParty(party,content);
    XeenEquipmentPurchaseResult r;r.category=category;
    r.goldBefore=r.goldAfter=party.monsterTreasure->gold;
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
        XeenInventoryCategory category,std::size_t slot,std::uint16_t content,unsigned side,unsigned shop) {
    XeenEquipmentPurchaseCandidate candidate;
    auto &r=candidate.result;r=xeenQuoteEquipmentPurchase(party,member,category,slot,content,side,shop);
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
    xeenValidateEquipmentPurchaseEconomyDelta(candidate.economyBefore,candidate.economyAfter,category,slot,r.offer);
    xeenValidateCurrentServiceEconomy(candidate.economyAfter,content);
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
        XeenInventoryCategory category,std::size_t slot,const XeenItem &expected) {
    const unsigned c=static_cast<unsigned>(category);
    if(c>=4 || slot>=8 || !xeenSupportedEquipmentOffer(0,0,category,expected) ||
        !xeenSameItem(before.wares[0][0][c][slot],expected))
        throw std::invalid_argument("Equipment Buy source preimage is invalid");
    auto depleted=before.wares[0][0][c];depleted[slot]={};xeenCompactItems(depleted);
    if(before.bank!=after.bank)throw std::invalid_argument("Equipment Buy changed bank balances");
    for(unsigned s=0;s<2;++s)for(unsigned p=0;p<4;++p)for(unsigned a=0;a<4;++a)
        if(!sameCategory(s==0 && p==0 && a==c?depleted:before.wares[s][p][a],after.wares[s][p][a]))
            throw std::invalid_argument("Equipment Buy changed an unauthorized merchant record");
}
}
