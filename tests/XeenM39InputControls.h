// Original Journey lifecycle and production SDL intake; injected keys are controls.
#ifndef MMODERN_M39_INPUT_CONTROLS_H
#define MMODERN_M39_INPUT_CONTROLS_H
template<class Handler,class Idle,class Show>
bool m39InputControls(const std::string &control,const IndexedFrame &first,const Handler &handler,
    const Idle &idle,const Show &show,const std::function<bool()> &escape,
    const std::function<std::string()> &status,XeenEventFlow &flow,
    std::uint64_t &now,std::uint64_t &cycle,std::function<void()> &composeProbe) {
    const auto combat=[&]()->const XeenCombat &{check(flow.encounter() && flow.encounter()->combat(),"M39 input control lost combat");return *flow.encounter()->combat();};
    const auto present=[&](const IndexedFrame &f){check(handler.acceptsFrame(f.presentation()),"M39 input current concrete frame");handler.framePresented(f.presentation());};
    const auto action=[&](PlayerAction a){handler.beginCycle(++cycle);const auto f=handler.withDisplayedInput(a,*handler.displayedInput());if(f)present(*f);};
    const auto tick=[&]{now+=100;handler.beginCycle(++cycle);if(const auto f=idle())present(*f);};
    const auto quiet=[&]{for(unsigned i=0;i<500 && !flow.canSave();++i)tick();check(flow.canSave(),"M39 input prefix Quiet bound");};
    present(first);action(NavigationAction::MoveForward);quiet();action(ShootAction{});quiet();action(NavigationAction::MoveForward);
    for(unsigned i=0;i<500;++i){
        if(flow.encounter()->combat() && combat().phase()==XeenCombatPhase::PlayerReady) {
            if(combat().participant()==4)break;
            action(BlockAction{});
        }else tick();
    }
    check(combat().phase()==XeenCombatPhase::PlayerReady && combat().participant()==4,"M39 input original Orc prefix");
    const SDL_Keycode code=control=="attack"?SDLK_SPACE:control=="block"?SDLK_b:control=="run"?SDLK_r:control=="cast"?SDLK_c:SDLK_1;
    const auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){
        SDL_Event e{};e.type=type;e.key.state=type==SDL_KEYUP?SDL_RELEASED:SDL_PRESSED;
        e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);e.key.repeat=repeat;e.key.timestamp=SDL_GetTicks();
        check(SDL_PeepEvents(&e,1,SDL_ADDEVENT,0,0)==1,"M39 input native queue");
    };
    const auto initialTicket=combat().ticket();
    unsigned stage=0,iterations=0,accepted=0,dispatched=0,received=0,retired=0;
    bool transition=false,cosmetic=false,fresh=false,shown=false;
    std::optional<std::uint64_t> stableInput,cosmeticInput;
    std::optional<XeenCombat::Ticket> stableTicket;
    IndexedFrame::Presentation stableFrame;
    const auto baselineCommands=replay_test::commands;
    const auto matches=[&](const PlayerAction &a){return control=="attack"?std::holds_alternative<InteractionAction>(a):
        control=="block"?std::holds_alternative<BlockAction>(a):control=="run"?std::holds_alternative<RevisitCompletedAction>(a):
        control=="cast"?std::holds_alternative<CastSpellAction>(a):std::holds_alternative<SelectInventorySlotAction>(a);};
    nativeInputReceived=[&](const SDL_Event &e){if(fresh && e.type==SDL_KEYDOWN && e.key.keysym.sym==code && !e.key.repeat)++received;};
    nativeInputRetired=[&](const SDL_KeyboardEvent &e){if(fresh && e.keysym.sym==code && !e.repeat)++retired;};
    auto native=handler;native.closed={};native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};
    native.withPresentedInput=[&](const PlayerAction &a,std::uint64_t input,const auto &origin){
        const auto generation=combat().result().generation;
        if(fresh && matches(a))++dispatched;
        auto f=handler.withPresentedInput(a,input,origin);
        if(fresh && matches(a) && combat().result().generation!=generation)++accepted;
        return f;
    };
    native.framePresented=[&](const auto &frame){handler.framePresented(frame);shown=true;};
    composeProbe=[&]{
        if(!transition && combat().phase()==XeenCombatPhase::PlayerReady && combat().participant()==5) {
            // This key arrives after the old actor's Block but before the new actor's presentation.
            transition=true;key(code);key(code,SDL_KEYDOWN,1);
        }
    };
    const auto drive=[&]()->std::optional<IndexedFrame>{
        check(++iterations<100,"M39 input SDL lifecycle bound");
        if(stage==0){check(shown,"M39 input initial PlayerReady presented");key(SDLK_b);++stage;return {};}
        if(!transition)return idle();
        check((combat().phase()==XeenCombatPhase::PlayerReady && combat().participant()==5) || (fresh && accepted),"M39 input unexpected actor work");
        if(stage==1) {
            check(shown && !combat().current(initialTicket) && replay_test::commands==baselineCommands+1,"M39 early input crossed actor handoff");
            key(code);key(code,SDL_KEYDOWN,1);++stage;return {};
        }
        if(stage==2) {
            check(replay_test::commands==baselineCommands+1 && !combat().cast(),"M39 held/repeated input crossed actor handoff");
            key(code,SDL_KEYUP);key(SDLK_b,SDL_KEYUP);++stage;return {};
        }
        if(stage==3) {
            // Keyup was drained in its own batch. The same ready actor is still presented.
            check(flow.journeyInputCurrent(handler.displayedInput()),"M39 stable PlayerReady authority unavailable");
            stableInput=handler.displayedInput();stableTicket=combat().ticket();stableFrame=flow.frame().presentation();
            const auto encounter=flow.encounter()->ticket();const auto generation=combat().result().generation;
            fresh=true;
            composeProbe=[&]{
                if(cosmetic)return;
                check(combat().current(*stableTicket) && combat().participant()==5,"M39 cosmetic changed semantic combat ticket");
                cosmetic=true;key(code);key(code,SDL_KEYUP);
                // Another eligible action in this physical batch must not run.
                key(code==SDLK_b?SDLK_SPACE:SDLK_b);key(code==SDLK_b?SDLK_SPACE:SDLK_b,SDL_KEYUP);
            };
            now+=100;auto f=idle();composeProbe={};
            cosmeticInput=handler.displayedInput();
            check(cosmetic && f && f->presentation()!=stableFrame && !flow.acceptsFrame(stableFrame) &&
                !flow.acceptsFrame(IndexedFrame{}.presentation()) && flow.encounter()->current(encounter) &&
                combat().current(*stableTicket),"M39 cosmetic concrete/semantic identity split");
            check(flow.journeyInputCurrent(stableInput) && flow.acceptsInputFrame(stableFrame) &&
                !flow.acceptsInputFrame(f->presentation()),"M39 cosmetic must retain A until acquired B handoff");
            handler.withPresentedInput(AttackAction{},*stableInput,f->presentation());
            check(combat().result().generation==generation,"M39 early response to unpresented cosmetic frame executed");
            shown=false;++stage;return f;
        }
        if(stage==4) {
            check(received==1 && retired==0 && dispatched==1 && accepted==1 &&
                !combat().current(*stableTicket),"M39 first live-A cosmetic tap was lost or duplicated");
            // Replay still names A; it cannot authorize the semantic successor.
            const auto generation=combat().result().generation;
            handler.withPresentedInput(AttackAction{},*stableInput,stableFrame);
            check(combat().result().generation==generation,"M39 retired A response authorized successor");
            ++stage;return {};
        }
        std::cout<<"M39 INPUT TRACE "<<control<<" received="<<received<<" retired="<<retired<<" dispatched="<<dispatched
            <<" accepted="<<accepted<<" epoch="<<*stableInput<<"->"<<*cosmeticInput<<"->"<<*handler.displayedInput()
            <<" original-ticket-current="<<combat().current(*stableTicket)<<'\n';
        check(cosmeticInput==stableInput && received==1 && retired==0 && dispatched==1 && accepted==1,"M39 live-A cosmetic input must execute exactly once");
        check(replay_test::commands==baselineCommands+(control=="attack" || control=="block" || control=="run"?2:1),"M39 second action executed in same native batch");
        check(!combat().current(*stableTicket) && !combat().current(XeenCombat::Ticket{}),"M39 stale/foreign semantic ticket accepted");
        const auto generation=combat().result().generation;
        const auto commands=replay_test::commands;
        handler.withDisplayedInput(AttackAction{},*stableInput);
        check(combat().result().generation==generation && replay_test::commands==commands,"M39 old displayed input repeated accepted command");
        SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);++stage;return {};
    };
    const auto ok=show(flow.frame(),native,escape,drive,status);
    composeProbe={};nativeInputReceived={};nativeInputRetired={};
    check(ok && accepted==1 && stage==6,"M39 native PlayerReady input control failed");
    std::cout<<"M39 PLAYERREADY INPUT PASSED "<<control<<'\n';return true;
}
#endif
