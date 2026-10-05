#include "XeenRegionalCombatTestSupport.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
using namespace regional_combat_test;
using regional_combat_test::RegionalCombatFixture;
namespace {
std::vector<Draw> victoryTape(){return {{1,2,2},{1,2,2},{1,2,2},{1,2,2},{1,20,10},{1,3,3},{1,3,3},{1,20,10}};}
void refused(RegionalCombatFixture &f){rejects([&]{f.snapshot();},"encounter");}
void end(RegionalCombatFixture &f){
    f.enter();f.action(Command::Attack);f.action(Command::Attack);
    check(f.combat->phase()==Phase::VictoryAwaitingEnd,"lethal remains distinct from End");
    check(f.service().status==Status::Victory&&f.p.encounterContext->minutes==491,"End charges exactly once");
}
void captureBoundaries(){
    RegionalCombatFixture f{XeenCombatRandom(victoryTape())};
    const auto initial=f.snapshot();check(initial.journey&&initial.journey->schema==9&&initial.journey->content==14,"current quiet snapshot");
    f.actionBefore(XeenEncounterAction::Wait);refused(f);

    auto prior=tape;tape=&f.rng;
    check(f.flow->attachJourney(f.flow->ticket(),[]{}),"genuine combat attachment");f.combat=f.flow->combat();refused(f);
    f.combat->command(f.combat->ticket(),Command::Attack);refused(f);
    while(f.combat->pending()==Work::Action)f.service();f.action(Command::Attack);refused(f);
    check(f.combat->phase()==Phase::VictoryAwaitingEnd,"lethal publication");
    check(f.service().status==Status::Victory,"successful End");refused(f);
    const auto old=f.flow->ticket();check(f.flow->retireJourney(old),"genuine retirement");
    check(!f.flow->retireJourney(old),"retirement capability consumed");refused(f);f.present();
    auto saved=f.snapshot();tape=prior;
    check(saved.journey&&saved.journey->actors[5].accounted&&saved.journey->context==f.p.encounterContext,"current capture fields");
    for(unsigned i=0;i<30;++i)check(saved.journey->supplements[i].owner==i&&xeen_state::sameInputs(saved.journey->supplements[i].inputs,*f.p.roster.combatInputs(i)),"all supplements captured");
    const auto bytes=XeenSaveFormat::encode(saved);check(bytes[8]==5,"current envelope");
    check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes))==bytes,"current encode/decode exactness");
    auto detached=XeenPartyLoader().loadFromResources(chr(),pty());auto copied=f.camera;
    check(!XeenSaveState::canCapture(detached,f.camera,f.w)&&!XeenSaveState::canCapture(f.p,copied,f.w)&&XeenSaveState::canCapture(f.p,f.camera,f.w),"capture bound to exact owners");
    const auto lease=f.flow->holdJourneyWork(XeenCombatBoundary::Work::Inventory);refused(f);
    f.flow->releaseJourneyWork(XeenCombatBoundary::Work::Inventory,lease);
    check(XeenSaveFormat::encode(f.snapshot())==bytes,"released external work leaves exact snapshot");
    f.w.discardMapCache();f.w.map(23);f.w.objectFile(23);check(XeenSaveFormat::encode(f.snapshot())==bytes,"cache reconstruction retains capture");
    regional_test::Fixture restored(XeenSaveFormat::decode(bytes));
    check(XeenSaveFormat::encode(restored.snapshot())==bytes,"restored regional completed combat is exact");
    f.flow.reset();check(!XeenSaveState::canCapture(f.p,f.camera,f.w),"Journey destruction closes capture authority");
}
void retirementAndIntegrity(){
    RegionalCombatFixture f{XeenCombatRandom(victoryTape())};end(f);
    const auto old=f.combat->ticket();const auto lease=f.flow->boundary().hold(XeenCombatBoundary::Work::Inventory);
    check(!f.flow->retireJourney(f.flow->ticket()),"busy retirement refused");
    f.flow->boundary().release(XeenCombatBoundary::Work::Inventory,lease);
    check(f.combat->fail(old).status==Status::Stale,"obsolete post-End failure cannot poison fresh retirement");
    check(f.flow->retireJourney(f.flow->ticket()),"fresh retirement remains valid");f.present();
    for(unsigned kind=0;kind<3;++kind){
        RegionalCombatFixture changed;
        if(kind==0)++changed.p.roster.at(0).currentHp;
        if(kind==1)++changed.p.roster.at(29).currentSp;
        if(kind==2)changed.camera.direction=XeenDirection::North;
        check(!changed.flow->journeyQuiet(),"changed active/inactive/camera state invalidates capture");refused(changed);
    }
    {
        RegionalCombatFixture t{XeenCombatRandom(victoryTape())};end(t);
        check(t.flow->retireJourney(t.flow->ticket()),"tamper fixture retirement");t.present();
        auto &actor=const_cast<XeenActor &>(t.w.sessionState().actors()[0]);
        --actor.hp;check(!t.flow->journeyQuiet(),"changed actor HP invalidates capture");refused(t);
        ++actor.hp;refused(t);
    }
}
void overlayCallbackGuards(){
    for(unsigned mode=0;mode<3;++mode){
        XeenWorld *ptr=nullptr;
        XeenWorld w([&](auto id){if(mode==0)ptr->markEncounterSession();return regional_test::map(id);},
            [&](auto id){if(mode==1)ptr->markEncounterSession();return regional_test::objects(id);});ptr=&w;
        const auto load=[&](auto id){if(mode==2)w.markEncounterSession();return regional_test::events(id);};
        rejects([&]{w.restoreSessionState({{23,0}},{{23,0}},load);},"encounter");
        check(w.hasEncounterState()&&w.sessionState().disabledObjectCount()==0&&w.sessionState().disabledEventCount()==0,"marked callback cannot publish overlays");
    }
    const auto saved=regional_test::snapshot();RegionalCombatFixture marked;marked.flow.reset();marked.p.encounterContext.reset();
    XeenCamera camera;XeenGameFlags flags;XeenWorld world(regional_test::map,regional_test::objects);unsigned calls=0;
    auto resources=regional_test::resources();resources.loadEvents=[&](auto id){++calls;return regional_test::events(id);};
    resources.loadMonsterStatistics=[&]{++calls;return regional_test::statistics();};
    rejects([&]{XeenSaveState::restoreBeforeGameplay(saved,resources,marked.p,camera,flags,world,[&](auto &,const auto &,const auto &,const auto &){++calls;});},"encounter");
    check(calls==0,"detached destination marker refuses before providers");
}
}
int main(){probe_fired::expect("XeenCombatRandom::draw");try{captureBoundaries();retirementAndIntegrity();overlayCallbackGuards();std::cout<<"Regional combat capture and restore guards passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
