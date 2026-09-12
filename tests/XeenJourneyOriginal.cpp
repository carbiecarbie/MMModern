// Optional read-only original-resource validation; not an SDL/process acceptance witness.
#include "XeenCombatTestSupport.h"
#include "app/XeenEncounterFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "platform/XeenSaveFile.h"
#include "XeenRestoreReplayProbe.h"
#include <iostream>
using namespace combat_test;
int main(int argc, char **argv) {
	try {
		if (argc != 2) throw std::invalid_argument("usage: mmodern_journey_original <original-installation>");
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasDarkside(), "World of Xeen installation required");
		XeenAssetSource assets(*installation); XeenMapLoader maps;
		XeenEventLoader loader([&](const std::string &name) -> std::optional<Bytes> {
			if (!assets.hasInitialResource(name)) return {}; return assets.readInitialResource(name);
		});
		const auto characters = assets.readInitialResource("maze.chr");
		const auto context = XeenGameplayContextFormat::parse(assets.readInitialResource("maze.pty"));
		const auto rawMonsters = assets.readCloudsMonsterStatisticsFromDarkArchive();
		check(rawMonsters.has_value(), "original monster statistics required");
		const auto monsters = XeenMonsterFormat::parse(*rawMonsters);
		const auto event = loader.load(20);
		const auto signature = XeenSaveFile::fingerprint(*installation);
		const auto restore = [&](const XeenSaveSnapshot &saved, const std::vector<XeenActor> &expected, bool wait) {
			auto value = saved;
			for (unsigned pass=0;pass<3;++pass) {
				XeenWorld w([&](auto id){return maps.loadGeometryMap(assets,id);},[&](auto id){return maps.loadObjects(assets,id);});
				XeenPartyState p; XeenCamera c; XeenGameFlags f;
				XeenSaveState::Resources resources{signature,{},[&](auto id){return loader.load(id);},{},{},[&]{return monsters;}};
				XeenEventPresenter::Clock clock=[]{return 0;};
				std::unique_ptr<XeenEncounterFlow> restored;
				{
					replay_test::Scope scope;
					XeenSaveState::restoreBeforeGameplay(XeenSaveFormat::decode(XeenSaveFormat::encode(value)),resources,p,c,f,w,
						[&](auto &w,const auto &,const auto &c,const auto &) {
							assets.validateNormalMonster(8); assets.validateAttackMonster(8);
							check(!XeenOutdoorScene().build(w,c,nullptr,nullptr,0,XeenMonsterAppearance{0}).empty(),"original restored actor scene");
						});
					restored=std::make_unique<XeenEncounterFlow>(w,p,c,f,clock,XeenJourneyRestoreTag{});
					check(restored->prepareJourneyFrame(restored->ticket(),[] {})&&restored->presentJourney(restored->ticket()),"original restored binding handoff");
				}
				check(replay_test::unexpected==0,"original restore has no gameplay replay");
				const auto captured=XeenSaveState::capture(signature,p,c,f,w);
				check(XeenSaveFormat::encode(captured)==XeenSaveFormat::encode(value),"original all base/supplement/context/overlay/seed bytes exact");
				for(unsigned i=0;i<30;++i) {
					check(xeen_state::sameCharacter(saved.characters[i],p.roster.at(i)),"all original 30 base owners exact");
					check(xeen_state::sameInputs(saved.journey->supplements[i].inputs,*p.roster.combatInputs(i)),"all original 30 supplements exact");
				}
				sameActors(expected,w.sessionState().actors());
				if(wait&&pass==2)check(restored->journeyAction(restored->ticket(),XeenEncounterAction::Wait).outcome==XeenEncounterOutcome::Engaged&&
					p.encounterContext->minutes==500&&p.encounterContext->ctr24==3,"original moved restore explicit Wait delta");
				value=captured;
			}
		};
		{
			auto p=XeenPartyLoader().loadInitialCloudsParty(assets);auto c=XeenActorApproach::kEntry;XeenGameFlags f;
			XeenWorld w([&](auto id){return maps.loadGeometryMap(assets,id);},[&](auto id){return maps.loadObjects(assets,id);});
			XeenEventPresenter::Clock clock=[]{return 0;};
			XeenEncounterFlow flow(w,p,c,f,clock,XeenJourneySetup{characters,context,monsters,event,56});
			const auto present=[&]{check(flow.prepareJourneyFrame(flow.ticket(),[] {})&&flow.presentJourney(flow.ticket()),"original moved frame");};
			present(); flow.journeyAction(flow.ticket(),XeenEncounterAction::Right);present();
			flow.journeyAction(flow.ticket(),XeenEncounterAction::Forward);
			for(unsigned n=0;n<3;++n)flow.journeyPulse(flow.ticket());present();
			check(c.x==14&&c.y==1&&c.direction==XeenDirection::East&&p.encounterContext->minutes==490&&p.encounterContext->ctr24==2&&
				w.sessionState().actors()[5].x==13&&w.sessionState().actors()[5].y==1,"original moved source literal oracle");
			restore(XeenSaveState::capture(signature,p,c,f,w),w.sessionState().actors(),true);
		}
		auto party = XeenPartyLoader().loadInitialCloudsParty(assets);
		const auto before = party.roster.characters();
		XeenWorld world([&](XeenMapIdentity id) { return maps.loadGeometryMap(assets,id); },
			[&](XeenMapIdentity id) { return maps.loadObjects(assets,id); });
		auto camera = XeenActorApproach::kEntry; XeenGameFlags flags;
		XeenEventPresenter::Clock clock = [] { return 0; };
		XeenEncounterFlow flow(world,party,camera,flags,clock,XeenJourneySetup{characters,context,monsters,event,56});
		const auto present = [&] { return flow.prepareJourneyFrame(flow.ticket(),[] {}) && flow.presentJourney(flow.ticket()); };
		check(present(), "original initial handoff");
		const auto actors = world.sessionState().actors();
		check(actors.size() == 27, "complete original actor collection");
		for (unsigned id = 0; id < 30; ++id) {
			check(xeen_state::sameCharacter(before[id],party.roster.at(id)), "initialization preserves all original base fields");
			check(party.roster.combatInputs(id).has_value(), "complete original supplements");
		}
		check(flow.journeyTransfer(flow.ticket(),5,0,XeenInventoryCategory::Accessories,1).status == XeenTransferStatus::Success,
			"original pre-combat transfer");
		check(present(), "original transfer frame");
		check(flow.journeyEquipment(flow.ticket(),0,XeenInventoryCategory::Accessories,1,XeenEquipmentOperation::Equip).status ==
			XeenEquipmentStatus::Success, "original pre-combat equip");
		check(present(), "original equip frame");
		check(!flow.combat(), "no preparation combat owner");
		check(flow.journeyAction(flow.ticket(),XeenEncounterAction::Wait).outcome == XeenEncounterOutcome::Engaged, "original engagement");
		check(flow.attachJourney(flow.ticket(),[] {}), "original current attachment");
		unsigned commands = 0;
		for (unsigned n = 0; n < 200 && flow.combat()->phase() != Phase::Victory; ++n) {
			auto &combat = *flow.combat();
			if (combat.phase() == Phase::PlayerReady) { combat.command(combat.ticket(),commands++ < 6 ? Command::Block : Command::Attack); }
			else combat.service(combat.ticket());
		}
		check(commands == 8 && flow.combat()->phase() == Phase::Victory, "original seed56 action trace");
		check(flow.retireJourney(flow.ticket()) && present(), "original mutable retirement");
		const int hp[]{12,16,12,10,-17,5}, sp[]{2,0,2,0,7,9};
		for (unsigned i = 0; i < 6; ++i) {
			const auto &c = party.roster.at(kXeenCombatOwners[i]);
			check(c.currentHp == hp[i] && c.currentSp == sp[i] && party.roster.combatInputs(c.rosterId)->experience == (i == 4 ? 0u : 100u),
				"original literal HP/SP/XP oracle");
		}
		check(party.encounterContext->minutes == 492 && party.encounterContext->ctr24 == 1, "original literal End context");
		check(party.roster.at(1).conditions[12] == 1 && party.roster.at(1).conditions[13] == 1 &&
			xeenSameItem(party.roster.at(1).armor[0],{0,2,128,3}) && xeenSameItem(party.roster.at(1).armor[1],{38,10,128,9}),
			"original literal injury and broken armor");
		const auto defeated = world.sessionState().actors();
		restore(XeenSaveState::capture(signature,party,camera,flags,world),defeated,false);

		for (int y = 1; y <= 2; ++y) for (int x = 13; x <= 14; ++x) for (unsigned d = 0; d < 4; ++d) {
			world.discardMapCache();
			XeenActorApproach::validateEnvironment(world,world.sessionState().actors(),event);
			const auto view = XeenActorApproach::classify(world.sessionState().actors(),{20,x,y,static_cast<XeenDirection>(d)});
			check(!view.engaged(), "original defeated four-cell/facing isolation");
			for (unsigned i = 0; i < 27; ++i) {
				check(xeen_state::sameActor(defeated[i],world.sessionState().actors()[i]), "all original actor fields survive reconstruction");
				if (i != 5) check(xeen_state::sameActor(actors[i],defeated[i]), "original bystanders unchanged");
			}
		}
		std::cout << "Original Journey: moved and retired states restored three times each without replay; moved Wait minute500/ctr24=3; seed56 8 commands, minute492, exact 30 owners/supplements, 27 actors and 16 defeated views passed\n";
		return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
