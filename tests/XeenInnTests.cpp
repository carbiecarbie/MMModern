#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "app/XeenGameplayServices.h"
#include "app/Application.h"
#include "app/XeenAudio.h"
#include "games/xeen/XeenEventTrigger.h"
#include <chrono>
#include <fstream>
#include <thread>
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/CloudsUiComposer.h"
#include "formats/xeen/XeenVoc.h"
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenInnTestAccess {
 static std::optional<unsigned> woundAndStage(XeenEventFlow &f) {
  auto &e=*f._encounter;auto &actors=e._world._sessionState.mutableMonsters(e._camera.mapId);
  unsigned index=0;for(;index<actors.size();++index)if(actors[index].lifecycle==XeenActorLifecycle::Present && actors[index].statistics)break;
  if(index<actors.size()){actors[index].hp=1;actors[index].status=XeenActorStatus::Sleep;}
  auto pending=e._world.treasureStaging().value();pending.gold=17;pending.gems=8;pending.ready=true;pending.weapons[0]={0,1,0,0};
  e._world.treasureStaging().publish(pending);e.retainJourney();f._encounterFrame=e.ticket();return index<actors.size()?std::optional<unsigned>{index}:std::nullopt;
 }
 static void recordAdmission(XeenEventFlow &f) {
  auto &e=*f._encounter;const auto ticket=e.ticket();auto ids=std::vector<std::uint8_t>(e._party.party.activeRosterIds());
  auto bad=xeenDeletedCombatInputs();bad.luck.reset();bool refused=false;
  try{e.innRecord(ticket,xeenDeletedCharacter(2),bad,ids);}catch(const std::invalid_argument &){refused=true;}
  check(refused && e.current(ticket),"invalid detached supplement never publishes");
  check(e.innRecord(ticket,e._party.roster.at(2),*e._party.roster.combatInputs(2),ids),"complete record operation admitted");
  check(!e.innRecord(ticket,xeenDeletedCharacter(2),xeenDeletedCombatInputs(),ids),"stale record ticket rejected");
  f._encounterFrame=e.ticket();
 }
 static void callbackAdmission(XeenEventFlow &f) {
  auto &e=*f._encounter;const auto ticket=e.ticket();auto ids=std::vector<std::uint8_t>(e._party.party.activeRosterIds());
  const auto before=std::string(e._party.roster.at(0).name);bool nested=true,rejected=false;
  try{rejected=!e.prepareJourneyFrame(ticket,[&]{
   nested=e.innRecord(ticket,xeenDeletedCharacter(0),xeenDeletedCombatInputs(),{});
   e._party.roster.at(1).birthDay=7;
  });}catch(const std::exception &){rejected=true;}
  check(!nested && rejected && e._party.roster.at(0).name==before && e._party.party.activeRosterIds()==ids,
   "callback record reentry and mutation rejected without record or membership publication");
 }
 static bool forcedSaveWith(XeenEventFlow &f,bool inside,const XeenCamera &camera) {
  // Refresh the retained preimage over the probed owners, so only capture admission differs.
  auto &e=*f._encounter;const auto door=e._camera;const auto edit=f._innUi->edit;
  e._party.insideInn=inside;e._camera=camera;f._innUi->edit=XeenEventFlow::InnUi::Edit::SavePending;e.retainJourney();
  bool saved=true;try{f.forcedInnSave();}catch(const std::exception &){saved=false;}
  e._party.insideInn=true;e._camera=door;f._innUi->edit=edit;e.retainJourney();f._encounterFrame=e.ticket();
  return saved;
 }
 static IndexedFrame refresh(XeenEventFlow &f){return f.renderInn();}
 static std::size_t top(const XeenEventFlow &f){return f._innUi->top;}
 static bool menu(const XeenEventFlow &f){return f._innUi && f._innUi->phase==XeenEventFlow::InnUi::Phase::Menu;}
 static unsigned phase(const XeenEventFlow &f){return unsigned(f._innUi->phase);}
 static unsigned scroll(const XeenEventFlow &f){return f._innUi->scroll;}
 static bool returned(const XeenEventFlow &f){return !f._innUi && !f._innReturnPassPending && !f._pending && !f._handoffPending && !f._explorationPass && f.encounterFrameCurrent();}
 static std::uint64_t deadline(const XeenEventFlow &f){return f._innUi->deadline;}
 static const auto &error(const XeenEventFlow &f){return f._dialogError;}
 static auto member(const XeenEventFlow &f){return f._inventory.source;}
 static auto &party(XeenEventFlow &f){return f._party;}
 static const auto &closing(const XeenEventFlow &f){return f._innUi->closing;}
 static IndexedFrame syntheticLodging(XeenEventFlow &f) {
  auto &e=*f._encounter;const auto owner=e._party.party.activeRosterIds().back();
  e._party.roster.at(owner).savedMaze=561;e._party.party=e._party.party.dismissed(e._party.party.size()-1);
  e._party.firstSerializedCount=e._party.effectiveSerializedCount=e._party.party.size();
  e.retainJourney();f._encounterFrame=e.ticket();return f.renderEncounter();
 }
 static IndexedFrame pendingExitTail(XeenEventFlow &f) {
  auto &e=*f._encounter;
  // Exercise Exit independently after Sign In has already consumed its tail.
  const auto owner=e._party.party.activeRosterIds().back();e._party.roster.at(owner).savedMaze=561;
  e._party.party=e._party.party.dismissed(e._party.party.size()-1);
  e._party.firstSerializedCount=e._party.effectiveSerializedCount=e._party.party.size();
  e.retainJourney();f._encounterFrame=e.ticket();return f.renderInn();
 }
 static void stockObserver(XeenEventFlow &f,unsigned &calls) {
  auto &e=*f._encounter;
  f.smithBoundary=[&e,&calls](auto boundary){if(boundary==XeenSmithBoundary::StockComplete) {
   const auto &day=*e._smith->departure;
   check(day.context().day==11 && day.economy().bank==day.beforeEconomy().bank,"stock completes before day-11 interest");++calls;
  }};
 }
 static void rejectUnrelatedLease(XeenEventFlow &f) {
  auto &e=*f._encounter;const auto lease=e._boundary.hold(XeenCombatBoundary::Work::Inventory);
  bool refused=false;try{f.forcedInnSave();}catch(const std::logic_error &){refused=true;}
  e._boundary.release(XeenCombatBoundary::Work::Inventory,lease);
  f._encounterFrame=e.ticket(); // Retire only the synthetic test lease, not the save capture.
  check(refused,"forced Inn Save refuses unrelated work before adopting generation");
 }
 static std::string debug(const XeenEventFlow &f){return " ui="+std::to_string(bool(f._smithUi))+" phase="+(f._smithUi?std::to_string(unsigned(f._smithUi->phase)):"none")+" feedback="+(f._smithUi?f._smithUi->feedback:"")+" settlement="+std::to_string(f._smithSettlement)+" pending="+std::to_string(bool(f._pending))+" handoff="+std::to_string(f._handoffPending)+" frame="+std::to_string(f.encounterFrameCurrent());}
};
}
namespace {
std::filesystem::path preview;
void rules() {
    XeenPartyState p;
    for(unsigned id=0;id<30;++id){auto &c=p.roster.at(id);c.name="Member";c.savedMaze=28;c.xeenSide=0;}
    p.roster.at(2).savedMaze=30;p.roster.at(4).xeenSide=1;p.roster.at(5).name.clear();
    const auto list=xeenInnList(p,28);
    check(list.size()==21 && list[0]==0 && list[1]==1 && list[2]==3 && list.back()==23,"town, side, named, first-24 list in ID order");
    check(xeenInnList(p,30)==std::vector<std::uint8_t>{2},"other town list");
    const auto townOnly=xeenInnList(p,28,true);
    check(townOnly.size()==22 && townOnly[2]==3 && townOnly[3]==4,"post-Delete rebuild tests town without side");
    check(xeenInnLastPage(0)==0 && xeenInnLastPage(4)==0 && xeenInnLastPage(5)==4 && xeenInnLastPage(24)==20,"four-face last page");
    const auto input=xeenInnInput(2,3);
    check(input.key('1') && input.key('2') && !input.key('3') && !input.key(InputKey::Enter) && !input.key(InputKey::F1+3),"empty rows, Enter, inactive F-keys ignored");
    check(input.click(16,16) && input.click(117,16) && !input.click(16,59) && !input.click(117,59),"missing face is unclickable");
    for(auto key:{unsigned('e'),unsigned('x'),InputKey::Escape,InputKey::Space})check(bool(input.key(key)),"all Inn Exit keys");
    const auto remove=xeenInnInput(4,1,true);
    check(remove.click(225,84) && !remove.click(249,84) && !remove.key('1') && !remove.key(InputKey::F1+1),"Remove prompt escape bounds and active selection");
}
struct Visit:Fixture {
    std::vector<XeenSaveSnapshot> saves;
    XeenGameplayContext beforeSignIn;
    Visit(Inputs &in,const XeenSaveSnapshot &source):Fixture(in,source) {
        flow->drawTavernArt=[&in](auto &image){in.assets.drawTavern(image);};
        flow->tavernFoodOffer=[&in](unsigned n){return in.assets.tavernFoodOffer(n);};
        flow->tavernString=[&in](std::size_t n){return in.assets.tavernString(n);};
        flow->loadSampleDuration=[&in](const char *n,XeenSceneArchive side){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(n,side));};
        flow->loadInnScreen=[&in](const XeenPartyState &p){
            in.assets.loadPalette("mm4.pal");in.assets.loadRawFramebuffer("back.raw");
            CloudsUiComposer::drawBorder(p,[&](const char *n,unsigned f,int x,int y){in.assets.drawSprite(n,f,x,y);});
            in.assets.drawSprite("restorex.icn",0,8,149);
            for(const auto &face:CloudsUiComposer::buildPortraitPlacements(p))in.assets.drawSprite(face.resourceName,face.frame,face.x,face.y);
            return in.assets.snapshot();
        };
        flow->saveInn=[this,&in]{
            XeenGameplayServices services{programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font,{},{},{}};
            xeenSaveGameplay(services,w,p,c,f,*flow,{},[](auto &,const auto &,const auto &,const auto &){},nullptr,
                [this](const auto &snapshot,const auto &check){check();saves.push_back(snapshot);});
        };
    }
    void pulse(){now+=100;flow->beginCycle(++cycle);if(auto frame=flow->updatePresentation())present(*frame);}
    auto dialog(){return flow->inputContext(flow->frame().presentation()).dialog;}
    void key(unsigned k){act(DialogKeyAction{k});}
    void wait(const std::function<bool()> &done,const char *message){for(unsigned n=0;n<1500 && !done();++n)pulse();if(!done())throw std::runtime_error(std::string(message)+" inn="+std::to_string(bool(p.insideInn))+" saves="+std::to_string(saves.size())+" camera="+std::to_string(c.mapId.number)+":"+std::to_string(c.x)+","+std::to_string(c.y)+" activity="+std::to_string(unsigned(w.sessionState().journeyActivity()))+XeenInnTestAccess::debug(*flow));}
    void sign(bool door=false){if(door) {act(NavigationAction::MoveForward);wait([&]{return flow->canSave() || (dialog() && bool(dialog()->key('s')));},"arrival before Tavern interaction");if(!dialog() || !dialog()->key('s'))act(InteractionAction{});}else act(InteractionAction{});wait([&]{return dialog() && bool(dialog()->key('s'));},"Tavern lobby");beforeSignIn=*p.encounterContext;key('s');wait([&]{return XeenInnTestAccess::menu(*flow);},"Sign In reaches Inn");}
};
XeenSaveSnapshot source(Inputs &in,unsigned town,unsigned day=8,unsigned minutes=600) {
    auto s=in.service(day);s.camera={town,town==49?14:24,town==28?5:town==30?16:13,XeenDirection::North};
    const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,town),in.statistics);
    auto &table=s.map(town).monsters;table.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
    for(const auto &a:actors)table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
    s.journey->context->minutes=minutes;return s;
}
void signIn(Inputs &in,unsigned town,unsigned minutes) {
    auto s=source(in,town,10,minutes);
    if(minutes==300)s.camera={town,town==49?14:24,town==28?4:town==30?15:12,XeenDirection::North};
    s.journey->context->newDay=false;
    s.food=75;s.journey->context->effects[XeenPartyEffect::Bless]=3;
    s.journey->serviceEconomy->bank.gold=10000;s.journey->serviceEconomy->bank.gems=100;
    s.characters[18].conditions[13]=1;s.characters[14].conditions[14]=1;s.characters[11].conditions[15]=1;
    Visit v(in,s);unsigned stockCalls=0;XeenInnTestAccess::stockObserver(*v.flow,stockCalls);v.sign(minutes==300);
    check(v.saves.size()==1 && v.saves[0].insideInn && v.saves[0].activeRosterIds==s.activeRosterIds,"Sign In saves pre-Inn membership exactly once");
    const auto saved=v.saves[0];
    check(saved.camera.mapId==XeenMapIdentity(town) && saved.camera.x==(town==28?18:town==30?19:11) && saved.camera.y==(town==28?4:town==30?16:12) && saved.camera.direction==XeenDirection::West,"DOS exit including Shangri-La correction");
    for(auto id:s.activeRosterIds)check(saved.characters[id].savedMaze==town && saved.characters[id].xeenSide==0,"Sign In lodging");
    check(saved.journey->context->day==11 && saved.journey->context->minutes==v.beforeSignIn.minutes && saved.journey->context->newDay==(v.beforeSignIn.minutes<300),"Sign In calendar and dawn latch");
    check(saved.journey->serviceEconomy->bank.gold==10100 && saved.journey->serviceEconomy->bank.gems==101,"Sign In merchant-day interest");
    check(stockCalls==1 && saved.journey->serviceEconomy->wares!=s.journey->serviceEconomy->wares,"Sign In regenerates merchants before interest on day 11");
    check(saved.food==s.food && saved.journey->context->effects==s.journey->context->effects,"no food or dawn clears");
    for(auto id:s.activeRosterIds)check(saved.characters[id].conditions==s.characters[id].conditions && saved.characters[id].currentHp==s.characters[id].currentHp && saved.characters[id].currentSp==s.characters[id].currentSp,"no condition ticks or recovery");
    check(!v.flow->canSave() && v.programState.innSongStarted,"Sign In starts the Inn song flag");
    if(town==28 && minutes==300 && !preview.empty()) {
        const auto &image=v.flow->frame();std::ofstream out(preview,std::ios::binary);out<<"P6\n320 200\n255\n";
        for(auto pixel:image.pixels)for(unsigned channel=0;channel<3;++channel)out.put(char(image.palette[pixel*3+channel]));
    }
    v.key(InputKey::Enter);v.key(InputKey::F1+6);check(v.saves.size()==1,"unmatched input does not save");
    for(unsigned member=0;member<6;++member) {
        v.key(InputKey::F1+member);check(v.flow->inventoryOpen(),"Inn active F1-F6 sheet");v.key(InputKey::Escape);check(!v.flow->inventoryOpen(),"sheet returns to Inn");
    }
    v.key('r');v.key(InputKey::Escape);check(v.p.party.size()==6,"Remove Escape cancels");
    v.key('c');check(v.dialog()->anyKey,"Create unsupported notice");v.act(AcknowledgeAction{});
    v.key('d');check(v.dialog()->key('1') && !v.dialog()->key(InputKey::F1),"Delete selection excludes F-keys");v.key(InputKey::Escape);
    v.key('1');check(v.p.party.size()==6 && !v.dialog()->anyKey,"duplicate Add silently ignored before full check");
    for(unsigned n=0;n<6 && v.p.party.size();++n) {v.key('r');v.key(InputKey::F1);}
    check(v.p.party.size()==0,"Remove did not compact to zero");
    check(v.saves.size()==1,"Remove never saves");v.key('x');check(v.dialog()->anyKey && v.flow->innOpen(),"empty-party Exit refused");v.act(AcknowledgeAction{});
    v.key('r');check(v.p.party.size()==0 && !v.dialog()->key(InputKey::F1),"Remove is ignored with zero members");
    v.key('1');v.key('2');check(v.p.party.size()==2,"Add from zero appends");
    v.key(InputKey::F1+2);check(!v.flow->inventoryOpen(),"inactive F3 ignored with two members");
    const auto wounded=XeenInnTestAccess::woundAndStage(*v.flow);
    const auto loadGeneration=v.w.inputMapLoadGeneration();unsigned doorCalls=0;
    v.flow->reportManual=[&](const auto &result){if(std::holds_alternative<XeenManualEventCompleted>(result))++doorCalls;};
    const auto cell=v.w.sampleCell(v.c.mapId,v.c.x,v.c.y);const bool flagged=cell && (cell->cell->rawAttributes&kXeenAutomaticEventFlag);
    v.key('x');v.wait([&]{return !v.flow->innOpen() && v.flow->canSave();},"Inn Exit returns to ordinary exploration");
    check(v.saves.size()==2 && !v.saves.back().insideInn && v.saves.back().activeRosterIds.size()==2,"Exit saves edited members before gameplay");
    check(!v.programState.innSongStarted,"Inn Exit clears song flag");
    check(v.w.inputMapLoadGeneration()>loadGeneration,"Exit unconditionally reloads the same map");
    if(wounded) {
        const auto &actor=v.w.sessionState().regionalActors(town).at(*wounded);
        check(actor.hp==actor.statistics->baseHp(),"same-map reload rebuilds monster HP");
    }
    check(doorCalls==unsigned(flagged),"return makes one ordinary flagged-cell Event call at the door");
    check(!v.w.treasureStaging().value().pending() && v.w.treasureStaging().value().weapons[0].id==1,"gameplay entry clears pending totals/readiness and retains stale array bytes");
    Visit load(in,saved);const auto before=*load.p.encounterContext;
    load.present(load.flow->openInn());load.wait([&]{return XeenInnTestAccess::menu(*load.flow);},"Load opens Inn");
    check(*load.p.encounterContext==before && load.saves.empty(),"Load adds no day and makes no Sign In Save");
}
void deleteRecords(Inputs &in,const GameInstallation &installation) {
    auto s=source(in,28);s.characters[2]=s.characters[6];s.characters[2].rosterId=2;s.characters[2].savedMaze=28;
    s.journey->supplements[2].inputs=s.journey->supplements[6].inputs;
    Visit v(in,s);v.sign();XeenInnTestAccess::recordAdmission(*v.flow);
    v.present(XeenInnTestAccess::refresh(*v.flow));
    const auto before=v.saves.back();
    v.key('d');v.key(InputKey::F1);v.act(SelectMemberAction{0});
    check(!v.flow->inventoryOpen() && v.p.roster.at(0).name==s.characters[0].name,"F-key and party portraits do not select Delete");
    check(v.dialog()->click(225,120) && !v.dialog()->click(249,120),"Delete escape button bounds");v.key(InputKey::Escape);
    v.key('d');v.key('3');
    check(v.dialog()->key('y') && !v.dialog()->key(InputKey::Enter) && !v.dialog()->key(InputKey::Space),"Y only confirmation");
    check(std::get<DialogKeyAction>(*v.dialog()->click(129,112)).key=='y' && std::get<DialogKeyAction>(*v.dialog()->click(185,112)).key=='n',"original confirm buttons");
    v.key(InputKey::Enter);v.key(InputKey::Space);v.key('n');check(v.saves.size()==1,"cancel and unmatched confirmation never save");
    v.key('d');v.key('3');v.key(InputKey::Escape);check(v.p.roster.at(2).name==s.characters[2].name,"confirmation Escape cancels");
    unsigned attempts=0;const auto save=v.flow->saveInn;
    v.flow->saveInn=[&]{++attempts;if(attempts==1)throw std::runtime_error("Windows error 112");save();};
    v.key('d');v.key('3');v.key('y');
    check(v.p.roster.at(2).name.empty() && attempts==1 && v.saves.size()==1,"lodged Delete publishes before failed write");
    const auto cleared=v.p.roster.at(2);const auto members=v.p.party.activeRosterIds();
    v.key(InputKey::Enter);check(attempts==2 && v.saves.size()==2 && v.p.party.activeRosterIds()==members,"retry writes once without membership replay");
    remove_test::checkSameCharacter(cleared,v.saves.back().characters[2]);
    remove_test::checkSameCharacter(xeenDeletedCharacter(2),v.saves.back().characters[2]);
    const auto originalRoster=XeenCharacterFormat::parseRoster(in.chr);
    const auto &empty=originalRoster.at(3);
    auto originalEmpty=empty;originalEmpty.rosterId=2;originalEmpty.learnedSpells=XeenCharacterFormat::parseLearnedSpells(in.chr,3);
    remove_test::checkSameCharacter(originalEmpty,v.saves.back().characters[2]);
    check(save_test::sameInputs(*v.p.roster.combatInputs(2),XeenCharacterFormat::parseCombatInputs(in.chr,3,true,true,true)),"deleted supplement equals original empty record");
    check(v.saves.back().journey->context==before.journey->context && v.saves.back().journey->random==before.journey->random,"Delete no time or RNG");
    // Deleting the last two faces on page four keeps that now-empty page.
    v.key(InputKey::Down);check(XeenInnTestAccess::top(*v.flow)==4,"last page");
    v.key('d');v.key('2');v.key('y');v.key('d');v.key('1');v.key('y');
    check(XeenInnTestAccess::top(*v.flow)==4 && !v.dialog()->key('1'),"last face Delete keeps empty last page");
    v.key(InputKey::Down);check(XeenInnTestAccess::top(*v.flow)==4,"Down does not clamp the kept page");
    v.key(InputKey::Up);
    while(v.p.party.size()) {v.key('d');v.key('1');v.key('y');}
    check(v.saves.back().activeRosterIds.empty() && v.p.insideInn,"last active member deleted and saved");
    const auto saved=v.saves.back();save_test::sameSnapshot(saved,XeenSaveFormat::decode(XeenSaveFormat::encode(saved)));
    v.key('d');check(!v.dialog()->key('1') && v.dialog()->key('x'),"empty list D ignored");
    Visit load(in,saved);const auto day=*load.p.encounterContext;load.present(load.flow->openInn());load.wait([&]{return XeenInnTestAccess::menu(*load.flow);},"empty Load reopens Inn");
    check(load.p.party.size()==0 && *load.p.encounterContext==day && load.saves.empty() && load.p.party.dismissedRosterIds().empty(),"empty Load replays no Sign In, time or Dismiss");
    load.key('x');check(load.dialog()->anyKey,"empty Load Exit refused");
    auto sword=s;sword.characters[0].weapons[8].id=34;sword.characters[0].weapons[8].state=255;
    Visit protectedVisit(in,sword);protectedVisit.programState.audio=XeenAudio::installed(installation,in.assets,0,false);
    std::vector<unsigned> effects;protectedVisit.programState.audio->effectSink([&](unsigned effect){effects.push_back(effect);});
    protectedVisit.sign();effects.clear();protectedVisit.key('d');protectedVisit.key('1');
    check(protectedVisit.dialog()->anyKey && protectedVisit.p.roster.at(0).name==sword.characters[0].name && protectedVisit.saves.size()==1 && effects.empty(),"Slayer Sword in any state refuses without a sound");
    protectedVisit.act(AcknowledgeAction{});protectedVisit.key('3');
    check(effects==std::vector<unsigned>{21},"the same effect sink records the full-party Add");protectedVisit.act(AcknowledgeAction{});effects.clear();
    protectedVisit.key('d');protectedVisit.key('3');protectedVisit.key('y');
    check(protectedVisit.p.roster.at(2).name.empty() && protectedVisit.saves.size()==2 && effects.empty(),"confirmed Delete makes no sound");
    Visit callback(in,s);callback.sign();XeenInnTestAccess::callbackAdmission(*callback.flow);
}

