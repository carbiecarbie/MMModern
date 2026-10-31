#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenJumpRules.h"
#include "games/xeen/XeenTerrainRules.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenRestoreGuard.h"
#include <iostream>
#include <map>
#include <tuple>

using namespace training_test;
namespace mmodern {
// Synthetic future-facing control: loaded actors normally arrive Physical.
// Force a recovery budget yield inside the existing retained border work.
struct XeenRestTestAccess {
    static void scenes(Fixture &f,const XeenObjectVisualResolver &resolver) {
        const auto draw=[&resolver](XeenWorld &w,const XeenPartyState &,const XeenCamera &c,std::uint64_t,XeenMonsterAppearance) {
            if(w.map(c.mapId).geometry.isOutdoors())XeenOutdoorScene().build(w,c,&resolver,nullptr,0);
            else XeenIndoorScene().build(w,c,&resolver,nullptr,0);
            return XeenEventFlow::Composition{frame(),false};
        };
        f.flow->_transitionCompose=draw;
        f.flow->_encounterCompose=[&f,draw](auto phase,auto appearance){return draw(f.w,f.p,f.c,phase,appearance);};
    }
    static void budgetCrossing(XeenEventFlow &f,const XeenCamera &to,const XeenEventFile &events) {
        auto &e=*f._encounter;auto work=std::make_unique<XeenRegionalActionCandidate>();auto &c=*work;
        c.destinationWorld=e._world.transitionCandidate();c.destinationFlags=c.destinationWorld->loadEventDestination(e._camera.mapId,to.mapId,true,e._flags);
        c.destinationEvents=events;c.camera=to;c.context=*e._party.encounterContext;c.owners=e._party.party.activeOrder();
        c.actors=c.destinationWorld->sessionState().regionalActors(to.mapId);c.random=XeenCombatRandom(*e._world.sessionState().journeyRandom());
        for(unsigned n=0;n<6;++n){c.characters[n]=e._party.roster.at(c.owners[n]);c.inputs[n]=*e._party.roster.combatInputs(c.owners[n]);}
        c.revision=e._state.revision();c.remaining=1;c.classify=true;c.result.outcome=XeenEncounterOutcome::Accepted;
        c.opportunity.emplace(c.destinationWorld->map(to.mapId),c.actors,to,c.characters,c.inputs,c.context.year,0x3f);
        c.opportunity->staged=false;
        for(auto &a:c.opportunity->actors)a.status=XeenActorStatus::Sleep;
        e._regionalWork=std::move(work);
    }
    static XeenEncounterResult pulse(XeenEventFlow &f) {
        auto &e=*f._encounter;auto result=e.journeyPulse(e.ticket());f._encounterFrame=e.ticket();return result;
    }
};
// Return control uses the real World load and Encounter publication owners.
struct XeenMonsterTestAccess {
    static void returnTo(Fixture &f,XeenCamera destination) {
        auto &flow=*f.flow;auto &encounter=*flow._encounter;
        check(encounter.journeyQuiet(),"Return requires a quiet boundary");
        encounter.journeySavePreimage().check();auto candidate=f.w.transitionCandidate();
        XeenPartyState party;candidate->copyEventParty(f.p,party);
        const auto flags=candidate->loadEventDestination(f.c.mapId,destination.mapId,true,f.f);
        auto events=f.in.events.load(destination.mapId);const auto arrival=candidate->prepareTransitionArrival(destination);
        auto prepared=std::make_shared<XeenRestoreGuard>(f.w,f.p,f.c,f.f);
        prepared->retainResources(encounter.journeySavePreimage());prepared->prepareSpellMapPublication(*candidate);
        prepared->cameraValue=destination;prepared->flagValues=flags.values();
        encounter.journeySavePreimage().check();f.w.checkTransitionTreasure(*candidate);
        f.w.publishTransition(*candidate);f.c=destination;f.f=flags;encounter._journeyEvents=std::move(events);
        encounter._journeyPreimage.swap(prepared);++encounter._generation;encounter._journeyPreimage->adoptMutationBoundary();
        encounter.publishArrival(arrival);encounter.holdJourneyFrame();flow._encounterFrame=encounter.ticket();
        f.present(flow.refresh(true));
    }
};
}
namespace {
using R=XeenLearnedSpellRules;
using S=XeenLearnedSpell;

// Local resource overrides exercise the real Flow without changing shared fixtures.
struct EventFixture {
    mmodern::XeenProgramState programState{mmodern::XeenWallClock::fixed(0)};
    Inputs &in;std::map<XeenMapIdentity,XeenEventFile> files;
    XeenWorld w;XeenPartyState p;XeenCamera c;XeenGameFlags f;XeenEventSystem events;
    std::unique_ptr<XeenEventFlow> flow;std::uint64_t cycle=0,now=0;
    XeenEventFile script(XeenMapIdentity id) {const auto it=files.find(id);return it==files.end()?in.events.load(id):it->second;}
    EventFixture(Inputs &input,const XeenSaveSnapshot &source,std::map<XeenMapIdentity,XeenEventFile> scripts,
        XeenWorld::MapLoader maps={},XeenWorld::ObjectLoader objects={}):in(input),files(std::move(scripts)),
        w(maps?maps:in.mapLoader(),objects?objects:in.objectLoader()),
        events([this](auto id){return XeenEventScript(script(id));},[this](auto id){return in.texts.load(id);}) {
        auto resources=in.resources();resources.loadEvents=[this](auto id){return script(id);};
        XeenSaveState::restoreBeforeGameplay(source,resources,p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
        flow=std::make_unique<XeenEventFlow>(programState,w,events,p,c,f,in.font,[](auto){return XeenEventFlow::Composition{frame(),false};},
            XeenEventPresenter::NpcDraw{},[this]{return now;},XeenEventPresenter::RandomFrame{},nullptr,
            [](auto,auto){return XeenEventFlow::Composition{frame(),false};},nullptr,
            [](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{frame(),false};});
        flow->dialogText=&in.assets.uiText();
        flow->drawDialogSprite=[this](auto &image,const char *resource,unsigned id,int x,int y){in.assets.drawDialogSprite(image,resource,id,x,y);};
        present(flow->frame());check(flow->canSave(),"Opening Flow fixture did not restore quietly");
    }
    void present(const IndexedFrame &image){if(image.presentation())flow->framePresented(image.presentation());}
    void act(PlayerAction action){flow->beginCycle(++cycle);present(flow->handle(action,flow->displayedInput(),flow->frame().presentation()));}
    XeenSaveSnapshot snapshot(){return XeenSaveState::capture(in.signature,p,c,f,w);}
};
XeenEventRecord row(const XeenCamera &c,unsigned line,unsigned op,std::vector<std::uint8_t> parameters={}) {
    XeenEventRecord r;r.x=c.x;r.y=c.y;r.direction=4;r.line=line;r.opcode=op;
    r.parameters=std::move(parameters);r.lengthField=5+r.parameters.size();r.fileOffset=line*12;return r;
}

void table(Inputs &in,XeenSaveSnapshot &s,XeenMapIdentity id) {
    if(s.find(id) && s.find(id)->monsters)return;
    const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,id),in.statistics);
    auto &saved=s.map(id).monsters;saved.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
    for(const auto &a:actors)saved->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
}
XeenCamera quietCell(Inputs &in,XeenMapIdentity id,bool extension=false) {
    XeenWorld w(in.mapLoader(),in.objectLoader());const auto &g=w.map(id).geometry;
    const auto actors=XeenActorApproach::actorsFromResources(w.objectFile(id),in.statistics);
    const int limit=g.isOutdoors()?16:32;
    for(int y=0;y<limit;++y)for(int x=extension?16:0;x<limit;++x) {
        // Part 0: (0,0) North deliberately rejects all Event records.
        if(x==0 && y==0)continue;
        const XeenCamera c{id,x,y,XeenDirection::North};
        const auto cell=w.sampleCell(id,x,y);if(!cell)continue;
        if(g.isOutdoors() && XeenMovement::outdoorDestination(g,*cell->cell,{true,true,true,true},id.side)!=XeenMovementResult::Moved)continue;
        const auto view=g.isOutdoors()?XeenActorApproach::classify(actors,c):XeenIndoorScene().classifyActors(w,c,actors);
        if(!view.engaged())return c;
    }
    throw std::runtime_error("No quiet original location in map "+std::to_string(id.number));
}
void admission(Inputs &in) {
    XeenWorld w(in.mapLoader(),in.objectLoader());unsigned primary=0,extensions=0;
    w.markEncounterSession(XeenEncounterEntry::Journey);
    for(unsigned id=1;id<100;++id) {
        char name[16];std::snprintf(name,sizeof(name),"maze%04u.dat",id);
        if(!in.assets.hasInitialResource(name))continue;
        ++primary;const auto c=quietCell(in,id);
        check(bool(w.prepareEventTransition(c)),"Resource-backed Clouds primary map refused");
        auto invalid=c;invalid.x=w.map(id).geometry.isOutdoors()?16:32;
        check(!w.prepareEventTransition(invalid),"Coordinate beyond logical capacity admitted");
        if(!w.map(id).geometry.isOutdoors())for(unsigned d:{0u,1u})if(const auto tile=w.map(id).geometry.neighbors[d]) {
            ++extensions;check(!w.prepareEventTransition({XeenMapIdentity(tile),0,0,XeenDirection::North}),"Physical indoor tile admitted as a primary destination");
        }
    }
    check(primary>32 && extensions>0,"Opening inventory lost original primary maps or extensions");
    check(w.prepareEventTransition({XeenMapIdentity(XeenSide::Darkside,28),1,1,XeenDirection::North}).status==XeenTransitionResult::Status::Available,"Ordinary Darkside map refused");
    check(!w.prepareEventTransition({XeenMapIdentity(XeenSide::Darkside,113),1,1,XeenDirection::North}),"Mixed Darkside map admitted");
    check(!w.prepareEventTransition({99,1,1,XeenDirection::North}),"Absent primary admitted");
    std::cout<<"Opening primary maps="<<primary<<" indoor extension links="<<extensions<<'\n';
}
void roundtrip(Inputs &in) {
    auto s=in.base();s.maps.clear();
    // A logical multi-tile interior, a town and an outdoor map share the same
    // existing wire records. No map-23 actor table or manifest is required.
    std::optional<XeenCamera> interior;
    for(unsigned id=29;id<85 && !interior;++id) {
        try {
            const auto m=in.maps.loadGeometryMap(in.assets,id);
            if(!m.geometry.isOutdoors() && m.geometry.neighbors[1])interior=quietCell(in,id,true);
        } catch(const std::runtime_error &) {}
    }
    check(bool(interior),"No original multi-tile interior found");
    for(auto id:{XeenMapIdentity(24),XeenMapIdentity(30),XeenMapIdentity(interior->mapId)})table(in,s,id);
    s.camera=*interior;s.journey->context->lightAndResistances[0]=7;
    XeenWorld lookup(in.mapLoader(),in.objectLoader());const auto cell=lookup.sampleCell(s.camera.mapId,s.camera.x,s.camera.y);
    check(cell && cell->mapId!=s.camera.mapId,"Multi-tile camera lost logical identity");
    s.map(cell->mapId).explored.emplace();s.map(cell->mapId).explored->at(0)=1;
    const auto evt=in.events.load(s.camera.mapId);
    if(!evt.records.empty())s.map(s.camera.mapId).events.push_back({0,0});
    const auto bytes=XeenSaveFormat::encode(s);
    Fixture f(in,XeenSaveFormat::decode(bytes));const auto restored=f.snapshot();
    check(!restored.find(23) && XeenSaveFormat::encode(restored)==bytes,"Generic multi-map restore replayed Event, reward, time, light or RNG");
    Fixture second(in,XeenSaveFormat::decode(XeenSaveFormat::encode(restored)));
    check(XeenSaveFormat::encode(second.snapshot())==bytes,"Second multi-map reload changed persistent state");
    std::cout<<"Roundtrip logical="<<s.camera.mapId<<" physical="<<cell->mapId<<" position="<<s.camera.x<<','<<s.camera.y<<'\n';
}
void hiddenSwitchRoundTrips(Inputs &in) {
    const auto resolver=XeenObjectVisualResolver::load(in.assets);
    const auto scenes=[&](Fixture &f) {
        XeenRestTestAccess::scenes(f,resolver);f.present(f.flow->refresh(true));
    };
    const auto interact=[](Fixture &f) {
        unsigned completed=0,failures=0;
        f.flow->reportManual=[&](const auto &result) {
            completed+=std::holds_alternative<XeenManualEventCompleted>(result);
            failures+=std::holds_alternative<XeenEventExecutionError>(result);
        };
        f.act(InteractionAction{});
        for(unsigned n=0;n<1000 && !f.flow->canSave();++n) {
            f.now+=25;f.flow->beginCycle(++f.cycle);
            if(auto image=f.flow->updatePresentation())f.present(*image);
            const auto input=f.flow->inputContext(f.flow->frame().presentation());
            if(input.readyForAction && input.dialog && input.dialog->anyKey)f.act(AcknowledgeAction{});
        }
        f.flow->reportManual={};
        check(completed==1 && !failures && f.flow->canSave(),"Map 81 switch did not complete at Quiet");
    };
    const auto turn=[](Fixture &f,NavigationAction action,XeenDirection expected) {
        f.act(action);
        for(unsigned n=0;n<1000 && !f.flow->canSave();++n) {
            f.now+=25;f.flow->beginCycle(++f.cycle);
            if(auto image=f.flow->updatePresentation())f.present(*image);
            const auto input=f.flow->inputContext(f.flow->frame().presentation());
            if(input.readyForAction && input.dialog && input.dialog->anyKey)f.act(AcknowledgeAction{});
        }
        check(f.flow->canSave() && f.c.direction==expected,"Map 81 turn did not finish at Quiet");
    };
    const auto objects=[](XeenWorld &world) {
        std::vector<std::tuple<int,int,bool>> values;
        const auto count=world.objectFile(81).entities.objects.size();
        for(std::size_t i=0;i<count;++i) {
            const XeenObjectIdentity id{81,i};const auto object=world.effectiveObject(id);
            const bool removed=world.sessionState().isObjectDisabled(id);
            values.emplace_back(removed?-128:int(object.x),removed?-128:int(object.y),world.effectiveObjectFlip(id));
        }
        return values;
    };
    for(int y:{23,26,29}) {
        auto source=in.base();source.camera={81,1,y,XeenDirection::West};
        Fixture live(in,source);scenes(live);interact(live);
        const auto rng=live.w.sessionState().journeyRandom();
        const auto skeleton=live.w.sessionState().skeletonSeed();
        const auto snapshot=live.snapshot();const auto *map=snapshot.find(81);
        check(map && std::any_of(map->objects.begin(),map->objects.end(),[](const auto &v) {
            return v.x==-128 && v.y==-128 && v.flipped;
        }),"Original switch did not produce a hidden flipped MoveObj");
        const auto bytes=XeenSaveFormat::encode(snapshot);
        Fixture restored(in,XeenSaveFormat::decode(bytes));
        check(live.w.sessionState().journeyRandom()==rng && restored.w.sessionState().journeyRandom()==rng &&
            live.w.sessionState().skeletonSeed()==skeleton && restored.w.sessionState().skeletonSeed()==skeleton &&
            XeenSaveFormat::encode(restored.snapshot())==bytes,
            "Original switch restore changed save bytes or RNG");
        scenes(restored);scenes(live);
        check(objects(live.w)==objects(restored.w),"Original switch objects differ after the scene draw");
        if(y==23) {
            Fixture north(in,XeenSaveFormat::decode(bytes));scenes(north);
            turn(north,NavigationAction::TurnRight,XeenDirection::North);
            const auto *northMap=north.w.sessionState().mapState(81);
            check(northMap && std::any_of(northMap->objectOverrides.begin(),northMap->objectOverrides.end(),[&](const auto &v) {
                return v.x==-128 && v.y==-128 && !north.w.effectiveObjectFlip(v.id);
            }),"North draw did not produce a hidden unflipped MoveObj");
            const auto northBytes=XeenSaveFormat::encode(north.snapshot());
            Fixture northRestored(in,XeenSaveFormat::decode(northBytes));
            check(XeenSaveFormat::encode(northRestored.snapshot())==northBytes &&
                north.w.sessionState().journeyRandom()==northRestored.w.sessionState().journeyRandom(),
                "North-facing hidden MoveObj did not round-trip with its RNG");
            check(std::any_of(northMap->objectOverrides.begin(),northMap->objectOverrides.end(),[&](const auto &v) {
                return v.x==-128 && v.y==-128 && !north.w.effectiveObjectFlip(v.id) &&
                    northRestored.w.sessionState().isObjectDisabled(v.id);
            }),"Hidden unflipped MoveObj was not restored as Remove");
            scenes(northRestored);
            turn(north,NavigationAction::TurnLeft,XeenDirection::West);
            turn(northRestored,NavigationAction::TurnLeft,XeenDirection::West);
            interact(north);interact(northRestored);
            check(objects(north.w)==objects(northRestored.w) &&
                north.w.sessionState().journeyRandom()==northRestored.w.sessionState().journeyRandom() &&
                XeenSaveFormat::encode(north.snapshot())==XeenSaveFormat::encode(northRestored.snapshot()),
                "Switch after an unflipped hidden MoveObj restore changed objects, RNG or save bytes");

            Fixture travel(in,XeenSaveFormat::decode(bytes));scenes(travel);
            const auto beforeTravel=objects(travel.w);
            const auto departure=travel.flow->mrWizard();check(bool(departure),"Mr Wizard refused departure from map 81");
            travel.present(*departure);
            for(unsigned n=0;n<1000 && !travel.flow->canSave();++n) {
                travel.now+=25;travel.flow->beginCycle(++travel.cycle);
                if(auto image=travel.flow->updatePresentation())travel.present(*image);
            }
            check(travel.flow->canSave() && travel.c.mapId!=XeenMapIdentity(81),"Departure did not reach Quiet elsewhere");
            const auto awayRng=travel.w.sessionState().journeyRandom();const auto away=XeenSaveFormat::encode(travel.snapshot());
            Fixture returned(in,XeenSaveFormat::decode(away));
            check(returned.w.sessionState().journeyRandom()==awayRng && XeenSaveFormat::encode(returned.snapshot())==away,
                "Saving away from map 81 changed bytes or RNG on restore");
            XeenRestTestAccess::scenes(returned,resolver);
            const XeenCamera destination{81,1,y,XeenDirection::West};
            XeenMonsterTestAccess::returnTo(travel,destination);XeenMonsterTestAccess::returnTo(returned,destination);
            const auto afterTravel=objects(travel.w);
            check(beforeTravel.size()==afterTravel.size() && std::equal(beforeTravel.begin(),beforeTravel.end(),afterTravel.begin(),
                [](const auto &a,const auto &b){return std::get<0>(a)==std::get<0>(b) && std::get<1>(a)==std::get<1>(b);}),
                "I5 switch object positions changed after departure and return");
            check(objects(travel.w)==objects(returned.w) && travel.w.sessionState().journeyRandom()==returned.w.sessionState().journeyRandom(),
                "Returned map 81 objects or RNG differ after the scene draw");
            interact(travel);interact(returned);
            check(objects(travel.w)==objects(returned.w) && travel.w.sessionState().journeyRandom()==returned.w.sessionState().journeyRandom(),
                "Switch after departure and restore changed object positions, flips or RNG");
        }
        interact(live);interact(restored);
        check(objects(live.w)==objects(restored.w) && live.w.sessionState().journeyRandom()==restored.w.sessionState().journeyRandom() &&
            live.w.sessionState().skeletonSeed()==restored.w.sessionState().skeletonSeed() &&
            XeenSaveFormat::encode(live.snapshot())==XeenSaveFormat::encode(restored.snapshot()),
            "Next original switch changed object positions, flips or RNG after restore");
    }
}
void travel(Inputs &in) {
    XeenWorld w(in.mapLoader(),in.objectLoader());const XeenCamera from{24,8,8,XeenDirection::East};
    for(unsigned n=1;n<=5;++n) {
        const auto to=R::travel(w,from,S::TownPortal,n,nullptr,false);
        const auto &g=w.map(27+n).geometry;
        check(!to.unsupported && to.reload && to.destination && to.destination->mapId==XeenMapIdentity(27+n) &&
            to.destination->x==g.runX && to.destination->y==g.runY && to.destination->direction==from.direction,"Original Town Portal destination differs");
        XeenCharacter caster;caster.lloydMap=27+n;caster.lloydX=g.runX;caster.lloydY=g.runY;
        const auto beacon=R::travel(w,from,S::LloydsBeacon,2,&caster,false);
        check(beacon.destination && !beacon.unsupported && beacon.reload,"Beacon Return refused an open town");
        const auto set=R::travel(w,*beacon.destination,S::LloydsBeacon,1,&caster,false);
        check(set.beacon && set.beacon->at(0)==27+n && !set.destination,"Beacon Set lost the new map");
    }
    const auto c=quietCell(in,33);const auto run=xeenRunDestination(w,in.events.load(33),c);
    if(run) {
        const auto distortion=R::travel(w,c,S::TimeDistortion,0,nullptr,false);
        check(distortion.destination && distortion.destination->x==run->x && distortion.destination->y==run->y && !distortion.reload,"Indoor Run and Time Distortion disagree");
    }
    // Every outdoor edge is resolved from DAT neighbors, without changing an
    // interior's logical map when its sampling crosses a physical tile seam.
    unsigned edges=0;
    for(unsigned id=1;id<=24;++id)for(unsigned d=0;d<4;++d) {
        const auto &g=w.map(id).geometry;if(!g.neighbors[d])continue;
        XeenCamera c{id,d==1?15:d==3?0:8,d==0?15:d==2?0:8,XeenDirection(d)};
        const auto old=c;const auto moved=XeenMovement().apply(w,c,NavigationAction::MoveForward,{true,true,true,true});
        if(moved!=XeenMovementResult::Moved)continue;
        ++edges;check(c.mapId==XeenMapIdentity(g.neighbors[d]) && w.prepareEventTransition(c),"Outdoor edge did not resolve its primary neighbor");
        const auto teleport=R::travel(w,old,S::Teleport,1,nullptr,false);
        if(!teleport.failed)check(teleport.destination && teleport.reload && teleport.destination->mapId==c.mapId && teleport.destination->x==c.x && teleport.destination->y==c.y,"Teleport edge normalization differs from movement");
    }
    check(edges>4,"No original outdoor border witnesses");
}

