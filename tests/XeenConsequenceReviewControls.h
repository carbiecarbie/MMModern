// Included by the opt-in original-resource executable inside its fixture namespace.
// Artificial rare-condition/mixed-candidate arrangements, never route evidence.
void repaint(Domain &d) { d.flow->holdJourneyFrame();d.present(); }
void sameLive(const Domain &d,const XeenSaveSnapshot &before) {
 for(unsigned i=0;i<30;++i){check(xeen_state::sameCharacter(d.party.roster.at(i),before.characters[i]),"Unpublished character changed");check(xeen_state::sameInputs(*d.party.roster.combatInputs(i),before.journey->supplements[i].inputs),"Unpublished supplemental owner changed");}
 check(d.party.encounterContext==before.journey->context && d.party.monsterTreasure==before.journey->treasure && d.world.sessionState().journeyRandom()==before.journey->random,"Unpublished time/purse/RNG changed");
 for(unsigned i=0;i<19;++i){const auto &a=d.world.sessionState().actors()[i];const auto &b=before.journey->actors[i];check(a.x==b.x && a.y==b.y && a.hp==b.hp && a.activated==b.activated && a.lifecycle==b.lifecycle,"Unpublished actor changed");}
}
void refusedPendingShoot(Source &s) {
 Domain initial(s);const auto fresh=initial.save();
 for(unsigned mode=0;mode<3;++mode){auto fixture=fresh;
  if(mode==0)for(auto &w:fixture.characters[14].weapons)if(w.frame==4)w.frame=0;
  if(mode==1)fixture.characters[14].conditions[8]=1;
  if(mode==2)fixture.journey->context->minutes=1240;
  Domain d(s,fixture);check(d.flow->handle(NavigationAction::MoveForward),"Arm pending approach through typed movement");repaint(d);
  check(d.flow->state().pending()==2,"Refusal test must retain old work, not Quiet");
  const auto old=d.flow->ticket();const auto actors=d.world.sessionState().actors();const auto random=d.world.sessionState().journeyRandom();const auto context=d.party.encounterContext;
  XeenRestoreGuard retained(d.world,d.party,d.camera,d.flags);
  check(d.flow->handle(ShootAction{}),"Refused Shoot reports feedback");
  check(!d.flow->appearance().projectile && retained.current() && d.flow->state().pending()==2 && d.flow->ticket().generation==old.generation,"Refused Shoot consumed pending work or authority");
  for(unsigned i=0;i<19;++i)check(xeen_state::sameActor(actors[i],d.world.sessionState().actors()[i]),"Refused Shoot moved an actor");
  check(random==d.world.sessionState().journeyRandom() && context==d.party.encounterContext && !d.flow->result().movementOpportunities,"Refused Shoot consumed opportunity/RNG/time");
  check(d.flow->journeyRefusal().find(mode==2?"time boundary":"no awake eligible")!=std::string::npos,"Specific admission refusal");
  repaint(d);
  // An ordinary input is still admitted: no retained Shoot intent blocks it.
  check(d.flow->handle(NavigationAction::TurnRight),"Refusal retained a Shoot obligation");
 }
 auto invalid=fresh;for(auto &w:invalid.characters[14].weapons)if(w.frame==4)w.material=1;
 Domain d(s,invalid);const auto old=d.save();check(d.flow->handle(ShootAction{}),"Unsupported contribution refusal");repaint(d);save_test::sameSnapshot(old,d.save());
 check(d.flow->journeyRefusal().find("weapon")!=std::string::npos,"Invalid equipped contribution feedback");
 std::cout<<"REVIEW pending Shoot: no missile, sleeping missile user, time and invalid equipment admission PASS\n";
}
void chargedWait(Source &s) {
 Domain initial(s);auto fixture=initial.save();fixture.journey->context->minutes=941;
 for(unsigned id:{7u,8u,9u}){auto &a=fixture.journey->actors[id];a.x=6;a.y=11;a.activated=true;}
 Domain d(s,fixture);d.flow->handle(NavigationAction::MoveForward);repaint(d);
 check(d.flow->state().pending()==2 && d.party.encounterContext->minutes==951,"Wait fixture retains old opportunity before Quiet");
 auto before=fixture;before.camera=d.camera;before.journey->context=d.party.encounterContext;
 for(unsigned i=0;i<19;++i){const auto &a=d.world.sessionState().actors()[i];before.journey->actors[i].activated=a.activated;}
 tape.clear();for(unsigned i=0;i<6;++i){tape.push_back({1,10,2});tape.push_back({0,9,0});}
 for(unsigned i=0;i<3;++i){tape.push_back({0,5,i});tape.push_back({1,20,1});}
 // Yield inside the second opportunity, after the first is fully prepared.
 for(unsigned i=0;i<46;++i)tape.push_back({0,5,0,true});
 for(unsigned i=0;i<3;++i){tape.push_back({0,5,5-i});tape.push_back({1,20,1});}
 XeenCombatRandom expected(*d.world.sessionState().journeyRandom());for(const auto &draw:tape)realDraw(&expected,draw.lo,draw.hi);
 cursor=0;taped=true;check(d.flow->handle(WaitAction{}),"Charged Wait accepted with old work pending");
 check(cursor==64 && d.flow->result().outcome==XeenEncounterOutcome::Pending,"Wait yields at exact raw budget in second opportunity");sameLive(d,before);
 check(d.flow->state().pending()==2 && !d.flow->canSave(),"No partial opportunity publication/Quiet");
 repaint(d);const auto r=d.flow->journeyPulse(d.flow->ticket());taped=false;
 check(d.world.sessionState().journeyRandom()==expected.continuation(),"Wait publishes exact RNG continuation after both opportunities");
 check(cursor==70 && r.movementOpportunities==2 && r.consequences && r.consequences->count==6,"Both opportunities publish in one resumed candidate");
 for(unsigned i=0;i<6;++i){const auto &shot=r.consequences->shots[i];check(shot.source.recordIndex==7+i%3 && shot.x==(i<3?6:7) && shot.y==11 && shot.distance==(i<3?2u:1u),"Retained old/new shot origin and append order");check(shot.attack.targetOwner==kXeenCombatOwners[i<3?i:8-i],"Old/new target draw ordering");}
 check(d.party.encounterContext->minutes==961 && d.party.encounterContext->ctr24==2 && d.world.sessionState().journeyRandom()->count==70 && d.flow->state().phase()==XeenEncounterPhase::Engaged,"Tick precedes old/new ranged work; one final contact publication");
 for(unsigned id:{7u,8u,9u})check(d.world.sessionState().actors()[id].x==8,"Both movement passes published once");
 std::cout<<"REVIEW charged Wait: tick12 -> old shots6 -> rejection46/yield64 -> new shots6; atomic70 PASS\n";
}
XeenCombat &attachSnake(Domain &d){
 d.flow->journeyAction(d.flow->ticket(),XeenEncounterAction::Forward);d.flow->journeyPulse(d.flow->ticket());
 check(d.flow->attachJourney(d.flow->ticket(),[]{}),"Artificial Snake attachment");return *d.flow->combat();
}
void poisonInitiative(Source &s) {
 Domain initial(s);const auto fresh=initial.save();
 for(bool poison:{false,true}){
  auto fixture=fresh;auto &snake=fixture.journey->actors[12];snake.x=8;snake.y=11;snake.activated=true;
  for(unsigned i=0;i<6;++i){const auto id=kXeenCombatOwners[i];fixture.characters[id].currentHp=1000;fixture.characters[id].conditions[8]=1;fixture.journey->supplements[id].inputs.speed={i<2?17:1,0};}
  Domain d(s,fixture);auto &c=attachSnake(d);check(c.pending()==XeenCombatWork::Enemy,"Snake18 precedes sleeping players17");
  std::vector<XeenCombatRandom::Draw> draws;constexpr unsigned saves[]{23,24,22,27,24,25};
  for(unsigned i=0;i<6;++i){draws.push_back({1,10,1});draws.push_back({1,saves[i],i==1 || (i==0 && !poison)?1:saves[i]});}
  attack(c,draws);check(c.participant()==(poison?1:0) && c.pending()==XeenCombatWork::Round,"Published Poison changes actual selected participant before owed movement");
  check(d.world.sessionState().journeyRandom()->count==12,"Six wake/damage/save pairs publish together");
  c.service(c.ticket());check(c.phase()==XeenCombatPhase::PlayerReady && c.participant()==(poison?1:0),"Selection preserved across owed movement");
  c.command(c.ticket(),XeenCombatCommand::Block);check(c.participant()==(poison?0:1),"Current Speed orders the other awake player");
  c.command(c.ticket(),XeenCombatCommand::Block);check(c.pending()==XeenCombatWork::Round,"Four awake zero-Speed participants excluded");
 }
 auto zero=fresh;auto &snake=zero.journey->actors[12];snake.x=8;snake.y=11;snake.activated=true;
 for(auto id:kXeenCombatOwners){zero.characters[id].currentHp=1000;zero.characters[id].conditions[3]=1;zero.journey->supplements[id].inputs.speed={1,0};}
 Domain d(s,zero);auto &c=attachSnake(d);
 const std::vector<XeenCombatRandom::Draw> misses(6,{1,20,1});attack(c,misses);
 c.service(c.ticket()); // Finish transferred attachment movement first.
 const auto context=d.party.encounterContext;const auto actors=d.world.sessionState().actors();
 for(unsigned cycle=0;cycle<3;++cycle){const auto random=d.world.sessionState().journeyRandom();const auto revision=c.result().revision;
  check(c.service(c.ticket()).status==XeenCombatStatus::Pending && c.pending()==XeenCombatWork::Enemy,"Zero-Speed inner reset yields to automatic enemy service");
  check(random==d.world.sessionState().journeyRandom() && context==d.party.encounterContext && revision==c.result().revision,"Inner reset has no draw, charge or publication");
  attack(c,misses);check(d.world.sessionState().journeyRandom()->count==12+6*cycle,"Bounded next cycle consumes exactly six hit draws");
  for(unsigned i=0;i<19;++i)check(xeen_state::sameActor(actors[i],d.world.sessionState().actors()[i]),"Inner cycle invented movement");
 }
 std::cout<<"REVIEW Poison initiative17->16, zero-Speed exclusion and three bounded automatic cycles PASS\n";
}
void terminalNotices(Source &s) {
 const auto verify=[](Domain &d,const char *status,const char *reason){const auto text=d.flow->notice();check(text.find(status)!=std::string::npos && text.find(reason)!=std::string::npos,"Terminal notice lacks actual status/reason");
  for(const char *control:{"Quiet/approach","Automatic combat","/ End","Arrows move","F Shoot","I inventory","F9 quiet","Space/B"})check(text.find(control)==std::string::npos,"Terminal advertises unavailable controls");
  check(text.find("Esc exits")!=std::string::npos && !d.flow->canSave(),"Terminal exit feedback/save exclusion");};
 Domain original(s);auto saved=original.save();saved.journey->context->minutes=1250;
 {Domain d(s,saved);d.flow->handle(WaitAction{});verify(d,"SUPPORT STOP","time boundary");}
 {Domain d(s);d.flow->fail(d.flow->ticket());verify(d,"FAILED","state/resource or preparation");}
 saved=original.save();auto &a=saved.journey->actors[9];a.x=8;a.y=11;a.activated=true;
 for(auto failure:{XeenCombatFailure::Time,XeenCombatFailure::Integrity,XeenCombatFailure::Preparation,XeenCombatFailure::Observation,XeenCombatFailure::Overflow}){
  Domain d(s,saved);auto &c=attachSnake(d);c.fail(c.ticket(),failure);
  const char *reason=failure==XeenCombatFailure::Time?"time boundary":failure==XeenCombatFailure::Integrity?"state/resource changed":failure==XeenCombatFailure::Preparation?"preparation failed":failure==XeenCombatFailure::Observation?"presentation failed":"numeric limit";
  verify(d,failure==XeenCombatFailure::Time?"SUPPORT STOP":"FAILED",reason);
 }
 std::cout<<"REVIEW derived terminal notices: exploration time; combat time/integrity/preparation/observation/overflow PASS\n";
}
void zeroHitVolley(Source &s) {
 Domain initial(s);auto saved=initial.save();for(unsigned n=0;n<2;++n){auto &a=saved.journey->actors[9-n];a.x=8-n;a.y=11;a.activated=true;}
 Domain d(s,saved);tape={{1,2,1},{1,2,1},{1,2,1},{1,20,19},{1,56,56}};cursor=0;taped=true;zeroResistanceFixture=true;
 d.flow->handle(ShootAction{});d.present();
 for(unsigned i=0;i<20 && d.party.encounterContext->minutes==480;++i){d.now+=100;d.flow->idle();repaint(d);}
 zeroResistanceFixture=false;taped=false;
 check(cursor==5 && d.world.sessionState().journeyRandom()->count==5 && d.party.encounterContext->minutes==490 && d.flow->state().pending()==3,"Zero physical hit spends shooter, skips next identity, charges exactly once");
 check(d.world.sessionState().actors()[9].hp==25 && d.world.sessionState().actors()[8].hp==25,"Zero hit does not wound either target");
 std::cout<<"REVIEW ARTIFICIAL pure-resolver100% resistance operand: production volley spends hit at5 draws, charge then pending3 PASS\n";
}
void shootRevalidation(Source &source,const std::filesystem::path &game) {
 Domain original(source);auto saved=original.save();saved.characters[14].currentHp=1;
 auto &a=saved.journey->actors[9];a.x=5;a.y=11;a.activated=true;
 const auto path=std::filesystem::temp_directory_path()/"mmodern-m33-review-revalidation.mms";
 XeenSaveFile::write(path,saved);
 combat_gameplay_test::Harness h(game);auto services=h.services();services.resources.regionalManifest=source.manifest();
 bool shown=false;
 services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
  shown=true;handler.framePresented(h.flow->frame().presentation());
  const auto press=[&](const PlayerAction &action){handler.beginCycle(++h.cycle);handler.withDisplayedInput(action,*handler.displayedInput());handler.framePresented(h.flow->frame().presentation());};
  press(NavigationAction::MoveForward);
  check(h.flow->encounter()->state().pending()==2,"Revalidation starts with pending old work");
  tape={{0,5,2},{1,20,19},{1,5,5},{1,10,1}};cursor=0;taped=true;
  press(ShootAction{});
  check(h.flow->encounter()->state().pending()==1 && !h.flow->canSave(),"Eligible Shoot retains intent and services first old pulse");
  for(unsigned n=0;n<60 && !h.flow->canSave();++n){h.now+=100;handler.beginCycle(++h.cycle);idle();handler.framePresented(h.flow->frame().presentation());}
  taped=false;
  check(cursor==4 && h.world->sessionState().journeyRandom()->count==4,"Only old enemy target/hit/parameter/damage draws");
  check(!h.party->roster.at(14).canAct() && h.party->encounterContext->minutes==490,"Disabled shooter does not charge Shoot");
  check(h.world->sessionState().actors()[9].x==6 && h.flow->encounter()->state().pending()==0 && h.flow->canSave(),"Old opportunity publishes once; refused intent returns Quiet");
  check(h.flow->encounter()->notice().find("no awake eligible missile user")!=std::string::npos,"Firing revalidation reports lost eligibility");
  return true;
 };
 check(Application().playGameplay(services,{},path,true)==0 && shown,"Production typed/idle Flow revalidation");
 std::filesystem::remove(path);
 std::cout<<"REVIEW typed/idle/EventFlow: old shot disables sole missile user; no volley/charge; Quiet capture restored PASS\n";
}
void playerRayControls(Source &s) {
 Domain original(s);
 // Original terrain, artificial camera/actor arrangements: each obstructed row
 // and a map-edge exit run the actual typed volley through charge publication.
 struct Case {int x,y;XeenDirection facing;int tx,ty;unsigned blocked;};
 for(const auto &v:std::array<Case,4>{{{0,0,XeenDirection::East,1,0,1},{0,1,XeenDirection::East,2,1,2},{1,0,XeenDirection::North,1,3,3},{0,0,XeenDirection::South,8,11,4}}}) {
  auto saved=original.save();saved.camera={23,v.x,v.y,v.facing};auto &actor=saved.journey->actors[9];actor.x=v.tx;actor.y=v.ty;actor.activated=true;
  if(v.blocked==2)saved.journey->actors[18].y=2;
  if(v.blocked==3){saved.journey->actors[17].x=0;saved.journey->actors[18].x=0;}
  // Persistent activation permits this representation fixture's quiet view.
  for(auto &a:saved.journey->actors)a.activated=true;
  Domain d(s,saved);const auto actors=d.world.sessionState().actors();
  std::vector<unsigned> visualRows;
  tape.clear();cursor=0;taped=true;d.flow->handle(ShootAction{});d.present();
  for(unsigned n=0;n<20 && d.party.encounterContext->minutes==480;++n){d.now+=100;d.flow->idle();repaint(d);if(const auto p=d.flow->appearance().projectile){check(!p->enemy&&!p->source&&p->lane==2,"Blocked/edge shooter identity without target");visualRows.push_back(p->row);}}taped=false;
  std::vector<unsigned> expectedRows;for(unsigned row=0;row<(v.blocked<4?v.blocked:1);++row)expectedRows.push_back(row);check(visualRows==expectedRows,"Obstruction/edge visual stops before excluded row");
  check(cursor==0 && d.world.sessionState().journeyRandom()->count==0 && d.party.encounterContext->minutes==490 && d.flow->state().pending()==3,"Blocked/edge volley charges once without hit draws or actor opportunity");
  for(unsigned i=0;i<19;++i)check(xeen_state::sameActor(actors[i],d.world.sessionState().actors()[i]),"Blocked/edge volley changes no actor");
  check(d.flow->notice().find(v.blocked<4?"terrain row "+std::to_string(v.blocked):"empty center rows")!=std::string::npos,"Blocked/edge production outcome");
 }
 // Artificial immutable geometry exercises rare middle 15 through the same
 // production player-ray query; no altered map is admitted to a live Journey.
 auto map=original.world.map(23);constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
 for(unsigned f=0;f<4;++f)for(unsigned row=1;row<4;++row)for(unsigned middle=0;middle<16;++middle){
  XeenCamera camera{23,8,8,static_cast<XeenDirection>(f)};
  for(unsigned n=1;n<4;++n){auto &cell=map.geometry.cells[(8+dy[f]*n)*16+8+dx[f]*n];const auto m=n==row?middle:0;cell.rawWord=m<<4;cell.geometry=XeenOutdoorLayers{0,static_cast<std::uint8_t>(m),0,0};}
  const bool blocked=middle==1||middle==3||middle==6||middle==7||middle==9||middle==10||middle==12;
  check(xeenPlayerRayRows(map,camera)==(blocked?row:4),"All player middles at each forward row/direction");
  if(middle==15){auto actor=original.world.sessionState().actors()[9];actor.x=8+dx[f]*row;actor.y=8+dy[f]*row;
   check(xeenOutdoorRangedRay(map,camera,actor)==(f==1),"Middle15 player admission differs from enemy non-east rays; east retains raw mask rule");}
 }
 for(unsigned f=0;f<4;++f)for(unsigned distance=0;distance<3;++distance){XeenCamera camera{23,f==1?15-int(distance):f==3?int(distance):8,f==0?15-int(distance):f==2?int(distance):8,static_cast<XeenDirection>(f)};
  for(auto &cell:map.geometry.cells)std::get<XeenOutdoorLayers>(cell.geometry).middle=0;
  check(xeenPlayerRayRows(map,camera)==distance+1,"Edge terminates before wrap or fourth forward step");}
 std::cout<<"REVIEW production blocked rows1/2/3 and map edge; ARTIFICIAL pure ray192 middle/direction/row and12 edge controls PASS\n";
}
void reviewControls(Source &s){playerRayControls(s);refusedPendingShoot(s);chargedWait(s);poisonInitiative(s);terminalNotices(s);zeroHitVolley(s);}
