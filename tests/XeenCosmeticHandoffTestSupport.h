#ifndef MMODERN_COSMETIC_HANDOFF_TEST_SUPPORT_H
#define MMODERN_COSMETIC_HANDOFF_TEST_SUPPORT_H
// Artificial presentation faults through the real Application/Flow/SDL boundary.
namespace cosmetic_handoff_test {
using namespace combat_gameplay_test;
inline void changingPixels(XeenGameplayServices &s) {
 const auto compose=s.composeEncounter;
 s.composeEncounter=[compose,serial=0u](auto &w,const auto &p,const auto &c,auto phase,auto appearance) mutable {
  auto result=compose(w,p,c,phase,appearance);result.frame.pixels[0]=static_cast<std::uint8_t>(++serial);return result;
 };
}
// 0 omitted idle; 1 deferred correct initial; 2 stale idle; 3 stale initial;
// 4 stale action result; 5 another, already destroyed Flow's initial frame.
inline bool exercise(Harness &h,const SdlWindow::FrameUpdateHandler &handler,
 const SdlWindow::IdleFrameHandler &idle,const std::function<bool()> &escape,
 const std::function<std::string()> &status,unsigned mode,SDL_Keycode key,const PlayerAction &action,
 const std::function<void()> &unchanged,const std::function<void()> &accepted) {
 static std::optional<IndexedFrame> previousOwner;
 const auto foreign=previousOwner;const auto old=h.flow->frame();previousOwner=old;
 const auto epoch=handler.displayedInput();
 const bool inventory=h.flow->inventoryOpen();
 const bool modal=inventory || h.flow->presentationGeneration().has_value();
 unsigned loops=0,acknowledged=0;bool pending=false,correctSent=false,issued=false,triggered=false,delay=false;
 std::optional<IndexedFrame> correct;
 const auto blocked=[&] {
  unchanged();handler.withDisplayedInput(action,*epoch);unchanged();
  const auto saves=h.saves;handler.withDisplayedInput(SaveGameAction{},*epoch);
  check(h.saves==saves&&!h.flow->canSave()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"Wrong/missing frame cannot admit input/F9/capture");
  unchanged();check(acknowledged==0,"Wrong/missing frame acknowledged the current frame");
 };
 const auto cosmetic=[&] {
  h.now+=100;handler.beginCycle(++h.cycle);correct=idle();pending=true;
  check(correct && correct->pixels!=old.pixels && correct->presentation()!=old.presentation(),"Changed pixels carry distinct immutable identity");
  check(handler.displayedInput()==epoch,"Cosmetic substitution must preserve semantic epoch");
  check(correct->presentation()->pixels==correct->pixels,"Identity binds the exact composed content");
  // A mutable carrier cannot pair other pixels with the bound identity: SDL
  // consumes the retained const snapshot, not subsequently edited draft fields.
  auto edited=*correct;edited.pixels=old.pixels;
  check(edited.presentation()->pixels==correct->pixels,"Immutable binding survives carrier edits");
  bool refused=false;try{handler.framePresented(old.presentation());}catch(const std::logic_error &){refused=true;}
  check(refused,"Direct stale acknowledgment rejected without altering live ownership");
  blocked();
 };
 auto first=old;
 if(mode==1||mode==3||mode==5){cosmetic();if(mode==1){first=*correct;correctSent=true;}else if(mode==5){check(bool(foreign),"Retained previous owner fixture");first=*foreign;check(!handler.acceptsFrame(first.presentation()),"Wrong owner/incarnation rejected");bool refused=false;try{handler.framePresented(first.presentation());}catch(const std::logic_error &){refused=true;}check(refused,"Wrong-owner direct acknowledgment rejected");blocked();}}
 auto windowHandler=handler;
 windowHandler.beginCycle=[&](std::uint64_t){handler.beginCycle(++h.cycle);};
 windowHandler.framePresented=[&](const auto &frame){if(pending&&!issued){check(frame==correct->presentation(),"SDL must acknowledge supplied current identity only");++acknowledged;}handler.framePresented(frame);};
 const auto queue=[&]{SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;check(SDL_PushEvent(&e)==1,"Fresh handoff key");e.type=SDL_KEYUP;check(SDL_PushEvent(&e)==1,"Fresh handoff release");};
 if(mode==4)windowHandler.withDisplayedInput=[&](const auto &a,std::uint64_t input)->std::optional<IndexedFrame>{
  if(!triggered){triggered=true;cosmetic();delay=true;return old;}return handler.withDisplayedInput(a,input);
 };
 const bool ok=SdlWindow().showInteractive(first,"Bound concrete frame",windowHandler,escape,[&]()->std::optional<IndexedFrame>{
  check(++loops<12,"Bounded stale-frame recovery");
  if(!pending){if(mode==4){queue();return {};}cosmetic();return mode==2?std::optional<IndexedFrame>{old}:std::nullopt;}
  if(!correctSent){blocked();if(delay){delay=false;return {};}correctSent=true;if(mode==3||mode==5)queue();return correct;}
  check(acknowledged>0,"Correct supplied frame must be presented");
  if(!issued){check(h.flow->journeyInputCurrent(handler.displayedInput()),"Correct frame opens semantic input");check(h.flow->canSave()==!modal && XeenSaveState::canCapture(*h.party,*h.camera,*h.world)==!modal,"Only Quiet capture opens after correct upload");unchanged();issued=true;queue();return {};}
  accepted();SDL_Event quit{};quit.type=SDL_QUIT;SDL_PushEvent(&quit);return {};
 },status);
 check(ok&&issued&&acknowledged>0,"Deferred current frame recovers all upload paths");
 check(!h.flow->canSave()&&!XeenSaveState::canCapture(*h.party,*h.camera,*h.world),"Closed SDL cannot leave capture authority");
 std::cout<<"BOUND HANDOFF context="<<(inventory?"inventory":modal?"event":"quiet")<<" mode="<<mode<<" stale/omitted refusal and deferred current input PASS\n";
 return ok;
}
}
#endif
