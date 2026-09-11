#include "XeenEncounterTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "games/xeen/XeenOutdoorScene.h"
#include <iostream>
#include <limits>

using namespace encounter_test;
using Action = XeenEncounterAction;
namespace {
struct Production : Fixture {
	std::uint64_t now=0;
	XeenFontFormat font{gameplay_test::fontBytes()};
	XeenGameFlags flags;
	XeenEventSystem system{[](XeenMapIdentity){return XeenEventScript(events());},
		[](XeenMapIdentity){return XeenEventTextFile{};}};
	std::unique_ptr<XeenEventFlow> flow;
	unsigned initializations=0, spriteChecks=0, compositions=0, reports=0, rebuilds=0;
	std::vector<std::uint64_t> ordinaryPhases;
	std::string lastReport;
	bool ordinaryAnimated=false;
	std::function<void()> onClock, onCompose, onReport, onSprite, onRebuild;
	Production() { const char name[]="Skeleton"; std::copy(name,name+8,statistics[8].raw.begin()); }
	void startFlow() {
		XeenEncounterSetup setup{evt,[&](XeenWorld &w,XeenPartyState &p,XeenCamera &c,XeenEncounterState &s){
			++initializations;return XeenActorApproach::initialize(w,p,c,s,statistics,context,evt);
		},[&](std::uint8_t image){++spriteChecks;check(image==42,"image/type confusion");if(onSprite)onSprite();}};
		flow=std::make_unique<XeenEventFlow>(world,system,p,camera,flags,font,
			[](std::uint64_t)->XeenEventFlow::Composition{throw std::runtime_error("ordinary composer reached");},
			XeenEventPresenter::NpcDraw{},[&]{if(onClock){auto callback=onClock;callback();}return now;},
			XeenEventPresenter::RandomFrame{},nullptr,&setup,[&](std::uint64_t ordinary,std::uint8_t frame){
				++compositions;
				ordinaryPhases.push_back(ordinary);
				if(onCompose){auto callback=onCompose;callback();}
				world.map(20);world.objectFile(20);
				XeenEventFlow::Composition c;c.frame.width=320;c.frame.height=200;c.frame.pixels.resize(64000);
				c.frame.pixels[0]=frame;c.frame.pixels[1]=camera.x;c.frame.pixels[2]=camera.y;
				c.frame.pixels[3]=ordinary%256;c.containsOrdinaryAnimation=ordinaryAnimated;
				return c;
			});
		flow->reportText=[&](const std::string &text){++reports;lastReport=text;if(onReport)onReport();};
		flow->rebuildEncounterPresentation=[&]{++rebuilds;if(onRebuild)onRebuild();};
	}
	const XeenEncounterFlow &coordinator() const {return *flow->encounter();}
	void east() {flow->handle(NavigationAction::TurnRight);flow->handle(NavigationAction::MoveForward);}
};

void ordinaryNavigation() {
	Production f;f.ordinaryAnimated=true;f.startFlow();
	const auto phase=[&](std::uint64_t expected){
		check(f.ordinaryPhases.back()==expected&&f.flow->frame().pixels[3]==expected,"production ordinary phase");
	};
	phase(0);f.now=25;f.flow->handle(NavigationAction::MoveBackward);phase(1);
	check(f.coordinator().state().pending()==0&&f.coordinator().frame()==0,"blocked visual step changed encounter work");
	f.now=124;f.flow->updatePresentation();phase(1);
	f.now=125;f.flow->updatePresentation();phase(2); // Action rearmed to 25+100.
	const auto actorFrame=f.coordinator().frame();const auto actorDeadline=f.coordinator().cosmeticDeadline();
	f.now=150;f.flow->handle(NavigationAction::TurnRight);phase(0);
	check(f.coordinator().frame()==actorFrame&&f.coordinator().cosmeticDeadline()==actorDeadline,"turn reset actor cosmetics");
	f.now=175;f.world.discardMapCache();f.flow->refresh(true);phase(0);
	f.now=249;f.flow->updatePresentation();phase(0);
	f.now=250;f.flow->updatePresentation();phase(1); // Reset rearmed to 150+100, cache did not rearm.
	f.now=275;f.flow->handle(NavigationAction::MoveForward);phase(2);
	check(f.coordinator().actionPending()==3&&f.coordinator().state().pending()==2&&
		f.camera.x==14&&f.p.encounterContext->minutes==490,"ordinary action affected East action3/pulse2");
	f.now=300;f.flow->refresh(true);phase(2);
	f.now=374;f.flow->updatePresentation();phase(2);check(f.coordinator().state().pending()==2,"early gameplay pulse");
	f.now=375;f.flow->updatePresentation();phase(3);check(f.coordinator().state().pending()==1,"due gameplay pulse missing");
	f.flow->updatePresentation();phase(3);check(f.coordinator().state().pending()==1,"same-time replay");
	f.now=10000;f.flow->updatePresentation();phase(4);check(f.coordinator().state().pending()==0,"backlog gameplay changed");
	f.now=10099;f.flow->updatePresentation();phase(4);
	f.now=10100;f.flow->updatePresentation();phase(5);
	Production skew;skew.ordinaryAnimated=true;skew.startFlow();skew.flow->handle(NavigationAction::TurnRight);
	unsigned clockCalls=0;
	skew.onClock=[&]{if(++clockCalls==3)skew.now=50;}; // After action/pulse adoption, before ordinary timing.
	skew.flow->handle(NavigationAction::MoveForward);skew.onClock={};
	skew.now=100;skew.flow->updatePresentation();
	check(skew.coordinator().state().pending()==1&&skew.ordinaryPhases.back()==1,"gameplay pulse advanced ordinary phase before its deadline");
	skew.now=150;skew.flow->updatePresentation();
	check(skew.coordinator().state().pending()==1&&skew.ordinaryPhases.back()==2,"ordinary deadline consumed gameplay work");
	for(const auto move:{NavigationAction::MoveForward,NavigationAction::MoveBackward}) {
		Production terminal;terminal.startFlow();
		if(move==NavigationAction::MoveBackward){terminal.flow->handle(NavigationAction::TurnRight);terminal.flow->handle(NavigationAction::TurnRight);}
		terminal.flow->handle(move);
		check(terminal.ordinaryPhases.back()==1&&terminal.coordinator().state().phase()==XeenEncounterPhase::Engaged,
			"successful same-facing movement must step once even when its pulse engages");
	}
}

void collisionFeedback() {
	Production fresh;fresh.startFlow();const auto party=fresh.p;const auto actors=fresh.world.sessionState().actors();
	unsigned modalReports=0;
	fresh.flow->reportManual=[&](const auto &){++modalReports;};fresh.flow->reportAutomatic=[&](const auto &){++modalReports;};
	fresh.flow->reportInventory=[&](const auto &){++modalReports;};fresh.flow->reportEquipment=[&](const auto &){++modalReports;};
	fresh.flow->handle(NavigationAction::MoveBackward);
	check(fresh.lastReport.find("Movement blocked by terrain.")!=std::string::npos&&fresh.reports==1&&
		fresh.camera.x==13&&fresh.camera.y==1&&fresh.p.encounterContext==party.encounterContext&&
		fresh.coordinator().actionPending()==0&&fresh.coordinator().state().pending()==0&&!fresh.coordinator().deadline()&&
		!fresh.flow->inventoryOpen()&&!fresh.flow->presentationGeneration()&&modalReports==0,"fresh collision feedback/side effects");
	sameActors(actors,fresh.world.sessionState().actors());sameParty(party,fresh.p);
	fresh.flow->handle(NavigationAction::TurnRight);
	check(fresh.lastReport.find("Movement blocked")==std::string::npos,"later action retained collision label");
	Production failure;failure.startFlow();const auto revision=failure.coordinator().state().revision();
	failure.onReport=[] {throw std::runtime_error("collision report failed");};
	failure.flow->handle(NavigationAction::MoveBackward);
	check(failure.coordinator().actionResult().revision==revision+1&&failure.coordinator().state().revision()==revision+3&&
		failure.coordinator().state().phase()==XeenEncounterPhase::SupportStopped&&failure.reports==1&&failure.rebuilds==1&&
		failure.ordinaryPhases.back()==1&&failure.p.encounterContext->minutes==480&&failure.p.encounterContext->ctr24==0,
		"collision reporting replayed action/pulse or ordinary step");
	for(bool failPulse:{false,true}) {
		Production old;old.startFlow();old.east();old.flow->handle(NavigationAction::TurnRight); // South, pending 1.
		old.world.discardMapCache();auto &cell=old.terrain.geometry.cells[14];cell.rawWord=15;cell.geometry=XeenOutdoorLayers{15,0,0,0};
		const auto context=old.p.encounterContext;const auto before=old.coordinator().state().revision();
		unsigned clocks=0;
		if(failPulse) {
			old.onClock=[&]{if(++clocks==2){old.world.discardMapCache();old.failMap=true;}};
			old.onCompose=[&]{old.failMap=false;};
		}
		old.flow->handle(NavigationAction::MoveForward);
		check(old.coordinator().actionResult().outcome==XeenEncounterOutcome::Blocked&&old.coordinator().actionPending()==1&&
			old.coordinator().state().pending()==0&&old.p.encounterContext==context&&old.camera.x==14&&old.camera.y==1,
			"blocked action lost/charged old pending work");
		check(old.coordinator().state().revision()==before+2,"blocked action/pulse publication count");
		if(failPulse)check(old.coordinator().state().phase()==XeenEncounterPhase::SupportStopped&&
			old.lastReport.find("Support stop:")!=std::string::npos&&old.lastReport.find("Movement blocked")==std::string::npos,
			"old-work terminal notice lost to collision feedback");
		else check(old.coordinator().result().movementOpportunities==1&&old.anchor().x==14&&old.anchor().y==2&&
			old.lastReport.find("Movement blocked by terrain.")!=std::string::npos,"collision feedback paused old movement");
	}
}

void timing() {
	Production f;f.startFlow();
	check(f.initializations==1&&f.spriteChecks==1&&f.compositions==1,"startup order/count");
	check(f.coordinator().frame()==0&&f.coordinator().state().pending()==0,"startup timing");
	f.flow->beginCycle(1);f.east();
	check(f.camera.x==14&&f.p.encounterContext->minutes==490&&f.coordinator().actionPending()==3&&
		f.coordinator().state().pending()==2,"East action3/pulse2");
	for(unsigned t:{99,100,199,200}) {
		f.now=t;f.flow->beginCycle(t+2);f.flow->updatePresentation();
		const auto pending=t<100?2U:t<200?1U:0U;
		check(f.coordinator().state().pending()==pending,"99/100/199/200 boundary");
		f.flow->updatePresentation();check(f.coordinator().state().pending()==pending,"same-time replay");
	}
	check(f.anchor().x==13&&f.anchor().y==1&&f.p.encounterContext->minutes==490,"idle approach/time");
	const auto revision=f.coordinator().state().revision();
	f.now=1000;f.flow->updatePresentation();check(f.coordinator().state().revision()==revision,"pulse at zero pending");
	check(f.coordinator().frame()==3,"cosmetic no-backlog");
	auto actors=f.world.sessionState().actors();auto context=f.p.encounterContext;
	const auto deadline=f.coordinator().cosmeticDeadline();
	f.world.discardMapCache();f.flow->refresh(true);f.flow->refresh();
	sameActors(actors,f.world.sessionState().actors());check(f.coordinator().frame()==3&&
		f.coordinator().cosmeticDeadline()==deadline&&context==f.p.encounterContext&&f.initializations==1,"redraw reset authority/timing");
	f.flow->handle(NavigationAction::TurnLeft);f.flow->handle(NavigationAction::TurnLeft);
	f.flow->handle(WaitAction{});check(f.anchor().x==14&&f.p.encounterContext->minutes==500&&
		f.coordinator().state().phase()==XeenEncounterPhase::Engaged,"delayed reveal/wait");
	check(f.coordinator().notice().find("Engaged: Skeleton. M26 stops before combat.")!=std::string::npos,"terminal notice");
	f.now=5000;f.flow->updatePresentation();check(f.coordinator().frame()==3,"terminal frame changed");
	Production jump;jump.startFlow();jump.east();jump.now=10000;jump.flow->updatePresentation();
	check(jump.coordinator().state().pending()==1&&jump.coordinator().deadline()==10100,"backlog processed");
	jump.now=500;jump.flow->updatePresentation();check(jump.coordinator().deadline()==10100&&jump.coordinator().state().pending()==1,"backward time rearmed");
	Production batch;batch.startFlow();batch.flow->beginCycle(1);batch.onCompose=[&]{batch.now+=200;};batch.east();
	batch.flow->updatePresentation();check(batch.coordinator().state().pending()==2,"same-batch slow input double pulse");
	batch.flow->beginCycle(2);batch.flow->updatePresentation();check(batch.coordinator().state().pending()==1,"later due batch suppressed");
	rejects([&]{batch.flow->beginCycle(1);});
	Production reentrant;reentrant.startFlow();reentrant.onCompose=[&]{
		const auto ticket=reentrant.coordinator().ticket();
		reentrant.flow->handle(WaitAction{});reentrant.flow->refresh();reentrant.flow->initial();reentrant.flow->updatePresentation();
		check(reentrant.coordinator().current(ticket),"reentrant Flow callback published");
	};reentrant.east();check(reentrant.coordinator().state().pending()==2,"reentrant input bypass");
}

void actionsAndBypasses() {
	for(int mode=0;mode<4;++mode) {
		Production f;f.startFlow();auto party=f.p;
		if(mode==0) f.flow->handle(WaitAction{});
		if(mode==1) f.flow->handle(NavigationAction::MoveForward);
		if(mode==2) {f.east();f.flow->handle(WaitAction{});check(f.coordinator().actionResult().movementOpportunities==2,"rapid Wait lost old/new opportunity");}
		if(mode==3) {f.flow->handle(NavigationAction::TurnRight);f.flow->handle(NavigationAction::TurnRight);f.flow->handle(NavigationAction::MoveBackward);}
		check(f.coordinator().state().phase()==XeenEncounterPhase::Engaged&&f.anchor().hp==20,"action engagement");
		sameParty(party,f.p);const auto actors=f.world.sessionState().actors();auto context=f.p.encounterContext;
		const auto camera=f.camera;const auto revision=f.coordinator().state().revision();
		for(const PlayerAction &a : std::vector<PlayerAction>{WaitAction{},NavigationAction::MoveForward,NavigationAction::MoveBackward,
			NavigationAction::TurnLeft,NavigationAction::TurnRight,InteractionAction{},AcknowledgeAction{},YesAction{},NoAction{},
			InspectInventoryAction{},EquipmentInventoryAction{},TransferInventoryAction{},SelectMemberAction{0},
			SelectInventorySlotAction{0},SaveGameAction{},CancelInteractionAction{}}) f.flow->handle(a);
		f.flow->abandonPresentation();f.flow->initial();f.flow->invalidateInventory();f.flow->refresh(true);
		check(!f.flow->respond(1,XeenPresentationResponse::Acknowledged)&&!f.flow->handlesEscape()&&!f.flow->inventoryOpen(),"terminal modal bypass");
		rejects([&]{f.flow->acceptManual(XeenManualEventResult{});});
		rejects([&]{f.flow->acceptAutomatic(XeenAutomaticEventResult{});});
		check(f.coordinator().state().revision()==revision&&context==f.p.encounterContext&&save_test::sameCamera(camera,f.camera),"terminal replay");
		sameActors(actors,f.world.sessionState().actors());sameParty(party,f.p);
	}
	Production f;f.startFlow();f.flow->handle(NavigationAction::MoveBackward);
	check(f.p.encounterContext->minutes==480&&f.p.encounterContext->ctr24==0&&f.coordinator().actionResult().outcome==XeenEncounterOutcome::Blocked,"real collision charged");
	f.east();f.flow->handle(NavigationAction::MoveForward);
	check(f.coordinator().state().reason()==XeenEncounterStop::Envelope&&f.camera.x==14&&f.p.encounterContext->minutes==490&&
		f.coordinator().notice().find("(15,1)")!=std::string::npos,"envelope stop");
	Production time;time.startFlow();time.east();time.p.encounterContext->minutes=950;
	auto actors=time.world.sessionState().actors();time.flow->handle(WaitAction{});
	check(time.coordinator().state().reason()==XeenEncounterStop::Time&&time.p.encounterContext->minutes==950,"time boundary");sameActors(actors,time.world.sessionState().actors());
	Production dest;dest.startFlow();dest.east();dest.flow->handle(NavigationAction::TurnLeft);dest.flow->handle(NavigationAction::MoveForward);
	check(dest.anchor().x==14&&dest.anchor().y==2&&dest.coordinator().state().phase()==XeenEncounterPhase::Engaged,"candidate destination old work");
}

void failures() {
	for(int kind=0;kind<7;++kind) {
		Production f;f.startFlow();f.east();const auto party=f.p;const auto actors=f.world.sessionState().actors();
		unsigned clockCalls=0;
		if(kind==0) f.onClock=[] {throw std::runtime_error("clock");};
		if(kind==1) {f.world.discardMapCache();f.failMap=true;}
		if(kind==2) f.onCompose=[&] {f.onCompose={};throw std::runtime_error("compose");};
		if(kind==3) f.onReport=[] {throw std::runtime_error("report");};
		if(kind==4) {f.onCompose=[] {throw std::runtime_error("rebuild");};}
		if(kind==5) f.onClock=[&]{if(++clockCalls==2)throw std::runtime_error("post-action clock");};
		if(kind==6) f.flow->beforeEncounterFrameCopy=[] {throw std::bad_alloc();};
		if(kind==1||kind==4) rejects([&]{f.flow->handle(NavigationAction::TurnLeft);});
		else f.flow->handle(NavigationAction::TurnLeft);
		check(f.coordinator().state().phase()==XeenEncounterPhase::SupportStopped&&f.coordinator().state().pending()==0,"current failure did not stop");
		check(f.rebuilds<=1,"repeated recovery");
		if(kind<=1) {sameActors(actors,f.world.sessionState().actors());check(f.p.encounterContext==party.encounterContext,"pre-action failure published");}
		else check(f.camera.direction==XeenDirection::North&&f.p.encounterContext->ctr24==3,"published action lost");
	}
	Production engaged;engaged.startFlow();engaged.onReport=[] {throw std::runtime_error("engagement notice");};
	engaged.flow->handle(WaitAction{});check(engaged.coordinator().state().phase()==XeenEncounterPhase::Engaged&&engaged.rebuilds==1,"notice failure lost engagement");
	for(int phase=0;phase<4;++phase) for(bool throws:{false,true}) {
		Production f;f.startFlow();f.east();auto newer=f.coordinator().state();bool called=false;
		auto nested=[&]{if(called)return;called=true;XeenActorApproach::action(f.world,f.p,f.camera,newer,Action::Left,f.evt);
			if(throws)throw std::runtime_error("obsolete callback");};
		if(phase==0)f.onClock=nested;
		if(phase==1)f.onCompose=nested;
		if(phase==2)f.onReport=nested;
		if(phase==0) rejects([&]{f.flow->handle(NavigationAction::TurnLeft);});
		else { // Refresh/report callbacks start from the current state on entry.
			if(phase==2) f.onReport=[&]{newer=f.coordinator().state();nested();};
			if(phase==1) f.onCompose=[&]{newer=f.coordinator().state();nested();};
			if(phase==3) f.flow->beforeEncounterFrameCopy=[&]{newer=f.coordinator().state();nested();};
			rejects([&]{f.flow->handle(NavigationAction::TurnLeft);});
		}
		check(called&&XeenActorApproach::authoritative(f.world,f.p,f.camera,newer)&&
			newer.phase()==XeenEncounterPhase::Exploring&&newer.pending()>0,"stale callback stopped/retired newer work");
		check(f.rebuilds==0,"stale callback attempted recovery");
	}
	for(bool stop:{false,true}) {
		Production f;f.startFlow();auto newer=f.coordinator().state();
		f.onCompose=[&]{
			if(stop)XeenActorApproach::stop(f.world,newer,XeenEncounterStop::Domain);
			else XeenActorApproach::action(f.world,f.p,f.camera,newer,Action::Wait,f.evt);
		};
		rejects([&]{f.flow->refresh();});
		check(XeenActorApproach::authoritative(f.world,f.p,f.camera,newer)&&newer.phase()==
			(stop?XeenEncounterPhase::SupportStopped:XeenEncounterPhase::Engaged),"stale presentation overwrote terminal authority");
	}
}

void ordinaryWait() {
	Fixture f;XeenFontFormat font{gameplay_test::fontBytes()};XeenGameFlags flags;
	XeenEventSystem events([](XeenMapIdentity){return XeenEventScript(encounter_test::events());},[](XeenMapIdentity){return XeenEventTextFile{};});
	unsigned compositions=0,clocks=0;
	XeenEventFlow flow(f.world,events,f.p,f.camera,flags,font,[&](std::uint64_t){
		++compositions;XeenEventFlow::Composition c;c.frame.width=320;c.frame.height=200;c.frame.pixels.resize(64000);return c;
	},{},[&]{++clocks;return std::uint64_t{0};});
	for(bool inventory:{false,true}) {
		if(inventory)flow.handle(InspectInventoryAction{});
		const auto count=compositions,timeCalls=clocks;const auto pixels=flow.frame().pixels;
		const auto open=flow.inventoryOpen();flow.handle(WaitAction{});
		check(count==compositions&&timeCalls==clocks&&pixels==flow.frame().pixels&&open==flow.inventoryOpen(),"ordinary period not a no-op");
	}
	check(!f.world.hasEncounterState()&&f.world.sessionState().actors().empty(),"ordinary map20 access initialized actors");
}

void projections() {
	Fixture f;f.start();auto actors=f.world.sessionState().actors();
	const int orders[]{118,94,90,91}, slots[]{0,3,12,13}, queries[]{2,7,5,9}, xs[]{-5,-7,-112,98};
	for(unsigned d=0;d<4;++d)for(int placement=0;placement<4;++placement) {
		XeenCamera c{20,13,1,static_cast<XeenDirection>(d)};
		const int forwardX[]{0,1,0,-1},forwardY[]{1,0,-1,0};
		const int lateral=placement==2?-1:placement==3?1:0;
		actors[5].x=c.x+(placement?forwardX[d]:0)+lateral*forwardY[d];
		actors[5].y=c.y+(placement?forwardY[d]:0)-lateral*forwardX[d];
		const auto commands=XeenOutdoorScene::actorCommands(actors,c,7);check(commands.size()==1,"missing actor projection");
		const auto &v=commands[0];const auto *a=v.actor();const auto options=v.drawOptions();
		check(a&&a->image==42&&a->frame==7&&a->identity.recordIndex==5&&a->selectedSlot==slots[placement]&&
			v.originalOrder==orders[placement]&&v.sampleIndex==queries[placement]&&v.x==xs[placement]&&v.y==(placement?34:2)&&
			options.scaleIndex==(placement?8:0)&&options.bottomClipped==(placement==0)&&options.sceneClipped&&!options.horizontalFlip&&!options.enlarge,"literal monster placement");
	}
	actors[0].x=13;actors[0].y=2;rejects([&]{XeenOutdoorScene::actorCommands(actors,XeenActorApproach::kEntry,0);});
}
}
int main(){try{ordinaryNavigation();collisionFeedback();timing();actionsAndBypasses();failures();projections();ordinaryWait();std::cout<<"Encounter production Flow timing, terminal, projection and reentrancy controls passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
