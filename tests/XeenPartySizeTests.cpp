#include "XeenRegionalJourneyTestSupport.h"
#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "app/XeenGameplayServices.h"
#include "app/Application.h"
#include "formats/xeen/XeenVoc.h"
#include "platform/XeenSaveFile.h"
#include <chrono>
#include <filesystem>
#include "games/xeen/XeenJourneyProgression.h"
#include "games/xeen/XeenRestRules.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
#include <fstream>
using namespace mmodern;
using save_test::check;
using save_test::rejects;
namespace mmodern {
struct XeenPartyInterfaceTestAccess {
    static bool fatal(const XeenEventFlow &f){return f._fatal;}
};
}
namespace {
const std::vector<std::uint8_t> ids{23,5,17,8,2,19};
template<class C> void finish(C &candidate,XeenCombatRandom &rng,unsigned budget=1) {
    for(unsigned n=0;n<1000;++n) {XeenConsequenceDraw draw{rng,budget,{}};if(candidate.service(draw))return;}
    throw std::runtime_error("Party-size candidate did not finish");
}
void rules() {
    for(unsigned count=1;count<=6;++count) {
        const auto party=XeenParty::fromRosterIds({ids.begin(),ids.begin()+count});
        const auto order=party.activeOrder();check(order.size()==count && order.mask()==(1u<<count)-1,"counted order");
        rejects([&]{(void)order[count];});
        XeenConsequenceCharacters chars(count);XeenConsequenceInputs inputs(count);
        for(unsigned n=0;n<count;++n) {
            auto &c=chars[n];c.rosterId=order[n];c.permanentLevel=3;c.birthYear=592;c.currentHp=1;
            c.intellect=c.personality=c.endurance={15,0};c.conditions[8]=1;
            auto &i=inputs[n];i.might=i.speed=i.accuracy={15,0};i.luck=XeenAttributeValue{15,0};
            i.resistances.emplace();i.poisonResistance=XeenAttributeValue{0,0};
        }
        rejects([&]{(void)chars[count];});rejects([&]{(void)inputs[count];});
        XeenGameplayContext context;context.year=610;
        XeenRestRecovery rest(chars,inputs,context,2*count);
        check(rest.consumed==count && rest.food==count && rest.characters.size()==count,"Rest and food use active count");
        for(const auto &c:rest.characters)check(c.currentHp>1 && !c.conditions[8],"active Rest recovery");
        for(auto &c:chars)c.conditions[8]=0;
        XeenMonsterRecord record;record.raw[16]=100;record.raw[38]=113;
        XeenActor actor;actor.id={23,1};actor.statistics=record;actor.hp=100;actor.lifecycle=XeenActorLifecycle::Present;
        std::array<const XeenCharacter *,6> pointers{};
        for(unsigned n=0;n<count;++n)pointers[n]=&chars[n];
        const auto lethal=xeenPrepareJourneyLethal(actor,pointers,inputs,{},order.mask());
        for(unsigned n=0;n<count;++n)check(lethal.experience[n]==2*(record.experience()/count),"XP active recipient division");
        XeenPartyVolley volley;volley.count=count;std::fill_n(volley.eligible.begin(),count,true);
        unsigned shots=0;while(volley.select()){check(volley.shooter<count,"inactive Poison Volley shot");volley.settle(true);++shots;}
        check(shots==count && !volley.remaining(),"one Poison Volley shot per member");
        std::vector<XeenCombatRandom::Draw> tape;
        if(count==6)tape={{1,6,2},{0,4,3}};
        else if(count>1)tape={{0,count-1,count-1}};
        XeenCombatRandom random(tape);XeenMonsterTargetCandidate target(chars,1,order.mask());
        if(count==1){XeenConsequenceDraw zero{random,0,{}};check(target.service(zero),"one target must need no draw budget");}
        else if(count==6) {
            XeenConsequenceDraw one{random,1,{}};check(!target.service(one) && random.position()==1,"six-target continuation retains first draw");
        }
        finish(target,random);
        check(target.mask==(1u<<(count==6?3:count-1)) && random.position()==tape.size(),"DOS count-specific target draws");
        // Compare a longer smaller-party sequence against independent raw calls.
        XeenCombatRandom actual(719),expected(719);
        for(unsigned attack=0;attack<20;++attack) {
            unsigned index=0;
            if(count==6){auto roll=*expected.draw(1,6);index=roll==6?5:*expected.draw(0,4);}
            else if(count>1)index=*expected.draw(0,count-1);
            XeenMonsterTargetCandidate next(chars,1,order.mask());finish(next,actual);
            check(next.mask==(1u<<index) && actual==expected,"smaller-party RNG sequence");
        }
        if(count==6) {
            XeenCombatRandom last(std::vector<XeenCombatRandom::Draw>{{1,6,6}});
            XeenMonsterTargetCandidate sixth(chars,1,order.mask());finish(sixth,last);
            check(sixth.mask==32 && last.position()==1,"sixth target costs one draw");
        }
        if(count>1) {
            chars[count-1].conditions[12]=1;
            std::vector<XeenCombatRandom::Draw> fallback=count==6?
                std::vector<XeenCombatRandom::Draw>{{1,6,6},{0,4,0}}:
                std::vector<XeenCombatRandom::Draw>{{0,count-1,count-1},{0,count-2,0}};
            XeenCombatRandom r(fallback);XeenMonsterTargetCandidate t(chars,1,order.mask());finish(t,r);
            check(t.mask==1 && r.position()==fallback.size(),"disabled participants remain in initial draw before fallback");
        }
    }
}
// Local synthetic combat graph: the flee threshold is deliberately certain.
struct SmallCombat {
    XeenEventPresenter::Clock clock=[]{return 0;};
    XeenProgramState program{mmodern::XeenWallClock::fixed(0)};XeenPartyState party;XeenCamera camera;XeenGameFlags flags;
    XeenWorld world{[](auto){auto map=regional_journey_test::regionalMap();map.geometry.difficulties[7]=127;return map;},
                    [](auto){return regional_journey_test::regionalObjects();}};
    std::unique_ptr<XeenEncounterFlow> flow;
    explicit SmallCombat(const XeenSaveSnapshot &snapshot) {
        auto resources=regional_test::resources();resources.loadInitialParty=[]()->XeenPartyState{throw std::runtime_error("restore reattached CHR");};
        XeenSaveState::restoreBeforeGameplay(snapshot,resources,party,camera,flags,world,[](auto &,const auto &,const auto &,const auto &){});
        flow=std::make_unique<XeenEncounterFlow>(program,world,party,camera,flags,clock,XeenJourneyRestoreTag{});
        check(flow->prepareJourneyFrame(flow->ticket(),[]{}) && flow->presentJourney(flow->ticket()),"small graph frame");
    }
    void enter() {
        check(flow->journeyAction(flow->ticket(),XeenEncounterAction::Wait).outcome==XeenEncounterOutcome::Engaged,"small Run/defeat contact");
        check(flow->attachJourney(flow->ticket(),[]{}),"small Run/defeat attachment");
    }
};
void runAndDefeat(const XeenSaveSnapshot &source,unsigned count) {
    SmallCombat running(source);running.enter();auto *combat=running.flow->combat();
    for(unsigned step=0;step<2000 && combat->phase()!=combat_test::Phase::Disengaged;++step) {
        if(combat->phase()==combat_test::Phase::PlayerReady)combat->command(combat->ticket(),combat_test::Command::Run);
        else check(combat->service(combat->ticket()).status!=combat_test::Status::Failed,"small-party Run service");
    }
    check(combat->phase()==combat_test::Phase::Disengaged && !combat->participants() && running.party.party.size()==count,"Run compacts all combat participants without changing membership");
    auto fragile=source;
    for(auto id:fragile.activeRosterIds){auto &c=fragile.characters[id];c.permanentLevel=1;c.currentHp=1;c.armor={};}
    SmallCombat losing(fragile);losing.enter();combat=losing.flow->combat();
    for(unsigned step=0;step<5000 && combat->phase()!=combat_test::Phase::Defeat;++step) {
        if(combat->phase()==combat_test::Phase::PlayerReady)combat->command(combat->ticket(),combat_test::Command::Block);
        else check(combat->service(combat->ticket()).status!=combat_test::Status::Failed,"small-party defeat service");
    }
    check(combat->phase()==combat_test::Phase::Defeat && losing.party.party.size()==count,"defeat uses active count");
    for(auto id:fragile.activeRosterIds)check(!xeenCombatTargetable(losing.party.roster.at(id)),"defeat checks every active member");
    rejects([&]{XeenSaveState::capture(regional_test::signature(),losing.party,losing.camera,losing.flags,losing.world);});
}
void savesAndCombat() {
    for(unsigned count=1;count<=6;++count) {
        auto bytes=combat_test::chr();
        for(unsigned id=0;id<30;++id) {
            const auto b=id*354;bytes[b+35]=20;bytes[b+342]=200;bytes[b+343]=3;
            for(unsigned a=0;a<7;++a)bytes[b+20+a*2]=20;
            bytes[b+166]=58;bytes[b+167]=2;bytes[b+169]=1;
            bytes[b+339]=173;bytes[b+340]=49;bytes[b+341]=2;
            bytes[b+37]=id+17;bytes[b+352]=id+61;bytes[b+353]=id+103;
        }
        regional_journey_test::Fixture f(bytes,[&](auto &f){f.p.party=XeenParty::fromRosterIds({ids.begin(),ids.begin()+count});});
        auto before=XeenSaveState::capture(regional_test::signature(),f.p,f.camera,f.flags,f.w);
        runAndDefeat(before,count);
        const auto wire=XeenSaveFormat::encode(before);check(wire[8]==23,"v23 envelope byte");
        auto decoded=XeenSaveFormat::decode(wire);save_test::sameSnapshot(before,decoded);
        check(XeenSaveFormat::encode(decoded)==wire,"byte-exact one-to-six round trip");
        for(unsigned id=0;id<30;++id)check(decoded.characters[id].savedMaze==561 && decoded.characters[id].townUnknown==173 &&
            decoded.characters[id].birthDay==id+17 && decoded.characters[id].adventuringSpell==id+61 && decoded.characters[id].combatSpell==id+103,"full inactive roster fields");
        {
            auto detached=f.w.transitionCandidate();XeenPartyState local;detached->copyEventParty(f.p,local);
            XeenGameFlags flags;XeenCamera camera=f.camera;XeenEventScriptValues script;
            XeenEventValueOwners owners{local,flags,camera,script};std::string reason;
            for(unsigned member=0;member<count;++member) {
                check(xeenEventGive(owners,16,37,member).status==XeenEventWriteStatus::Applied,"Event XP give");
                const auto probe=xeenEventProbe(owners,16,0,member,reason);check(probe && probe->value==37,"Event condition uses chosen member");
                check(xeenEventTake(owners,16,12,member).status==XeenEventWriteStatus::Applied,"Event XP take");
                check(xeenEventGive(owners,18,11,member).status==XeenEventWriteStatus::Applied && local.party.member(local.roster,member).conditions[11],"Event condition give");
                check(xeenEventTake(owners,18,11,member).status==XeenEventWriteStatus::Applied && !local.party.member(local.roster,member).conditions[11],"Event condition take");
                check(local.roster.combatInputs(ids[member])->experience==25,"Event values bind non-default roster ID");
            }
            check(local.roster.combatInputs(29)->experience==0 && !local.roster.at(29).conditions[11],"Events leave inactive roster untouched");
            check(xeenEventGive(owners,16,1,count).status==XeenEventWriteStatus::Unsupported,"Event refuses inactive member");
        }
        if(count>1) {
            const auto time=*f.p.encounterContext;const auto rng=*f.w.sessionState().journeyRandom();
            check(f.flow->journeyDismiss(f.flow->ticket(),1),"Dismiss accepted");f.present();
            check(f.p.party.size()==count-1 && f.p.party.activeRosterIds()[0]==ids[0] &&
                f.p.roster.at(ids[1]).savedMaze==561 && f.p.party.dismissedRosterIds().size()==1,"Dismiss keeps order and defers CHR writes");
            check(*f.p.encounterContext==time && *f.w.sessionState().journeyRandom()==rng,"Dismiss costs no time or RNG");
            auto saved=XeenSaveState::capture(regional_test::signature(),f.p,f.camera,f.flags,f.w);
            check(f.p.roster.at(ids[1]).savedMaze==28 && saved.characters[ids[1]].savedMaze==28 &&
                f.p.party.dismissedRosterIds().empty(),"capture lodges live roster and consumes tail");
        } else check(!f.flow->journeyDismiss(f.flow->ticket(),0),"last member refused");
        const auto engaged=f.action(XeenEncounterAction::Wait);
        if(engaged.outcome!=XeenEncounterOutcome::Engaged)throw std::runtime_error("size "+std::to_string(count)+" engagement: "+f.flow->notice()+" outcome "+std::to_string(unsigned(engaged.outcome)));
        check(f.flow->attachJourney(f.flow->ticket(),[]{}),"small-party attachment");auto *combat=f.flow->combat();
        check(combat->participants()==(1u<<f.p.party.size())-1,"combat population uses membership");
        for(unsigned n=0;n<300 && combat->phase()!=combat_test::Phase::VictoryAwaitingEnd;++n) {
            if(combat->phase()==combat_test::Phase::PlayerReady) {
                check(unsigned(combat->participant())<f.p.party.size(),"inactive combat turn");f.command(combat_test::Command::Attack);
            } else check(combat->service(combat->ticket()).status!=combat_test::Status::Failed,"small-party combat service");
        }
        check(combat->phase()==combat_test::Phase::VictoryAwaitingEnd,"small party can fight to victory");
    }
    auto inn=save_test::sample();inn.insideInn=true;inn.camera={28,18,4,XeenDirection::West};
    const auto bytes=XeenSaveFormat::encode(inn);save_test::sameSnapshot(inn,XeenSaveFormat::decode(bytes));
    check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes))==bytes,"Inn envelope exact round trip");
    for(const XeenCamera door:{XeenCamera{30,19,16,XeenDirection::West},XeenCamera{49,11,12,XeenDirection::West}}) {
        auto town=inn;town.camera=door;
        const auto encoded=XeenSaveFormat::encode(town);save_test::sameSnapshot(town,XeenSaveFormat::decode(encoded));
    }
    for(const XeenCamera invalid:{XeenCamera{28,17,4,XeenDirection::West},XeenCamera{30,19,16,XeenDirection::North},XeenCamera{49,11,11,XeenDirection::West}}) {
        auto town=inn;town.camera=invalid;rejects([&]{XeenSaveFormat::encode(town);});
    }
    auto bad=inn;bad.activeRosterIds={0,0};rejects([&]{XeenSaveFormat::encode(bad);});
    bad=inn;bad.activeRosterIds.clear();check(XeenSaveFormat::decode(XeenSaveFormat::encode(bad)).activeRosterIds.empty(),"empty Inn party roundtrip");
    bad=inn;bad.activeRosterIds={24};rejects([&]{XeenSaveFormat::encode(bad);});
    auto old=bytes;old[8]=18;
    try {XeenSaveFormat::decode(old);throw std::runtime_error("v18 accepted");}
    catch(const XeenUnsupportedSave &e){check(e.recognizableOlder && std::string(e.what()).find("older")!=std::string::npos,"v18 refusal");}
}
void rosterOracle(const char *game,const char *target) {
    const auto installation=xeenTestInstallationDetector().detect(game);check(bool(installation),"oracle installation");
    XeenAssetSource assets(*installation);const auto bytes=assets.readInitialResource("maze.chr");
    std::ofstream out(target);check(bool(out),"oracle output");
    for(unsigned id=0;id<30;++id) {
        const auto *r=bytes.data()+354*id;const auto prefix="snapshot.characters["+std::to_string(id)+"].";
        out<<prefix<<"birthDay\t"<<unsigned(r[37])<<'\n'<<prefix<<"xeenSide\t"<<unsigned(r[18])<<'\n'
           <<prefix<<"townUnknown\t"<<unsigned(r[339])<<'\n'<<prefix<<"savedMaze\t"<<unsigned(r[340]|r[341]<<8)<<'\n'
           <<prefix<<"adventuringSpell\t"<<unsigned(r[352])<<'\n'<<prefix<<"combatSpell\t"<<unsigned(r[353])<<'\n';
    }
    out<<"snapshot.insideInn\t0\n";check(bool(out),"oracle write");
}
void dismissedF9Save(training_test::Inputs &in,XeenSaveSnapshot source) {
    source.characters[6].savedMaze=30;source.characters[11].savedMaze=49;
    const auto target=std::filesystem::temp_directory_path()/("mmodern-m87-f9-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mmsave");
    XeenSaveFile::write(target,source);
    XeenProgramState program{mmodern::XeenWallClock::fixed(0)};XeenEventFlow *flow=nullptr;const XeenPartyState *live=nullptr;const XeenCamera *liveCamera=nullptr;
    std::uint64_t now=0,cycle=0;unsigned stages=0;bool played=false;
    XeenGameplayServices services{program,in.resources(),{},in.mapLoader(),in.objectLoader(),[&](auto id){return in.texts.load(id);},in.font,
        [](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{training_test::frame(),false};},{},
        [&](auto &f,const auto &){flow=&f;f.dialogText=&in.assets.uiText();f.drawDialogSprite=[&](auto &frame,const char *resource,unsigned id,int x,int y){in.assets.drawDialogSprite(frame,resource,id,x,y);};},{},
        [&](auto &,auto &,const auto &p,auto &camera,const auto &){live=&p;liveCamera=&camera;}};
    services.clock=[&]{return now;};services.observeSaveStage=[&](auto){++stages;};
    services.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{training_test::frame(),false};};
    services.validateEncounterSprite=[](auto){};services.validateCombatSprite=[](auto){};
    services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &status){
        const auto present=[&]{const auto frame=flow->frame().presentation();handler.framePresented(frame);if(handler.completeInputHandoff)handler.completeInputHandoff(frame);};
        const auto send=[&](PlayerAction action){present();now+=125;handler.beginCycle(++cycle);
            const auto input=handler.displayedInput();check(bool(input),"F9 flow displayed input");handler.withPresentedInput(action,*input,flow->frame().presentation());present();};
        const auto quiet=[&]{for(unsigned n=0;n<1000 && !flow->canSave();++n){now+=125;handler.beginCycle(++cycle);if(auto next=idle())present();}check(flow->canSave(),"F9 flow settles");};
        present();handler.beginCycle(++cycle);if(auto entry=idle())present();quiet();
        for(unsigned member:{5u,3u}){send(DismissAction{});send(DialogKeyAction{InputKey::F1+member});quiet();}
        check(live->party.dismissedRosterIds().size()==2 && live->roster.at(6).savedMaze==30 && live->roster.at(11).savedMaze==49,"F9 preimage retains two foreign lodgings");
        send(SaveGameAction{});quiet();
        const auto written=XeenSaveFile::read(target);
        check(status().find("Saved")!=std::string::npos && stages==3 && written.activeRosterIds==std::vector<std::uint8_t>({0,18,14,1}) &&
            written.characters[6].savedMaze==28 && written.characters[11].savedMaze==28 && live->roster.at(6).savedMaze==28 &&
            live->roster.at(11).savedMaze==28 && live->party.dismissedRosterIds().empty(),"actual F9 saves and lodges both members in file and live roster");
        const auto facing=static_cast<XeenDirection>(liveCamera->direction);send(NavigationAction::TurnRight);quiet();
        check(liveCamera->direction!=facing,"action after F9 changes the live facing");played=true;
        check(XeenSaveFormat::encode(XeenSaveFile::read(target))==XeenSaveFormat::encode(written),"action after F9 preserves the file");
        return true;
    };
    check(Application(program).playGameplay(services,{},target,true)==0 && played,"production F9 flow and subsequent action succeed");
    const auto written=XeenSaveFile::read(target);training_test::Fixture reload(in,written);
    check(reload.p.party.dismissedRosterIds().empty() && XeenSaveFormat::encode(reload.snapshot())==XeenSaveFormat::encode(written),"reload after F9 and another action exactly restores reduced membership and lodging");
    std::filesystem::remove(target);
}
void dismissInputsAndDeath(training_test::Inputs &in,const XeenSaveSnapshot &fresh) {
    for(bool button:{false,true}) {
        training_test::Fixture f(in,fresh,true);const auto before=f.snapshot();f.now+=125;f.act(DismissAction{});
        const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;check(bool(input),"Dismiss input is open");
        const auto shown=f.flow->frame().presentation();f.now+=125;f.flow->beginCycle(++f.cycle);
        const auto animated=f.flow->updatePresentation();check(bool(animated),"Dismiss continues ordinary scene draws");f.present(*animated);
        check(f.flow->frame().presentation()!=shown,"Dismiss publishes its animated redraw");
        const auto escape=input->click(226,121);check(bool(escape),"Dismiss Escape button hit");
        f.act(button?*escape:PlayerAction{DialogKeyAction{InputKey::Escape}});f.finishDeparture();
        check(XeenSaveFormat::encode(f.snapshot())==XeenSaveFormat::encode(before),"Dismiss Escape key/button leaves all durable state unchanged");
    }
    {
        training_test::Fixture f(in,fresh);f.now+=125;f.act(DismissAction{});
        const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
        const auto portrait=input->click(48,155);check(bool(portrait),"Dismiss active portrait hit");f.act(*portrait);f.finishDeparture();
        check(f.p.party.activeRosterIds()==std::vector<std::uint8_t>({0,14,11,1,6}) && f.p.party.dismissedRosterIds()==std::vector<std::uint8_t>{18},"portrait dismisses its active member");
    }
    auto mortal=fresh;mortal.activeRosterIds={0,18};mortal.characters[18].conditions[13]=1;mortal.characters[18].currentHp=-1;
    training_test::Fixture f(in,mortal);f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));};
    f.now+=125;f.act(DismissAction{});f.act(DialogKeyAction{InputKey::F1});
    check(f.p.party.activeRosterIds()==std::vector<std::uint8_t>{18} && !f.flow->canSave(),"dismissing the only targetable member blocks Save");
    for(unsigned n=0;n<250 && !f.flow->partyDeathComplete();++n) {
        check(!f.flow->canSave() && !XeenPartyInterfaceTestAccess::fatal(*f.flow),"ordinary death stays nonfatal and unsaveable");
        const auto context=f.flow->inputContext(f.flow->frame().presentation());
        if(context.readyForAction && context.dialog && context.dialog->anyKey)f.act(AcknowledgeAction{});
        f.now+=1000;f.flow->beginCycle(++f.cycle);if(auto next=f.flow->updatePresentation())f.present(*next);
    }
    check(f.flow->partyDeathComplete() && !XeenPartyInterfaceTestAccess::fatal(*f.flow) && !f.flow->canSave(),"Dismiss reaches the complete ordinary death flow");
}
void original(const char *path) {
    const auto installation=xeenTestInstallationDetector().detect(path);check(bool(installation),"original installation");
    training_test::Inputs in(*installation);
    XeenSaveSnapshot fresh;
    {
        auto party=XeenPartyLoader().loadInitialParty(in.assets);XeenCamera camera{28,18,4,XeenDirection::West};XeenGameFlags flags;
        XeenWorld world(in.mapLoader(),in.objectLoader());const auto defaults=in.setup();
        XeenJourneySetup setup{in.chr,defaults.context,in.statistics,in.city,defaults.seed,defaults.regionalManifest};
        setup.purse=defaults.purse;setup.worldFlags=defaults.worldFlags;setup.bank=defaults.bank;
        setup.regionalText=defaults.regionalText;setup.learnedNames=defaults.learnedNames;setup.learnedNamesProvider=defaults.learnedNamesProvider;
        setup.cityEventsProvider=[&]{return in.city;};setup.mainlandEventsProvider=[&]{return in.mainland;};setup.prepared=false;
        XeenEventPresenter::Clock clock=[]{return 0;};
        XeenEncounterFlow flow(in.programState,world,party,camera,flags,clock,setup);
        check(flow.prepareJourneyFrame(flow.ticket(),[]{}) && flow.presentJourney(flow.ticket()),"original fresh frame");
        fresh=XeenSaveState::capture(in.signature,party,camera,flags,world);
    }
    for(auto id:fresh.activeRosterIds)check(fresh.characters[id].savedMaze==28,"DOS word lodging reads Vertigo");
    dismissedF9Save(in,fresh);
    dismissInputsAndDeath(in,fresh);
    auto innSave=fresh;innSave.insideInn=true;
    training_test::Fixture inn(in,innSave);
    check(inn.p.insideInn && XeenSaveFormat::encode(inn.snapshot())==XeenSaveFormat::encode(innSave),"synthetic Inn restore/capture stays exact");
    auto foreignLodging=fresh;foreignLodging.characters[6].savedMaze=30;foreignLodging.characters[11].savedMaze=49;
    training_test::Fixture f(in,foreignLodging);const auto time=*f.p.encounterContext;
    const auto dismiss=[&](unsigned member){f.now+=125;
        const auto context=f.flow->inputContext(f.flow->frame().presentation());
        if(!context.readyForAction)throw std::runtime_error("Dismiss entry unready: screen "+std::to_string(unsigned(context.mainScreen))+" dialog "+std::to_string(bool(context.dialog))+" queue "+std::to_string(context.acceptsQueuedInput)+" pending "+std::to_string(f.flow->encounter()->state().pending()));
        f.act(DismissAction{});f.act(DialogKeyAction{InputKey::F1+member});f.finishDeparture();};
    dismiss(5);dismiss(3);
    if(f.p.party.activeRosterIds()!=std::vector<std::uint8_t>({0,18,14,1}))throw std::runtime_error("Dismiss UI count "+std::to_string(f.p.party.size())+" notice: "+f.flow->encounter()->notice()+" dialog "+std::to_string(bool(f.flow->inputContext(f.flow->frame().presentation()).dialog)));
    check(*f.p.encounterContext==time,"original new game dismisses two without time");
    f.act(SelectMemberAction{4});check(!f.flow->inventoryOpen(),"inactive F5 ignored with four members");
    bool fought=false;
    f.now+=125;f.act(NavigationAction::MoveForward);
    for(unsigned tick=0;tick<3000 && (!fought || !f.flow->canSave());++tick) {
        auto *combat=f.flow->encounter()->combat();fought=fought || bool(combat);
        if(combat && combat->phase()==combat_test::Phase::PlayerReady)f.act(AttackAction{});
        else {
            const auto context=f.flow->inputContext(f.flow->frame().presentation());
            if(context.readyForAction && context.dialog && context.dialog->anyKey)f.act(AcknowledgeAction{});
            else if(!fought && f.flow->canSave())f.act(WaitAction{});
        }
        f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);
    }
    check(fought && f.flow->canSave(),"original dismissed party fights and returns to a save boundary");
    check(f.p.roster.at(6).savedMaze==30 && f.p.roster.at(11).savedMaze==49 && f.p.party.dismissedRosterIds().size()==2,
        "two departures retain foreign lodging until production Save");
    const auto target=std::filesystem::temp_directory_path()/("mmodern-m87-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mmsave");
    XeenGameplayServices services{f.programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font,{},{},{}};
    unsigned preflights=0;
    xeenSaveGameplay(services,f.w,f.p,f.c,f.f,*f.flow,target,[&](auto &,const auto &party,const auto &,const auto &){
        ++preflights;check(party.party.size()==4,"production Save preflights reduced membership");});
    auto saved=XeenSaveFile::read(target);
    check(preflights==1 && saved.activeRosterIds==std::vector<std::uint8_t>({0,18,14,1}) &&
        saved.characters[6].savedMaze==28 && saved.characters[11].savedMaze==28 &&
        f.p.roster.at(6).savedMaze==28 && f.p.roster.at(11).savedMaze==28 && f.p.party.dismissedRosterIds().empty(),
        "production Save writes and publishes both deferred lodgings");
    training_test::Fixture loaded(in,saved);check(loaded.p.party.dismissedRosterIds().empty(),"Load clears Dismiss tail");
    check(XeenSaveFormat::encode(loaded.snapshot())==XeenSaveFormat::encode(saved),"original smaller party reload is exact");
    const auto resume=[](training_test::Fixture &g) {
        g.now+=125;g.act(WaitAction{});
        for(unsigned tick=0;tick<3000 && !g.flow->canSave();++tick) {
            const auto *combat=g.flow->encounter()->combat();
            if(combat && combat->phase()==combat_test::Phase::PlayerReady)g.act(AttackAction{});
            else {const auto context=g.flow->inputContext(g.flow->frame().presentation());
                if(context.readyForAction && context.dialog && context.dialog->anyKey)g.act(AcknowledgeAction{});}
            g.now+=125;g.flow->beginCycle(++g.cycle);if(auto frame=g.flow->updatePresentation())g.present(*frame);
        }
        check(g.flow->canSave(),"original smaller-party resumed action finishes");
    };
    resume(f);
    check(XeenSaveFormat::encode(XeenSaveFile::read(target))==XeenSaveFormat::encode(saved),"playing after Save leaves the written file intact");
    training_test::Fixture reloadedAfterAction(in,XeenSaveFile::read(target));
    check(reloadedAfterAction.p.party.dismissedRosterIds().empty(),"reload after another action clears the tail");
    resume(reloadedAfterAction);resume(loaded);
    check(XeenSaveFormat::encode(f.snapshot())==XeenSaveFormat::encode(reloadedAfterAction.snapshot()),"file reload after another action resumes exact RNG");
    std::filesystem::remove(target);
    check(XeenSaveFormat::encode(f.snapshot())==XeenSaveFormat::encode(loaded.snapshot()),"original smaller party resumes RNG and next action exactly");
    training_test::Fixture last(in,fresh);
    while(last.p.party.size()>1) {
        last.now+=125;last.act(DismissAction{});last.act(DialogKeyAction{InputKey::F1+1});last.finishDeparture();
    }
    last.now+=125;const auto before=last.snapshot();last.act(DismissAction{});last.act(DialogKeyAction{InputKey::F1});
    last.act(AcknowledgeAction{});last.finishDeparture();
    check(XeenSaveFormat::encode(last.snapshot())==XeenSaveFormat::encode(before),"last-member refusal changes no CHR/time/save state");
}
}
int main(int argc,char **argv) {
    try {if(argc==4 && std::string(argv[1])=="--roster-oracle")rosterOracle(argv[2],argv[3]);else if(argc==2 && std::string(argv[1])=="--combat")savesAndCombat();else if(argc==2)original(argv[1]);else {rules();savesAndCombat();}return 0;}
    catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
