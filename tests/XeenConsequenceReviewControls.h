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
  const auto old=d.flow->ticket();const std::vector<XeenActor> actors=d.world.sessionState().actors();const auto random=d.world.sessionState().journeyRandom();const auto context=d.party.encounterContext;
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
 const auto context=d.party.encounterContext;const std::vector<XeenActor> actors=d.world.sessionState().actors();
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
  Domain d(s,saved);const std::vector<XeenActor> actors=d.world.sessionState().actors();
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
  for(auto &cell:map.geometry.cells)xeenGet<XeenOutdoorLayers>(cell.geometry).middle=0;
  check(xeenPlayerRayRows(map,camera)==distance+1,"Edge terminates before wrap or fourth forward step");}
 std::cout<<"REVIEW production blocked rows1/2/3 and map edge; ARTIFICIAL pure ray192 middle/direction/row and12 edge controls PASS\n";
}
// M34 fixtures below deliberately alter saved representations for rare authority
// boundaries. They never stand in for the genuine production Run witnesses.
XeenSaveSnapshot runAuthorityFixture(Source &s,bool occupied=false) {
 Domain initial(s);auto saved=initial.save();saved.journey->schema=saved.journey->contract=5;
 for(unsigned slot=0;slot<6;++slot) {
  const auto id=kXeenCombatOwners[slot];saved.journey->supplements[id].inputs.speed={slot?1:255,0};
  if(slot) {saved.characters[id].currentHp=0;saved.characters[id].conditions[12]=1;}
 }
 saved.characters[kXeenCombatOwners[2]].conditions[13]=7;
 auto &a=saved.journey->actors[9];a.x=occupied?10:8;a.y=occupied?12:11;a.hp=16;a.activated=true;
 if(occupied)saved.camera={23,10,11,XeenDirection::North};
 auto &dead=saved.journey->actors[3];dead.x=dead.y=-128;dead.hp=0;dead.activated=false;dead.lifecycle=XeenActorLifecycle::Defeated;dead.accounted=true;
 auto &treasure=*saved.journey->treasure;treasure.pendingMask=8;treasure.pendingGold=10;treasure.armor[0]={3,{0,2,0,0}};
 return saved;
}
XeenCombat &runAuthorityReady(Domain &d,bool *automatic=nullptr) {
 const auto movement=d.flow->journeyAction(d.flow->ticket(),XeenEncounterAction::Forward);
 if(automatic)*automatic=movement.automaticEvent;
 if(d.flow->state().phase()==XeenEncounterPhase::Exploring)d.flow->journeyPulse(d.flow->ticket());
 check(d.flow->attachJourney(d.flow->ticket(),[]{}),"Artificial Run authority attachment");
 auto &c=*d.flow->combat();unsigned n=0;
 while(c.phase()!=XeenCombatPhase::PlayerReady){check(++n<30,"Run authority fixture reaches ready");const auto r=c.service(c.ticket());check(r.status!=XeenCombatStatus::Failed,"Run authority fixture preparation");}
 check(c.participant()==0,"Artificial sole awake fast owner acts first");return c;
}
XeenSaveSnapshot runAuthorityValues(const Domain &d,const XeenSaveSnapshot &base) {
 auto value=base;value.camera=d.camera;value.characters=d.party.roster.characters();
 value.journey->context=d.party.encounterContext;value.journey->treasure=d.party.monsterTreasure;value.journey->random=d.world.sessionState().journeyRandom();
 for(unsigned i=0;i<30;++i)value.journey->supplements[i].inputs=*d.party.roster.combatInputs(i);
 for(unsigned i=0;i<19;++i){const auto &a=d.world.sessionState().actors()[i];auto &b=value.journey->actors[i];b.x=a.x;b.y=a.y;b.hp=a.hp;b.activated=a.activated;b.lifecycle=a.lifecycle;b.status=a.status;}
 return value;
}
XeenCombatResult runAuthorityPublish(XeenCombat &c) {
 const auto accepted=c.command(c.ticket(),XeenCombatCommand::Run);
 if(accepted.status==XeenCombatStatus::Failed)return accepted;
 check(accepted.status==XeenCombatStatus::Pending,"Run intent has no immediate publication");
 tape={{1,100,1}};cursor=0;taped=true;const auto result=c.service(c.ticket());taped=false;
 return result;
}
void runNoticeFits(Source &s,const std::string &notice) {
 const XeenFontFormat font(s.assets.readArchiveResource("fnt"));
 IndexedFrame base(320,200,std::vector<std::uint8_t>(320*200));
 const auto split=notice.find("\n\n");check(split!=std::string::npos,"Consequence notice retains explicit roster boundary");
 XeenTextRenderOptions options;options.bounds={3,107,229,200};options.windowBounds={1,105,231,200};options.x=3;options.y=107;
 options.size=XeenFontSize::Reduced;options.paginate=true;options.drawWindow=true;
 auto rendered=XeenTextRenderer(font).render(base,notice.substr(0,split),options);
 if(rendered.pages.size()!=1)throw std::runtime_error("M34 original-font notice overflow: "+notice.substr(0,split));
 options.bounds={235,3,318,198};options.windowBounds={233,1,320,200};options.x=235;options.y=3;
 rendered=XeenTextRenderer(font).render(rendered.pages.front(),notice.substr(split+2),options);
 if(rendered.pages.size()!=1)throw std::runtime_error("M34 original-font roster overflow: "+notice.substr(split+2));
}
void runSignSupersession(Source &s) {
 auto setup=runAuthorityFixture(s);setup.camera={23,5,8,XeenDirection::North};
 for(auto &a:setup.journey->actors)if(a.lifecycle==XeenActorLifecycle::Present)a.activated=true;
 setup.journey->actors[9].x=5;setup.journey->actors[9].y=9;
 Domain d(s,setup);bool automatic=false;auto &c=runAuthorityReady(d,&automatic);
 check(automatic,"Original sign contact retains automatic dispatch obligation");
 runAuthorityPublish(c);const auto finish=c.service(c.ticket());
 check(finish.phase==XeenCombatPhase::Disengaged && finish.originAutomaticSuperseded && finish.origin &&
  finish.origin->x==5 && finish.origin->y==9 && finish.origin->direction==XeenDirection::North,"Finish explicitly supersedes original sign address");
 check(d.flow->retireJourney(d.flow->ticket()),"Sign-origin escape retires");d.present();
 check(d.flow->canSave() && d.world.sessionState().journeyActivity()!=XeenJourneyActivity::Event,"Old sign obligation cannot dispatch at relocated destination");
 check(d.flow->journeyAction(d.flow->ticket(),XeenEncounterAction::Right).outcome==XeenEncounterOutcome::Accepted,"Fresh navigation is responsive after sign supersession");d.present();
 std::cout<<"ARTIFICIAL original sign contact -> Run -> explicit origin-address supersession -> fresh navigation PASS\n";
}
void runAuthorityControls(Source &s) {
 const auto fixture=runAuthorityFixture(s);
 {
  // Original decoded EVT is immutable once attached. Corrupt a detached source
  // before restore: the manifest must reject the new destination event before
  // any candidate owner can be published.
  const auto events=s.evt;s.evt.records.at(56).x=10;s.evt.records.at(56).y=12;
  bool refused=false;try{Domain rejected(s,fixture);}catch(const std::exception &){refused=true;}s.evt=events;
  check(refused,"Changed original destination EVT rejects unpublished restoration");
 }
 unsigned probes=0;
 {Domain d(s,fixture);auto &c=runAuthorityReady(d);c.setProbe([&]{++probes;});const auto result=runAuthorityPublish(c);
  check(result.status==XeenCombatStatus::Advanced && result.runSuccess && c.participants()==0x3e && c.pending()==XeenCombatWork::FinishDisengagement,"Sole runner exhausts transient participation");}
 check(probes>=3,"Run has observable prepublication probes");
 for(unsigned failAt=1;failAt<=probes;++failAt) {
  Domain d(s,fixture);auto &c=runAuthorityReady(d);const auto before=runAuthorityValues(d,fixture);unsigned count=0;
  c.setProbe([&]{if(++count==failAt)throw std::bad_alloc();});const auto result=runAuthorityPublish(c);
  check(count==failAt && result.status==XeenCombatStatus::Failed && c.participants()==0x3f,"Every Run probe fails before mask/RNG publication");sameLive(d,before);
  check(c.service(c.ticket()).status==XeenCombatStatus::Refused && !d.flow->canSave(),"Failed Run cannot replay or save");
 }
 for(unsigned facing=0;facing<4;++facing) {
  auto setup=fixture;setup.camera.direction=static_cast<XeenDirection>(facing);
  for(auto &a:setup.journey->actors)if(a.lifecycle==XeenActorLifecycle::Present)a.activated=true;
  setup.characters[0].currentHp=1000;
  constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
  setup.journey->actors[9].x=setup.camera.x+dx[facing];setup.journey->actors[9].y=setup.camera.y+dy[facing];
  Domain d(s,setup);auto &c=runAuthorityReady(d);runAuthorityPublish(c);const auto finish=c.service(c.ticket());
  check(finish.phase==XeenCombatPhase::Disengaged && finish.destination && finish.destination->x==10 && finish.destination->y==12 &&
   finish.destination->direction==static_cast<XeenDirection>(facing) && d.camera.direction==setup.camera.direction,"All four facings survive original fixed relocation");
 }
 unsigned finishProbes=0;
 {Domain d(s,fixture);auto &c=runAuthorityReady(d);runAuthorityPublish(c);c.setProbe([&]{++finishProbes;});check(c.service(c.ticket()).status==XeenCombatStatus::Advanced,"Finish probe census");}
 check(finishProbes>=2,"Finish preparation probes observed");
 for(unsigned failAt=1;failAt<=finishProbes;++failAt) {
  Domain d(s,fixture);auto &c=runAuthorityReady(d);runAuthorityPublish(c);const auto before=runAuthorityValues(d,fixture);unsigned count=0;
  c.setProbe([&]{if(++count==failAt)throw std::bad_alloc();});check(c.service(c.ticket()).status==XeenCombatStatus::Failed,"Every finish probe fails closed");
  sameLive(d,before);check(d.camera.x==before.camera.x && d.camera.y==before.camera.y && c.participants()==0x3e && !d.flow->canSave(),"Failed finish retains published escape without relocation/refund");
 }
 for(unsigned phase=0;phase<2;++phase) {
  Domain d(s,fixture);auto &c=runAuthorityReady(d);if(phase)runAuthorityPublish(c);
  unsigned calls=0;c.setProbe([&]{++calls;check(c.service(c.ticket()).status==XeenCombatStatus::Refused,"Reentrant service cannot publish Run/finish");check(c.command(c.ticket(),XeenCombatCommand::Run).status==XeenCombatStatus::Refused,"Reentrant Run cannot consume another member");});
  const auto result=phase?c.service(c.ticket()):runAuthorityPublish(c);
  check(result.status==XeenCombatStatus::Advanced && calls>=2,"Outer publication survives refused reentrance");
 }
 for(unsigned field=0;field<5;++field) {
  Domain d(s,fixture);auto &c=runAuthorityReady(d);runAuthorityPublish(c);const auto before=runAuthorityValues(d,fixture);
  auto &g=const_cast<XeenMap &>(d.world.map(23)).geometry;const auto originalGeometry=g;
  if(field==0)g.runX^=1;if(field==1)g.difficulties[7]^=1;if(field==2)g.cells[12*16+10].rawAttributes^=1;
  if(field==3)d.world.discardMapCache();if(field==4)g.id=24;
  const auto result=c.service(c.ticket());
  if(field!=3){check(result.status==XeenCombatStatus::Failed && !d.flow->canSave(),"Changed Run metadata/destination/identity latches failure");sameLive(d,before);
   g=originalGeometry;check(c.service(c.ticket()).status==XeenCombatStatus::Refused && !d.flow->canSave(),"Restored equal resource bytes cannot revive failed finish");}
  else check(result.status==XeenCombatStatus::Advanced && c.phase()==XeenCombatPhase::Disengaged,"Unchanged cache reconstruction preserves finish authority");
 }
 {
  Domain d(s,fixture);auto &c=runAuthorityReady(d);runAuthorityPublish(c);c.service(c.ticket());const auto after=runAuthorityValues(d,fixture);
  auto &map=const_cast<XeenMap &>(d.world.map(23));const auto originalMap=map;map.geometry.runY^=1;
  bool retired=false;try{retired=d.flow->retireJourney(d.flow->ticket());}catch(const std::exception &){}
  check(!retired,"Changed retained map refuses retirement resource renewal");map=originalMap;
  try{retired=d.flow->retireJourney(d.flow->ticket());}catch(const std::exception &){}
  check(!retired && !d.flow->canSave(),"Retirement resource ABA cannot renew a failed guard");sameLive(d,after);
 }
 for(bool occupied:{false,true}) {
  const auto setup=runAuthorityFixture(s,occupied);Domain d(s,setup);auto &c=runAuthorityReady(d);
  const auto old=c.ticket();runAuthorityPublish(c);const auto before=runAuthorityValues(d,setup);
  check(c.service(old).status==XeenCombatStatus::Stale,"Stale pre-Run service cannot finish");sameLive(d,before);
  const auto result=c.service(c.ticket());check(result.status==XeenCombatStatus::Advanced && c.phase()==XeenCombatPhase::Disengaged,"Successful nonvictory finish");
  check(result.forfeitedGold==10 && d.party.monsterTreasure->dormant() && d.party.roster.at(kXeenCombatOwners[2]).conditions[13]==7,"Finish retains dormant items and prior Dead counter");
  check(d.camera.x==10 && d.camera.y==12 && d.camera.direction==setup.camera.direction,"Fixed destination preserves facing");
  const auto after=runAuthorityValues(d,setup);c.preparePresentation(c.ticket(),[]{});
  bool failed=false;try{c.preparePresentation(c.ticket(),[]{throw std::bad_alloc();});}catch(const std::bad_alloc &){failed=true;}
  check(failed,"Post-finish resource presentation failure injected");sameLive(d,after);
  c.preparePresentation(c.ticket(),[]{});check(d.flow->retireJourney(d.flow->ticket()),"Successful finish retires through Flow");
  check(!d.flow->combat() && !d.flow->canSave(),"Retirement has no intermediate saveable frame");
  auto notice=d.flow->notice();runNoticeFits(s,notice);
  if(!occupied){
   d.present();check(d.flow->handle(ShootAction{}),"Fresh no-missile Shoot produces readable refusal");
   check(!d.flow->journeyRefusal().empty(),"Quiet post-finish refusal is observable");
   runNoticeFits(s,d.flow->notice());
   d.flow->holdJourneyFrame();d.present();
   check(d.flow->journeyAction(d.flow->ticket(),XeenEncounterAction::Right).outcome==XeenEncounterOutcome::Accepted,"Fresh action supersedes finish notice");
   runNoticeFits(s,d.flow->notice());check(d.flow->notice().find("Disengaged;")==std::string::npos,"Consumed finish notice does not stack with new action feedback");
   check(d.flow->journeyInspection().find("Disengaged cause=")!=std::string::npos,"Read-only inspection retains full finish observation after its notice expires");
  }

  if(occupied){check(d.world.sessionState().journeyActivity()==XeenJourneyActivity::Attachment && d.flow->state().phase()==XeenEncounterPhase::Engaged,"Occupied same-origin destination requires immediate attachment");
   check(d.flow->attachJourney(d.flow->ticket(),[]{}),"New occupied-destination incarnation attaches");check(d.flow->combat()->participants()==0x3f && d.world.sessionState().actors()[9].hp==16,"New incarnation reintegrates owners without regenerating survivor");}
 }
 std::cout<<"ARTIFICIAL M34 Run/finish probe failures, reentrance, stale tickets, metadata/cache and occupied retirement controls PASS\n";
}
void reviewControls(Source &s){runSignSupersession(s);runAuthorityControls(s);playerRayControls(s);refusedPendingShoot(s);chargedWait(s);poisonInitiative(s);terminalNotices(s);zeroHitVolley(s);}
