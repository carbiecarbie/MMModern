#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenTraining.h"
#include "games/xeen/XeenTempleHeal.h"
#include "games/xeen/XeenArmorRepair.h"
#include <array>
#include <stdexcept>
#include <algorithm>
#include <string_view>

namespace mmodern {
namespace {

using R=XeenCharacterRules;
std::string str(std::string_view text) { return std::string(text); }
template<class V> std::string n(V value) { return std::to_string(value); }
const XeenCombatInputs *inputs(const XeenPartyState &p,const XeenCharacter &c) {
    const auto &i=p.roster.combatInputs(c.rosterId); return i?&*i:nullptr;
}
XeenCharacterRulesContext context(const XeenPartyState &p) {
    return {p.encounterContext?std::uint32_t(p.encounterContext->year):kCloudsInitialYear};
}
void partyButtons(DialogInput &input) {
    constexpr int faces[]{10,45,81,117,153,189};
    for(unsigned i=0;i<6;++i) { input.hits.push_back({faces[i],150,faces[i]+32,182,InputKey::F1+i});input.keys.push_back(InputKey::F1+i); }
}
void button(DialogInput &input,int l,int t,int r,int b,unsigned key,const char *resource=nullptr,unsigned frame=0) {
    input.hits.push_back({l,t,r,b,key,resource?std::optional<InputButton>{{resource,frame,l,t}}:std::nullopt});input.keys.push_back(key);
}
IndexedFrame render(const IndexedFrame &base,const XeenFontFormat &font,const std::string &text,
        XeenTextRect outer,XeenTextRect inner,bool border=true) {
    XeenTextRenderOptions options;options.originalControls=true;
    options.drawWindow=border;options.windowBounds=outer;options.bounds=inner;options.x=inner.left;options.y=inner.top;
    auto result=XeenTextRenderer(font).render(base,text,options);
    if(result.pages.size()!=1 || !result.diagnostics.empty()) throw std::runtime_error("Original dialog layout overflow");
    return std::move(result.pages.front());
}
void highlight(IndexedFrame &frame,std::size_t member,const XeenDialogSpriteDraw &draw) {
    // Interface::highlightChar uses the original char-selection glyph.
    if(draw && member<6) { constexpr int x[]{10,45,81,117,153,189};draw(frame,"global.icn",8,x[member]-1,149); }
}
const auto &conditions(const XeenDosText &text,const XeenCharacter &) { return text.table("CONDITION_NAMES"); }
std::uint32_t gold(const XeenPartyState &p) { return p.monsterTreasure?std::uint32_t(p.monsterTreasure->gold):0u; }
std::uint32_t gems(const XeenPartyState &p) { return p.monsterTreasure?std::uint32_t(p.monsterTreasure->gems):0u; }
}
std::string xeenDialogFormat(std::string_view format,const std::vector<std::string> &args) {
    if(format.size()>16384)throw std::length_error("Original dialog template exceeds bound");
    std::string result;std::size_t index=0;
    for(std::size_t i=0;i<format.size();) {
        if(format[i]!='%') { if(result.size()==16384)throw std::length_error("Original dialog text exceeds bound");result+=format[i++];continue; }
        if(++i<format.size() && format[i]=='%') { result+='%';++i;continue; }
        const bool zero=i<format.size() && format[i]=='0';
        unsigned width=0;while(i<format.size() && format[i]>='0' && format[i]<='9') {width=width*10+format[i++]-'0';if(width>32)throw std::invalid_argument("Original dialog format width exceeds bound");}
        if(i<format.size() && format[i]=='l') ++i;
        if(i==format.size() || std::string_view("sduic").find(format[i])==std::string_view::npos || index==args.size() || width>32)
            throw std::invalid_argument("Original dialog template argument mismatch");
        if(args[index].size()>16384)throw std::length_error("Original dialog argument exceeds bound");
        auto value=args[index++];if(format[i]=='c' && value.size()!=1)throw std::invalid_argument("Original dialog character argument mismatch");++i;
        if(value.size()<width) value.insert(0,width-value.size(),zero?'0':' ');
        result+=value;
        if(result.size()>16384) throw std::length_error("Original dialog text exceeds bound");
    }
    if(index!=args.size()) throw std::invalid_argument("Unused original dialog template arguments: used "+n(index)+" of "+n(args.size()));
    if(result.size()>16384)throw std::length_error("Original dialog text exceeds bound");
    return result;
}
std::string_view xeenDialogText(const XeenDosText &text,XeenDialogText id) {
    switch(id) {
    case XeenDialogText::ExchangingInCombat:return text.scalar("EXCHANGING_IN_COMBAT");
    case XeenDialogText::CursedItem:return text.scalar("CANNOT_REMOVE_CURSED_ITEM");
    case XeenDialogText::BackpackFull:return text.scalar("BACKPACK_IS_FULL");
    case XeenDialogText::NotProficient:return text.scalar("NOT_PROFICIENT");
    case XeenDialogText::EquippedAll:return text.scalar("EQUIPPED_ALL_YOU_CAN");
    case XeenDialogText::RemoveToEquip:return text.scalar("REMOVE_X_TO_EQUIP_Y");
    case XeenDialogText::Ring:return text.scalar("RING");
    case XeenDialogText::Medal:return text.scalar("MEDAL");
    case XeenDialogText::InNoCondition:return text.scalar("IN_NO_CONDITION");
    case XeenDialogText::Hurry:return text.scalar("WHATS_YOUR_HURRY");
    case XeenDialogText::UseInCombat:return text.scalar("USE_ITEM_IN_COMBAT");
    case XeenDialogText::NoSpecialAbilities:return text.scalar("NO_SPECIAL_ABILITIES");
    case XeenDialogText::CannotCastEngaged:return text.scalar("CANT_CAST_WHILE_ENGAGED");
    case XeenDialogText::WhichItem:return text.scalar("WHICH_ITEM");
    case XeenDialogText::PermanentlyDiscard:return text.scalar("PERMANENTLY_DISCARD");
    case XeenDialogText::BuyForGold:return text.scalar("BUY_X_FOR_Y_GOLD");
    case XeenDialogText::ItemsTitle:return text.scalar("X_FOR_THE_Y");
    case XeenDialogText::MiscCategory:return text.table("CATEGORY_NAMES")[3];
    case XeenDialogText::Charges:return text.scalar("FMT_CHARGES");
    case XeenDialogText::ItemNotBroken:return text.scalar("ITEM_NOT_BROKEN");
    case XeenDialogText::PartyNeedsRest:return text.scalar("THE_PARTY_NEEDS_REST");
    case XeenDialogText::RestComplete:return text.scalar("REST_COMPLETE");
    case XeenDialogText::PartyIsStarving:return text.scalar("PARTY_IS_STARVING");
    case XeenDialogText::HitSpellPointsRestored:return text.scalar("HIT_SPELL_POINTS_RESTORED");
    case XeenDialogText::TooDangerousToRest:return text.scalar("TOO_DANGEROUS_TO_REST");
    case XeenDialogText::SomeCharsMayDie:return text.scalar("SOME_CHARS_MAY_DIE");
    }
    throw std::invalid_argument("Unknown dialog text");
}
DialogInput xeenSheetInput() {
    DialogInput input;constexpr int x[]{10,61,112,177};
    for(unsigned col=0;col<4;++col) for(unsigned row=0;row<5;++row)
        button(input,x[col],24+23*row,x[col]+24,44+23*row,1001+col*5+row,"view.icn",(col*5+row)*2);
    button(input,285,11,309,31,'i',"view.icn",40);button(input,285,43,309,63,'q',"view.icn",42);
    button(input,285,75,309,95,'e',"view.icn",44);button(input,285,107,309,127,InputKey::Escape,"view.icn",46);
    for(auto key:{InputKey::Up,InputKey::Down,InputKey::Left,InputKey::Right,InputKey::Enter}) input.keys.push_back(key);
    partyButtons(input);return input;
}
DialogInput xeenItemsInput(bool misc,bool selection) {
    DialogInput input;
    if(selection) button(input,235,111,259,131,InputKey::Escape,"esc.icn");
    else {
        constexpr unsigned keys[]{'w','a','c','m','e','r','d','q',InputKey::Escape};
        for(unsigned col=0;col<9;++col) button(input,12+34*col,109,36+34*col,129,col==4 && misc?'u':keys[col],"items.icn",col==4 && misc?18:col*2);
        partyButtons(input);
    }
    for(unsigned row=0;row<9;++row) button(input,8,20+9*row,263,28+9*row,'1'+row);
    return input;
}
DialogInput xeenBuyInput(bool repair) {
    auto input=xeenItemsInput(false);
    constexpr unsigned keys[]{'w','a','c','m','b','s','i','f',InputKey::Escape};
    input.keys.clear();
    for(unsigned i=0;i<9;++i) {
        input.hits[i].key=keys[i];input.hits[i].button->resource="buy.icn";
    }
    for(const auto &hit:input.hits) input.keys.push_back(hit.key);
    input.hits[repair?7:4].button->frame=repair?15:9;
    return input;
}
DialogInput xeenLocationInput(XeenLocationDialog location) {
    DialogInput input;
    if(location==XeenLocationDialog::Training) {
        button(input,281,108,305,128,InputKey::Escape,"train.icn",2);
        button(input,242,108,266,128,'t',"train.icn",0);
    } else {
        button(input,261,108,285,128,InputKey::Escape,"esc.icn");
        if(location==XeenLocationDialog::Smith) {
            button(input,234,64,308,72,'b');
            // Maintainer's M51 entry contract retains the lobby R shortcut.
            input.keys.push_back('r');
        }
        else {
            button(input,234,54,308,62,'h');button(input,234,64,308,72,'d');button(input,234,74,308,82,'u');
        }
    }
    partyButtons(input);button(input,8,8,224,140,InputKey::Space);return input;
}
std::uint32_t xeenTempleUncurseCost(const XeenCharacter &c) {
    bool cursed=c.conditions[0]!=0;
    for(unsigned category=0;category<4;++category)
        for(const auto &item:*xeenInventoryItems(c,static_cast<XeenInventoryCategory>(category))) cursed|=(item.state&0x40)!=0;
    return cursed?c.currentLevel()*20:0;
}
std::string xeenNotEnoughGold(const XeenDosText &text) {
    return xeenDialogFormat(text.scalar("NOT_ENOUGH_X_IN_THE_Y"),{str(text.table("CONSUMABLE_NAMES")[0]),str(text.table("WHERE_NAMES")[0])});
}
std::string xeenServiceConfirm(const XeenDosText &text,bool repair,const std::string &name,std::uint32_t price) {
    // English getGoldPlurals is always the singular resource form.
    if(repair) return xeenDialogFormat(text.scalar("FIX_IDENTIFY_GOLD"),{str(text.table("FIX_IDENTIFY")[0]),name,n(price)});
    return xeenDialogFormat(text.scalar("BUY_X_FOR_Y_GOLD"),{name,n(price)});
}
std::string xeenLocationText(const XeenDosText &text,XeenLocationDialog location,const XeenPartyState &p,std::size_t member) {
    const auto &c=p.party.member(p.roster,member);
    const auto purse=gold(p);const auto money=purse>=1000000?n(purse/1000000)+" mil":n(purse);
    if(location==XeenLocationDialog::Smith) return xeenDialogFormat(text.scalar("BLACKSMITH_TEXT"),{c.name,money});
    if(location==XeenLocationDialog::Temple) {
        const auto heal=xeenQuoteTempleHeal(c,purse,*p.encounterContext);
        const auto uncurse=xeenTempleUncurseCost(c);
        return xeenDialogFormat(text.scalar("TEMPLE_TEXT"),{c.name,n(heal.price),"10",uncurse>9999?n(uncurse/1000)+"k":n(uncurse),money});
    }
    const auto r=xeenQuoteTraining(c,*p.roster.combatInputs(c.rosterId),purse,*p.encounterContext);
    std::string message;
    if(c.permanentLevel>=10) message=xeenDialogFormat(text.scalar("TRAINING_LEARNED_ALL"),{c.name});
    else if(r.missing) message=xeenDialogFormat(text.scalar("EXPERIENCE_FOR_LEVEL"),{c.name,n(r.missing),n(c.permanentLevel+1)});
    else message=xeenDialogFormat(text.scalar("ELIGIBLE_FOR_LEVEL"),{c.name,n(c.permanentLevel+1),n(r.cost)});
    return xeenDialogFormat(text.scalar("TRAINING_TEXT"),{message,money});
}
IndexedFrame drawXeenLocation(const XeenDosText &text,const IndexedFrame &base,const IndexedFrame &art,const XeenFontFormat &font,
        XeenLocationDialog location,const XeenPartyState &p,std::size_t member,const XeenDialogSpriteDraw &draw) {
    auto frame=base;
    for(int y=8;y<140;++y) std::copy_n(art.pixels.data()+y*320+8,216,frame.pixels.data()+y*320+8);
    frame=render(frame,font,xeenLocationText(text,location,p,member),{226,0,320,146},{234,8,312,138});
    if(draw) for(const auto &hit:xeenLocationInput(location).hits) if(hit.button) {
        const auto &b=*hit.button;draw(frame,b.resource,b.frame,b.x,b.y);
    }
    highlight(frame,member,draw);return frame;
}
std::uint32_t xeenBuyDisplayCost(XeenInventoryCategory category,const XeenItem &item) {
    // Read-only adaptation of pinned ItemsDialog::calcItemCost. This displays
    // all wares, including ones the purchase rules intentionally cannot buy.
    // ScummVM developers, GPL-3.0-or-later, 6814ee9b; constants.cpp tables.
    constexpr unsigned weapons[]{0,50,15,100,80,40,60,1,10,150,30,60,8,50,100,15,30,15,200,80,250,150,400,100,40,120,300,100,200,300,25,100,50,15,0};
    constexpr unsigned accessories[]{0,100,100,250,100,50,300,200,500,1000,2000};
    constexpr unsigned miscMaterial[]{0,50,1000,500,10,100,20,10,50,10,10,100,1,1,1,1,1,1,1,1,1,1};
    constexpr unsigned metal[]{10,25,5,75,2,5,10,20,50,2,3,5,10,20,30,40,50,60,70,80,90,100};
    constexpr unsigned elemental[]{0,2,3,4,5,10,15,20,30,2,3,4,5,10,15,20,2,4,5,10,20,2,4,8,16,32,2,3,4,5,10,15,20,30,5,10,25};
    if(!item.id) return 0;
    unsigned base=0;
    if(category==XeenInventoryCategory::Miscellaneous) {
        if(item.material>=std::size(miscMaterial) || item.id>75) return 0;
        base=miscMaterial[item.material]+(item.id<16?100:item.id<31?200:item.id<41?300:item.id<51?400:item.id<61?500:600);
    } else {
        if(category==XeenInventoryCategory::Weapons && item.id<std::size(weapons)) base=weapons[item.id];
        else if(category==XeenInventoryCategory::Armor && item.id<=13) base=kXeenArmorBaseCosts[item.id-1];
        else if(category==XeenInventoryCategory::Accessories && item.id<std::size(accessories)) base=accessories[item.id];
        else return 0;
        const auto m=item.material;
        if(m>=37 && m<=40) base/=m==37?10:m==39?2:4;
        else if(m>40 && m<59) base*=metal[m-37];
        if(m<37) base+=elemental[m]*100;
        else if(m>=59 && m-52<std::size(elemental)) base+=elemental[m-52]*100;
    }
    return std::max(1u,base);
}
IndexedFrame drawXeenBuy(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,const XeenItemCatalog &catalog,
        const XeenPartyState &p,const XeenInventorySelection &selection,bool repair,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,xeenDialogFormat(text.scalar("ITEMS_DIALOG_TEXT1"),{str(text.scalar("BTN_BUY")),str(text.scalar("BTN_SELL")),str(text.scalar("BTN_IDENTIFY")),str(text.scalar("BTN_FIX"))}),{0,101,320,146},{8,109,312,138});
    const auto &c=p.party.member(p.roster,selection.source);
    const auto &items=repair?*xeenInventoryItems(c,selection.category):p.serviceEconomy->wares[0][0][static_cast<unsigned>(selection.category)];
    std::vector<std::string> args{str(text.table("CATEGORY_NAMES")[static_cast<unsigned>(selection.category)]),repair?std::string(c.name):n(gold(p))};
    if(repair) args.push_back(str(text.scalar("COST")));
    else args.insert(args.begin()+1,"");
    for(unsigned i=0;i<9;++i) {
        const auto description=catalog.describe(selection.category,items[i]);
        if(description.empty) args.push_back(i==0?str(text.scalar("NO_ITEMS_AVAILABLE")):"");
        else {
            const auto quote=xeenQuoteArmorRepair(selection.category,items[i],gold(p));
            const auto cost=repair?(quote.outcome==XeenArmorRepairOutcome::Quoted?quote.price:xeenBuyDisplayCost(selection.category,items[i])/10):xeenBuyDisplayCost(selection.category,items[i]);
            args.push_back(xeenDialogFormat(text.scalar("ITEMS_DIALOG_LINE2"),{n(selection.slot==i?15:0),n(i+1),description.displayName,n(std::max(1u,cost))}));
        }
    }
    frame=render(frame,font,xeenDialogFormat(repair?text.scalar("X_FOR_Y"):text.scalar("AVAILABLE_GOLD_COST"),args),{0,0,320,108},{8,8,312,100});
    if(draw) {
        for(const auto &hit:xeenBuyInput(repair).hits) if(hit.button) {const auto &b=*hit.button;draw(frame,b.resource,b.frame,b.x,b.y);}
        if(selection.category!=XeenInventoryCategory::Miscellaneous) for(unsigned i=0;i<9;++i) if(items[i].id) {
            const unsigned id=items[i].id;
            const unsigned glyph=selection.category==XeenInventoryCategory::Weapons?(id<=17?1:id<=29 || id>33?13:4):
                selection.category==XeenInventoryCategory::Armor?(id<=7?3:id==8?2:id==9?5:id==10?9:id<=12?10:6):
                (id==1?8:id==2?12:id<=7?7:11);
            draw(frame,"equip.icn",xeenItemProficient(c,selection.category,id)?(repair?unsigned(items[i].frame):glyph):14,8,18+i*9);
        }
    }
    highlight(frame,selection.source,draw);return frame;
}
DialogInput xeenConfirmInput(bool large) {
    DialogInput input;
    button(input,large?120:129,large?133:112,large?144:153,large?153:132,'y',"confirm.icn",0);
    button(input,large?176:185,large?133:112,large?200:209,large?153:132,'n',"confirm.icn",2);
    input.keys.push_back(InputKey::Escape);return input;
}
std::optional<bool> xeenConfirmAnswer(unsigned key) {
    if(key=='y') return true;
    if(key=='n' || key==InputKey::Escape) return false;
    return {};
}
IndexedFrame drawXeenSheet(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,const XeenPartyState &p,
        std::size_t member,unsigned cursor,bool blink,const XeenDialogSpriteDraw &draw) {
    const auto &c=p.party.member(p.roster,member);const auto *in=inputs(p,c);const auto ctx=context(p);
    const auto stat=[&](unsigned a){return R::sheetStat(c,in,a,ctx);};
    const auto color=[&](unsigned a){return R::statColor(stat(a),R::sheetStat(c,in,a,ctx,true));};
    const int maxHp=R::maxHp(c,ctx),maxSp=R::maxSp(c,ctx);
    int totalResistance=0;for(unsigned i=0;i<6;++i) totalResistance+=R::sheetResistance(c,in,i);
    const auto condition=static_cast<unsigned>(c.worstCondition());
    const auto food=p.party.size()?std::uint16_t(p.food)/p.party.size()/3:0;
    const auto details=xeenDialogFormat(text.scalar("CHARACTER_DETAILS"),{
        str(text.scalar("PARTY_GOLD")),c.name,str(text.table("SEX_NAMES").at(static_cast<unsigned>(c.sex))),str(text.table("RACE_NAMES").at(static_cast<unsigned>(c.race))),str(text.table("CLASS_NAMES").at(static_cast<unsigned>(c.characterClass))),
        n(color(0)),n(stat(0)),n(color(5)),n(stat(5)),n(R::statColor(c.currentHp,maxHp)),n(int(c.currentHp)),n(R::currentExperience(c,in)),
        n(color(1)),n(stat(1)),n(color(6)),n(stat(6)),n(R::statColor(c.currentSp,maxSp)),n(int(c.currentSp)),n(gold(p)),
        n(color(2)),n(stat(2)),n(R::statColor(R::sheetAge(c,ctx),R::sheetAge(c,ctx,true))),n(R::sheetAge(c,ctx)),n(totalResistance),n(gems(p)),
        n(color(3)),n(stat(3)),n(R::statColor(c.currentLevel(),c.permanentLevel)),n(c.currentLevel()),n(R::skillCount(c)),n(food),food==1?" ":str(text.scalar("DAY_PLURAL")),
        n(color(4)),n(stat(4)),n(R::statColor(R::sheetArmorClass(c,in,ctx),R::sheetArmorClass(c,in,ctx,true))),n(R::sheetArmorClass(c,in,ctx)),n(R::awardCount(c)),
        n(condition<8?9:condition<12?32:condition<16?6:15),str(conditions(text,c)[condition]),"","","",""});
    auto frame=render(base,font,xeenDialogFormat(text.scalar("CHARACTER_TEMPLATE"),{details}),{0,0,320,146},{8,8,312,138});
    if(draw) {
        constexpr int x[]{2,53,104,169};
        for(unsigned col=0;col<4;++col) for(unsigned row=0;row<5;++row) draw(frame,"view.icn",(col*5+row)*2,x[col]+8,24+row*23);
        for(unsigned row=0;row<4;++row) draw(frame,"view.icn",40+row*2,285,11+row*32);
        if(cursor<20) { constexpr int cx[]{9,60,111,176};draw(frame,"view.icn",blink?49:48,cx[cursor/5],23+23*(cursor%5)); }
    }
    highlight(frame,member,draw);return frame;
}
XeenDialogPopup xeenSheetPopup(const XeenDosText &text,const XeenPartyState &p,std::size_t member,unsigned cell) {
    if(cell>=20 || cell==14) throw std::invalid_argument("Invalid stat popup");
    const auto &c=p.party.member(p.roster,member);const auto *in=inputs(p,c);const auto ctx=context(p);
    constexpr int x[]{61,112,177,34};XeenDialogPopup popup;
    popup.bounds={x[cell/5],24+23*int(cell%5),x[cell/5]+143,76+23*int(cell%5)};
    const auto name=cell<16?str(text.table("STAT_NAMES")[cell]):str(text.table("CONSUMABLE_NAMES")[cell-16]);
    if(cell<7) {
        constexpr int thresholds[]{3,5,7,9,11,13,15,17,19,21,25,30,35,40,50,75,100,125,150,175,200,225,250,65535};
        const int value=R::sheetStat(c,in,cell,ctx);unsigned rating=0;while(rating<23 && thresholds[rating]<=value)++rating;
        popup.text=xeenDialogFormat(text.scalar("CURRENT_MAXIMUM_RATING_TEXT"),{name,n(value),n(R::sheetStat(c,in,cell,ctx,true)),str(text.table("RATING_TEXT")[rating])});
    } else if(cell==7) popup.text=xeenDialogFormat(text.scalar("AGE_TEXT"),{name,n(R::sheetAge(c,ctx)),n(R::sheetAge(c,ctx,true)),n(c.originalDetails()?c.originalDetails()->birthDay:0),n(unsigned(c.birthYear))});
    else if(cell==8) { constexpr unsigned gains[]{5,6,6,7,8,6,5,4,7,6};const unsigned attacks=c.currentLevel()/gains[static_cast<unsigned>(c.characterClass)]+1;
        popup.text=xeenDialogFormat(text.scalar("LEVEL_TEXT"),{name,n(c.currentLevel()),n(int(c.permanentLevel)),n(attacks),attacks>1?"s":""}); }
    else if(cell<=11) {
        const int current=cell==9?R::sheetArmorClass(c,in,ctx):cell==10?int(c.currentHp):int(c.currentSp);
        const int max=cell==9?R::sheetArmorClass(c,in,ctx,true):cell==10?R::maxHp(c,ctx):R::maxSp(c,ctx);
        popup.text=xeenDialogFormat(text.scalar(cell==9?"CURRENT_MAXIMUM_SIGNED_TEXT":"CURRENT_MAXIMUM_TEXT"),{name,n(current),n(max)});popup.bounds.bottom=popup.bounds.top+42;
    } else if(cell==12) { std::vector<std::string> args{name};for(unsigned i=0;i<6;++i) args.push_back(n(R::sheetResistance(c,in,i)));
        popup.text=xeenDialogFormat(text.scalar("RESISTENCES_TEXT"),args);popup.bounds.bottom=popup.bounds.top+80;
    } else if(cell==13 || cell==19) {
        std::string lines;unsigned count=0;
        if(cell==13 && c.originalDetails()) {
            constexpr unsigned order[]{0,1,2,3,4,5,17,6,7,8,9,10,11,12,13,16,14,15};
            for(auto skill:order) if(c.originalDetails()->skills[skill]) {
                lines+="\n\t020"+str(text.table("SKILL_NAMES")[skill]);++count;
                if(skill==0) { int bonus=2*c.currentLevel()+(c.characterClass==XeenCharacterClass::Ninja?15:c.characterClass==XeenCharacterClass::Robber?30:0);
                    bonus+=c.race==XeenRace::Elf || c.race==XeenRace::Gnome?10:c.race==XeenRace::Dwarf?5:c.race==XeenRace::HalfOrc?-10:0;
                    lines+=n(std::max(0,bonus+R::equipmentBonus(c,10))); }
            }
        } else if(cell==19) for(unsigned i=0;i<16;++i) if(c.conditions[i]) {
            lines+="\n\t020"+str(conditions(text,c)[i]);if(i<12) lines+="\t095-"+n(unsigned(c.conditions[i]));++count;
        }
        if(!count) {lines=cell==13?str(text.scalar("NONE")):"\n\t020"+str(text.scalar("GOOD"));count=1;}
        popup.text="\x02\x03" "c"+name+"\x03l"+lines;
        popup.bounds.top-=int((cell==13?count:count-1)/2)*8;popup.bounds.bottom=popup.bounds.top+int(count)*9+26;
        if(popup.bounds.bottom>=200) { const int delta=popup.bounds.bottom-199;popup.bounds.top-=delta;popup.bounds.bottom-=delta; }
    } else if(cell==15) { const auto missing=R::experienceToNextLevel(c,in);
        popup.text=xeenDialogFormat(text.scalar("EXPERIENCE_TEXT"),{name,n(R::currentExperience(c,in)),missing?n(missing):str(text.scalar("ELIGIBLE"))});popup.bounds.bottom=popup.bounds.top+43;
    } else if(cell==16 || cell==17) {
        const auto bank=p.serviceEconomy?(cell==16?std::uint32_t(p.serviceEconomy->bank.gold):std::uint32_t(p.serviceEconomy->bank.gems)):0u;
        popup.text=xeenDialogFormat(text.scalar("IN_PARTY_IN_BANK"),{name,n(cell==16?gold(p):gems(p)),n(bank)});popup.bounds.bottom=popup.bounds.top+43;
    } else if(cell==18) { const auto days=p.party.size()?std::uint16_t(p.food)/p.party.size()/3:0;
        popup.text=xeenDialogFormat(text.scalar("FOOD_TEXT"),{name,n(std::uint16_t(p.food)),n(days),str(text.scalar(days==1?"DAY_SINGULAR":"DAY_PLURAL"))}); }
    return popup;
}
IndexedFrame drawXeenPopup(const IndexedFrame &base,const XeenFontFormat &font,const XeenDialogPopup &popup) {
    const auto b=popup.bounds;return render(base,font,popup.text,b,{b.left+8,b.top+8,b.right-8,b.bottom-8});
}
IndexedFrame drawXeenErrorScroll(const IndexedFrame &base,const XeenFontFormat &font,const std::string &message) {
    return render(base,font,"\x03" "c\v010\t000"+message,{52,149,268,198},{60,157,260,190});
}
IndexedFrame drawXeenConfirm(const IndexedFrame &base,const XeenFontFormat &font,const std::string &message,bool large,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,message,large?XeenTextRect{65,23,250,163}:XeenTextRect{99,59,237,141},large?XeenTextRect{73,31,242,155}:XeenTextRect{107,67,229,133});
    if(draw) {draw(frame,"confirm.icn",0,large?120:129,large?133:112);draw(frame,"confirm.icn",2,large?176:185,large?133:112);}
    return frame;
}
IndexedFrame drawXeenItemSelection(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,unsigned action,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,xeenDialogFormat(text.scalar("WHICH_ITEM"),{str(text.table("ITEM_ACTIONS").at(action))}),{50,103,266,139},{58,111,258,131});
    if(draw) draw(frame,"esc.icn",0,235,111);return frame;
}
IndexedFrame drawXeenItemTarget(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font) {
    return render(base,font,str(text.scalar("ON_WHO")),{228,106,320,146},{236,114,312,138});
}
std::string xeenBackpackFull(const XeenDosText &text,XeenInventoryCategory category,const std::string &name) {
    return xeenDialogFormat(text.table("CATEGORY_BACKPACK_IS_FULL").at(static_cast<unsigned>(category)),{name});
}
IndexedFrame drawXeenItems(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,const XeenItemCatalog &catalog,
        const XeenPartyState &p,const XeenInventorySelection &selection,const XeenDialogSpriteDraw &draw) {
    const bool misc=selection.category==XeenInventoryCategory::Miscellaneous;
    auto frame=render(base,font,xeenDialogFormat(text.scalar("ITEMS_DIALOG_TEXT1"),{str(misc?text.scalar("BTN_USE"):text.scalar("BTN_EQUIP")),str(text.scalar("BTN_REMOVE")),str(text.scalar("BTN_DISCARD")),str(text.scalar("BTN_QUEST"))}),{0,101,320,146},{8,109,312,138});
    const auto &c=p.party.member(p.roster,selection.source);
    const auto &items=*xeenInventoryItems(c,selection.category);
    std::vector<std::string> args{misc?"\x03l":"\x03" "c",str(text.table("CATEGORY_NAMES")[static_cast<unsigned>(selection.category)]),c.name,str(text.table("CLASS_NAMES")[static_cast<unsigned>(c.characterClass)]),misc?str(text.scalar("FMT_CHARGES")):" "};
    for(unsigned i=0;i<9;++i) {
        const auto description=catalog.describe(selection.category,items[i]);
        if(description.empty) args.push_back(i==0?str(text.scalar("NO_ITEMS_AVAILABLE")):"");
        else {
            std::vector<std::string> line{n(selection.slot==i?15:0),n(i+1),description.displayName};
            if(misc) line.push_back(n(description.counter));
            args.push_back(xeenDialogFormat(misc?text.scalar("ITEMS_DIALOG_LINE2"):text.scalar("ITEMS_DIALOG_LINE1"),line));
        }
    }
    frame=render(frame,font,xeenDialogFormat(text.scalar("X_FOR_THE_Y"),args),{0,0,320,108},{8,8,312,100});
    if(draw) {
        for(unsigned i=0;i<9;++i) draw(frame,"items.icn",i==4&&misc?18:i*2,12+i*34,109);
        if(!misc) for(unsigned i=0;i<9;++i) if(items[i].id) draw(frame,"equip.icn",xeenItemProficient(c,selection.category,items[i].id)?unsigned(items[i].frame):14,8,18+i*9);
    }
    highlight(frame,selection.source,draw);return frame;
}
}
