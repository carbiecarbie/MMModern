#include "XeenProbeFired.h"
#include "XeenTrainingTestSupport.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>

using namespace training_test;
namespace {
// Model an OS key becoming available just AFTER successful acquisition, at
// the next SDL sampling call. Idle-callback injection misses the late pump bug.
std::optional<SDL_Keycode> fresh;
unsigned sampled=0;
bool releaseBeforeFresh=false;
void key(SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0) {
    SDL_Event e{};e.type=type;e.key.timestamp=100;e.key.repeat=repeat;
    e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);
    check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"responsiveness enqueue failed");
}
void sample() {
    if(!fresh)return;
    const auto code=*fresh;fresh.reset();++sampled;
    if(releaseBeforeFresh)key(code,SDL_KEYUP);
    key(code);key(code,SDL_KEYDOWN,1);key(code,SDL_KEYUP);
}
}
extern "C" Uint32 __wrap_SDL_GetTicks(){probe_fired::hit("SDL_GetTicks");return 100;}
extern "C" void __real_SDL_PumpEvents();
extern "C" void __wrap_SDL_PumpEvents(){probe_fired::hit("SDL_PumpEvents");__real_SDL_PumpEvents();sample();}
extern "C" int __real_SDL_WaitEventTimeout(SDL_Event *,int);
extern "C" int __wrap_SDL_WaitEventTimeout(SDL_Event *e,int timeout){probe_fired::hit("SDL_WaitEventTimeout");sample();return __real_SDL_WaitEventTimeout(e,timeout);}
namespace {
// Uses real Flow owners, SDL queue/held-key filtering, concrete binding and
// dispatch. No retry press: one down/repeat/up per acquired target frame.
void press(Fixture &f,SDL_Keycode code,bool cosmetic=false,bool held=false,bool deferred=false) {
    const auto old=f.flow->frame().presentation();
    const auto semantic=f.flow->displayedInput();
    auto target=old;
    unsigned dispatches=0,cycles=0;bool armed=false,redrawn=false,allowInitial=!deferred;
    releaseBeforeFresh=held;
    const auto before=sampled;
    SdlWindow::FrameUpdateHandler handler=[](const PlayerAction &)->std::optional<IndexedFrame>{throw std::runtime_error("missing concrete input");};
    handler.protectAllKeys=true;
    handler.displayedInput=[&]{return f.flow->displayedInput();};
    handler.acceptsFrame=[&](const auto &frame){return allowInitial && f.flow->acceptsFrame(frame);};
    handler.framePresented=[&](const auto &frame){
        // Keys queued during acquisition are still pre-frame, even when their
        // timestamp equals the first real post-acquisition edge's timestamp.
        if(!armed && (!cosmetic || redrawn)) {key(code);if(!held)key(code,SDL_KEYUP);}
        f.flow->framePresented(frame);
        if(!armed && (!cosmetic || redrawn)) {target=frame;armed=true;fresh=code;}
    };
    handler.beginCycle=[&](auto){check(++cycles<8,"first physical press lost (no retry permitted)");f.flow->beginCycle(++f.cycle);};
    handler.withPresentedInput=[&](const auto &action,std::uint64_t input,const auto &origin)->std::optional<IndexedFrame>{
        check(origin==target,"fresh key changed concrete origin");++dispatches;
        return f.flow->handle(action,input,origin);
    };
    const bool ok=SdlWindow().showInteractive(f.flow->frame(),"First native press",handler,[]{return true;},[&]()->std::optional<IndexedFrame>{
        if(!allowInitial) {
            check(dispatches==0,"failed acquisition authorized a key");
            key(code);key(code,SDL_KEYUP);allowInitial=true;return f.flow->frame();
        }
        if(cosmetic && !redrawn) {
            redrawn=true;
            f.now+=100;
            auto replacement=f.flow->canSave()?f.flow->updatePresentation():std::optional<IndexedFrame>{f.flow->refresh(true)};
            check(bool(replacement),"cosmetic frame missing");target=replacement->presentation();
            check(target!=old && f.flow->displayedInput()==semantic,"cosmetic must retain semantic authority");
            // Equal-tick pre-acquisition pair must not become input for B.
            key(code);if(!held)key(code,SDL_KEYUP);return replacement;
        }
        if(dispatches) {SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}
        return {};
    });
    fresh.reset();
    std::cout<<"Native edge code="<<code<<" cosmetic="<<cosmetic<<" held="<<held<<" deferred="<<deferred
        <<" sampled="<<sampled-before<<" dispatched="<<dispatches<<" cycles="<<cycles<<" success="<<ok<<'\n';
    check(ok && armed && sampled==before+1 && dispatches==1,"first post-acquisition key must dispatch exactly once");
}
void movement(Inputs &in) {
    for(unsigned timing=0;timing<4;++timing)for(const auto code:{SDLK_w,SDLK_a,SDLK_s,SDLK_d}) {
        const bool redraw=timing==1 || timing==2;
        auto source=in.service();source.camera={28,10,10,XeenDirection::North};
        Fixture f(in,source,redraw);const auto minute=f.p.encounterContext->minutes;
        press(f,code,redraw,timing==2,timing==3);
        const auto expectedY=code==SDLK_w?11:code==SDLK_s?9:10;
        const auto expectedDirection=code==SDLK_a?XeenDirection::West:code==SDLK_d?XeenDirection::East:XeenDirection::North;
        check(f.c.x==10 && f.c.y==expectedY && f.c.direction==expectedDirection,"one WASD edge did not produce exactly one navigation action");
        check(f.p.encounterContext->minutes==minute+(code==SDLK_w || code==SDLK_s?1:0),"navigation time duplicated");
        std::cout<<"First press "<<SDL_GetKeyName(code)<<" cosmetic="<<redraw<<" passed\n";
    }
}
void training(Inputs &in) {
    Fixture f(in,in.service());
    press(f,SDLK_SPACE);f.prepare();
    check(XeenTrainingTestAccess::menu(*f.flow),"first Space did not open service");
    press(f,SDLK_F2);
    press(f,SDLK_RETURN);
    check(XeenTrainingTestAccess::quote(*f.flow),"first menu Enter did not open quote");
    press(f,SDLK_RETURN,true);f.prepare();
    check(f.p.roster.at(18).permanentLevel==4 && f.p.roster.combatInputs(18)->experience==6000 &&
        f.p.monsterTreasure->gold==710 && f.p.encounterContext->day==9,"fresh quote Enter did not purchase exactly once");
    press(f,SDLK_RETURN);
    check(XeenTrainingTestAccess::menu(*f.flow),"first result Enter did not acknowledge");
    std::cout<<"First Space/F2/menu Enter/quote Enter/result Enter passed\n";
}
}
int main(int argc,char **argv) {
    probe_fired::expect("SDL_GetTicks");probe_fired::expect("SDL_PumpEvents");probe_fired::expect("SDL_WaitEventTimeout");
    try {
        check(argc==2,"usage: native-responsiveness <installation>");
        SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
        const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"original installation absent");
        Inputs in(*installation);movement(in);training(in);return 0;
    }catch(const std::exception &e){fresh.reset();std::cerr<<e.what()<<'\n';return 1;}
}
