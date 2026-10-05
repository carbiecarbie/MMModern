#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenEventTrigger.h"
#include <iostream>
using namespace training_test;
namespace mmodern {
struct XeenCityEventTestAccess {
	static void composer(XeenEventFlow &flow,XeenEventFlow::TransitionCompose compose) {flow._transitionCompose=std::move(compose);}
};
}
namespace {
void settle(Fixture &);
unsigned component(XeenWorld &world) {
	std::bitset<1024> seen;std::vector<std::pair<int,int>> queue{{15,0}};seen.set(15);
	for(unsigned cursor=0;cursor<queue.size();++cursor)for(unsigned d=0;d<4;++d) {
		const auto [x,y]=queue[cursor];XeenCamera camera{28,x,y,XeenDirection(d)};
		if(XeenMovement().apply(world,camera,NavigationAction::MoveForward)!=XeenMovementResult::Moved)continue;
		const auto cell=int(camera.y)*32+camera.x;if(!seen[cell]) {seen.set(cell);queue.emplace_back(camera.x,camera.y);}
	}
	return unsigned(seen.count());
}
void coverage(Inputs &in) {
	XeenWorld base(in.mapLoader());base.markEncounterSession(XeenEncounterEntry::Journey);
	auto world=base.transitionCandidate();check(component(*world)==424,"Original closed-grate geometry component changed");
	auto party=XeenPartyLoader().loadInitialCloudsParty(in.assets);
	XeenConsequenceCharacters chars;XeenConsequenceInputs inputs;
	for(unsigned n=0;n<6;++n) {chars[n]=party.roster.at(kXeenCombatOwners[n]);chars[n].permanentLevel=3;
		inputs[n]=XeenCharacterFormat::parseCombatInputs(in.chr,kXeenCombatOwners[n],true,true,true);}
	unsigned thief=0;for(unsigned n=1;n<6;++n)if(XeenCharacterRules::thievery(chars[n])>XeenCharacterRules::thievery(chars[thief]))thief=n;
	const auto context=*in.base().journey->context;
	for(int y=0;y<32;++y)for(int x=0;x<32;++x)for(unsigned d=0;d<4;++d) {
		const XeenCamera camera{28,x,y,XeenDirection(d)};const auto sample=world->sampleCell(28,x,y);
		if(wallAt(*sample->cell,camera.direction)!=9)continue;
		XeenBarrierCandidate rule(*world,camera,chars,inputs,{1,77,0},context,false);
		if(rule.selection)rule.choose(thief);
		XeenCombatRandom tape(std::vector<XeenCombatRandom::Draw>{{1,4,4},{1,20,20}});
		for(unsigned n=0;n<10;++n){XeenConsequenceDraw draw{tape,1,{}};if(rule.service(draw))break;}
		check(rule.done && rule.opened,"Lawful reference Thievery did not open a grate for geometry coverage");
		world->setBarrier(camera,rule.targetWall,true);
	}
	check(component(*world)==622,"Lawfully opened-grate geometry component changed");
	std::cout<<"Original geometry components: 424 closed, 622 after lawful grate unlocking\n";
}
void automaticBash(Inputs &in,const XeenSaveSnapshot &base) {
	auto start=base;start.camera={28,20,19,XeenDirection::North};
	XeenEventRecord label;label.x=20;label.y=20;label.direction=4;label.opcode=0x27;label.parameters={0};label.lengthField=6;
	XeenEventRecord exit=label;exit.line=1;exit.opcode=0x12;exit.parameters.clear();exit.lengthField=5;exit.fileOffset=7;
	XeenEventFile events{28,"maze0028.evt",true,{label,exit}};
	const auto geometry=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);
		if(!map.geometry.isOutdoors())for(auto &cell:map.geometry.cells){cell.rawWord=0;cell.geometry=XeenIndoorWalls{};cell.surfaceIndex=1;cell.rawAttributes=1;cell.flags=0;}
		if(id==XeenMapIdentity(111))map.geometry.cells[4*16+4].rawAttributes|=kXeenAutomaticEventFlag;
		return map;};
	Fixture f(in,start,false,&events,geometry);unsigned completed=0;
	f.flow->reportManual=[&](const auto &result){if(std::holds_alternative<XeenManualEventCompleted>(result))++completed;};
	f.act(BashAction{});settle(f);
	check(f.c.x==20 && f.c.y==20 && completed==1,"Bash movement must dispatch the destination automatic Event exactly once");
}
void settle(Fixture &f) {
	for(unsigned n=0;n<1000 && !f.flow->canSave();++n) {
		f.now+=125;f.flow->beginCycle(++f.cycle);
		if(const auto frame=f.flow->updatePresentation())f.present(*frame);
		if(f.flow->encounter()->combat())throw std::runtime_error("Barrier test entered unexpected combat");
	}
	if(!f.flow->canSave())throw std::runtime_error("Barrier did not return to quiet input: activity="+
		std::to_string(unsigned(f.w.sessionState().journeyActivity()))+" selector="+std::to_string(f.flow->canCancelInteraction())+" "+f.flow->encounter()->notice());
}
XeenSaveSnapshot source(Inputs &in) {
	auto s=in.service();XeenWorld world(in.mapLoader());
	auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,28),in.statistics);
	for(unsigned n=0;n<actors.size();++n) {
		const auto &a=s.journey->vertigoActors->at(n);actors[n].x=a.x;actors[n].y=a.y;
		actors[n].hp=a.hp;actors[n].lifecycle=a.lifecycle;actors[n].activated=a.activated;
	}
	for(int y=1;y<31;++y)for(int x=1;x<31;++x)for(unsigned d=0;d<4;++d) {
		const XeenCamera camera{28,x,y,XeenDirection(d)};const auto cell=world.sampleCell(28,x,y);
		if(!cell || wallAt(*cell->cell,camera.direction)!=9 || (cell->cell->rawAttributes&0x80))continue;
		if(std::any_of(actors.begin(),actors.end(),[&](const auto &a){return a.lifecycle==XeenActorLifecycle::Present && a.x==x && a.y==y;}))continue;
		const auto view=XeenIndoorScene().classifyActors(world,camera,actors);if(view.engaged())continue;
		// Avoid immediately meeting an actor after this one opening. This is a
		// fixture selection only, never content admission in gameplay.
		const unsigned opposite=d^2;constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
		if(std::any_of(actors.begin(),actors.end(),[&](const auto &a){return a.lifecycle==XeenActorLifecycle::Present &&
			std::abs(int(a.x)-(x+dx[d]))+std::abs(int(a.y)-(y+dy[d]))<5;}))continue;
		(void)opposite;s.camera=camera;
		for(unsigned n=0;n<actors.size();++n)if(view.activation[n])s.journey->vertigoActors->at(n).activated=true;
		return s;
	}
	throw std::runtime_error("No original quiet closed grate fixture");
}
}
int main(int argc,char **argv) {
	try {
		check(argc==2,"usage: barrier-original <installation>");const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"Installation unavailable");
		Inputs in(*installation);const auto initial=source(in);Fixture f(in,initial);
		coverage(in);
		automaticBash(in,initial);
		Fixture outdoors(in,in.base());const auto outdoorContext=*outdoors.p.encounterContext;
		const auto outdoorHp=outdoors.p.roster.at(kXeenCombatOwners[0]).currentHp;
		outdoors.act(BashAction{});
		check(outdoors.p.encounterContext->minutes==outdoorContext.minutes+10 && outdoors.p.encounterContext->ctr24==outdoorContext.ctr24 &&
			outdoors.p.roster.at(kXeenCombatOwners[0]).currentHp==outdoorHp && outdoors.w.sessionState().barriers().empty(),
			"Outdoor Bash must charge ten minutes without HP, ctr24 or wall changes");
		const auto before=XeenSaveFormat::encode(f.snapshot());const auto rng=f.w.sessionState().journeyRandom();
		f.act(InteractionAction{});check(f.flow->canCancelInteraction() && !f.flow->canSave(),"Original grate did not acquire WhoWill/busy save boundary");
		const auto input=f.flow->inputContext(f.flow->frame().presentation());
		check(input.dialog && input.dialog->hits.size()==6 && !input.acceptsQueuedInput,"WhoWill needs strict keyboard and portrait mouse input");
		const auto selectorFrame=f.flow->frame().presentation();const auto selectorInput=f.flow->displayedInput();
		f.act(CancelInteractionAction{});check(f.flow->canSave() && XeenSaveFormat::encode(f.snapshot())==before,"Cancelled real grate changed HP/XP/RNG/walls/time");
		f.flow->handle(SelectMemberAction{0},selectorInput,selectorFrame);
		check(XeenSaveFormat::encode(f.snapshot())==before,"Retired selector input published an unlock");
		auto asleep=initial;asleep.characters[kXeenCombatOwners[0]].conditions[8]=1;Fixture refused(in,asleep);
		const auto asleepBytes=XeenSaveFormat::encode(refused.snapshot());refused.act(InteractionAction{});
		refused.act(DialogKeyAction{InputKey::F1});check(refused.flow->canCancelInteraction() && refused.w.sessionState().journeyRandom()==rng,"Ineligible unlock member must remain at WhoWill without RNG/injury");
		refused.act(DialogKeyAction{InputKey::Escape});check(XeenSaveFormat::encode(refused.snapshot())==asleepBytes,"Dialog mouse/key cancellation changed durable state");
		unsigned chosen=0;
		for(unsigned n=0;n<5 && f.w.sessionState().barriers().empty();++n) {
			f.act(InteractionAction{});
			unsigned best=0;int score=-1;
			for(unsigned member=0;member<6;++member)if(f.p.party.member(f.p.roster,member).canAct()) {
				const auto value=XeenCharacterRules::thievery(f.p.party.member(f.p.roster,member));if(value>score){score=value;best=member;}
			}
			f.act(SelectMemberAction{best});settle(f);
			chosen=best;
		}
		check(f.w.sessionState().barriers().size()==2 && !(f.w.sessionState().journeyRandom()==rng),"Original grate did not unlock with both durable faces and live RNG");
		const auto saved=f.snapshot();Fixture restored(in,saved);
		Fixture retry(in,initial);bool failed=false;
		XeenCityEventTestAccess::composer(*retry.flow,[&](auto &,const auto &,const auto &,auto,auto) {
			if(!failed) {failed=true;throw std::runtime_error("Injected barrier composition failure");}
			return XeenEventFlow::Composition{frame(),false};
		});
		retry.act(InteractionAction{});
		try {retry.act(SelectMemberAction{chosen});}catch(const std::runtime_error &) {}
		for(unsigned n=0;n<100 && !failed;++n) {
			retry.now+=125;retry.flow->beginCycle(++retry.cycle);
			try {if(const auto next=retry.flow->updatePresentation())retry.present(*next);}catch(const std::runtime_error &) {}
		}
		check(failed && retry.w.sessionState().barriers().empty() && retry.w.sessionState().journeyRandom()==rng &&
			retry.p.roster.at(kXeenCombatOwners[chosen]).currentHp==initial.characters[kXeenCombatOwners[chosen]].currentHp,
			"Composition failure published barrier HP/RNG/walls");
		settle(retry);check(XeenSaveFormat::encode(retry.snapshot())==XeenSaveFormat::encode(saved),"Retry replayed trap rolls, injuries or unlock XP");
		check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved),"Injuries/unlock XP/walls did not restore byte-exactly");
		f.act(InteractionAction{});restored.act(InteractionAction{});settle(f);settle(restored);
		check(XeenSaveFormat::encode(f.snapshot())==XeenSaveFormat::encode(restored.snapshot()),"Next toggle after restore changed state or gameplay RNG");
		f.act(InteractionAction{});settle(f);
		const auto movementTime=f.p.encounterContext->minutes;const auto movementHp=f.p.roster.at(kXeenCombatOwners[0]).currentHp;
		f.act(BashAction{});settle(f);
		check(f.p.encounterContext->minutes==movementTime+2 && f.p.roster.at(kXeenCombatOwners[0]).currentHp==movementHp &&
			(f.c.x!=initial.camera.x || f.c.y!=initial.camera.y),"Passable-wall Bash must move, charge twice and avoid HP costs");
		Fixture bash(in,initial);const auto start=*bash.p.encounterContext;
		const auto hp=bash.p.roster.at(kXeenCombatOwners[0]).currentHp;bash.act(BashAction{});settle(bash);
		check(bash.p.encounterContext->minutes==start.minutes+1 && bash.p.roster.at(kXeenCombatOwners[0]).currentHp==hp-2,"Original Bash did not charge one minute/two HP");
		std::cout<<"Original grate cancel/unlock/toggle, v5 round-trip/next action and Bash passed at "<<initial.camera.x<<','<<initial.camera.y<<'\n';return 0;
	} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
