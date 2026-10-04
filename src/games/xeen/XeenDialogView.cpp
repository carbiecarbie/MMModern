#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include <array>
#include <stdexcept>
#include <algorithm>
#include <string_view>
#include "XeenDialogEnglish.inc"
namespace mmodern {
namespace {
namespace T=generated_dialog_text;
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
const auto &conditions(const XeenCharacter &c) { return c.sex==XeenSex::Female?T::CONDITION_NAMES_F:T::CONDITION_NAMES_M; }
std::uint32_t gold(const XeenPartyState &p) { return p.monsterTreasure?std::uint32_t(p.monsterTreasure->gold):0u; }
std::uint32_t gems(const XeenPartyState &p) { return p.monsterTreasure?std::uint32_t(p.monsterTreasure->gems):0u; }
}
std::string xeenDialogFormat(std::string_view format,const std::vector<std::string> &args) {
    std::string result;std::size_t index=0;
    for(std::size_t i=0;i<format.size();) {
        if(format[i]!='%') { result+=format[i++];continue; }
        if(++i<format.size() && format[i]=='%') { result+='%';++i;continue; }
        const bool zero=i<format.size() && format[i]=='0';
        unsigned width=0;while(i<format.size() && format[i]>='0' && format[i]<='9') width=width*10+format[i++]-'0';
        if(i<format.size() && format[i]=='l') ++i;
        if(i==format.size() || std::string_view("sduic").find(format[i])==std::string_view::npos || index==args.size() || width>32)
            throw std::invalid_argument("Original dialog template argument mismatch");
        auto value=args[index++];++i;
        if(value.size()<width) value.insert(0,width-value.size(),zero?'0':' ');
        result+=value;
        if(result.size()>16384) throw std::length_error("Original dialog text exceeds bound");
    }
    if(index!=args.size()) throw std::invalid_argument("Unused original dialog template arguments: used "+n(index)+" of "+n(args.size()));
    return result;
}
std::string_view xeenDialogText(XeenDialogText text) {
    switch(text) {
    case XeenDialogText::ExchangingInCombat:return T::EXCHANGING_IN_COMBAT;
    case XeenDialogText::CursedItem:return T::CANNOT_REMOVE_CURSED_ITEM;
    case XeenDialogText::BackpackFull:return T::BACKPACK_IS_FULL;
    case XeenDialogText::NotProficient:return T::NOT_PROFICIENT;
    case XeenDialogText::EquippedAll:return T::EQUIPPED_ALL_YOU_CAN;
    case XeenDialogText::RemoveToEquip:return T::REMOVE_X_TO_EQUIP_Y;
    case XeenDialogText::Ring:return T::RING;
    case XeenDialogText::Medal:return T::MEDAL;
    case XeenDialogText::InNoCondition:return T::IN_NO_CONDITION;
    case XeenDialogText::Hurry:return T::WHATS_YOUR_HURRY;
    case XeenDialogText::UseInCombat:return T::USE_ITEM_IN_COMBAT;
    case XeenDialogText::NoSpecialAbilities:return T::NO_SPECIAL_ABILITIES;
    case XeenDialogText::CannotCastEngaged:return T::CANT_CAST_WHILE_ENGAGED;
    case XeenDialogText::WhichItem:return T::WHICH_ITEM;
    case XeenDialogText::PermanentlyDiscard:return T::PERMANENTLY_DISCARD;
    case XeenDialogText::BuyForGold:return T::BUY_X_FOR_Y_GOLD;
    case XeenDialogText::ItemsTitle:return T::X_FOR_THE_Y;
    case XeenDialogText::MiscCategory:return T::CATEGORY_NAMES[3];
    case XeenDialogText::Charges:return T::FMT_CHARGES;
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
IndexedFrame drawXeenSheet(const IndexedFrame &base,const XeenFontFormat &font,const XeenPartyState &p,
        std::size_t member,unsigned cursor,bool blink,const XeenDialogSpriteDraw &draw) {
    const auto &c=p.party.member(p.roster,member);const auto *in=inputs(p,c);const auto ctx=context(p);
    const auto stat=[&](unsigned a){return R::sheetStat(c,in,a,ctx);};
    const auto color=[&](unsigned a){return R::statColor(stat(a),R::sheetStat(c,in,a,ctx,true));};
    const int maxHp=R::maxHp(c,ctx),maxSp=R::maxSp(c,ctx);
    int totalResistance=0;for(unsigned i=0;i<6;++i) totalResistance+=R::sheetResistance(c,in,i);
    const auto condition=static_cast<unsigned>(c.worstCondition());
    const auto food=p.party.size()?p.originalFood()/p.party.size()/3:0;
    const auto details=xeenDialogFormat(T::CHARACTER_DETAILS,{
        str(T::PARTY_GOLD),c.name,str(T::SEX_NAMES.at(static_cast<unsigned>(c.sex))),str(T::RACE_NAMES.at(static_cast<unsigned>(c.race))),str(T::CLASS_NAMES.at(static_cast<unsigned>(c.characterClass))),
        n(color(0)),n(stat(0)),n(color(5)),n(stat(5)),n(R::statColor(c.currentHp,maxHp)),n(int(c.currentHp)),n(R::currentExperience(c,in)),
        n(color(1)),n(stat(1)),n(color(6)),n(stat(6)),n(R::statColor(c.currentSp,maxSp)),n(int(c.currentSp)),n(gold(p)),
        n(color(2)),n(stat(2)),n(R::statColor(R::sheetAge(c,ctx),R::sheetAge(c,ctx,true))),n(R::sheetAge(c,ctx)),n(totalResistance),n(gems(p)),
        n(color(3)),n(stat(3)),n(R::statColor(c.currentLevel(),c.permanentLevel)),n(c.currentLevel()),n(R::skillCount(c)),n(food),str(T::DAYS[food==1?0:1]),
        n(color(4)),n(stat(4)),n(R::statColor(R::sheetArmorClass(c,in,ctx),R::sheetArmorClass(c,in,ctx,true))),n(R::sheetArmorClass(c,in,ctx)),n(R::awardCount(c)),
        n(condition<8?9:condition<12?32:condition<16?6:15),str(conditions(c)[condition]),"","","",""});
    auto frame=render(base,font,xeenDialogFormat(T::CHARACTER_TEMPLATE,{details}),{0,0,320,146},{8,8,312,138});
    if(draw) {
        constexpr int x[]{2,53,104,169};
        for(unsigned col=0;col<4;++col) for(unsigned row=0;row<5;++row) draw(frame,"view.icn",(col*5+row)*2,x[col]+8,24+row*23);
        for(unsigned row=0;row<4;++row) draw(frame,"view.icn",40+row*2,285,11+row*32);
        if(cursor<20) { constexpr int cx[]{9,60,111,176};draw(frame,"view.icn",blink?49:48,cx[cursor/5],23+23*(cursor%5)); }
    }
    highlight(frame,member,draw);return frame;
}
XeenDialogPopup xeenSheetPopup(const XeenPartyState &p,std::size_t member,unsigned cell) {
    if(cell>=20 || cell==14) throw std::invalid_argument("Invalid stat popup");
    const auto &c=p.party.member(p.roster,member);const auto *in=inputs(p,c);const auto ctx=context(p);
    constexpr int x[]{61,112,177,34};XeenDialogPopup popup;
    popup.bounds={x[cell/5],24+23*int(cell%5),x[cell/5]+143,76+23*int(cell%5)};
    const auto name=cell<16?str(T::STAT_NAMES[cell]):str(T::CONSUMABLE_NAMES[cell-16]);
    if(cell<7) {
        constexpr int thresholds[]{3,5,7,9,11,13,15,17,19,21,25,30,35,40,50,75,100,125,150,175,200,225,250,65535};
        const int value=R::sheetStat(c,in,cell,ctx);unsigned rating=0;while(rating<23 && thresholds[rating]<=value)++rating;
        popup.text=xeenDialogFormat(T::CURRENT_MAXIMUM_RATING_TEXT,{name,n(value),n(R::sheetStat(c,in,cell,ctx,true)),str(T::RATING_TEXT[rating])});
    } else if(cell==7) popup.text=xeenDialogFormat(T::AGE_TEXT,{name,n(R::sheetAge(c,ctx)),n(R::sheetAge(c,ctx,true)),str(T::BORN[0]),n(c.originalDetails()?c.originalDetails()->birthDay:0),n(unsigned(c.birthYear))});
    else if(cell==8) { constexpr unsigned gains[]{5,6,6,7,8,6,5,4,7,6};const unsigned attacks=c.currentLevel()/gains[static_cast<unsigned>(c.characterClass)]+1;
        popup.text=xeenDialogFormat(T::LEVEL_TEXT,{name,n(c.currentLevel()),n(int(c.permanentLevel)),n(attacks),attacks>1?"s":""}); }
    else if(cell<=11) {
        const int current=cell==9?R::sheetArmorClass(c,in,ctx):cell==10?int(c.currentHp):int(c.currentSp);
        const int max=cell==9?R::sheetArmorClass(c,in,ctx,true):cell==10?R::maxHp(c,ctx):R::maxSp(c,ctx);
        popup.text=xeenDialogFormat(T::CURRENT_MAXIMUM_TEXT,{name,n(current),n(max)});popup.bounds.bottom=popup.bounds.top+42;
    } else if(cell==12) { std::vector<std::string> args{name};for(unsigned i=0;i<6;++i) args.push_back(n(R::sheetResistance(c,in,i)));
        popup.text=xeenDialogFormat(T::RESISTENCES_TEXT,args);popup.bounds.bottom=popup.bounds.top+80;
    } else if(cell==13 || cell==19) {
        std::string lines;unsigned count=0;
        if(cell==13 && c.originalDetails()) {
            constexpr unsigned order[]{0,1,2,3,4,5,17,6,7,8,9,10,11,12,13,16,14,15};
            for(auto skill:order) if(c.originalDetails()->skills[skill]) {
                lines+="\n\t020"+str(T::SKILL_NAMES[skill]);++count;
                if(skill==0) { int bonus=2*c.currentLevel()+(c.characterClass==XeenCharacterClass::Ninja?15:c.characterClass==XeenCharacterClass::Robber?30:0);
                    bonus+=c.race==XeenRace::Elf || c.race==XeenRace::Gnome?10:c.race==XeenRace::Dwarf?5:c.race==XeenRace::HalfOrc?-10:0;
                    lines+=n(std::max(0,bonus+R::equipmentBonus(c,10))); }
            }
        } else if(cell==19) for(unsigned i=0;i<16;++i) if(c.conditions[i]) {
            lines+="\n\t020"+str(conditions(c)[i]);if(i<12) lines+="\t095-"+n(unsigned(c.conditions[i]));++count;
        }
        if(!count) {lines=cell==13?str(T::NONE):"\n\t020"+str(T::GOOD);count=1;}
        popup.text="\x02\x03" "c"+name+"\x03l"+lines;
        popup.bounds.top-=int((cell==13?count:count-1)/2)*8;popup.bounds.bottom=popup.bounds.top+int(count)*9+26;
        if(popup.bounds.bottom>=200) { const int delta=popup.bounds.bottom-199;popup.bounds.top-=delta;popup.bounds.bottom-=delta; }
    } else if(cell==15) { const auto missing=R::experienceToNextLevel(c,in);
        popup.text=xeenDialogFormat(T::EXPERIENCE_TEXT,{name,n(R::currentExperience(c,in)),missing?n(missing):str(T::ELIGIBLE)});popup.bounds.bottom=popup.bounds.top+43;
    } else if(cell==16 || cell==17) {
        const auto bank=p.serviceEconomy?(cell==16?std::uint32_t(p.serviceEconomy->bank.gold):std::uint32_t(p.serviceEconomy->bank.gems)):0u;
        popup.text=xeenDialogFormat(T::IN_PARTY_IN_BANK,{name,n(cell==16?gold(p):gems(p)),n(bank)});popup.bounds.bottom=popup.bounds.top+43;
    } else if(cell==18) { const auto days=p.party.size()?p.originalFood()/p.party.size()/3:0;
        popup.text=xeenDialogFormat(T::FOOD_TEXT,{name,n(p.originalFood()),str(T::FOOD_ON_HAND[0]),n(days),str(T::DAYS[days==1?0:1])}); }
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
IndexedFrame drawXeenItemSelection(const IndexedFrame &base,const XeenFontFormat &font,unsigned action,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,xeenDialogFormat(T::WHICH_ITEM,{str(T::ITEM_ACTIONS.at(action))}),{50,103,266,139},{58,111,258,131});
    if(draw) draw(frame,"esc.icn",0,235,111);return frame;
}
IndexedFrame drawXeenItemTarget(const IndexedFrame &base,const XeenFontFormat &font) {
    return render(base,font,str(T::ON_WHO),{228,106,320,146},{236,114,312,138});
}
std::string xeenBackpackFull(XeenInventoryCategory category,const std::string &name) {
    return xeenDialogFormat(T::CATEGORY_BACKPACK_IS_FULL.at(static_cast<unsigned>(category)),{name});
}
IndexedFrame drawXeenItems(const IndexedFrame &base,const XeenFontFormat &font,const XeenItemCatalog &catalog,
        const XeenPartyState &p,const XeenInventorySelection &selection,const XeenDialogSpriteDraw &draw) {
    const bool misc=selection.category==XeenInventoryCategory::Miscellaneous;
    auto frame=render(base,font,xeenDialogFormat(T::ITEMS_DIALOG_TEXT1,{str(misc?T::BTN_USE:T::BTN_EQUIP),str(T::BTN_REMOVE),str(T::BTN_DISCARD),str(T::BTN_QUEST)}),{0,101,320,146},{8,109,312,138});
    const auto &c=p.party.member(p.roster,selection.source);
    const auto &items=*xeenInventoryItems(c,selection.category);
    std::vector<std::string> args{misc?"\x03l":"\x03" "c",str(T::CATEGORY_NAMES[static_cast<unsigned>(selection.category)]),c.name,str(T::CLASS_NAMES[static_cast<unsigned>(c.characterClass)]),misc?str(T::FMT_CHARGES):" "};
    for(unsigned i=0;i<9;++i) {
        const auto description=catalog.describe(selection.category,items[i]);
        if(description.empty) args.push_back(i==0?str(T::NO_ITEMS_AVAILABLE):"");
        else {
            std::vector<std::string> line{n(selection.slot==i?15:0),n(i+1),description.displayName};
            if(misc) line.push_back(n(description.counter));
            args.push_back(xeenDialogFormat(misc?T::ITEMS_DIALOG_LINE2:T::ITEMS_DIALOG_LINE1,line));
        }
    }
    frame=render(frame,font,xeenDialogFormat(T::X_FOR_THE_Y,args),{0,0,320,108},{8,8,312,100});
    if(draw) {
        for(unsigned i=0;i<9;++i) draw(frame,"items.icn",i==4&&misc?18:i*2,12+i*34,109);
        if(!misc) for(unsigned i=0;i<9;++i) if(items[i].id) draw(frame,"equip.icn",xeenItemProficient(c,selection.category,items[i].id)?unsigned(items[i].frame):14,8,18+i*9);
    }
    highlight(frame,selection.source,draw);return frame;
}
}
