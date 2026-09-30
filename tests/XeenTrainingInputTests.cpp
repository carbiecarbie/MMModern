#include "XeenTrainingTestSupport.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdlib>
#include <iostream>
#include <new>

namespace input_allocation {bool failNext=false,failed=false;}
void *operator new(std::size_t size) {
    if(input_allocation::failNext){input_allocation::failNext=false;input_allocation::failed=true;throw std::bad_alloc();}
    if(auto *p=std::malloc(size?size:1))return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept {std::free(p);}
void operator delete(void *p,std::size_t) noexcept {std::free(p);}
void *operator new[](std::size_t size){return ::operator new(size);}
void operator delete[](void *p) noexcept {::operator delete(p);}
void operator delete[](void *p,std::size_t) noexcept {::operator delete(p);}
extern "C" Uint32 __wrap_SDL_GetTicks(){return 100;}
using namespace training_test;
namespace {
struct Owners {
    std::array<XeenCharacter,30> characters;
    std::array<XeenCombatInputs,30> inputs;
    XeenMonsterTreasure treasure;
    XeenGameplayContext context;
    XeenServiceEconomy economy;
    XeenJourneyRandomState random;
    explicit Owners(const Fixture &f):treasure(*f.p.monsterTreasure),context(*f.p.encounterContext),
        economy(*f.p.serviceEconomy),random(*f.w.sessionState().journeyRandom()) {
        for(unsigned n=0;n<30;++n){characters[n]=f.p.roster.at(n);inputs[n]=*f.p.roster.combatInputs(n);}
    }
    void unchanged(const Fixture &f) const {
        for(unsigned n=0;n<30;++n)check(xeen_state::sameCharacter(characters[n],f.p.roster.at(n)) &&
            xeen_state::sameInputs(inputs[n],*f.p.roster.combatInputs(n)),"stale frame changed level/XP/HP/SP/reset/inactive owner");
        check(treasure==*f.p.monsterTreasure && context==*f.p.encounterContext && economy==*f.p.serviceEconomy &&
            random==*f.w.sessionState().journeyRandom(),"stale frame published payment/day/stock/bank/RNG");
    }
};
auto state(Inputs &in) {
    auto s=in.service();
    for(unsigned id:{0u,18u,2u}) {
        s.characters[id].temporaryLevel=2;s.characters[id].endurance.temporary=5;
        s.journey->supplements[id].inputs.might.temporary=7;
    }
    return s;
}
void quote(Fixture &f){f.enter();f.act(SelectMemberAction{1});f.act(AcknowledgeAction{});check(XeenTrainingTestAccess::quote(*f.flow),"quote not reached");}
void replay(Fixture &f,const IndexedFrame::Presentation &frame,std::uint64_t input) {
    f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(AcknowledgeAction{},input,frame));
}
void paid(const Fixture &f) {
    check(f.p.roster.at(18).permanentLevel==4 && f.p.roster.combatInputs(18)->experience==6000 &&
        f.p.monsterTreasure->gold==710 && f.p.encounterContext->day==9 && f.p.roster.at(18).currentHp==64 &&
        f.p.roster.at(18).currentSp==0,"fresh B confirmation did not publish exactly one purchase");
}
void authority(Inputs &in) {
    {
        Fixture f(in,state(in));quote(f);const Owners before(f);
        const auto a=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        input_allocation::failed=false;input_allocation::failNext=true;f.act(AcknowledgeAction{});
        const auto b=f.flow->frame().presentation();
        check(input_allocation::failed && a!=b && input==f.flow->displayedInput() && XeenTrainingTestAccess::quote(*f.flow),
            "confirmation fault did not retain semantics with a replacement concrete frame");
        before.unchanged(f);replay(f,a,input);
        check(XeenTrainingTestAccess::quote(*f.flow),"stale A confirmation authorized replacement B");before.unchanged(f);
        f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(AcknowledgeAction{},input));
        check(XeenTrainingTestAccess::quote(*f.flow),"semantic-only response borrowed the current concrete frame");before.unchanged(f);
        replay(f,b,input);f.prepare();paid(f);replay(f,b,input);paid(f);
        std::cout<<"Confirmation allocation failure: stale A rejected; fresh B purchased once\n";
    }
    for(auto boundary:{XeenTrainingBoundary::BeforeDeparture,XeenTrainingBoundary::BeforeEventSettlement}) {
        Fixture f(in,state(in));f.enter();unsigned calls=0;
        f.flow->trainingBoundary=[&](auto here){if(here==boundary && ++calls<=2)throw std::bad_alloc();};
        f.act(CancelInteractionAction{});const Owners before(f);
        const auto a=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        f.act(AcknowledgeAction{});const auto b=f.flow->frame().presentation();
        check(a!=b && input==f.flow->displayedInput() && calls==2,"departure/settlement retry spent semantic authority");
        before.unchanged(f);replay(f,a,input);check(calls==2,"stale departure/settlement frame reached callback");before.unchanged(f);
        replay(f,b,input);check(f.flow->canSave() && f.p.encounterContext->day==9 && calls==3,"fresh settlement duplicated/lost departure");
    }
    for(unsigned phase=0;phase<3;++phase) {
        Fixture f(in,state(in));quote(f);
        if(phase==1){f.act(AcknowledgeAction{});f.prepare();}
        if(phase==2){f.act(CancelInteractionAction{});f.act(SelectMemberAction{0});f.act(AcknowledgeAction{});}
        const Owners before(f);const auto a=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        const auto text=XeenTrainingTestAccess::text(*f.flow);f.present(f.flow->refresh(true));const auto b=f.flow->frame().presentation();
        check(a!=b && input==f.flow->displayedInput() && text==XeenTrainingTestAccess::text(*f.flow),"cosmetic redraw changed semantic authority/content");
        replay(f,a,input);before.unchanged(f);check(text==XeenTrainingTestAccess::text(*f.flow),"stale cosmetic/result response changed phase");
        replay(f,b,input);
        if(phase)check(XeenTrainingTestAccess::menu(*f.flow),"fresh result/refusal acknowledgement refused");else {f.prepare();paid(f);}
    }
}
void key(SDL_Keycode code,Uint32 type) {
    SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);e.key.timestamp=100;
    check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"native retry input enqueue failed");
}
void edge(){key(SDLK_RETURN,SDL_KEYUP);key(SDLK_RETURN,SDL_KEYDOWN);key(SDLK_RETURN,SDL_KEYUP);}
void native(Inputs &in) {
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
    for(unsigned mode=0;mode<6;++mode) {
        Fixture f(in,state(in));unsigned hookCalls=0;
        if(mode==1 || mode==2) {
            f.enter();f.flow->trainingBoundary=[&](auto here){if(here==(mode==1?XeenTrainingBoundary::BeforeDeparture:
                XeenTrainingBoundary::BeforeEventSettlement) && ++hookCalls<=2)throw std::bad_alloc();};
            f.act(CancelInteractionAction{});
        } else {quote(f);if(mode==4){f.act(AcknowledgeAction{});f.prepare();}
            if(mode==5){f.act(CancelInteractionAction{});f.act(SelectMemberAction{0});f.act(AcknowledgeAction{});}}
        const Owners before(f);const auto a=f.flow->frame().presentation();const auto semantic=*f.flow->displayedInput();
        IndexedFrame::Presentation shown=a,b;unsigned stage=0,dispatches=0,cycles=0;bool failed=false;
        SdlWindow::FrameUpdateHandler handler=[](const PlayerAction &)->std::optional<IndexedFrame>{throw std::runtime_error("unbound native input");};
        handler.protectAllKeys=true;handler.displayedInput=[&]{return f.flow->displayedInput();};
        handler.acceptsFrame=[&](const auto &frame){return f.flow->acceptsFrame(frame);};
        handler.framePresented=[&](const auto &frame){f.flow->framePresented(frame);shown=frame;};
        handler.beginCycle=[&](auto){check(++cycles<1000,"native retry loop bound");f.flow->beginCycle(++f.cycle);};
        const auto dispatch=[&](const PlayerAction &action,std::uint64_t input,const IndexedFrame::Presentation &frame)->std::optional<IndexedFrame> {
            if(dispatches++==0 && mode<3) {
                if(mode==0){input_allocation::failed=false;input_allocation::failNext=true;}
                auto result=f.flow->handle(action,input,frame);failed=mode==0?input_allocation::failed:hookCalls==2;return result;
            }
            return f.flow->handle(action,input,frame);
        };
        handler.withDisplayedInput=[&](const auto &action,std::uint64_t input){return dispatch(action,input,shown);};
        handler.withPresentedInput=dispatch;
        const bool ok=SdlWindow().showInteractive(f.flow->frame(),"Training concrete retry",handler,[]{return true;},[&]()->std::optional<IndexedFrame> {
            if(stage==0){++stage;if(mode<3)edge();return {};}
            if(stage==1) {
                ++stage;std::optional<IndexedFrame> redraw;
                if(mode>=3)redraw=f.flow->refresh(true);else check(failed,"native confirmation/settlement failure was not exercised");
                b=f.flow->frame().presentation();
                check(a!=b && semantic==f.flow->displayedInput(),"native retry/redraw did not preserve semantic identity");before.unchanged(f);
                // Queued in idle after dispatch/upload, before B is acquired.
                // Equal SDL timestamps deliberately defeat a time-only fence.
                edge();return redraw;
            }
            if(stage==2) {
                ++stage;check(shown==b,"replacement B was not acquired");
                if(const auto update=f.flow->updatePresentation())f.present(*update);
                before.unchanged(f);
                check((mode<3?dispatches==1:dispatches==0),"key queued before B acquisition reached gameplay");
                edge();return {};
            }
            auto update=f.flow->updatePresentation();
            if((mode==0 || mode==3) && f.p.roster.at(18).permanentLevel==3)return update;
            if((mode==1 || mode==2) && !f.flow->canSave()) {
                check(f.p.encounterContext->day==9 && hookCalls==3,"fresh B settlement did not publish once");
                return update;
            }
            if(mode==0 || mode==3)paid(f);
            else if(mode>=4){before.unchanged(f);check(XeenTrainingTestAccess::menu(*f.flow),"fresh B result/refusal response refused");}
            else check(f.flow->canSave() && f.p.encounterContext->day==9 && hookCalls==3,"fresh B settlement did not finish once");
            SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return update;
        });
        check(ok,"native stale concrete-frame regression failed");
        std::cout<<"Native concrete-frame mode "<<mode<<": pre-acquisition key rejected; fresh B accepted\n";
    }
}
void menuText(Fixture &f) {
    const auto text=XeenTrainingTestAccess::text(*f.flow);
    check(text.find("Enter: quote")!=std::string::npos && text.find("Escape: depart (one day)")!=std::string::npos &&
        text.find("Enter confirms")==std::string::npos && text.find("Escape cancels")==std::string::npos &&
        text.find("Enter: confirm")==std::string::npos,"eligible member menu contradicts quote/departure controls");
}
void controls(Inputs &in) {
    Fixture f(in,state(in));f.enter();f.act(SelectMemberAction{1});menuText(f);
    f.act(AcknowledgeAction{});auto text=XeenTrainingTestAccess::text(*f.flow);
    check(text.find("Enter: confirm  Escape: cancel")!=std::string::npos && text.find("Enter: quote")==std::string::npos &&
        text.find("Escape: depart")==std::string::npos,"quote controls contradict its confirmation phase");
    f.act(CancelInteractionAction{});menuText(f);f.train(1);menuText(f);
    f.act(SelectMemberAction{4});menuText(f);f.act(SelectMemberAction{1});menuText(f);
    f.act(AcknowledgeAction{});f.act(AcknowledgeAction{});f.prepare();
    text=XeenTrainingTestAccess::text(*f.flow);check(text.find("Enter: acknowledge")!=std::string::npos &&
        text.find("Enter: confirm")==std::string::npos && text.find("Escape: cancel")==std::string::npos,"result controls contradict acknowledgement");
    f.act(AcknowledgeAction{});f.act(AcknowledgeAction{});text=XeenTrainingTestAccess::text(*f.flow);
    check(text.find("Not enough experience.")!=std::string::npos && text.find("Enter: acknowledge")!=std::string::npos,"refusal controls omit acknowledgement");
    Fixture limited(in,in.service(98));limited.enter();XeenTrainingTestAccess::limit(*limited.flow,2,UINT64_MAX-8);
    limited.act(SelectMemberAction{1});text=XeenTrainingTestAccess::text(*limited.flow);
    check(text.find("Enter/Escape: retry")!=std::string::npos && text.find("Enter: confirm")==std::string::npos,"departure-only controls contradict settlement");
    std::cout<<"Phase-specific Training menu/quote/result/refusal/departure controls passed\n";
}
}
int main(int argc,char **argv) {
    try {
        check(argc==3,"usage: training-input <installation> <authority|native|controls>");
        const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"original installation absent");Inputs in(*installation);
        const std::string mode=argv[2];if(mode=="authority")authority(in);else if(mode=="native")native(in);else if(mode=="controls")controls(in);else throw std::runtime_error("invalid input test mode");
        return 0;
    }catch(const std::exception &e){input_allocation::failNext=false;std::cerr<<e.what()<<'\n';return 1;}
}