void emptySaveRecovery(Inputs &in) {
    Visit v(in,source(in,28));v.sign();
    while(v.p.party.size()){v.key('r');v.key(InputKey::F1);}
    v.key('d');v.key('1');v.key('y');const auto saved=v.saves.back();
    check(saved.activeRosterIds.empty(),"Delete captures prior Remove-to-zero");
    const auto written=v.saves.size();auto wrong=v.c;++wrong.y;
    check(!XeenInnTestAccess::forcedSaveWith(*v.flow,false,v.c) && !XeenInnTestAccess::forcedSaveWith(*v.flow,true,wrong) && v.saves.size()==written,
        "forced Inn save refuses zero members outside the Inn and at a wrong door");
    check(XeenInnTestAccess::forcedSaveWith(*v.flow,true,v.c) && v.saves.size()==written+1,"the same empty forced save is admitted at the Inn door");
    bool yellow=false;drawXeenInn(in.assets.uiText(),frame(),in.font,v.p,xeenInnList(v.p,28),0,
        [&](auto &,const char *name,unsigned index,int x,int y){if(std::string(name)=="global.icn" && index==8 && x==9 && y==149)yellow=true;});
    check(yellow,"empty Inn retains yellow first-slot selection frame");
    for(unsigned bad=0;bad<7;++bad) {
        auto invalid=saved;
        if(bad==0)invalid.insideInn=false;
        if(bad==1)invalid.camera={28,19,4,XeenDirection::West};
        if(bad==2)invalid.camera={30,19,15,XeenDirection::West};
        if(bad==3)invalid.camera={49,11,12,XeenDirection::North};
        if(bad==4)invalid.journey->supplements[0].inputs.luck.reset();
        if(bad==5)invalid.journey->supplements[0].inputs.resistances.reset();
        if(bad==6)invalid.journey->supplements[0].inputs.poisonResistance.reset();
        XeenPartyState target;XeenCamera camera{23,1,2,XeenDirection::East};XeenGameFlags flags;
        XeenWorld world(in.mapLoader(),in.objectLoader());unsigned callbacks=0;auto resources=in.resources();
        const auto initial=resources.loadInitialParty;resources.loadInitialParty=[&]{++callbacks;return initial();};
        save_test::rejects([&]{XeenSaveState::restoreBeforeGameplay(invalid,resources,target,camera,flags,world,
            [&](auto &,const auto &,const auto &,const auto &){++callbacks;});});
        check(callbacks==0 && target.party.size()==0 && target.roster.at(0).name.empty() &&
            camera.mapId==XeenMapIdentity(23) && camera.x==1 && camera.y==2 && camera.direction==XeenDirection::East,
            "invalid empty save rejected before callbacks or partial owner publication");
    }
    Visit load(in,XeenSaveFormat::decode(XeenSaveFormat::encode(saved)));load.present(load.flow->openInn());load.wait([&]{return XeenInnTestAccess::menu(*load.flow);},"empty recovery Load");
    load.key('x');check(load.dialog()->anyKey,"empty recovery Exit refusal");load.act(AcknowledgeAction{});
    load.key('1');check(load.p.party.size()==1 && load.saves.empty(),"Add recovers empty Inn without saving or replaying Sign In");
    load.key('x');load.wait([&]{return !load.flow->innOpen() && load.flow->canSave();},"recovered Exit");
    check(load.saves.size()==1 && !load.saves.back().insideInn,"recovered Exit saves");
}

