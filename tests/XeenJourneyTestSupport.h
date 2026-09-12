#ifndef MMODERN_JOURNEY_TEST_SUPPORT_H
#define MMODERN_JOURNEY_TEST_SUPPORT_H
#include "XeenCombatTestSupport.h"
#include "app/XeenEncounterFlow.h"
namespace journey_test {
using namespace combat_test;
struct Fixture {
	Bytes bytes;
	XeenPartyState p;
	XeenCamera camera = XeenActorApproach::kEntry;
	XeenGameFlags flags;
	XeenEventFile event = events();
	std::vector<XeenMonsterRecord> monsters = statistics();
	std::function<void()> onMap;
	std::function<void()> onObjects;
	XeenWorld w{[this](XeenMapIdentity) { if (onMap) onMap(); return map(); },
		[this](XeenMapIdentity) { if (onObjects) onObjects(); return objects(); }};
	XeenEventPresenter::Clock clock = [] { return 0; };
	std::unique_ptr<XeenEncounterFlow> flow;
	explicit Fixture(Bytes data = chr(), const std::function<void(Fixture &)> &prepare = {}) : bytes(std::move(data)), p(XeenPartyLoader().loadFromResources(bytes,pty())) {
		if (prepare) prepare(*this);
		flow = std::make_unique<XeenEncounterFlow>(w,p,camera,flags,clock,
			XeenJourneySetup{bytes,XeenGameplayContextFormat::parse(pty()),monsters,event,56});
		check(!flow->journeyQuiet(), "initial frame lease must block");
		check(flow->prepareJourneyFrame(flow->ticket(),[] {}), "initial frame preparation");
		check(flow->presentJourney(flow->ticket()), "initial Journey frame");
	}
	void present() {
		if (w.sessionState().journeyActivity() == XeenJourneyActivity::Presentation) {
			check(flow->prepareJourneyFrame(flow->ticket(),[] {}), "headless frame preparation");
			check(flow->presentJourney(flow->ticket()), "headless frame handoff");
		}
	}
	auto action(XeenEncounterAction a) { auto r = flow->journeyAction(flow->ticket(),a); present(); return r; }
	auto pulse() { auto r = flow->journeyPulse(flow->ticket()); present(); return r; }
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
