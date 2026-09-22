// Included by the original-resource controls executable. These are labelled
// artificial saved arrangements and literal draws, never genuine route evidence.
XeenSaveSnapshot disengagementFixture(Source &source) {
 Domain fresh(source,{},5);auto saved=fresh.save();
 for(auto id:kXeenCombatOwners) {
  saved.characters[id].currentHp=1000;
  saved.journey->supplements[id].inputs.speed={255,0};
 }
 auto &survivor=saved.journey->actors[9];survivor.x=8;survivor.y=11;survivor.hp=16;survivor.activated=true;
 return saved;
}
XeenCombat &disengagementContact(Domain &d) {
 check(d.flow->journeyAction(d.flow->ticket(),XeenEncounterAction::Forward).outcome==XeenEncounterOutcome::Accepted,"Artificial M34 contact move");
 if(d.flow->state().phase()==XeenEncounterPhase::Exploring)d.flow->journeyPulse(d.flow->ticket());
 check(d.flow->attachJourney(d.flow->ticket(),[]{}),"Artificial M34 combat attachment");
 auto &combat=*d.flow->combat();
 for(unsigned n=0;n<40 && combat.phase()!=XeenCombatPhase::PlayerReady;++n) {
  const auto r=combat.service(combat.ticket());
  check(r.status!=XeenCombatStatus::Failed && r.status!=XeenCombatStatus::SupportStopped,"M34 contact automatic work");
 }
 check(combat.phase()==XeenCombatPhase::PlayerReady && combat.participants()==0x3f,"M34 initial full participation");
 return combat;
}
XeenCombatResult literalRun(XeenCombat &combat,unsigned roll) {
 const auto owner=combat.participant();const auto before=combat.participants();
 const auto old=combat.ticket();const auto random=combat.random().continuation();
 check(combat.command(old,XeenCombatCommand::Run).status==XeenCombatStatus::Pending,"Run prepares through ordinary combat authority");
 check(combat.participants()==before && combat.random().continuation()==random,"Run intent publishes no roll or membership");
 check(combat.command(old,XeenCombatCommand::Run).status==XeenCombatStatus::Stale,"Stale Run cannot queue a second action");
 attack(combat,{{1,100,roll}});const auto result=combat.result();
 check(result.operation==XeenCombatOperation::PlayerRun && result.actingOwner==kXeenCombatOwners[owner] && result.runRoll==roll && result.runSuccess==(roll<100),"Literal named individual Run result");
 check(combat.participants()==(roll<100?(before&~(1u<<owner)):before),"Only successful acting slot leaves participation");
 return result;
}
void disengagementSurvivor(Source &source) {
 auto saved=disengagementFixture(source);Domain d(source,saved);auto &combat=disengagementContact(d);
 const auto before=d.party.roster.characters();const auto membership=d.party.party.activeRosterIds();
 const auto actors=d.world.sessionState().actors();const auto context=d.party.encounterContext;const auto origin=d.camera;
 const auto startRandom=d.world.sessionState().journeyRandom()->count;
 for(unsigned owner=0;owner<6;++owner) {
  check(combat.phase()==XeenCombatPhase::PlayerReady && combat.participant()==int(owner),"Equal-speed Run retains stable owner order and earlier actions");
  literalRun(combat,owner==5?99:1);
  for(unsigned n=0;n<30;++n)check(xeen_state::sameCharacter(before[n],d.party.roster.at(n)),"Run leaves every durable character byte untouched");
  check(d.party.party.activeRosterIds()==membership && d.party.encounterContext==context,"Run retains membership and adds no time");
  check(!d.flow->canSave() && !XeenSaveState::canCapture(d.party,d.camera,d.world),"Partial Run and pending finish remain unsaveable");
 }
 check(combat.phase()==XeenCombatPhase::DisengagementPending && combat.pending()==XeenCombatWork::FinishDisengagement,"Last runner creates distinct finish obligation");
 check(d.world.sessionState().journeyRandom()->count==startRandom+6 && xeen_state::sameCamera(origin,d.camera),"Six accepted draws precede relocation");
 const auto finish=combat.service(combat.ticket());
 check(finish.operation==XeenCombatOperation::FinishDisengagement && finish.phase==XeenCombatPhase::Disengaged && finish.exitCause==XeenCombatExitCause::DirectRun && !finish.casualties,("Successful non-victory finish op="+std::to_string(unsigned(finish.operation))+" phase="+std::to_string(unsigned(finish.phase))+" failure="+std::to_string(unsigned(finish.failure))+" cause="+std::to_string(unsigned(finish.exitCause))).c_str());
 check(d.camera.x==10 && d.camera.y==12 && d.camera.direction==origin.direction && d.party.encounterContext==context,"Fixed relocation preserves facing and time");
 for(unsigned n=0;n<19;++n) {
  const auto &a=actors[n],&b=d.world.sessionState().actors()[n];
  check(a.id==b.id && a.x==b.x && a.y==b.y && a.hp==b.hp && a.lifecycle==b.lifecycle && (!a.activated || b.activated),"Relocation retains every actor identity, position, wound and lifecycle");
 }
 check(d.world.sessionState().actors()[9].hp==16 && !d.world.sessionState().accountedMonsters().count({23,9}),"Wounded live survivor never regenerates or accounts at finish");
 check(!d.flow->canSave(),"Successful finish is not a premature quiet save boundary");
 const auto stale=d.flow->ticket();check(d.flow->retireJourney(stale),"Separate successful-finish retirement");
 check(!d.flow->combat() && !d.flow->canSave() && d.world.sessionState().journeyActivity()==XeenJourneyActivity::Presentation,"Retirement retains mandatory presentation before Quiet");
 check(!d.flow->retireJourney(stale),"Retirement proof cannot replay");
 d.present();check(d.flow->canSave(),"Fresh presented relocation becomes saveable");
 const auto checkpoint=d.save();Domain restored(source,checkpoint);save_test::sameSnapshot(checkpoint,restored.save());
 check(XeenSaveFormat::encode(checkpoint)==XeenSaveFormat::encode(restored.save()),"Survivor retirement restore has exact 5/5 bytes");
 for(auto *branch:{&d,&restored}) {
  check(branch->flow->journeyAction(branch->flow->ticket(),XeenEncounterAction::Right).outcome==XeenEncounterOutcome::Accepted,"Both restored branches continue ordinary mutation");
  branch->present();
 }
 save_test::sameSnapshot(d.save(),restored.save());
 check(XeenSaveFormat::encode(d.save())==XeenSaveFormat::encode(restored.save()),"Identical further mutation preserves exact bytes");
 std::cout<<"M34 ARTIFICIAL six ordered Runs, surviving HP16, fixed retirement, no Quiet gap, 5/5 restore and further turn PASS\n";
}
void disengagementMixedVictory(Source &source) {
 const auto saved=disengagementFixture(source);Domain d(source,saved);auto &combat=disengagementContact(d);
 const auto selected=combat.selectedTarget();
 literalRun(combat,100);
 check(combat.participants()==0x3f && combat.participant()==1 && combat.selectedTarget()==selected,"Failed Run spends owner0 action and retains target");
 const auto escaped=kXeenCombatOwners[combat.participant()];const auto xp=d.party.roster.combatInputs(escaped)->experience;
 literalRun(combat,1);check(combat.participant()==2,"Successful peer Run never replays faster acted owner");
 for(unsigned n=0;n<200 && combat.phase()!=XeenCombatPhase::Victory;++n) {
  check(combat.phase()!=XeenCombatPhase::Failed && combat.phase()!=XeenCombatPhase::Defeat && combat.phase()!=XeenCombatPhase::SupportStopped,"Mixed Run branch remains admitted");
  if(combat.phase()==XeenCombatPhase::PlayerReady) {
   check(kXeenCombatOwners[combat.participant()]!=escaped,"Escaped member excluded from later turns");
   combat.command(combat.ticket(),XeenCombatCommand::Attack);
  } else combat.service(combat.ticket());
 }
 check(combat.phase()==XeenCombatPhase::Victory && d.world.sessionState().accountedMonsters().count({23,9}),"Mixed Run preserves successful-victory End boundary");
 check(d.party.roster.combatInputs(escaped)->experience==xp,"Escaped owner receives no subsequent kill XP");
 bool awarded=false;for(auto id:kXeenCombatOwners)if(id!=escaped)awarded=awarded || d.party.roster.combatInputs(id)->experience>saved.journey->supplements[id].inputs.experience;
 check(awarded && d.party.party.activeRosterIds()==saved.activeRosterIds,"Remaining participants receive XP without roster mutation");
 check(d.flow->retireJourney(d.flow->ticket()),"Victory after partial escape retires through M33 End");
 std::cout<<"M34 ARTIFICIAL failed100/success1 Run, stable acted peers, continued victory and escaped-member XP exclusion PASS\n";
}
void disengagementOwedRound(Source &source) {
 for(bool escapedStatDeath:{false,true}) {
  auto saved=disengagementFixture(source);saved.journey->context->minutes=949;
  for(unsigned slot=1;slot<6;++slot) {
   const auto owner=kXeenCombatOwners[slot];saved.journey->supplements[owner].inputs.speed={1,0};
   saved.characters[owner].conditions[13]=slot==1?0:7;
  }
  saved.characters[kXeenCombatOwners[1]].currentHp=1;
  if(escapedStatDeath) {saved.characters[0].conditions[3]=1;saved.journey->supplements[0].inputs.might={1,0};}
  Domain d(source,saved);auto &combat=disengagementContact(d);
  check(combat.participant()==0 && d.party.encounterContext->minutes==959,"Owed-round fixture precedes960 tick");
  literalRun(combat,1);check(combat.pending()==XeenCombatWork::Enemy,"Slow participant follows first enemy after escape");
  attack(combat,{{0,4,0},{1,20,1}});
  check(combat.phase()==XeenCombatPhase::PlayerReady && combat.participant()==1,"First miss retains slow participant's turn");
  combat.command(combat.ticket(),XeenCombatCommand::Block);
  check(combat.pending()==XeenCombatWork::Round,"Block reaches next ordinary charged-round opportunity");
  combat.service(combat.ticket());
  check(combat.pending()==XeenCombatWork::Enemy && d.party.encounterContext->minutes==959,"Faster enemy precedes owed charged movement and minute");
  attack(combat,{{0,4,0},{1,20,19},{1,5,5},{1,10,10}});
  check(combat.phase()==XeenCombatPhase::DisengagementPending && combat.pending()==XeenCombatWork::Round && combat.exitCause()==XeenCombatExitCause::AttritionAfterEscape,"Incapacitation preserves owed charged round and attrition cause");
  const auto origin=d.camera;const auto actor=d.world.sessionState().actors()[9];
  for(unsigned n=0;n<8 && combat.pending()==XeenCombatWork::Round;++n)combat.service(combat.ticket());
  check(d.party.encounterContext->minutes==960 && d.party.encounterContext->ctr24==saved.journey->context->ctr24+1,"Owed charged minute and exploration step are retained exactly once");
  if(escapedStatDeath) {
   check(combat.phase()==XeenCombatPhase::Defeat && d.party.roster.at(0).conditions[13]==2 && d.party.roster.at(0).currentHp==1000,"Full-party time kills escaped zero-Might owner without clamping HP");
   check(xeen_state::sameCamera(origin,d.camera) && !d.flow->canSave(),"Owed-tick whole-party defeat prevents relocation/save");
  } else {
   check(combat.pending()==XeenCombatWork::FinishDisengagement,"Owed movement drains before attrition finish");
   const auto finished=combat.service(combat.ticket());
   check(finished.phase==XeenCombatPhase::Disengaged && finished.exitCause==XeenCombatExitCause::AttritionAfterEscape && finished.casualties==2 && !finished.forfeitedGold,"Attrition converts only newly abandoned casualty");
   check(d.party.roster.at(kXeenCombatOwners[1]).currentHp==-9 && d.party.roster.at(kXeenCombatOwners[1]).conditions[12]==1 && d.party.roster.at(kXeenCombatOwners[1]).conditions[13]==1,"Casualty conversion preserves injury and adds only Dead byte");
   for(unsigned slot=2;slot<6;++slot)check(d.party.roster.at(kXeenCombatOwners[slot]).conditions[13]==8,"Already-Dead counters receive tick only, no casualty reset");
   check(d.world.sessionState().actors()[9].hp==actor.hp && d.party.encounterContext->minutes==960,"Finish neither regenerates survivor nor adds End minute");
  }
 }
 std::cout<<"M34 ARTIFICIAL escaped/slow participant, enemy before owed charged round, attrition casualty and full-party escaped stat-death tick PASS\n";
}
XeenGameplayServices disengagementServices(Source &source,combat_gameplay_test::Harness &h) {
 h.signature=source.signature;h.font=XeenFontFormat(source.assets.readArchiveResource("fnt"));
 auto services=h.services();services.resources.signature=source.signature;
 services.resources.loadInitialParty=[&]{return XeenPartyLoader().loadFromResources(source.chr,source.pty);};
 services.resources.loadInitialCharacters=[&]{return source.chr;};
 services.resources.loadInitialContext=[&]{return XeenGameplayContextFormat::parse(source.pty);};
 services.resources.loadMonsterStatistics=[&]{return source.mon;};
 services.resources.loadEvents=[&](auto){return source.evt;};
 services.resources.regionalManifest=source.manifest();
 services.maps=[&](auto id){return source.maps.loadGeometryMap(source.assets,id);};
 services.objects=[&](auto id){return source.maps.loadObjects(source.assets,id);};
 services.validateEncounterSprite=[&](std::uint8_t image){source.assets.validateNormalMonster(image);};
 services.validateCombatSprite=[&](std::uint8_t image){source.assets.validateAttackMonster(image);};
 services.composeEncounter=[&](auto &w,const auto &party,const auto &camera,auto phase,auto appearance){
  XeenEventFlow::Composition result;
  result.frame=CloudsMapComposer().compose(source.assets,w,party,camera,{610},nullptr,phase,&result.containsOrdinaryAnimation,appearance);
  return result;
 };
 return services;
}
void disengagementSemanticInput(Source &source) {
 const auto path=std::filesystem::temp_directory_path()/"mmodern-m34-artificial-input.mms";
 XeenSaveFile::write(path,disengagementFixture(source));combat_gameplay_test::Harness h;
 auto services=disengagementServices(source,h);bool checked=false;
 services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
  const auto present=[&]{check(handler.frameCurrent(),"M34 semantic current concrete frame");handler.framePresented(h.flow->frame().presentation());};
  const auto press=[&](const PlayerAction &a){handler.beginCycle(++h.cycle);handler.withDisplayedInput(a,*handler.displayedInput());present();};
  present();const auto oldFrame=h.flow->frame().presentation();press(NavigationAction::MoveForward);
  for(unsigned n=0;n<80 && (!h.flow->encounter()->combat() || h.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n){h.now+=100;handler.beginCycle(++h.cycle);idle();present();}
  auto *combat=h.flow->encounter()->combat();check(combat && combat->phase()==XeenCombatPhase::PlayerReady,"M34 semantic combat ready");
  press(SaveGameAction{});check(!h.saves,"Combat F9 invokes zero capture/preflight/write providers");
  const auto oldInput=*handler.displayedInput();const auto before=combat->participants();
  handler.beginCycle(++h.cycle);handler.withDisplayedInput(RevisitCompletedAction{},oldInput);
  check(combat->phase()==XeenCombatPhase::PreparingAction && combat->result().operation==XeenCombatOperation::PlayerRun,"Native R semantic action normalizes to typed Run");
  XeenRestoreGuard retained(*h.world,*h.party,*h.camera,*h.flags);
  handler.withDisplayedInput(RevisitCompletedAction{},oldInput);handler.withDisplayedInput(SaveGameAction{},oldInput);
  check(retained.current() && combat->participants()==before && !h.saves,"Same-batch/stale R and F9 cannot publish or invoke providers");
  for(const auto &invalid:std::vector<IndexedFrame::Presentation>{oldFrame,{},std::make_shared<const IndexedFrame>(h.flow->frame())}) {
   bool rejected=false;try{handler.framePresented(invalid);}catch(const std::exception &){rejected=true;}
   check(rejected && retained.current(),"Previous, missing or foreign concrete frame cannot present pending Run");
  }
  present();tape={{1,100,1}};cursor=0;taped=true;
  for(unsigned n=0;n<40 && combat->phase()!=XeenCombatPhase::PlayerReady;++n){h.now+=100;handler.beginCycle(++h.cycle);idle();if(combat->phase()!=XeenCombatPhase::PlayerReady)present();}
  taped=false;check(cursor==1 && combat->participants()==(before&~1u),"Exactly one Run publication from native-semantic R");
  XeenRestoreGuard nextOwner(*h.world,*h.party,*h.camera,*h.flags);
  handler.withDisplayedInput(RunAction{},*handler.displayedInput());
  check(nextOwner.current() && combat->phase()==XeenCombatPhase::PlayerReady,"Current semantic ticket without concrete ready frame cannot Run");
  present();const auto current=*handler.displayedInput();
  handler.withDisplayedInput(RunAction{},oldInput);handler.withDisplayedInput(SaveGameAction{},oldInput);
  check(nextOwner.current() && !h.saves,"Previous owner cannot Run or save at next owner");
  handler.beginCycle(++h.cycle);handler.withDisplayedInput(RunAction{},current);present();
  check(combat->phase()==XeenCombatPhase::PreparingAction && combat->result().actingOwner==kXeenCombatOwners[1],"Fresh displayed typed Run reaches the next stable owner");
  press(SaveGameAction{});check(!h.saves,"Pending fresh Run F9 invokes no providers");checked=true;return true;
 };
 check(Application().playGameplay(services,{},path,true)==0 && checked,"M34 original-resource semantic input control");
 std::filesystem::remove(path);
 std::cout<<"M34 ARTIFICIAL semantic/native-R equivalence, stale and previous-frame refusal, fresh next-owner input, zero blocked F9 providers PASS\n";
}
void disengagementNativeInput(Source &source) {
 const auto path=std::filesystem::temp_directory_path()/"mmodern-m34-artificial-native.mms";
 XeenSaveFile::write(path,disengagementFixture(source));combat_gameplay_test::Harness h;
 auto services=disengagementServices(source,h);unsigned accepted=0,stage=0,cycles=0;
 services.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status){
  auto wrapped=handler;
  wrapped.withDisplayedInput=[&](const PlayerAction &a,std::uint64_t input){
   const auto *combat=h.flow->encounter()->combat();const auto generation=combat?combat->result().generation:0;
   auto frame=handler.withDisplayedInput(a,input);combat=h.flow->encounter()->combat();
   if(std::holds_alternative<RevisitCompletedAction>(a) && combat && combat->result().generation!=generation && combat->phase()==XeenCombatPhase::PreparingAction && combat->result().operation==XeenCombatOperation::PlayerRun)++accepted;
   return frame;
  };
  const auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0){SDL_Event event{};event.type=type;event.key.keysym.sym=code;event.key.repeat=repeat;check(SDL_PushEvent(&event)==1,"M34 native queued key");};
  const auto scripted=[&]()->std::optional<IndexedFrame>{
   h.now+=20;auto frame=idle();check(++cycles<300,"M34 automated native input bounded cycles");
   // Queue fresh keys only after a stable presented iteration. A frame returned
   // by idle is uploaded later by SDL and correctly fences already queued keys.
   if(frame)return frame;
   const auto *combat=h.flow->encounter()->combat();const bool ready=combat && combat->phase()==XeenCombatPhase::PlayerReady;
   switch(stage){
    case 0:key(SDLK_UP);stage=1;break;
    case 1:if(ready)stage=2;break;
    case 2:
     tape.assign(6,{1,100,1});cursor=0;taped=true;
     key(SDLK_r);key(SDLK_r,SDL_KEYDOWN,1);key(SDLK_r,SDL_KEYUP);key(SDLK_r);key(SDLK_F9);key(SDLK_SPACE);key(SDLK_b);stage=3;break;
    case 3:if(ready){check(accepted==1 && combat->participants()==0x3e && !h.saves,("Native same-batch R/repeat/Space/B/F9 accepts exactly one Run accepted="+std::to_string(accepted)+" mask="+std::to_string(combat->participants())+" saves="+std::to_string(h.saves)+" cursor="+std::to_string(cursor)).c_str());stage=4;}break;
    case 4:key(SDLK_r);stage=5;break;
    case 5:check(accepted==1,"Held native R cannot act for next owner");key(SDLK_r,SDL_KEYUP);stage=6;break;
    case 6:key(SDLK_r);stage=7;break;
    case 7:if(ready){check(accepted==2,"Released fresh native R accepts second owner");stage=8;}break;
    case 8:key(SDLK_r,SDL_KEYUP);stage=9;break;
    case 9:key(SDLK_r);stage=10;break;
    case 10:if(ready)stage=8;else if(!combat)stage=11;break;
    case 11:if(h.flow->canSave()){check(accepted==6 && cursor==6 && !h.saves,"Exactly six native Runs finish and retire without stale F9");stage=12;}break;
    case 12:key(SDLK_r);key(SDLK_SPACE);key(SDLK_F9);stage=13;break;
    case 13:check(accepted==6 && !h.saves,"Held R/Space/F9 crossing retirement has no action or save");key(SDLK_r,SDL_KEYUP);key(SDLK_SPACE,SDL_KEYUP);key(SDLK_F9,SDL_KEYUP);stage=14;break;
    case 14:key(SDLK_F9);stage=15;break;
    case 15:check(h.saves==3,"Fresh native F9 after handoff calls capture/preflight/write exactly once");key(SDLK_ESCAPE);stage=16;break;
    default:break;
   }
   return frame;
  };
  return SdlWindow().showInteractive(first,"M34 automated original-resource native controls",wrapped,escape,scripted,status);
 };
 const char *video=SDL_getenv("SDL_VIDEODRIVER"),*renderer=SDL_getenv("SDL_RENDER_DRIVER");
 const std::string oldVideo=video?video:"",oldRenderer=renderer?renderer:"";
 SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_RENDER_DRIVER","software",1);
 const int result=Application().playGameplay(services,{},path,true);taped=false;
 SDL_setenv("SDL_VIDEODRIVER",oldVideo.c_str(),1);SDL_setenv("SDL_RENDER_DRIVER",oldRenderer.c_str(),1);
 check(result==0 && stage==16,"M34 native input sequence completed");
 std::filesystem::remove(path);
 std::cout<<"M34 ARTIFICIAL automated SDL R held/repeat/same-batch and retirement R/Space/F9 fences; fresh F9 provider path PASS\n";
}
void disengagementFinishPresentation(Source &source) {
 for(bool occupied:{false,true})for(bool attrition:{false,true}) {
  auto saved=runAuthorityFixture(source,occupied);
  if(attrition) {saved.characters[kXeenCombatOwners[1]].currentHp=1;saved.characters[kXeenCombatOwners[1]].conditions[12]=0;}
  const auto path=std::filesystem::temp_directory_path()/"mmodern-m34-finish-presentation.mms";
  XeenSaveFile::write(path,saved);combat_gameplay_test::Harness h;auto services=disengagementServices(source,h);
  const auto compose=services.composeEncounter;IndexedFrame base;
  services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto appearance){auto result=compose(w,p,c,phase,appearance);base=result.frame;return result;};
  bool completed=false;
  services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
   const auto present=[&]{handler.framePresented(h.flow->frame().presentation());};
   const auto press=[&](const PlayerAction &a){handler.beginCycle(++h.cycle);handler.withDisplayedInput(a,*handler.displayedInput());present();};
   const auto tick=[&]{h.now+=1000;handler.beginCycle(++h.cycle);idle();};
   const auto values=[&]{
    auto value=saved;value.camera=*h.camera;value.characters=h.party->roster.characters();value.activeRosterIds=h.party->party.activeRosterIds();
    value.journey->context=h.party->encounterContext;value.journey->treasure=h.party->monsterTreasure;value.journey->random=h.world->sessionState().journeyRandom();
    for(unsigned i=0;i<30;++i)value.journey->supplements[i].inputs=*h.party->roster.combatInputs(i);
    for(unsigned i=0;i<19;++i){const auto &a=h.world->sessionState().actors()[i];auto &b=value.journey->actors[i];b.x=a.x;b.y=a.y;b.hp=a.hp;b.activated=a.activated;b.lifecycle=a.lifecycle;b.status=a.status;b.accounted=h.world->sessionState().accountedMonsters().count(a.id);}
    return value;
   };
   present();press(NavigationAction::MoveForward);
   for(unsigned n=0;n<80 && (!h.flow->encounter()->combat() || h.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n){tick();present();}
   check(h.flow->encounter()->combat() && h.flow->encounter()->combat()->participant()==0,"Finish presentation fixture reaches original combat");
   for(unsigned exit=0;exit<(occupied?2u:1u);++exit) {
    const auto oldFrame=h.flow->frame().presentation();const auto oldInput=*handler.displayedInput();
    const auto oldTicket=h.flow->encounter()->combat()->ticket();
    bool retried=false;std::unique_ptr<XeenRestoreGuard> publication;
    h.flow->beforeEncounterFrameCopy=[&]{
     if(!retried && h.flow->encounter()->notice().find("Disengaged; gold forfeited ")!=std::string::npos) {
      publication=std::make_unique<XeenRestoreGuard>(*h.world,*h.party,*h.camera,*h.flags);
      retried=true;throw std::runtime_error("Artificial destination frame-copy retry");
     }
    };
    press(RunAction{});
    check(h.flow->encounter()->notice().find("Disengaged;")==std::string::npos,"New intent cannot retain prior finish feedback");
    for(unsigned n=0;n<100 && !retried;++n) {
     auto *c=h.flow->encounter()->combat();
     if(c && c->phase()==XeenCombatPhase::PreparingAction) {tape={{1,100,1}};cursor=0;taped=true;}
     else if(c && c->pending()==XeenCombatWork::Enemy) {tape={{0,4,0},{1,20,19},{1,5,5},{1,10,10}};cursor=0;taped=true;}
     tick();taped=false;
     if(!retried)present();
    }
    h.flow->beforeEncounterFrameCopy={};
    check(retried && publication && publication->current(),"Destination retry preserves every durable owner, actor, RNG/time and consequence");
    const auto notice=h.flow->encounter()->notice();
    const unsigned forfeited=(!exit && !attrition) || (exit && attrition)?10:0;
    check(notice.find("Disengaged; gold forfeited "+std::to_string(forfeited))!=std::string::npos,"Matching finish forfeiture is visible in destination notice");
    check((notice.find("Abandoned:")!=std::string::npos)==!exit,"Only the current exit reports newly abandoned members");
    if(!exit)for(unsigned slot:{1u,3u,4u,5u})check(notice.find(h.party->roster.at(kXeenCombatOwners[slot]).name,notice.find("Abandoned:"))!=std::string::npos,"Casualty names survive destination attachment");
    const bool ready=attrition && !exit;
    check(notice.find(ready?"Ready treasure: +10 gold":"Dormant items; no gold owed")!=std::string::npos,"Matching ready/dormant consequence is visible");
    check(h.party->monsterTreasure->gold==saved.journey->treasure->gold && h.party->monsterTreasure->pendingGold==(ready?10:0) &&
     h.party->monsterTreasure->armor==saved.journey->treasure->armor,"Feedback does not credit, forfeit twice or alter item provenance");
    check(h.party->roster.at(kXeenCombatOwners[2]).conditions[13]==7,"Feedback preserves existing Dead counter");
    check(h.camera->x==10 && h.camera->y==12 && h.world->sessionState().actors()[9].hp==16,"Relocation and wounded survivor remain published exactly once");
    auto *replacement=h.flow->encounter()->combat();
    check(bool(replacement)==occupied,"Destination occupancy determines immediate attachment");
    if(replacement)check(!replacement->current(oldTicket) && replacement->participants()==0x3f &&
     h.world->sessionState().journeyActivity()!=XeenJourneyActivity::Quiet,"Immediate incarnation replaces old authority without Quiet");
    check(h.flow->encounter()->appearance().kind==XeenMonsterSpriteKind::Normal &&
     !h.flow->encounter()->appearance().projectile,"Destination presentation does not inherit old ATT or projectile state");
    const auto replacementTicket=replacement?std::optional<XeenCombat::Ticket>{replacement->ticket()}:std::nullopt;
    // The real rendered frame must contain exactly the checked notice, not just a diagnostic string.
    XeenTextRenderOptions options;options.bounds={3,107,229,200};options.windowBounds={1,105,231,200};options.x=3;options.y=107;
    options.size=XeenFontSize::Reduced;options.paginate=true;options.drawWindow=true;
    const auto split=notice.find("\n\n");auto rendered=XeenTextRenderer(h.font).render(base,notice.substr(0,split),options);
    check(rendered.pages.size()==1,"Destination consequence panel fits original font");
    options.bounds={235,3,318,198};options.windowBounds={233,1,320,200};options.x=235;options.y=3;
    rendered=XeenTextRenderer(h.font).render(rendered.pages.front(),notice.substr(split+2),options);
    check(rendered.pages.size()==1 && rendered.pages.front().pixels==h.flow->frame().pixels,"Concrete destination frame renders all matching consequence feedback");
    bool rejected=false;try{handler.framePresented(oldFrame);}catch(const std::exception &){rejected=true;}
    check(rejected,"Prior incarnation frame cannot authorize destination input");
    handler.beginCycle(++h.cycle);handler.withDisplayedInput(RunAction{},oldInput);handler.withDisplayedInput(NavigationAction::MoveForward,oldInput);
    handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());
    check(publication->current() && !h.saves,"No input or F9 gap before exact destination presentation");
    const auto retryFrame=h.flow->frame().presentation();h.flow->refresh(true);
    check(h.flow->encounter()->notice()==notice && publication->current(),"Recomposition retains same finish without replaying gameplay");
    rejected=false;try{handler.framePresented(retryFrame);}catch(const std::exception &){rejected=true;}
    check(rejected,"Recomposition requires its exact concrete frame");const auto beforePresentation=values();present();
    save_test::sameSnapshot(beforePresentation,values());
    check(!replacement || (publication->current() && replacement->current(*replacementTicket) && replacement->participants()==0x3f),"Presentation alone changes neither combat incarnation nor participation");
    if(!occupied)check(h.flow->encounter()->canSave(),"Normal destination still becomes Quiet only after presentation");
   }
   completed=true;return true;
  };
  check(Application().playGameplay(services,{},path,true)==0 && completed,"Actual EventFlow finish handoff regression");
  std::filesystem::remove(path);
 }
 std::cout<<"M34 ARTIFICIAL EventFlow occupied/normal DirectRun/Attrition, repeated finishes, exact frame and retry feedback PASS\n";
}
void disengagementControls(Source &source) {disengagementFinishPresentation(source);disengagementSurvivor(source);disengagementMixedVictory(source);disengagementOwedRound(source);disengagementSemanticInput(source);disengagementNativeInput(source);}
