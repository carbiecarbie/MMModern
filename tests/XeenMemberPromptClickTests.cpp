#include "SdlMouseTestSupport.h"
// A portrait click queued while a member prompt redraws its scene reaches the
// prompt through the real SDL loop, as the original keeps polling the same
// portrait buttons across draw3d.
#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
using namespace training_test;
namespace {
XeenSaveSnapshot firstAidSource(Inputs &in) {
    auto s=in.service();s.camera={23,9,11,XeenDirection::East};
    for(auto *actors:{&s.map(23).monsters->slots,&s.map(28).monsters->slots})for(auto &a:*actors) {
        a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
    }
    auto &c=s.characters[s.activeRosterIds[0]];
    c.characterClass=XeenCharacterClass::Cleric;c.hasSpells=true;c.currentSp=100;c.learnedSpells.emplace();c.learnedSpells->fill(0);
    for(auto *items:{&c.weapons,&c.armor,&c.accessories})for(auto &item:*items)item.frame=0;
    c.currentSpell=*XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Clerical,std::uint8_t(XeenLearnedSpell::FirstAid));
    c.learnedSpells->at(c.currentSpell)=1;
    s.characters[s.activeRosterIds[1]].currentHp=1;save_test::clearExplored(s);
    return s;
}
void queue(SDL_Event event) {
    if(SDL_PeepEvents(&event,1,SDL_ADDEVENT,0,0)!=1)throw std::runtime_error("member prompt click queue");
}
// Motion onto the second portrait, then a click with no delay, all queued
// while the prompt's scenery redraw is composed and not yet presented.
void clickPortrait(int x,int y) {
    SDL_Event event{};event.type=SDL_MOUSEMOTION;
    SDL_MouseButtonEvent pos{};sdl_mouse_test::position(pos,x-6,y-4);
    event.motion.windowID=pos.windowID;event.motion.x=pos.x;event.motion.y=pos.y;event.motion.timestamp=SDL_GetTicks();queue(event);
    sdl_mouse_test::position(pos,x,y);event.motion.x=pos.x;event.motion.y=pos.y;queue(event);
    event={};event.type=SDL_MOUSEBUTTONDOWN;event.button.button=SDL_BUTTON_LEFT;event.button.state=SDL_PRESSED;
    sdl_mouse_test::position(event.button,x,y);event.button.timestamp=SDL_GetTicks();queue(event);
    event.type=SDL_MOUSEBUTTONUP;event.button.state=SDL_RELEASED;queue(event);
}
void castTargetClickDuringRedraw(Inputs &in) {
    const auto s=firstAidSource(in);const auto target=s.activeRosterIds[1];
    Fixture f(in,s,true);
    f.act(CastSpellAction{});f.act(SelectMemberAction{0});
    f.flow->beginCycle(++f.cycle);
    const auto first=f.flow->handle(DialogKeyAction{'c'},f.flow->displayedInput(),f.flow->frame().presentation());
    check(f.flow->inputContext(first.presentation()).dialog && f.p.roster.at(target).currentHp==1,"First Aid target prompt absent");
    SdlWindow::FrameUpdateHandler handler=[](const PlayerAction &)->std::optional<IndexedFrame>{throw std::runtime_error("unversioned prompt action");};
    auto &flow=*f.flow;
    handler.displayedInput=[&]{return flow.displayedInput();};
    handler.inputContext=[&](const auto &origin){return flow.inputContext(origin);};
    handler.acceptsFrame=[&](const auto &frame){return flow.acceptsFrame(frame);};
    handler.acceptsInputFrame=[&](const auto &frame){return flow.acceptsInputFrame(frame);};
    handler.completeInputHandoff=[&](const auto &frame){flow.completeInputHandoff(frame);};
    handler.framePresented=[&](const auto &frame){flow.framePresented(frame,true);};
    handler.frameCurrent=[&]{return flow.encounterFrameCurrent();};
    handler.protectAllKeys=flow.journey();
    handler.presentationClock=[&]{return flow.presentationNow();};
    handler.nextPresentationDeadline=[&]{return flow.nextPresentationDeadline();};
    handler.beginCycle=[&](std::uint64_t){f.now+=55;flow.beginCycle(++f.cycle);};
    unsigned redraws=0,loops=0;bool clicked=false;
    handler.withPresentedInput=[&](const PlayerAction &action,std::uint64_t input,const auto &frame)->std::optional<IndexedFrame>{
        return flow.handle(action,input,frame);
    };
    const bool ok=SdlWindow().showInteractive(first,"Member prompt click",handler,[]{return true;},[&]()->std::optional<IndexedFrame>{
        check(++loops<400,"Portrait click during redraw was lost");
        if(f.p.roster.at(target).currentHp>1 && flow.canSave()) {SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return {};}
        auto next=flow.updatePresentation();
        if(next && !clicked) {
            check(next->presentation()!=first.presentation(),"Prompt redraw keeps its frame identity");
            if(++redraws==2){clickPortrait(60,165);clicked=true;}
        } else if(!next && clicked && !flow.canSave() && flow.inputContext(flow.frame().presentation()).dialog &&
            flow.inputContext(flow.frame().presentation()).dialog->anyKey) {
            // Settle a result notice, if any, with an ordinary key.
            SDL_Event key{};key.type=SDL_KEYDOWN;key.key.keysym.sym=SDLK_SPACE;key.key.timestamp=SDL_GetTicks();queue(key);
            key.type=SDL_KEYUP;queue(key);
        }
        return next;
    });
    check(ok && clicked && redraws>=2,"Member prompt SDL loop did not run");
    check(f.p.roster.at(target).currentHp>1,"Portrait click queued during the prompt redraw was not delivered");
    std::cout<<"First Aid portrait click queued during a scenery redraw selected the member\n";
}
}
int main(int argc,char **argv) {try {
    check(argc==2,"usage: member-prompt-click <installation>");
    auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Installation unavailable");
    Inputs in(*installation);
    castTargetClickDuringRedraw(in);
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
