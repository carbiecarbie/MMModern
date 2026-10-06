#include "platform/sdl/SdlWindow.h"
#include "core/DialogInput.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace mmodern;
namespace {
Uint32 now=0;
unsigned uploads=0,pressedPresents=0,pumps=0,actions=0,sceneBuilds=1;
unsigned idleCalls=0,acquisitions=0,idleAtPress=0,acquiredAtPress=0;
Uint32 lastPressed=0,maxCursorGap=0;
bool pressed=false,injected=false;
std::vector<Uint32> actionTimes;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
void push(SDL_Event event){check(SDL_PushEvent(&event)==1,"Cannot enqueue motion fixture event");}
void moveKey(){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_UP;push(e);e.type=SDL_KEYUP;push(e);}
void motionBurst(){for(unsigned i=0;i<1000;++i){SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.x=100+i%100;e.motion.y=100;push(e);}}
}
extern "C" Uint32 __wrap_SDL_GetTicks(){return now;}
extern "C" void __wrap_SDL_Delay(Uint32 delay){now+=delay;}
extern "C" void __real_SDL_PumpEvents();
extern "C" void __wrap_SDL_PumpEvents(){if(pressed)++pumps;__real_SDL_PumpEvents();}
extern "C" int __real_SDL_UpdateTexture(SDL_Texture *,const SDL_Rect *,const void *,int);
extern "C" int __wrap_SDL_UpdateTexture(SDL_Texture *texture,const SDL_Rect *rect,const void *pixels,int pitch){
    int width=0;SDL_QueryTexture(texture,nullptr,nullptr,&width,nullptr);
    if(width==320){
        ++uploads;
        const bool next=(*static_cast<const Uint32 *>(pixels)&255)==1;
        if(next){lastPressed=now;idleAtPress=idleCalls;acquiredAtPress=acquisitions;}
        else if(pressed)maxCursorGap=std::max(maxCursorGap,now-lastPressed);
        pressed=next;
    }
    return __real_SDL_UpdateTexture(texture,rect,pixels,pitch);
}
extern "C" void __real_SDL_RenderPresent(SDL_Renderer *);
extern "C" void __wrap_SDL_RenderPresent(SDL_Renderer *renderer){
    if(pressed){
        ++pressedPresents;maxCursorGap=std::max(maxCursorGap,now-lastPressed);lastPressed=now;
        check(idleCalls==idleAtPress && acquisitions==acquiredAtPress,"Cursor redraw acquired or rebuilt a gameplay frame");
        if(!injected){injected=true;motionBurst();moveKey();motionBurst();}
    }
    __real_SDL_RenderPresent(renderer);
}
int main(){try{
    IndexedFrame frame(320,200,std::vector<std::uint8_t>(64000));frame.palette[5]=1;
    SdlWindow::FrameUpdateHandler handler=[&](const PlayerAction &action)->std::optional<IndexedFrame>{
        check(std::get<NavigationAction>(action)==NavigationAction::MoveForward,"Motion changed the queued action");
        check(!pressed,"Move applied before restoring the button");
        actionTimes.push_back(now);++actions;++sceneBuilds;return frame;
    };
    handler.inputContext=[](const auto &){return InputContext{1,true,true,MainScreen::Exploration};};
    handler.cursorImage=[] {IndexedFrame cursor(2,2,{1,1,1,1});cursor.palette[3]=255;return cursor;};
    handler.drawButton=[](auto &target,const auto &){target.pixels[0]=1;};
    handler.framePresented=[](const auto &){if(++acquisitions==1){motionBurst();moveKey();motionBurst();}};
    const auto ok=SdlWindow().showInteractive(frame,"Motion during button feedback",handler,{},[&]()->std::optional<IndexedFrame>{
        ++idleCalls;check(idleCalls<=2,"Motion delayed a ready move to another loop");
        if(actions==2){SDL_Event e{};e.type=SDL_QUIT;push(e);}return {};
    });
    check(ok && actions==2 && sceneBuilds==3,"Motion burst rebuilt scenes or lost movement");
    check(actionTimes==std::vector<Uint32>{100,200},"Motion extended the original button hold or delayed a ready move");
    check(uploads==7,"Cursor movement reuploaded scene pixels"); // Initial + pressed/restored/action for two moves.
    check(pressedPresents>=20 && maxCursorGap<=10 && pumps>=20,"Cursor froze during button feedback");
    std::cout<<"4000 motion events, two ready moves, two scene updates; cursor gap <=10 ms; unchanged 100 ms holds\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
