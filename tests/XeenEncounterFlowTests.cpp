#include "XeenRegionalJourneyTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "games/xeen/XeenOutdoorScene.h"
#include <iostream>
#include <limits>
using namespace encounter_test;
using Action=XeenEncounterAction;
namespace {
struct Production {
    XeenPartyState p;XeenCamera camera;XeenGameFlags flags;
    XeenMap terrain=regional_journey_test::regionalMap();
    XeenObjectFile objects=regional_journey_test::regionalObjects();
    bool failMap=false;std::function<void()> onMapFailure;
    XeenWorld world{[&](auto){if(failMap){if(onMapFailure)onMapFailure();throw std::runtime_error("map failure");}return terrain;},[&](auto){return objects;}};
    XeenEventSystem system{[](auto id){return XeenEventScript(regional_test::events(id));},regional_test::texts};
    XeenFontFormat font{gameplay_test::fontBytes()};
    std::unique_ptr<XeenEventFlow> flow;
    std::uint64_t now=0;
    unsigned initializations=0,spriteChecks=0,compositions=0,reports=0,rebuilds=0;
    std::vector<std::uint64_t> ordinaryPhases;
    std::string lastReport;
    bool ordinaryAnimated=false;
    std::function<void()> onClock,onCompose,onReport,onSprite,onRebuild;
    void startFlow(){
        regional_journey_test::Fixture source;
        const auto saved=XeenSaveState::capture(regional_test::signature(),source.p,source.camera,source.flags,source.w);
        XeenSaveState::restoreBeforeGameplay(saved,regional_test::resources(),p,camera,flags,world,[](auto &,const auto &,const auto &,const auto &){});
        ++initializations;
        flow=std::make_unique<XeenEventFlow>(world,system,p,camera,flags,font,
            [](std::uint64_t)->XeenEventFlow::Composition{throw std::runtime_error("ordinary composer reached");},
            XeenEventPresenter::NpcDraw{},[&]{if(onClock){auto callback=onClock;callback();}return now;},XeenEventPresenter::RandomFrame{},nullptr,
            [&](std::uint64_t ordinary,XeenMonsterAppearance appearance){
                check(appearance.valid(),"valid actor appearance");++compositions;ordinaryPhases.push_back(ordinary);
                if(onCompose){auto callback=onCompose;callback();}
                world.map(23);world.objectFile(23);
                XeenEventFlow::Composition c;c.frame.width=320;c.frame.height=200;c.frame.pixels.resize(64000);
                c.frame.pixels[0]=appearance.frame;c.frame.pixels[1]=camera.x;c.frame.pixels[2]=camera.y;c.frame.pixels[3]=ordinary%256;
                c.containsOrdinaryAnimation=ordinaryAnimated;return c;
            });
        flow->prepareJourneySprites=[&]{++spriteChecks;if(onSprite)onSprite();};
        flow->reportText=[&](const auto &text){++reports;lastReport=text;if(onReport)onReport();};
        flow->rebuildEncounterPresentation=[&]{++rebuilds;if(onRebuild)onRebuild();};
        flow->framePresented(flow->frame().presentation());
    }
    const XeenEncounterFlow &coordinator()const{return *flow->encounter();}
    const XeenActor &anchor()const{return world.sessionState().actors()[5];}
    void handle(const PlayerAction &action){flow->handle(action,flow->displayedInput(),flow->frame().presentation());flow->framePresented(flow->frame().presentation());}
    void update(){flow->updatePresentation();flow->framePresented(flow->frame().presentation());}
    void refresh(bool rebuild=false){flow->refresh(rebuild);flow->framePresented(flow->frame().presentation());}
    void east(){handle(NavigationAction::TurnRight);handle(NavigationAction::MoveForward);}
};
void ordinaryNavigation() {
	Production f;f.ordinaryAnimated=true;f.startFlow();
	const auto phase=[&](std::uint64_t expected){
		check(f.ordinaryPhases.back()==expected&&f.flow->frame().pixels[3]==expected,"production ordinary phase");
	};
	phase(0);f.now=25;f.handle(NavigationAction::MoveBackward);phase(1);
	check(f.coordinator().state().pending()==0&&f.coordinator().frame()==0,"blocked visual step changed encounter work");
	f.now=124;f.update();phase(1);
	f.now=125;f.update();phase(2); // Action rearmed to 25+100.
	const auto actorFrame=f.coordinator().frame();const auto actorDeadline=f.coordinator().cosmeticDeadline();
	f.now=150;f.handle(NavigationAction::TurnRight);phase(0);
	check(f.coordinator().frame()==actorFrame&&f.coordinator().cosmeticDeadline()==actorDeadline,"turn reset actor cosmetics");
	f.now=175;f.world.discardMapCache();f.refresh(true);phase(0);
	f.now=249;f.update();phase(0);
	f.now=250;f.update();phase(1); // Reset rearmed to 150+100, cache did not rearm.
	f.now=275;f.handle(NavigationAction::MoveForward);phase(2);
	check(f.coordinator().actionPending()==3&&f.coordinator().state().pending()==2&&
		f.camera.x==14&&f.p.encounterContext->minutes==490,"ordinary action affected East action3/pulse2");
	f.now=300;f.refresh(true);phase(2);
	f.now=374;f.update();phase(2);check(f.coordinator().state().pending()==2,"early gameplay pulse");
	f.now=375;f.update();phase(3);check(f.coordinator().state().pending()==1,"due gameplay pulse missing");
	f.update();phase(3);check(f.coordinator().state().pending()==1,"same-time replay");
	f.now=10000;f.update();phase(4);check(f.coordinator().state().pending()==0,"backlog gameplay changed");
	f.now=10099;f.update();phase(4);
	f.now=10100;f.update();phase(5);
	Production skew;skew.ordinaryAnimated=true;skew.startFlow();skew.handle(NavigationAction::TurnRight);
	unsigned clockCalls=0;
	skew.onClock=[&]{if(++clockCalls==3)skew.now=50;}; // After action/pulse adoption, before ordinary timing.
	skew.handle(NavigationAction::MoveForward);skew.onClock={};
	skew.now=100;skew.update();
	check(skew.coordinator().state().pending()==1&&skew.ordinaryPhases.back()==1,"gameplay pulse advanced ordinary phase before its deadline");
	skew.now=150;skew.update();
	check(skew.coordinator().state().pending()==1&&skew.ordinaryPhases.back()==2,"ordinary deadline consumed gameplay work");
	for(const auto move:{NavigationAction::MoveForward,NavigationAction::MoveBackward}) {
		Production terminal;terminal.startFlow();
		if(move==NavigationAction::MoveBackward){terminal.handle(NavigationAction::TurnRight);terminal.handle(NavigationAction::TurnRight);}
		terminal.handle(move);
		check(terminal.ordinaryPhases.back()==1&&terminal.coordinator().state().phase()==XeenEncounterPhase::Engaged,
			"successful same-facing movement must step once even when its pulse engages");
	}
}

void timing() {
	Production f;f.startFlow();
	check(f.initializations==1&&f.spriteChecks==0&&f.compositions==1,"startup order/count");
	check(f.coordinator().frame()==0&&f.coordinator().state().pending()==0,"startup timing");
	f.flow->beginCycle(1);f.east();
	check(f.camera.x==14&&f.p.encounterContext->minutes==490&&f.coordinator().actionPending()==3&&
		f.coordinator().state().pending()==2,"East action3/pulse2");
	for(unsigned t:{99,100,199,200}) {
		f.now=t;f.flow->beginCycle(t+2);f.update();
		const auto pending=t<100?2U:t<200?1U:0U;
		check(f.coordinator().state().pending()==pending,"99/100/199/200 boundary");
		f.update();check(f.coordinator().state().pending()==pending,"same-time replay");
	}
	check(f.anchor().x==13&&f.anchor().y==1&&f.p.encounterContext->minutes==490,"idle approach/time");
	const auto revision=f.coordinator().state().revision();
	f.now=1000;f.update();check(f.coordinator().state().revision()==revision,"pulse at zero pending");
	check(f.coordinator().frame()==3,"cosmetic no-backlog");
	std::vector<XeenActor> actors=f.world.sessionState().actors();auto context=f.p.encounterContext;
	const auto deadline=f.coordinator().cosmeticDeadline();
	f.world.discardMapCache();f.refresh(true);f.refresh();
	sameActors(actors,f.world.sessionState().actors());check(f.coordinator().frame()==3&&
		f.coordinator().cosmeticDeadline()==deadline&&context==f.p.encounterContext&&f.initializations==1,"redraw reset authority/timing");
	Production jump;jump.startFlow();jump.east();jump.now=10000;jump.update();
	check(jump.coordinator().state().pending()==1&&jump.coordinator().deadline()==10100,"backlog processed");
	jump.now=500;jump.update();check(jump.coordinator().deadline()==10100&&jump.coordinator().state().pending()==1,"backward time rearmed");
	Production batch;batch.startFlow();batch.flow->beginCycle(1);batch.onCompose=[&]{batch.now+=200;};batch.east();
	batch.update();check(batch.coordinator().state().pending()==2,"same-batch slow input double pulse");
	batch.flow->beginCycle(2);batch.update();check(batch.coordinator().state().pending()==1,"later due batch suppressed");
	rejects([&]{batch.flow->beginCycle(1);});
	Production reentrant;reentrant.startFlow();reentrant.onCompose=[&]{
		const auto ticket=reentrant.coordinator().ticket();
		reentrant.flow->handle(WaitAction{});reentrant.flow->refresh();reentrant.flow->initial();reentrant.flow->updatePresentation();
		check(reentrant.coordinator().current(ticket),"reentrant Flow callback published");
	};reentrant.east();check(reentrant.coordinator().state().pending()==2,"reentrant input bypass");
}

void collisionFeedback(){
    Production f;f.startFlow();const auto context=f.p.encounterContext;const auto actors=f.world.sessionState().actors();
    unsigned modal=0;f.flow->reportManual=[&](const auto &){++modal;};f.flow->reportAutomatic=[&](const auto &){++modal;};
    f.flow->reportInventory=[&](const auto &){++modal;};f.flow->reportEquipment=[&](const auto &){++modal;};
    f.handle(NavigationAction::MoveBackward);
    check(f.coordinator().journeyRefusal()=="Movement blocked by terrain."&&f.camera.x==13&&f.camera.y==1&&f.p.encounterContext==context&&
        f.coordinator().actionPending()==0&&f.coordinator().state().pending()==0&&!f.coordinator().deadline()&&!f.flow->inventoryOpen()&&!f.flow->presentationGeneration()&&modal==0,"fresh collision notice and side effects");
    sameActors(actors,f.world.sessionState().actors());f.handle(NavigationAction::TurnRight);
    check(f.coordinator().journeyRefusal().find("Movement blocked")==std::string::npos,"later action clears collision notice");
    for(bool failPulse:{false,true}){
        Production old;auto &cell=old.terrain.geometry.cells[14];cell.rawWord=15;cell.surfaceIndex=15;cell.geometry=XeenOutdoorLayers{15,0,0,0};
        old.startFlow();old.east();old.handle(NavigationAction::TurnRight);
        const auto context=old.p.encounterContext;unsigned clocks=0;bool fired=false;
        if(failPulse){old.onClock=[&]{if(++clocks==2){fired=true;old.world.discardMapCache();old.failMap=true;}};old.onCompose=[&]{old.failMap=false;};}
        try{old.handle(NavigationAction::MoveForward);}catch(const std::exception &){}
        check(old.coordinator().actionResult().outcome==XeenEncounterOutcome::Blocked&&old.coordinator().actionPending()==1&&old.p.encounterContext==context&&old.camera.x==14&&old.camera.y==1,
            "blocked action retains old work without charging");
        if(failPulse)check(fired&&!old.flow->canSave(),"pending pulse failure cannot open save boundary");
        else check(old.coordinator().state().pending()==0&&old.coordinator().result().movementOpportunities==1&&old.anchor().x==14&&old.anchor().y==2&&old.coordinator().journeyRefusal()=="Movement blocked by terrain.","blocked action drains exactly the prior opportunity");
    }

}
void failures(){
    for(unsigned kind=0;kind<7;++kind){

        Production f;f.startFlow();f.east();const auto context=f.p.encounterContext;const auto actors=f.world.sessionState().actors();
        unsigned clocks=0;bool fired=false;
        if(kind==0)f.onClock=[&]{fired=true;throw std::runtime_error("clock");};
        if(kind==1){f.world.discardMapCache();f.failMap=true;f.onMapFailure=[&]{fired=true;};}
        if(kind==2)f.onCompose=[&]{fired=true;f.onCompose={};throw std::runtime_error("compose once");};
        if(kind==3)f.onReport=[&]{fired=true;throw std::runtime_error("report");};
        if(kind==4)f.onCompose=[&]{fired=true;throw std::runtime_error("compose and recovery");};
        if(kind==5)f.onClock=[&]{if(++clocks==2){fired=true;throw std::runtime_error("post-action clock");}};
        if(kind==6)f.flow->beforeEncounterFrameCopy=[&]{fired=true;throw std::bad_alloc();};
        try{f.handle(NavigationAction::TurnLeft);}catch(const std::exception &){}
        check(fired,"navigation failure seam fired");check(f.rebuilds<=1,"bounded presentation recovery");
        if(kind<=1){sameActors(actors,f.world.sessionState().actors());check(f.p.encounterContext==context,"pre-action failure publishes no time");}
        else check(f.camera.direction==XeenDirection::North&&f.p.encounterContext->ctr24==3,"post-action failure retains published action");
        if(kind==0||kind==1||kind==3||kind==4||kind==5)check(!f.flow->canSave(),"failed navigation never opens save boundary");
        else{
            check(f.rebuilds==1&&f.flow->acceptsFrame(f.flow->frame().presentation())&&f.flow->acceptsInputFrame(f.flow->frame().presentation()),"one-shot presentation failure recovers a matching presented frame");
            f.onCompose={};f.flow->beforeEncounterFrameCopy={};f.now=100;f.update();
            check(f.flow->canSave(),"recovered navigation remains usable through quiet boundary");
        }
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
	std::vector<XeenActor> actors=XeenActorApproach::actorsFromResources(mob(),stats());
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
		for(std::uint8_t frame=0;frame<4;++frame) {
			const XeenMonsterAppearance attack{XeenMonsterSpriteKind::Attack,frame};
			if(placement) { rejects([&]{XeenOutdoorScene::actorCommands(actors,c,attack);}); continue; }
			const auto draws=XeenOutdoorScene::actorCommands(actors,c,attack);
			const auto &draw=draws.at(0);
			check(draw.originalOrder==121&&draw.x==-5&&draw.y==2&&draw.actor()->kind==XeenMonsterSpriteKind::Attack&&
				draw.actor()->frame==frame&&draw.drawOptions().scaleIndex==0&&draw.drawOptions().sceneClipped&&
				draw.drawOptions().bottomClipped,"literal ATT placement");
		}
		rejects([&]{XeenOutdoorScene::actorCommands(actors,c,{XeenMonsterSpriteKind::Attack,4});});
	}
	actors[0]=actors[5];actors[0].id={20,0};actors[0].x=13;actors[0].y=2;
	const XeenCamera projectionCamera{20,13,1,XeenDirection::North};
	const auto multiple=XeenOutdoorScene::actorCommands(actors,projectionCamera,0);
	check(std::any_of(multiple.begin(),multiple.end(),[](const auto &c){return c.actor()->identity.recordIndex==0;}),"selected-slot renderer admits additional original identities");
	actors[0].statistics.reset();rejects([&]{XeenOutdoorScene::actorCommands(actors,projectionCamera,0);});
}
}
int main(){try{ordinaryNavigation();timing();collisionFeedback();failures();projections();ordinaryWait();std::cout<<"Regional Flow timing and detached projections passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