template<class F> void quiet(F &f) {
    for(unsigned n=0;n<2000 && !f.flow->canSave();++n) {
        check(!f.flow->encounter()->combat(),"Opening route unexpectedly entered combat");
        if(f.flow->presentationGeneration())f.act(AcknowledgeAction{});
        f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);
    }
    check(f.flow->canSave(),"Opening route did not settle");
}

void crossing(Inputs &in) {
    XeenWorld w(in.mapLoader(),in.objectLoader());std::optional<XeenCamera> start;
    for(unsigned id=1;id<=24 && !start;++id)for(unsigned d=0;d<4 && !start;++d)for(int edge=0;edge<16 && !start;++edge) {
        const XeenCamera from{id,d==1?15:d==3?0:edge,d==0?15:d==2?0:edge,XeenDirection(d)};auto to=from;
        if(XeenMovement().apply(w,to,NavigationAction::MoveForward,{true,true,true,true})!=XeenMovementResult::Moved || to.mapId==from.mapId)continue;
        const auto cell=w.sampleCell(to.mapId,to.x,to.y);
        const auto terrain=xeenTerrainArrival(cell->geometry->surfaceTypes[cell->cell->surfaceIndex],true,false,false,to);
        if(terrain.damage || terrain.falling || terrain.dead || terrain.minutes || (cell->cell->rawAttributes&kXeenAutomaticEventFlag))continue;
        bool contact=false;
        for(const auto &c:{from,to})contact=contact || XeenActorApproach::classify(XeenActorApproach::actorsFromResources(w.objectFile(c.mapId),in.statistics),c).engaged();
        if(!contact)start=from;
    }
    check(bool(start),"No quiet original outdoor border witness");
    auto source=in.base();source.camera=*start;table(in,source,start->mapId);
    auto expected=*start;check(XeenMovement().apply(w,expected,NavigationAction::MoveForward,{true,true,true,true})==XeenMovementResult::Moved,"Original border witness changed");
    for(auto owner:source.activeRosterIds)for(unsigned skill:{9u,11u,14u})source.characters[owner].skills[skill]=1;
    Fixture f(in,source);const auto time=*f.p.encounterContext;const auto random=f.w.sessionState().journeyRandom();
    // A drawn chest frame must follow the same departure lifetime as Event travel.
    const XeenObjectIdentity departed{start->mapId,0},retained{expected.mapId,0};
    f.w.objectFrameDrawn(departed,1);f.w.objectFlipDrawn(departed,true);
    f.w.objectFrameDrawn(retained,2);f.w.objectFlipDrawn(retained,false);
    f.act(NavigationAction::MoveForward);
    check(f.p.encounterContext->minutes==time.minutes+10 && f.w.sessionState().journeyRandom()==random,"Crossing changed original time or drew unexpected RNG");
    quiet(f);
    check(xeen_state::sameCamera(f.c,expected),"Live crossing failed to publish normalized camera");
    check(f.w.sessionState().hasRegionalActors(start->mapId) && f.w.sessionState().hasRegionalActors(expected.mapId),"Crossing lost the departed table or omitted destination actors");
    check(!f.w.objectFrame(departed) && f.w.objectFrame(retained)==2 &&
        f.w.scenePresentation().objectFlips.at(departed) && !f.w.scenePresentation().objectFlips.at(retained),
        "Crossing discarded candidate object frames/flips or retained a departed chest frame");
    const auto bytes=XeenSaveFormat::encode(f.snapshot());Fixture loaded(in,XeenSaveFormat::decode(bytes));
    check(bytes==XeenSaveFormat::encode(loaded.snapshot()),"Outdoor crossing restore replayed arrival");
    f.act(NavigationAction::MoveBackward);quiet(f);
    check(xeen_state::sameCamera(f.c,*start) && !f.w.objectFrame(departed) && !f.w.objectFrame(retained),
        "Walking back restored a stale opened-chest frame");
    std::cout<<"Live crossing "<<start->mapId<<" -> "<<expected.mapId<<" edge="<<start->x<<","<<start->y<<'\n';
}

