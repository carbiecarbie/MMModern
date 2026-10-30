#include "games/xeen/XeenDialogView.h"
#include "formats/xeen/XeenEventTextFormat.h"
#include <algorithm>
#include <stdexcept>

namespace mmodern {
// Layout adapted from ScummVM 6814ee9b dialogs_quests/awards.cpp and window.cpp
// (ScummVM developers, GPL-3.0-or-later). DOS M77 Part 0 supplies the keys.
XeenQuestDisplay xeenQuestDisplay(const XeenPartyState &p) {
    XeenQuestDisplay result;
    for(unsigned i=0;i<XeenQuestItems::kCount;++i)result.items[i]=p.questItems.at(i);
    for(std::size_t i=0;i<result.quests.size();++i)result.quests[i]=p.questFlags.values()[i];
    if(p.worldFlags)for(unsigned i=1;i<128;++i)result.notes[i]=p.worldFlags->values[i];
    // DOS's flag-128 bit aliases quest flag 0, immediately after world flags.
    result.notes[128]=p.questFlags.isSet(0);
    return result;
}
std::vector<std::string> xeenJournalStrings(const std::vector<std::uint8_t> &bytes,bool awards) {
    if(bytes.empty() || bytes.back()!=0)throw std::invalid_argument("Unterminated journal text resource");
    auto strings=XeenEventTextFormat::parse(bytes);
    if(strings.size()<(awards?88u:184u))throw std::invalid_argument("Incomplete journal text resource");
    for(const auto &s:strings)XeenDosText::validateControls(s);
    return strings;
}
void XeenJournalUi::key(unsigned key,std::size_t count) {
    const auto limit=page==XeenJournalPage::Awards?(count>9?count-9:0):(count?count-1:0);
    top=std::min(top,limit);
    if(page!=XeenJournalPage::Awards) {
        if(key=='i' || key=='q' || key=='a') {
            page=key=='i'?XeenJournalPage::Items:key=='q'?XeenJournalPage::Quests:XeenJournalPage::Notes;
            top=0;return;
        }
        if(key==InputKey::Home)top=0;
        if(key==InputKey::End)top=limit;
        if(key==InputKey::PageUp)top=top>3?top-3:0;
        if(key==InputKey::PageDown)top=std::min(top+3,limit);
    }
    if(key==InputKey::Up && top)--top;
    if(key==InputKey::Down)top=std::min(top+1,limit);
}
std::vector<std::string> xeenQuestLines(const XeenDosText &text,const XeenQuestDisplay &p,
        XeenJournalPage page,const std::vector<std::string> &notes) {
    std::vector<std::string> lines;
    bool clouds=false,dark=false;
    const auto title=[&](bool isDark) {
        auto &shown=isDark?dark:clouds;
        if(!shown) {lines.emplace_back(text.scalar(isDark?"DARKSIDE_OF_XEEN_LINE":"CLOUDS_OF_XEEN_LINE"));shown=true;}
    };
    if(page==XeenJournalPage::Items) {
        for(unsigned i=0;i<85;++i)if(p.items[i]) {
            title(i>=35);
            const auto &name=text.table("QUEST_ITEM_NAMES").at(i);
            lines.push_back(i==17 || i==26 || i>=79?
                xeenDialogFormat(text.scalar("QUEST_ITEM_COUNT"),{std::to_string(p.items[i]),name,p.items[i]>1?"s":" "}):name);
        }
    } else if(page==XeenJournalPage::Quests) {
        for(unsigned flag=1;flag<=56;++flag)if(p.quests[flag]) {title(flag>=30);lines.push_back(notes.at(flag-1));}
    } else if(page==XeenJournalPage::Notes) {
        for(unsigned flag=1;flag<=128;++flag)if(p.notes[flag]) {title(flag>=73);lines.push_back(notes.at(flag-1+56));}
    } else throw std::invalid_argument("Awards is not a quest page");
    return lines;
}
std::vector<std::string> xeenAwardLines(const XeenDosText &text,const XeenCharacter &c,
        const std::vector<std::string> &awards) {
    std::vector<std::string> lines;
    for(unsigned id=0;id<88;++id)if(c.awards[XeenCharacter::awardIndex(id)]) {
        auto line=awards.at(id);
        if(id==9)line=xeenDialogFormat(line,{std::to_string(c.awards[9])});
        if(id==17)line=xeenDialogFormat(line,{text.table("RACE_NAMES").at(unsigned(c.race))});
        lines.push_back(std::move(line));
    }
    return lines;
}
DialogInput xeenJournalInput(bool awards) {
    DialogInput input;
    constexpr int xs[]{12,80,148,216,250,284};
    constexpr unsigned keys[]{'i','q','a',InputKey::Up,InputKey::Down,InputKey::Escape};
    for(unsigned i=awards?3:0;i<6;++i) {
        input.hits.push_back({xs[i],109,xs[i]+24,129,keys[i],
            InputButton{awards?"award.icn":"quest.icn",(awards?i-3:i)*2,xs[i],109}});
        input.keys.push_back(keys[i]);
    }
    if(!awards)for(auto key:{InputKey::PageUp,InputKey::PageDown,InputKey::Home,InputKey::End})input.keys.push_back(key);
    return input;
}
std::string xeenJournalText(const XeenDosText &text,const XeenJournalUi &ui,
        const std::vector<std::string> &lines,const XeenCharacter *member) {
    const bool awards=ui.page==XeenJournalPage::Awards;
    std::vector<std::string> args;
    if(awards) {
        if(!member)throw std::invalid_argument("Awards requires a member");
        args={member->name,text.table("CLASS_NAMES").at(unsigned(member->characterClass))};
    }
    if(ui.page==XeenJournalPage::Items && lines.empty())return std::string(text.scalar("NO_QUEST_ITEMS"));
    const auto visible=ui.page==XeenJournalPage::Quests?3u:9u;
    for(unsigned i=0;i<visible;++i) {
        const auto row=ui.top+i;
        if(row<lines.size())args.push_back(lines[row]);
        else if(lines.empty() && i==1 && ui.page!=XeenJournalPage::Items)
            args.emplace_back(text.scalar(awards?"NO_AWARDS":ui.page==XeenJournalPage::Quests?"NO_CURRENT_QUESTS":"NO_AUTO_NOTES"));
        else args.push_back(ui.page==XeenJournalPage::Items?std::string(text.scalar("QUEST_ITEM_BLANK")):"");
    }
    return xeenDialogFormat(text.scalar(awards?"AWARDS_FOR":ui.page==XeenJournalPage::Items?"QUEST_ITEMS_DATA":
        ui.page==XeenJournalPage::Quests?"CURRENT_QUESTS_DATA":"AUTO_NOTES_DATA"),args);
}
IndexedFrame drawXeenJournal(const XeenDosText &text,const IndexedFrame &base,const XeenFontFormat &font,
        const XeenJournalUi &ui,const std::vector<std::string> &lines,const XeenDialogSpriteDraw &draw,
        const XeenCharacter *member) {
    const bool awards=ui.page==XeenJournalPage::Awards;
    XeenTextRenderOptions options;options.originalControls=options.drawWindow=true;
    options.windowBounds={0,101,320,146};options.bounds={8,109,312,138};options.x=8;options.y=109;
    auto toolbar=XeenTextRenderer(font).render(base,std::string(text.scalar(awards?"AWARDS_TEXT":"QUESTS_DIALOG_TEXT")),options);
    if(toolbar.pages.size()!=1 || !toolbar.diagnostics.empty())throw std::runtime_error("Journal toolbar overflow");
    auto frame=std::move(toolbar.pages.front());
    options.stopAtBottom=true;
    options.windowBounds={0,0,320,108};options.bounds={8,8,312,100};options.x=8;options.y=8;
    auto contents=XeenTextRenderer(font).render(frame,xeenJournalText(text,ui,lines,member),options);
    if(contents.pages.size()!=1 || !contents.diagnostics.empty())throw std::runtime_error("Journal contents overflow");
    frame=std::move(contents.pages.front());
    if(draw)for(const auto &hit:xeenJournalInput(awards).hits) {
        const auto &b=*hit.button;draw(frame,b.resource,b.frame,b.x,b.y);
    }
    return frame;
}
}
