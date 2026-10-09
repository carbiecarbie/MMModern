#include "XeenTestInstallation.h"
#include "games/xeen/XeenEquipmentPurchase.h"
#include "games/xeen/XeenServiceDay.h"
#include "XeenTrainingTestSupport.h"
#include <iostream>
#include <limits>
#include <fstream>
#include <filesystem>
#include <regex>
using namespace mmodern;
using save_test::check;
using save_test::rejects;
namespace {
using Outcome=XeenEquipmentPurchaseOutcome;
void sameCategory(const XeenItemCategory &a,const XeenItemCategory &b,const char *message) {
    for(unsigned i=0;i<9;++i)check(xeenSameItem(a[i],b[i]),message);
}
// An independent vector erase/append oracle, without the production compactor.
XeenItemCategory removeOracle(const XeenItemCategory &before,unsigned removed) {
    std::vector<XeenItem> retained;
    for(unsigned i=0;i<9;++i)if(i!=removed && before[i].id)retained.push_back(before[i]);
    XeenItemCategory expected{};
    std::copy(retained.begin(),retained.end(),expected.begin());return expected;
}
void prices() {
    constexpr unsigned weapons[]={50,15,100,80,40,60,1,10,150,30,60,8,50,100,15,30,15,200,80,250,150,400,100,40,120,300,100,200,300,25,100,50,15};
    constexpr unsigned armor[]={20,100,200,400,600,1000,2000,100,60,40,250,200,100};
    for(unsigned id=1;id<=33;++id)
        check(xeenEquipmentPurchasePrice(XeenInventoryCategory::Weapons,{0,std::uint8_t(id),0,0})==weapons[id-1],"weapon base-cost oracle mismatch");
    for(unsigned id=1;id<=13;++id)
        check(xeenEquipmentPurchasePrice(XeenInventoryCategory::Armor,{0,std::uint8_t(id),0,0})==armor[id-1],"armor base-cost oracle mismatch");
    for(unsigned c=0;c<4;++c)for(unsigned id:{0u,1u,13u,14u,33u,34u,255u})
    for(unsigned material:{0u,1u,37u,38u,40u,255u})for(unsigned state:{0u,1u,63u,64u,128u})for(unsigned frame:{0u,1u,3u,13u}) {
        const XeenItem item{std::uint8_t(material),std::uint8_t(id),std::uint8_t(state),std::uint8_t(frame)};
        const bool expected=material==0 && state==0 && frame==0 && id>=1 && ((c==0 && id<=33) || (c==1 && id<=13));
        const auto category=static_cast<XeenInventoryCategory>(c);
        check(xeenSupportedEquipmentOffer(0,0,category,item)==expected,"plain offer domain differs");
        check(bool(xeenEquipmentPurchasePrice(category,item))==expected,"excluded modifier/category gained a price");
        check(!xeenSupportedEquipmentOffer(1,0,category,item) && !xeenSupportedEquipmentOffer(0,1,category,item),"other site gained Buy");
    }
}
void pinnedReference(const std::filesystem::path &source) {
    const auto read=[](const std::filesystem::path &path){std::ifstream input(path,std::ios::binary);
        check(bool(input),"pinned reference input unavailable");return std::string(std::istreambuf_iterator<char>(input),{});};
    const auto constants=read(source/"devtools/create_mm/create_xeen/constants.cpp");
    const auto parse=[&](const char *name,unsigned extent) {
        const auto start=constants.find(std::string("LangConstants::")+name+"[");check(start!=std::string::npos,"pinned numeric table missing");
        const auto open=constants.find('{',start),close=constants.find('}',open);check(open!=std::string::npos && close!=std::string::npos,"pinned numeric extent missing");
        const auto body=constants.substr(open+1,close-open-1);const std::regex number("[0-9]+");std::vector<unsigned> values;
        for(auto i=std::sregex_iterator(body.begin(),body.end(),number);i!=std::sregex_iterator();++i)values.push_back(std::stoul(i->str()));
        check(values.size()==extent,"pinned numeric extent differs");return values;
    };
    const auto divisors=parse("ITEM_SKILL_DIVISORS",4);check(divisors==std::vector<unsigned>{1,2,100,10},"pinned Buy divisor changed");
    const auto weapons=parse("WEAPON_BASE_COSTS",35),armor=parse("ARMOR_BASE_COSTS",14);
    for(unsigned category=0;category<2;++category)for(unsigned id=1;id<=(category?13u:33u);++id) {
        const auto expected=std::max(1u,(category?armor:weapons)[id]/divisors[0]);
        check(xeenEquipmentPurchasePrice(static_cast<XeenInventoryCategory>(category),{0,std::uint8_t(id),0,0})==expected,"pinned Buy base-cost/divisor reference differs");
    }
    const auto dialog=read(source/"engines/mm/xeen/dialogs/dialogs_items.cpp");
    const auto cost=dialog.find("int ItemsDialog::calcItemCost");check(cost!=std::string::npos,"pinned Buy cost function missing");
    check(dialog.find("case ITEMMODE_BUY:\n\t\tlevel = 0;",cost)!=std::string::npos ||
        dialog.find("case ITEMMODE_BUY:\r\n\t\tlevel = 0;",cost)!=std::string::npos,"pinned Buy stopped forcing divisor zero");
}
void preparedRules(training_test::Inputs &inputs) {
    auto source=inputs.base();
    training_test::Fixture fixture(inputs,source);
    auto &p=fixture.p;const auto initialEconomy=*p.serviceEconomy;
    const auto initialCharacters=p.roster.characters();const auto initialContext=*p.encounterContext;
    constexpr XeenItemCategory literalWeapons{{{0,10,0,0},{0,6,0,0},{0,15,0,0},{0,10,0,0},{0,6,0,0},{0,4,0,0},{37,20,0,0},{40,16,0,0},{}}};
    constexpr XeenItemCategory literalArmor{{{0,6,0,0},{0,4,0,0},{0,6,0,0},{0,3,0,0},{0,5,0,0},{40,8,0,0},{48,6,0,0},{},{}}};
    sameCategory(initialEconomy.wares[0][0][0],literalWeapons,"seed-7 literal weapons differ");
    sameCategory(initialEconomy.wares[0][0][1],literalArmor,"seed-7 literal armor differs");
    check(source.journey->random==XeenJourneyRandomState{1,1652828136u,901},"seed-7 generation cursor differs");
    p.monsterTreasure->gold=10000;
    for(unsigned c=0;c<2;++c)for(unsigned slot=0;slot<(c?7u:8u);++slot) {
        const auto category=static_cast<XeenInventoryCategory>(c);
        const auto record=initialEconomy.wares[0][0][c][slot];
        const auto candidate=xeenPrepareEquipmentPurchase(p,0,category,slot);
        if(record.material){check(candidate.result.outcome==Outcome::Unsupported,"modified actual offer purchased");continue;}
        const auto &r=candidate.result;
        check(r.outcome==Outcome::Purchased,"actual plain physical offer refused");
        if(r.outcome!=Outcome::Purchased)continue;
        check(r.owner==0 && r.member==0 && r.offerSlot==slot && xeenSameItem(r.offer,record) &&
            r.activeRosterIds==std::array<std::uint8_t,6>{0,18,14,11,1,6},"physical/member identity missing");
        sameCategory(r.stockAfter,removeOracle(initialEconomy.wares[0][0][c],slot),"every physical removal-position oracle differs");
        check(candidate.economyBefore==initialEconomy && candidate.economyAfter.bank==initialEconomy.bank,"candidate changed bank/preimage");
        xeenValidateEquipmentPurchaseEconomyDelta(initialEconomy,candidate.economyAfter,category,slot,record);
    }
    for(auto category:{XeenInventoryCategory::Accessories,XeenInventoryCategory::Miscellaneous}) {
        const auto r=xeenQuoteEquipmentPurchase(p,0,category,0);
        check(r.outcome==Outcome::Unsupported && r.price==0,"unsupported actual category invented price");
    }
    check(xeenQuoteEquipmentPurchase(p,0,XeenInventoryCategory::Armor,8).outcome==Outcome::Empty,"empty physical row not distinguished");
    check(xeenQuoteEquipmentPurchase(p,6,XeenInventoryCategory::Weapons,0).outcome==Outcome::InvalidParticipant,"inactive recipient admitted");
    check(xeenQuoteEquipmentPurchase(p,0,static_cast<XeenInventoryCategory>(255),0).outcome==Outcome::InvalidCategory,"invalid category admitted");
    check(xeenQuoteEquipmentPurchase(p,0,XeenInventoryCategory::Weapons,9).outcome==Outcome::InvalidSlot,"invalid physical slot admitted");
    check(xeenQuoteEquipmentPurchase(p,0,XeenInventoryCategory::Weapons,1).outcome==Outcome::Quoted,
        "content-14 physical Buy offer refused");
    for(unsigned price:{60u,200u})for(int delta:{-1,0,1}) {
        p.monsterTreasure->gold=price+delta;
        const auto category=price==60?XeenInventoryCategory::Weapons:XeenInventoryCategory::Armor;
        const unsigned slot=price==60?1:3;
        const auto quote=xeenQuoteEquipmentPurchase(p,0,category,slot);
        const auto candidate=xeenPrepareEquipmentPurchase(p,0,category,slot);
        check(quote.outcome==Outcome::Quoted && quote.price==price && quote.shortfall==(delta<0?1u:0u) &&
            quote.goldAfter==(delta<0?price-1:unsigned(delta)),"funds quote underflow/shortfall differs");
        check(candidate.result.outcome==(delta<0?Outcome::InsufficientGold:Outcome::Purchased) &&
            candidate.result.goldAfter==(delta<0?price-1:unsigned(delta)),"59/60/61 or 199/200/201 funds differ");
    }
    p.monsterTreasure->gold=UINT32_MAX;
    check(xeenPrepareEquipmentPurchase(p,0,XeenInventoryCategory::Weapons,1).result.goldAfter==4294967235u,"u32-high purse was narrowed");
    p.monsterTreasure->gold=0;p.monsterTreasure->gems=UINT32_MAX;p.serviceEconomy->bank={UINT32_MAX,UINT32_MAX};
    check(xeenPrepareEquipmentPurchase(p,0,XeenInventoryCategory::Weapons,1).result.outcome==Outcome::InsufficientGold,"gems/bank paid Buy");
    p.monsterTreasure->gold=59;p.monsterTreasure->pendingGold=10;p.monsterTreasure->pendingMask=1;
    check(xeenPrepareEquipmentPurchase(p,0,XeenInventoryCategory::Weapons,1).result.outcome==Outcome::InsufficientGold,"pending treasure paid Buy");
    p.monsterTreasure->pendingGold=0;p.monsterTreasure->pendingMask=0;
    p.monsterTreasure->gold=10000;
    auto &recipient=p.roster.at(0);recipient.weapons={};
    recipient.weapons[2]={0,7,0,0};recipient.weapons[6]={0,12,0,0};
    recipient.weapons[0]={255,0,255,0};recipient.armor[8]={241,0,242,0};
    recipient.currentHp=30000;recipient.currentSp=30000;recipient.conditions[8]=1;
    for(auto &item:recipient.armor)item.frame=0;
    recipient.characterClass=XeenCharacterClass::Sorcerer;
    {
        XeenMutationWatch watch;watch.add(&p,sizeof(p));
        const auto candidate=xeenPrepareEquipmentPurchase(p,0,XeenInventoryCategory::Weapons,1);
        const XeenItemCategory expected{{{0,7,0,0},{0,12,0,0},{0,6,0,0},{},{},{},{},{},{}}};
        check(candidate.result.outcome==Outcome::Purchased && candidate.result.recipientSlot==2 && watch.current(),"condition/proficiency/above-maximum changed Buy or live owners");
        sameCategory(candidate.result.recipientAfter,expected,"free tail with holes did not compact/append exact bytes");
        check(recipient.armor[8].material==241 && recipient.armor[8].state==242 && recipient.currentHp==30000 && recipient.currentSp==30000,
            "candidate normalized untouched metadata/current stats");
    }
    recipient.weapons[8]={0,15,0,0};p.monsterTreasure->gold=0;
    const auto full=xeenQuoteEquipmentPurchase(p,0,XeenInventoryCategory::Weapons,1);
    check(full.outcome==Outcome::DestinationFull && full.price==0 && full.goldAfter==0,"tail/full-before-funds precedence differs");
    // All pure operations above retain stock, all nonselected owners and time.
    check(*p.serviceEconomy==XeenServiceEconomy{initialEconomy.wares,{UINT32_MAX,UINT32_MAX}} && *p.encounterContext==initialContext,"pure candidate changed stock/context");
    for(unsigned id=1;id<30;++id)remove_test::checkSameCharacter(p.roster.at(id),initialCharacters[id]);
    // Duplicate weapon-6 offers are separate physical quantities. After row 2
    // purchase, original row 5 shifts to row 4; selecting row 2 would be ID15.
    p.roster.at(0).weapons={};p.roster.at(0).conditions[8]=0;p.monsterTreasure->gold=120;
    const auto first=xeenPrepareEquipmentPurchase(p,0,XeenInventoryCategory::Weapons,1);
    p.serviceEconomy=first.economyAfter;p.roster.at(0).weapons=first.result.recipientAfter;p.monsterTreasure->gold=first.result.goldAfter;
    check(p.serviceEconomy->wares[0][0][0][1].id==15 && p.serviceEconomy->wares[0][0][0][3].id==6,"duplicate physical shift differs");
    const auto second=xeenPrepareEquipmentPurchase(p,0,XeenInventoryCategory::Weapons,3);
    check(second.result.outcome==Outcome::Purchased && second.result.goldAfter==0 && second.result.recipientSlot==1,
        "second duplicate quantity failed");
    rejects([&]{xeenValidateEquipmentPurchaseEconomyDelta(*p.serviceEconomy,second.economyAfter,XeenInventoryCategory::Weapons,1,first.result.offer);});
}
XeenServiceEconomy generated() {
    XeenCombatRandom rng(7);XeenMerchantStockCandidate stock;
    while(!stock.complete()){XeenConsequenceDraw draw{rng,64,{}};stock.service(draw);}
    XeenServiceEconomy economy;economy.wares=stock.wares();economy.bank={199,4252442868u};return economy;
}
void rebinds() {
    auto economy=generated();const XeenJourneyRandomState cursor{1,799325555u,1101};
    XeenGameplayContext context;context.year=610;context.minutes=803;context.ctr24=23;
    auto after=economy;after.wares[0][0][1]=removeOracle(economy.wares[0][0][1],3);
    const auto offer=economy.wares[0][0][1][3];
    const XeenItemCategory expectedArmor{{{0,6,0,0},{0,4,0,0},{0,6,0,0},{0,5,0,0},{40,8,0,0},{48,6,0,0},{},{},{}}};
    sameCategory(after.wares[0][0][1],expectedArmor,"literal seed-7 armor depletion differs");
    for(unsigned day:{8u,9u,10u,97u,98u}) {
        context.day=day;XeenServiceDayCandidate old(context,economy,cursor);
        const bool trigger=day==10;
        if(trigger)rejects([&]{old.rebindPurchase(after,XeenInventoryCategory::Armor,3,offer);});
        while(!old.complete())old.service(64);
        const auto reservedEconomy=old.economy();const auto reservedCursor=old.continuation();
        auto rebound=old.rebindPurchase(after,XeenInventoryCategory::Armor,3,offer);
        check(old.beforeEconomy()==economy && old.economy()==reservedEconomy && old.continuation()==reservedCursor,"rebind mutated retained obligation");
        check(rebound.complete() && rebound.triggered()==trigger && rebound.beforeEconomy()==after &&
            rebound.beforeContext()==context && rebound.beforeRandom()==cursor && rebound.context()==old.context(),"rebind lost complete candidate/preimages");
        check(rebound.context().day==day+1 && rebound.context().minutes==803 && rebound.context().ctr24==23,"purchase/rebind advanced ordinary time");
        if(trigger)check(rebound.economy()==reservedEconomy && rebound.continuation()==XeenJourneyRandomState{1,2959920300u,2009} &&
            rebound.economy().bank==XeenBankBalances{200,0},"trigger rebind rerolled/restocked/repeated bank");
        else check(rebound.economy()==after && rebound.continuation()==cursor,"nontrigger rebind lost depletion");
        auto afterTwice=after;const auto second=afterTwice.wares[0][0][0][1];afterTwice.wares[0][0][0]=removeOracle(afterTwice.wares[0][0][0],1);
        auto twice=rebound.rebindPurchase(afterTwice,XeenInventoryCategory::Weapons,1,second);
        check(twice.beforeEconomy()==afterTwice && twice.economy()==(trigger?reservedEconomy:afterTwice) && twice.continuation()==reservedCursor,"repeated purchase rebind changed reserved work");
        const auto retained=twice.economy();check(twice.service(0) && twice.economy()==retained && twice.continuation()==reservedCursor,"completed rebound repeated RNG/interest");
    }
    context.day=99;XeenServiceDayCandidate rollover(context,economy,cursor);check(rollover.context().day==0 && rollover.context().year==611,"Purchase reservation rolls year");
    context.day=8;XeenServiceDayCandidate old(context,economy,cursor);
    for(unsigned s=0;s<2;++s)for(unsigned p=0;p<4;++p)for(unsigned c=0;c<4;++c)for(unsigned slot=0;slot<9;++slot)for(unsigned field=0;field<4;++field) {
        auto tampered=after;auto &item=tampered.wares[s][p][c][slot];
        auto &value=field==0?item.material:field==1?item.id:field==2?item.state:item.frame;value^=1;
        rejects([&]{old.rebindPurchase(tampered,XeenInventoryCategory::Armor,3,offer);});
    }
    auto badBank=after;++badBank.bank.gold;rejects([&]{old.rebindPurchase(badBank,XeenInventoryCategory::Armor,3,offer);});
    auto unchanged=economy;rejects([&]{old.rebindPurchase(unchanged,XeenInventoryCategory::Armor,3,offer);});
    rejects([&]{old.rebindPurchase(after,XeenInventoryCategory::Armor,2,offer);});
    for(unsigned day:{8u,10u})for(unsigned fault=0;fault<4;++fault) {
        context.day=day;XeenServiceDayCandidate tampered(context,economy,cursor);
        while(!tampered.complete())tampered.service(64);
        if(fault==0)++const_cast<XeenGameplayContext &>(tampered.context()).day;
        if(fault==1)++const_cast<XeenServiceEconomy &>(tampered.economy()).bank.gold;
        if(fault==2)const_cast<XeenServiceEconomy &>(tampered.economy()).wares[1][3][3][8].frame=1;
        if(fault==3)++const_cast<XeenGameplayContext &>(tampered.beforeContext()).minutes;
        rejects([&]{tampered.rebindPurchase(after,XeenInventoryCategory::Armor,3,offer);});
    }
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2 || (argc==4 && std::string(argv[2])=="--reference"),"usage: equipment-purchase-rules <original-installation> [--reference <clean-pinned-source>]");
        const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"installation unavailable");
        prices();if(argc==4)pinnedReference(argv[3]);
        else {training_test::Inputs inputs(*installation);preparedRules(inputs);rebinds();}
        std::cout<<"M42 literal prices, funds, physical delivery/depletion and checked departure rebind passed\n";return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