void flowRefusals(Inputs &in) {
    for(unsigned action:{3u,6u,8u,9u,10u,11u,13u,255u,256u}) {
        const bool neighbor=action==256;
        auto source=in.base();source.camera=neighbor?XeenCamera{23,9,11,XeenDirection::East}:quietCell(in,29);
        table(in,source,source.camera.mapId);
        const auto c=source.camera;
        const auto first=row(c,0,0x1b,{34,123,0,0,0});
        const auto unsupported=neighbor?row(c,1,35,{16,0,1}):row(c,1,0x11,{std::uint8_t(action)});
        const auto id=c.mapId;EventFixture f(in,source,{{id,{id,XeenEventLoader::resourceNameForMap(id),true,{first,unsupported}}}});
        unsigned errors=0;f.flow->reportManual=[&](const auto &result) {
            if(const auto *error=std::get_if<XeenEventExecutionError>(&result)) {
                ++errors;check(error->kind==XeenEventExecutionErrorKind::UnsupportedOperand,"Flow refusal became a preparation error");
            }
        };
        const auto before=XeenSaveFormat::encode(f.snapshot());
        // Part 0: click has no surviving Wait; a flagged cell still gets its final call.
        const bool flagged=f.w.sampleCell(c.mapId,c.x,c.y)->cell->rawAttributes&kXeenAutomaticEventFlag;
        f.act(InteractionAction{false});quiet(f);
        check(errors==(flagged?2u:1u) && f.flow->encounter()->notice().find("not supported yet")!=std::string::npos,
            "Unsupported operand did not reach the Flow notice");
        check(before==XeenSaveFormat::encode(f.snapshot()),"Flow refusal published a prelude, time, RNG or map change");
    }
}

