#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "app/XeenGameplayServices.h"
#include "app/Application.h"
#include "app/XeenAudio.h"
#include "tools/XeenTestSave.h"
#include "games/xeen/XeenEndingScore.h"
#include "games/xeen/XeenScriptedRules.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "tools/XeenDosExport.h"
#include "platform/XeenWonStore.h"
#include "TestTreeRemoval.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenMonsterTestAccess {
 static void retainDestinationWithoutActors(XeenEventFlow &flow,XeenMapIdentity map) {
  auto &e=*flow._encounter;e._world._sessionState._mapStates.state(map);
  e.retainJourney();flow._encounterFrame=e.ticket();
 }
 static XeenGameFlags pendingFlags(const XeenEventFlow &flow){return flow._pending->state.workingGameFlags;}
 static void markCameraVisit(XeenEventFlow &flow) {
  auto &e=*flow._encounter;e._world._sessionState._mapStates.state(e._camera.mapId).explored.emplace();
  e.retainJourney();flow._encounterFrame=e.ticket();
 }
 static std::size_t changeUnvisitedActor(XeenEventFlow &flow,bool killed) {
  auto &e=*flow._encounter;auto &state=e._world._sessionState;
  auto &map=state._mapStates.state(e._camera.mapId);
  const auto actor=std::find_if(map.monsters->begin(),map.monsters->end(),[](const auto &a){return a.lifecycle==XeenActorLifecycle::Present;});
  if(actor==map.monsters->end())throw std::logic_error("Unvisited actor fixture needs a live MOB record");
  const auto record=actor->id.recordIndex;
  if(killed) {
   actor->x=actor->y=-128;actor->hp=0;actor->lifecycle=XeenActorLifecycle::Defeated;
   state._accountedMonsters.insert(actor->id);
  }else {actor->x=actor->original.x==0?1:0;actor->y=0;}
  e.retainJourney();flow._encounterFrame=e.ticket();return record;
 }
 static bool endingSaveFailed(const XeenEventFlow &flow){return !flow._endingError.empty() && !flow._endingSnapshot;}
};
}
namespace {
namespace fs=std::filesystem;
std::string bytes(const fs::path &path) {std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
struct Directory {
 fs::path path=fs::temp_directory_path()/("mmodern-m92-"+std::to_string(GetCurrentProcessId()));
 Directory(){removeTestTree(path);check(fs::create_directories(path),"Ending test directory");}
 ~Directory(){removeTestTree(path);}
};
XeenSaveSnapshot scored(Inputs &in) {
 auto s=in.base();s.camera={28,18,4,XeenDirection::West};s.headCache.reset();
 constexpr unsigned classes[]{1,0,2,5,3,4},levels[]{15,15,14,17,15,15};
 constexpr unsigned xp[]{1735710,2243510,2361562,1989740,2266910,1749910};
 check(s.activeRosterIds.size()==6,"Six score records");
 for(unsigned n=0;n<6;++n) {
  const auto id=s.activeRosterIds[n];s.characters[id].characterClass=static_cast<XeenCharacterClass>(classes[n]);
  s.characters[id].permanentLevel=levels[n];
  s.journey->supplements[id].inputs.experience=xp[n]+(n==2 || n==5?0:150000);
  s.characters[id].awards[42]=1;
 }
 s.characters[s.activeRosterIds[2]].conditions[unsigned(XeenCondition::Unconscious)]=1;
 s.characters[s.activeRosterIds[2]].conditions[unsigned(XeenCondition::Eradicated)]=1;
 s.characters[s.activeRosterIds[5]].conditions[unsigned(XeenCondition::Dead)]=1;
 s.characters[s.activeRosterIds[5]].conditions[unsigned(XeenCondition::Eradicated)]=1;
 s.journey->playTimeSeconds=62545;s.soundOptions.efx=s.soundOptions.music=false;
 s.questItems[8]=1;s.gameFlags[60]=true;s.questFlags[23]=false;return s;
}
XeenEventFile endingEvent(unsigned opcode=30) {
 XeenEventRecord r;r.x=18;r.y=4;r.direction=4;r.line=0;r.opcode=opcode;r.lengthField=5;
 return {28,"ending-test.evt",true,{r}};
}
void score(Inputs &in) {
 const auto source=scored(in);Fixture f(in,source);
 check(xeenCloudsEndingScore(f.p,62545)==71001722 && xeenFormatEndingScore(71001722)=="0071001722","Calibrated six live records including terminal members");
 check(xeenFormatEndingScore(0)=="0000000000" && xeenFormatEndingScore(4294967295u)=="4294967295","Unsigned minimum-width decimal score");
 // XP reconstruction is independent of conditions; the DOS time word is signed.
 check(xeenCloudsEndingScore(f.p,328u*3600u)==71000000u+std::uint32_t(-32736),"Time product wraps and sign extends at 16 bits");
}
void tail(Inputs &in,const fs::path &directory,bool retry,bool wonFailure=false) {
 auto source=scored(in);auto event=endingEvent();unsigned liveMap28Loads=0;
 const auto loader=in.mapLoader();
 Fixture f(in,source,false,&event,[&](auto id){if(id==XeenMapIdentity(28))++liveMap28Loads;return loader(id);});
 const auto loadedAtEntry=liveMap28Loads;
 std::uint32_t now=0;f.programState.wallClock=XeenWallClock([&]{return now;});
 XeenGameplayServices services{f.programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font};
 services.resources.loadEvents=[&](auto id){return id==XeenMapIdentity(28)?event:in.events.load(id);};
 unsigned markers=0,writes=0;std::vector<std::vector<std::uint8_t>> attempts;
 const auto target=directory/(retry?"retry.mmsave":wonFailure?"won-failure.mmsave":"ending.mmsave");
 f.flow->writeWon=[&](auto){++markers;check(f.p.cloudsCompleted && !f.f.isSet(75),"Completion then won before flag 75");return !wonFailure;};
 f.flow->saveForced=[&](auto *captured) {
  check(f.p.cloudsCompleted && f.f.isSet(75) && f.c.mapId==28 && f.c.x==18 && f.c.y==4,"Win and relocation before save");
  check(!XeenSoundOptions(f.p.soundOptions).efx && !XeenSoundOptions(f.p.soundOptions).music,"Save before forced audio switches");
  xeenSaveGameplay(services,f.w,f.p,f.c,f.f,*f.flow,target,[](auto &,const auto &,const auto &,const auto &){},nullptr,
   [&](const auto &s,const auto &checkSource){
    ++writes;attempts.push_back(XeenSaveFormat::encode(s));checkSource();
    if(retry && writes==1)throw std::runtime_error("Injected ending write failure");
    XeenSaveFile::write(target,s);
   },captured);
 };
 f.act(InteractionAction{});
 if(wonFailure){check(markers==1 && writes==0 && !f.flow->endingScore(),"Won failure presents a notice and continues on acknowledgement");f.act(AcknowledgeAction{});}
 if(retry) {
  check(!f.flow->endingScore() && writes==1,"Write failure retains ending recovery");
  now=50000;f.act(AcknowledgeAction{});
  check(writes==2 && attempts[0]==attempts[1],"Retry writes identical captured bytes without another tail");
 }
 check(markers==1 && f.flow->endingScore()==71001722,"One marker and frozen real ending score");
 check(XeenSoundOptions(f.p.soundOptions).efx && XeenSoundOptions(f.p.soundOptions).music,"Live sound enabled only after write");
 const auto saved=XeenSaveFile::read(target);
 check(saved.journey->playTimeSeconds==62545 && f.p.playTimeSeconds==62545 && f.programState.playTimeMark==now,"Snapshot play time frozen; successful commit resets mark");
 check(!saved.soundOptions.efx && !saved.soundOptions.music && saved.journey->cloudsCompleted,"Saved sound precedes live switches");
 check(saved.questItems[8]==1 && saved.gameFlags[60] && saved.gameFlags[75] && !saved.questFlags[23],"Tail preserves Scepter, quest 23 and flags");
 check(liveMap28Loads==loadedAtEntry,"Tail never reloads the live map");
 for(auto id:saved.activeRosterIds) {
  check(saved.characters[id].conditions==source.characters[id].conditions && saved.characters[id].awards==source.characters[id].awards &&
   saved.journey->supplements[id].inputs.experience==source.journey->supplements[id].inputs.experience,
   "Ordinary live conditions, awards, equipment and XP retained");
 }
}
void postWorld(Inputs &in) {
 auto source=scored(in);source.gameFlags[75]=true;
 Fixture before(in,source);auto after=source;after.journey->cloudsCompleted=true;Fixture completed(in,after);
 for(unsigned map=74;map<=79;++map) {
  auto caster=before.p.party.member(before.p.roster,0);caster.lloydMap=map;caster.lloydX=1;caster.lloydY=1;caster.lloydSide=0;
  const auto pre=XeenLearnedSpellRules::travel(before.w,before.c,XeenLearnedSpell::LloydsBeacon,2,&caster,before.p.cloudsCompleted);
  const auto post=XeenLearnedSpellRules::travel(completed.w,completed.c,XeenLearnedSpell::LloydsBeacon,2,&caster,completed.p.cloudsCompleted);
  check(!pre.failed && post.failed==(map>=75 && map<=78),"Beacon refusal uses saved completion only on maps 75-78");
  if(post.failed)check(!post.destination && !xeenTravelFmEffect(XeenLearnedSpell::LloydsBeacon,true,false,2),"Refusal has no successful effect 51");
 }
 XeenGameFlags flags;flags.set(75);XeenMapEntity castle;castle.resourceId=85;castle.x=4;castle.y=5;
 for(auto side:{XeenSide::Clouds,XeenSide::Darkside}) {
  check(xeenMapRuleObject(castle,{XeenMapIdentity(side,27),0},xeenMapRuleView(flags,false)).x==4,"Flag 75 alone cannot suppress castle");
  check(xeenMapRuleObject(castle,{XeenMapIdentity(side,27),0},xeenMapRuleView({},true)).x==-128,"Completion suppression has no side guard");
 }
 Fixture prewinReload(in,source);check(!prewinReload.p.cloudsCompleted,"Pre-win Load restores false even after completed party");
}
void mirrors(Inputs &in,const fs::path &directory) {
 in.texts=XeenEventTextLoader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
  if(!in.assets.hasArchiveResource(name))return {};return in.assets.readArchiveResource(name);
 },[&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {return in.assets.readArchiveResource(name,XeenSceneArchive::DarksideOnly);});
 for(const auto word:{"showtime","bloopers"}) {
  auto source=scored(in);source.camera={28,17,4,XeenDirection::North};source.gameFlags[75]=true;
  auto event=endingEvent(21);event.records[0].x=17;event.records[0].parameters={0,2,0,3};event.records[0].lengthField=9;
  auto next=event.records[0];next.line=1;next.opcode=30;next.parameters.clear();next.lengthField=5;event.records.push_back(next);
  Fixture f(in,source,false,&event);unsigned markers=0,writes=0;
  XeenGameplayServices services{f.programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font};
  services.resources.loadEvents=[&](auto id){return id==XeenMapIdentity(28)?event:in.events.load(id);};
  const auto path=directory/(std::string(word)+".mmsave");
  f.flow->writeWon=[&](auto){++markers;return true;};
  f.flow->saveForced=[&](auto *captured){++writes;xeenSaveGameplay(services,f.w,f.p,f.c,f.f,*f.flow,path,[](auto &,const auto &,const auto &,const auto &){},nullptr,{},captured);};
  f.now+=125;f.act(InteractionAction{});f.now+=125;f.act(TextInputAction{word});f.now+=125;f.act(DialogKeyAction{InputKey::Enter});
  check(!markers && !f.p.cloudsCompleted && f.c.mapId==28 && f.c.x==17 && f.c.y==4 && f.c.direction==XeenDirection::North,"Mirror words retain position, facing and completion; no won store");
  if(std::string(word)=="showtime") {
   check(writes==1 && f.flow->endingScore()==71001722,"Showtime saves and uses the real score");
   const auto saved=XeenSaveFile::read(path);check(!saved.journey->cloudsCompleted && saved.camera.x==17 && !saved.soundOptions.efx,"Showtime save precedes sound switches without marking completion");
  } else {
   for(unsigned n=0;n<50 && !f.flow->canSave();++n){f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);}
   check(!writes && !f.flow->endingScore() && f.flow->canSave(),"Bloopers resumes gameplay with no next Event line or refusal");
  }
 }
}
void applicationEnding(Inputs &in,const fs::path &directory,bool hasTarget=true,bool rewriteFailure=false,unsigned opcode=30) {
 auto source=scored(in);auto event=endingEvent(opcode);
 if(opcode!=30) {source.camera={{XeenSide::Darkside,51},2,13,XeenDirection::North};event.mapId=source.camera.mapId;event.records[0].x=2;event.records[0].y=13;source.darksideSong=7*xeenMapMusicRow(in.maps.loadGeometryMap(in.assets,source.camera.mapId).geometry);}
 const auto wonKind=opcode==57?XeenWonKind::Darkside:opcode==58?XeenWonKind::World:XeenWonKind::Clouds;
 const auto outcomeKind=opcode==57?XeenSessionOutcome::Kind::DarksideEnding:opcode==58?XeenSessionOutcome::Kind::WorldEnding:XeenSessionOutcome::Kind::CloudsEnding;
 std::uint32_t wall=100;XeenProgramState program{XeenWallClock([&]{return wall;})};
 const auto target=directory/(rewriteFailure?"rewrite-ending.mmsave":hasTarget?"application-ending.mmsave":"no-target.mmsave"),settings=directory/"application-settings.ini";
 if(rewriteFailure)check(xeenWriteWon(settings,wonKind),"Earlier successful win");
 const auto earlierSettings=bytes(settings);
 Application app(program,{},settings);XeenSessionOutcome outcome;
 struct Lock {HANDLE value=INVALID_HANDLE_VALUE;~Lock(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}} lock;
 if(rewriteFailure) {
  check(opcode==57?program.darksideWon:opcode==58?program.worldWon:program.cloudsWon,"Earlier selected win read at application entry");
  lock.value=CreateFileW(settings.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
  check(lock.value!=INVALID_HANDLE_VALUE,"Lock earlier won file against rewrite");
 }
 unsigned notices=0;
 XeenGameplayServices services{program,in.resources(),{},in.mapLoader(),in.objectLoader(),[&](auto id){return in.texts.load(id);},in.font};
 services.restoreSnapshot=std::make_shared<const XeenSaveSnapshot>(source);if(hasTarget)services.developerSavePath=target;services.outcome=&outcome;
 services.resources.loadEvents=[&](auto id){return id==event.mapId?event:in.events.load(id);};
 std::uint64_t now=0;services.clock=[&]{return now;};
 services.compose=[](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{frame(),false};};
 services.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{frame(),false};};
 services.configureFlow=[&](auto &flow,const auto &){flow.dialogText=&in.assets.uiText();};
 services.show=[&](auto visible,const auto &h,const auto &,const auto &idle,const auto &) {
  wall=110;if(!h.finished()){h.framePresented(visible.presentation());h.completeInputHandoff(visible.presentation());}
  bool sent=false;
  for(unsigned n=1;n<200 && !h.finished();++n) {
   now+=125;h.beginCycle(n);
   if(auto image=idle()){visible=*image;if(!h.finished()){h.framePresented(visible.presentation());h.completeInputHandoff(visible.presentation());}}
   if(h.finished())break;
   const auto context=h.inputContext(visible.presentation());
   if(context.readyForAction && context.dialog && context.dialog->anyKey) {
    ++notices;const auto image=h.withPresentedInput(AcknowledgeAction{},*h.displayedInput(),visible.presentation());
    if(image){visible=*image;if(!h.finished()){h.framePresented(visible.presentation());h.completeInputHandoff(visible.presentation());}}
   } else if(!sent && context.readyForAction) {
    sent=true;wall=110;const auto image=h.withPresentedInput(InteractionAction{},*h.displayedInput(),visible.presentation());
    if(image){visible=*image;if(!h.finished()){h.framePresented(visible.presentation());h.completeInputHandoff(visible.presentation());}}
   }
  }
  check(h.finished(),"Gameplay session must finish at CloudsEnding");return true;
 };
 check(app.playGameplay(services,{},target,true)==0 && outcome.kind==outcomeKind && outcome.score==71001722,"Application transports the score and matching ending kind");
 if(rewriteFailure)check(notices>0 && bytes(settings)==earlierSettings,"Failed rewrite shows a notice and retains every earlier settings value");
 const auto wonSettings=xeenReadSettings(settings);
 check(opcode==57?(program.darksideWon && wonSettings.darksideWon):opcode==58?(program.worldWon && wonSettings.worldWon):(program.cloudsWon && wonSettings.won),"Application writes only the selected win in its supplied settings path");
 if(hasTarget) {
  const auto saved=XeenSaveFile::read(target);
  check((opcode==57?saved.journey->darksideCompleted:opcode==58?saved.journey->worldCompleted:saved.journey->cloudsCompleted) && program.playTimeMark==110,"Successful ending write and mark reset");
 }
 else check(!fs::exists(target) && program.playTimeMark==100,"Developer no-target ending makes no write and keeps the mark, as no-target Sign In");
}
void originalAudioRecovery(Inputs &in,const GameInstallation &installation,const fs::path &directory) {
 const auto input=directory/"before-mirror.mmsave",target=directory/"audio-ending.mmsave";
 developer::generateTestSave(installation,"m92-sixth-mirror",input);
 auto source=XeenSaveFile::read(input);source.soundOptions.efx=source.soundOptions.music=false;source.journey->playTimeSeconds=62545;
 Fixture expected(in,source);const auto score=xeenCloudsEndingScore(expected.p,62545);
 const auto blocker=directory/"settings-blocker";{std::ofstream out(blocker);out<<"file";}
 XeenProgramState program{XeenWallClock::fixed(0)};program.audio=XeenAudio::installed(installation,in.assets,0,false,std::filesystem::path("mmodern-nonexistent-rom-fixture"));
 Application app(program,{},blocker/"settings.ini");XeenSessionOutcome outcome;
 bool beforeCapture=true;unsigned writeAttempts=0,acknowledgements=0;
 const auto maps=in.mapLoader();
 XeenGameplayServices services{program,in.resources(),{},[&](auto id){
  check(!beforeCapture || id!=XeenMapIdentity(28),"Audio must not load relocated Vertigo before the first ending capture");return maps(id);
 },in.objectLoader(),[&](auto id){return in.texts.load(id);},in.font};
 services.restoreSnapshot=std::make_shared<const XeenSaveSnapshot>(source);services.developerSavePath=target;services.outcome=&outcome;
 services.observeSaveStage=[&](auto stage) {
  if(stage==XeenGameplayServices::SaveStage::Capture)beforeCapture=false;
  if(stage==XeenGameplayServices::SaveStage::Write && ++writeAttempts==1)throw std::runtime_error("Injected original ending write failure");
 };
 std::uint64_t now=0;services.clock=[&]{return now;};services.presentationClock=[&]{return PresentationClock::fromMilliseconds(now);};
 services.compose=[](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{frame(),false};};
 services.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{frame(),false};};
 services.configureFlow=[&](auto &flow,const auto &){flow.dialogText=&in.assets.uiText();};
 services.show=[&](auto visible,const auto &h,const auto &,const auto &idle,const auto &) {
  const auto present=[&]{h.framePresented(visible.presentation());h.completeInputHandoff(visible.presentation());};present();bool sent=false;
  for(unsigned n=1;n<1000 && !h.finished();++n) {
   now+=125;h.beginCycle(n);if(auto image=idle()){visible=*image;present();}
   if(h.finished())break;
   const auto context=h.inputContext(visible.presentation());if(!context.readyForAction)continue;
   std::optional<PlayerAction> action;
   if(!sent){sent=true;action=InteractionAction{};}
   else if(context.dialog && context.dialog->anyKey){action=AcknowledgeAction{};++acknowledgements;}
   if(action)if(auto image=h.withPresentedInput(*action,*h.displayedInput(),visible.presentation())){visible=*image;present();}
  }
  check(h.finished(),"Original mirror ending with audio must finish after notices and retry");return true;
 };
 check(app.playGameplay(services,{},input,true)==0 && outcome.kind==XeenSessionOutcome::Kind::CloudsEnding && outcome.score==score,"Original mirror/audio Application ending outcome");
 const auto saved=XeenSaveFile::read(target);
 check(writeAttempts==2 && acknowledgements>=3 && !program.cloudsWon && !saved.find(28) && saved.journey->cloudsCompleted,"Won failure continues, save retries, and map 28 remains untouched");
 check(!saved.soundOptions.efx && !saved.soundOptions.music && program.audio->soundOptions().efx && program.audio->soundOptions().music,"Actual audio switches change only after the forced write");
}

