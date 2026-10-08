// Included by the existing original-resource consequence harness.
XeenJourneySetup freshSetup(Source &s,XeenDifficulty difficulty) {
 XeenJourneySetup setup{s.chr,XeenGameplayContextFormat::parse(s.pty),s.statistics,s.city,1,s.regional()};
 setup.prepared=false;setup.context.difficulty=difficulty;
 setup.purse=XeenCharacterFormat::parseMonsterPurse(s.pty);setup.bank=XeenCharacterFormat::parseBankBalances(s.pty);
 setup.regionalRecovery=XeenQuestFlagFormat::parseRegionalRecovery(s.pty);setup.regionalText=s.texts.load(23);
 setup.learnedNames=s.names;setup.learnedNamesProvider=[&s]{return s.names;};
 setup.cityEventsProvider=[&s]{return s.city;};setup.mainlandEventsProvider=[&s]{return s.mainland;};return setup;
}
struct FreshDomain {
 Source &source;XeenWorld world;XeenPartyState party;XeenCamera camera;XeenGameFlags flags;
 std::uint64_t now=0;
 XeenEventPresenter::Clock clock=[this]{return now;};
 std::unique_ptr<XeenEncounterFlow> flow;
 FreshDomain(Source &s,XeenDifficulty difficulty):source(s),world(s.mapLoader(),s.objectLoader()),
  party(XeenPartyLoader().loadFromResources(s.chr,s.pty)),camera(XeenCharacterFormat::parsePartyLocation(s.pty)) {
  auto setup=freshSetup(s,difficulty);
  flow=std::make_unique<XeenEncounterFlow>(world,party,camera,flags,clock,setup);
  present();
 }
 void present(){check(flow->prepareJourneyFrame(flow->ticket(),[]{}) && flow->presentJourney(flow->ticket()),"Fresh original presentation");}
};
void freshState(Source &s,const XeenPartyState &p,const XeenCamera &c,const XeenGameFlags &f,XeenWorld &w,XeenDifficulty difficulty) {
 initial_oracle::roster(p,s.chr,true);
 check(xeen_state::sameCamera(c,{28,18,4,XeenDirection::West}),"Original PTY camera");
 const auto context=XeenGameplayContextFormat::parse(s.pty);auto expected=context;expected.difficulty=difficulty;
 check(p.encounterContext==expected && p.food==90 && p.monsterTreasure->gold==800 && p.monsterTreasure->gems==10 &&
  !p.monsterTreasure->pendingGold && !p.monsterTreasure->pendingMask,"Original calendar/food/purse");
 check(p.serviceEconomy && p.serviceEconomy->bank.gold==0 && p.serviceEconomy->bank.gems==0,"Original bank without interest");
 // M40's pinned controlled-seed stock oracle, covering both sides/four shops.
 check(m40_test::sha256(m40_test::stockBytes(*p.serviceEconomy))=="bd3799d8b453a2877656c87d4add0d513914ca7679afac20e9b87dae6e031d84" &&
  w.sessionState().journeyRandom()==XeenJourneyRandomState{1,2477276124u,878},"First gameplay draws generate original stock exactly once");
 check(w.sessionState().disabledEventCount()==0 && w.sessionState().disabledObjectCount()==0 &&
  w.sessionState().accountedMonsters().empty() && w.sessionState().barriers().empty(),"Fresh resource overlays");
 for(bool flag:f.values())check(!flag,"Fresh Clouds game flags");
 for(bool flag:p.questFlags.values())check(!flag,"Fresh quest flags");
 for(auto item:p.questItems.counts())check(!item,"Fresh Clouds quest items");
 check(p.regionalRecovery && !p.regionalRecovery->worldFlag16,"Fresh world flag default");
 const auto originalCity=XeenActorApproach::actorsFromResources(s.maps.loadObjects(s.assets,28),s.statistics);
 const auto city=w.sessionState().regionalActors(28);
 check(city.size()==46,"All original city actors published");
 for(unsigned n=0;n<46;++n) {
  auto expectedActor=originalCity[n];expectedActor.activated=n==35;
  check(xeen_state::sameActor(city[n],expectedActor),"Every fresh city record, HP and activation");
 }
 const auto mainland=XeenActorApproach::actorsFromResources(s.maps.loadObjects(s.assets,23),s.statistics);
 check(w.sessionState().actors().size()==19,"All mainland records staged");
 for(unsigned n=0;n<19;++n)check(xeen_state::sameActor(w.sessionState().actors()[n],mainland[n]),"Mainland staged without activation or wounds");
 initial_oracle::view(XeenIndoorScene().classifyActors(w,c,city));
 check(!xeenRegionalEvent(s.city,c),"No original West/all-direction arrival Event");
 const auto east=xeenRegionalEvent(s.city,{28,18,4,XeenDirection::East});
 check(east && s.city.records[*east].opcode==2,"Original East DoorTextSml Event");
}
void freshSaveControls(Source &source) {
 initial_oracle::pty(source.pty);
 for(auto difficulty:{XeenDifficulty::Adventurer,XeenDifficulty::Warrior}) {
  const auto path=std::filesystem::temp_directory_path()/"mmodern-m52-initial.mms";
  combat_gameplay_test::Harness h;auto services=disengagementServices(source,h);
  services.originalStart=difficulty;services.loadInitialCamera=[&]{return XeenCharacterFormat::parsePartyLocation(source.pty);};
  services.sampleJourneySeed=[]{return 1;};
  std::vector<std::uint8_t> before;bool shown=false;
  services.show=[&](const auto &first,const auto &handler,const auto &,const auto &,const auto &) {
   check(first.isValid(),"Fresh first frame");handler.framePresented(h.flow->frame().presentation());
   check(h.flow->canSave(),"Immediate original first frame is saveable");
   freshState(source,*h.party,*h.camera,*h.flags,*h.world,difficulty);
   before=XeenSaveFormat::encode(XeenSaveState::capture(source.signature,*h.party,*h.camera,*h.flags,*h.world));
   handler.beginCycle(++h.cycle);handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());
   check(XeenSaveFormat::encode(XeenSaveFile::read(path))==before,"Immediate F9 without move or pulse");
   freshState(source,*h.party,*h.camera,*h.flags,*h.world,difficulty);shown=true;return true;
  };
  check(Application().playGameplay(services,{},path,false,XeenEncounterEntry::Journey)==0 && shown,"Fresh application hook");
  services.originalStart.reset();shown=false;
  services.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &) {
   handler.framePresented(h.flow->frame().presentation());
   freshState(source,*h.party,*h.camera,*h.flags,*h.world,difficulty);
   handler.beginCycle(++h.cycle);handler.withDisplayedInput(SaveGameAction{},*handler.displayedInput());
   check(XeenSaveFormat::encode(XeenSaveFile::read(path))==before,"Restore/re-save retains every v6 byte");shown=true;return true;
  };
  check(Application().playGameplay(services,{},path,true)==0 && shown,"Immediate saved original resume");
  std::filesystem::remove(path);
 }
}
void freshFailureControls(Source &s) {
 for(unsigned fault=0;fault<5;++fault) {
  auto p=XeenPartyLoader().loadFromResources(s.chr,s.pty);const auto before=p.roster.characters();
  auto c=XeenCharacterFormat::parsePartyLocation(s.pty);XeenGameFlags f;
  XeenWorld w([&](auto id){
   if(fault==4){++p.roster.at(29).currentHp;--p.roster.at(29).currentHp;}
   return s.maps.loadGeometryMap(s.assets,id);
  },[&](auto id){auto mob=s.maps.loadObjects(s.assets,id);if(fault==0 && id==XeenMapIdentity(28))mob.entities.monsters.pop_back();return mob;});
  auto setup=freshSetup(s,XeenDifficulty::Warrior);
  setup.mainlandEventsProvider=[&]{auto e=s.mainland;if(fault==1)e.mapId=28;return e;};
  if(fault==2)setup.cityEventsProvider=[&]{auto e=s.city;e.records[0].opcode^=1;return e;};
  if(fault==3)setup.cityEventsProvider=[&]()->XeenEventFile{throw std::bad_alloc();};
  bool failed=false;try{XeenEncounterFlow flow(w,p,c,f,[]{return 0;},setup);}catch(const std::exception &){failed=true;}
  check(failed && !w.sessionState().encounterInitialized() && w.sessionState().actors().empty() &&
   !w.sessionState().hasRegionalActors(28) && !p.roster.combatMarked() && !p.encounterContext && !p.serviceEconomy && !p.monsterTreasure && !p.regionalRecovery,"Failed fresh preparation publishes no gameplay candidate");
  for(unsigned n=0;n<30;++n)check(xeen_state::sameCharacter(before[n],p.roster.at(n)),"Failed fresh preparation retains original CHR");
 }
}
void freshDifficultyHits(Source &s) {
 for(auto difficulty:{XeenDifficulty::Adventurer,XeenDifficulty::Warrior}) {
  // Real initial corridor and bow; only random outcomes are controlled.
  FreshDomain d(s,difficulty);const auto hp=d.world.sessionState().regionalActors(28)[35].hp;
  tape={{1,2,1},{1,2,1},{1,2,1},{1,20,19},{1,50,50}};cursor=0;taped=true;
  check(d.flow->handle(ShootAction{}),"Original fresh Shoot accepted");d.present();
  for(unsigned n=0;n<50 && d.party.encounterContext->minutes==480;++n) {
   d.now+=100;d.flow->idle();d.flow->holdJourneyFrame();d.present();
  }
  taped=false;
  const int damage=difficulty==XeenDifficulty::Adventurer?9:3;
  check(cursor==5 && d.world.sessionState().regionalActors(28)[35].hp==std::max(hp-damage,0) &&
   d.flow->notice().find("Shoot owner 14 -> actor 35 hit "+std::to_string(damage))!=std::string::npos,"Fresh original Shoot difficulty damage and draws");
  // A separate fresh game walks into the same original Slime without teleport.
  FreshDomain melee(s,difficulty);
  for(unsigned n=0;n<6 && melee.flow->state().phase()==XeenEncounterPhase::Exploring;++n) {
   melee.flow->journeyAction(melee.flow->ticket(),XeenEncounterAction::Forward);
   if(melee.world.sessionState().journeyActivity()==XeenJourneyActivity::Presentation)melee.present();
   if(melee.flow->state().phase()==XeenEncounterPhase::Exploring) {
    melee.flow->journeyPulse(melee.flow->ticket());
    if(melee.world.sessionState().journeyActivity()==XeenJourneyActivity::Presentation)melee.present();
   }
  }
  check(melee.flow->attachJourney(melee.flow->ticket(),[]{}),"Original corridor contact attaches melee");
  auto &combat=*melee.flow->combat();
  for(unsigned n=0;n<50 && combat.phase()!=XeenCombatPhase::PlayerReady;++n)combat.service(combat.ticket());
  check(combat.phase()==XeenCombatPhase::PlayerReady,"Fresh original player turn");
  const auto owner=kXeenCombatOwners[combat.participant()];const auto &c=melee.party.roster.at(owner);
  const auto &input=*melee.party.roster.combatInputs(owner);
  constexpr unsigned counts[]{3,2,3,2,2,4,1,2,4,2,3,2,2,1,1,1,1,4,4,3,2,4,2,2,2,5,3,3,3,3,5,4,2,6};
  constexpr unsigned sides[]{3,3,4,5,4,2,3,3,3,3,3,2,4,10,6,8,9,4,3,6,8,5,6,4,5,3,5,6,7,2,2,2,2,4};
  std::vector<XeenCombatRandom::Draw> draws;unsigned weapon=0;
  for(const auto &item:c.weapons)if(item.frame==1 || item.frame==13)for(unsigned n=0;n<counts[item.id-1];++n){draws.push_back({1,sides[item.id-1],1});++weapon;}
  draws.push_back({1,20,19});
  const int might=XeenCharacterRules::physicalBonus(XeenCharacterRules::effectivePhysical(c,input,XeenCharacterRules::PhysicalAttribute::Might,{610}));
  const int expected=std::max(int(weapon)*(difficulty==XeenDifficulty::Adventurer?3:1)+might,1);
  check(combat.command(combat.ticket(),XeenCombatCommand::Attack).status==XeenCombatStatus::Pending,"Fresh real melee command");
  const auto count=draws.size();attack(combat,draws);
  check(cursor==count && combat.result().damage==expected,"Fresh original melee difficulty damage and unchanged draws");
 }
 std::cout<<"M52 both fresh difficulties: immediate F9/v6 restore, complete owners, failures, real melee and Shoot PASS\n";
}