void crossingArrivals(Inputs &in) {
    // Controlled arrivals on original DAT topology isolate contact, automatic
    // Events and a fall, rather than relying on a conveniently quiet border.
    const XeenCamera from{2,15,0,XeenDirection::East},to{6,0,0,XeenDirection::East};
    for(unsigned kind=0;kind<3;++kind) {
        auto source=in.base();source.camera=from;table(in,source,2);
        for(auto &map:source.maps)if(map.monsters)for(auto &a:map.monsters->slots) {
            a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
        }
        for(auto owner:source.activeRosterIds) {
            source.characters[owner].currentHp=1000;
            for(unsigned skill:{9u,11u,14u})source.characters[owner].skills[skill]=1;
        }
        source.journey->context->effects.fill(0);
        const auto maps=[&](auto id) {
            auto map=in.maps.loadGeometryMap(in.assets,id);
            if(id==to.mapId) {
                map.geometry.cells[0].rawAttributes=kind==1?kXeenAutomaticEventFlag:0;
                if(kind==2)map.geometry.surfaceTypes[map.geometry.cells[0].surfaceIndex]=10;
            }
            return map;
        };
        const auto objects=[&](auto id) {
            auto mob=in.maps.loadObjects(in.assets,id);
            if(id==to.mapId) {
                if(kind==0) {
                    auto it=std::find_if(mob.entities.monsters.begin(),mob.entities.monsters.end(),[](const auto &a){return a.isActive();});
                    check(it!=mob.entities.monsters.end(),"Contact arrival has no original monster");it->x=it->y=0;
                } else {mob.resourcePresent=false;mob.entities={};}
            }
            return mob;
        };
        const XeenEventFile automatic{6,"maze0006.evt",true,{row(to,0,0x1b,{34,123,0,0,0}),row(to,1,0x12)}};
        EventFixture f(in,source,kind==1?std::map<XeenMapIdentity,XeenEventFile>{{6,automatic}}:std::map<XeenMapIdentity,XeenEventFile>{},maps,objects);
        unsigned completed=0;f.flow->reportManual=[&](const auto &result){if(std::holds_alternative<XeenManualEventCompleted>(result))++completed;};
        const auto random=f.w.sessionState().journeyRandom();const auto minutes=f.p.encounterContext->minutes;
        f.act(NavigationAction::MoveForward);
        if(kind==0) {
            for(unsigned n=0;n<100 && !f.flow->encounter()->combat();++n) {
                f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);
            }
            check(f.flow->encounter()->combat() && xeen_state::sameCamera(f.c,to) && f.w.sessionState().hasRegionalActors(6),
                "Contact arrival did not publish destination actors before entering combat");
        } else {
            quiet(f);
            check(f.w.sessionState().regionalActors(6).empty(),"Missing optional MOB did not stage an empty table");
            if(kind==1)check(xeen_state::sameCamera(f.c,to) && completed==1 && f.p.purse->gold==123,
                "Border automatic Event did not execute exactly once in its destination");
            else {
                check(f.c.mapId==XeenMapIdentity(23) && f.c.x==12 && f.c.y==10 && f.c.direction==XeenDirection::North,
                    "Border terrain fall did not reach the original fall destination");
                for(auto owner:source.activeRosterIds)check(f.p.roster.at(owner).currentHp==900,"Border fall damage was lost or replayed");
            }
            check(f.p.encounterContext->minutes==minutes+10 && f.w.sessionState().journeyRandom()==random,
                "Border arrival repeated time or consumed unexpected RNG");
            const auto bytes=XeenSaveFormat::encode(f.snapshot());EventFixture loaded(in,XeenSaveFormat::decode(bytes),{},maps,objects);
            check(bytes==XeenSaveFormat::encode(loaded.snapshot()),"Empty-table arrival restore replayed an Event, fall, time or RNG");
        }
    }
}

