// Opt-in, read-only original-resource rule evidence. Not SDL/runtime acceptance.
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenSaveState.h"
#include "app/XeenEncounterFlow.h"
#include "XeenRegionalOracle.h"
#include "XeenJourneyResourceTestSupport.h"
#include <iostream>
#include "platform/XeenSaveFile.h"
#include <stdexcept>
using namespace mmodern;
int main(int argc,char **argv) {
	try {
		if (argc!=2 && argc!=3) throw std::invalid_argument("usage: mmodern_regional_original <installation> [legacy-3-save]");
		const auto installation=XeenInstallationDetector().detect(argv[1]);
		if (!installation) throw std::runtime_error("Installation not found");
		XeenAssetSource assets(*installation);XeenMapLoader maps;
		const auto map=maps.loadGeometryMap(assets,23);
		const auto mob=maps.loadObjects(assets,23);
		XeenEventLoader loader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasInitialResource(name)) return {};return assets.readInitialResource(name);
		});
		const auto events=loader.load(23);
		const auto mon=assets.readCloudsMonsterStatisticsFromDarkArchive();
		if (!mon) throw std::runtime_error("MON missing");
		const auto statistics=XeenMonsterFormat::parse(*mon);
		xeenValidateRegionalManifest(map,mob,events,statistics,assets.readInitialResource("maze0023.dat"),
			assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));
		const auto component=XeenMovement::component(map,9,11,{});
		std::set<std::pair<int,int>> addresses;
		unsigned automaticCells=0;
		for (int y=0;y<16;++y) for(int x=0;x<16;++x) if(component[y*16+x]) {
			if(map.geometry.cells[y*16+x].rawAttributes&0x10) {++automaticCells;if(x!=5 || y!=9)throw std::runtime_error("Unknown mainland automatic trigger");}
			for(unsigned d=0;d<4;++d) {
				const XeenCamera c{23,x,y,static_cast<XeenDirection>(d)};
				const auto found=xeenRegionalEvent(events,c);
				if(found)addresses.insert({x,y});
				if(xeenRegionalSign(events,c)!=(x==5 && y==9 && d==0))throw std::runtime_error("Original event/facing admission mismatch");
			}
		}
		if(automaticCells!=1 || addresses!=std::set<std::pair<int,int>>{{0,1},{4,5},{5,9},{5,13},{7,7},{8,2},{8,10},{9,11},{10,13},{12,12}})
			throw std::runtime_error("Original mainland event addresses changed");
		if (component.count()!=121 || !component[2*16+8] || map.geometry.runX!=10 || map.geometry.runY!=12)
			throw std::runtime_error("Original mainland/Run metadata mismatch");
		const auto actors=XeenActorApproach::actorsFromResources(mob,statistics);
		if (actors.size()!=19) throw std::runtime_error("Original actor count mismatch");
		for (const auto &a:actors) {
			assets.validateNormalMonster(a.statistics->image());
			const auto closure=xeenActorClosure(map,a);
			for(int y=0;y<16;++y)for(int x=0;x<16;++x)
				if((xeenRegionalActorTerrain(map,a,x,y)==XeenMonsterTerrain::Allowed)!=regional_test::terrain(map,x,y))throw std::runtime_error("Original actor terrain/reference mismatch");
			std::cout << "Actor " << a.id.recordIndex << " type=" << a.original.resourceId << " spawn=" << a.x << ',' << a.y
				<< " HP=" << a.hp << " closure=" << closure.count() << '\n';
		}
		std::cout << "Original regional manifest, 121-cell mainland, Run (10,12), 19 actors and normal sprites passed\n";
		const auto chr=assets.readInitialResource("maze.chr");
		const auto pty=assets.readInitialResource("maze.pty");
		const auto context=XeenGameplayContextFormat::parse(pty);
		const auto purse=XeenCharacterFormat::parseMonsterPurse(pty);
		if (purse.gold!=800 || purse.gems!=10 || purse.pending()) throw std::runtime_error("Original purse mismatch");
		constexpr std::array<unsigned,6> activeResistances{7,10,2,5,7,0};
		for (unsigned owner=0;owner<30;++owner) {
			const auto input=XeenCharacterFormat::parseCombatInputs(chr,owner,true,true);
			if (!input.resistances || !input.luck) throw std::runtime_error("Missing original consequence input");
			const auto &r=*input.resistances;
			const auto pos=std::find(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),owner);
			if (pos!=kXeenCombatOwners.end()) {
				const auto expected=activeResistances[pos-kXeenCombatOwners.begin()];
				if (r.coldPermanent!=expected || r.electricalPermanent!=expected || r.coldTemporary || r.electricalTemporary)
					throw std::runtime_error("Original active resistances mismatch");
			}
			if (r.coldPermanent!=chr[354*owner+313] || r.coldTemporary!=chr[354*owner+314] ||
				r.electricalPermanent!=chr[354*owner+315] || r.electricalTemporary!=chr[354*owner+316])
				throw std::runtime_error("Original complete resistance inputs mismatch");
		}
		std::cout << "Original M33 purse and thirty resistance records passed (resource readers only)\n";
		const XeenRegionalManifest manifest=[&](const auto &m,const auto &o,const auto &e,const auto &s) {
			xeenValidateRegionalManifest(m,o,e,s,assets.readInitialResource("maze0023.dat"),assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));
		};

        if(argc==3) {
            auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
            XeenWorld world([&](auto id){return maps.loadGeometryMap(assets,id);},[&](auto id){return maps.loadObjects(assets,id);});
            auto camera=xeenJourneyContent(3).entry;XeenGameFlags flags;XeenEventPresenter::Clock clock=[]{return 0;};
            XeenEncounterFlow flow(world,party,camera,flags,clock,XeenJourneySetup{chr,context,statistics,events,1,3,manifest});
            if(!flow.prepareJourneyFrame(flow.ticket(),[]{}) || !flow.presentJourney(flow.ticket()))throw std::runtime_error("Legacy initial presentation");
            XeenSaveFile::write(XeenSaveFile::resolve(argv[2],argv[1]),XeenSaveState::capture(XeenSaveFile::fingerprint(*installation),party,camera,flags,world));
            std::cout<<"Explicit legacy 3/3 initial fixture saved\n";
        }
		journey_resources_test::run([&]{return XeenPartyLoader().loadInitialCloudsParty(assets);},
			XeenJourneySetup{chr,context,statistics,events,1,3,manifest},
			[&](auto id){return maps.loadGeometryMap(assets,id);},[&](auto id){return maps.loadObjects(assets,id);},
			XeenSaveResourceSignature{{1,2},XeenArchiveFingerprint{3,4}});
		std::cout << "312 fresh/restored regional resource renewal cases passed\n";
		for (const std::string route:{"F","RRFLFRFRFFRFRF","LFFRF","LFFFRFFFFRF"}) {
			auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
			XeenWorld world([&](auto id){return maps.loadGeometryMap(assets,id);},[&](auto id){return maps.loadObjects(assets,id);});
			auto camera=xeenJourneyContent(3).entry;XeenGameFlags flags;XeenEventPresenter::Clock clock=[]{return 0;};
			XeenEncounterFlow flow(world,party,camera,flags,clock,XeenJourneySetup{chr,context,statistics,events,1,3,manifest});
			const auto present=[&] {
				if (world.sessionState().journeyActivity()==XeenJourneyActivity::Presentation &&
					(!flow.prepareJourneyFrame(flow.ticket(),[]{}) || !flow.presentJourney(flow.ticket()))) throw std::runtime_error("Headless presentation failed");
			};
			present();
			regional_test::Oracle oracle(actors,camera);
			oracle.compare(world,camera,*party.encounterContext);
			for (char input:route) {
				const auto a=input=='F' ? XeenEncounterAction::Forward : input=='L' ? XeenEncounterAction::Left : XeenEncounterAction::Right;
				const auto r=flow.journeyAction(flow.ticket(),a);
				if (r.outcome==XeenEncounterOutcome::Refused) throw std::runtime_error("Regional action refused");
				oracle.action(input);oracle.compare(world,camera,*party.encounterContext);
				world.discardMapCache();
				flow.holdJourneyFrame();
				if(!flow.prepareJourneyFrame(flow.ticket(),[&]{(void)world.map(23);(void)world.objectFile(23);}) || !flow.presentJourney(flow.ticket()))throw std::runtime_error("Pending cache reconstruction failed");
				oracle.compare(world,camera,*party.encounterContext);
				unsigned pulses=0;
				do {
					if(flow.state().phase()!=XeenEncounterPhase::Exploring)break;
					oracle.pulse(map);flow.journeyPulse(flow.ticket());++pulses;
					oracle.compare(world,camera,*party.encounterContext);
					if(oracle.stopped!=(flow.state().phase()==XeenEncounterPhase::SupportStopped))throw std::runtime_error("Reference stop boundary mismatch");
				} while(flow.state().pending());
				if(oracle.stopped && route=="LFFRF" && (pulses!=3 || oracle.actors[9].x!=8 || oracle.actors[9].y!=12 || oracle.ctr24!=5))throw std::runtime_error("Third-pulse publication witness mismatch");
				present();
				world.discardMapCache();flow.holdJourneyFrame();
				if(!flow.prepareJourneyFrame(flow.ticket(),[&]{(void)world.map(23);(void)world.objectFile(23);}) || !flow.presentJourney(flow.ticket()))throw std::runtime_error("Settled cache reconstruction failed");
				oracle.compare(world,camera,*party.encounterContext);
				if (flow.state().phase()!=XeenEncounterPhase::Exploring) break;
			}
			std::cout << "Route " << route << " camera=" << camera.x << ',' << camera.y << ',' << unsigned(camera.direction)
				<< " minute=" << party.encounterContext->minutes << " ctr24=" << party.encounterContext->ctr24 << " phase=" << unsigned(flow.state().phase()) << '\n';
			if (route=="F") {
				if (!flow.journeyQuiet() || camera.x!=8 || camera.y!=11 || party.encounterContext->minutes!=490 ||
					world.sessionState().actors()[9].x!=6 || world.sessionState().actors()[9].y!=11) throw std::runtime_error("First quiet witness mismatch");
				const auto signature=XeenSaveResourceSignature{{1,2},XeenArchiveFingerprint{3,4}};
				const auto snapshot=XeenSaveState::capture(signature,party,camera,flags,world);
				const auto encoded=XeenSaveFormat::encode(snapshot);auto base=snapshot;base.journey.reset();
				if (encoded.size()-XeenSaveFormat::encode(base).size()!=1651 || XeenSaveFormat::encode(XeenSaveFormat::decode(encoded))!=encoded)
					throw std::runtime_error("Schema-3 codec mismatch");
				const auto restored=[&](const XeenSaveSnapshot &saved,bool advance=false,bool contact=false) {
					XeenPartyState p;XeenCamera c;XeenGameFlags f;
					XeenWorld w([&](auto id){return maps.loadGeometryMap(assets,id);},[&](auto id){return maps.loadObjects(assets,id);});
					XeenSaveState::Resources resources{signature,{},[&](auto id){return loader.load(id);},{},{},[&]{return statistics;},manifest};
					XeenSaveState::restoreBeforeGameplay(saved,resources,p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
					XeenEncounterFlow bound(w,p,c,f,clock,XeenJourneyRestoreTag{});
					if (!bound.prepareJourneyFrame(bound.ticket(),[]{}) || !bound.presentJourney(bound.ticket()) ||
						XeenSaveFormat::encode(XeenSaveState::capture(signature,p,c,f,w))!=XeenSaveFormat::encode(saved)) throw std::runtime_error("Exact regional restore mismatch");
					if (advance) {
						const auto before=*p.encounterContext;const auto r=bound.journeyAction(bound.ticket(),XeenEncounterAction::Wait);
						if (r.reason!=XeenEncounterStop::Time || !(*p.encounterContext==before)) throw std::runtime_error("Temporal boundary published state");
					}
					if (contact) {
						bound.journeyAction(bound.ticket(),XeenEncounterAction::Forward);
						const auto r=bound.journeyPulse(bound.ticket());
						if (r.reason!=XeenEncounterStop::RegionalContact || c.x!=7 || c.y!=11 || p.encounterContext->minutes!=500 ||
							bound.combat() || XeenSaveState::canCapture(p,c,w) || !w.sessionState().accountedMonsters().empty())
							throw std::runtime_error("Regional contact publication/terminal boundary mismatch");
					}
				};
				restored(snapshot);
				// Representation/restore controls: these wounds, dates and defeated values
				// are not claimed as M32-produced combat/calendar outcomes.
				auto represented=snapshot;represented.journey->actors[0].hp=1;represented.journey->context->day=42;
				represented.journey->context->year=611;represented.journey->context->minutes=1000;represented.journey->random->count=123;
				restored(represented);
				represented=snapshot;auto &defeated=represented.journey->actors[0];defeated.x=defeated.y=-128;defeated.hp=0;defeated.activated=false;defeated.lifecycle=XeenActorLifecycle::Defeated;defeated.accounted=true;
				restored(represented);
				represented=snapshot;represented.disabledObjects.push_back({23,0});represented.disabledEvents.push_back({23,56});represented.questItems[18]=7;
				restored(represented);
				represented=snapshot;represented.journey->context->minutes=950;restored(represented,true);
				for (unsigned id:{0U,12U,14U,17U,18U}) {
					represented=snapshot;auto &a=represented.journey->actors[id];a.x=7;a.y=11;a.activated=true;restored(represented,false,true);
				}
				represented=snapshot;
				for (unsigned id:{0U,12U,14U}) {auto &a=represented.journey->actors[id];a.x=7;a.y=11;a.activated=true;}
				restored(represented,false,true);
				const auto reject=[&](const XeenSaveSnapshot &bad) {
					bool failed=false;try {restored(bad);} catch(const std::exception &) {failed=true;}
					if(!failed)throw std::runtime_error("Invalid regional restore admitted");
				};
				for(unsigned mutation=0;mutation<12;++mutation) {
					auto bad=snapshot;auto &actor=bad.journey->actors[0];
					switch(mutation) {
					case 0:bad.camera.x=13;bad.camera.y=8;break;
					case 1:actor.hp=26;break;case 2:actor.hp=0;break;case 3:actor.accounted=true;break;
					case 4:actor.x=-128;break;case 5:actor.status=XeenActorStatus::Unsupported;break;
					case 6:actor.lifecycle=XeenActorLifecycle::Disabled;break;
					case 7:actor.x=8;actor.y=11;actor.activated=true;break;
					case 8:bad.journey->actors[9].activated=false;break;
					case 9:bad.journey->context->minutes=1260;break;
					case 10:bad.journey->context->newDay=true;break;
					case 11:bad.characters[0].conditions[3]=1;break;
					}reject(bad);
				}
				const auto r=flow.journeyAction(flow.ticket(),XeenEncounterAction::Wait);present();
				if (r.reason!=XeenEncounterStop::Ranged || party.encounterContext->minutes!=490 || flow.journeyQuiet() || XeenSaveState::canCapture(party,camera,world))
					throw std::runtime_error("Ranged stop must retain prior state and refuse capture");
			}
			if (route=="RRFLFRFRFFRFRF" && (!flow.journeyQuiet() || camera.x!=10 || camera.y!=11 || camera.direction!=XeenDirection::North || party.encounterContext->minutes!=550))
				throw std::runtime_error("Eastern quiet witness mismatch");
			if (route=="LFFRF" && (camera.x!=8 || camera.y!=9 || party.encounterContext->minutes!=510 || flow.state().reason()!=XeenEncounterStop::Ranged))
				throw std::runtime_error("Central frontier mismatch");
			if (route=="LFFFRFFFFRF" && (camera.x!=5 || camera.y!=9 || camera.direction!=XeenDirection::North || party.encounterContext->minutes!=560 || flow.journeyQuiet()))
				throw std::runtime_error("Sign geometry/live actor witness mismatch");
		}
		for(unsigned fault=0;fault<8;++fault) {
			auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
			bool changed=false;std::function<void()> onMap;
			XeenWorld world([&](auto id){if(onMap)onMap();auto m=maps.loadGeometryMap(assets,id);if(changed)m.geometry.runX=11;return m;},[&](auto id){return maps.loadObjects(assets,id);});
			auto camera=xeenJourneyContent(3).entry;XeenGameFlags flags;XeenEventPresenter::Clock clock=[]{return 0;};
			XeenEncounterFlow flow(world,party,camera,flags,clock,XeenJourneySetup{chr,context,statistics,events,1,3,manifest});
			if(!flow.prepareJourneyFrame(flow.ticket(),[]{}) || !flow.presentJourney(flow.ticket()))throw std::runtime_error("Fault fixture presentation");
			const std::vector<XeenActor> before=world.sessionState().actors();const auto time=*party.encounterContext;const auto old=flow.ticket();
			std::uint64_t lease=0;
			if(fault==0)const_cast<XeenMap &>(world.map(23)).geometry.runX=11;
			if(fault==1){changed=true;world.discardMapCache();}
			if(fault==2){world.discardMapCache();onMap=[]{throw std::runtime_error("Regional provider failure");};}
			if(fault==3 || fault==4){world.discardMapCache();onMap=[&]{lease=flow.boundary().hold(XeenCombatBoundary::Work::Inventory);if(fault==3)flow.boundary().release(XeenCombatBoundary::Work::Inventory,lease);};}
			if(fault==5){const auto hp=party.roster.at(0).currentHp;--party.roster.at(0).currentHp;if(flow.journeyQuiet())throw std::runtime_error("Unadopted mutation admitted");party.roster.at(0).currentHp=hp;}
			if(fault==6){flow.holdJourneyFrame();for(unsigned n=0;n<2;++n)if(flow.prepareJourneyFrame(flow.ticket(),[]{throw std::bad_alloc();}))throw std::runtime_error("Faulty presentation admitted");}
			if(fault==7)const_cast<XeenActor &>(world.sessionState().actors()[18]).statistics->raw[20]^=1;
			XeenEncounterResult result;
			try {result=flow.journeyAction(old,XeenEncounterAction::Forward);}
			catch(const std::logic_error &){if(fault!=1)throw; /* Changed reload now latches fatal integrity. */}
			if(result.outcome==XeenEncounterOutcome::Accepted || !(*party.encounterContext==time) || camera.x!=9 || camera.y!=11 || XeenSaveState::canCapture(party,camera,world))throw std::runtime_error("Fault published gameplay or permitted capture");
			for(unsigned i=0;i<19;++i)if(fault!=7 && !xeen_state::sameActor(before[i],world.sessionState().actors()[i]))throw std::runtime_error("Fault changed actors");
			if(fault==3 || fault==4) {
				if(flow.state().phase()!=XeenEncounterPhase::Exploring || (fault==4 && flow.boundary().quiet()))throw std::runtime_error("Obsolete work stopped or released replacement");
				onMap={};if(fault==4)flow.boundary().release(XeenCombatBoundary::Work::Inventory,lease);
			} else if(fault==0 || fault==1) {
				changed=false;world.discardMapCache();if(flow.journeyQuiet())throw std::runtime_error("Cache reconstruction revived invalidated owner");
			}
		}
		std::cout << "Regional original-resource schedules, restoration, cache and fault controls passed\n";
		{
			// Synthetic rule control: an explicit test-only manifest accepts an altered
			// in-memory automatic address. Production's byte manifest rejects it.
			auto alteredMap=map;alteredMap.geometry.cells[11*16+8].rawAttributes|=0x10;
			auto alteredEvents=events;alteredEvents.records[0].x=8;alteredEvents.records[0].y=11;alteredEvents.records[0].direction=4;alteredEvents.records[0].line=0;
			auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
			XeenWorld world([&](auto){return alteredMap;},[&](auto){return mob;});
			auto camera=xeenJourneyContent(3).entry;XeenGameFlags flags;XeenEventPresenter::Clock clock=[]{return 0;};
			auto sourceCharacters=chr;
			XeenJourneySetup setup{sourceCharacters,context,statistics,alteredEvents,1,3,{}};
			setup.regionalManifest=[&](const auto &,const auto &,const auto &,const auto &){setup.seed=7;setup.context.minutes=999;sourceCharacters.clear();};
			XeenEncounterFlow flow(world,party,camera,flags,clock,setup);
			if(world.sessionState().journeyRandom()->state!=1 || party.encounterContext->minutes!=480)throw std::runtime_error("Manifest callback replaced retained preparation arguments");
			if(!flow.prepareJourneyFrame(flow.ticket(),[]{}) || !flow.presentJourney(flow.ticket()))throw std::runtime_error("Synthetic automatic setup");
			const std::vector<XeenActor> before=world.sessionState().actors();const auto time=*party.encounterContext;
			const auto result=flow.journeyAction(flow.ticket(),XeenEncounterAction::Forward);
			if(result.outcome!=XeenEncounterOutcome::Refused || camera.x!=9 || !(*party.encounterContext==time))throw std::runtime_error("Unknown automatic chain published navigation");
			for(unsigned i=0;i<19;++i)if(!xeen_state::sameActor(before[i],world.sessionState().actors()[i]))throw std::runtime_error("Unknown automatic chain changed actors");
			std::cout << "Synthetic unknown automatic destination refused before publication\n";
		}
		return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n';return 1; }
}
