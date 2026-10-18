#include "XeenTestInstallation.h"
#include "XeenPurchaseTestSupport.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <iostream>
#include <regex>
#include <fstream>
using namespace mmodern;
using namespace purchase_test;
namespace {
using C=XeenInventoryCategory;
using A=XeenItemService;
using O=XeenArmorRepairOutcome;
const XeenItemCatalog &originalCatalog(Inputs &in) {
    static const auto catalog=loadXeenItemCatalog(in.assets).catalog;return catalog;
}
struct Fixture:purchase_test::Fixture {
    Fixture(Inputs &in,const XeenSaveSnapshot &s):purchase_test::Fixture(in,s,false,&originalCatalog(in)){}
};
void preview(Inputs &in,const IndexedFrame &frame,const std::string &name) {
    if(const auto *path=std::getenv("MMODERN_SERVICE_PREVIEW")) {
        std::ofstream out(std::string(path)+"/"+name+".ppm",std::ios::binary);out<<"P6\n320 200\n255\n";
        const auto palette=in.assets.readArchiveResource("mm4.pal");
        for(auto pixel:frame.pixels)for(unsigned channel=0;channel<3;++channel){const unsigned char value=palette[pixel*3+channel]*4;out.write(reinterpret_cast<const char *>(&value),1);}
    }
}
std::vector<unsigned> table(const std::string &source,const char *name) {
    const auto start=source.find(std::string("LangConstants::")+name+"[");check(start!=std::string::npos,"reference table missing");
    const auto open=source.find('{',start),end=source.find('}',open);
    auto body=std::regex_replace(source.substr(open+1,end-open-1),std::regex("//[^\\n]*"),"");
    std::vector<unsigned> values;const std::regex number("[0-9]+");
    for(auto i=std::sregex_iterator(body.begin(),body.end(),number);i!=std::sregex_iterator();++i)values.push_back(std::stoul(i->str()));
    return values;
}
void prices(const char *root) {
    std::ifstream input(std::filesystem::path(root)/"devtools/create_mm/create_xeen/constants.cpp");check(bool(input),"reference unavailable");
    const std::string source(std::istreambuf_iterator<char>(input),{});
    const std::vector<std::vector<unsigned>> bases{table(source,"WEAPON_BASE_COSTS"),table(source,"ARMOR_BASE_COSTS"),table(source,"ACCESSORY_BASE_COSTS"),table(source,"MISC_BASE_COSTS")};
    const auto containers=table(source,"MISC_MATERIAL_COSTS"),metal=table(source,"METAL_BASE_MULTIPLIERS"),elements=table(source,"ELEMENTAL_DAMAGE"),attributes=table(source,"ATTRIBUTE_BONUSES");
    for(unsigned c=0;c<4;++c)for(unsigned id=1;id<bases[c].size();++id)
    for(unsigned m= c==3?1:0;m<(c==3?containers.size():131);++m) {
        unsigned value=bases[c][id];
        if(c==3)value+=containers[m];
        else if(m<37)value+=elements[m]*100;
        else if(m<41)value/=m==37?10:m==39?2:4;
        else if(m<59)value*=metal[m-37];
        else value+=attributes[m-59]*100; // DOS table; reference's lookup is invalid.
        for(unsigned merchant:{0u,1u,127u,128u,255u})for(unsigned a=0;a<4;++a) {
            const unsigned divisor=a==0?1:a==1?((merchant&127)?1:2):a==2?100:10;
            const XeenItem item{std::uint8_t(m),std::uint8_t(id),255,0};
            check(xeenItemPrice(C(c),item,A(a),merchant)==std::max(1u,value/divisor),"all-category price oracle differs");
        }
    }
    check(xeenItemPrice(C::Weapons,{38,7,0,0},A::Fix)==1,"minimum after metal division");
    check(xeenItemPrice(C::Armor,{121,10,128,0},A::Identify)==4,"DOS attribute addend before divisor");
    for(unsigned c=0;c<4;++c) {
        const XeenItem raw{std::uint8_t(c==3?10:59),1,0xc5,13};
        const auto fixed=xeenPrepareItemService(C(c),raw,100000,A::Fix);
        auto expected=raw;expected.state=0x45;
        check(fixed.outcome==O::Repaired && xeenSameItem(fixed.after,expected),"Fix changed preserved bytes");
        const auto identified=xeenPrepareItemService(C(c),raw,100000,A::Identify);
        check(identified.outcome==O::Identified && xeenSameItem(identified.after,raw),"Identify added durable state");
        check(xeenQuoteItemService(C(c),raw,100000,A::Sell).outcome==O::Refused,"cursed Sell admitted");
        auto broken=raw;broken.state=0x85;
        check(xeenPrepareItemService(C(c),broken,0,A::Sell).outcome==O::Sold,"broken Sell refused");
        auto intact=raw;intact.state=5;
        check(xeenQuoteItemService(C(c),intact,100,A::Fix).outcome==O::Intact,"intact Fix admitted");
        check(xeenPrepareItemService(C(c),broken,0,A::Identify).outcome==O::InsufficientGold,"Identify funds refusal");
        XeenItemCategory items{};items[0]=broken;items[1]=intact;
        const auto discarded=xeenDiscardItems(items,0);
        check(discarded && xeenSameItem((*discarded)[0],intact) && !(*discarded)[1].id,"Discard did not compact exact bytes");
        items[0]=raw;check(!xeenDiscardItems(items,0),"cursed Discard admitted");
    }
    check(xeenQuoteItemService(C::Weapons,{0,34,128,0},0,A::Sell).outcome==O::Refused,"slayer sword sold");
    for(unsigned state:{0u,1u,63u,64u,128u,255u}) {
        const XeenItem item{10,37,std::uint8_t(state),0};
        check(xeenItemUseAdmission(C::Weapons,item,true,true)==XeenItemUseAdmission::InvalidCategory,"non-Misc Use admitted");
        check(xeenItemUseAdmission(C::Miscellaneous,item,false,false)==XeenItemUseAdmission::NoWorld,"Use world precedence");
        check(xeenItemUseAdmission(C::Miscellaneous,item,true,false)==XeenItemUseAdmission::CannotAct,"Use owner gate");
        check(xeenItemUseAdmission(C::Miscellaneous,item,true,true)==((state==1 || state==63)?XeenItemUseAdmission::Admitted:XeenItemUseAdmission::NoAbility),"Use bad/charge gate");
    }
}
void key(Fixture &f,unsigned k,bool mouse) {
    const auto dialog=f.flow->inputContext(f.flow->frame().presentation()).dialog;
    check(bool(dialog),"dialog controls absent");
    if(mouse) {
        const auto hit=std::find_if(dialog->hits.begin(),dialog->hits.end(),[&](auto h){return h.key==k;});
        if(hit!=dialog->hits.end()){const auto a=dialog->click(hit->left,hit->top);check(bool(a),"mouse action absent");f.act(*a);return;}
        if(dialog->anyClick){f.act(*dialog->click(100,100));return;}
        check(false,"requested mouse control absent");
    }
    const auto action=dialog->key(k);check(bool(action),"requested keyboard control absent");f.act(*action);
}
void replay(Fixture &f,const IndexedFrame::Presentation &frame,std::optional<std::uint64_t> input) {
    const Owners paid(f);f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(YesAction{},input,frame));paid.unchanged(f);
}
XeenSaveSnapshot inventorySource(Inputs &in) {
    auto s=service(in,8,10000);
    auto &c=s.characters[0];c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};
    for(unsigned n=0;n<4;++n) {
        auto &items=*xeenInventoryItems(c,C(n));items[0]={std::uint8_t(n==3?10:59),1,0x85,0};items[1]={std::uint8_t(n==3?10:38),1,1,0};
    }
    return s;
}
void services(Inputs &in,bool mouse) {
    constexpr unsigned tabs[]{'w','a','c','m'};
    for(unsigned c=0;c<4;++c)for(unsigned action:{unsigned(A::Sell),unsigned(A::Identify),unsigned(A::Fix)})
    for(unsigned fault=0;fault<3;++fault) {
        auto source=inventorySource(in);Fixture f(in,source);f.enter();key(f,'b',mouse);key(f,tabs[c],mouse);
        key(f,action==1?'s':action==2?'i':'f',mouse);const Owners before(f);
        key(f,'1',mouse);check(XeenPurchaseTestAccess::quote(*f.flow),"all-category service quote absent");
        const auto verb=action==1?"Sell ":action==2?"Identify ":"Fix ";
        check(XeenPurchaseTestAccess::text(*f.flow).find(verb)!=std::string::npos,"service confirmation names the wrong operation");
        const auto raw=(*xeenInventoryItems(f.p.roster.at(0),C(c)))[0];
        const auto price=*xeenItemPrice(C(c),raw,A(action),f.p.roster.at(0).skills[8]);
        key(f,'n',mouse);before.unchanged(f);key(f,'1',mouse);
        bool fired=false;
        if(fault)f.flow->smithBoundary=[&](auto boundary){if(boundary==(fault==1?XeenSmithBoundary::BeforeRepair:XeenSmithBoundary::AfterRepair) && !fired){fired=true;throw std::bad_alloc();}};
        const auto frame=f.flow->frame().presentation();const auto input=f.flow->displayedInput();key(f,'y',mouse);
        if(fault==1){before.unchanged(f);check(fired,"before-publication fault absent");key(f,'y',mouse);}
        if(fault==2)check(fired,"after-publication fault absent");
        f.flow->smithBoundary={};
        check(f.p.monsterTreasure->gold==(action==1?10000+price:10000-price),"payment/credit not exactly once");
        const auto &items=*xeenInventoryItems(f.p.roster.at(0),C(c));
        if(action==1)check(xeenSameItem(items[0],(*xeenInventoryItems(source.characters[0],C(c)))[1]),"Sell compaction bytes differ");
        else {auto expected=raw;if(action==3)expected.state&=0x7f;check(xeenSameItem(items[0],expected),"paid service changed wrong bytes");}
        check(*f.p.encounterContext==before.context && *f.w.sessionState().journeyRandom()==before.random,"service charged time/RNG");
        replay(f,frame,input);
        if(action==2) {
            const auto dialog=f.flow->inputContext(f.flow->frame().presentation()).dialog;
            check(dialog->anyKey && dialog->keyMouseWait,"Identify detail window missing original dismissal");
            if(!mouse && !fault)preview(in,f.flow->frame(),"identify-"+std::to_string(c));
            key(f,InputKey::Enter,mouse);
        }
        f.leave();check(f.p.encounterContext->day==9,"service departure not one day");
        const auto saved=f.snapshot();Fixture restored(in,saved);save_test::sameSnapshot(saved,restored.snapshot());
    }
    Fixture f(in,service(in,8,100000));f.enter();key(f,'b',mouse);
    for(unsigned c=0;c<4;++c) {
        key(f,tabs[c],mouse);
        const auto stock=f.p.serviceEconomy->wares[0][0][c];
        check(stock[0].id!=0,"original seeded stock category absent");
        key(f,InputKey::F1+2,mouse); // empty recipient categories in the original roster
        const auto quote=xeenQuoteEquipmentPurchase(f.p,2,C(c),0);check(quote.outcome==XeenEquipmentPurchaseOutcome::Quoted,"generated Buy category refused");
        const auto before=*f.p.monsterTreasure;key(f,'1',mouse);const auto frame=f.flow->frame().presentation();const auto input=f.flow->displayedInput();key(f,'y',mouse);
        check(f.p.monsterTreasure->gold==before.gold-quote.price,"Buy debit differs");replay(f,frame,input);
        bool found=false;for(const auto &item:*xeenInventoryItems(f.p.party.member(f.p.roster,2),C(c)))found|=xeenSameItem(item,stock[0]);check(found,"Buy lost item bytes");
    }
    f.leave();check(f.p.encounterContext->day==9,"four Buy operations charged extra days");
    auto source=inventorySource(in);source.journey->treasure->gold=1000000;
    auto &recipient=source.characters[source.activeRosterIds[2]];
    recipient.weapons={};recipient.armor={};recipient.accessories={};recipient.miscellaneous={};
    Fixture depleted(in,source);depleted.enter();key(depleted,'b',mouse);key(depleted,InputKey::F1+2,mouse);
    for(unsigned c=0;c<4;++c) {
        key(depleted,tabs[c],mouse);
        for(unsigned slot=0;slot<8 && depleted.p.serviceEconomy->wares[0][0][c][0].id;++slot) {
            key(depleted,'1',mouse);check(XeenPurchaseTestAccess::quote(*depleted.flow),"remaining generated stock lacks Buy confirmation");key(depleted,'y',mouse);
        }
        check(!depleted.p.serviceEconomy->wares[0][0][c][0].id,"Buy did not exhaust category");
    }
    depleted.leave();check(depleted.p.encounterContext->day==9,"depleted shop departure differs");
    const auto saved=depleted.snapshot();Fixture restored(in,saved);save_test::sameSnapshot(saved,restored.snapshot());
}
void details(Inputs &in) {
    auto source=inventorySource(in);const auto catalog=loadXeenItemCatalog(in.assets).catalog;
    const auto &text=in.assets.uiText();
    for(unsigned c=0;c<4;++c) {
        const auto raw=(*xeenInventoryItems(source.characters[0],C(c)))[0];
        auto intact=raw;intact.state&=0x3f;
        const auto a=xeenIdentifiedDetails(text,source.characters[0],C(c),raw,in.names);
        const auto b=xeenIdentifiedDetails(text,source.characters[0],C(c),intact,in.names);
        check(a==b,"broken Identify hides full properties");
        const auto display=catalog.describe(C(c),raw).displayName;
        check(display.find("Broken")!=std::string::npos,"broken base naming absent");
        const auto full=xeenDialogFormat(text.scalar("IDENTIFY_ITEM_MSG"),{display,a});
        check(drawXeenIdentifiedDetails(training_test::frame(),in.font,full).isValid(),"Identify original detail layout overflow");
        auto owner=source.characters[0];owner.characterClass=XeenCharacterClass::Sorcerer;
        check(xeenIdentifiedDetails(text,owner,C(c),raw,in.names)==a,"Identify proficiency depends on owner");
    }
    check(catalog.describe(C::Weapons,{3,7,0x85,0}).displayName==catalog.describe(C::Weapons,{0,7,0x80,0}).displayName,"broken weapon shows modifier/suffix");
    const auto elec=xeenIdentifiedDetails(text,source.characters[0],C::Weapons,{9,7,128,0},in.names);
    check(elec.find("+2 Elec Damage")!=std::string::npos,"broken elemental detail missing");
}
void refusals(Inputs &in,bool mouse) {
    constexpr unsigned tabs[]{'w','a','c','m'};
    for(unsigned c=0;c<4;++c) {
        auto source=inventorySource(in);source.journey->treasure->gold=0;
        (*xeenInventoryItems(source.characters[0],C(c)))[0].state=0xc5;
        Fixture f(in,source);f.enter();key(f,'b',mouse);key(f,tabs[c],mouse);
        const Owners before(f);
        key(f,'s',mouse);key(f,'1',mouse);
        check(!XeenPurchaseTestAccess::quote(*f.flow) && f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey,"cursed Sell lacks refusal");
        before.unchanged(f);key(f,InputKey::Enter,mouse);
        key(f,'f',mouse);key(f,'2',mouse);
        check(!XeenPurchaseTestAccess::quote(*f.flow) && f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey,"intact Fix lacks refusal");
        before.unchanged(f);key(f,InputKey::Enter,mouse);
        for(unsigned action:{'i','f'}) {
            key(f,action,mouse);key(f,'1',mouse);check(XeenPurchaseTestAccess::quote(*f.flow),"paid refusal lacks confirmation");key(f,'y',mouse);
            check(f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey,"funds refusal absent");before.unchanged(f);key(f,InputKey::Enter,mouse);
        }
        f.leave();
        // A full physical tail wins over insufficient gold in every category.
        source=inventorySource(in);source.journey->treasure->gold=0;
        auto &items=*xeenInventoryItems(source.characters[0],C(c));items[8]=items[1];
        Fixture full(in,source);full.enter();key(full,'b',mouse);key(full,tabs[c],mouse);const Owners fullBefore(full);key(full,'1',mouse);
        check(!XeenPurchaseTestAccess::quote(*full.flow) && full.flow->inputContext(full.flow->frame().presentation()).dialog->anyKey,"full category admitted Buy");fullBefore.unchanged(full);full.leave();

        source=inventorySource(in);source.camera={28,8,5,XeenDirection::West};
        (*xeenInventoryItems(source.characters[0],C(c)))[0].state=0xc5;
        Fixture cursed(in,source);cursed.act(SelectMemberAction{0});key(cursed,'i',mouse);key(cursed,tabs[c],mouse);const Owners cursedBefore(cursed);
        key(cursed,'d',mouse);key(cursed,'1',mouse);
        check(cursed.flow->inputContext(cursed.flow->frame().presentation()).dialog->anyKey,"cursed Discard lacks refusal");cursedBefore.unchanged(cursed);
    }
}
void sheet(Inputs &in,bool mouse) {
    constexpr unsigned tabs[]{'w','a','c','m'};
    for(unsigned c=0;c<4;++c) {
        auto s=inventorySource(in);s.camera={28,8,5,XeenDirection::West};Fixture f(in,s);
        f.act(SelectMemberAction{0});key(f,'i',mouse);key(f,tabs[c],mouse);const Owners before(f);
        key(f,'d',mouse);key(f,'1',mouse);check(!f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey,"Discard confirmation absent");
        if(!mouse)preview(in,f.flow->frame(),"discard-"+std::to_string(c));
        key(f,'n',mouse);before.unchanged(f);
        key(f,'d',mouse);key(f,'1',mouse);const auto frame=f.flow->frame().presentation();const auto input=f.flow->displayedInput();key(f,'y',mouse);
        const auto &items=*xeenInventoryItems(f.p.roster.at(0),C(c));
        check(xeenSameItem(items[0],(*xeenInventoryItems(s.characters[0],C(c)))[1]) && !items[1].id,"Discard lost compaction/removed wrong item");
        check(f.p.monsterTreasure->gold==before.treasure.gold && *f.p.encounterContext==before.context && *f.w.sessionState().journeyRandom()==before.random,"Discard charged money/time/RNG");
        replay(f,frame,input);key(f,InputKey::Escape,mouse);key(f,InputKey::Escape,mouse);
        check(f.flow->canSave(),"Discard did not settle inventory");
        const auto saved=f.snapshot();Fixture restored(in,saved);save_test::sameSnapshot(saved,restored.snapshot());
    }
    auto s=inventorySource(in);s.camera={28,8,5,XeenDirection::West};s.characters[0].miscellaneous[0]={10,55,0x81,0};Fixture f(in,s);
    f.act(InspectInventoryAction{});key(f,'m',mouse);key(f,'1',mouse);const Owners before(f);key(f,'u',mouse);before.unchanged(f);
    check(f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey,"Broken item Use lacks notice");
}
}
int main(int argc,char **argv) {
    try {check(argc==3,"usage: mmodern_m61_part_a_tests <game> <reference>");prices(argv[2]);const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"installation unavailable");Inputs in(*installation);details(in);services(in,false);services(in,true);refusals(in,false);refusals(in,true);sheet(in,false);sheet(in,true);
        std::cout<<"M61 Part A prices, services, preserved bytes, details, Discard, Use admission and parity passed\n";return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
