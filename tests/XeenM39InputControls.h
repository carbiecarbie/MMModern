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
    const auto stableTicket=combat().ticket();
    std::optional<std::uint64_t> stableInput,cosmeticInput;
    IndexedFrame::Presentation stableFrame,firstOrigin;
    const auto baselineCommands=replay_test::commands;
    unsigned stage=0,iterations=0,accepted=0,dispatched=0,received=0,retired=0,total=0;
    bool cosmetic=false,transition=false,shown=false;
    const auto matches=[&](const PlayerAction &a){return control=="attack"?std::holds_alternative<InteractionAction>(a):
        control=="block"?std::holds_alternative<BlockAction>(a):control=="run"?std::holds_alternative<RevisitCompletedAction>(a):
        control=="cast"?std::holds_alternative<CastSpellAction>(a):std::holds_alternative<SelectInventorySlotAction>(a);};
    nativeInputReceived=[&](const SDL_Event &e){if(transition && e.type==SDL_KEYDOWN && e.key.keysym.sym==code && !e.key.repeat)++received;};
    nativeInputRetired=[&](const SDL_KeyboardEvent &e){if(transition && e.keysym.sym==code && !e.repeat)++retired;};
    auto native=handler;native.closed={};native.beginCycle=[&](std::uint64_t){handler.beginCycle(++cycle);};
    native.framePresented=[&](const auto &frame){handler.framePresented(frame);shown=true;};
    native.withPresentedInput=[&](const PlayerAction &a,std::uint64_t input,const auto &origin){
        check(handler.inputContext(origin).readyForAction && handler.acceptsInputFrame(origin),"M39 queued input drained before current readiness");
        const auto generation=combat().result().generation;
        const bool second=total++!=0;
        if(second){check(matches(a) && origin!=firstOrigin,"M39 FIFO changed action or two actions ran on one frame");++dispatched;}
        else {check(std::holds_alternative<BlockAction>(a),"M39 cosmetic first Block lost");firstOrigin=origin;}
        auto f=handler.withPresentedInput(a,input,origin);
        if(second && combat().result().generation!=generation)++accepted;
        return f;
    };
    const auto drive=[&]()->std::optional<IndexedFrame>{
        check(++iterations<100,"M39 input SDL lifecycle bound");
        if(stage==0){
            check(shown,"M39 input initial PlayerReady presented");
            stableFrame=flow.frame().presentation();stableInput=handler.displayedInput();
            const auto encounter=flow.encounter()->ticket();const auto generation=combat().result().generation;
            composeProbe=[&]{if(cosmetic)return;cosmetic=true;key(SDLK_b);key(SDLK_b,SDL_KEYUP);};
            now+=100;auto f=idle();composeProbe={};cosmeticInput=handler.displayedInput();
            check(cosmetic && f && f->presentation()!=stableFrame && !flow.acceptsFrame(stableFrame) &&
                !flow.acceptsFrame(IndexedFrame{}.presentation()) && flow.encounter()->current(encounter) &&
                combat().current(stableTicket),"M39 cosmetic concrete/semantic identity split");
            check(flow.journeyInputCurrent(stableInput) && flow.acceptsInputFrame(stableFrame) &&
                !flow.acceptsInputFrame(f->presentation()),"M39 cosmetic must retain A until acquired B handoff");
            handler.withPresentedInput(AttackAction{},*stableInput,f->presentation());
            check(combat().result().generation==generation,"M39 early response to unpresented cosmetic frame executed");
            // This physical edge arrives during the first action's redraw. Its
            // repeat and held duplicate must not create another queued command.
            composeProbe=[&]{if(transition || total!=1)return;transition=true;key(code);key(code,SDL_KEYDOWN,1);key(code);};
            ++stage;return f;
        }
        if(total<2)return idle();
        check(transition && received==2 && retired==0 && dispatched==1 && accepted==1 && total==2,
            "M39 queued handoff tap lost/duplicated or held/repeat accepted");
        check(cosmeticInput==stableInput,"M39 cosmetic changed semantic input");
        check(replay_test::commands==baselineCommands+(control=="attack" || control=="block" || control=="run"?2:1),"M39 queued commands lost/duplicated");
        check(!combat().current(stableTicket) && !combat().current(XeenCombat::Ticket{}),"M39 stale/foreign semantic ticket accepted");
        const auto generation=combat().result().generation;const auto commands=replay_test::commands;
        handler.withPresentedInput(AttackAction{},*stableInput,stableFrame);
        check(combat().result().generation==generation,"M39 retired A response authorized successor");
        handler.withDisplayedInput(AttackAction{},*stableInput);
        check(combat().result().generation==generation && replay_test::commands==commands,"M39 old displayed input repeated accepted command");
        key(code,SDL_KEYUP);
        std::cout<<"M39 INPUT TRACE "<<control<<" received="<<received<<" retired="<<retired<<" dispatched="<<dispatched
            <<" accepted="<<accepted<<" total-ready-frames="<<total<<'\n';
        SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);++stage;return {};
    };
    const auto ok=show(flow.frame(),native,escape,drive,status);
    composeProbe={};nativeInputReceived={};nativeInputRetired={};
    check(ok && accepted==1 && stage==2,"M39 native PlayerReady input control failed");
    std::cout<<"M39 PLAYERREADY INPUT PASSED "<<control<<'\n';return true;
}
#endif
