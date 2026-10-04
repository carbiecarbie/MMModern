#ifndef MMODERN_SERVICE_INPUT_TEST_SUPPORT_H
#define MMODERN_SERVICE_INPUT_TEST_SUPPORT_H
#include "XeenProbeFired.h"
#include "XeenPurchaseTestSupport.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <cstdlib>
#include <new>
namespace input_allocation {inline bool failNext=false,failed=false;}
void *operator new(std::size_t size) {
    if(input_allocation::failNext) {input_allocation::failNext=false;input_allocation::failed=true;throw std::bad_alloc();}
    if(auto *p=std::malloc(size?size:1)) return p;throw std::bad_alloc();
}
void operator delete(void *p) noexcept {std::free(p);}
void operator delete(void *p,std::size_t) noexcept {std::free(p);}
void *operator new[](std::size_t size) {return ::operator new(size);}
void operator delete[](void *p) noexcept {::operator delete(p);}
void operator delete[](void *p,std::size_t) noexcept {::operator delete(p);}
extern "C" Uint32 __wrap_SDL_GetTicks() {probe_fired::hit("SDL_GetTicks");return 1;}
namespace service_input_test {
using namespace purchase_test;
using F=training_test::Fixture;
std::unique_ptr<F> fixture(Inputs &in,bool smith) {
    if(smith) return std::make_unique<Fixture>(in,service(in));
    return std::make_unique<F>(in,in.service());
}
void enter(F &f,bool smith) {
    f.act(InteractionAction{});f.prepare();
    if(smith) {f.act(DialogKeyAction{'b'});f.act(DialogKeyAction{'a'});f.act(DialogKeyAction{'4'});}
    else f.act(SelectMemberAction{1});
}
PlayerAction intent(bool smith) {return DialogKeyAction{static_cast<unsigned>(smith?'y':'t')};}
void paid(const F &f,bool smith) {
    if(smith) {
        check(f.p.monsterTreasure->gold==670 && f.p.roster.at(0).armor[4].id==3 &&
            f.p.serviceEconomy->wares[0][0][1][3].id==5 && XeenPurchaseTestAccess::browse(*f.flow),"Buy payment/delivery/depletion/result removal differs");
    } else check(f.p.monsterTreasure->gold==710 && f.p.roster.at(18).permanentLevel==4 &&
        f.p.roster.combatInputs(18)->experience==6000 && f.p.encounterContext->day==9 &&
        XeenTrainingTestAccess::menu(*f.flow),"one-step Training payment/XP/member-day differs");
}
void replay(F &f,PlayerAction action,const IndexedFrame::Presentation &origin,std::uint64_t epoch) {
    f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(action,epoch,origin));
}
void authority(Inputs &in,bool smith) {
    for(unsigned kind=0;kind<3;++kind) {
        auto ptr=fixture(in,smith);auto &f=*ptr;enter(f,smith);const Owners before(f);
        const auto a=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        if(kind==0) f.present(f.flow->refresh(true));
        else if(kind==1) {
            input_allocation::failed=false;input_allocation::failNext=true;f.act(intent(smith));
            check(input_allocation::failed,"service intent allocation fault unexercised");
        } else {
            bool fault=false;
            if(smith)f.flow->smithBoundary=[&](auto here){if(here==XeenSmithBoundary::BeforePurchase && !fault){fault=true;throw std::bad_alloc();}};
            else f.flow->trainingBoundary=[&](auto here){if(here==XeenTrainingBoundary::Quote && !fault){fault=true;throw std::bad_alloc();}};
            f.act(intent(smith));check(fault,"retained service preparation fault unexercised");
            f.flow->smithBoundary={};f.flow->trainingBoundary={};
        }
        const auto b=f.flow->frame().presentation();
        check(a!=b && input==f.flow->displayedInput(),"service retry/redraw lost semantic epoch or concrete origin");before.unchanged(f);
        replay(f,intent(smith),a,input);before.unchanged(f);
        f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(intent(smith),input));before.unchanged(f);
        replay(f,intent(smith),b,input);
        if(!smith)f.prepare();paid(f,smith);const Owners after(f);
        replay(f,intent(smith),b,input);after.unchanged(f);
        f.act(CancelInteractionAction{});if(smith)f.act(CancelInteractionAction{});
        check(f.flow->canSave() && f.p.encounterContext->day==(smith?9:10),"service concrete retry repeated/lost departure");
    }
    for(unsigned boundary=0;boundary<2;++boundary) {
        auto ptr=fixture(in,smith);auto &f=*ptr;f.act(InteractionAction{});f.prepare();unsigned calls=0;
        if(smith)f.flow->smithBoundary=[&](auto here){if(here==(boundary?XeenSmithBoundary::BeforeEventSettlement:XeenSmithBoundary::BeforeDeparture) && ++calls<=2)throw std::bad_alloc();};
        else f.flow->trainingBoundary=[&](auto here){if(here==(boundary?XeenTrainingBoundary::BeforeEventSettlement:XeenTrainingBoundary::BeforeDeparture) && ++calls<=2)throw std::bad_alloc();};
        f.act(CancelInteractionAction{});const auto a=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        f.act(CancelInteractionAction{});const auto b=f.flow->frame().presentation();const Owners before(f);
        check(a!=b && input==f.flow->displayedInput() && calls==2,"mandatory retry spent finite semantics");
        replay(f,CancelInteractionAction{},a,input);before.unchanged(f);check(calls==2,"stale departure invoked callback");
        replay(f,CancelInteractionAction{},b,input);check(f.flow->canSave() && f.p.encounterContext->day==9 && calls==3,"mandatory fresh retry duplicated/lost departure");
    }
}
void key(SDL_Keycode code,Uint32 type,bool repeat=false) {
    SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);e.key.repeat=repeat;e.key.timestamp=100;
    check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"native service key enqueue failed");
}
void tap(SDL_Keycode code) {key(code,SDL_KEYUP);key(code,SDL_KEYDOWN);key(code,SDL_KEYUP);}
void click(int x,int y,Uint8 button=SDL_BUTTON_LEFT) {
    SDL_Event e{};e.type=SDL_MOUSEBUTTONDOWN;e.button.button=button;e.button.windowID=SDL_GetWindowID(SDL_GetWindowFromID(1));e.button.x=x*3;e.button.y=y*3;e.button.timestamp=100;
    check(SDL_PushEvent(&e)==1,"native service click enqueue failed");
}
void native(Inputs &in,bool smith) {
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
    for(bool mouse:{false,true}) {
        auto ptr=fixture(in,smith);auto &f=*ptr;enter(f,smith);
        const Owners before(f);const auto a=f.flow->frame().presentation();const auto epoch=*f.flow->displayedInput();
        IndexedFrame::Presentation shown=a;unsigned stage=0,dispatches=0,feedback=0,cycles=0;
        SdlWindow::FrameUpdateHandler handler=[](const auto &)->std::optional<IndexedFrame>{throw std::runtime_error("unbound service input");};
        handler.protectAllKeys=true;
        handler.inputContext=[&](const auto &origin){return f.flow->inputContext(origin);};handler.displayedInput=[&]{return f.flow->displayedInput();};
        handler.acceptsFrame=[&](const auto &frame){return f.flow->acceptsFrame(frame);};
        handler.acceptsInputFrame=[&](const auto &origin){return f.flow->acceptsInputFrame(origin);};
        handler.completeInputHandoff=[&](const auto &origin){f.flow->completeInputHandoff(origin);};
        handler.framePresented=[&](const auto &frame){f.flow->framePresented(frame,true);shown=frame;};
        handler.beginCycle=[&](auto){check(++cycles<400,"native service input loop bound");f.flow->beginCycle(++f.cycle);};
        handler.drawButton=[&](auto &frame,const InputButton &b){++feedback;check(b.frame==0,"service button normal frame differs");
            f.flow->drawDialogSprite(frame,b.resource,b.pressedFrame(),b.x,b.y);};
        handler.withPresentedInput=[&](const auto &action,auto input,const auto &origin)->std::optional<IndexedFrame>{
            ++dispatches;check(dispatches==1,"held/batched service input duplicated intent");return f.flow->handle(action,input,origin);};
        handler.withDisplayedInput=[&](const auto &action,auto input){return handler.withPresentedInput(action,input,shown);};
        const auto ok=SdlWindow().showInteractive(f.flow->frame(),"Original service input",handler,[]{return true;},[&]()->std::optional<IndexedFrame>{
            if(stage==0) {++stage;click(smith?130:243,smith?113:109,SDL_BUTTON_RIGHT);click(319,199);tap(SDLK_RETURN);return {};}
            if(stage==1) {++stage;check(!dispatches,"right/outside/Enter reached service action");before.unchanged(f);
                if(mouse) {click(smith?130:243,smith?113:109);click(smith?130:243,smith?113:109);}
                else {key(smith?SDLK_y:SDLK_t,SDL_KEYUP);key(smith?SDLK_y:SDLK_t,SDL_KEYDOWN);key(smith?SDLK_y:SDLK_t,SDL_KEYDOWN);key(smith?SDLK_y:SDLK_t,SDL_KEYDOWN,true);}
                return {};
            }
            if(!dispatches)return {};
            auto next=f.flow->updatePresentation();
            if(!smith && f.p.roster.at(18).permanentLevel==3)return next;
            paid(f,smith);check(dispatches==1 && feedback==1,"native service intent/pressed feedback count differs");
            SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return next;
        });
        if(!ok)std::cerr<<"stage="<<stage<<" dispatches="<<dispatches<<" mouse="<<mouse<<"\n";
        check(ok,"native service input failed");
    }
}
void controls(Inputs &in,bool smith) {
    auto ptr=fixture(in,smith);auto &f=*ptr;f.act(InteractionAction{});f.prepare();const Owners before(f);
    for(unsigned i=0;i<40;++i){f.act(AcknowledgeAction{});f.act(NavigationAction::TurnRight);f.act(SaveGameAction{});f.act(SelectMemberAction{0});}
    before.unchanged(f);
    const auto context=f.flow->inputContext(f.flow->frame().presentation());check(!context.acceptsQueuedInput && context.dialog,"service queue policy differs");
    if(smith) {
        check(context.dialog->key('b') && !context.dialog->key('r'),"original Browse lobby keys differ");
        f.act(DialogKeyAction{'b'});f.act(DialogKeyAction{'a'});f.act(DialogKeyAction{'4'});
        check(XeenPurchaseTestAccess::quote(*f.flow),"original row Confirm absent");
        f.act(AcknowledgeAction{});before.unchanged(f);f.act(DialogKeyAction{'n'});before.unchanged(f);
        f.act(DialogKeyAction{'4'});f.act(DialogKeyAction{'y'});paid(f,true);
    } else {
        check(context.dialog->key('t') && !context.dialog->key(InputKey::Enter),"original T keys differ");
        // Arturius lacks the prepared XP; ineligible T is silent and unchanged.
        f.act(DialogKeyAction{'t'});before.unchanged(f);check(XeenTrainingTestAccess::menu(*f.flow),"ineligible T opened a result");
        f.act(SelectMemberAction{1});f.act(DialogKeyAction{'t'});f.prepare();paid(f,false);
    }
}
int run(int argc,char **argv,bool smith) {
    if(argc==3 && std::string(argv[2])=="native")probe_fired::expect("SDL_GetTicks");
    try {
        check(argc==3,"usage: service-input <installation> <authority|native|controls>");
        const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"installation unavailable");Inputs in(*installation);
        const std::string mode=argv[2];if(mode=="authority")authority(in,smith);else if(mode=="native")native(in,smith);else if(mode=="controls")controls(in,smith);else throw std::runtime_error("unknown service test mode");
        std::cout<<"Original service "<<mode<<": strict input, stale origins and one-step intent passed\n";return 0;
    } catch(const std::exception &e){input_allocation::failNext=false;std::cerr<<e.what()<<'\n';return 1;}
}
}
#endif