void crossingBudget(Inputs &in) {
    auto source=in.base();source.camera={2,15,0,XeenDirection::East};table(in,source,2);
    const XeenCamera to{6,0,0,XeenDirection::East};
    const auto objects=[&](auto id) {
        auto mob=in.maps.loadObjects(in.assets,id);
        if(id==to.mapId) {
            mob.entities.monsters.clear();
            for(unsigned n=0;n<107;++n)mob.entities.monsters.push_back({15,15,0,0,0});
        }
        return mob;
    };
    EventFixture f(in,source,{}, {},objects);f.w.objectFrameDrawn({2,0},1);
    const auto before=f.w.transitionCandidate();const auto random=*f.w.sessionState().journeyRandom();const auto context=*f.p.encounterContext;
    XeenRestTestAccess::budgetCrossing(*f.flow,to,in.events.load(6));
    const auto pending=XeenRestTestAccess::pulse(*f.flow);
    check(pending.outcome==XeenEncounterOutcome::Pending && xeen_state::sameCamera(f.c,source.camera) &&
        !f.w.sessionState().hasRegionalActors(6) && f.w.sessionState().journeyRandom()==random && *f.p.encounterContext==context &&
        xeen_state::sameMapStates(f.w.sessionState().mapStates(),before->sessionState().mapStates()) && f.w.objectFrame({2,0})==1,
        "Budget yield published destination actors, camera, time, RNG or departure presentation early");
    const auto done=XeenRestTestAccess::pulse(*f.flow);XeenCombatRandom expected(random);
    for(unsigned n=0;n<107;++n)check(bool(expected.draw(1,50)),"Synthetic recovery oracle rejected a draw");
    check(done.outcome==XeenEncounterOutcome::Accepted && xeen_state::sameCamera(f.c,to) &&
        f.w.sessionState().regionalActors(6).size()==107 && f.w.sessionState().journeyRandom()==expected.continuation() &&
        !f.w.objectFrame({2,0}),"Resumed border work did not publish once after exhausting its recovery budget");
}

