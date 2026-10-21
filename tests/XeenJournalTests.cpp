#include "XeenDosTextTestSupport.h"
#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include <iostream>
using namespace mmodern;
using training_test::check;
namespace {
void unit() {
    const auto &text=dos_test::text();
    std::vector<std::string> notes(184),awards(88);
    for(unsigned i=0;i<notes.size();++i)notes[i]="note"+std::to_string(i);
    for(unsigned i=0;i<awards.size();++i)awards[i]="award"+std::to_string(i);
    awards[9]="%d wins";awards[17]="Legendary %s";
    XeenQuestDisplay p;
    for(auto page:{XeenJournalPage::Items,XeenJournalPage::Quests,XeenJournalPage::Notes})
        check(xeenQuestLines(text,p,page,notes).empty(),"empty page acquired rows");
    p.items[0]=1;p.items[17]=1;p.items[26]=2;
    for(unsigned i=79;i<=84;++i)p.items[i]=3;
    auto lines=xeenQuestLines(text,p,XeenJournalPage::Items,notes);
    check(lines.size()==11 && lines[0]==text.scalar("CLOUDS_OF_XEEN_LINE") &&
        lines[1]==text.table("QUEST_ITEM_NAMES")[0] && lines[4]==text.scalar("DARKSIDE_OF_XEEN_LINE"),"item titles/order");
    for(auto pair:{std::pair{2u,17u},std::pair{3u,26u},std::pair{5u,79u},std::pair{6u,80u},
            std::pair{7u,81u},std::pair{8u,82u},std::pair{9u,83u},std::pair{10u,84u}})
        check(lines[pair.first]==xeenDialogFormat(text.scalar("QUEST_ITEM_COUNT"),
            {std::to_string(p.items[pair.second]),text.table("QUEST_ITEM_NAMES")[pair.second],pair.second==17?" ":"s"}),"multi-count singular/plural");
    p={};p.items[35]=4;
    lines=xeenQuestLines(text,p,XeenJournalPage::Items,notes);
    check(lines.size()==2 && lines[0]==text.scalar("DARKSIDE_OF_XEEN_LINE") && lines[1]==text.table("QUEST_ITEM_NAMES")[35],"Darkside-only page");
    p={};p.quests[0]=true;p.quests[1]=true;p.quests[29]=true;p.quests[30]=true;p.quests[56]=true;
    check(xeenQuestLines(text,p,XeenJournalPage::Quests,notes)==std::vector<std::string>{
        std::string(text.scalar("CLOUDS_OF_XEEN_LINE")),"note0","note28",std::string(text.scalar("DARKSIDE_OF_XEEN_LINE")),"note29","note55"},"quest flag range/offset/titles");
    p.notes[0]=true;p.notes[1]=true;p.notes[72]=true;p.notes[73]=true;p.notes[128]=true;
    check(xeenQuestLines(text,p,XeenJournalPage::Notes,notes)==std::vector<std::string>{
        std::string(text.scalar("CLOUDS_OF_XEEN_LINE")),"note56","note127",std::string(text.scalar("DARKSIDE_OF_XEEN_LINE")),"note128","note183"},"Auto Notes World offset/boundaries");
    XeenPartyState party;party.questItems.eventGrant(17);party.questFlags.set(2);party.questFlags.set(0);
    party.worldFlags.emplace();party.worldFlags->values[0]=true;party.worldFlags->values[127]=true;
    const auto display=xeenQuestDisplay(party);
    check(display.items[17]==1 && display.items[35]==0 && display.quests[2] && !display.quests[30] &&
        !display.notes[0] && display.notes[127] && display.notes[128],"Clouds adapter/quest-zero alias");
    XeenCharacter c;c.awards[9]=231;c.awards[17]=1;c.awards[73]=1;c.awards[81]=1;
    c.awards[126]=2;c.awards[127]=3;
    lines=xeenAwardLines(text,c,awards);
    check(lines==std::vector<std::string>{"231 wins","Legendary "+text.table("RACE_NAMES")[unsigned(c.race)],"award73","award81"},"award lookup/full Warzone byte/race");
    c.awards.fill(0);check(xeenAwardLines(text,c,awards).empty(),"No Awards source");
    for(auto page:{XeenJournalPage::Items,XeenJournalPage::Quests,XeenJournalPage::Notes,XeenJournalPage::Awards}) {
        XeenJournalUi ui{page};
        const std::size_t limit=page==XeenJournalPage::Awards?11:19;
        for(unsigned i=0;i<50;++i)ui.key(InputKey::Down,20);check(ui.top==limit,"Down upper limit");
        ui.key('u',20);ui.key('d',20);ui.key(InputKey::F1,20);check(ui.top==limit,"non-DOS keys scroll");
        ui.key(InputKey::Up,20);check(ui.top==limit-1,"Up single row");
        if(page==XeenJournalPage::Awards) {
            for(auto k:{InputKey::Home,InputKey::End,InputKey::PageUp,InputKey::PageDown,'i'+0u,'q'+0u,'a'+0u})ui.key(k,20);
            check(ui.top==10 && ui.page==page,"Awards accepts quest-only keys");
        } else {
            ui.key(InputKey::Home,20);check(ui.top==0,"Home");
            ui.key(InputKey::PageDown,20);check(ui.top==3,"Page Down three");
            ui.key(InputKey::PageUp,20);check(ui.top==0,"Page Up three");
            ui.key(InputKey::End,20);check(ui.top==19,"End");
            ui.key(InputKey::PageDown,20);check(ui.top==19,"Page Down clamp");
            for(auto k:{'i','q','a'}) {ui.top=10;ui.key(k,20);check(ui.top==0,"page reset");}
        }
        for(unsigned i=0;i<50;++i)ui.key(InputKey::Up,20);check(ui.top==0,"Up lower limit");
        ui.key(InputKey::PageUp,0);ui.key(InputKey::End,0);ui.key(InputKey::Down,0);check(ui.top==0,"empty scroll");
    }
    for(bool award:{false,true}) {
        const auto input=xeenJournalInput(award);
        check(input.hits.size()==(award?3:6) && !input.key('u') && !input.key('d') && !input.key(InputKey::F1),"journal input keys");
        for(const auto &h:input.hits) {
            check(input.click(h.left,h.top) && input.click(h.right-1,h.bottom-1) && !input.click(h.right,h.bottom),"journal button bounds");
            check(h.button && h.button->pressedFrame()==h.button->frame+1,"journal pressed frames");
        }
    }
    for(bool award:{false,true}) {
        std::vector<std::uint8_t> bytes(award?88:184,0);
        check(xeenJournalStrings(bytes,award).size()==bytes.size(),"journal string parser");
        bytes.pop_back();bool failed=false;try{xeenJournalStrings(bytes,award);}catch(...){failed=true;}
        check(failed,"short journal text accepted");
    }
}
void original(const char *path) {
    training_test::Inputs in(*xeenTestInstallationDetector().detect(path));
    const auto &text=in.assets.uiText();
    const auto notes=xeenJournalStrings(in.assets.readArchiveResource("qnotes.bin",XeenSceneArchive::DarksideOnly),false);
    const auto awards=xeenJournalStrings(in.assets.readArchiveResource("award.bin",XeenSceneArchive::DarksideOnly),true);
    const auto draw=[&](auto &f,const char *n,unsigned i,int x,int y){
        in.assets.drawDialogSprite(f,n,i,x,y,XeenSceneArchive::DarksideOnly);};
    XeenCharacter member;member.name="Tester";
    for(auto page:{XeenJournalPage::Items,XeenJournalPage::Quests,XeenJournalPage::Notes,XeenJournalPage::Awards}) {
        const auto message=xeenJournalText(text,{page}, {},page==XeenJournalPage::Awards?&member:nullptr);
        const auto expected=page==XeenJournalPage::Items?"No Quest Items":page==XeenJournalPage::Quests?"No Current Quests":page==XeenJournalPage::Notes?"No Auto Notes":"No Awards";
        check(message.find(expected)!=std::string::npos,"original empty-page text");
        check(drawXeenJournal(text,training_test::frame(),in.font,{page},{},draw,page==XeenJournalPage::Awards?&member:nullptr).isValid(),"original empty layout");
    }
    // Every original name/note can reach the same renderer, including long notes.
    XeenQuestDisplay all;all.items.fill(1);all.quests.fill(true);all.notes.fill(true);
    for(auto page:{XeenJournalPage::Items,XeenJournalPage::Quests,XeenJournalPage::Notes}) {
        const auto rows=xeenQuestLines(text,all,page,notes);
        for(std::size_t top=0;top<rows.size();++top)
            check(drawXeenJournal(text,training_test::frame(),in.font,{page,top},rows,draw).isValid(),"original journal row layout");
    }
    member.awards.fill(1);member.awards[9]=231;
    const auto allAwards=xeenAwardLines(text,member,awards);
    for(std::size_t top=0;top+9<=allAwards.size();++top)
        check(drawXeenJournal(text,training_test::frame(),in.font,{XeenJournalPage::Awards,top},allAwards,draw,&member).isValid(),"original awards row layout");
    auto source=in.service();source.camera={23,9,11,XeenDirection::West};
    for(auto &a:source.map(23).monsters->slots) {a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    auto &monster=source.map(23).monsters->slots[5];monster.x=8;monster.y=11;monster.lifecycle=XeenActorLifecycle::Present;monster.accounted=false;
    source.questItems[17]=3;source.questItems[26]=1;source.questFlags[2]=true;
    source.journey->worldFlags->values[1]=true;source.journey->worldFlags->values[72]=true;
    for(unsigned id=0;id<20;++id)source.characters[source.activeRosterIds[0]].awards[id]=id==9?231:1;
    training_test::Fixture f(in,source);
    f.flow->loadJournalText=[&](bool award){return in.assets.readArchiveResource(award?"award.bin":"qnotes.bin",XeenSceneArchive::DarksideOnly);};
    const auto normalDraw=f.flow->drawDialogSprite;
    f.flow->drawDialogSprite=[&](auto &frame,const char *name,unsigned id,int x,int y){
        if(std::string(name)=="quest.icn" || std::string(name)=="award.icn")draw(frame,name,id,x,y);else normalDraw(frame,name,id,x,y);};
    const auto before=XeenSaveFormat::encode(f.snapshot());
    const auto main=xeenMainScreenClick(290,100,MainScreen::Exploration);check(main && std::holds_alternative<QuestsAction>(*main),"main-screen Quest route");
    f.act(*main);
    for(auto key:{'i','q','a'}) {
        const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
        const auto b=input->button(unsigned(key));f.act(*input->click(b->x,b->y));
        const auto page=key=='i'?XeenJournalPage::Items:key=='q'?XeenJournalPage::Quests:XeenJournalPage::Notes;
        const auto rows=xeenQuestLines(text,xeenQuestDisplay(f.p),page,notes);
        const auto expectedFrame=drawXeenJournal(text,training_test::frame(),in.font,{page},rows,draw);
        // Compare the entire covered area; the bottom party strip is the caller's.
        check(std::equal(expectedFrame.pixels.begin(),expectedFrame.pixels.begin()+320*146,f.flow->frame().pixels.begin()),"live quest page differs from original layout");
        f.act(DialogKeyAction{InputKey::End});f.act(DialogKeyAction{InputKey::Home});
    }
    {const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;f.act(*input->click(284,109));}
    check(XeenSaveFormat::encode(f.snapshot())==before,"main Quests changes state/time/RNG");
    const auto dialogs=[&] {
        const auto *combat=f.flow->encounter()->combat();
        const auto beforeDialogs=combat?std::vector<std::uint8_t>{}:XeenSaveFormat::encode(f.snapshot());
        const auto beforeCharacters=f.p.roster.characters();const auto beforeContext=f.p.encounterContext;
        const auto beforeQuests=xeenQuestDisplay(f.p);const auto beforeWorld=f.p.worldFlags;
        const auto beforeTreasure=f.p.purse;const auto beforeFood=f.p.food;
        const auto beforeEconomy=f.p.serviceEconomy;const auto beforeMembers=f.p.party.activeRosterIds();
        const auto beforeCamera=f.c;const auto beforeFlags=f.f.values();
        const auto beforeRandom=f.w.sessionState().journeyRandom();
        const auto actor=combat?combat->participant():-1;
        const auto combatRandom=combat?std::optional<XeenCombatRandom>{combat->random()}:std::nullopt;
        f.act(SelectMemberAction{0});f.act(DialogKeyAction{'i'});
        const auto items=f.flow->frame().pixels;
        f.act(DialogKeyAction{'q'});
        check(f.flow->inputContext(f.flow->frame().presentation()).dialog->hits.size()==6,"Items Quest route");
        f.act(CancelInteractionAction{});check(f.flow->frame().pixels==items,"Quests did not return to Items");
        f.act(CancelInteractionAction{});const auto sheet=f.flow->frame();
        f.act(DialogKeyAction{1015});
        auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
        check(input->hits.size()==3 && !input->key(InputKey::F1),"Awards has member selection");
        const auto awardFrame=drawXeenJournal(text,sheet,in.font,{XeenJournalPage::Awards},
            xeenAwardLines(text,f.p.party.member(f.p.roster,0),awards),draw,&f.p.party.member(f.p.roster,0));
        check(std::equal(awardFrame.pixels.begin(),awardFrame.pixels.begin()+320*146,f.flow->frame().pixels.begin()),"sheet Awards member/layout");
        const auto firstAwards=f.flow->frame().pixels;
        f.act(SelectMemberAction{1});f.act(DialogKeyAction{InputKey::F1+1});f.act(DialogKeyAction{'u'});f.act(DialogKeyAction{'d'});
        check(f.flow->frame().pixels==firstAwards,"Awards changed member or accepted U/D");
        for(unsigned n=0;n<30;++n)f.act(*input->click(250,109));
        const auto awardRows=xeenAwardLines(text,f.p.party.member(f.p.roster,0),awards);
        const auto lastAwards=drawXeenJournal(text,sheet,in.font,{XeenJournalPage::Awards,awardRows.size()-9},awardRows,draw,&f.p.party.member(f.p.roster,0));
        check(lastAwards.pixels==f.flow->frame().pixels,"Awards mouse Down did not stop with nine rows");
        f.act(*input->click(216,109));f.act(DialogKeyAction{InputKey::Down});
        check(lastAwards.pixels==f.flow->frame().pixels,"Awards mouse Up/key Down parity");
        f.act(CancelInteractionAction{});check(f.flow->frame().pixels==drawXeenSheet(text,sheet,in.font,f.p,0,14,false,normalDraw).pixels,"Awards did not return to sheet");
        f.act(CancelInteractionAction{});
        if(!combat)check(XeenSaveFormat::encode(f.snapshot())==beforeDialogs,"nested screens change saved state/time/RNG");
        check(f.p.encounterContext==beforeContext && xeenQuestDisplay(f.p).items==beforeQuests.items &&
            xeenQuestDisplay(f.p).quests==beforeQuests.quests && f.p.worldFlags==beforeWorld &&
            f.p.purse==beforeTreasure && f.p.food==beforeFood &&
            f.p.serviceEconomy==beforeEconomy && f.p.party.activeRosterIds()==beforeMembers &&
            xeen_state::sameCamera(f.c,beforeCamera) && f.f.values()==beforeFlags && f.w.sessionState().journeyRandom()==beforeRandom,
            "nested screens changed party/world/time/RNG");
        for(unsigned owner=0;owner<30;++owner)check(xeen_state::sameCharacter(f.p.roster.at(owner),beforeCharacters[owner]),"nested screen changed a character");
        if(combat)check(combat->participant()==actor && combat->phase()==XeenCombatPhase::PlayerReady && combat->random()==*combatRandom,"nested screen spent combat action/RNG");
    };
    dialogs();
    f.act(WaitAction{});
    for(unsigned n=0;n<1500 && (!f.flow->encounter()->combat() || f.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n) {
        f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);
    }
    check(f.flow->encounter()->combat() && f.flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady,"original combat entry");
    dialogs();
}
}
int main(int argc,char **argv) {try {unit();if(argc>1) {
    std::ofstream log("journal-original-detail.log");auto *previous=std::cout.rdbuf(log.rdbuf());
    try {original(argv[1]);}catch(...) {std::cout.rdbuf(previous);throw;}
    std::cout.rdbuf(previous);
}std::cout<<"Quests/Awards contents, DOS keys, layouts and read-only modal paths passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
