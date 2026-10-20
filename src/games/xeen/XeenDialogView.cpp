#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenEquipmentPurchase.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenTraining.h"
#include "games/xeen/XeenTempleHeal.h"
#include "games/xeen/XeenArmorRepair.h"
#include "games/xeen/detail/XeenItemTables.h"
#include "games/xeen/XeenItemServiceRules.h"
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
    return {p.encounterContext?std::uint32_t(p.encounterContext->year):kCloudsInitialYear,
        p.encounterContext?unsigned(p.encounterContext->effects[XeenPartyEffect::Bless]):0u};
}
void partyButtons(DialogInput &input) {
    constexpr int faces[]{10,45,81,117,153,189};
    for(unsigned i=0;i<6;++i) { input.hits.push_back({faces[i],150,faces[i]+32,182,InputKey::F1+i});input.keys.push_back(InputKey::F1+i); }
}
void button(DialogInput &input,int l,int t,int r,int b,unsigned key,const char *resource=nullptr,unsigned frame=0) {
    input.hits.push_back({l,t,r,b,key,resource?std::optional<InputButton>{{resource,frame,l,t}}:std::nullopt});input.keys.push_back(key);
}
IndexedFrame render(const IndexedFrame &base,const XeenFontFormat &font,const std::string &text,
        XeenTextRect outer,XeenTextRect inner,bool border=true,bool startupColors=false,bool stopAtBottom=false,
        XeenFontSize size=XeenFontSize::Normal) {
    XeenTextRenderOptions options;options.originalControls=true;options.startupColors=startupColors;
    options.drawWindow=border;options.windowBounds=outer;options.bounds=inner;options.x=inner.left;options.y=inner.top;
    options.stopAtBottom=stopAtBottom;
    options.size=size;
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
// Screen layout from pinned dialogs_spells.cpp/window.cpp; DOS Part 0 confirms
// the screen contents, marker positions, slot guard and any-key/mouse wait.
IndexedFrame drawXeenSpellInformation(const XeenDosText &text,const IndexedFrame &base,
        const XeenFontFormat &font,const XeenSpellInformation &info,const XeenDialogSpriteDraw &draw) {
    if(info.failed)throw std::invalid_argument("Failed spell has no information screen");
    if(info.detection) {
        auto frame=render(base,font,str(text.scalar("DETECT_MONSTERS")),{231,48,317,141},{239,56,309,133});
        if(draw) {
            draw(frame,"detctmon.icn",0,243,80);
            for(unsigned row=0;row<7;++row)for(unsigned col=0;col<7;++col)
                if(info.grid[row*7+col])draw(frame,"detctmon.icn",std::min(info.grid[row*7+col],3u),244+9*col,81+7*row);
            draw(frame,"global.icn",unsigned(info.facing)+1,270,101);
        }
        return frame;
    }
    std::vector<std::string> rows(3);
    for(unsigned i=0;i<3;++i)if(info.monsters[i]) {
        const auto &m=*info.monsters[i];
        const auto hp=m.hp>999?n(m.hp/1000)+"k":n(m.hp);
        rows[i]=xeenDialogFormat(text.scalar("MONSTER_DETAILS"),{m.name,hp,n(m.ac),n(m.attacks),text.table("MONSTER_SPECIAL_ATTACKS").at(m.special)});
    }
    return render(base,font,xeenDialogFormat(text.scalar("IDENTIFY_MONSTERS"),rows),{20,142,290,199},{28,150,282,191});
}
std::string xeenTreasureText(const XeenDosText &text,const XeenTreasureDisplay &treasure,std::size_t page) {
    if(page>treasure.items.size())throw std::out_of_range("Treasure display page");
    auto result=xeenDialogFormat(text.scalar("PARTY_FOUND"),{n(treasure.gold),n(treasure.gems)});
    if(treasure.stage==XeenTreasureDisplay::Stage::Blank){result+=text.scalar("GIVE_TREASURE_FORMATTING");return result;}
    if(!treasure.items.empty()) {
        const auto &line=treasure.items[std::min(page,treasure.items.size()-1)];
        const auto name=line.quest?xeenDialogFormat(text.scalar("QUEST_FOUND_PREFIX"),{line.itemName}):line.itemName;
        result+=text.scalar("GIVE_TREASURE_FORMATTING");
        result+=xeenDialogFormat(text.scalar("X_FOUND_Y"),{line.recipient,name});
    }
    if(treasure.stage==XeenTreasureDisplay::Stage::Final ||
        (treasure.stage==XeenTreasureDisplay::Stage::Pages && page==treasure.items.size()))result+=text.scalar("HIT_A_KEY");
    return result;
}
IndexedFrame drawXeenTreasure(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,
        const XeenTreasureDisplay &treasure,std::size_t page,bool warning) {
    const auto stream=warning?xeenDialogFormat(text.scalar("PARTY_FOUND"),{n(treasure.gold),n(treasure.gems)}):
        xeenTreasureText(text,treasure,page);
    auto frame=render(base,font,stream,{226,0,320,146},{234,8,312,138});
    return warning?drawXeenErrorScroll(frame,font,str(text.scalar("BACKPACKS_FULL_PRESS_KEY"))):frame;
}
IndexedFrame drawXeenDeathFire(const IndexedFrame &saved,unsigned evenFrame,const XeenDialogSpriteDraw &draw) {
    if(evenFrame>34 || evenFrame%2 || !draw)throw std::invalid_argument("Invalid Clouds death fire composition");
    auto frame=saved;
    const auto resource="fire"+std::to_string(evenFrame/10+1)+".vga";
    draw(frame,resource.c_str(),evenFrame%10,0,0);
    draw(frame,resource.c_str(),evenFrame%10+1,160,0);
    // Calibrated DOS DAT table at 0x47a44; pairs accumulate above the fire.
    constexpr int y[]{196,187,179,169,159,147,138,127,113,101,86,73,60,48,36,23,10};
    for(unsigned pair=0;pair<evenFrame/2;++pair) {
        draw(frame,"death.vga",pair*2,0,y[pair]);
        draw(frame,"death.vga",pair*2+1,160,y[pair]);
    }
    return frame;
}
IndexedFrame drawXeenDeathFinal(const IndexedFrame &base,const XeenDialogSpriteDraw &draw) {
    if(!draw)throw std::invalid_argument("Missing Clouds death sprite provider");
    auto frame=base;draw(frame,"death.vga",34,0,0);draw(frame,"death.vga",35,160,0);return frame;
}
IndexedFrame drawXeenDeathLaugh(const IndexedFrame &saved,std::optional<unsigned> index,const XeenDialogSpriteDraw &draw) {
    if((index && *index>21) || !draw)throw std::invalid_argument("Invalid Clouds death laughter composition");
    auto frame=saved;
    if(index)draw(frame,"death1.vga",*index,0,0);
    return frame;
}
std::string xeenDialogFormat(std::string_view format,const std::vector<std::string> &args) {
    if(format.size()>16384)throw std::length_error("Original dialog template exceeds bound");
    std::string result;std::size_t index=0;
    for(std::size_t i=0;i<format.size();) {
        if(format[i]!='%') { if(result.size()==16384)throw std::length_error("Original dialog text exceeds bound");result+=format[i++];continue; }
        if(++i<format.size() && format[i]=='%') { result+='%';++i;continue; }
        const bool signedValue=i<format.size() && format[i]=='+';
        if(signedValue)++i;
        const bool zero=i<format.size() && format[i]=='0';
        unsigned width=0;while(i<format.size() && format[i]>='0' && format[i]<='9') {width=width*10+format[i++]-'0';if(width>32)throw std::invalid_argument("Original dialog format width exceeds bound");}
        if(i<format.size() && format[i]=='l') ++i;
        if(i==format.size() || std::string_view("sduic").find(format[i])==std::string_view::npos || index==args.size() || width>32)
            throw std::invalid_argument("Original dialog template argument mismatch");
        if(args[index].size()>16384)throw std::length_error("Original dialog argument exceeds bound");
        auto value=args[index++];if(format[i]=='c' && value.size()!=1)throw std::invalid_argument("Original dialog character argument mismatch");++i;
        if(signedValue && !value.empty() && value.front()!='-' && value.front()!='+')value.insert(value.begin(),'+');
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
// Pinned dialogs_exchange.cpp / dialogs_quick_fight.cpp; DOS Exchange cancels
// only on the visible button, rather than the reference's y=245 extent.
DialogInput xeenExchangeInput() {
    DialogInput input;partyButtons(input);
    button(input,225,120,249,140,InputKey::Escape,"esc.icn");return input;
}
DialogInput xeenQuickFightInput() {
    DialogInput input;partyButtons(input);
    button(input,242,108,266,128,'n',"train.icn",0);
    button(input,281,108,305,128,InputKey::Escape,"train.icn",2);
    input.keys.push_back(InputKey::Enter);return input;
}
IndexedFrame drawXeenExchange(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,str(text.scalar("EXCHANGE_WITH_WHOM")),{50,112,266,148},{58,120,258,140});
    if(draw)draw(frame,"esc.icn",0,225,120);return frame;
}
IndexedFrame drawXeenQuickFight(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,
        const XeenCharacter &c,std::size_t slot,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,xeenDialogFormat(text.scalar("QUICK_FIGHT_TEXT"),{c.name,str(text.table("QUICK_FIGHT_OPTIONS").at(c.quickOption))}),{226,0,320,146},{234,8,312,138});
    if(draw) {draw(frame,"train.icn",0,242,108);draw(frame,"train.icn",2,281,108);}
    highlight(frame,slot,draw);return frame;
}

// Adapted from pinned dialogs_spells.cpp and window.cpp (ScummVM developers,
// GPL-3.0-or-later). All displayed templates come from the installed DOS module.
void XeenSpellListUi::scroll(unsigned key,std::size_t count) {
    if(key==InputKey::Up) {if(top)--top;}
    if(key==InputKey::Down && count>top+10)++top;
    if(key==InputKey::PageUp)top=top>=10?top-10:0;
    if(key==InputKey::PageDown && count)top=std::min(top+10,((count-1)/10)*10);
}
std::optional<std::size_t> XeenSpellListUi::row(unsigned key,std::size_t count) const {
    if(key<'0' || key>'9')return {};
    const auto index=top+(key=='0'?9:key-'1');
    return index<count?std::optional<std::size_t>{index}:std::nullopt;
}
DialogInput xeenCastInput(bool switching) {
    DialogInput input;if(switching)partyButtons(input);
    button(input,234,108,259,128,'c',"cast.icn",0);
    button(input,261,108,285,128,'n',"cast.icn",2);
    button(input,288,108,312,128,InputKey::Escape,"cast.icn",4);return input;
}
DialogInput xeenSpellListInput(bool selecting,bool switching) {
    DialogInput input;
    button(input,187,26,198,36,InputKey::Up,"scroll.icn",0);
    button(input,187,111,198,121,InputKey::Down,"scroll.icn",2);
    // DOS stores these rows with height 9 (DAT button table 0x4f19a), so they
    // tile without the reference's one-line gap.
    for(unsigned i=0;i<10;++i)button(input,40,28+9*i,187,37+9*i,i==9?'0':'1'+i);
    button(input,174,123,198,133,InputKey::Escape);
    button(input,187,35,198,73,InputKey::PageUp);
    button(input,187,74,198,112,InputKey::PageDown);
    if(selecting) {button(input,132,123,168,133,'s');input.keys.push_back(InputKey::Enter);}
    if(switching)partyButtons(input);return input;
}
DialogInput xeenSpellTargetInput() {DialogInput input;partyButtons(input);input.keys.push_back(InputKey::Escape);return input;}
IndexedFrame drawXeenProtectionCorners(const IndexedFrame &base,const XeenGameplayContext &context,const XeenDialogSpriteDraw &draw) {
    auto frame=base;if(draw)for(unsigned i=0;i<4;++i)
        draw(frame,"fecp.brd",8+2*i+(context.lightAndResistances[i+2]?1:0),i%2?219:8,i<2?8:134);
    return frame;
}
DialogInput xeenElementInput() {
    DialogInput input;constexpr unsigned keys[]{'f','e','c','a'};
    for(unsigned i=0;i<4;++i)button(input,60+30*i,92,84+30*i,112,keys[i],"element.icn",2*i);
    input.keys.push_back(InputKey::Escape);return input;
}
std::optional<unsigned> xeenElementSelection(unsigned key) {
    constexpr unsigned keys[]{'f','e','c','a'};
    for(unsigned i=0;i<4;++i)if(key==keys[i])return i;return {};
}
IndexedFrame drawXeenElement(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,str(text.scalar("WHICH_ELEMENT2")),{50,71,182,129},{58,79,174,121});
    if(draw)for(unsigned i=0;i<4;++i)draw(frame,"element.icn",2*i,60+30*i,92);
    return frame;
}
DialogInput xeenSpellConfirmInput(bool info) {
    DialogInput input;
    if(info)input.anyKey=input.anyClick=true;
    else {button(input,120,133,144,153,'y',"confirm.icn",0);button(input,176,133,200,153,'n',"confirm.icn",2);input.keys.push_back(InputKey::Escape);}
    return input;
}
IndexedFrame drawXeenCast(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,
        const XeenCharacter &c,std::size_t member,const XeenLearnedSpellNames &names,const XeenDialogSpriteDraw &draw) {
    const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
    const auto id=category?XeenLearnedSpellRules::spellForSlot(*category,c.currentSpell):std::nullopt;
    const auto cost=XeenLearnedSpellRules::cost(id.value_or(76),c.currentLevel());
    auto frame=render(base,font,xeenDialogFormat(text.scalar("CAST_SPELL_DETAILS"),{c.name,names.names[id.value_or(76)],n(cost.sp),n(cost.gems),n(c.currentSp)}),{226,0,320,146},{234,8,312,138});
    if(draw)for(const auto &hit:xeenCastInput().hits)if(hit.button) {const auto &b=*hit.button;draw(frame,b.resource,b.frame,b.x,b.y);}
    if(draw)draw(frame,"border.icn",52,215,32); // Window 10 covers the right statue.
    highlight(frame,member,draw);return frame;
}
IndexedFrame drawXeenSpellList(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,
        const XeenCharacter &c,const XeenLearnedSpellNames &names,const XeenSpellListUi &ui,std::uint32_t money,
        bool guild,bool info,const XeenDialogSpriteDraw &draw,std::uint32_t year,XeenMapIdentity map) {
    const auto rows=XeenLearnedSpellRules::rows(c,guild,info,map);
    const bool noncaster=R::maxSp(c,{year})==0;
    const auto error=noncaster?text.scalar("NOT_A_SPELL_CASTER"):text.scalar(guild?"SPELLS_LEARNED_ALL":"NO_KNOWN_SPELLS");
    const bool empty=rows.empty() || (noncaster && !guild);
    auto frame=render(base,font,xeenDialogFormat(text.scalar("SPELLS_FOR"),{empty || noncaster?"":str(text.scalar("SPELL_LINES_0_TO_9")),c.name}),{27,6,207,142},{35,14,199,134});
    std::vector<std::string> args;
    for(unsigned i=0;i<10;++i) {
        const auto position=ui.top+i;std::string name;
        if(!empty && position<rows.size()) {
            const auto id=*XeenLearnedSpellRules::spellForSlot(*XeenLearnedSpellRules::categoryForClass(c.characterClass),rows[position]);
            const auto cost=XeenLearnedSpellRules::cost(id,c.currentLevel());
            name=guild?xeenDialogFormat(text.scalar("SPELL_GOLD_ROW"),{names.names[id],n(XeenLearnedSpellRules::purchasePrice(id,c.characterClass))}):
                xeenDialogFormat(text.scalar("SPELL_COST_ROW"),{names.names[id],n(cost.sp),n(cost.gems)});
        } else if(empty && i==0)name=error;
        args.push_back(n(ui.selection==position?15:9));args.push_back(std::move(name));
    }
    args.push_back(str(text.scalar(guild?"SPELL_GOLD":"SPELL_PTS")));args.push_back(n(guild?money:std::uint32_t(c.currentSp)));
    // Window 37 continues the reduced font selected by SPELLS_FOR in window 25.
    frame=render(frame,font,xeenDialogFormat(text.scalar("SPELLS_DIALOG_SPELLS"),args),{27,6,195,142},{35,14,187,134},false,false,false,XeenFontSize::Reduced);
    if(draw) {draw(frame,"scroll.icn",4,39,26);draw(frame,"scroll.icn",0,187,26);draw(frame,"scroll.icn",2,187,111);if(!guild)draw(frame,"scroll.icn",5,132,123);}
    return frame;
}
IndexedFrame drawXeenSpellConfirm(const IndexedFrame &base,const XeenFontFormat &font,const std::string &message,bool info,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,message,{65,23,250,163},{73,31,242,155},true,false,true);
    if(draw && !info){draw(frame,"confirm.icn",0,120,133);draw(frame,"confirm.icn",2,176,133);}return frame;
}
IndexedFrame drawXeenSpellTarget(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font) {
    return render(base,font,str(text.scalar("ON_WHO")),{228,106,320,146},{236,114,312,138});
}
std::vector<std::string> xeenSpellDescriptions(const std::vector<std::uint8_t> &bytes) {
    if(bytes.empty() || bytes.size()>65535)throw std::invalid_argument("Invalid spldesc.bin size");
    std::vector<std::string> result;std::size_t start=0;
    for(std::size_t i=0;i<bytes.size();++i)if(!bytes[i]) {
        if(i-start>2048 || result.size()==256)throw std::invalid_argument("Invalid spldesc.bin entry");
        result.emplace_back(reinterpret_cast<const char *>(bytes.data()+start),i-start);
        XeenDosText::validateControls(result.back());start=i+1;
    }
    if(start!=bytes.size() || result.size()<76)throw std::invalid_argument("Truncated spldesc.bin");
    return result;
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
DialogInput xeenItemSpellInput(unsigned mode,bool selection) {
    DialogInput input;
    if(selection)button(input,235,111,259,131,InputKey::Escape,"esc.icn");
    else {
        const unsigned keys[]{'w','a','c','m',unsigned(mode==6?'e':mode==4?'r':'g'),InputKey::Escape};
        for(unsigned i=0;i<6;++i)button(input,i==5?284:12+34*i,109,i==5?308:36+34*i,129,keys[i],"buy.icn",i*2);
    }
    for(unsigned row=0;row<9;++row)button(input,8,20+9*row,263,28+9*row,'1'+row);
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
DialogInput xeenBuyInput(XeenItemService action) {
    auto input=xeenBuyInput(false);
    input.hits[4].button->frame=8;
    const unsigned selected=action==XeenItemService::Buy?4:action==XeenItemService::Sell?5:action==XeenItemService::Identify?6:7;
    input.hits[selected].button->frame=selected*2+1;
    return input;
}
bool xeenGuildMember(const XeenPartyState &p,std::size_t member,XeenMapIdentity map) {
    if(member>=p.party.size() || map.side!=XeenSide::Clouds)return false;
    const unsigned award=map.number==49?5:map.number>=28 && map.number<=32?map.number-28:255;
    return award<6 && p.party.member(p.roster,member).awards[award]!=0;
}
std::optional<std::string> xeenLocationUnsupportedAction(XeenLocationDialog location,unsigned key) {
    if(location==XeenLocationDialog::Tavern) {
        if(key=='s')return "Sign In: not supported yet";
    }
    return {};
}
DialogInput xeenLocationInput(XeenLocationDialog location) {
    DialogInput input;
    if(location==XeenLocationDialog::Bank) {
        button(input,234,108,259,128,'d',"bank.icn",0);
        button(input,261,108,285,128,'w',"bank.icn",2);
        button(input,288,108,312,128,InputKey::Escape,"bank.icn",4);
    } else if(location==XeenLocationDialog::Tavern) {
        button(input,281,108,305,128,InputKey::Escape,"tavern.icn",0);
        button(input,242,108,266,128,'s',"tavern.icn",2);
        button(input,234,54,308,62,'d');button(input,234,64,308,72,'f');
        button(input,234,74,308,82,'t');button(input,234,84,308,92,'r');
    } else if(location==XeenLocationDialog::Training) {
        button(input,281,108,305,128,InputKey::Escape,"train.icn",2);
        button(input,242,108,266,128,'t',"train.icn",0);
    } else {
        button(input,261,108,285,128,InputKey::Escape,"esc.icn");
        if(location==XeenLocationDialog::Smith) {
            button(input,234,64,308,72,'b');
            // Maintainer's M51 entry contract retains the lobby R shortcut.
            input.keys.push_back('r');
        }
        else if(location==XeenLocationDialog::Guild) {
            button(input,234,64,308,72,'b');button(input,234,74,308,82,'s');
        }
        else {
            button(input,234,54,308,62,'h');button(input,234,64,308,72,'d');button(input,234,74,308,82,'u');
        }
    }
    if(location!=XeenLocationDialog::Bank) partyButtons(input);
    button(input,8,8,224,140,InputKey::Space);return input;
}
DialogInput xeenBankInput() {
    DialogInput input;
    button(input,234,108,259,128,'o',"bank2.icn",0);
    button(input,261,108,285,128,'e',"bank2.icn",2);
    button(input,288,108,312,128,InputKey::Escape,"bank2.icn",4);
    return input;
}
DialogInput xeenFoodConfirmInput() {
    DialogInput input;
    button(input,235,75,259,95,'y',"confirm.icn",0);
    button(input,260,75,284,95,'n',"confirm.icn",2);
    input.keys.push_back(InputKey::Escape);return input;
}
std::string xeenBankText(const XeenDosText &text,const XeenPartyState &p,bool withdrawal,bool refresh) {
    const auto money=[](std::uint32_t value) {return value>=1000000?n(value/1000000)+" mil":n(value);};
    return xeenDialogFormat(text.scalar(refresh?"GOLD_GEMS_2":"GOLD_GEMS"),{
        text.table("DEPOSIT_WITHDRAWL").at(withdrawal?1:0),
        money(withdrawal?std::uint32_t(p.serviceEconomy->bank.gold):gold(p)),
        money(withdrawal?std::uint32_t(p.serviceEconomy->bank.gems):gems(p))});
}
IndexedFrame drawXeenBankCurrency(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,
        const XeenPartyState &p,bool withdrawal,const XeenDialogSpriteDraw &draw,bool refresh) {
    auto frame=render(base,font,xeenBankText(text,p,withdrawal),{226,26,320,146},{234,34,312,138});
    if(refresh)frame=render(frame,font,xeenBankText(text,p,withdrawal,true),{226,26,320,146},{234,34,312,138},false);
    if(draw)for(const auto &hit:xeenBankInput().hits)if(hit.button) {
        const auto &b=*hit.button;draw(frame,b.resource,b.frame,b.x,b.y);
    }
    return frame;
}
IndexedFrame drawXeenBankAmount(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,
        const std::string &digits,const XeenDialogSpriteDraw &,char cursor) {
    if(digits.size()>10 || digits.find_first_not_of("0123456789")!=std::string::npos)
        throw std::invalid_argument("Invalid Bank amount input");
    // NumericInput in window 35 clears 77 pixels, centers only the digits,
    // then draws its cursor at the resulting write position.
    XeenTextRenderOptions options;options.originalControls=true;
    options.bounds={234,34,312,138};options.x=234;options.y=34;
    auto result=XeenTextRenderer(font).render(base,str(text.scalar("AMOUNT"))+"\x03l\t000\x04" "077\x03" "c"+digits,options);
    if(result.pages.size()!=1 || !result.diagnostics.empty())throw std::runtime_error("Original Bank amount layout overflow");
    options.x=result.writeX;options.y=result.writeY;options.size=result.writeSize;options.colorIndex=result.writeColor;
    auto frame=std::move(result.pages.front());
    if(cursor) {
        auto cursorFrame=XeenTextRenderer(font).render(frame,std::string(1,cursor),options);
        if(cursorFrame.pages.size()!=1 || !cursorFrame.diagnostics.empty())throw std::runtime_error("Original Bank cursor layout overflow");
        frame=std::move(cursorFrame.pages.front());
    }
    return frame;
}
IndexedFrame drawXeenFoodOffer(const IndexedFrame &base,const XeenFontFormat &font,const std::string &offer,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,offer,{0,143,320,199},{8,151,312,191});
    if(draw) {
        draw(frame,"global.icn",7,232,74);
        draw(frame,"confirm.icn",0,235,75);draw(frame,"confirm.icn",2,260,75);
    }
    return frame;
}
std::string xeenNotEnoughGold(const XeenDosText &text) {
    return xeenNotEnough(text,0);
}
std::string xeenNotEnough(const XeenDosText &text,unsigned consumable) {
    return xeenDialogFormat(text.scalar("NOT_ENOUGH_X_IN_THE_Y"),{str(text.table("CONSUMABLE_NAMES").at(consumable)),str(text.table("WHERE_NAMES")[0])});
}
std::string xeenServiceConfirm(const XeenDosText &text,bool repair,const std::string &name,std::uint32_t price) {
    // English getGoldPlurals is always the singular resource form.
    // The DOS table orders Identify before Fix, unlike the reference constants.
    if(repair) return xeenDialogFormat(text.scalar("FIX_IDENTIFY_GOLD"),{str(text.table("FIX_IDENTIFY")[1]),name,n(price)});
    return xeenDialogFormat(text.scalar("BUY_X_FOR_Y_GOLD"),{name,n(price)});
}
std::string xeenServiceConfirm(const XeenDosText &text,XeenItemService action,const std::string &name,std::uint32_t price) {
    if(action==XeenItemService::Buy || action==XeenItemService::Fix)
        return xeenServiceConfirm(text,action==XeenItemService::Fix,name,price);
    if(action==XeenItemService::Sell)return xeenDialogFormat(text.scalar("SELL_X_FOR_Y_GOLD"),{name,n(price)});
    return xeenDialogFormat(text.scalar("FIX_IDENTIFY_GOLD"),{text.table("FIX_IDENTIFY")[0],name,n(price)});
}
std::string xeenTempleText(const XeenDosText &text,const XeenPartyState &p,std::size_t member,
        const XeenTempleSurcharges &prices,bool templeBlessed) {
    const auto &c=p.party.member(p.roster,member);
    const auto purse=gold(p);const auto money=purse>=1000000?n(purse/1000000)+" mil":n(purse);
    const auto heal=xeenQuoteTempleHeal(c,purse,*p.encounterContext,prices);
    const auto uncurse=xeenTempleUncurseCost(c,prices);
    return xeenDialogFormat(text.scalar("TEMPLE_TEXT"),{c.name,n(heal.price),n(templeBlessed?0:prices.donation),uncurse>9999?n(uncurse/1000)+"k":n(uncurse),money});
}
std::string xeenLocationText(const XeenDosText &text,XeenLocationDialog location,const XeenPartyState &p,std::size_t member,
        XeenMapIdentity map,bool templeBlessed,std::string_view tavernStatus) {
    const auto &c=p.party.member(p.roster,member);
    const auto purse=gold(p);const auto money=purse>=1000000?n(purse/1000000)+" mil":n(purse);
    if(location==XeenLocationDialog::Bank) {
        const auto money=[](std::uint32_t value) {return value>=1000000?n(value/1000000)+" mil":n(value);};
        return xeenDialogFormat(text.scalar("BANK_TEXT"),{money(p.serviceEconomy->bank.gold),money(p.serviceEconomy->bank.gems),money(gold(p)),money(gems(p))});
    }
    if(location==XeenLocationDialog::Tavern) return xeenDialogFormat(text.scalar("TAVERN_TEXT"),{c.name,tavernStatus.empty()?str(text.scalar("FOOD_AND_DRINK")):str(tavernStatus),money});
    if(location==XeenLocationDialog::Guild) {
        const auto description=xeenGuildMember(p,member,map)?xeenDialogFormat(text.scalar("GUILD_TEXT"),{c.name}):str(text.scalar("GUILD_NOT_MEMBER_TEXT"));
        return xeenDialogFormat(text.scalar("GUILD_OPTIONS"),{description,money});
    }
    if(location==XeenLocationDialog::Smith) return xeenDialogFormat(text.scalar("BLACKSMITH_TEXT"),{c.name,money});
    if(location==XeenLocationDialog::Temple)
        return xeenTempleText(text,p,member,xeenTempleSurcharges(map),templeBlessed);
    const auto r=xeenQuoteTraining(c,*p.roster.combatInputs(c.rosterId),purse,*p.encounterContext,map);
    std::string message;
    if(r.outcome==XeenTrainingOutcome::Cap) message=xeenDialogFormat(text.scalar("TRAINING_LEARNED_ALL"),{c.name});
    else if(r.missing) message=xeenDialogFormat(text.scalar("EXPERIENCE_FOR_LEVEL"),{c.name,n(r.missing),n(c.permanentLevel+1)});
    else message=xeenDialogFormat(text.scalar("ELIGIBLE_FOR_LEVEL"),{c.name,n(c.permanentLevel+1),n(r.cost)});
    return xeenDialogFormat(text.scalar("TRAINING_TEXT"),{message,money});
}
IndexedFrame drawXeenLocation(const XeenDosText &text,const IndexedFrame &base,const IndexedFrame &art,const XeenFontFormat &font,
        XeenLocationDialog location,const XeenPartyState &p,std::size_t member,XeenMapIdentity map,
        const XeenDialogSpriteDraw &draw,bool templeBlessed,std::string_view tavernStatus,const XeenTempleSurcharges *templePrices) {
    auto frame=base;
    for(int y=8;y<140;++y) std::copy_n(art.pixels.data()+y*320+8,216,frame.pixels.data()+y*320+8);
    frame=render(frame,font,location==XeenLocationDialog::Temple && templePrices ?
        xeenTempleText(text,p,member,*templePrices,templeBlessed):xeenLocationText(text,location,p,member,map,templeBlessed,tavernStatus),{226,0,320,146},{234,8,312,138});
    if(draw) for(const auto &hit:xeenLocationInput(location).hits) if(hit.button) {
        const auto &b=*hit.button;draw(frame,b.resource,b.frame,b.x,b.y);
    }
    highlight(frame,member,draw);return frame;
}
IndexedFrame drawXeenTavernPopup(const IndexedFrame &base,const XeenFontFormat &font,const std::string &message) {
    return render(base,font,message,{0,143,320,199},{8,151,312,191});
}
std::uint32_t xeenBuyDisplayCost(XeenInventoryCategory category,const XeenItem &item) {
    return xeenItemPrice(category,item,XeenItemService::Buy).value_or(0);
}
IndexedFrame drawXeenBuy(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,const XeenItemCatalog &catalog,
        const XeenPartyState &p,const XeenInventorySelection &selection,XeenItemService action,const XeenDialogSpriteDraw &draw,XeenMapIdentity map) {
    const bool repair=action!=XeenItemService::Buy;
    auto frame=render(base,font,xeenDialogFormat(text.scalar("ITEMS_DIALOG_TEXT1"),{str(text.scalar("BTN_BUY")),str(text.scalar("BTN_SELL")),str(text.scalar("BTN_IDENTIFY")),str(text.scalar("BTN_FIX"))}),{0,101,320,146},{8,109,312,138});
    const auto &c=p.party.member(p.roster,selection.source);
    const auto &items=repair?*xeenInventoryItems(c,selection.category):p.serviceEconomy->wares[0][xeenIronworksShop(map)][static_cast<unsigned>(selection.category)];
    std::vector<std::string> args{str(text.table("CATEGORY_NAMES")[static_cast<unsigned>(selection.category)]),repair?std::string(c.name):n(gold(p))};
    if(repair) args.push_back(str(text.scalar("COST")));
    else args.insert(args.begin()+1,"");
    for(unsigned i=0;i<9;++i) {
        const auto description=catalog.describe(selection.category,items[i]);
        if(description.empty) args.push_back(i==0?str(text.scalar("NO_ITEMS_AVAILABLE")):"");
        else {
            const auto cost=xeenItemPrice(selection.category,items[i],action,c.skills[8]).value_or(0);
            args.push_back(xeenDialogFormat(text.scalar("ITEMS_DIALOG_LINE2"),{n(selection.slot==i?15:0),n(i+1),description.coloredName(selection.slot==i?15:0),n(std::max(1u,cost))}));
        }
    }
    frame=render(frame,font,xeenDialogFormat(repair?text.scalar("X_FOR_Y"):text.scalar("AVAILABLE_GOLD_COST"),args),{0,0,320,108},{8,8,312,100});
    if(draw) {
        for(const auto &hit:xeenBuyInput(action).hits) if(hit.button) {const auto &b=*hit.button;draw(frame,b.resource,b.frame,b.x,b.y);}
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
IndexedFrame drawXeenBuy(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,const XeenItemCatalog &catalog,
        const XeenPartyState &p,const XeenInventorySelection &selection,bool repair,const XeenDialogSpriteDraw &draw) {
    return drawXeenBuy(text,base,font,catalog,p,selection,repair?XeenItemService::Fix:XeenItemService::Buy,draw);
}
unsigned xeenCombatHealthColor(int hp,int maxHp) {
    // DOS 0x2e211..0x2e272; the middle color begins at floor(maxHP/4).
    return hp==maxHp?15:hp>=maxHp/4?9:32;
}
std::string xeenCombatPanelText(const XeenDosText &text,const std::array<XeenCombatPanelRow,3> &rows) {
    std::vector<std::string> args;
    constexpr const char *templates[]{"COMBAT_ROW_1","COMBAT_ROW_2","COMBAT_ROW_3"};
    for(unsigned i=0;i<3;++i)args.push_back(rows[i].name.empty()?std::string{}:
        xeenDialogFormat(text.scalar(templates[i]),{n(xeenCombatHealthColor(rows[i].hp,rows[i].maxHp)),rows[i].name}));
    return xeenDialogFormat(text.scalar("COMBAT_DETAILS"),args);
}
IndexedFrame drawXeenCombatPanel(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,
        const std::array<XeenCombatPanelRow,3> &rows,std::optional<unsigned> selected,const XeenDialogSpriteDraw &draw) {
    auto frame=render(base,font,xeenCombatPanelText(text,rows),{225,1,320,73},{233,9,312,65});
    if(selected && *selected<3 && draw)draw(frame,"combat.icn",32,233,27+10*int(*selected));
    return frame;
}
DialogInput xeenConfirmInput(bool large) {
    DialogInput input;
    button(input,large?120:129,large?133:112,large?144:153,large?153:132,'y',"confirm.icn",0);
    button(input,large?176:185,large?133:112,large?200:209,large?153:132,'n',"confirm.icn",2);
    input.keys.push_back(InputKey::Escape);return input;
}
std::string xeenIdentifiedDetails(const XeenDosText &text,const XeenCharacter &owner,
        XeenInventoryCategory category,const XeenItem &item,const XeenLearnedSpellNames &names) {
    using namespace item_tables;
    const auto none=str(text.scalar("FIELD_NONE"));
    std::vector<std::string> fields(8,none);
    std::string classes;
    for(unsigned i=0;i<10;++i) {
        auto c=owner;c.characterClass=static_cast<XeenCharacterClass>(i);
        if(xeenItemProficient(c,category,item.id))classes+=text.table("CLASS_NAMES")[i].substr(0,2)+" ";
    }
    fields[0]=classes.size()==30?str(text.scalar("ALL")):classes;
    if(!xeenItemPrice(category,item,XeenItemService::Identify))return xeenDialogFormat(text.scalar("ITEM_DETAILS"),fields);
    const unsigned m=item.material;
    if(category==XeenInventoryCategory::Miscellaneous) {
        if(item.id<std::size(miscSpells))fields[7]=names.names.at(miscSpells[item.id]);
    } else {
        if(m>=59) {
            unsigned index=0;while(index+1<std::size(attributeCategories) && unsigned(attributeCategories[index])<m-59)++index;
            fields[6]=xeenDialogFormat(text.scalar("ATTR_XY_BONUS"),{n(attributes[m-59]),text.table("ATTRIBUTE_NAMES")[index]});
        }
        const bool weapon=category==XeenInventoryCategory::Weapons;
        if(m<(weapon?37u:36u) && (weapon?elemental[m]:resistances[m])>0) {
            unsigned index=0;while(index+1<std::size(elementCategories) && unsigned(elementCategories[index])<m)++index;
            fields[weapon?3:4]=xeenDialogFormat(text.scalar(weapon?"ELEMENTAL_XY_DAMAGE":"ATTR_XY_BONUS"),
                {n(weapon?elemental[m]:resistances[m]),text.table("ELEMENTAL_NAMES")[index]});
        }
        if(weapon) {
            const int bonus=m>=37 && m<=58?metalDamage[m-37]:0;
            fields[2]=xeenDialogFormat(text.scalar("DAMAGE_X_TO_Y"),{n(weaponBase[item.id]+bonus),n(weaponBase[item.id]*weaponMultiplier[item.id]+bonus)});
            if(m>=37 && m<=58)fields[1]=xeenDialogFormat(text.scalar("SIGNED_BONUS"),{n(metalHit[m-37])});
            if((item.state&63)>0 && (item.state&63)<7)fields[7]=xeenDialogFormat(text.scalar("EFFECTIVE_AGAINST"),{text.table("EFFECTIVENESS_NAMES")[item.state&63]});
        } else if(category==XeenInventoryCategory::Armor)
            fields[5]=xeenDialogFormat(text.scalar("SIGNED_BONUS"),{n(armorAc[item.id]+(m>=37 && m<=58?metalAc[m-37]:0))});
    }
    return xeenDialogFormat(text.scalar("ITEM_DETAILS"),fields);
}
IndexedFrame drawXeenIdentifiedDetails(const IndexedFrame &base,const XeenFontFormat &font,const std::string &message) {
    return render(base,font,message,{0,7,320,138},{8,15,312,130});
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
        n(condition<8?9:condition<12?32:condition<16?6:15),str(conditions(text,c)[condition]),
        condition==16 && p.encounterContext && p.encounterContext->effects[XeenPartyEffect::Bless]?str(text.scalar("PLUS_14")):"",
        condition==16 && p.encounterContext && p.encounterContext->effects[XeenPartyEffect::PowerShield]?str(text.scalar("PLUS_14")):"",
        condition==16 && p.encounterContext && p.encounterContext->effects[XeenPartyEffect::HolyBonus]?str(text.scalar("PLUS_14")):"",
        condition==16 && p.encounterContext && p.encounterContext->effects[XeenPartyEffect::Heroism]?str(text.scalar("PLUS_14")):""});
    auto frame=render(base,font,xeenDialogFormat(text.scalar("CHARACTER_TEMPLATE"),{details}),{0,0,320,146},{8,8,312,138});
    if(draw) {
        constexpr int x[]{2,53,104,169};
        for(unsigned col=0;col<4;++col) for(unsigned row=0;row<5;++row) draw(frame,"view.icn",(col*5+row)*2,x[col]+8,24+row*23);
        for(unsigned row=0;row<4;++row) draw(frame,"view.icn",40+row*2,285,11+row*32);
        if(cursor<20) { constexpr int cx[]{9,60,111,176};draw(frame,"view.icn",blink?49:48,cx[cursor/5],23+23*(cursor%5)); }
    }
    highlight(frame,member,draw);return frame;
}
// Adapted from dialogs_quick_ref.cpp / dialogs_info.cpp at pinned ScummVM
// 6814ee9b (ScummVM developers, GPL-3.0-or-later). DOS format signatures
// and alignment fragments come from the bounded installed module.
std::string xeenQuickReferenceText(const XeenDosText &text,const XeenPartyState &p,const std::vector<std::size_t> &members) {
    if(members.size()>8)throw std::invalid_argument("Quick Reference member count exceeds window");
    std::vector<std::string> args(8);
    for(unsigned row=0;row<members.size();++row) {
        const auto &c=p.party.member(p.roster,members[row]);const auto *in=inputs(p,c);const auto ctx=context(p);
        const auto &name=text.table("CLASS_NAMES").at(unsigned(c.characterClass));
        const auto condition=unsigned(c.worstCondition());const auto &status=conditions(text,c).at(condition);
        if(name.size()<3 || status.size()<4)throw std::invalid_argument("Short Quick Reference abbreviation");
        const auto ac=R::sheetArmorClass(c,in,ctx);
        args[row]=xeenDialogFormat(text.scalar("QUICK_REF_LINE"),{
            n(row*10+24),n(row+1),c.name,name.substr(0,1),name.substr(1,1),name.substr(2,1),
            n(R::statColor(c.currentLevel(),c.permanentLevel)),n(unsigned(c.permanentLevel)),
            n(R::statColor(c.currentHp,R::maxHp(c,ctx))),n(int(c.currentHp)),
            n(R::statColor(c.currentSp,R::maxSp(c,ctx))),n(int(c.currentSp)),
            n(R::statColor(ac,R::sheetArmorClass(c,in,ctx,true))),n(ac),
            n(condition<8?9:condition<12?32:condition<16?6:15),
            status.substr(0,1),status.substr(1,1),status.substr(2,1),status.substr(3,1)});
    }
    const auto food=p.party.size()?std::uint16_t(p.food)/p.party.size()/3:0;
    args.insert(args.end(),{n(gold(p)),n(gems(p)),n(food),str(text.scalar(food==1?"DAY_SINGULAR":"DAY_PLURAL"))});
    return xeenDialogFormat(text.scalar("QUICK_REFERENCE"),args);
}
IndexedFrame drawXeenQuickReference(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,
        const XeenPartyState &p,const std::vector<std::size_t> &members) {
    return render(base,font,"\1"+xeenQuickReferenceText(text,p,members),{0,0,320,146},{8,8,312,138});
}
XeenDialogPopup xeenInfoPopup(const XeenDosText &text,const XeenGameplayContext &ctx) {
    std::string effects;unsigned rows=0;
    const auto left=str(text.scalar("INFO_ALIGN_LEFT")),right=str(text.scalar("INFO_ALIGN_RIGHT"));
    if(ctx.lightAndResistances[0]) {
        effects=xeenDialogFormat(text.scalar("LIGHT_COUNT_TEXT"),{n(unsigned(ctx.lightAndResistances[0]))});++rows;
    }
    constexpr const char *resistance[]{"FIRE_RESISTANCE_TEXT","ELECTRICITY_RESISTANCE_TEXT","COLD_RESISTANCE_TEXT","POISON_RESISTANCE_TEXT"};
    for(unsigned i=0;i<4;++i)if(ctx.lightAndResistances[i+2]) {
        effects+=xeenDialogFormat(text.scalar(resistance[i]),{std::string(1,rows?'\1':'\n'),left,right,n(unsigned(ctx.lightAndResistances[i+2]))});++rows;
    }
    constexpr XeenPartyEffect index[]{XeenPartyEffect::Clairvoyance,XeenPartyEffect::Levitate,XeenPartyEffect::WalkOnWater};
    constexpr const char *effect[]{"CLAIRVOYANCE_TEXT","LEVITATE_TEXT","WALK_ON_WATER_TEXT"};
    for(unsigned i=0;i<3;++i)if(ctx.effects[index[i]]) {
        std::vector<std::string> args{std::string(1,rows?'\1':'\n'),left};
        if(i!=2)args.push_back(right); // DOS Walk on Water has no right-aligned value.
        effects+=xeenDialogFormat(text.scalar(effect[i]),args);++rows;
    }
    const unsigned hour=ctx.minutes/60;
    return {xeenDialogFormat(text.scalar("GAME_INFORMATION"),{str(text.scalar("WORLD_GAME_TEXT")),
        text.table("WEEK_DAY_STRINGS").at(ctx.day%10),n(hour>12?hour-12:hour?hour:12),n(ctx.minutes%60),
        hour>11?"p":"a",n(unsigned(ctx.day)),n(unsigned(ctx.year)),effects}),
        {88,20,248,112+(rows?int(rows)*9+13:0)}};
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
        if(cell==13) {
            constexpr unsigned order[]{0,1,2,3,4,5,17,6,7,8,9,10,11,12,13,16,14,15};
            for(auto skill:order) if(c.skills[skill]) {
                lines+="\n\t020"+str(text.table("SKILL_NAMES")[skill]);++count;
                if(skill==0) { int bonus=2*c.currentLevel()+(c.characterClass==XeenCharacterClass::Ninja?15:c.characterClass==XeenCharacterClass::Robber?30:0);
                    bonus+=c.race==XeenRace::Elf || c.race==XeenRace::Gnome?10:c.race==XeenRace::Dwarf?5:c.race==XeenRace::HalfOrc?-10:0;
                    lines+=n(std::max(0,bonus+R::equipmentBonus(c,10))); }
            }
        } else if(cell==19) for(unsigned i=0;i<16;++i) if(c.conditions[i]) {
            lines+="\n\t020"+str(conditions(text,c)[i]);if(i<12) lines+="\t095-"+n(unsigned(c.conditions[i]));++count;
        }
        if(!count) {lines=cell==13?str(text.scalar("NONE")):"\n\t020"+str(text.scalar("GOOD"));count=1;}
        if(cell==19 && p.encounterContext) {
            constexpr XeenPartyEffect indices[]{XeenPartyEffect::Bless,XeenPartyEffect::PowerShield,XeenPartyEffect::HolyBonus,XeenPartyEffect::Heroism};
            constexpr const char *names[]{"BLESSED","POWER_SHIELD","HOLY_BONUS","HEROISM"};
            for(unsigned i=0;i<4;++i)if(p.encounterContext->effects[indices[i]]) {
                lines+=xeenDialogFormat(text.scalar(names[i]),{n(unsigned(p.encounterContext->effects[indices[i]]))});++count;
            }
        }
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
IndexedFrame drawXeenErrorScroll(const IndexedFrame &base,const XeenFontFormat &font,const std::string &message,bool startupColors) {
    return render(base,font,"\x03" "c\v010\t000"+message,{52,149,268,198},{60,157,260,190},true,startupColors);
}
IndexedFrame drawXeenConfirm(const IndexedFrame &base,const XeenFontFormat &font,const std::string &message,bool large,const XeenDialogSpriteDraw &draw,bool startupColors,bool stopAtBottom) {
    auto frame=render(base,font,message,large?XeenTextRect{65,23,250,163}:XeenTextRect{99,59,237,141},large?XeenTextRect{73,31,242,155}:XeenTextRect{107,67,229,133},true,startupColors,stopAtBottom);
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
        const XeenPartyState &p,const XeenInventorySelection &selection,const XeenDialogSpriteDraw &draw,unsigned mode) {
    const bool misc=selection.category==XeenInventoryCategory::Miscellaneous;
    auto frame=render(base,font,mode?xeenDialogFormat(text.scalar("ITEMS_DIALOG_TEXT2"),{str(text.scalar(mode==6?"BTN_ENCHANT":mode==4?"BTN_RECHARGE":"BTN_GOLD"))}):xeenDialogFormat(text.scalar("ITEMS_DIALOG_TEXT1"),{str(misc?text.scalar("BTN_USE"):text.scalar("BTN_EQUIP")),str(text.scalar("BTN_REMOVE")),str(text.scalar("BTN_DISCARD")),str(text.scalar("BTN_QUEST"))}),{0,101,320,146},{8,109,312,138});
    const auto &c=p.party.member(p.roster,selection.source);
    const auto &items=*xeenInventoryItems(c,selection.category);
    std::vector<std::string> args{misc?"\x03l":"\x03" "c",str(text.table("CATEGORY_NAMES")[static_cast<unsigned>(selection.category)]),c.name,str(text.table("CLASS_NAMES")[static_cast<unsigned>(c.characterClass)]),misc?str(text.scalar("FMT_CHARGES")):" "};
    for(unsigned i=0;i<9;++i) {
        const auto description=catalog.describe(selection.category,items[i]);
        if(description.empty) args.push_back(i==0?str(text.scalar("NO_ITEMS_AVAILABLE")):"");
        else {
            std::vector<std::string> line{n(selection.slot==i?15:0),n(i+1),description.coloredName(selection.slot==i?15:0)};
            if(mode==11)line.push_back(n(xeenItemPrice(selection.category,items[i],XeenItemService::Buy).value_or(1)));
            else if(misc)line.push_back(n(description.counter));
            args.push_back(xeenDialogFormat(misc || mode==11?text.scalar("ITEMS_DIALOG_LINE2"):text.scalar("ITEMS_DIALOG_LINE1"),line));
        }
    }
    if(mode) {
        args.erase(args.begin(),args.begin()+5);
        args.insert(args.begin(),{str(text.table("CATEGORY_NAMES")[static_cast<unsigned>(selection.category)]),c.name,str(text.scalar(mode==11?"COST":"CHARGES"))});
    }
    frame=render(frame,font,xeenDialogFormat(text.scalar(mode?"X_FOR_Y":"X_FOR_THE_Y"),args),{0,0,320,108},{8,8,312,100});
    if(draw) {
        if(mode)for(unsigned i=0;i<6;++i)draw(frame,"buy.icn",i*2,i==5?284:12+i*34,109);
        else for(unsigned i=0;i<9;++i) draw(frame,"items.icn",i==4&&misc?18:i*2,12+i*34,109);
        if(!misc) for(unsigned i=0;i<9;++i) if(items[i].id) draw(frame,"equip.icn",xeenItemProficient(c,selection.category,items[i].id)?unsigned(items[i].frame):14,8,18+i*9);
    }
    highlight(frame,selection.source,draw);return frame;
}
}
