#ifndef MMODERN_REGIONAL_JOURNEY_TEST_SUPPORT_H
#define MMODERN_REGIONAL_JOURNEY_TEST_SUPPORT_H
#include "XeenCombatTestSupport.h"
#include "XeenRegionalTestSupport.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "app/XeenEncounterFlow.h"
namespace regional_journey_test {
using namespace combat_test;
inline XeenMap regionalMap(){
    auto m=regional_test::map();
    for(unsigned i:{13u,28u}){auto &c=m.geometry.cells[i];c.rawWord=15;c.surfaceIndex=15;c.geometry=XeenOutdoorLayers{15,0,0,0};}
    return m;
}
inline XeenObjectFile regionalObjects(){
    auto m=regional_test::objects();
    for(unsigned i=0;i<19;++i)m.entities.monsters[i]={int(i%7),int(12+i/7),0,0,8};
    m.entities.monsters[5]={13,2,0,0,8};return m;
}
struct Fixture {
	Bytes bytes;
	XeenPartyState p;
	XeenCamera camera = XeenCamera{23,13,1,XeenDirection::North};
	XeenGameFlags flags;
	XeenEventFile event = regional_test::events(23);
	std::vector<XeenMonsterRecord> monsters = regional_test::statistics();
	std::function<void()> onMap;
	std::function<void()> onObjects;
	XeenWorld w{[this](XeenMapIdentity) { if (onMap) onMap(); return regionalMap(); },
		[this](XeenMapIdentity) { if (onObjects) onObjects(); return regionalObjects(); }};
	XeenEventPresenter::Clock clock = [] { return 0; };
	std::unique_ptr<XeenEncounterFlow> flow;
	explicit Fixture(Bytes data = chr(), const std::function<void(Fixture &)> &prepare = {},unsigned minutes=480) : bytes(std::move(data)), p(XeenPartyLoader().loadFromResources(bytes,pty())) {
		if (prepare) prepare(*this);
        auto saved=regional_test::snapshot();saved.camera=camera;saved.journey->context->minutes=minutes;
        saved.characters=p.roster.characters();saved.questItems=p.questItems.counts();
        saved.questFlags=p.questFlags.values();saved.gameFlags=flags.values();
        saved.disabledObjects.assign(w.sessionState().disabledObjects().begin(),w.sessionState().disabledObjects().end());
        saved.disabledEvents.assign(w.sessionState().disabledEvents().begin(),w.sessionState().disabledEvents().end());
        for(unsigned i=0;i<30;++i){
            saved.characters[i].learnedSpells=XeenCharacterFormat::parseLearnedSpells(bytes,i);
            saved.journey->supplements[i]={static_cast<std::uint8_t>(i),XeenCharacterFormat::parseCombatInputs(bytes,i,true,true,true)};
        }
        saved.journey->skeletonSeed=0;saved.journey->random=XeenJourneyRandomState{1,56,0};
        const auto actors=XeenActorApproach::actorsFromResources(regionalObjects(),monsters);
        for(unsigned i=0;i<19;++i){saved.journey->actors[i].x=actors[i].x;saved.journey->actors[i].y=actors[i].y;saved.journey->actors[i].activated=i==5;}
        auto r=regional_test::resources();r.loadEvents=[this](auto id){return id==XeenMapIdentity(23)?event:regional_test::events(id);};
        XeenSaveState::restoreBeforeGameplay(saved,r,p,camera,flags,w,[](auto &,const auto &,const auto &,const auto &){});
        flow=std::make_unique<XeenEncounterFlow>(w,p,camera,flags,clock,XeenJourneyRestoreTag{});
		check(!flow->journeyQuiet(), "initial frame lease must block");
		check(flow->prepareJourneyFrame(flow->ticket(),[] {}), "initial frame preparation");
		check(flow->presentJourney(flow->ticket()), "initial Journey frame");
	}
    const XeenActor &anchor() const {return w.sessionState().actors().at(5);}
    auto input(XeenEncounterAction a){action(a);return pulse();}
    void present() {
		if (w.sessionState().journeyActivity() == XeenJourneyActivity::Presentation) {
			check(flow->prepareJourneyFrame(flow->ticket(),[] {}), "headless frame preparation");
			check(flow->presentJourney(flow->ticket()), "headless frame handoff");
		}
	}
	XeenEncounterResult action(XeenEncounterAction a) { auto r = flow->journeyAction(flow->ticket(),a); present(); return r; }
	XeenEncounterResult pulse() { auto r = flow->journeyPulse(flow->ticket()); present(); return r; }
	void engage() {
		check(action(XeenEncounterAction::Wait).outcome == XeenEncounterOutcome::Engaged, "Journey genuine engagement");
		check(!flow->journeyQuiet() && !flow->combat(), "attachment remains blocked without combat");
		check(flow->attachJourney(flow->ticket(), [] {}), "Journey current attachment");
	}
	XeenCombatResult command(Command command) {
		auto *combat = flow->combat();
		auto result = combat->command(combat->ticket(),command);
		for (unsigned i = 0; combat->pending() == Work::Action && i < 100; ++i) result = combat->service(combat->ticket());
		return result;
	}
	void lethal() {
		for (unsigned i = 0; i < 200 && flow->combat()->phase() != Phase::VictoryAwaitingEnd; ++i) {
			auto *combat = flow->combat();
			if (combat->phase() == Phase::PlayerReady) command(Command::Attack);
			else check(combat->service(combat->ticket()).status != Status::Failed, "Journey service failed");
		}
		check(flow->combat()->phase() == Phase::VictoryAwaitingEnd, "genuine Journey lethal result");
	}
};
}
#endif