void fullAndFailures(Inputs &in,const GameInstallation &installation) {
    auto s=source(in,28,8,600);s.characters[2]=s.characters[6];s.characters[2].rosterId=2;
    s.journey->supplements[2].inputs=s.journey->supplements[6].inputs;
    Visit v(in,s);v.programState.audio=XeenAudio::installed(installation,in.assets,0,false);
    std::vector<unsigned> effects;v.programState.audio->effectSink([&](unsigned effect){effects.push_back(effect);});
    unsigned attempts=0;const auto saving=v.flow->saveInn;
    v.flow->saveInn=[&]{++attempts;if(attempts==1)throw std::runtime_error("Windows error 112");saving();};
    v.act(InteractionAction{});v.wait([&]{return v.dialog() && bool(v.dialog()->key('s'));},"failure fixture Tavern");v.key('s');
    v.wait([&]{return attempts==1;},"Sign In write attempted");const auto day=*v.p.encounterContext;
    for(unsigned n=0;n<20;++n)v.pulse();check(attempts==1 && v.saves.empty(),"failed save waits for player retry");
    XeenInnTestAccess::rejectUnrelatedLease(*v.flow);check(attempts==1,"unrelated lease never reaches the writer");
    v.key(InputKey::Enter);v.wait([&]{return XeenInnTestAccess::menu(*v.flow);},"Sign In retry opens Inn");
    check(*v.p.encounterContext==day && v.saves.size()==1,"retry never repeats the day");
    check(*v.saves[0].journey->serviceEconomy==*s.journey->serviceEconomy,"nontriggering Sign In preserves all merchant stock and bank balances");
    v.key(InputKey::Down);check(XeenInnTestAccess::top(*v.flow)==4,"Down pages four");
    v.key(InputKey::Down);check(XeenInnTestAccess::top(*v.flow)==4 && !v.dialog()->key('4'),"last page clamps and missing fourth face is disabled");
    v.key(InputKey::Up);check(XeenInnTestAccess::top(*v.flow)==0,"Up pages four");
    effects.clear();v.key('1');check(effects.empty() && !v.dialog()->anyKey,"duplicate Add is silent even at capacity");
    v.key('3');check(v.dialog()->anyKey && effects==std::vector<unsigned>{21},"full-party Add says full with FX21");v.act(AcknowledgeAction{});
    const auto written=XeenSaveFormat::encode(v.saves[0]);
    v.key('r');v.key(InputKey::F1);check(v.saves.size()==1,"Remove leaves written Sign In membership intact");
    v.flow->saveInn=[&]{throw std::runtime_error("Exit disk error");};v.key('x');
    check(v.flow->innOpen() && v.p.insideInn && v.dialog()->anyKey && v.saves.size()==1,"Exit write failure stays in Inn");
    v.act(AcknowledgeAction{});v.flow->saveInn=saving;v.key('x');v.wait([&]{return v.flow->canSave();},"Exit retry returns");
    check(v.saves.size()==2 && XeenSaveFormat::encode(v.saves[0])==written && v.saves[1].activeRosterIds.size()==5,"Exit retry saves once without changing Sign In file image");
}
void innItems(Inputs &in) {
    auto s=source(in,28);s.insideInn=true;s.camera={28,18,4,XeenDirection::West};
    // A charged Town Portal item would invalidate the retained door if admitted.
    s.characters[s.activeRosterIds[0]].miscellaneous[0]={1,55,3,0};
    Visit v(in,s);v.present(v.flow->openInn());v.wait([&]{return XeenInnTestAccess::menu(*v.flow);},"item fixture opens Inn");
    v.key(InputKey::F1);
    for(unsigned member=0;member<6;++member){v.key(InputKey::F1+member);check(XeenInnTestAccess::member(*v.flow)==member,"Inn character sheet permits active member switching");}
    v.key(InputKey::F1);v.key('i');
    for(unsigned member=0;member<6;++member){v.key(InputKey::F1+member);check(XeenInnTestAccess::member(*v.flow)==0,"Inn Items refuses all F-keys with no selection");}
    v.key('m');v.key('1');
    const auto characters=v.p.roster.characters();const auto camera=v.c;const auto context=*v.p.encounterContext;
    const auto random=v.w.sessionState().journeyRandom();
    const auto unchanged=[&]{
        for(unsigned id=0;id<30;++id)check(xeen_state::sameCharacter(characters[id],v.p.roster.at(id)),"Inn item refusal preserves roster");
        check(v.p.insideInn && v.c.mapId==camera.mapId && v.c.x==camera.x && v.c.y==camera.y && v.c.direction==camera.direction &&
            *v.p.encounterContext==context && v.w.sessionState().journeyRandom()==random && v.saves.empty(),"Inn item refusal preserves time, camera and RNG");
    };v.key('u');
    check(XeenInnTestAccess::error(*v.flow)==std::optional<std::string>{std::string(in.assets.uiText().scalar("WHATS_YOUR_HURRY"))},"Inn Use gives the original DOS map-zero hurry refusal");
    unchanged();
    v.act(AcknowledgeAction{});
    for(unsigned member=0;member<6;++member)v.key(InputKey::F1+member);
    check(!XeenInnTestAccess::error(*v.flow),"Inn F1-F6 item transfer silently refused");unchanged();
    XeenPartyState p;XeenCamera c;c.mapId=0;
    check(xeenDosMapZero(p,c),"literal DOS map zero");c.mapId=28;check(!xeenDosMapZero(p,c),"ordinary town admits Use");p.insideInn=true;check(xeenDosMapZero(p,c),"retained Inn door is DOS map zero");
}
void dismissThenSign(Inputs &in,unsigned town) {
    auto s=source(in,town);const auto departed=s.activeRosterIds.back();s.characters[departed].savedMaze=561;
    Visit v(in,s);v.now+=125;v.act(DismissAction{});v.key(InputKey::F1+5);v.wait([&]{return v.flow->canSave();},"Dismiss finishes before Sign In");
    check(v.p.party.size()==5 && v.p.roster.at(departed).savedMaze==561,"Dismiss defers lodging");
    v.sign();check(v.saves.size()==1 && v.saves[0].characters[departed].savedMaze==28,"Sign In save lodges Dismiss tail in Vertigo");
    for(auto id:v.saves[0].activeRosterIds)check(v.saves[0].characters[id].savedMaze==town,"Sign In lodges active members at the visited town");
}
void scrollCadence(Inputs &in,const std::filesystem::path &log={}) {
    auto s=source(in,28);s.insideInn=true;s.camera={28,18,4,XeenDirection::West};Visit v(in,s);
    unsigned compositions=0;const auto load=v.flow->loadInnScreen;
    v.flow->loadInnScreen=[&](const auto &p){++compositions;return load(p);};
    // Real original-resource composition; optional wall-clock run also logs costs.
    const auto epoch=std::chrono::steady_clock::now();std::ofstream out;const bool wall=!log.empty();
    if(!log.empty()){out.open(log);out<<"phase,step,start_ms,composed_ms,acquired_ms,deadline_ms\n";}
    const auto elapsed=[&]{return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-epoch).count();};
    auto first=v.flow->openInn(true);v.present(first);
    if(wall)out<<"0,22,0,"<<elapsed()<<','<<elapsed()<<','<<PresentationClock::ceilMilliseconds(XeenInnTestAccess::deadline(*v.flow))<<'\n';
    unsigned closing=1,opening=0;std::vector<std::uint64_t> openStarts;std::optional<std::uint64_t> priorOpeningDeadline;
    while(!XeenInnTestAccess::menu(*v.flow)) {
        const auto expected=XeenInnTestAccess::deadline(*v.flow);
        v.now=std::max(v.now,PresentationClock::ceilMilliseconds(expected));
        if(wall) {
            std::this_thread::sleep_until(epoch+std::chrono::milliseconds(v.now));
            // Windows may wake slightly before the rounded millisecond target.
            while(std::uint64_t(elapsed())<v.now)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        v.flow->beginCycle(++v.cycle);
        const auto start=wall?std::uint64_t(elapsed()):v.now;
        if(wall)v.now=start;
        auto next=v.flow->updatePresentation();check(bool(next),"scroll deadline produces the next frame");
        const auto phase=XeenInnTestAccess::phase(*v.flow),step=XeenInnTestAccess::scroll(*v.flow);
        if(phase==1)check(next->palette==first.palette,"Inn palette retained until the VGA retrace");
        const auto composed=wall?std::uint64_t(elapsed()):v.now;
        const auto deadline=XeenInnTestAccess::deadline(*v.flow);
        if(phase==2 && priorOpeningDeadline)check(deadline==PresentationClock::after(*priorOpeningDeadline,1),"scroll advances its scheduled tick before composition");
        if(!wall && (phase==0 || phase==2))v.now+=phase==2 && step==10?80:18;
        if(wall)v.now=elapsed();v.present(*next);
        const auto acquired=PresentationClock::fromMilliseconds(v.now);
        const auto expectedDeadline=phase==0 || phase==2?std::max(deadline,PresentationClock::after(acquired,1)):deadline;
        check(XeenInnTestAccess::deadline(*v.flow)==expectedDeadline,"late acquisition loses its tick instead of compressing the next wait");
        if(phase==2)priorOpeningDeadline=expectedDeadline;
        if(phase==0)++closing;
        if(phase==2){++opening;openStarts.push_back(start);}
        if(wall)out<<phase<<','<<step<<','<<start<<','<<composed<<','<<v.now<<','<<PresentationClock::ceilMilliseconds(deadline)<<'\n';
    }
    check(closing==22 && opening==22,"DOS closes and opens through all 22 steps");
    check(compositions==1,"Inn composed once for opening, retained through the first menu frame");
    if(!wall)for(unsigned n=2;n<openStarts.size();++n) {
        const auto gap=openStarts[n]-openStarts[n-1];
        check(n==10?gap>=109 && gap<=110:gap>=54 && gap<=55,"scroll cadence never compresses a tick after late acquisition");
        check(PresentationClock::tick(PresentationClock::fromMilliseconds(openStarts[n]))>
            PresentationClock::tick(PresentationClock::fromMilliseconds(openStarts[n-1])),"no two scroll steps in one BIOS tick");
    }
}
void scrollSkip(Inputs &in) {
    for(bool closing:{false,true})for(unsigned kind=0;kind<4;++kind) {
        auto s=source(in,28);s.insideInn=true;s.camera={28,18,4,XeenDirection::West};Visit v(in,s);
        v.present(v.flow->openInn(closing));
        if(!closing)v.wait([&]{return XeenInnTestAccess::phase(*v.flow)==2;},"skip fixture reaches opening");
        const auto context=v.flow->inputContext(v.flow->frame().presentation());const auto controls=context.dialog;
        check(context.readyForAction && controls && controls->anyKey && controls->anyClick && controls->keyMouseWait,"scroll polls any key and either mouse button");
        const auto action=kind==0?controls->key('x'):kind==1?controls->key(InputKey::Escape):kind==2?controls->key(InputKey::F1+5):controls->click(319,199);
        check(bool(action),"scroll accepts arbitrary input outside buttons");
        const PlayerAction delivered=kind==0?PlayerAction{DialogKeyAction{'x'}}:kind==1?PlayerAction{DialogKeyAction{InputKey::Escape}}:kind==2?PlayerAction{DialogKeyAction{InputKey::F1+5}}:*action;
        const auto old=v.flow->frame().presentation();const auto input=v.flow->displayedInput();const auto now=v.now;
        const auto before=*v.p.encounterContext;const auto random=v.w.sessionState().journeyRandom();
        v.flow->beginCycle(++v.cycle);auto final=v.flow->handle(delivered,input,old);
        check(!v.flow->updatePresentation(),"skip final image must be acquired before continuation");
        v.present(final);
        check(v.now==now && v.saves.empty() && v.p.insideInn && *v.p.encounterContext==before && v.w.sessionState().journeyRandom()==random,"skip has no wait, save, menu action or gameplay mutation");
        if(closing) {
            check(XeenInnTestAccess::phase(*v.flow)==4,"closing skip reaches final closed image");
            const auto expected=drawXeenScroll(XeenInnTestAccess::closing(*v.flow),1,v.flow->drawDialogSprite);
            check(final.pixels==expected.pixels,"closing skip draws hand00 plus first marble");
            v.flow->beginCycle(++v.cycle);const auto palette=v.flow->updatePresentation();check(bool(palette),"closed image has no timed hold");v.present(*palette);
            v.wait([&]{return XeenInnTestAccess::phase(*v.flow)==2;},"closing skip proceeds into opening without forwarding its input");
            check(!XeenInnTestAccess::menu(*v.flow),"closing key never skips both scrolls");
            v.wait([&]{return XeenInnTestAccess::menu(*v.flow);},"opening completes after closing skip");
        } else {
            check(XeenInnTestAccess::menu(*v.flow) && !v.flow->inventoryOpen(),"opening skip does not forward Exit or F-key to menu");
            auto expected=v.flow->loadInnScreen(v.p);
            expected=drawXeenInn(in.assets.uiText(),expected,in.font,v.p,xeenInnList(v.p,v.c.mapId),0,v.flow->drawDialogSprite);
            check(final.pixels==expected.pixels && final.palette==expected.palette,"opening skip flips the complete Inn image");
        }
        v.flow->handle(DialogKeyAction{'x'},input,old);check(v.flow->innOpen() && v.saves.empty(),"old scroll origin cannot reach the menu");
    }
}
void scrollStall(Inputs &in) {
    for(bool closing:{false,true}) {
        auto s=source(in,28);s.insideInn=true;s.camera={28,18,4,XeenDirection::West};Visit v(in,s);
        v.present(v.flow->openInn(closing));
        if(!closing)v.wait([&]{return XeenInnTestAccess::phase(*v.flow)==2;},"stall fixture reaches opening");
        const auto step=XeenInnTestAccess::scroll(*v.flow);
        v.now=PresentationClock::ceilMilliseconds(XeenInnTestAccess::deadline(*v.flow))+1000;
        v.flow->beginCycle(++v.cycle);const auto next=v.flow->updatePresentation();check(bool(next),"stalled scroll publishes one successor");
        check(XeenInnTestAccess::scroll(*v.flow)==(closing?step-1:step+1),"composition stall skips no steps");
        const auto provisional=XeenInnTestAccess::deadline(*v.flow);v.now+=80;v.present(*next);
        const auto acquired=PresentationClock::fromMilliseconds(v.now);
        const auto deadline=std::max(provisional,PresentationClock::after(acquired,1));
        check(XeenInnTestAccess::deadline(*v.flow)==deadline,"Inn composes/acquires late using the later next BIOS tick");
        v.flow->beginCycle(++v.cycle);check(!v.flow->updatePresentation(),"Inn never advances twice in the acquisition tick");
        v.now=(deadline-1)/1000000;v.flow->beginCycle(++v.cycle);check(!v.flow->updatePresentation(),"Inn retains the full post-acquisition counter wait");
        v.now+=100;v.present(*next);check(XeenInnTestAccess::deadline(*v.flow)==deadline,"Repeated Inn presentation never postpones its deadline");
    }
}
void roundTripAndFight(Inputs &in) {
    const auto slot=std::filesystem::temp_directory_path()/("mmodern-m87-inn-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mmsave");
    const auto bind=[&](Visit &v){v.flow->saveInn=[&v,&in,&slot]{
        XeenGameplayServices services{v.programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font,{},{},{}};
        xeenSaveGameplay(services,v.w,v.p,v.c,v.f,*v.flow,slot,[](auto &,const auto &,const auto &,const auto &){});
    };};
    auto start=source(in,28);start.camera={28,24,4,XeenDirection::North};Visit first(in,start);bind(first);first.sign(true);
    const auto signedIn=XeenSaveFile::read(slot);check(signedIn.insideInn,"native Sign In slot");
    Visit loaded(in,signedIn);bind(loaded);loaded.present(loaded.flow->openInn());
    loaded.wait([&]{return XeenInnTestAccess::menu(*loaded.flow);},"native slot reopens Inn");
    check(*loaded.p.encounterContext==*signedIn.journey->context,"slot reload adds no day");
    for(unsigned n=0;n<2;++n){loaded.key('r');loaded.key(InputKey::F1+4);}
    check(loaded.p.party.size()==4 && XeenSaveFile::read(slot).activeRosterIds.size()==6,"Remove twice leaves slot unchanged");
    loaded.key('x');loaded.wait([&]{return loaded.flow->canSave();},"native Exit save and reload map");
    check(!XeenSaveFile::read(slot).insideInn && XeenSaveFile::read(slot).activeRosterIds.size()==4,"native Exit writes new membership");
    loaded.now+=125;loaded.act(NavigationAction::MoveForward);bool fought=false;
    for(unsigned n=0;n<4000 && (!fought || !loaded.flow->canSave());++n) {
        const auto *combat=loaded.flow->encounter()->combat();fought=fought || bool(combat);
        if(combat && combat->phase()==XeenCombatPhase::PlayerReady)loaded.act(AttackAction{});
        else if(const auto controls=loaded.dialog();controls && controls->anyKey)loaded.act(AcknowledgeAction{});
        else if(!fought && loaded.flow->canSave())loaded.act(WaitAction{});
        loaded.pulse();
    }
    check(fought && loaded.flow->canSave(),"original Inn return party fights and reaches Save");
    XeenGameplayServices services{loaded.programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font,{},{},{}};
    xeenSaveGameplay(services,loaded.w,loaded.p,loaded.c,loaded.f,*loaded.flow,slot,[](auto &,const auto &,const auto &,const auto &){});
    const auto after=XeenSaveFile::read(slot);Visit restored(in,after);
    check(!restored.p.insideInn && XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(after),"post-Inn fight Save/Load exact");
    std::filesystem::remove(slot);
}