void omittedTableGuards(Inputs &in,const GameInstallation &installation,const fs::path &directory) {
 auto unvisited=scored(in);check(!unvisited.find(28),"Fixture has an unvisited camera map");
 Fixture loaded(in,unvisited);
 const auto original=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,28),in.statistics);
 const auto actors=loaded.w.sessionState().regionalActors(28);
 check(actors.size()==original.size(),"Omitted unvisited camera table restores original MOB actors");
 for(unsigned n=0;n<actors.size();++n)check(actors[n].x==original[n].x && actors[n].y==original[n].y &&
  actors[n].original.resourceId==original[n].original.resourceId,"Original unvisited actor positions and identities");
 auto retained=unvisited;retained.map(28).tavernTips=0;
 XeenWorld world(in.mapLoader(),in.objectLoader());XeenPartyState party;XeenCamera camera;XeenGameFlags flags;
 bool refused=false,preflight=false;
 try{XeenSaveState::restoreBeforeGameplay(retained,in.resources(),party,camera,flags,world,[&](auto &,const auto &,const auto &,const auto &){preflight=true;});}
 catch(const std::invalid_argument &){refused=true;}
 check(refused && !preflight,"Preflight refuses retained camera state without its monster table");
 // Pristine runtime reconstruction preserves an omitted table byte-for-byte.
 check(XeenSaveFormat::encode(XeenSaveState::capture(in.signature,loaded.p,loaded.c,loaded.f,loaded.w))==XeenSaveFormat::encode(unvisited),"Unvisited camera reconstruction is not a saved visit");
 for(bool killed:{false,true}) {
  Fixture changed(in,unvisited);
  const auto record=XeenMonsterTestAccess::changeUnvisitedActor(*changed.flow,killed);
  check(!changed.w.sessionState().mapState(28)->explored,"Actor mutation must not explore the unvisited camera map");
  const auto saved=XeenSaveState::capture(in.signature,changed.p,changed.c,changed.f,changed.w);
  const auto *map=saved.find(28);
  check(map && map->monsters && !map->explored && saved.maps.size()==unvisited.maps.size()+1,"Moved or killed actor alone must retain the reconstructed camera table");
  const auto &slot=map->monsters->slots.at(record);
  if(killed)check(slot.x==-128 && slot.y==-128 && slot.lifecycle==XeenActorLifecycle::Defeated && slot.accounted,"Killed unvisited actor is saved and accounted");
  else check(slot.x==(original[record].x==0?1:0) && slot.y==0 && slot.lifecycle==XeenActorLifecycle::Present && !slot.accounted,"Moved unvisited actor saves its changed position");
  const auto wire=XeenSaveFormat::encode(saved);Fixture restored(in,XeenSaveFormat::decode(wire));
  check(XeenSaveFormat::encode(XeenSaveState::capture(in.signature,restored.p,restored.c,restored.f,restored.w))==wire,"Actor-only camera mutation reloads and re-saves byte-identically");
 }
 XeenMonsterTestAccess::markCameraVisit(*loaded.flow);
 // A cached snapshot cannot drop an entire visited camera entry either.
 XeenGameplayServices services{loaded.programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font};
 std::optional<XeenSaveSnapshot> dropped=unvisited;bool wrote=false;refused=false;
 try{xeenSaveGameplay(services,loaded.w,loaded.p,loaded.c,loaded.f,*loaded.flow,{},[](auto &,const auto &,const auto &,const auto &){},nullptr,
  [&](const auto &,const auto &){wrote=true;},&dropped);}catch(const std::logic_error &){refused=true;}
 check(refused && !wrote,"Live preflight refuses an omitted table for a retained camera map");
 const auto path=directory/"guard-mirror.mmsave";developer::generateTestSave(installation,"m92-sixth-mirror",path);
 Fixture ending(in,XeenSaveFile::read(path));XeenMonsterTestAccess::retainDestinationWithoutActors(*ending.flow,28);
 XeenGameplayServices endingServices{ending.programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font};
 ending.flow->writeWon=[](auto) {return true;};wrote=false;
 ending.flow->saveForced=[&](auto *captured){xeenSaveGameplay(endingServices,ending.w,ending.p,ending.c,ending.f,*ending.flow,{},
  [](auto &,const auto &,const auto &,const auto &){},nullptr,[&](const auto &,const auto &){wrote=true;},captured);};
 ending.act(InteractionAction{});
 for(unsigned n=0;n<100 && !XeenMonsterTestAccess::endingSaveFailed(*ending.flow);++n) {
  ending.now+=125;ending.flow->beginCycle(++ending.cycle);
  if(auto image=ending.flow->updatePresentation())ending.present(*image);
  if(ending.flow->presenter().blocksGameplay())ending.act(AcknowledgeAction{});
 }
 check(!wrote && XeenMonsterTestAccess::endingSaveFailed(*ending.flow),"Forced capture refuses a retained destination without actors, even when its entry is empty");
}
void developerEnding(Inputs &in) {
 auto party=XeenPartyLoader().loadInitialParty(in.assets);party.questItems.eventGrant(8);
 XeenCamera camera{78,4,9,XeenDirection::West};XeenGameFlags flags;
 XeenWorld world(in.mapLoader(),in.objectLoader());XeenEventSystem events([&](auto id){return XeenEventScript(in.events.load(id));},[&](auto id){return in.texts.load(id);});
 XeenProgramState program{XeenWallClock::fixed(0)};std::uint64_t now=0;
 XeenEventFlow flow(program,world,events,party,camera,flags,in.font,[](auto){return XeenEventFlow::Composition{frame(),false};},{},[&]{return now;});
 flow.dialogText=&in.assets.uiText();std::vector<std::string> messages;flow.reportText=[&](const auto &text){messages.push_back(text);};
 flow.handle(InteractionAction{});
 const std::string expected="Clouds ending: not supported in this mode";
 for(unsigned n=0;n<100 && std::find(messages.begin(),messages.end(),expected)==messages.end();++n) {now+=125;flow.handle(AcknowledgeAction{});}
 check(std::find(messages.begin(),messages.end(),expected)!=messages.end(),"Non-Journey Sixth Mirror must show its ending notice");
 check(!party.cloudsCompleted && !flags.isSet(75) && camera.mapId==78,"Developer ending does not apply a Journey tail");
 flow.handle(AcknowledgeAction{});check(!flow.blocksGameplay(),"Developer ending notice ends the Event and can be dismissed");
}