void atomicRefusals(Inputs &in) {
    // On map 29 Food has no tabled rule; scene actions remain excluded.
    // Temple uses process prices, including on untabled Clouds maps.
    // M79 admits Bank, Ironworks, Guild and Training through their shared owners.
    auto source=in.base();source.camera=quietCell(in,29);table(in,source,29);
    Fixture fixture(in,source);
    for(unsigned action:{3u,6u,8u,9u,10u,11u,13u,255u}) {
        auto world=fixture.w.transitionCandidate();XeenPartyState party;world->copyEventParty(fixture.p,party);
        auto camera=fixture.c;auto flags=fixture.f;
        XeenRestoreGuard before(*world,party,camera,flags);
        XeenEventFile file{29,"maze0029.evt",true,{}};
        const auto add=[&](unsigned op,std::vector<std::uint8_t> parameters) {
            XeenEventRecord r;r.x=camera.x;r.y=camera.y;r.direction=4;r.line=file.records.size();r.opcode=op;
            r.parameters=std::move(parameters);r.lengthField=5+r.parameters.size();file.records.push_back(r);
        };
        add(0x1b,{34,123,0,0,0}); // A gold prelude must never publish.
        add(0x11,{std::uint8_t(action)});
        const auto result=XeenEventInterpreter().begin(camera,party,flags,*world,[&](auto){return XeenEventScript(file);},{});
        const auto *error=std::get_if<XeenEventExecutionError>(&result);
        check(error && error->kind==XeenEventExecutionErrorKind::UnsupportedOperand && before.current(),"Unsupported town service published a prelude, time or RNG");
    }
    auto world=fixture.w.transitionCandidate();XeenPartyState party;world->copyEventParty(fixture.p,party);
    auto camera=fixture.c;camera.mapId=23;camera.x=9;camera.y=11;auto flags=fixture.f;
    XeenRestoreGuard before(*world,party,camera,flags);
    XeenEventRecord r;r.x=9;r.y=11;r.direction=4;r.opcode=35;r.parameters={16,0,1};r.lengthField=8;
    const XeenEventFile file{23,"maze0023.evt",true,{r}};
    const auto result=XeenEventInterpreter().begin(camera,party,flags,*world,[&](auto){return XeenEventScript(file);},{});
    const auto *error=std::get_if<XeenEventExecutionError>(&result);
    check(error && error->kind==XeenEventExecutionErrorKind::UnsupportedOperand && error->message.find("not supported yet")!=std::string::npos && before.current(),"Unverified neighbor AlterCellFlag was not an atomic unsupported refusal");
}