std::shared_ptr<XeenAudio> countingAudio(Inputs &in,const GameInstallation &installation,unsigned &songs) {
    std::ifstream moduleFile(installation.uiModule->physicalPath,std::ios::binary);
    const std::vector<std::uint8_t> module((std::istreambuf_iterator<char>(moduleFile)),{});
    auto loader=[&in,&songs](const std::string &name,XeenSceneArchive archive){if(name=="inn.m")++songs;return in.assets.readArchiveResource(name,archive);};
    return std::make_shared<XeenAudio>(3,std::make_shared<const std::vector<std::uint8_t>>(loader("promus",XeenSceneArchive::IntroOnly)),loader,module,0,false);
}
void signInSong(Inputs &in,const GameInstallation &installation) {
    unsigned songs=0;Visit v(in,source(in,28));v.programState.audio=countingAudio(in,installation,songs);
    v.sign();
    // Keep cycling past the first load, so a second request would be counted.
    for(unsigned n=0,after=0;n<600 && after<40;++n){v.pulse();v.programState.audio->advance(v.flow->presentationNow());if(songs)++after;}
    check(songs==1 && v.programState.innSongStarted,"Sign In requests inn.m once and sets the song flag");
}
void productionLoad(Inputs &in,const GameInstallation &installation,const XeenSaveSnapshot &saved,bool title=false) {
    unsigned songs=0;XeenProgramState program;program.innSongStarted=title;std::uint64_t now=0;unsigned shown=0;
    program.audio=countingAudio(in,installation,songs);
    XeenGameplayServices services{program,in.resources(),{},in.mapLoader(),in.objectLoader(),
        [&](auto id){return in.texts.load(id);},in.font,
        [](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{frame(),false};},{},{},{}};
    services.restoreSnapshot=std::make_shared<XeenSaveSnapshot>(saved);services.initialSlot=4;
    services.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{frame(),false};};
    services.clock=[&]{return now;};services.validateEncounterSprite=[](auto){};services.validateCombatSprite=[](auto){};
    services.configureFlow=[&](auto &flow,const auto &){flow.dialogText=&in.assets.uiText();flow.drawDialogSprite=[&](auto &f,const char *n,unsigned i,int x,int y){in.assets.drawDialogSprite(f,n,i,x,y);};flow.loadInnScreen=[](const auto &){return frame();};};
    services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){check(p.insideInn && *p.encounterContext==*saved.journey->context,"production Load publishes Inn without time");};
    services.show=[&](const auto &first,const auto &handler,const auto &,const auto &idle,const auto &){
        auto current=first;handler.framePresented(current.presentation());
        for(unsigned n=0;n<60;++n){now+=100;handler.beginCycle(n+1);if(auto next=idle()){current=*next;handler.framePresented(current.presentation());}}
        const auto context=handler.inputContext(current.presentation());
        check(context.dialog && context.dialog->key('r') && context.mainScreen==MainScreen::None,"production title/panel Load reaches Inn input");++shown;return true;};
    const auto result=Application(program).playGameplay(services,saved.camera,std::filesystem::path("inn.mmsave"),true,XeenEncounterEntry::Ordinary,{});
    check(result==0 && shown==1 && songs==unsigned(!title) && program.innSongStarted,"production title/panel Load song flag");
}
void productionNoTarget(Inputs &in) {
    const auto saved=source(in,28);XeenProgramState program;program.innSongStarted=true;std::uint64_t now=0;unsigned stages=0,writes=0;
    XeenEventFlow *flow=nullptr;
    XeenGameplayServices services{program,in.resources(),{},in.mapLoader(),in.objectLoader(),
        [&](auto id){return in.texts.load(id);},in.font,
        [](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{frame(),false};},{},{},{}};
    services.eventFixture=saved.camera;services.saveUnavailable="fixture";
    services.initialFlags=[]{return XeenGameFlags{};};services.sampleJourneySeed=[]{return 7u;};
    services.resources.loadInitialContext=[&]{return XeenGameplayContextFormat::parse(in.pty);};
    services.resources.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(in.pty);};
    services.resources.loadInitialBankBalances=[&]{return XeenCharacterFormat::parseBankBalances(in.pty);};
    services.resources.loadInitialWorldFlags=[&]{return XeenQuestFlagFormat::parseWorldFlags(in.pty);};
    services.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{frame(),false};};
    services.clock=[&]{return now;};services.validateEncounterSprite=[](auto){};services.validateCombatSprite=[](auto){};
    services.observeSaveStage=[&](auto){++writes;};
    services.configureFlow=[&](auto &f,const auto &){
        flow=&f;f.dialogText=&in.assets.uiText();
        f.drawDialogSprite=[&](auto &image,const char *n,unsigned i,int x,int y){in.assets.drawDialogSprite(image,n,i,x,y);};
        f.drawTavernArt=[&](auto &image){in.assets.drawTavern(image);};
        f.tavernFoodOffer=[&](unsigned n){return in.assets.tavernFoodOffer(n);};
        f.tavernString=[&](std::size_t n){return in.assets.tavernString(n);};
        f.loadSampleDuration=[&](const char *n,XeenSceneArchive side){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(n,side));};
        f.loadInnScreen=[](const auto &){return frame();};
    };
    services.show=[&](const auto &first,const auto &handler,const auto &,const auto &idle,const auto &){
        check(!program.innSongStarted,"gameplay entry outside Inn clears title song flag");
        auto current=first;check(handler.acceptsFrame(current.presentation()),"target-less first frame current");handler.framePresented(current.presentation());handler.completeInputHandoff(current.presentation());
        const auto acquire=[&](const auto &next){if(next){current=*next;
            if(!handler.acceptsFrame(current.presentation()))throw std::runtime_error("target-less successor stale at stage "+std::to_string(stages));
            handler.framePresented(current.presentation());handler.completeInputHandoff(current.presentation());}};
        const auto act=[&](const PlayerAction &action){acquire(handler.withPresentedInput(action,*handler.displayedInput(),current.presentation()));};
        acquire(std::optional<IndexedFrame>{XeenInnTestAccess::syntheticLodging(*flow)});
        auto &p=XeenInnTestAccess::party(*flow);const auto departed=p.party.dismissedRosterIds().back();std::uint8_t exitTail=0;
        check(p.party.size()==5 && p.roster.at(departed).savedMaze==561 && p.party.dismissedRosterIds().size()==1,"target-less pending Dismiss fixture");
        for(unsigned n=0;n<1500 && stages<4;++n){
            now+=100;handler.beginCycle(n+1);acquire(idle());
            const auto context=handler.inputContext(current.presentation());
            if(stages==0){act(InteractionAction{});++stages;}
            else if(stages==1 && context.dialog && context.dialog->key('s')){act(DialogKeyAction{'s'});++stages;}
            else if(stages==2 && flow->innOpen() && XeenInnTestAccess::menu(*flow)) {
                const auto list=xeenInnList(p,28);
                check(p.party.dismissedRosterIds().empty() && p.roster.at(departed).savedMaze==28 &&
                    std::find(list.begin(),list.end(),departed)!=list.end(),"target-less Sign In lodges Dismiss tail and lists it");
                act(DialogKeyAction{'d'});act(DialogKeyAction{'1'});act(DialogKeyAction{'y'});
                check(p.roster.at(0).name.empty() && p.party.size()==4 && writes==0,"target-less Delete clears and returns without writing");
                exitTail=p.party.activeRosterIds().back();
                acquire(std::optional<IndexedFrame>{XeenInnTestAccess::pendingExitTail(*flow)});
                act(DialogKeyAction{'x'});++stages;
            } else if(stages==3 && XeenInnTestAccess::returned(*flow)) {
                check(p.party.dismissedRosterIds().empty() && p.roster.at(exitTail).savedMaze==28,"target-less Exit consumes pending tail without writing");++stages;
            }
        }
        if(stages!=4 || writes || flow->innOpen())throw std::runtime_error("target-less production Sign In/Exit stage="+std::to_string(stages)+" writes="+std::to_string(writes)+" inn="+std::to_string(flow->innOpen())+XeenInnTestAccess::debug(*flow));return true;
    };
    check(Application(program).playGameplay(services,saved.camera,{},false,XeenEncounterEntry::Journey,{})==0,"target-less production session");
}

}
int main(int argc,char **argv) {
    try {rules();check(argc>=2 && argc<=4,"usage: inn-tests <original-installation> [preview.ppm] [timing.csv]");if(argc>=3)preview=argv[2];
        const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"installation");Inputs in(*installation);
        for(unsigned town:{28u,30u,49u})for(unsigned minute:{299u,300u})signIn(in,town,minute);
        deleteRecords(in,*installation);emptySaveRecovery(in);signInSong(in,*installation);fullAndFailures(in,*installation);roundTripAndFight(in);innItems(in);dismissThenSign(in,30);dismissThenSign(in,49);scrollCadence(in);scrollSkip(in);scrollStall(in);productionNoTarget(in);
        if(argc==4)scrollCadence(in,argv[3]);
        auto inn=source(in,28);inn.camera={28,18,4,XeenDirection::West};inn.insideInn=true;productionLoad(in,*installation,inn);productionLoad(in,*installation,inn,true);inn.activeRosterIds.clear();productionLoad(in,*installation,inn);productionLoad(in,*installation,inn,true);
        std::cout<<"Inn rules, original-data Sign In, both saves, membership, sheets and Load passed\n";return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
