#include "XeenCheckpointTestSupport.h"
#include "XeenVisualRemoveTestSupport.h"
#include "XeenPartySnapshotTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace mmodern;
using namespace remove_test;

namespace {
bool sameCamera(const XeenCamera &a, const XeenCamera &b) {
	return a.mapId==b.mapId && a.x==b.x && a.y==b.y && a.direction==b.direction;
}


void runCase(XeenAssetSource &assets, const std::filesystem::path &output,
		const std::string &name, bool sdl) {
	const bool held = name=="owned", yes = name!="no";
	const auto originalParty = assets.readInitialResource("maze.pty");
	const auto originalRoster = assets.readInitialResource("maze.chr");
	const auto originalEvt = assets.readInitialResource("maze0023.evt");
	const auto originalMob = assets.readInitialResource("maze0023.mob");
	const auto initial = XeenPartyLoader().loadInitialCloudsParty(assets);
	for (std::size_t i=0; i<35; ++i)
		check(initial.questItems.at(i)==originalParty.at(747+i),"real initial prefix mapping");
	check(initial.questItems.at(17)==0,"initial Phirna count not zero");
	// Controlled in-memory fixture only. Original resources and runtime controls
	// are unchanged; all three paths still enter production interaction at line 0.
	auto partyBytes=originalParty;
	if(held) partyBytes.at(XeenQuestItemFormat::kCloudsOffset+17)=1;
	auto party=XeenPartyLoader().loadFromResources(originalRoster,partyBytes);
	const auto partyBefore=party.questItems.counts();
	const auto membersBefore=partySnapshot(party);
	const auto questFlagsBefore=party.questFlags.values();
	bool harvested=false;
	const XeenMapLoader mapLoader;
	int mapLoads=0, objectLoads=0, scriptLoads=0, textLoads=0;
	XeenWorld world([&](XeenMapIdentity id){++mapLoads;return mapLoader.loadGeometryMap(assets,id);},
		[&](XeenMapIdentity id){++objectLoads;return mapLoader.loadObjects(assets,id);});
	XeenEventLoader loader([&](const std::string &resource)->std::optional<std::vector<std::uint8_t>>{
		if(!assets.hasInitialResource(resource))return {};return assets.readInitialResource(resource);
	});
	XeenEventTextLoader textLoader([&](const std::string &resource)->std::optional<std::vector<std::uint8_t>>{
		if(!assets.hasArchiveResource(resource))return {};return assets.readArchiveResource(resource);
	});
	XeenEventSystem events([&](XeenMapIdentity id){++scriptLoads;return XeenEventScript(loader.load(id));},
		[&](XeenMapIdentity id){++textLoads;return textLoader.load(id);});
	XeenCamera camera=checkpoint_test::phirna; const auto start=camera;
	auto flags=XeenGameFlagsLoader().loadInitialCloudsFlags(assets);const auto flagsBefore=flags.values();
	const auto geometry=geometrySnapshot(world.map(23).geometry);
	const auto objects=world.objectFile(23);
	check(objects.entities.objects.at(13).resourceId==111,"original Phirna resource identity");
	check(world.selectObject(camera)==XeenObjectIdentity{23,13},"real plant selection");
	const auto script=loader.load(23);
	const std::array<std::size_t,11> offsets{1056,1063,1072,1078,1087,1094,1103,1113,1119,1125,1132};
	const std::array<int,11> opcodes{1,9,0x12,9,1,9,0x0c,0x0e,0x12,0x29,9};
	const std::vector<std::vector<std::uint8_t>> operands{{30},{44,0,3},{},{21,99,9},{31},{44,1,6},
		{0,0,21,99},{},{},{32},{44,1,11}};
	check(script.records.size()==170,"real EVT count");
	for(std::size_t i=0;i<11;++i){const auto &r=script.records.at(125+i);
		check(r.fileOffset==offsets[i] && r.x==8 && r.y==2 && r.direction==4 && r.line==i &&
			r.opcode==opcodes[i] && r.parameters==operands[i],"real Phirna record mismatch");}
	const XeenFontFormat font(assets.readArchiveResource("fnt"));
	const CloudsMapComposer composer; const XeenCharacterRulesContext rules{kCloudsInitialYear};
	XeenEventFlow flow(world,events,party,camera,flags,font,[&]{return composer.compose(assets,world,party,camera,rules);});
	const auto base=flow.frame();
	const auto resolver=XeenObjectVisualResolver::load(assets);
	const auto draw=XeenOutdoorScene().build(world,camera,&resolver);
	check(std::any_of(draw.begin(),draw.end(),[](const auto &c){return c.object() && c.object()->visual.identity==XeenObjectIdentity{23,13};}),
		"visible Phirna draw command absent");
	visual_remove_test::save(base,output/(name+"-before.bmp"));
	std::vector<int> presentedText;
	std::vector<int> presentationLines;
	std::optional<XeenManualEventResult> terminal;
	std::optional<XeenPresentationRequest> successRequest;
	flow.reportText=[](const std::string &message){throw std::runtime_error("presentation diagnostic: "+message);};
	flow.reportManual=[&](const XeenManualEventResult &r){
		if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&r)){
			presentationLines.push_back(s->request.source.line);
			if(s->request.textIndex==31)successRequest=s->request;
			if(s->request.textIndex)presentedText.push_back(*s->request.textIndex);
		}else {terminal=r;if(yes && !held && std::holds_alternative<XeenManualEventCompleted>(r))harvested=true;}
	};
	auto unchanged=[&]{
		auto expected=partyBefore;if(harvested)expected[17]=1;
		checkPartyQuestState(party,expected,questFlagsBefore);
		check(partySnapshot(party)==membersBefore &&
			flags.values()==flagsBefore,"unrelated party/flags changed or root count wrong");
		check(world.sessionState().disabledObjectCount()==(harvested?1:0) &&
			world.sessionState().disabledEventCount()==(harvested?11:0),"unexpected Remove scope");
		check(world.isObjectDisabled({23,13})==harvested,"plant visibility/ownership mismatch");
	};
	struct Step { SDL_Keycode key; PlayerAction action; bool pending; const char *image; };
	std::vector<Step> steps{{SDLK_SPACE,InteractionAction{},true,"question"},
		{SDLK_UP,NavigationAction::MoveForward,true,nullptr},
		{yes?SDLK_y:SDLK_n,yes?PlayerAction{YesAction{}}:PlayerAction{NoAction{}},yes,yes?"message":"result"}};
	if(yes){steps.push_back({SDLK_UP,NavigationAction::MoveForward,true,nullptr});
		steps.push_back({SDLK_RETURN,AcknowledgeAction{},false,"result"});}
	steps.push_back({SDLK_RIGHT,NavigationAction::TurnRight,false,"recovered"});
	std::size_t stepIndex=0;
	auto advance=[&](const PlayerAction &action)->std::optional<IndexedFrame>{
		check(stepIndex<steps.size(),"repeat key escaped SDL suppression");
		const auto &step=steps[stepIndex];
		check(action.index()==step.action.index(),"SDL action mapping/order");
		if(const auto *navigation=std::get_if<NavigationAction>(&action))
			check(*navigation==std::get<NavigationAction>(step.action),"SDL navigation mapping");
		const auto frame=flow.handle(action);
		check(flow.blocksGameplay()==step.pending,"unexpected presentation blocking");
		if(stepIndex+1<steps.size())check(sameCamera(camera,start),"pending/completed interaction moved camera");
		else check(camera.direction==XeenDirection::East,"navigation did not recover");
		unchanged();
		if(yes && !held && stepIndex==4){
			check(harvested && sameCamera(camera,start),"harvest needed movement");
			check(successRequest.has_value(),"missing success presentation");
			const auto effective=composer.compose(assets,world,party,camera,rules);
			XeenEventPresenter oracle(font);
			check(frame.pixels==oracle.present(effective,*successRequest).frame.pixels,
				"returned harvest frame is not effective scene with retained success text");
			check(frame.pixels!=effective.pixels,"success text erased");
			const auto commands=XeenOutdoorScene().build(world,camera,&resolver);
			check(std::none_of(commands.begin(),commands.end(),[](const auto &c){
				return c.object() && c.object()->visual.identity==XeenObjectIdentity{23,13};}),"plant draw remains");
		}
		if(step.image)visual_remove_test::save(frame,output/(name+"-"+step.image+".bmp"));
		if(stepIndex==0){
			check(frame.pixels!=base.pixels,"question not rendered");
			world.discardMapCache();events.discardScriptCache();events.discardTextCache();assets.discardSpriteCache();
			check(flow.refresh(true).pixels==frame.pixels && flow.blocksGameplay(),"pending cache reconstruction lost question");
		}
		++stepIndex;return frame;
	};
	if(sdl){
		std::atomic<std::size_t> handled{0};std::atomic<bool> finished{false};
		std::thread input([&]{
			auto push=[](SDL_Keycode key,int repeat){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.state=SDL_PRESSED;
				e.key.keysym.sym=key;e.key.repeat=repeat;return SDL_PushEvent(&e)==1;};
			for(int attempt=0;attempt<200 && !finished && !SDL_WasInit(SDL_INIT_VIDEO);++attempt)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			if(!finished)push(SDLK_SPACE,1);
			for(std::size_t i=0;i<steps.size() && !finished;++i){
				if(!push(steps[i].key,0))break;
				for(int n=0;n<500 && !finished && handled<=i;++n)std::this_thread::sleep_for(std::chrono::milliseconds(10));
				if(handled<=i)break;
			}
			if(!finished){if(held){SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}else push(SDLK_ESCAPE,0);}
		});
		const bool ok=SdlWindow().showInteractive(base,"M17B Phirna "+name,[&](const PlayerAction &action){
			auto frame=advance(action);handled=stepIndex;return frame;});
		finished=true;input.join();check(ok && stepIndex==steps.size(),"SDL decision path failed");
	}else for(const auto &step:steps)advance(step.action);
	check(terminal.has_value(),"no terminal result");
	const auto *d=std::get_if<XeenManualEventCompleted>(&*terminal);
	check(d && d->instructionCount==(held?5:yes?18:3),"unexpected original path instruction count");
	std::cout<<name<<": completed; count="<<party.questItems.at(17)<<", plant "
		<<(harvested?"removed":"present")<<", "<<d->instructionCount<<" instructions\n";
	check(presentedText==(yes?std::vector<int>{30,held?32:31}:std::vector<int>{30}),"wrong original presentation sequence");
	check(presentationLines==(yes?std::vector<int>{0,1,held?9:4,held?10:5}:std::vector<int>{0,1}),"wrong original suspension addresses");

	// Re-interaction must consume eleven effective None records, with no text.
	camera=start;flow.refresh();
	auto interactAgain=[&]{
		const auto presentations=presentedText.size();terminal.reset();
		flow.handle(InteractionAction{});
		if(!harvested)flow.handle(NoAction{});
		check(terminal && std::holds_alternative<XeenManualEventCompleted>(*terminal) && !flow.blocksGameplay(),
			"reconstructed interaction failed");
		if(harvested)check(std::get<XeenManualEventCompleted>(*terminal).instructionCount==11 &&
			presentedText.size()==presentations,"removed cell replayed original branch");
		unchanged();
	};
	interactAgain();flow.handle(NavigationAction::TurnRight);flow.handle(NavigationAction::TurnLeft);
	const auto persistent=flow.frame();
	camera={1,1,14,XeenDirection::West};flow.refresh();camera=start;
	check(flow.refresh().pixels==persistent.pixels,"leave/return lost scene");
	// Independent cache operations available in the existing architecture:
	// geometry and object caches share discardMapCache.
	for(int cache=0;cache<5;++cache){
		const auto oldMaps=mapLoads,oldObjects=objectLoads,oldScripts=scriptLoads,oldTexts=textLoads;
		const auto oldSprites=assets.spriteLoadCount();
		if(cache==0 || cache==4)world.discardMapCache();
		if(cache==1 || cache==4)events.discardScriptCache();
		if(cache==2 || cache==4)events.discardTextCache();
		if(cache==3 || cache==4)assets.discardSpriteCache();
		flow.refresh(true);interactAgain();
		if(cache==0 || cache==4)check(mapLoads>oldMaps && objectLoads>oldObjects,"map/object providers not reloaded");
		if(cache==1 || cache==4)check(scriptLoads>oldScripts,"script provider not reloaded");
		if(cache==2 || cache==4){
			// Removed plant has no text. Exercise original Castle text via the
			// same party/world/events/presenter graph to prove its cache reload.
			camera={1,8,8,XeenDirection::West};flow.refresh();flow.handle(InteractionAction{});
			check(flow.blocksGameplay(),"Castle question missing");flow.handle(NoAction{});
			check(textLoads>oldTexts,"text provider not reloaded");
			camera=start;flow.refresh();
		}
		if(cache==3 || cache==4)check(assets.spriteLoadCount()>oldSprites,"sprite cache not reloaded");
		// Clear retained No/question text before comparing identical scene states.
		flow.handle(NavigationAction::TurnRight);flow.handle(NavigationAction::TurnLeft);
		check(flow.frame().pixels==composer.compose(assets,world,party,camera,rules).pixels,"cache restored stale scene");
		unchanged();
	}
	visual_remove_test::save(flow.frame(),output/(name+"-rebuilt.bmp"));
	sameEntities(objects.entities,world.objectFile(23).entities);
	check(geometrySnapshot(world.map(23).geometry)==geometry,"geometry changed");
	for(std::size_t i=0;i<script.records.size();++i){
		const auto effective=world.effectiveEvent({23,i},script.records[i]);
		check(sameRecord(script.records[i],effective,false) &&
			effective.opcode==((harvested && i>=125 && i<=135)?0:script.records[i].opcode),"unrelated event changed");
	}
	// New party/world/event/presenter owners, never reusing the old continuation.
	auto freshParty=XeenPartyLoader().loadInitialCloudsParty(assets);
	XeenWorld freshWorld([&](XeenMapIdentity id){return mapLoader.loadGeometryMap(assets,id);},
		[&](XeenMapIdentity id){return mapLoader.loadObjects(assets,id);});
	XeenEventSystem freshEvents([&](XeenMapIdentity id){return XeenEventScript(loader.load(id));},
		[&](XeenMapIdentity id){return textLoader.load(id);});
	auto freshCamera=start;auto freshFlags=XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
	XeenEventFlow fresh(freshWorld,freshEvents,freshParty,freshCamera,freshFlags,font,
		[&]{return composer.compose(assets,freshWorld,freshParty,freshCamera,rules);});
	check(!fresh.blocksGameplay() && freshParty.questItems.at(17)==0 && freshWorld.selectObject(start)==XeenObjectIdentity{23,13} &&
		fresh.frame().pixels==base.pixels,"new session retained grant/removal/presentation");
	for(std::size_t i=125;i<=135;++i)check(sameRecord(script.records[i],freshWorld.effectiveEvent({23,i},script.records[i])),"fresh event not restored");
	visual_remove_test::save(fresh.frame(),output/(name+"-fresh.bmp"));
	fresh.handle(InteractionAction{});check(fresh.blocksGameplay(),"fresh line-0 question missing");
	fresh.handle(NoAction{});check(!fresh.blocksGameplay() && freshParty.questItems.at(17)==0,"fresh No path failed");
	check(originalParty==assets.readInitialResource("maze.pty") && originalRoster==assets.readInitialResource("maze.chr") &&
		originalEvt==assets.readInitialResource("maze0023.evt") && originalMob==assets.readInitialResource("maze0023.mob"),"original resource bytes changed");
	check(XeenPartyLoader().loadInitialCloudsParty(assets).questItems.at(17)==0,"controlled state leaked into fresh party");
	std::cout<<"Provider reloads maps="<<mapLoads<<" objects="<<objectLoads<<" scripts="<<scriptLoads<<" texts="<<textLoads<<'\n';
}
}

int main(int argc,char **argv) {
	try {
		check(argc==3 || (argc==4 && std::string(argv[3])=="sdl"),"usage: mmodern_phirna_smoke <game-directory> <output-directory> [sdl]");
		std::filesystem::create_directories(argv[2]);
		const auto installation=XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(),"Clouds installation unavailable");
		XeenAssetSource assets(*installation,320,200);
		for(const auto *name:{"no","yes","owned"})runCase(assets,argv[2],name,argc==4);
		std::cout<<"M17B original harvesting, refusal, no-replay, reconstruction and fresh-session acceptance OK\n";
		return 0;
	}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