void newMapBarriers(Inputs &in) {
    auto source=in.base();std::optional<XeenCamera> chosen;
    for(unsigned id:{30u,33u,51u}) {
        if(chosen)break;
        auto s=source;table(in,s,id);s.camera=quietCell(in,id);
        for(auto &a:s.map(id).monsters->slots)if(a.lifecycle==XeenActorLifecycle::Present) {
            a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
        }
        for(auto owner:s.activeRosterIds)s.characters[owner].currentHp=1000;
        Fixture seed(in,s);auto world=seed.w.transitionCandidate();
        XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
        for(unsigned n=0;n<6;++n) {const auto owner=s.activeRosterIds[n];characters[n]=seed.p.roster.at(owner);inputs[n]=*seed.p.roster.combatInputs(owner);}
        for(int y=0;y<32 && !chosen;++y)for(int x=0;x<32 && !chosen;++x)for(unsigned d=0;d<4 && !chosen;++d) {
            const XeenCamera c{id,x,y,XeenDirection(d)};if(!world->sampleCell(id,x,y))continue;
            constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
            const auto to=world->sampleCell(id,x+dx[d],y+dy[d]);if(!to)continue;
            const auto terrain=xeenTerrainArrival(world->map(id).geometry.surfaceTypes[to->cell->surfaceIndex],false,false,false,c);
            if(terrain.damage || terrain.falling || terrain.dead || terrain.minutes)continue;
            XeenBarrierCandidate rule(*world,c,characters,inputs,*world->sessionState().journeyRandom(),*seed.p.encounterContext,false);
            if(rule.handled && rule.selection) {chosen=c;source=std::move(s);}
        }
    }
    check(bool(chosen),"No supported original lock in a new map");source.camera=*chosen;
    Fixture f(in,source);const auto before=XeenSaveFormat::encode(f.snapshot());
    f.act(InteractionAction{});
    check(f.flow->canCancelInteraction() && !f.flow->canSave(),"New-map lock did not reach the original character selector");
    f.act(CancelInteractionAction{});quiet(f);
    check(before==XeenSaveFormat::encode(f.snapshot()),"Cancelled new-map lock changed state, time or RNG");
    XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
    for(unsigned n=0;n<6;++n) {const auto owner=source.activeRosterIds[n];characters[n]=f.p.roster.at(owner);inputs[n]=*f.p.roster.combatInputs(owner);}
    auto candidate=f.w.transitionCandidate();const auto originalWall=wallAt(*f.w.sampleCell(f.c.mapId,f.c.x,f.c.y)->cell,f.c.direction);
    const auto minutes=f.p.encounterContext->minutes;XeenCombatRandom random(*f.w.sessionState().journeyRandom());
    XeenBarrierCandidate oracle(*candidate,f.c,characters,inputs,random.continuation(),*f.p.encounterContext,true);
    for(unsigned n=0;;++n){check(n<100,"Bash oracle did not settle");XeenConsequenceDraw draw{random,64,{}};if(oracle.service(draw))break;}
    f.act(BashAction{});quiet(f);
    check(f.p.encounterContext->minutes==minutes+1+unsigned(oracle.moved) &&
        f.w.sessionState().journeyRandom()==random.continuation() && xeen_state::sameCamera(f.c,oracle.camera),
        "New-map Bash outcome, exact cost or RNG differs from the original rule");
    check(wallAt(*f.w.sampleCell(chosen->mapId,chosen->x,chosen->y)->cell,chosen->direction)==(oracle.opened?oracle.targetWall:originalWall),
        "New-map Bash published the wrong wall outcome");
    for(unsigned n=0;n<6;++n)check(f.p.roster.at(source.activeRosterIds[n]).currentHp==oracle.characters[n].currentHp,"New-map Bash paid the wrong HP cost");
    std::cout<<"New-map lock/Bash "<<chosen->mapId<<" at "<<chosen->x<<','<<chosen->y<<'\n';
}
}
int main(int argc,char **argv) {try {
    check(argc==2,"usage: opening-original <installation>");const auto install=xeenTestInstallationDetector().detect(argv[1]);check(bool(install),"Missing installation");
    Inputs in(*install);admission(in);roundtrip(in);hiddenSwitchRoundTrips(in);travel(in);crossing(in);atomicRefusals(in);flowRefusals(in);crossingArrivals(in);crossingBudget(in);newMapBarriers(in);
    std::cout<<"M78 data-derived admission, travel and generic multi-map restore passed\n";return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}}
