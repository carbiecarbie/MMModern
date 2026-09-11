#include "XeenEncounterTestSupport.h"
#include <iostream>
#include <type_traits>

using namespace encounter_test;
using Action=XeenEncounterAction;
namespace {
struct ReentrantFixture {
	XeenPartyState p = party();
	XeenCamera camera = XeenActorApproach::kEntry;
	XeenEncounterState state;
	const std::vector<XeenMonsterRecord> statistics = stats();
	const XeenGameplayContext context = XeenGameplayContextFormat::parse(pty());
	const XeenEventFile evt = events();
	std::function<void()> onMap;
	XeenWorld world{[&](XeenMapIdentity) {
		if (onMap) {
			auto callback = std::move(onMap);
			onMap = {};
			callback();
		}
		return map();
	}, [](XeenMapIdentity) { return mob(); }};
	XeenEncounterResult start() {
		return XeenActorApproach::initialize(world,p,camera,state,statistics,context,evt);
	}
	XeenEncounterResult action(Action a) {
		return XeenActorApproach::action(world,p,camera,state,a,evt);
	}
	XeenEncounterResult pulse() { return XeenActorApproach::pulse(world,p,camera,state,evt); }
};

void stopDuringPreparation() {
	ReentrantFixture f;
	f.start(); f.action(Action::Right); f.pulse();
	const auto beforeParty = f.p;
	const auto beforeCamera = f.camera;
	const auto beforeActors = f.world.sessionState().actors();
	auto stale = f.state;
	std::uint64_t stoppedRevision = 0;
	f.world.discardMapCache();
	f.onMap = [&] {
		const auto stop = XeenActorApproach::stop(f.world,f.state,XeenEncounterStop::Reporting);
		check(stop.outcome == XeenEncounterOutcome::Stopped,"provider did not stop encounter");
		stoppedRevision = f.state.revision();
	};
	const auto result = f.action(Action::Forward);
	check(stoppedRevision != 0 && result.outcome != XeenEncounterOutcome::Accepted,
		"Forward accepted after provider terminated encounter");
	auto preserved = [&] {
		check(f.world.sessionState().encounterTerminal() && f.state.phase() == XeenEncounterPhase::SupportStopped &&
			f.state.reason() == XeenEncounterStop::Reporting && f.state.revision() == stoppedRevision &&
			f.state.pending() == 0,"provider stop/revision/pending work was overwritten");
		check(save_test::sameCamera(beforeCamera,f.camera) && f.p.encounterContext == beforeParty.encounterContext,
			"stopped Forward published camera/time/ctr24");
		sameActors(beforeActors,f.world.sessionState().actors()); sameParty(beforeParty,f.p);
	};
	preserved();
	check(f.pulse().outcome == XeenEncounterOutcome::Terminal,"pulse resumed stopped encounter");
	check(XeenActorApproach::action(f.world,f.p,f.camera,stale,Action::Forward,f.evt).outcome ==
		XeenEncounterOutcome::Stale,"pre-stop state replayed Forward");
	preserved();
}

void initializationDuringPreparation() {
	ReentrantFixture f;
	auto beforeInitialization = f.state;
	XeenEncounterState beforeWait;
	XeenPartyState innerParty;
	std::vector<XeenActor> innerActors;
	std::uint64_t innerRevision = 0;
	f.onMap = [&] {
		check(f.start().outcome == XeenEncounterOutcome::Started,"inner initialization failed");
		beforeWait = f.state;
		check(f.action(Action::Wait).outcome == XeenEncounterOutcome::Engaged,"inner Wait did not engage");
		innerRevision = f.state.revision(); innerParty = f.p; innerActors = f.world.sessionState().actors();
	};
	// Existing initialization refusal contract is an exception, not a Started result.
	rejects([&] { f.start(); });
	auto preserved = [&] {
		check(innerRevision > 1 && f.state.revision() == innerRevision && f.state.pending() == 0 &&
			f.state.phase() == XeenEncounterPhase::Engaged && f.world.sessionState().encounterTerminal() &&
			f.world.sessionState().encounterInitialized(),"outer initialization replaced inner authority/revision");
		check(f.p.encounterContext == innerParty.encounterContext && f.p.encounterContext->minutes == 490 &&
			f.world.sessionState().actors().at(5).y == 1 && save_test::sameCamera(f.camera,XeenActorApproach::kEntry),
			"outer initialization reset inner gameplay");
		sameActors(innerActors,f.world.sessionState().actors()); sameParty(innerParty,f.p);
	};
	preserved();
	check(f.pulse().outcome == XeenEncounterOutcome::Terminal,"pulse resumed inner engagement");
	for (auto *stale : {&beforeInitialization,&beforeWait})
		check(XeenActorApproach::action(f.world,f.p,f.camera,*stale,Action::Wait,f.evt).outcome ==
			XeenEncounterOutcome::Stale,"pre-initialization/Wait state replayed");
	rejects([&] { f.start(); });
	preserved();
}

void staleFailureDuringPreparation(bool throwAfterInner) {
	ReentrantFixture f;
	f.start(); f.action(Action::Right); f.pulse();
	auto stale = f.state;
	XeenPartyState innerParty;
	XeenCamera innerCamera;
	XeenEncounterState innerState;
	std::vector<XeenActor> innerActors;
	f.world.discardMapCache();
	f.onMap = [&] {
		check(f.action(Action::Forward).outcome == XeenEncounterOutcome::Accepted,"inner Forward failed");
		innerParty = f.p; innerCamera = f.camera; innerState = f.state;
		innerActors = f.world.sessionState().actors();
		if (throwAfterInner) throw std::runtime_error("provider failure after inner publication");
	};
	const auto result = f.action(Action::Forward);
	auto preserved = [&] {
		check(f.camera.x == 14 && f.camera.y == 1 && save_test::sameCamera(f.camera,innerCamera) &&
			f.p.encounterContext == innerParty.encounterContext && f.p.encounterContext->minutes == 490,
			"stale failure changed inner camera/context");
		check(f.state.revision() == 4 && f.state.revision() == innerState.revision() &&
			f.state.pending() == 3 && f.state.pending() == innerState.pending() &&
			f.state.phase() == XeenEncounterPhase::Exploring && f.state.phase() == innerState.phase() &&
			f.state.reason() == XeenEncounterStop::None && f.state.reason() == innerState.reason() &&
			!f.world.sessionState().encounterTerminal() && f.world.sessionState().encounterInitialized() &&
			f.world.sessionState().encounterMarked(),"stale failure stopped newer authoritative work");
		sameActors(innerActors,f.world.sessionState().actors()); sameParty(innerParty,f.p);
	};
	preserved();
	check(result.outcome == XeenEncounterOutcome::Stale && result.revision == 4 &&
		result.movementOpportunities == 0,"outer failure was not observationally refused");
	check(XeenActorApproach::action(f.world,f.p,f.camera,stale,Action::Forward,f.evt).outcome ==
		XeenEncounterOutcome::Stale,"stale outer Forward replayed");
	check(XeenActorApproach::pulse(f.world,f.p,f.camera,stale,f.evt).outcome ==
		XeenEncounterOutcome::Stale,"stale outer pulse replayed");
	preserved();
	check(f.pulse().outcome == XeenEncounterOutcome::Pulsed && f.state.pending() == 2 &&
		f.state.revision() == 5,"inner authorized pending work was lost");
}

void authoritativeFailureStops() {
	for (int failure = 0; failure < 3; ++failure) {
		ReentrantFixture f;
		f.start(); f.action(Action::Right); f.pulse(); f.action(Action::Forward);
		if (failure == 2) f.p.encounterContext->minutes = 950;
		const auto beforeParty = f.p;
		const auto beforeCamera = f.camera;
		const auto beforeActors = f.world.sessionState().actors();
		if (failure == 1) {
			f.world.discardMapCache();
			f.onMap = [] { throw std::runtime_error("current preparation failure"); };
		}
		const auto result = f.action(failure == 2 ? Action::Wait : Action::Forward);
		const auto reason = failure == 0 ? XeenEncounterStop::Envelope :
			failure == 1 ? XeenEncounterStop::Preparation : XeenEncounterStop::Time;
		check(result.outcome == XeenEncounterOutcome::Stopped && result.reason == reason &&
			f.state.reason() == reason && f.state.phase() == XeenEncounterPhase::SupportStopped &&
			f.world.sessionState().encounterTerminal() && f.state.pending() == 0 && f.state.revision() == 5,
			"current authorization no longer stops/retire pending work");
		check(save_test::sameCamera(f.camera,beforeCamera) && f.p.encounterContext == beforeParty.encounterContext,
			"authoritative refusal published gameplay");
		sameActors(beforeActors,f.world.sessionState().actors()); sameParty(beforeParty,f.p);
	}
}

void viewAndMovement() {
	static_assert(!std::is_convertible<XeenObjectIdentity,XeenMonsterIdentity>::value);
	XeenObjectFile m{20,"synthetic",true,{}};
	// Literal North coordinates and first selected slots for ALL twelve source queries.
	const int x[]{10,10,9,11,10,9,11,10,9,8,11,12};
	const int y[]{10,11,11,11,12,12,12,13,13,13,13,13};
	const int slot[]{0,3,12,13,6,14,15,9,16,18,17,19};
	for(int i=0;i<12;++i)m.entities.monsters.push_back({x[i],y[i],0,0,8});
	const auto original=XeenActorApproach::actorsFromResources(m,stats());
	for(unsigned d=0;d<4;++d) {
		auto a=original;
		for(auto &v:a){int dx=v.x-10,dy=v.y-10;
			if(d==1){v.x=10+dy;v.y=10-dx;} if(d==2){v.x=10-dx;v.y=10-dy;} if(d==3){v.x=10-dy;v.y=10+dx;}}
		const auto view=XeenActorApproach::classify(a,{20,10,10,static_cast<XeenDirection>(d)});
		for(int i=0;i<12;++i)check(view.activation[i] && view.slots[slot[i]]->recordIndex==static_cast<std::size_t>(i),"query/facing/selection mismatch");
		check(view.placements[0]==XeenActorPlacement::SameCell && view.placements[1]==XeenActorPlacement::Forward &&
			view.placements[2]==XeenActorPlacement::ForwardLeft && view.placements[3]==XeenActorPlacement::ForwardRight,"four placements");
		for(const auto &v:a)check(!v.activated,"pure classifier mutated activation");
	}
	m.entities.monsters.assign(4,{10,11,0,0,8});
	auto a=XeenActorApproach::actorsFromResources(m,stats());
	auto v=XeenActorApproach::classify(a,{20,10,10,XeenDirection::North});
	check(!v.engaged() && v.slots[3]->recordIndex==0 && v.slots[4]->recordIndex==1 && v.slots[5]->recordIndex==2 && v.activation[3],"three slots erased fourth activation");
	a[0].x=10;a[0].y=10;check(XeenActorApproach::classify(a,{20,10,10,XeenDirection::North}).engaged(),"same cell engagement");
	a[0].x=0;a[0].y=31; a[1].x=31;a[1].y=0;a[2].x=-1;a[2].y=5;a[3].x=32;a[3].y=5;
	auto occupancy=XeenActorApproach::occupancy(a);check(occupancy[31*32]==1 && occupancy[31]==1 && occupancy[5*32+31]==0,"offscreen/signed occupancy");
	m.entities.monsters={{9,11,0,0,8}};a=XeenActorApproach::actorsFromResources(m,stats());a[0].activated=true;
	auto allowed=[](const XeenActor &,int,int){return XeenMonsterTerrain::Allowed;};
	auto moved=XeenActorApproach::move(a,{20,10,10,XeenDirection::North},allowed);
	check(moved[0].x==10 && moved[0].y==11,"North primary or one-move/two-pass changed");
	moved=XeenActorApproach::move(a,{20,10,10,XeenDirection::East},allowed);
	check(moved[0].x==9 && moved[0].y==10,"East primary changed");
	moved=XeenActorApproach::move(a,{20,10,10,XeenDirection::South},[](const XeenActor &,int x,int y){
		return x==10 && y==11?XeenMonsterTerrain::Blocked:XeenMonsterTerrain::Allowed;});
	check(moved[0].x==9 && moved[0].y==10,"terrain fallback changed");
	for(int i=0;i<3;++i)m.entities.monsters.push_back({10,11,0,0,8});
	a=XeenActorApproach::actorsFromResources(m,stats());a[0].activated=true;
	moved=XeenActorApproach::move(a,{20,10,10,XeenDirection::North},allowed);
	check(moved[0].x==9 && moved[0].y==11,"occupancy failure triggered fallback");
	// Earlier record initially blocked; later record vacates destination. Pass two retries once.
	for(auto &r:a)r.activated=true;
	moved=XeenActorApproach::move(a,{20,10,10,XeenDirection::North},allowed);
	check(moved[0].x==10 && moved[0].y==11 && moved[1].y==10 && moved[2].y==10 && moved[3].y==10,"two-pass ordering");
	a.resize(1);a[0].x=10;a[0].y=10;
	moved=XeenActorApproach::move(a,{20,10,10,XeenDirection::North},allowed);sameActors(a,moved);
	a[0].status=XeenActorStatus::Unsupported;rejects([&]{XeenActorApproach::move(a,{20,10,10,XeenDirection::North},allowed);});
	a[0].status=XeenActorStatus::Physical;a[0].statistics->raw[32]=1;
	rejects([&]{XeenActorApproach::move(a,{20,10,10,XeenDirection::North},allowed);});
	sameActors(a,XeenActorApproach::move(a,{20,10,10,XeenDirection::North},allowed,false));
}
void traces() {
	Fixture f;auto before=f.p;auto r=f.start();
	check(r.view.slots[3]->recordIndex==5 && !r.view.engaged() && f.anchor().activated && f.state.pending()==0,"startup literal trace");
	for(int i=0;i<5;++i)f.pulse();check(f.anchor().y==2 && f.p.encounterContext->minutes==480,"idle created movement");
	r=f.action(Action::Wait);check(r.outcome==XeenEncounterOutcome::Engaged && f.anchor().y==1 && f.camera.y==1 && f.p.encounterContext->minutes==490 &&
		f.state.phase()==XeenEncounterPhase::Engaged && f.state.pending()==0 && f.anchor().hp==20,"fresh Wait approach trace");
	sameParty(before,f.p);auto actorBefore=f.world.sessionState().actors();auto context=f.p.encounterContext;
	f.input(Action::Forward);f.action(Action::Wait);f.pulse();sameActors(actorBefore,f.world.sessionState().actors());
	check(context==f.p.encounterContext,"terminal resumed");
	Fixture forward;forward.start();forward.action(Action::Forward);
	check(forward.camera.y==2 && forward.anchor().y==2 && forward.state.pending()==3 && forward.p.encounterContext->minutes==490,"direct forward intermediate");
	forward.pulse();check(forward.state.phase()==XeenEncounterPhase::Engaged && forward.state.pending()==0,"direct forward engagement");
	Fixture backward;backward.start();backward.input(Action::Right);backward.input(Action::Right);
	backward.action(Action::Backward);check(backward.camera.y==2 && backward.state.pending()==3 &&
		backward.p.encounterContext->minutes==490,"accepted backward step");backward.pulse();
	Fixture east;east.start();east.input(Action::Right);east.action(Action::Forward);
	check(east.camera.x==14 && east.state.pending()==3 && east.anchor().y==2,"delayed intermediate count3");
	east.pulse();check(east.state.pending()==2 && east.anchor().y==2,"post-action pulse duplicated");
	east.pulse();check(east.state.pending()==1 && east.anchor().y==2,"early delayed movement");
	east.pulse();check(east.state.pending()==0 && east.anchor().x==13 && east.anchor().y==1 && east.camera.x==14 &&
		east.p.encounterContext->minutes==490 && east.anchor().activated,"East delayed approach/latch");
	east.input(Action::Left);r=east.input(Action::Left);check(r.view.slots[3]->recordIndex==5,"two turns reveal actor");
	east.action(Action::Wait);check(east.anchor().x==14 && east.anchor().y==1 && east.p.encounterContext->minutes==500 && east.state.phase()==XeenEncounterPhase::Engaged,"West wait trace");
	Fixture rapid;rapid.start();rapid.input(Action::Right);rapid.input(Action::Forward);r=rapid.action(Action::Wait);
	check(r.movementOpportunities==2 && rapid.anchor().x==14 && rapid.anchor().y==1 && rapid.p.encounterContext->minutes==500,"old/new rapid opportunities");
	Fixture destination;destination.start();destination.input(Action::Right);destination.input(Action::Forward);
	destination.input(Action::Left);r=destination.action(Action::Forward);
	check(r.movementOpportunities==1 && destination.camera.x==14 && destination.camera.y==2 &&
		destination.anchor().x==14 && destination.anchor().y==2 && destination.state.pending()==3,
		"old opportunity did not use candidate destination camera");destination.pulse();
	Fixture turn;turn.start();turn.input(Action::Right);turn.input(Action::Forward);
	turn.input(Action::Left);check(turn.state.pending()==1 && turn.p.encounterContext->minutes==490,"turn minute charge");
	turn.input(Action::Right);check(turn.state.pending()==0 && turn.anchor().y==1,"turn did not finish old work");
	Fixture blocked;blocked.start();blocked.input(Action::Backward);
	check(blocked.camera.y==1 && blocked.p.encounterContext->minutes==480 && blocked.p.encounterContext->ctr24==0,"real blocked step charged");
	blocked.input(Action::Right);blocked.input(Action::Forward);blocked.input(Action::Right); // South, pending 1
	// Make the south destination a real space collision outside the four admitted cells.
	blocked.world.discardMapCache();auto &cell=blocked.terrain.geometry.cells[14];cell.rawWord=15;cell.geometry=XeenOutdoorLayers{15,0,0,0};
	const auto ctr=blocked.p.encounterContext->ctr24;r=blocked.action(Action::Forward);
	check(r.outcome==XeenEncounterOutcome::Blocked && blocked.p.encounterContext->ctr24==ctr,"blocked step counters");
	blocked.pulse();check(blocked.state.pending()==0 && blocked.anchor().x==14 && blocked.anchor().y==2,"blocked pulse old move ordering");
	Fixture time;time.start();time.input(Action::Right);time.input(Action::Forward);
	time.p.encounterContext->minutes=950;time.p.encounterContext->ctr24=23;
	auto old=time.world.sessionState().actors();auto cam=time.camera;auto ctx=time.p.encounterContext;
	r=time.action(Action::Wait);check(r.reason==XeenEncounterStop::Time && time.state.pending()==0 && time.p.encounterContext==ctx && save_test::sameCamera(cam,time.camera),"950 refusal published facts");sameActors(old,time.world.sessionState().actors());
	Fixture wrap;wrap.start();wrap.p.encounterContext->ctr24=23;wrap.action(Action::Right);check(wrap.p.encounterContext->ctr24==0,"ctr24 wrap");
	Fixture boundary;boundary.start();boundary.input(Action::Right);boundary.input(Action::Forward);old=boundary.world.sessionState().actors();ctx=boundary.p.encounterContext;
	r=boundary.action(Action::Forward);check(r.reason==XeenEncounterStop::Envelope && boundary.camera.x==14 && boundary.p.encounterContext==ctx,"passable boundary became geography");sameActors(old,boundary.world.sessionState().actors());
}
void lifetimeAndFailures() {
	Fixture f;check(!f.world.hasEncounterState(),"ordinary cache initialized encounter");f.world.map(20);f.world.objectFile(20);
	check(!f.world.hasEncounterState(),"ordinary map20 cache activated actors");
	f.world.disableObject({20,0});f.start();auto p=f.p;auto actors=f.world.sessionState().actors();auto rev=f.state.revision();
	rejects([&]{f.start();});sameParty(p,f.p);sameActors(actors,f.world.sessionState().actors());check(f.p.encounterContext==p.encounterContext && f.state.revision()==rev,"reinitialization changed context");
	// Test-only injected live HP: no production damage/reset command is introduced.
	const_cast<XeenActor &>(f.anchor()).hp=7;
	f.input(Action::Right);f.input(Action::Forward);f.pulse();f.pulse();actors=f.world.sessionState().actors();p=f.p;
	f.statistics.clear();f.world.discardMapCache();f.world.map(20);f.world.objectFile(20);
	sameActors(actors,f.world.sessionState().actors());sameParty(p,f.p);check(f.anchor().hp==7 && f.world.isObjectDisabled({20,0}) && f.mapLoads==2 && f.mobLoads==2,"cache reset authority");
	auto stale=f.state;f.action(Action::Left);actors=f.world.sessionState().actors();p=f.p;
	auto r=XeenActorApproach::action(f.world,f.p,f.camera,stale,Action::Wait,f.evt);
	check(r.outcome==XeenEncounterOutcome::Stale,"stale value replayed");sameActors(actors,f.world.sessionState().actors());check(f.p.encounterContext==p.encounterContext,"stale time replay");
	Fixture other;other.start();r=XeenActorApproach::action(other.world,f.p,f.camera,f.state,Action::Wait,f.evt);check(r.outcome==XeenEncounterOutcome::Stale,"foreign owners accepted");
	const auto revision=f.state.revision();f.action(Action::Unsupported);check(f.state.revision()==revision,"unsupported command was pulse");
	Fixture failed;failed.statistics[8].raw[46]=1;auto ordinary=failed.p;rejects([&]{failed.start();});
	check(failed.world.sessionState().encounterMarked() && !failed.world.sessionState().encounterInitialized() && failed.world.sessionState().actors().empty() && !failed.p.encounterContext,"partial initialization");sameParty(ordinary,failed.p);
	Fixture failure;failure.start();failure.input(Action::Right);failure.input(Action::Forward);actors=failure.world.sessionState().actors();p=failure.p;auto camera=failure.camera;
	failure.world.discardMapCache();failure.failMap=true;r=failure.action(Action::Wait);
	check(r.reason==XeenEncounterStop::Preparation && failure.state.phase()==XeenEncounterPhase::SupportStopped && failure.state.pending()==0 && save_test::sameCamera(camera,failure.camera) && failure.p.encounterContext==p.encounterContext,"preparation failure published gameplay");sameActors(actors,failure.world.sessionState().actors());
	Fixture reporting;reporting.start();reporting.action(Action::Wait);actors=reporting.world.sessionState().actors();
	XeenActorApproach::stop(reporting.world,reporting.state,XeenEncounterStop::Reporting);reporting.action(Action::Wait);
	sameActors(actors,reporting.world.sessionState().actors());check(reporting.p.encounterContext->minutes==490,"report failure replayed action");
	Fixture fresh;fresh.start();check(fresh.anchor().hp==20 && fresh.anchor().y==2,"fresh owner did not restore originals");
	for(int which=0;which<6;++which) {
		Fixture bad;
		if(which==0)bad.objects.entities.monsters[0]={14,4,0,0,8}; // offscreen but inside scan
		if(which==1)bad.objects.entities.monsters[0]={14,4,7,0,-1};
		if(which==2)bad.statistics[8].raw[32]=1;
		if(which==3)bad.terrain.geometry.cells[29].rawAttributes=1;
		if(which==4){XeenEventRecord e;e.x=13;e.y=1;bad.evt.records.push_back(e);}
		if(which==5)bad.p.roster.at(0).conditions[0]=1;
		rejects([&]{bad.start();});check(!bad.p.encounterContext && bad.world.sessionState().actors().empty(),"bad admission leaked state");
	}
}
}
int main(int argc, char **argv) {
	try {
		if (argc == 2 && std::string(argv[1]) == "reentrant-stop") stopDuringPreparation();
		else if (argc == 2 && std::string(argv[1]) == "reentrant-initialize") initializationDuringPreparation();
		else if (argc == 2 && std::string(argv[1]) == "stale-support") staleFailureDuringPreparation(false);
		else if (argc == 2 && std::string(argv[1]) == "stale-exception") staleFailureDuringPreparation(true);
		else if (argc == 2 && std::string(argv[1]) == "current-failure") authoritativeFailureStops();
		else {
			check(argc == 1,"unknown test selection");
			viewAndMovement(); traces(); lifetimeAndFailures();
			stopDuringPreparation(); initializationDuringPreparation();
			staleFailureDuringPreparation(false); staleFailureDuringPreparation(true); authoritativeFailureStops();
		}
		std::cout << "Actor classification, transitions and lifetime tests passed\n";
		return 0;
	} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
