#include "XeenRegionalJourneyTestSupport.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
#include <type_traits>
using namespace encounter_test;
using Action=XeenEncounterAction;
namespace {
using regional_journey_test::Fixture;
struct Observation {
    std::array<XeenCharacter,30> characters;
    std::optional<XeenGameplayContext> context;
    XeenCamera camera;
    std::vector<XeenActor> actors;
    explicit Observation(Fixture &f):characters(f.p.roster.characters()),context(f.p.encounterContext),camera(f.camera),actors(f.w.sessionState().actors()){}
    void unchanged(Fixture &f)const{
        for(unsigned i=0;i<30;++i)check(xeen_state::sameCharacter(characters[i],f.p.roster.at(i)),"actor transition changed character facts");
        check(context==f.p.encounterContext&&save_test::sameCamera(camera,f.camera),"refused actor transition published camera/time");
        sameActors(actors,f.w.sessionState().actors());
    }
};
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
	m.entities.monsters=std::vector<XeenMapEntity>(4,{10,11,0,0,8});
	auto a=XeenActorApproach::actorsFromResources(m,stats());
	auto v=XeenActorApproach::classify(a,{20,10,10,XeenDirection::North});
	check(!v.engaged() && v.slots[3]->recordIndex==0 && v.slots[4]->recordIndex==1 && v.slots[5]->recordIndex==2 && v.activation[3],"three slots erased fourth activation");
	a[0].x=10;a[0].y=10;check(XeenActorApproach::classify(a,{20,10,10,XeenDirection::North}).engaged(),"same cell engagement");
	a[0].x=0;a[0].y=31; a[1].x=31;a[1].y=0;a[2].x=-1;a[2].y=5;a[3].x=32;a[3].y=5;
	auto occupancy=XeenActorApproach::occupancy(a);check(occupancy[31*32]==1 && occupancy[31]==1 && occupancy[5*32+31]==0,"offscreen/signed occupancy");
	m.entities.monsters=std::vector<XeenMapEntity>{{9,11,0,0,8}};a=XeenActorApproach::actorsFromResources(m,stats());a[0].activated=true;
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

void traces(){
    Fixture f;
    check(f.flow->state().pending()==0&&f.anchor().activated&&f.anchor().x==13&&f.anchor().y==2,"regional startup actor observation");
    for(unsigned i=0;i<5;++i)f.pulse();check(f.anchor().y==2&&f.p.encounterContext->minutes==480,"idle created movement");
    auto r=f.action(Action::Wait);
    check(r.outcome==XeenEncounterOutcome::Engaged&&f.anchor().y==1&&f.camera.y==1&&f.p.encounterContext->minutes==490&&f.flow->state().pending()==0&&f.anchor().hp==20,"Wait approach trace");
    Observation engaged(f);f.action(Action::Forward);f.action(Action::Wait);f.pulse();engaged.unchanged(f);
    Fixture forward;forward.action(Action::Forward);
    check(forward.camera.y==2&&forward.anchor().y==2&&forward.flow->state().pending()==3&&forward.p.encounterContext->minutes==490,"direct forward intermediate");
    forward.pulse();check(forward.flow->state().phase()==XeenEncounterPhase::Engaged&&forward.flow->state().pending()==2,"direct forward engagement retains pending count");
    Fixture backward;backward.input(Action::Right);backward.input(Action::Right);backward.action(Action::Backward);
    check(backward.camera.y==2&&backward.flow->state().pending()==3&&backward.p.encounterContext->minutes==490,"accepted backward step");backward.pulse();
    Fixture east;east.input(Action::Right);east.action(Action::Forward);
    check(east.camera.x==14&&east.flow->state().pending()==3&&east.anchor().y==2,"delayed intermediate count3");
    east.pulse();check(east.flow->state().pending()==2&&east.anchor().y==2,"post-action pulse duplicated");
    east.pulse();check(east.flow->state().pending()==1&&east.anchor().y==2,"early delayed movement");
    east.pulse();check(east.flow->state().pending()==0&&east.anchor().x==13&&east.anchor().y==1&&east.camera.x==14&&east.p.encounterContext->minutes==490&&east.anchor().activated,"East delayed approach/latch");
    east.input(Action::Left);r=east.input(Action::Left);check(r.view.slots[3]->recordIndex==5,"two turns reveal actor");
    east.action(Action::Wait);check(east.anchor().x==14&&east.anchor().y==1&&east.p.encounterContext->minutes==500&&east.flow->state().phase()==XeenEncounterPhase::Engaged,"West wait trace");
    Fixture rapid;rapid.input(Action::Right);rapid.input(Action::Forward);r=rapid.action(Action::Wait);
    check(r.movementOpportunities==2&&rapid.anchor().x==14&&rapid.anchor().y==1&&rapid.p.encounterContext->minutes==500,"old/new rapid opportunities");
    Fixture destination;destination.input(Action::Right);destination.input(Action::Forward);destination.input(Action::Left);r=destination.action(Action::Forward);
    check(r.movementOpportunities==1&&destination.camera.x==14&&destination.camera.y==2&&destination.anchor().x==14&&destination.anchor().y==2&&destination.flow->state().pending()==3,"old opportunity uses candidate destination");destination.pulse();
    Fixture turn;turn.input(Action::Right);turn.input(Action::Forward);turn.input(Action::Left);
    check(turn.flow->state().pending()==1&&turn.p.encounterContext->minutes==490,"turn minute charge");
    turn.input(Action::Right);check(turn.flow->state().pending()==0&&turn.anchor().y==1,"turn finishes old work");
    Fixture blocked;Observation before(blocked);r=blocked.action(Action::Backward);blocked.pulse();
    check(r.outcome==XeenEncounterOutcome::Blocked&&blocked.p.encounterContext->ctr24==0,"blocked step charged");before.unchanged(blocked);
    Fixture wrap;
    for(unsigned i=0;i<24;++i)wrap.input(Action::Right);
    check(wrap.p.encounterContext->ctr24==0&&wrap.p.encounterContext->minutes==480,"ctr24 wraps without turn time");
}
void providerFailure(){
    Fixture f;f.input(Action::Right);f.input(Action::Forward);Observation before(f);
    f.w.discardMapCache();f.onMap=[]{throw std::runtime_error("current preparation failure");};
    try{f.action(Action::Wait);}catch(const std::exception &){}
    check(!f.flow->journeyQuiet(),"current provider failure leaves quiet boundary closed");before.unchanged(f);
}
void staleProvider(bool throws){
    for(bool objects:{false,true})for(bool pending:{false,true})for(bool directStop:{false,true}){
        Fixture f;if(pending){f.input(Action::Right);f.action(Action::Forward);}
        const auto old=f.flow->ticket();const auto pendingBefore=f.flow->state().pending();Observation before(f);f.w.discardMapCache();unsigned calls=0;
        auto callback=[&]{++calls;const auto lease=f.flow->boundary().hold(XeenCombatBoundary::Work::Inventory);f.flow->boundary().release(XeenCombatBoundary::Work::Inventory,lease);
            if(directStop){auto state=f.flow->state();check(XeenActorApproach::stop(f.w,state,XeenEncounterStop::Reporting).outcome==XeenEncounterOutcome::Stale,"direct stop refuses expired delegated authority");}
            if(throws)throw std::runtime_error("obsolete provider failure");};
        if(objects)f.onObjects=callback;else f.onMap=callback;
        const auto result=pending?f.flow->journeyPulse(old):f.flow->journeyAction(old,Action::Forward);
        check(result.outcome!=XeenEncounterOutcome::Accepted&&result.outcome!=XeenEncounterOutcome::Pulsed,"obsolete action/pulse refused");
        check(calls==1&&!f.flow->current(old)&&f.flow->state().pending()==pendingBefore,"provider fired and preserved pending work");before.unchanged(f);
        f.onMap={};f.onObjects={};
        if(pending){check(f.pulse().outcome==XeenEncounterOutcome::Pulsed&&f.flow->state().pending()==2,"fresh pulse services preserved work");}
        else check(f.action(Action::Right).outcome==XeenEncounterOutcome::Accepted,"fresh authority survives obsolete callback");
        check(!f.flow->fail(old,XeenEncounterStop::Reporting),"stale stop cannot replace new authority");
    }
}
void nestedInitialization(){for(bool mutation:{false,true}){
    auto bytes=regional_test::characterBytes();auto p=XeenPartyLoader().loadFromResources(bytes,regional_test::partyBytes());
    auto camera=xeenJourneyContent().entry;XeenGameFlags flags;
    const auto resources=regional_test::resources();auto monsters=regional_test::statistics();auto event=regional_test::events(23);
    XeenJourneySetup setup{bytes,XeenGameplayContextFormat::parse(regional_test::partyBytes()),monsters,event,1,resources.regionalManifest};
    setup.purse=XeenMonsterTreasure{};setup.regionalRecovery=XeenRegionalRecoveryState{};setup.regionalText=regional_test::texts(23);
    setup.learnedNames=XeenLearnedSpellNames{};setup.learnedNamesProvider=resources.loadLearnedSpellNames;setup.vertigoManifest=resources.vertigoManifest;
    setup.bank=XeenBankBalances{};setup.cityEventsProvider=[]{return regional_test::events(28);};
    XeenEventPresenter::Clock clock=[]{return 0;};bool fired=false;XeenWorld *world=nullptr;
    XeenWorld w([&](auto id){if(!fired){fired=true;if(mutation)++p.roster.at(0).currentHp;else rejects([&]{XeenEncounterFlow nested(*world,p,camera,flags,clock,setup);},"fresh");}return regional_test::map(id);},regional_test::objects);world=&w;
    if(mutation){
        rejects([&]{XeenEncounterFlow flow(w,p,camera,flags,clock,setup);});
        check(fired&&w.hasEncounterState()&&!p.encounterContext&&!p.roster.combatInputs(0)&&p.roster.at(0).currentHp==11,"failed admission keeps external mutation without partial owner attachment");
        w.discardMapCache();(void)w.map(23);continue;
    }
    XeenEncounterFlow flow(w,p,camera,flags,clock,setup);
    check(fired&&w.sessionState().actors().size()==19&&p.encounterContext->minutes==480,"nested initialization cannot overwrite fresh publication");
    check(flow.prepareJourneyFrame(flow.ticket(),[]{})&&flow.presentJourney(flow.ticket())&&flow.journeyQuiet(),"outer fresh initialization remains usable");
}}
void lifetimeAndFailures(){
    Fixture f;const auto actors=f.w.sessionState().actors();const auto characters=f.p.roster.characters();
    f.w.discardMapCache();f.w.map(23);f.w.objectFile(23);sameActors(actors,f.w.sessionState().actors());
    for(unsigned i=0;i<30;++i)check(xeen_state::sameCharacter(characters[i],f.p.roster.at(i)),"cache reconstruction retains party");
    auto stale=f.flow->state();f.action(Action::Right);Observation current(f);
    check(XeenActorApproach::action(f.w,f.p,f.camera,stale,Action::Wait,f.event).outcome!=XeenEncounterOutcome::Accepted,"direct stale state cannot publish");current.unchanged(f);
    Fixture other;auto foreign=f.flow->state();
    check(XeenActorApproach::action(other.w,f.p,f.camera,foreign,Action::Wait,f.event).outcome!=XeenEncounterOutcome::Accepted,"foreign owners cannot publish");current.unchanged(f);
    const auto revision=f.flow->state().revision();f.action(Action::Unsupported);check(f.flow->state().revision()==revision,"unsupported command is not a pulse");
    Fixture fresh;check(fresh.anchor().hp==20&&fresh.anchor().y==2,"fresh owners restore source actor facts");
}
}
int main(int argc,char **argv){try{
    const std::string selection=argc==2?argv[1]:"";
    if(selection=="reentrant-stop")staleProvider(false);
    else if(selection=="reentrant-initialize"){nestedInitialization();}
    else if(selection=="stale-support")staleProvider(false);
    else if(selection=="stale-exception")staleProvider(true);
    else if(selection=="current-failure"){providerFailure();}
    else{check(argc==1,"unknown test selection");viewAndMovement();traces();lifetimeAndFailures();staleProvider(false);staleProvider(true);providerFailure();nestedInitialization();}
    std::cout<<"Regional actor rules, transitions and lifetime tests passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