XeenSaveSnapshot preset(Inputs &in,const GameInstallation &installation,const fs::path &directory,const char *name) {
 const auto path=directory/(std::string(name)+".mmsave");if(!fs::exists(path))developer::generateTestSave(installation,name,path);
 auto source=XeenSaveFile::read(path);source.soundOptions.efx=source.soundOptions.music=false;return source;
}
// The route includes draw-driven actor countdowns, so both presentation clocks
// use the same fake time. No real-time spin is needed to enter the World Event.
struct WinFixture {
 XeenProgramState programState{XeenWallClock::fixed(0)};
 XeenWorld w;XeenPartyState p;XeenCamera c;XeenGameFlags f;XeenEventSystem events;
 std::unique_ptr<XeenEventFlow> flow;std::uint64_t now=0,cycle=0;
 WinFixture(Inputs &in,const XeenSaveSnapshot &source,XeenWorld::MapLoader loader):w(std::move(loader),in.objectLoader()),
  events([&in](auto id){return XeenEventScript(in.events.load(id));},[&in](auto id){return in.texts.load(id);}) {
  XeenSaveState::restoreBeforeGameplay(XeenSaveFormat::decode(XeenSaveFormat::encode(source)),in.resources(),p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
  flow.reset(new XeenEventFlow(programState,w,events,p,c,f,in.font,[](auto){return XeenEventFlow::Composition{frame(),false};},
   {},[this]{return now;},{},nullptr,[](auto,auto){return XeenEventFlow::Composition{frame(),false};},nullptr,
   [](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{frame(),false};},&in.assets.uiText(),[this]{return now*1000000;}));
  flow->drawDialogSprite=[&in](auto &image,const char *resource,unsigned id,int x,int y){in.assets.drawDialogSprite(image,resource,id,x,y);};
  present(flow->frame());check(flow->canSave(),"Original win preset starts quiet");
 }
 void present(const IndexedFrame &image){if(image.presentation())flow->framePresented(image.presentation());}
 void act(PlayerAction action){flow->beginCycle(++cycle);present(flow->handle(action,flow->displayedInput(),flow->frame().presentation()));}
};
void newWin(Inputs &in,const GameInstallation &installation,const fs::path &directory,bool world,bool retry,bool wonFailure) {
 auto source=preset(in,installation,directory,world?"m106-world":"m106-alamar");
 auto entryCamera=source.camera;if(world)entryCamera.y=15;else entryCamera.direction=XeenDirection::North;
 unsigned castleviewLoads=0;const auto loader=in.mapLoader();
 WinFixture f(in,source,[&](auto id){if(id==XeenMapIdentity(XeenSide::Darkside,29))++castleviewLoads;return loader(id);});std::uint32_t wall=0;f.programState.wallClock=XeenWallClock([&]{return wall;});
 unsigned markers=0,writes=0;bool failureChecked=false, worldInvoked=false;std::vector<std::vector<std::uint8_t>> attempts;
 XeenGameplayServices services{f.programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font};
 const auto path=directory/(world?"world-result.mmsave":"darkside-result.mmsave");
 const auto camera=world?entryCamera:XeenCamera{{XeenSide::Darkside,29},25,21,XeenDirection::North};
 f.flow->writeWon=[&](auto kind) {
  ++markers;check(kind==(world?XeenEndingRequest::World:XeenEndingRequest::Darkside),"Correct won store selected");
  check(!f.p.darksideCompleted && !f.p.worldCompleted,"Won store precedes new completion");
  check(f.p.questItems.at(53)==(world?source.questItems[53]:1),"Darkside quest 53 precedes won store");
  check(save_test::sameCamera(f.c,entryCamera),"Won store precedes move");return !wonFailure;
 };
 std::optional<XeenJourneyRandomState> frozenRandom;std::optional<XeenGameplayContext> frozenContext;
 f.flow->saveForced=[&](auto *captured) {
  check(save_test::sameCamera(f.c,camera) && f.p.darksideCompleted==!world && f.p.worldCompleted==world,"Completion and camera precede forced save");
  check(!XeenSoundOptions(f.p.soundOptions).efx && !XeenSoundOptions(f.p.soundOptions).music,"Forced save precedes sound switches");
  check(world || (!castleviewLoads && !f.w.sessionState().hasRegionalActors({XeenSide::Darkside,29})),"Darkside tail must not load Castleview actors");
  xeenSaveGameplay(services,f.w,f.p,f.c,f.f,*f.flow,path,[](auto &,const auto &,const auto &,const auto &){},nullptr,
   [&](const auto &s,const auto &sourceCheck) {
    sourceCheck();++writes;attempts.push_back(XeenSaveFormat::encode(s));
    if(!frozenRandom){frozenRandom=s.journey->random;frozenContext=s.journey->context;}
    if(retry && writes==1)throw std::runtime_error("Injected M106 forced save failure");
    XeenSaveFile::write(path,s);
   },captured);
 };
 const auto inspectFailure=[&] {
  if(retry && writes==1 && !failureChecked && !f.flow->endingScore()) {
   check(markers==1,"No repeated won store before successful save");wall=50000;failureChecked=true;
  }
 };
 if(world)f.act(NavigationAction::MoveForward);
 else {
  f.act(NavigationAction::TurnLeft);
  for(unsigned n=0;n<100 && !f.flow->canSave();++n) {f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);}
  check(f.flow->canSave(),"Alamar first turn settles before facing North");
  f.act(NavigationAction::TurnLeft);
 }
 inspectFailure();
 for(unsigned n=0;n<1000 && !f.flow->endingScore();++n) {
  f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);inspectFailure();
  if(world && !worldInvoked && f.flow->canSave()) {worldInvoked=true;f.act(InteractionAction{});inspectFailure();}
  const auto input=f.flow->inputContext(f.flow->frame().presentation());
  if(!f.flow->endingScore() && input.dialog && input.dialog->anyKey)f.act(AcknowledgeAction{});
 }
 if(!f.flow->endingScore())std::cerr<<"Unfinished win world="<<world<<" markers="<<markers<<" writes="<<writes<<" pending="<<f.flow->encounter()->state().pending()<<" quiet="<<f.flow->canSave()<<" attributes="<<unsigned(f.w.sampleCell(f.c.mapId,f.c.x,f.c.y)->cell->rawAttributes)<<'\n';
 check(f.flow->endingScore().has_value() && markers==1 && writes==(retry?2:1),"Original win reaches one tail and one successful save");
 if(retry)check(failureChecked && attempts[0]==attempts[1],"Failed save retries exactly the captured bytes");
 const auto saved=XeenSaveFile::read(path);
 check(saved.journey->random==frozenRandom && saved.journey->context==frozenContext,"Retry advances neither Journey RNG nor calendar");
 check(saved.journey->darksideCompleted==!world && saved.journey->worldCompleted==world && !saved.journey->cloudsCompleted,"New completions decoded independently");
 check(save_test::sameCamera(saved.camera,camera) && saved.gameFlags==source.gameFlags && saved.questFlags==source.questFlags,"Win tail preserves flags and uses DOS camera");
 check(saved.questItems[53]==(world?source.questItems[53]:1) && saved.questItems[78]==(world?source.questItems[78]:0),"Win tail quest counters match DOSBox");
 for(unsigned id=0;id<30;++id) {
  const bool active=std::find(source.activeRosterIds.begin(),source.activeRosterIds.end(),id)!=source.activeRosterIds.end();
  check(saved.journey->supplements[id].inputs.experience==source.journey->supplements[id].inputs.experience+(!world && active?5000000u:0u),"Live XP reward exactly once, inactive roster retained");
  auto expected=source.characters[id];if(!world && active)expected.awards[75]=1;
  check(saved.characters[id].awards==expected.awards && xeen_state::sameItemCategory(saved.characters[id].weapons,expected.weapons),"Live award and equipment saved");
 }
 if(!world)check(*f.flow->endingScore()==1992200000u,"Level-200 Alamar score equals DOSBox capture with zero whole play minutes");
 check(*f.flow->endingScore()==xeenCloudsEndingScore(f.p,saved.journey->playTimeSeconds),"M92 formula on saved live party, with failure time excluded");
 Fixture reload(in,saved);
 const auto again=XeenSaveState::capture(in.signature,reload.p,reload.c,reload.f,reload.w);
 save_test::sameSnapshot(saved,again);
 const auto reencoded=XeenSaveFormat::encode(again);
 check(reencoded==attempts.back(),"Original win reload and re-save are byte-identical");
 if(!retry && !wonFailure) {
  // Play goes on from the loaded win: an ordinary step settles (Darkside audit R09).
  const auto from=reload.c;reload.act(NavigationAction::MoveForward);
  for(unsigned n=0;n<1000 && !reload.flow->canSave();++n) {
   reload.now+=125;reload.flow->beginCycle(++reload.cycle);if(auto image=reload.flow->updatePresentation())reload.present(*image);
   const auto input=reload.flow->inputContext(reload.flow->frame().presentation());if(input.dialog && input.dialog->anyKey)reload.act(AcknowledgeAction{});
  }
  check(reload.flow->canSave() && !save_test::sameCamera(reload.c,from),"Loaded win save refused an ordinary step");
 }
 const auto exported=developer::projectDosSave(installation,saved,"M106 result");
 for(const auto &m:exported.darkside)if(m.name=="maze.pty") {
  check(m.bytes[606]==!world && m.bytes[607]==0 && m.bytes[608]==world && m.bytes[609]==0,"DOS PTY 606/608 completion words match capture");
 }
}
void endingMirrors(Inputs &in,const GameInstallation &installation,const fs::path &directory) {
 for(bool clouds:{false,true})for(unsigned ending=0;ending<4;++ending) {
  const char *words[]{"showdown","genesis","showtime","bloopers"};
  auto source=preset(in,installation,directory,clouds?"m106-clouds-mirror":"m106-mirror");
  source.camera.direction=XeenDirection::West;
  // Preserve nonzero completions and a nonunit Cube counter as well as the all-false case.
  for(bool completed:{false,true}) {
   source.journey->cloudsCompleted=source.journey->darksideCompleted=source.journey->worldCompleted=completed;
   source.questItems[53]=7;
   Fixture f(in,source);unsigned markers=0,writes=0;std::optional<XeenSaveSnapshot> saved;
   f.flow->writeWon=[&](auto){++markers;return true;};
   XeenGameplayServices services{f.programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font};
   f.flow->saveForced=[&](auto *captured) {
    xeenSaveGameplay(services,f.w,f.p,f.c,f.f,*f.flow,{},[](auto &,const auto &,const auto &,const auto &){},nullptr,
     [&](const auto &s,const auto &checkSource){checkSource();++writes;saved=s;},captured);
   };
   f.act(InteractionAction{});
   for(unsigned n=0;n<300;++n) {
    const auto context=f.flow->inputContext(f.flow->frame().presentation());
    if(context.dialog && context.dialog->textEntry)break;
    f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);f.act(AcknowledgeAction{});
   }
   check(f.flow->inputContext(f.flow->frame().presentation()).dialog->textEntry,"Original mirror reaches typed prompt");
   // The Vertigo line-0 CallEvent marks game flag 231 before the word is entered.
   if(clouds) {
    const auto pendingFlags=XeenMonsterTestAccess::pendingFlags(*f.flow);
    check(pendingFlags.isSet(231) && !source.gameFlags[231],"Vertigo mirror CallEvent sets flag 231 before ConfirmWord");
   }
   f.act(TextInputAction{words[ending]});f.act(DialogKeyAction{InputKey::Enter});
   for(unsigned n=0;n<100 && !f.flow->endingScore() && !f.flow->canSave();++n) {
    f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);
   }
   check(!markers && writes==(ending==3?0:1),"Every mirror skips won write; endings save once, Bloopers never saves");
   check(save_test::sameCamera(f.c,source.camera) && f.p.questItems.at(53)==7 && f.p.cloudsCompleted==completed &&
    f.p.darksideCompleted==completed && f.p.worldCompleted==completed,"Every mirror skips all win stores and movement");
   if(ending==3)check(!f.flow->endingScore() && f.flow->canSave(),"Bloopers exits Event without running next line");
   else {
    const XeenEndingRequest requests[]{XeenEndingRequest::Showdown,XeenEndingRequest::Genesis,XeenEndingRequest::Showtime};
    check(f.flow->endingRequest()==requests[ending] && f.flow->endingScore().has_value(),"Mirror result requests the matching ending kind");
    check(saved && saved->journey->cloudsCompleted==completed && saved->journey->darksideCompleted==completed && saved->journey->worldCompleted==completed &&
     !saved->soundOptions.efx && !saved->soundOptions.music,"Mirror saved completions and sound precede switches");
    if(clouds) {
     auto pty=in.assets.readInitialResource("maze.pty",XeenSide::Darkside);XeenCharacterFormat::writeDosParty(pty,*saved);
     check(pty[687]==1,"Clouds mirror capture PTY 687: flag 231, MSB-first");
    }
   }
  }
 }
}
void completionCopies(Inputs &in) {
 for(unsigned mask=0;mask<8;++mask) {
  auto source=in.base();source.journey->cloudsCompleted=mask&1;source.journey->darksideCompleted=mask&2;source.journey->worldCompleted=mask&4;
  Fixture f(in,source);
  const auto matches=[&](const auto &p){return p.cloudsCompleted==bool(mask&1) && p.darksideCompleted==bool(mask&2) && p.worldCompleted==bool(mask&4);};
  auto plain=in.resources().loadInitialParty();plain.cloudsCompleted=bool(mask&1);plain.darksideCompleted=bool(mask&2);plain.worldCompleted=bool(mask&4);
  XeenPartyState copy(plain),assigned;assigned=plain;XeenPartyState moved(std::move(copy));copy=std::move(assigned);
  check(matches(moved) && matches(copy),"All completion combinations survive party copy/move");
  auto candidate=f.w.transitionCandidate();candidate->copyEventParty(f.p,assigned);check(matches(assigned),"Detached Event candidate keeps all completions");
  {
   XeenRestoreGuard guard(f.w,f.p,f.c,f.f);XeenRestoreGuard movedGuard(std::move(guard));check(movedGuard.current(),"Moved completion guard current");
  }
  auto live=in.resources().loadInitialParty();live.cloudsCompleted=live.darksideCompleted=live.worldCompleted=true;
  XeenWorld world(in.mapLoader(),in.objectLoader());XeenCamera camera;XeenGameFlags flags;
  XeenSaveState::restoreBeforeGameplay(source,in.resources(),live,camera,flags,world,[](auto &,const auto &,const auto &,const auto &){});
  check(matches(live),"Restore false completions into an already completed party");
  XeenEncounterFlow session(f.programState,world,live,camera,flags,[]{return 0;},XeenJourneyRestoreTag{});
  check(session.prepareJourneyFrame(session.ticket(),[]{}) && session.presentJourney(session.ticket()),"Restored completion capture boundary");
  const auto restored=XeenSaveState::capture(in.signature,live,camera,flags,world);
  check(XeenSaveFormat::encode(restored)==XeenSaveFormat::encode(source),"All completion combinations restore exact bytes");
  auto pty=in.assets.readInitialResource("maze.pty",XeenSide::Darkside);XeenCharacterFormat::writeDosParty(pty,restored);
  check(pty[604]==bool(mask&1) && !pty[605] && pty[606]==bool(mask&2) && !pty[607] && pty[608]==bool(mask&4) && !pty[609],"Every completion combination projects independent DOS words");
 }
 for(unsigned field=0;field<3;++field) {
  Fixture f(in,in.base());XeenRestoreGuard guard(f.w,f.p,f.c,f.f);
  if(field==0)f.p.cloudsCompleted=true;else if(field==1)f.p.darksideCompleted=true;else f.p.worldCompleted=true;
  check(!guard.current(),"Restore guard rejects mutation of each completion");
 }
}
void won(const fs::path &directory) {
 PWSTR value=nullptr;check(SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&value)),"Real local appdata lookup");
 const auto real=fs::path(value)/"MMModern"/"settings.ini";CoTaskMemFree(value);
 const bool existed=fs::exists(real);const auto original=bytes(real);
 const auto path=directory/"settings.ini";
 check(xeenSettingsFile() && *xeenSettingsFile()!=real,"CTest child processes inherit a private settings override");
 check(!xeenReadWon(path),"Missing means not won");
 for(const char *malformed:{"", "won=yes\n", "won=1\nscore=42\n", "won=1\nwon=0\n", "won=0\n"}) {
  {std::ofstream out(path);out<<malformed;}check(!xeenReadWon(path),"Malformed or false means not won");
 }
 {std::ofstream out(path);out<<"won=1\n";}check(xeenReadSettings(path).won && !xeenReadSettings(path).darksideIntroSeen,"M92 settings");
 check(xeenWriteSeen(path,true) && xeenReadSettings(path).won && xeenReadSettings(path).darksideIntroSeen,"Seen preserves win");
 check(xeenWriteWon(path) && bytes(path)=="won=1\ndarkside_intro_seen=1\ndarkside_won=0\nworld_won=0\n" && xeenReadWon(path) && !fs::exists(path.string()+".tmp"),"Atomic settings publication");
 check(xeenWriteSeen(path,false) && xeenReadWon(path) && !xeenReadSettings(path).darksideIntroSeen,"Clear seen preserves win");
 check(xeenWriteSeen(path,true),"Restore seen");
 const auto handle=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 check(handle!=INVALID_HANDLE_VALUE,"Lock settings replacement");
 check(!xeenWriteWon(path) && bytes(path)=="won=1\ndarkside_intro_seen=1\ndarkside_won=0\nworld_won=0\n" && !fs::exists(path.string()+".tmp"),"Failed publication preserves old complete file and cleans temporary");
 CloseHandle(handle);check(xeenWriteWon(path),"Atomic replacement of an existing file");
 for(unsigned mask=0;mask<16;++mask) {
  const auto seed=[&] {std::ofstream out(path);out<<"world_won="<<bool(mask&4)<<"\nwon="<<bool(mask&1)<<"\ndarkside_won="<<bool(mask&2)<<"\ndarkside_intro_seen="<<bool(mask&8)<<"\n";};
  const auto matches=[&](unsigned expected) {
   const auto s=xeenReadSettings(path);return s.won==bool(expected&1) && s.darksideWon==bool(expected&2) && s.worldWon==bool(expected&4) && s.darksideIntroSeen==bool(expected&8);
  };
  seed();check(matches(mask),"Four-key reader accepts every independent settings combination and key order");
  const XeenWonKind kinds[]{XeenWonKind::Clouds,XeenWonKind::Darkside,XeenWonKind::World};
  for(unsigned kind=0;kind<3;++kind) {
   seed();check(xeenWriteWon(path,kinds[kind]) && matches(mask|(1u<<kind)),"Each win preserves every other win and seen value");
   seed();const auto before=bytes(path);
   const auto locked=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
   check(locked!=INVALID_HANDLE_VALUE,"Lock each settings combination");
   const bool failed=!xeenWriteWon(path,kinds[kind]) && !xeenWriteSeen(path,!bool(mask&8));
   CloseHandle(locked);
   check(failed && bytes(path)==before && matches(mask) && !fs::exists(path.string()+".tmp"),"Failed win/intro replacement retains every earlier value and cleans temporary");
  }
  for(bool seen:{false,true}) {seed();check(xeenWriteSeen(path,seen) && matches((mask&7)|(seen?8:0)),"Intro D/seen writes preserve all three wins");}
 }
 for(bool win:{false,true})for(bool seen:{false,true}) {
  {std::ofstream out(path,std::ios::binary);out<<"darkside_intro_seen="<<seen<<"\r\nwon="<<win<<"\r\n";}
  auto s=xeenReadSettings(path);check(s.won==win && s.darksideIntroSeen==seen && !s.darksideWon && !s.worldWon,"Legacy two-key settings default new wins false");
 }
 const auto blocker=directory/"blocker";{std::ofstream out(blocker);out<<"file";}
 check(!xeenWriteWon(blocker/"settings.ini"),"Directory/write failure is not won");
 check(fs::exists(real)==existed && bytes(real)==original,"Player's real settings untouched");
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"usage: ending-tests <installation>");Directory directory;
 const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation");Inputs in(*installation);
 for(unsigned opcode:{57u,58u})for(bool failedRewrite:{false,true})applicationEnding(in,directory.path,true,failedRewrite,opcode);
 completionCopies(in);
 for(bool world:{false,true}) {newWin(in,*installation,directory.path,world,false,false);newWin(in,*installation,directory.path,world,true,false);newWin(in,*installation,directory.path,world,false,true);newWin(in,*installation,directory.path,world,true,true);}
 endingMirrors(in,*installation,directory.path);
 score(in);tail(in,directory.path,false);tail(in,directory.path,true);tail(in,directory.path,false,true);postWorld(in);mirrors(in,directory.path);applicationEnding(in,directory.path);applicationEnding(in,directory.path,false);applicationEnding(in,directory.path,true,true);omittedTableGuards(in,*installation,directory.path);developerEnding(in);originalAudioRecovery(in,*installation,directory.path);won(directory.path);
 std::cout<<"Clouds ending score, tail, save recovery, won store and post-world controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
