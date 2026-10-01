#include "XeenPurchaseTestSupport.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <iostream>
#include <cstdlib>
#include <new>
namespace input_allocation {bool failNext=false,failed=false;}
void *operator new(std::size_t size){
    if(input_allocation::failNext){input_allocation::failNext=false;input_allocation::failed=true;throw std::bad_alloc();}
    if(auto *p=std::malloc(size?size:1))return p;throw std::bad_alloc();
}
void operator delete(void *p) noexcept{std::free(p);}
void operator delete(void *p,std::size_t) noexcept{std::free(p);}
void *operator new[](std::size_t size){return ::operator new(size);}
void operator delete[](void *p) noexcept{::operator delete(p);}
void operator delete[](void *p,std::size_t) noexcept{::operator delete(p);}
extern "C" Uint32 __wrap_SDL_GetTicks(){return 100;}
using namespace purchase_test;
namespace {
void quote(Fixture &f){f.enter();f.quote(XeenInventoryCategory::Armor,3);}
void replay(Fixture &f,const IndexedFrame::Presentation &frame,std::uint64_t input,PlayerAction action=AcknowledgeAction{}){
    f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(action,input,frame));
}
void authority(Inputs &in) {
    {
        Fixture f(in,service(in));quote(f);const Owners before(f);
        const auto a=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        input_allocation::failed=false;input_allocation::failNext=true;f.act(AcknowledgeAction{});
        const auto b=f.flow->frame().presentation();
        check(input_allocation::failed && a!=b && input==f.flow->displayedInput() && XeenPurchaseTestAccess::quote(*f.flow),"failed confirmation did not retain semantics/new concrete origin");
        before.unchanged(f);replay(f,a,input);before.unchanged(f);
        f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(AcknowledgeAction{},input));before.unchanged(f);
        check(XeenPurchaseTestAccess::quote(*f.flow),"semantic-only confirmation borrowed current frame");
        replay(f,b,input);armorPaid(f);replay(f,b,input);armorPaid(f);
        check(XeenPurchaseTestAccess::reservation(*f.flow)==2 && XeenPurchaseTestAccess::operation(*f.flow)==1,"stale retry paid/rebound twice");f.leave();
    }
    for(unsigned phase=0;phase<3;++phase) {
        Fixture f(in,service(in));quote(f);
        if(phase==1)f.act(AcknowledgeAction{});
        if(phase==2){f.act(CancelInteractionAction{});f.act(SelectInventorySlotAction{8});f.act(AcknowledgeAction{});}
        const Owners before(f);const auto a=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        const auto text=XeenPurchaseTestAccess::text(*f.flow);f.present(f.flow->refresh(true));const auto b=f.flow->frame().presentation();
        check(a!=b && input==f.flow->displayedInput() && text==XeenPurchaseTestAccess::text(*f.flow),"cosmetic changed Smith semantic authority/text");
        replay(f,a,input);before.unchanged(f);check(text==XeenPurchaseTestAccess::text(*f.flow),"stale cosmetic input changed phase");
        replay(f,b,input);if(phase)check(XeenPurchaseTestAccess::browse(*f.flow),"fresh fixed-result acknowledgment refused");else armorPaid(f);f.leave();
    }
    // Make-before-break: an acquired A remains actionable while B is only a
    // composed cosmetic candidate. Its semantic action supersedes that B.
    {
        Fixture f(in,service(in));quote(f);const auto a=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        const auto b=f.flow->refresh(true).presentation();check(a!=b && f.flow->acceptsInputFrame(a),"cosmetic broke acquired origin before acquisition");
        replay(f,a,input);armorPaid(f);check(!f.flow->acceptsFrame(b),"obsolete cosmetic successor survived semantic action");f.leave();
    }
    for(auto boundary:{XeenSmithBoundary::BeforeDeparture,XeenSmithBoundary::BeforeEventSettlement}) {
        Fixture f(in,service(in));f.enter();unsigned calls=0;
        f.flow->smithBoundary=[&](auto here){if(here==boundary && ++calls<=2)throw std::bad_alloc();};
        f.act(CancelInteractionAction{});const Owners before(f);const auto a=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        f.act(AcknowledgeAction{});const auto b=f.flow->frame().presentation();
        check(a!=b && input==f.flow->displayedInput() && calls==2,"mandatory retry consumed semantic revisions");
        before.unchanged(f);replay(f,a,input);before.unchanged(f);check(calls==2,"stale mandatory origin reached callbacks");
        replay(f,b,input);check(f.flow->canSave() && f.p.encounterContext->day==9 && calls==3,"fresh mandatory retry repeated/lost day");
    }
    {
        Fixture f(in,service(in));quote(f);const auto old=f.flow->frame().presentation();const auto input=*f.flow->displayedInput();
        f.act(AcknowledgeAction{});f.act(AcknowledgeAction{});f.act(SelectInventorySlotAction{1});f.act(AcknowledgeAction{});
        const Owners after(f);replay(f,old,input);after.unchanged(f);check(XeenPurchaseTestAccess::quote(*f.flow),"old shifted-slot quote revived purchase authority");f.leave();
    }
}
void controls(Inputs &in) {
    Fixture f(in,service(in));f.enter();denied(f);
    auto text=XeenPurchaseTestAccess::text(*f.flow);check(text.find("B: Buy")!=std::string::npos && text.find("R/Enter: Armor repair")!=std::string::npos,"lobby modes/old Enter missing");
    const auto initial=f.flow->displayedInput();const auto revision=XeenPurchaseTestAccess::revision(*f.flow);
    for(unsigned n=0;n<50;++n){f.act(SelectMemberAction{0});f.act(NavigationAction::MoveForward);f.act(SaveGameAction{});}
    check(initial==f.flow->displayedInput() && revision==XeenPurchaseTestAccess::revision(*f.flow),"unchanged member/wrong/F9 drained optional authority");
    f.buy();text=XeenPurchaseTestAccess::text(*f.flow);
    check(text.find("Buy Weapons")!=std::string::npos && text.find("Choose a physical row.")!=std::string::npos && text.find("9 Empty")!=std::string::npos,"Buy physical unfiltered initial view differs");
    for(unsigned c=0;c<4;++c){
        for(unsigned slot=0;slot<9;++slot){f.act(SelectInventorySlotAction{slot});text=XeenPurchaseTestAccess::text(*f.flow);check(text.find("Row "+std::to_string(slot+1)+" M/ID/S/F")!=std::string::npos,"selected physical raw identity omitted");}
        f.act(NavigationAction::TurnRight);
    }
    f.act(NavigationAction::TurnRight);f.act(SelectInventorySlotAction{3});f.act(AcknowledgeAction{});text=XeenPurchaseTestAccess::text(*f.flow);
    check(text.find("Buy price: 200 gold")!=std::string::npos && text.find("Carried gold: 870")!=std::string::npos && text.find("Gold after: 670")!=std::string::npos,"quote numeric facts incomplete");
    denied(f);f.act(AcknowledgeAction{});armorPaid(f);text=XeenPurchaseTestAccess::text(*f.flow);
    check(text.find("Purchased unequipped. Paid 200 gold.")!=std::string::npos && text.find("Recipient slot 5")!=std::string::npos && text.find("Gold: 870 -> 670")!=std::string::npos,"fixed result missing exact movement/purse facts");
    denied(f);f.act(AcknowledgeAction{});f.act(SelectInventorySlotAction{0});f.act(AcknowledgeAction{});text=XeenPurchaseTestAccess::text(*f.flow);
    check(text.find("Shortfall: 330 gold")!=std::string::npos,"insufficient quote lacks truthful shortfall");
    f.act(AcknowledgeAction{});text=XeenPurchaseTestAccess::text(*f.flow);check(text.find("Not enough carried gold.")!=std::string::npos,"insufficient result misleading");f.leave();
}
void key(SDL_Keycode code,Uint32 type,bool repeat=false) {
    SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);e.key.timestamp=100;e.key.repeat=repeat;
    check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"native Smith key enqueue failed");
}
void edge(SDL_Keycode code){key(code,SDL_KEYUP);key(code,SDL_KEYDOWN);key(code,SDL_KEYUP);}
void protectedEdge(SDL_Keycode code){
    key(code,SDL_KEYUP);key(code,SDL_KEYDOWN);key(code,SDL_KEYDOWN);key(code,SDL_KEYDOWN,true);key(code,SDL_KEYUP);
}
void native(Inputs &in) {
    SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
    for(unsigned mode=0;mode<4;++mode) {
        Fixture f(in,service(in));quote(f);if(mode==2)f.act(AcknowledgeAction{});
        if(mode==3){f.act(CancelInteractionAction{});f.act(SelectInventorySlotAction{8});f.act(AcknowledgeAction{});}
        const Owners before(f);const auto a=f.flow->frame().presentation();const auto semantic=*f.flow->displayedInput();
        IndexedFrame::Presentation shown=a,b;unsigned stage=0,dispatches=0,cycles=0;bool failed=false;
        SdlWindow::FrameUpdateHandler handler=[](const PlayerAction &)->std::optional<IndexedFrame>{throw std::runtime_error("unbound Smith native input");};
        handler.protectAllKeys=true;handler.displayedInput=[&]{return f.flow->displayedInput();};
        handler.acceptsFrame=[&](const auto &p){return f.flow->acceptsFrame(p);};
        handler.acceptsInputFrame=[&](const auto &p){return f.flow->acceptsInputFrame(p);};
        handler.completeInputHandoff=[&](const auto &p){f.flow->completeInputHandoff(p);};
        handler.framePresented=[&](const auto &p){f.flow->framePresented(p,true);shown=p;};
        handler.beginCycle=[&](auto){check(++cycles<1000,"native Smith loop bound");f.flow->beginCycle(++f.cycle);};
        const auto dispatch=[&](const PlayerAction &action,std::uint64_t input,const IndexedFrame::Presentation &p)->std::optional<IndexedFrame>{
            if(dispatches++==0 && mode==0){input_allocation::failed=false;input_allocation::failNext=true;auto r=f.flow->handle(action,input,p);failed=input_allocation::failed;return r;}
            return f.flow->handle(action,input,p);
        };
        handler.withDisplayedInput=[&](const auto &action,std::uint64_t input){return dispatch(action,input,shown);};handler.withPresentedInput=dispatch;
        const bool ok=SdlWindow().showInteractive(f.flow->frame(),"Smith concrete input",handler,[]{return true;},[&]()->std::optional<IndexedFrame>{
            if(stage==0){++stage;if(mode==0)edge(SDLK_RETURN);return {};}
            if(stage==1){++stage;std::optional<IndexedFrame> redraw;if(mode)redraw=f.flow->refresh(true);else check(failed,"native optional allocation fault unexercised");
                b=f.flow->frame().presentation();check(a!=b && semantic==f.flow->displayedInput(),"native replacement changed semantic input");before.unchanged(f);
                edge(SDLK_RETURN);return redraw;}
            if(stage==2){++stage;check(shown==b,"new native concrete origin unacquired");
                if(mode==0)before.unchanged(f);
                else if(mode==1)armorPaid(f);
                else{before.unchanged(f);check(XeenPurchaseTestAccess::browse(*f.flow),"acquired cosmetic predecessor did not accept its queued acknowledgment");}
                check(dispatches==1,"queued predecessor input was relabeled for the cosmetic successor");
                // Strict retry A was already spent, so mode0 needs a fresh B.
                // Cosmetic A remained valid through the bounded handoff batch;
                // its one action superseded B and queued keys must not cross C.
                key(SDLK_RETURN,SDL_KEYDOWN,true);key(SDLK_RETURN,SDL_KEYDOWN,true);edge(SDLK_RETURN);edge(SDLK_RETURN);return {};}
            if(mode<2){armorPaid(f);check(XeenPurchaseTestAccess::result(*f.flow),"batched confirmations acknowledged unshown result");}
            else{before.unchanged(f);check(XeenPurchaseTestAccess::browse(*f.flow),"fresh native result/refusal acknowledgment refused");}
            check(dispatches==(mode==0?2u:1u),"held/repeated/old-origin input crossed the next semantic frame");
            SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return {};
        });
        check(ok,"native Smith concrete-frame regression failed");
    }
    // Every semantic selection and B/R mode begins with a first native edge.
    // Two mode keys in the same batch must not enter Repair after opening Buy.
    {
        Fixture f(in,service(in));f.enter();unsigned stage=0,cycles=0;
        SdlWindow::FrameUpdateHandler h=[&](const PlayerAction &)->std::optional<IndexedFrame>{throw std::runtime_error("unbound Smith controls");};
        h.protectAllKeys=true;h.displayedInput=[&]{return f.flow->displayedInput();};h.acceptsFrame=[&](const auto &p){return f.flow->acceptsFrame(p);};
        h.acceptsInputFrame=[&](const auto &p){return f.flow->acceptsInputFrame(p);};h.completeInputHandoff=[&](const auto &p){f.flow->completeInputHandoff(p);};
        h.framePresented=[&](const auto &p){f.flow->framePresented(p,true);};h.beginCycle=[&](auto){check(++cycles<1000,"native controls loop bound");f.flow->beginCycle(++f.cycle);};
        h.withPresentedInput=[&](const auto &a,auto input,const auto &p)->std::optional<IndexedFrame>{return f.flow->handle(a,input,p);};
        bool ok=SdlWindow().showInteractive(f.flow->frame(),"Smith first edges",h,[]{return true;},[&]()->std::optional<IndexedFrame>{
            // Queue the next physical edge only after its actual predecessor
            // frame has been acquired; semantic state can change before upload.
            if(!f.flow->acceptsInputFrame(f.flow->frame().presentation()))return {};
            switch(stage++) {
            case 0:protectedEdge(SDLK_b);edge(SDLK_r);break;
            case 1:check(XeenPurchaseTestAccess::text(*f.flow).find("Buy Weapons")!=std::string::npos,"first B/batched R did not open Buy");protectedEdge(SDLK_F2);break;
            case 2:check(XeenPurchaseTestAccess::text(*f.flow).find("Tyro")!=std::string::npos,"first recipient edge lost");protectedEdge(SDLK_RIGHT);break;
            case 3:check(XeenPurchaseTestAccess::text(*f.flow).find("Buy Armor")!=std::string::npos,"first category edge lost");protectedEdge(SDLK_4);break;
            case 4:check(XeenPurchaseTestAccess::selected(*f.flow),"first row edge lost");protectedEdge(SDLK_RETURN);break;
            case 5:check(XeenPurchaseTestAccess::quote(*f.flow),"first quote edge lost");protectedEdge(SDLK_ESCAPE);break;
            case 6:check(XeenPurchaseTestAccess::browse(*f.flow),"first cancellation edge lost");protectedEdge(SDLK_ESCAPE);break;
            case 7:check(XeenPurchaseTestAccess::lobby(*f.flow),"first browser exit edge lost");protectedEdge(SDLK_r);break;
            case 8:check(XeenPurchaseTestAccess::text(*f.flow).find("Enter: quote")!=std::string::npos,"first Repair edge lost");protectedEdge(SDLK_ESCAPE);break;
            case 9:protectedEdge(SDLK_ESCAPE);break;
            default:check(f.flow->canSave() && f.p.encounterContext->day==9 && f.p.monsterTreasure->gold==870,"first departure edge lost or batch mutated owners");
                SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);break;
            }return {};
        });check(ok,"native Smith control first-edge regression failed");
    }
}
}
int main(int argc,char **argv){
    try {
        check(argc==3,"usage: purchase-input <installation> <authority|native|controls>");const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"installation unavailable");Inputs in(*installation);
        const std::string mode=argv[2];if(mode=="authority")authority(in);else if(mode=="native")native(in);else if(mode=="controls")controls(in);else throw std::runtime_error("unknown input test mode");
        std::cout<<"M42 "<<mode<<" concrete-frame, responsiveness and modal authority passed\n";return 0;
    }catch(const std::exception &e){input_allocation::failNext=false;std::cerr<<e.what()<<'\n';return 1;}
}
