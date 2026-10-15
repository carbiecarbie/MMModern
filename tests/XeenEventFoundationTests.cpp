#include "XeenRegionalSaveGameplayTestSupport.h"
#include "XeenCombatGameplayTestSupport.h"
#include "games/xeen/XeenStateEquality.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include <iostream>
using namespace mmodern;
using regional_test::check;
namespace {
XeenEventRecord record(unsigned line,unsigned opcode,std::vector<std::uint8_t> parameters={}) {
    XeenEventRecord r;r.x=20;r.y=20;r.direction=4;r.line=line;r.opcode=opcode;
    r.parameters=std::move(parameters);r.lengthField=5+r.parameters.size();r.fileOffset=line*10;return r;
}
struct Fixture : regional_save_test::Fixture {
    XeenEventFile script{30,"maze0030.evt",true,{}};
    unsigned unavailableLoads=0;
	void present(const SdlWindow::FrameUpdateHandler &handler) {
		handler.framePresented(flow->frame().presentation());
		handler.completeInputHandoff(flow->frame().presentation());
	}
	void send(const SdlWindow::FrameUpdateHandler &handler,const PlayerAction &action) {
		present(handler);handler.beginCycle(++cycle);
		handler.withDisplayedInput(action,*handler.displayedInput());present(handler);
	}
    auto services() {
        auto s=regional_save_test::Fixture::services();
        s.eventFixture=XeenCamera{30,20,20,XeenDirection::North};
        s.saveUnavailable="disabled in the Event developer session";
        s.maps=[this](auto id) {
            if (id==XeenMapIdentity(31)) ++unavailableLoads;
            auto map=regional_test::map(id);
            if (id!=XeenMapIdentity(23)) {
				map.heads.emplace();
                map.geometry.flags2=0;map.geometry.neighbors.fill(id.number);
                for(auto &cell:map.geometry.cells) {
                    cell.rawWord=0;cell.rawAttributes=1;cell.geometry=XeenIndoorWalls{};cell.surfaceIndex=1;
                }
            }
            return map;
        };
        s.resources.loadEvents=[this](auto id){return id==script.mapId?script:regional_test::events(id);};
        s.objects=[](auto id) {
            auto mob=regional_test::objects(id);
            if(id==XeenMapIdentity(30)) {
                mob.entities.monsters.push_back({100,100,0,0,8});
                mob.entities.monsters.push_back({100,100,0,15,-1});
                mob.entities.monsters.push_back({-128,-128,0,15,-1});
            }
            return mob;
        };
        s.texts=[](auto id) {
            auto text=regional_test::texts(id);text.resourceName=XeenEventTextLoader::resourceNameForMap(id);
            text.strings.resize(id==XeenMapIdentity(23)?44:64);return text;
        };
        return s;
    }
};
void sessionAndRefusal(bool publishedPrefix) {
    Fixture h;
    // XP's sparkle is a published script prefix. Quest flags after it remain
    // candidate-only and must be discarded when the travel operation refuses.
    if(publishedPrefix) h.script.records.push_back(record(0,12,{0,0,16,1,0,0,0}));
    else h.script.records.push_back(record(0,0x14,{100,5,0,0,0})); // Candidate-only random gold.
    const auto line=1u;
    h.script.records.push_back(record(line,0x0b,{0,6,6}));
    h.script.records.push_back(record(line+1,12,{0,0,104,3}));
    h.script.records.push_back(record(line+2,7,{31,1,1}));
    auto s=h.services();bool shown=false,prefixSeen=false;unsigned errors=0;
    s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &status) {
        shown=true;
        check(h.world->developerEventMap()==XeenMapIdentity(30) && h.camera->x==20 && h.camera->y==20,
            "Selected indoor map and 32x32 camera were not published");
        check(h.world->sessionState().hasRegionalActors(30) && h.world->sessionState().hasRegionalActors(23),
            "Selected map did not use complete staged owner tables");
        const auto actors=h.world->sessionState().regionalActors(30);
		h.flow->prepareJourneySprites();
        check(actors.size()==3 && actors[0].x==100 && actors[1].lifecycle==XeenActorLifecycle::Unresolved &&
            actors[2].lifecycle==XeenActorLifecycle::Disabled,"Original parked and unbound slots were changed");
        check(h.party->roster.at(0).permanentLevel==3 && h.party->encounterContext->day==8,
            "Developer prerequisites differ from the prepared Journey");
        check(!h.flow->canSave() && !h.world->journeyCaptureEligible(*h.party,*h.camera),"Developer session acquired save authority");
        bool rejected=false;try {static_cast<void>(h.capture());}catch(const std::exception &){rejected=true;}
        check(rejected,"Direct snapshot capture bypassed developer save refusal");
        h.send(handler,SaveGameAction{});
        check(status().find("Event developer session")!=std::string::npos,"F9 did not explain developer save refusal");
        auto before=h.world->transitionCandidate();
        const auto random=*h.world->sessionState().journeyRandom();
        const auto gold=h.party->monsterTreasure->gold;
        const auto xp=h.party->roster.combatInputs(0)->experience;
        h.flow->reportManual=[&](const auto &result) {
            if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&result))
                if(pending->request.kind==XeenPresentationKind::PortraitEffect) prefixSeen=true;
            if(const auto *failure=std::get_if<XeenEventExecutionError>(&result)) {
                ++errors;check(failure->kind==XeenEventExecutionErrorKind::UnsupportedTeleportDestination,
                    "Unavailable travel was reported as an integrity failure");
                check(failure->instructionCount==4,"Refusal did not execute the candidate RNG and map prefix");
            }
        };
        h.send(handler,InteractionAction{});
        for(unsigned n=0;h.flow->blocksGameplay() && n<300;++n) {
            h.now+=125;handler.beginCycle(++h.cycle);idle();h.present(handler);
        }
        if(errors!=1 || h.flow->blocksGameplay())throw std::runtime_error("Refused travel did not return ordinary session input: prefix="+
            std::to_string(publishedPrefix)+", errors="+std::to_string(errors)+", blocked="+
            std::to_string(h.flow->blocksGameplay())+", quiet="+std::to_string(h.flow->encounter()->journeyQuiet()));
        check(h.camera->mapId==XeenMapIdentity(30) && h.camera->x==20 && h.camera->y==20 &&
            !h.party->questFlags.isSet(3) && h.party->monsterTreasure->gold==gold && *h.world->sessionState().journeyRandom()==random &&
            xeen_state::sameMapStates(h.world->sessionState().mapStates(),before->sessionState().mapStates()) && !h.unavailableLoads,
            "Unavailable destination leaked candidate map, flag, party or random values");
        check(h.party->roster.combatInputs(0)->experience==xp+(publishedPrefix?1u:0u) && prefixSeen==publishedPrefix,
            "Refused travel lost or replayed an already published XP prefix");
        h.send(handler,NavigationAction::TurnRight);
        check(h.camera->direction==XeenDirection::East,"Developer session could not continue after refusal");
        return true;
    };
    check(Application().playGameplay(s,{}, {},false,XeenEncounterEntry::Journey,7)==0 && shown,
        "Developer session did not use the normal Application startup");
}
void integrity() {
    Fixture h;h.script.records={record(0,0x12)};auto s=h.services();bool shown=false;
    s.show=[&](const auto &,const auto &,const auto &,const auto &,const auto &){shown=true;return true;};
    const auto maps=s.maps;
    s.maps=[&](auto id){auto value=maps(id);if(id==XeenMapIdentity(30))value.geometry.id=31;return value;};
    check(Application().playGameplay(s,{}, {},false,XeenEncounterEntry::Journey,7)!=0 && !shown,
        "Selected-map admission bypassed resource identity validation");
    auto dark=h.services();dark.eventFixture->mapId={XeenSide::Darkside,30};
    check(Application().playGameplay(dark,{}, {},false,XeenEncounterEntry::Journey,7)!=0,"Developer session admitted Darkside");
    auto outside=h.services();outside.eventFixture->x=32;
    check(Application().playGameplay(outside,{}, {},false,XeenEncounterEntry::Journey,7)!=0,"Developer session admitted an invalid position");
    auto edge=h.services();edge.eventFixture=XeenCamera{30,15,8,XeenDirection::East};
    edge.maps=[&](auto id){auto map=maps(id);if(id==XeenMapIdentity(30))map.geometry.neighbors.fill(0);return map;};
    edge.objects=[](auto id) {
        auto mob=regional_test::objects(id);
        if(id==XeenMapIdentity(30))mob.entities.monsters.push_back({16,8,0,0,8});return mob;
    };
    check(Application().playGameplay(edge,{}, {},false,XeenEncounterEntry::Journey,7)!=0,
        "Startup published an activated monster outside the DAT footprint");
}
void originalSessions(const char *directory) {
    for(const XeenCamera start:{XeenCamera{27,10,10,XeenDirection::North},XeenCamera{49,0,0,XeenDirection::North}}) {
        combat_gameplay_test::Harness h{std::filesystem::path(directory)};auto s=h.services();
        XeenEventTextLoader texts([&](const std::string &name)->std::optional<regional_test::Bytes> {
            if(!h.assets->hasArchiveResource(name))return {};return h.assets->readArchiveResource(name);
        });
        s.texts=[&](auto id){return texts.load(id);};
        s.resources.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(h.assets->readInitialResource("maze.pty"));};
        s.resources.loadInitialBankBalances=[&]{return XeenCharacterFormat::parseBankBalances(h.assets->readInitialResource("maze.pty"));};
        s.resources.loadInitialWorldFlags=[&]{return XeenQuestFlagFormat::parseWorldFlags(h.assets->readInitialResource("maze.pty"));};
        s.resources.loadLearnedSpellNames=[&]{return XeenLearnedSpellNames::parse(*h.assets->readLearnedSpellNamesFromDarkArchive());};
        s.resources.regionalManifest=[&](const auto &m,const auto &o,const auto &e,const auto &statistics) {
            xeenValidateRegionalManifest(m,o,e,statistics,h.assets->readInitialResource("maze0023.dat"),
                h.assets->readInitialResource("maze0023.mob"),h.assets->readInitialResource("maze0023.evt"));
        };
        s.eventFixture=start;s.saveUnavailable="disabled in the Event developer session";bool shown=false;
        s.show=[&](const auto &frame,const auto &,const auto &,const auto &,const auto &) {
            shown=true;h.flow->prepareJourneySprites();
            check(frame.isValid() && h.camera->mapId==start.mapId && h.camera->x==start.x &&
                !h.flow->canSave(),"Original-data Application fixture did not render its selected map");return true;
        };
        check(Application().playGameplay(s,{}, {},false,XeenEncounterEntry::Journey,7)==0 && shown,
            "Original-data developer session startup failed");
    }
}
}
int main(int argc,char **argv) {
    std::ostringstream output;auto *stream=std::cout.rdbuf(output.rdbuf());
    try {sessionAndRefusal(false);sessionAndRefusal(true);integrity();if(argc==2)originalSessions(argv[1]);
        std::cout.rdbuf(stream);std::cout<<"Event developer session, unavailable travel, published prefix and integrity checks passed\n";return 0;
    }catch(const std::exception &error){std::cout.rdbuf(stream);std::cerr<<error.what()<<'\n';return 1;}
}
