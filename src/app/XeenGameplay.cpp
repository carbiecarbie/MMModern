#include "app/Application.h"
#include "app/XeenAudio.h"
#include "app/XeenGameplayServices.h"
#include "app/XeenTitleFlow.h"
#include "platform/XeenSaveFile.h"
#include <iostream>
#include <stdexcept>
#include <exception>
#include <random>
#include <chrono>
namespace mmodern {
namespace {
std::uint32_t committedSaveMark(const XeenGameplayServices &services,XeenEventFlow &flow) {
 try {return services.program.wallClock.now();}
 catch(const std::exception &e) {
  const auto message=std::string("Game saved, but wall clock reset failed after publication: ")+e.what()+
   ". Closing the gameplay session.";
  std::cerr<<message<<'\n';flow.closeGameplay();throw std::runtime_error(message);
 }
}
struct GameplayScope {
 bool &busy;
 explicit GameplayScope(bool &value) : busy(value) { busy = true; }
 ~GameplayScope() { busy = false; }
};
// Retain the authorization of the frame handed to the window. A later failed
// callback cannot recapture newer domain authority to legitimize its failure.
struct EncounterHandoff {
 XeenEventFlow &flow;
 std::optional<XeenEncounterFlow::Ticket> ticket;
 int exceptions = std::uncaught_exceptions();
 explicit EncounterHandoff(XeenEventFlow &f) : flow(f) { retain(); }
 void retain() {
  if (!flow.encounterFrameCurrent()) throw std::runtime_error("Stale encounter handoff");
  if (flow.encounter()) ticket = flow.encounter()->ticket();
 }
 void verify() const {
  if (ticket && (!flow.encounter()->current(*ticket) || !flow.encounterFrameCurrent()))
   throw std::runtime_error("Stale encounter startup callback");
 }
 void fail() noexcept { if (ticket) flow.failEncounterHandoff(*ticket); }
 ~EncounterHandoff() { if (std::uncaught_exceptions() > exceptions) fail(); }
};
}
void xeenSaveGameplay(const XeenGameplayServices &services, XeenWorld &world, XeenPartyState &party,
	XeenCamera &camera, XeenGameFlags &flags, XeenEventFlow &flow, const std::filesystem::path &target,
	const XeenSaveState::Preflight &preflight, std::function<void()> *nestedSourceCheck,
 const std::function<void(const XeenSaveSnapshot &,const std::function<void()> &)> &writer) {
	if (!flow.canSave() || !XeenSaveState::canCapture(party,camera,world))
		throw std::logic_error("Save boundary is unavailable");
	auto snapshot = XeenSaveState::capture(services.resources.signature,party,camera,flags,world);
	snapshot.name=services.saveName;
	XeenRestoreGuard before(world,party,camera,flags);
	before.check();
    snapshot.journey->playTimeSeconds=services.program.playTime(party.playTimeSeconds);before.check();
	const auto boundary = flow.beginSave();
	struct Lease {
		XeenEventFlow &flow; XeenEventFlow::SaveBoundary boundary;
		~Lease() { try { flow.endSave(boundary); } catch (...) { flow.closeGameplay(); } }
	} lease{flow,boundary};
	// Journey Flow retained this preimage before capture and admit
	// only its own explicit lease transition. Ordinary beginSave changes no owner.
	const auto heldPreimage = flow.encounter() ? flow.encounter()->retainSavePreimage() : nullptr;
	auto &retained = heldPreimage ? *heldPreimage : before;
	const auto check = [&] { retained.check(); if (!flow.saveCurrent(boundary)) throw std::logic_error("Save UI authorization changed"); };
	XeenRestoreGuard::Providers providers(retained,world,check);
	struct NestedSource {
		std::function<void()> *slot;
		std::function<void()> previous;
		~NestedSource() { if (slot) slot->swap(previous); }
	} nested{nestedSourceCheck,check};
	if (nested.slot) nested.slot->swap(nested.previous);
	const auto callback = [&](auto &&provider) {
		check();
		try { auto value = provider(); check(); const auto detached = value; return detached; }
		catch (...) { check(); throw; }
	};
	auto resources = services.resources;
	if (resources.loadInitialParty) resources.loadInitialParty = [&] { return callback(services.resources.loadInitialParty); };
	if (resources.loadInitialCharacters) resources.loadInitialCharacters = [&] { return callback(services.resources.loadInitialCharacters); };
	if (resources.loadInitialContext) resources.loadInitialContext = [&] { return callback(services.resources.loadInitialContext); };
	if (resources.loadMonsterStatistics) resources.loadMonsterStatistics = [&] { return callback(services.resources.loadMonsterStatistics); };
	if (resources.loadEvents) resources.loadEvents = [&](XeenMapIdentity id) { return callback([&] { return services.resources.loadEvents(id); }); };
	if (resources.loadRegionalText) resources.loadRegionalText = [&](XeenMapIdentity id) { return callback([&] { return services.resources.loadRegionalText(id); }); };
	const auto stage = [&](XeenGameplayServices::SaveStage stage) {
		check(); try { if (services.observeSaveStage) services.observeSaveStage(stage); }
		catch (...) { check(); throw; } check();
	};
	stage(XeenGameplayServices::SaveStage::Capture);
	XeenWorld candidate([&](XeenMapIdentity id) { return callback([&] { return services.maps(id); }); },
		[&](XeenMapIdentity id) { return callback([&] { return services.objects(id); }); });
	XeenPartyState p; XeenCamera c; XeenGameFlags f;
	stage(XeenGameplayServices::SaveStage::Preflight);
	XeenSaveState::restoreBeforeGameplay(snapshot,resources,p,c,f,candidate,[&](auto &w,const auto &p,const auto &c,const auto &f) {
		check(); try { preflight(w,p,c,f); } catch (...) { check(); throw; } check();
	});
	stage(XeenGameplayServices::SaveStage::Write);
	check(); if(writer)writer(snapshot,check);else XeenSaveFile::write(target,snapshot);
 check();
 // Publish only after the file committed. The next interval starts after I/O.
 const auto mark=committedSaveMark(services,flow);check();
 flow.commitSavedPlayTime(boundary,snapshot.journey->playTimeSeconds);
 services.program.playTimeMark=mark;
}

int Application::journeyRegion(const std::filesystem::path &directory, std::optional<std::uint32_t> seed,
  std::optional<std::filesystem::path> save) const {
 return gameplay(directory,xeenJourneyContent().entry,save,false,XeenEncounterEntry::Journey,seed);
}
int Application::playGameplay(const XeenGameplayServices &supplied, XeenCamera camera,
  const std::optional<std::filesystem::path> &target, bool resume, XeenEncounterEntry entry, std::optional<std::uint32_t> seed) const {
 bool exposed=false;
 try {
  XeenGameplayServices services = supplied;
  if(!services.clock) {
   services.clock=[] {return SdlWindow::presentationNanoseconds()/1000000;};
   if(!services.presentationClock)services.presentationClock=[] {return SdlWindow::presentationNanoseconds();};
  }
  std::function<void()> sourceCheck;
  const auto callback = [&](auto &&provider) {
   if (sourceCheck) sourceCheck();
   try { auto value = provider(); if (sourceCheck) sourceCheck(); return value; }
   catch (...) { if (sourceCheck) sourceCheck(); throw; }
  };
  services.maps = [&](XeenMapIdentity id) { return callback([&] { return supplied.maps(id); }); };
  services.objects = [&](XeenMapIdentity id) { return callback([&] { return supplied.objects(id); }); };
  services.resources.loadInitialParty = [&] { return callback(supplied.resources.loadInitialParty); };
  services.resources.loadEvents = [&](XeenMapIdentity id) { return callback([&] { return supplied.resources.loadEvents(id); }); };
  if (supplied.resources.loadInitialCharacters) services.resources.loadInitialCharacters = [&] { return callback(supplied.resources.loadInitialCharacters); };
  if (supplied.resources.loadInitialContext) services.resources.loadInitialContext = [&] { return callback(supplied.resources.loadInitialContext); };
  if (supplied.resources.loadMonsterStatistics) services.resources.loadMonsterStatistics = [&] { return callback(supplied.resources.loadMonsterStatistics); };
  if (supplied.resources.loadInitialPurse) services.resources.loadInitialPurse = [&] { return callback(supplied.resources.loadInitialPurse); };
  if (supplied.resources.loadInitialBankBalances) services.resources.loadInitialBankBalances = [&] { return callback(supplied.resources.loadInitialBankBalances); };
  if (supplied.resources.loadInitialWorldFlags) services.resources.loadInitialWorldFlags = [&] { return callback(supplied.resources.loadInitialWorldFlags); };
  if (supplied.texts) services.resources.loadRegionalText = [&](XeenMapIdentity id) { return callback([&] { return supplied.texts(id); }); };
  services.compose = [&](auto &w, const auto &p, const auto &c, auto phase) { return callback([&] { return supplied.compose(w,p,c,phase); }); };
  if (supplied.composeEncounter) services.composeEncounter = [&](auto &w, const auto &p, const auto &c, auto phase, auto actor) {
   return callback([&] { return supplied.composeEncounter(w,p,c,phase,actor); });
  };
  bool encounter = entry != XeenEncounterEntry::Ordinary;
  const bool original=services.originalStart.has_value();
  if((services.restoreSnapshot && !resume) || (services.publishInitial && (!original || resume)))
   throw std::invalid_argument("Invalid title session configuration");
  if(original && (resume || entry!=XeenEncounterEntry::Journey || seed ||
      static_cast<unsigned>(*services.originalStart)>1 || !services.loadInitialCamera))
   throw std::invalid_argument("Invalid original start configuration");
  if (encounter && resume) throw std::invalid_argument("Encounter entry cannot resume");
  if (services.eventFixture && (resume || original || !encounter || target || services.journeyStart ||
      !services.saveUnavailable || services.publishInitial || services.restoreSnapshot))
   throw std::invalid_argument("Event developer sessions require isolated unsaveable owners");
  if (seed && (resume || entry != XeenEncounterEntry::Journey || !*seed)) throw std::invalid_argument("Invalid Journey seed override");
  if (entry == XeenEncounterEntry::Journey) camera = original ? callback(services.loadInitialCamera) : services.eventFixture.value_or(services.journeyStart.value_or(xeenJourneyContent().entry));
  // A successful game entry clears pending currency and readiness and keeps the
  // four arrays (DOS play entry 0xcfe3..0xcff7); a failed or cancelled entry
  // leaves the running program's pending treasure exactly as it was.
  struct TreasureEntry {
   XeenTreasureStagingState &state;XeenTreasureStagingState::Saved before;bool committed=false;
   ~TreasureEntry(){if(!committed)state.restore(before);}
  } treasureEntry{services.program.treasure,services.program.treasure.save()};
  treasureEntry.state.enterGame();
  XeenWorld world(services.maps, services.objects);
  world.borrowTreasureStaging(services.program.treasure);
  XeenPartyState party;
  XeenGameFlags flags;
  const auto preflight = [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &) {
   if (w.sessionState().journey()) {
    if (!services.composeEncounter) throw std::invalid_argument("Missing Journey presentation provider");
    if (!services.composeEncounter(w,p,c,0,XeenMonsterAppearance{0}).frame.isValid())
     throw std::runtime_error("Invalid Journey first frame");
   } else if (!services.compose(w, p, c, std::uint64_t{0}).frame.isValid()) throw std::runtime_error("Invalid first gameplay frame");
   if (sourceCheck) sourceCheck();
  };
  if (resume) {
   if (!target) throw std::runtime_error("Resume requires a save path");
   const auto saved = services.restoreSnapshot ? *services.restoreSnapshot : XeenSaveFile::read(*target);
   services.saveName=saved.name;
   XeenSaveState::restoreBeforeGameplay(saved, services.resources, party, camera, flags, world, preflight);
   services.currentSlot=services.initialSlot;
   entry = world.sessionState().encounterEntry();
   encounter = entry != XeenEncounterEntry::Ordinary;
  } else {
   party = services.resources.loadInitialParty();
   party.playTimeSeconds=0;party.cloudsCompleted=false;
   flags = services.initialFlags();
  }
  for (const auto &diagnostic : party.diagnostics) std::cerr << "Party warning: " << diagnostic << '\n';
  XeenEventSystem events([&](XeenMapIdentity id) { return XeenEventScript(services.resources.loadEvents(id)); }, services.texts);
  const auto encounterEvents = encounter && !resume ? services.resources.loadEvents(camera.mapId) : XeenEventFile{};
  std::vector<std::uint8_t> journeyCharacters;
  std::vector<XeenMonsterRecord> journeyStatistics;
  std::optional<XeenJourneySetup> journeySetup;
  if (entry == XeenEncounterEntry::Journey && !resume) {
   if (!services.resources.loadInitialCharacters || !services.resources.loadInitialContext || !services.resources.loadMonsterStatistics)
    throw std::invalid_argument("Missing Journey initialization providers");
   journeyCharacters = services.resources.loadInitialCharacters();
   journeyStatistics = services.resources.loadMonsterStatistics();
   auto value = seed ? *seed : (services.sampleJourneySeed ? services.sampleJourneySeed() : std::random_device{}());
   if (!value) value = 1;
   journeySetup.emplace(XeenJourneySetup{journeyCharacters,services.resources.loadInitialContext(),journeyStatistics,encounterEvents,value});
   journeySetup->regionalManifest=services.resources.regionalManifest;
   journeySetup->start=services.journeyStart;
   journeySetup->eventFixture=services.eventFixture;
   journeySetup->eventsProvider=services.resources.loadEvents;
   if (services.eventFixture) journeySetup->mainlandEventsProvider=[&] {return services.resources.loadEvents(23);};
	journeySetup->prepared=!original;
	if(original) {
	 journeySetup->context.difficulty=*services.originalStart;
	 journeySetup->mainlandEventsProvider=[&] {return services.resources.loadEvents(23);};
	}
	journeySetup->cityEventsProvider=[&] { return services.resources.loadEvents(28); };
   {
    if (!services.resources.loadInitialBankBalances) throw std::invalid_argument("Missing original bank provider");
    journeySetup->bank=services.resources.loadInitialBankBalances();
   }
   {
    if(!services.resources.loadInitialPurse) throw std::invalid_argument("Missing original purse provider");
    journeySetup->purse=services.resources.loadInitialPurse();
   }
   {
    if (!services.resources.loadInitialWorldFlags) throw std::invalid_argument("Missing original regional recovery provider");
    if (!services.resources.loadRegionalText) throw std::invalid_argument("Missing regional text provider");
    journeySetup->worldFlags=services.resources.loadInitialWorldFlags();
    journeySetup->regionalText=services.resources.loadRegionalText(23);
   }
   {
    if (!services.resources.loadLearnedSpellNames) throw std::invalid_argument("Missing learned spell names provider");
    journeySetup->learnedNames=services.resources.loadLearnedSpellNames();
    journeySetup->learnedNamesProvider=services.resources.loadLearnedSpellNames;
   }
  }

  XeenEventFlow flow(services.program,world, events, party, camera, flags, services.font,
   [&](std::uint64_t phase) { return services.compose(world, party, camera, phase); }, services.npcDraw, services.clock, {}, services.catalog,
   [&](std::uint64_t ordinary, XeenMonsterAppearance actor) {
    const auto observedCamera = camera;
    {
     XeenRestoreGuard guard(world, party, camera, flags);
     XeenRestoreGuard::Providers providers(guard, world);
     try {
      auto frame = services.composeEncounter(world, party, observedCamera, ordinary, actor);
      guard.check();
      return frame;
     } catch (...) { guard.check(); throw; }
    }

	  }, journeySetup ? &*journeySetup : nullptr,
	  [&](XeenWorld &candidate, const XeenPartyState &candidateParty, const XeenCamera &candidateCamera,
	      std::uint64_t ordinary, XeenMonsterAppearance actor) {
	   return services.composeEncounter(candidate,candidateParty,candidateCamera,ordinary,actor);
	  }, nullptr, services.presentationClock);
  EncounterHandoff handoff(flow);
  flow.prepareJourneySprites = [&] {
   const auto ticket = flow.encounter()->ticket();
   if (!services.validateEncounterSprite || (!services.validateCombatSprite)) throw std::invalid_argument("Missing Journey sprite providers");
   const auto actors=world.sessionState().regionalActors(camera.mapId);
   for (unsigned i=0;i<actors.size();++i) {
    if(!actors[i].statistics) {
     if(actors[i].lifecycle!=XeenActorLifecycle::Unresolved &&
         !(actors[i].lifecycle==XeenActorLifecycle::Disabled && !actors[i].original.hasResource()))
      throw std::invalid_argument("Missing Journey sprite statistics");
     continue;
    }
    const auto image = actors[i].statistics->image();
    services.validateEncounterSprite(image);
    if (!flow.encounter()->current(ticket)) throw std::logic_error("Stale Journey normal sprite preparation");
    services.validateCombatSprite(image);
    if (!flow.encounter()->current(ticket)) throw std::logic_error("Stale Journey attack sprite preparation");
   }
  };
  if (services.configureFlow) services.configureFlow(flow, camera);
  handoff.verify();
  flow.saveInn=[&] {
   if(services.currentSlot && services.writeManaged)
    xeenSaveGameplay(services,world,party,camera,flags,flow,{},preflight,&sourceCheck,
     [&](const auto &snapshot,const auto &check){services.writeManaged(*services.currentSlot,snapshot,check);});
   else if(services.developerSavePath)
    xeenSaveGameplay(services,world,party,camera,flags,flow,*services.developerSavePath,preflight,&sourceCheck);
   else XeenSaveState::settleDismissedLodging(party,camera,flags,world);
  };
  if(party.insideInn) {flow.openInn();handoff.retain();}
  else services.program.innSongStarted=false;
  handoff.verify();
  if (!flow.frame().isValid()) throw std::runtime_error("Invalid first gameplay frame");
  std::cout << "Setup " << xeenInventoryInspection(party);
  if (flow.journey()) std::cout << flow.encounter()->journeyInspection() << flow.encounter()->notice() << '\n';
  // This is the only production new-session/resume initialization choice.
  auto first = resume || encounter ? flow.frame() : flow.initial();
  if (!first.isValid()) throw std::runtime_error("Invalid first gameplay frame");
  if(services.publishInitial) {
   if(!original || resume || !services.saveName)throw std::logic_error("Invalid New publication request");
   auto snapshot=XeenSaveState::captureInitialized(services.resources.signature,party,camera,flags,world);
   snapshot.journey->playTimeSeconds=services.program.playTime(0);
   snapshot.name=services.saveName;
   auto &guard=flow._encounter->journeySavePreimage();
   XeenRestoreGuard::Providers providers(guard,world);
   struct SourceCheck {
    std::function<void()> &slot;std::function<void()> previous;
    ~SourceCheck(){slot=std::move(previous);}
   } held{sourceCheck,std::move(sourceCheck)};
   sourceCheck=[&]{guard.check();handoff.verify();};
   XeenWorld scratch(services.maps,services.objects);XeenPartyState p;XeenCamera c;XeenGameFlags f;
   XeenSaveState::restoreBeforeGameplay(snapshot,services.resources,p,c,f,scratch,preflight);
   sourceCheck();
   bool published=false;
   try {published=services.publishInitial(snapshot,sourceCheck);sourceCheck();}
   catch(...) {sourceCheck();throw;}
   if(!published)return 5; // Cancellation discards only this unpublished session.
   const auto mark=committedSaveMark(services,flow);sourceCheck();
   flow.commitInitialPlayTime(snapshot.journey->playTimeSeconds);services.program.playTimeMark=mark;
   services.currentSlot=services.initialSlot;
  }
  bool entryPassPending=flow.journey() && !party.insideInn && (resume || services.publishInitial);
  bool arrivalDisplayed=false;
  treasureEntry.committed=true;
  if (services.observeGameplay) services.observeGameplay(world, events, party, camera, flags);
  handoff.verify();
  std::cout << "Map " << camera.mapId << ": camera X=" << camera.x << " Y=" << camera.y
      << " direction=" << static_cast<unsigned>(camera.direction) << '\n';
  const auto &geometry = world.map(camera.mapId).geometry;
  handoff.verify();
  if (!geometry.isOutdoors() && (geometry.flags2 & 0x4000))
   std::cout << "Warning: dark indoor map is rendered illuminated for diagnostics.\n";
  std::string status = "MMModern - Map " + std::to_string(camera.mapId.number);
  status += " - " + xeenInventorySummary(party);
  const auto looseTarget=services.developerSavePath?services.developerSavePath:
   services.initialSlot?std::optional<std::filesystem::path>{}:target;
  if (looseTarget) {
   status += " - F9 saves and replaces " + looseTarget->u8string();
   std::cout << "F9 saves and replaces " << looseTarget->u8string() << '\n';
  }
  if (resume) std::cout << "Resumed " << (services.currentSlot?services.saveName.value_or(""):target->u8string()) << '\n';
  bool dispatching = false;
  bool active = true;
  bool finished=false;
  std::unique_ptr<XeenTitleFlow> panel;
  std::shared_ptr<XeenRestoreGuard> panelGuard;
  IndexedFrame::Presentation panelOrigin;
  std::optional<XeenEncounterFlow::Ticket> panelTicket;
  const auto checkPanel=[&] {
   if(!active || !panel || !panelGuard || !flow.acceptsInputFrame(panelOrigin) ||
      (panelTicket && !flow.encounter()->current(*panelTicket)))
    throw std::logic_error("Stale control panel boundary");
   panelGuard->check();
  };
  const auto retainPanel=[&] {
   panelGuard=flow.journey() && !flow.encounter()->combat()?flow.encounter()->retainSavePreimage():
    std::make_shared<XeenRestoreGuard>(world,party,camera,flags);
   if(flow.encounter())panelTicket=flow.encounter()->ticket();
  };
  const auto publishSound=[&] {
   const auto value=panel->soundOptions();const XeenSoundOptions before=party.soundOptions;
   const auto toggle=panel->takeSoundToggle();
   if(value!=before) {
    checkPanel();
    if(!flow.configureSound(value))throw std::logic_error("Control panel sound publication refused");
    handoff.retain();retainPanel();
   }
   if(const auto &audio=services.program.audio) {
    if(toggle==1)audio->toggleMusic(value.music,flow.presentationNow());
    audio->observeOptions(value);
    if(toggle==0 && value.efx)audio->effect(20,flow.presentationNow());
   }
  };
  const auto panelAction=[&](const PlayerAction &action,const IndexedFrame::Presentation &origin) -> std::optional<IndexedFrame> {
   if(!panel->acceptsInput(origin))return {};
   checkPanel();
   {XeenRestoreGuard::Providers providers(*panelGuard,world,checkPanel);
    try {panel->handle(action,origin);checkPanel();}
    catch(const std::exception &e){checkPanel();panel->panelFailure(e.what());}}
   publishSound();
   if(!panel->current()) {
    panel.reset();panelGuard.reset();panelTicket.reset();flow._queueContext.reset();
    auto next=flow.completeExplorationInput();handoff.retain();return next;
   }
   if(panel->entry()) {
    const auto request=*panel->entry();
    if(request.kind==XeenSessionEntry::Kind::Save) {
     if(!flow.canSave() || !flow.journey())throw std::logic_error("Panel save authorization changed");
     bool saved=false;
     try {
      auto named=services;named.saveName=request.name;
      checkPanel();
      xeenSaveGameplay(named,world,party,camera,flags,flow,{},preflight,&sourceCheck,
       [&](const auto &snapshot,const auto &check){services.writeManaged(request.slot,snapshot,check);});
      saved=true;
     }catch(const std::exception &e) {
      handoff.retain();
      // An owner mismatch is fatal; a file/preflight failure is recoverable.
      if(!flow.encounterFrameCurrent())throw;
      panelTicket=flow.encounter()->ticket();checkPanel();
      auto message=std::string(e.what());
      if(message.find("Windows error 112")!=std::string::npos || message.find("Windows error 39")!=std::string::npos)
       message=std::string(flow.dosText().scalar("SAVE_AS_SPACE"));
      panel->panelResult(message);
     }
     if(saved) {
      // The file is committed. Later UI/provider failures cannot turn this
      // completed write into a reported save failure or retry the transaction.
      handoff.retain();retainPanel();checkPanel();
      services.currentSlot=request.slot;services.saveName=request.name;
      panel->panelCurrent(request.slot,request.name);
      status="MMModern - Game saved.";
      try {
       XeenRestoreGuard::Providers providers(*panelGuard,world,checkPanel);
       panel->panelResult(xeenDialogFormat(flow.dosText().scalar("SAVED_NOTICE"),{request.name}),true);
       checkPanel();
      }catch(const std::exception &e) {
       checkPanel();
       std::cerr<<"Game saved; confirmation presentation failed: "<<e.what()<<'\n';
       status="MMModern - Game saved; confirmation unavailable.";
       panel->panelResult("Game saved. Confirmation unavailable.");
       checkPanel();
      }
     }
    } else if(request.kind==XeenSessionEntry::Kind::Load) {
     if(!flow.canSave() || !flow.journey())throw std::logic_error("Panel load authorization changed");
     struct CheckScope {
      std::function<void()> &slot,previous;
      ~CheckScope(){slot=std::move(previous);}
     } held{sourceCheck,std::move(sourceCheck)};
     sourceCheck=checkPanel;
     try {
      checkPanel();XeenRestoreGuard::Providers providers(*panelGuard,world,checkPanel);
      if(!request.snapshot)throw std::logic_error("Missing immutable Load candidate");
      XeenWorld scratch(services.maps,services.objects);XeenPartyState p;XeenCamera c;XeenGameFlags f;
      XeenSaveState::restoreBeforeGameplay(*request.snapshot,services.resources,p,c,f,scratch,preflight);
      checkPanel();
      if(!services.outcome)throw std::logic_error("Missing Load session outcome");
      *services.outcome={XeenSessionOutcome::Kind::Load,0,request};finished=true;
     }catch(const std::exception &e){checkPanel();panel->loadFailure(e.what());}
    } else if(request.kind==XeenSessionEntry::Kind::Exit) {
     if(services.program.audio)services.program.audio->stopCd(flow.presentationNow());
     finished=true;
    }
    else if(request.kind==XeenSessionEntry::Kind::MrWizard) {
     checkPanel();
     // The original closes the panel and moves the party in one step.
     const bool combat=flow.encounter() && flow.encounter()->combat();
     if(auto moved=flow.mrWizard()) {
      panel.reset();panelGuard.reset();panelTicket.reset();flow._queueContext.reset();
      // A combat panel is nested in the interrupted input: no pass of its own.
      if(combat){handoff.retain();return moved;}
      auto next=flow.completeExplorationInput();handoff.retain();return next;
     }
     panel->panelFailure("not supported yet");
    }
   }
   checkPanel();return panel->frame();
  };
  const auto dispatch = [&](const PlayerAction &action, std::optional<std::uint64_t> input,
      const IndexedFrame::Presentation &inputFrame = {}) -> std::optional<IndexedFrame> {
   if(finished || entryPassPending)return {};
   if(panel) {
    if(!active || dispatching || finished)return {};
    GameplayScope scope(dispatching);
    try{return panelAction(action,inputFrame?inputFrame:panel->frame().presentation());}
    catch(...){handoff.fail();active=false;throw;}
   }
   // A service/handoff refusal is pure coordination: no owner/resource guard,
   // capture provider, path preparation or file operation may run here.
   if (std::holds_alternative<SaveGameAction>(action) && flow.journey() &&
       (dispatching || flow.serviceSaveBlocked())) {
    status="MMModern - Cannot save during service or pending presentation.";
    return std::nullopt;
   }
   // F9 is intercepted here, so it must pass the same displayed authority gate
   // as every Journey input before capture, providers, or target work.
   if (inputFrame && !flow.acceptsInputFrame(inputFrame)) return std::nullopt;
   if (flow.journey() && !flow.journeyInputCurrent(input)) return std::nullopt;
   // This irreversible entry decision needs no access to possibly closed owners.

   if (!active || dispatching) {
    if (std::holds_alternative<SaveGameAction>(action))
     status = "MMModern - Cannot save outside an idle gameplay boundary.";
    return std::nullopt;
   }
   // Unsafe encounter refusal precedes target handling and all save work.
   if (std::holds_alternative<SaveGameAction>(action) &&
       (flow.journey() || world.hasEncounterState() || party.encounterContext || party.roster.combatMarked()) &&
       !XeenSaveState::canCapture(party,camera,world)) {
    try { status = services.saveUnavailable ? std::string("MMModern - Saving is ")+services.saveUnavailable+"." :
        "MMModern - Cannot save: encounter session is unsaveable."; }
    catch (...) { handoff.fail(); active = false; throw; }
    return std::nullopt;
   }

   GameplayScope scope(dispatching);
   try {
   if(std::holds_alternative<ControlPanelAction>(action) && services.panel) {
    const bool combat=flow.encounter() && flow.encounter()->combat();
    const auto context=flow.inputContext(inputFrame?inputFrame:flow.frame().presentation());
    const bool developerQuiet=services.eventFixture && flow.encounter() && flow.encounterFrameCurrent() &&
        flow.encounter()->journeyQuiet() && !flow.blocksGameplay() && !flow.inventoryOpen();
    if(!flow.canSave() && !developerQuiet && !(combat && context.readyForAction && !context.dialog &&
       flow.encounter()->combat()->phase()==XeenCombatPhase::PlayerReady))return {};
    panelOrigin=inputFrame?inputFrame:flow.frame().presentation();retainPanel();
    {XeenRestoreGuard::Providers providers(*panelGuard,world);
     panel=services.panel(flow.frame(),combat,bool(world.map(camera.mapId).geometry.flags&0x8000),
      flow.journey() && !services.saveUnavailable,services.currentSlot,services.saveName.value_or(""));
     panelGuard->check();}
    panel->clocks([&] {
     const auto now=services.program.wallClock.now();
     return std::make_pair(now,now-services.program.playTimeMark+std::uint32_t(party.playTimeSeconds));
    });
    panel->soundOptions(party.soundOptions);publishSound();
    checkPanel();return panel->frame();
   }
   if (std::holds_alternative<SaveGameAction>(action)) {
    std::string message;
    bool success = false;
    if (flow.inventoryOpen()) message = "Cannot save while inventory is open. Close it and press F9 again.";
    else if (!flow.canSave()) message = "Cannot save while an interaction is pending.";
    else if (!flow.journey()) message = "Map exploration cannot save.";
    else if (services.saveUnavailable) message = std::string("Saving is ") + services.saveUnavailable + ".";
    else if (!looseTarget) message = "No developer save target configured. Use --save-file <path>.";
    else try {
     xeenSaveGameplay(services,world,party,camera,flags,flow,*looseTarget,preflight,&sourceCheck);
     success = true; message = "Saved";
    } catch (const std::exception &e) { flow._queueContext.reset(); message = std::string("Save failed: ") + e.what(); }

    handoff.retain();
    if (looseTarget) message += " [" + looseTarget->u8string() + "]";
    status = "MMModern - " + message;
    (success ? std::cout : std::cerr) << message << '\n';
    if (flow.inventoryOpen()) return flow.refuseInventorySave();

    if(!flow.blocksGameplay()) {auto next=flow.completeExplorationInput();handoff.retain();return next;}
    return std::nullopt;
   }
   auto mapped = action;
   if (flow.journey() && flow.encounter()->combat() && !flow.encounter()->combat()->cast() && std::holds_alternative<InteractionAction>(action)) mapped = AttackAction{};
   if (flow.encounter() && flow.encounter()->combat() && !flow.inventoryOpen() &&
       flow.encounter()->combat()->phase() == XeenCombatPhase::PlayerReady) {
    if (const auto *slot = std::get_if<SelectInventorySlotAction>(&action); slot && slot->slot < 3)
     mapped = SelectCombatTargetAction{static_cast<unsigned>(slot->slot)};
   }

   auto next = flow.handle(mapped,input,inputFrame);
   handoff.retain();
   return next;
   } catch (...) { handoff.fail(); active = false; throw; }
  };
  // Copied native callbacks retain only a liveness token after this scope ends.
  // Check it before touching any borrowed Flow/owner/dispatcher reference.
  const auto alive=std::make_shared<bool>(true);
  struct AudioClockScope {
   std::shared_ptr<XeenAudio> audio;
   XeenEventFlow &flow;PresentationClock::Source source;bool finished=false;
   void finish() {
    // Native close may already have retired the Flow's encounter ticket.
    // Consume time from its retained clock source, without reentering owners.
    if(audio){audio->advance(source());audio->rebaseClock(SdlWindow::presentationNanoseconds());}
    finished=true;
   }
   ~AudioClockScope() {
    if(!finished)try{finish();}catch(...){if(audio)audio->rebaseClock(SdlWindow::presentationNanoseconds());}
   }
  } audioClock{services.program.audio,flow,services.presentationClock?services.presentationClock:
    PresentationClock::Source{[clock=services.clock]{return PresentationClock::fromMilliseconds(clock());}}};
  if(audioClock.audio) {
   audioClock.audio->advance(SdlWindow::presentationNanoseconds());
   audioClock.audio->rebaseClock(flow.presentationNow());
  }
  std::optional<unsigned> soundLocation;
  std::optional<XeenMapIdentity> soundMap;
  const auto updateAudio=[&] {
   const auto &audio=services.program.audio;if(!audio)return;
   audio->observeOptions(party.soundOptions);
   const auto now=flow.presentationNow();
   audio->selectCdDisc(camera.mapId.side==XeenSide::Clouds?1:2,now);
   audio->advance(now);
   const auto location=flow.audioLocation();
   if(location!=soundLocation || soundMap!=camera.mapId) {
    if(location && *location==7) {
     if(!services.program.innSongStarted) {audio->requestInn(now);services.program.innSongStarted=true;}
    }
    else if(location)audio->requestLocation(*location,now);
    else if(soundLocation)audio->requestSong(audio->mapSong(world.map(camera.mapId).geometry),XeenSceneArchive::Clouds,223,now);
    else audio->requestMap(world.map(camera.mapId).geometry,now);
    soundLocation=location;soundMap=camera.mapId;
   }
  };
  updateAudio();
  struct CallbackLifetime {std::shared_ptr<bool> alive;~CallbackLifetime(){*alive=false;}} callbackLifetime{alive};
  if(services.panel)flow.continueExplorationPanel=[&](const PlayerAction &action) {
   return dispatch(action,flow.displayedInput(),flow.frame().presentation());
  };
  SdlWindow::FrameUpdateHandler handler = [&,alive](const PlayerAction &action) -> std::optional<IndexedFrame> {
   if(!*alive)return {};return dispatch(action,{});
  };
  handler.displayedInput = [&,alive] { return *alive?flow.displayedInput():std::optional<std::uint64_t>{}; };
  handler.inputContext = [&,alive](const auto &origin) {
   if(!*alive)return InputContext{};
   if(panel){checkPanel();auto context=panel->inputContext(origin);context.contextId|=std::uint64_t{1}<<63;return context;}
   auto context=flow.inputContext(origin);
   if(entryPassPending)context.readyForAction=context.readyForNavigation=context.restAvailable=false;
   return context;
  };
  if(flow.drawDialogSprite) handler.drawButton = [&,alive](IndexedFrame &frame,const InputButton &button) {
   if(!*alive)return;
   if(panel)checkPanel();
   try {if(flow.drawDialogSprite)flow.drawDialogSprite(frame,button.resource,button.pressedFrame(),button.x,button.y);}
   catch(...){if(panel)checkPanel();throw;}
   if(panel)checkPanel();
  };
  handler.acceptsFrame = [&,alive](const auto &frame) { return *alive && (panel?panel->acceptsFrame(frame):flow.acceptsFrame(frame)); };
  handler.acceptsInputFrame = [&,alive](const auto &frame) { return *alive && (panel?panel->acceptsInput(frame):flow.acceptsInputFrame(frame)); };
  handler.completeInputHandoff = [&,alive](const auto &frame) {
   if(!*alive)return;
   if(panel){checkPanel();panel->completeInput(frame);}else {flow.completeInputHandoff(frame);handoff.retain();}
  };
  handler.protectAllKeys = flow.journey();
  handler.nextPresentationDeadline=[&,alive]() -> std::optional<std::uint64_t> {
   if(!*alive)return {};
   if(finished && services.outcome && services.outcome->kind==XeenSessionOutcome::Kind::Title && services.program.audio)
    return services.program.audio->digitalDeadline(audioClock.source());
   auto next=panel?panel->nextPresentationDeadline():flow.nextPresentationDeadline();
   if(services.program.audio){const auto sound=services.program.audio->deadline();if(sound && (!next||*sound<*next))next=sound;}
   return next;
  };
  handler.presentationClock=[&,alive] {return *alive?flow.presentationNow():SdlWindow::presentationNanoseconds();};
  handler.withDisplayedInput = [&,alive](const PlayerAction &action,std::uint64_t input) -> std::optional<IndexedFrame> { if(!*alive)return {};return dispatch(action,input); };
  handler.withPresentedInput = [&,alive](const PlayerAction &action,std::uint64_t input,const auto &frame) -> std::optional<IndexedFrame> { if(!*alive)return {};return dispatch(action,input,frame); };
  handler.beginCycle = [&,alive](std::uint64_t cycle) {
   if(!*alive)return;
   if (!active) throw std::runtime_error("Gameplay session is closed");
   updateAudio();
   if(panel)checkPanel();else flow.beginCycle(cycle);
  };
  handler.frameCurrent = [&,alive] { if(!*alive)return false;if(panel)checkPanel();return active && flow.encounterFrameCurrent(); };
  handler.framePresented = [&,alive](const auto &frame) {
   if(!*alive)return;
   if(panel){checkPanel();panel->presented(frame);}else {
    flow.framePresented(frame,true);handoff.retain();
    if(entryPassPending && frame==first.presentation())arrivalDisplayed=true;
   }
  };
  handler.finished=[&,alive]{
   if(!*alive)return true;
   if(!finished)return false;
   if(services.outcome && services.outcome->kind==XeenSessionOutcome::Kind::Title && services.program.audio) {
    services.program.audio->observeOptions(party.soundOptions);
    return services.program.audio->titleTeardownReady(audioClock.source());
   }
   return true;
  };
  handler.failed = [&,alive] { if(!*alive)return;handoff.fail(); active = false;*alive=false; };
  handler.closed = [&,alive] { if(!*alive)return;flow.closeGameplay(); active = false;*alive=false; };
  const auto idle = [&,alive]() -> std::optional<IndexedFrame> {
   if(!*alive)return {};
   if (!active || dispatching) return std::nullopt;
   updateAudio();
   if(finished)return {}; // Drain the logical voice without further Flow work.
   GameplayScope scope(dispatching);
   if(panel) {
    try {
     checkPanel();const auto now=services.clock?services.clock():static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
     checkPanel();return panel->animate(now);
    }catch(...){handoff.fail();active=false;throw;}
   }
   try {
    if(entryPassPending) {
     if(!arrivalDisplayed)return {};
     entryPassPending=false;
     auto next=flow.completeExplorationInput();handoff.retain();return next;
    }
    auto next=flow.updatePresentation();handoff.retain();
    if(flow.partyDeathComplete()) {
     if(services.outcome)*services.outcome={XeenSessionOutcome::Kind::Title,0,{}};
     finished=true;
    }
    return next;
   }
   catch (...) { handoff.fail(); active = false; throw; }
  };
  handoff.verify();
  exposed=true;
  // Title-menu sessions show the player only the program name; developer
  // entries keep their diagnostic status line.
  const bool playerTitle=services.initialSlot && !services.developerSavePath;
  const auto shown=[&](std::string text){return playerTitle?std::string("MMModern"):text;};
  const bool ok = services.show(first, handler, [&,alive] { return *alive && (panel || flow.handlesEscape()); }, idle, [&,alive] {
   if(!*alive)return shown("MMModern - Session closed");
   try { return shown(status); } catch (...) { handoff.fail(); active = false; throw; }
  });
  audioClock.finish();
  if (!ok) handler.failed();
  flow.closeGameplay();
  active = false;
  flow.abandonPresentation();
  return ok ? 0 : 4;
 } catch (const std::exception &e) {
  std::cerr << "Gameplay startup failed";
  if (target) std::cerr << " [" << target->u8string() << ']';
  std::cerr << ": " << e.what() << '\n';
  return exposed && (supplied.restoreSnapshot || supplied.publishInitial)?4:3;
 }
}
}
