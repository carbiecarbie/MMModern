#include "XeenVisualRemoveTestSupport.h"
#include "XeenPartySnapshotTestSupport.h"
#include <algorithm>
#include "formats/xeen/XeenAssetSource.h"
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
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace mmodern;
using namespace remove_test;
namespace {
struct Step {
	SDL_Keycode key;
	PlayerAction action;
	std::function<void(const IndexedFrame &)> verify;
};

// All gameplay actions, including re-interaction after reconstruction and in a
// fresh session, use the same input path. Only diagnostic positioning/cache
// controls remain outside the production SDL -> PlayerAction -> Flow route.
void input(XeenEventFlow &flow, bool sdl, const std::vector<Step> &steps, bool quit=false) {
	std::size_t index=0;
	auto advance=[&](const PlayerAction &action)->std::optional<IndexedFrame> {
		check(index<steps.size(),"repeat escaped SDL suppression");
		const auto &step=steps[index];
		check(action.index()==step.action.index(),"SDL action mapping/order");
		if(const auto *n=std::get_if<NavigationAction>(&action))
			check(*n==std::get<NavigationAction>(step.action),"SDL navigation mapping");
		if(const auto *m=std::get_if<SelectMemberAction>(&action))
			check(m->partyIndex==std::get<SelectMemberAction>(step.action).partyIndex,"SDL member mapping");
		auto frame=flow.handle(action);step.verify(frame);++index;return frame;
	};
	if(!sdl) {for(const auto &step:steps)advance(step.action);return;}
	std::atomic<std::size_t> handled{0};std::atomic<bool> finished{false};
	std::thread sender([&] {
		auto push=[](SDL_Keycode key,int repeat) {
			SDL_Event e{};e.type=SDL_KEYDOWN;e.key.state=SDL_PRESSED;
			e.key.keysym.sym=key;e.key.repeat=repeat;return SDL_PushEvent(&e)==1;
		};
		for(int n=0;n<200 && !finished && !SDL_WasInit(SDL_INIT_VIDEO);++n)
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		for(std::size_t i=0;i<steps.size() && !finished;++i) {
			if(!push(steps[i].key,0))break;
			// Enqueued together: selection/cancel repeat is processed after the original
			// action has changed the pending state. It must still be ignored.
			push(steps[i].key,1);
			for(int n=0;n<500 && !finished && handled<=i;++n)
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			if(handled<=i)break;
		}
		if(!finished) {
			if(!quit && handled==steps.size())push(SDLK_ESCAPE,0);
			else {SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}
		}
	});
	const bool ok=SdlWindow().showInteractive(flow.frame(),"M18B Bone Whistle acceptance",
		[&](const PlayerAction &action){auto f=advance(action);handled=index;return f;},
		[&]{return flow.canCancelInteraction();});
	finished=true;sender.join();check(ok && index==steps.size(),"SDL path failed/timed out");
}

IndexedFrame checkpoint(XeenAssetSource &assets, const std::filesystem::path &output,
	bool cancel, bool sdl, const IndexedFrame *previousBase=nullptr) {
	auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
	const auto counts=party.questItems.counts();const auto members=partySnapshot(party);
	for(std::size_t i=0;i<counts.size();++i)
		check(counts[i]==assets.readInitialResource("maze.pty").at(747+i),"fresh quest counters differ from resources");
	const auto q0=counts.at(18); // Clouds script item 100: 100 - 82.
	const XeenMapLoader mapLoader;
	int mapLoads=0,objectLoads=0,scriptLoads=0,textLoads=0;
	XeenWorld world([&](XeenMapIdentity id){++mapLoads;return mapLoader.loadGeometryMap(assets,id);},
		[&](XeenMapIdentity id){++objectLoads;return mapLoader.loadObjects(assets,id);});
	XeenEventLoader loader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
		if(!assets.hasInitialResource(name))return {};return assets.readInitialResource(name);
	});
	XeenEventTextLoader texts([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
		if(!assets.hasArchiveResource(name))return {};return assets.readArchiveResource(name);
	});
	XeenEventSystem events([&](XeenMapIdentity id){++scriptLoads;return XeenEventScript(loader.load(id));},
		[&](XeenMapIdentity id){++textLoads;return texts.load(id);});
	XeenCamera camera{20,5,14,XeenDirection::North};const auto start=camera;
	auto flags=XeenGameFlagsLoader().loadInitialCloudsFlags(assets);const auto flagsBefore=flags.values();
	const XeenObjectIdentity bones{20,1};
	check(world.selectObject(camera)==bones,"original WhoWill selected object");
	const auto geometry=geometrySnapshot(world.map(20).geometry);
	const auto objects=world.objectFile(20);
	const auto script=loader.load(20);
	check(script.records.size()==16,"original map-20 record count");
	std::vector<std::size_t> cellRecords;
	for(std::size_t i=0;i<script.records.size();++i)
		if(script.records[i].x==5 && script.records[i].y==14)cellRecords.push_back(i);
	check(cellRecords.size()==5,"original Bone Whistle cell record count");
	const std::vector<int> opcodes{0x20,0x29,9,0x0c,0x0e};
	const std::vector<std::vector<std::uint8_t>> operands{{0,3},{0},{44,1,3},{0,0,21,100},{}};
	for(std::size_t i=0;i<cellRecords.size();++i) {
		const auto &r=script.records[cellRecords[i]];
		check(r.line==i && r.opcode==opcodes[i] && r.parameters==operands[i],"original chain changed");
	}
	const XeenFontFormat font(assets.readArchiveResource("fnt"));
	const CloudsMapComposer composer;const XeenCharacterRulesContext rules{kCloudsInitialYear};
	XeenEventFlow flow(world,events,party,camera,flags,font,[&]{return composer.compose(assets,world,party,camera,rules);});
	const auto base=flow.frame();const auto resolver=XeenObjectVisualResolver::load(assets);
	auto visible=[&] {
		const auto commands=XeenOutdoorScene().build(world,camera,&resolver);
		return std::any_of(commands.begin(),commands.end(),[&](const auto &c){return c.object() && c.object()->visual.identity==bones;});
	};
	check(visible(),"initial bones draw command absent");
	check(!flow.blocksGameplay() && !flow.presentationGeneration() && !flow.canCancelInteraction(),"fresh presentation inherited");
	if(previousBase)check(base.pixels==previousBase->pixels && base.palette==previousBase->palette,"new session inherited scene");
	visual_remove_test::save(base,output/(cancel?"fresh.bmp":"before.bmp"));
	bool harvested=false,castle=false;
	std::vector<int> lines;std::optional<XeenPresentationRequest> choice,success;
	std::optional<XeenManualEventResult> terminal;
	std::size_t selected=party.party.size();
	while(selected>0 && !party.party.member(party.roster,selected-1).canAct())--selected;
	check(selected>1,"need eligible non-first member to prove selection reset");--selected;
	flow.reportText=[](const std::string &message){throw std::runtime_error(message);};
	flow.reportManual=[&](const auto &r) {
		check(!std::holds_alternative<XeenEventExecutionError>(r),"original execution error");
		if(castle)return;
		if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&r)) {
			lines.push_back(s->request.source.line);
			check(s->state.selectedObject==bones,"continuation selected wrong world identity");
			if(s->request.source.line==0) {
				choice=s->request;
				check(s->state.activeCharacterIndex==0,"fresh dispatch inherited temporary selection");
				check(s->request.kind==XeenPresentationKind::CharacterSelection && s->request.text=="Bones" &&
					s->request.verbIndex==0 && s->request.textIndex==3 && s->request.source.fileOffset==7,"original WhoWill prompt/source");
			} else {
				check(s->state.activeCharacterIndex==selected,"selected context lost across display/ack");
				if(s->request.source.line==1) {
					success=s->request;
					check(s->request.kind==XeenPresentationKind::BottomWindowMessage && s->request.textIndex==0 &&
						s->request.text==texts.load(20).strings.at(0),"original next display");
				} else check(s->request.source.line==2 && s->request.response==XeenPresentationResponseRequirement::Acknowledgment,"original acknowledgment");
			}
		} else terminal=r;
	};
	auto unchanged=[&] {
		auto expected=counts;if(harvested)++expected[18];
		check(party.questItems.counts()==expected && partySnapshot(party)==members && flags.values()==flagsBefore,"unexpected party/flag mutation");
		check(world.sessionState().disabledObjectCount()==(harvested?1:0) &&
			world.sessionState().disabledEventCount()==(harvested?cellRecords.size():0),"unexpected Remove scope");
		for(std::size_t i=0;i<objects.entities.objects.size();++i)
			check(world.isObjectDisabled({20,i})==(harvested && i==1),"unrelated object disabled");
		sameEntities(objects.entities,world.objectFile(20).entities);
		check(geometrySnapshot(world.map(20).geometry)==geometry,"geometry changed");
		for(std::size_t i=0;i<script.records.size();++i) {
			const auto &r=script.records[i];const bool disabled=harvested && r.x==5 && r.y==14;
			const auto effective=world.effectiveEvent({20,i},r);
			check(world.isEventDisabled({20,i})==disabled && sameRecord(r,effective,false) &&
				effective.opcode==(disabled?0:r.opcode),"incorrect effective event or metadata");
		}
	};
	auto atStart=[&] {check(camera.mapId==start.mapId && camera.x==start.x && camera.y==start.y && camera.direction==start.direction,"unexpected camera mutation");};
	auto pending=[&](const IndexedFrame &) {unchanged();atStart();check(flow.blocksGameplay(),"pending input escaped block");};
	auto open=[&](const IndexedFrame &frame) {
		pending(frame);check(choice && flow.canCancelInteraction() && lines==std::vector<int>{0},"original line-0 dispatch");
		visual_remove_test::save(frame,output/(cancel?"fresh-choice.bmp":"choice.bmp"));
		if(!cancel)for(int verb=0;verb<32;++verb) {
			auto request=*choice;request.verbIndex=static_cast<std::uint8_t>(verb);
			XeenEventPresenter presenter(font);auto rendered=presenter.present(base,request);
			check(!rendered.response && presenter.diagnostics().empty(),"verb rendering diagnostic");
			visual_remove_test::save(rendered.frame,output/("verb-"+std::to_string(verb)+".bmp"));
		}
		if(!cancel) {
			auto request=*choice;
			request.refusal=party.party.member(party.roster,selected).name+" is in no condition to act.";
			XeenEventPresenter presenter(font);
			visual_remove_test::save(presenter.present(base,request).frame,output/"refusal-layout.bmp");
		}
	};
	input(flow,sdl,{{SDLK_SPACE,InteractionAction{},open},
		{SDLK_UP,NavigationAction::MoveForward,pending},
		{cancel?SDLK_ESCAPE:static_cast<SDL_Keycode>(SDLK_F1+selected),
			cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{selected}},
			[&](const IndexedFrame &frame) {
				unchanged();atStart();
				if(cancel) {
					check(terminal && std::holds_alternative<XeenManualEventCompleted>(*terminal) &&
						std::get<XeenManualEventCompleted>(*terminal).instructionCount==1 && !flow.blocksGameplay() &&
						lines==std::vector<int>{0} && frame.pixels==base.pixels,"cancel ran subsequent instruction/presentation");
					visual_remove_test::save(frame,output/"cancel-result.bmp");
				} else {
					check(!terminal && flow.blocksGameplay() && !flow.canCancelInteraction() && lines==std::vector<int>({0,1,2}),"selection failed display/ack");
					visual_remove_test::save(frame,output/"next-display.bmp");
				}
			}}},!cancel); // SDL_QUIT while acknowledgment is pending must still exit.
	if(cancel) {
		input(flow,sdl,{{SDLK_RIGHT,NavigationAction::TurnRight,[&](const IndexedFrame &){unchanged();check(camera.direction==XeenDirection::East,"cancel did not release navigation");}}});
		camera=start;flow.refresh();lines.clear();terminal.reset();
		input(flow,sdl,{{SDLK_SPACE,InteractionAction{},open},
			{SDLK_ESCAPE,CancelInteractionAction{},[&](const IndexedFrame &){unchanged();check(!flow.blocksGameplay() && lines==std::vector<int>{0},"cancel retry failed");}}});
	} else {
		const auto ackGeneration=flow.presentationGeneration();check(ackGeneration.has_value(),"missing ack owner");
		input(flow,sdl,{{static_cast<SDL_Keycode>(SDLK_F1+selected),SelectMemberAction{selected},[&](const IndexedFrame &f){pending(f);check(flow.presentationGeneration()==ackGeneration,"F-key acknowledged next message");}},
			{SDLK_UP,NavigationAction::MoveForward,pending},
			{SDLK_RETURN,AcknowledgeAction{},[&](const IndexedFrame &frame) {
				check(terminal && std::holds_alternative<XeenManualEventCompleted>(*terminal) &&
					std::get<XeenManualEventCompleted>(*terminal).instructionCount==10,"harvest not completed in original ten instructions");
				harvested=true;unchanged();atStart();
				check(!flow.blocksGameplay() && !flow.presentationGeneration() && success && !visible(),"harvest left pending UI or visible bones");
				const auto effective=composer.compose(assets,world,party,camera,rules);XeenEventPresenter oracle(font);
				check(frame.pixels==oracle.present(effective,*success).frame.pixels && frame.pixels!=effective.pixels,"immediate removal/retained success text mismatch");
				check(base.pixels!=effective.pixels,"bones made no initial pixel contribution");
				visual_remove_test::save(frame,output/"result.bmp");
				std::cout<<"Harvest completed: q0="<<q0<<", item 100="<<party.questItems.at(18)<<", Clouds map 20 record 1 removed; instructions="
					<<std::get<XeenManualEventCompleted>(*terminal).instructionCount<<"; effective None records:";
				for(auto i:cellRecords)std::cout<<' '<<i;std::cout<<'\n';
			}}});
		auto repeat=[&](const IndexedFrame &) {
			unchanged();atStart();check(!flow.blocksGameplay() && !visible() && !world.selectObject(camera) &&
				lines==std::vector<int>({0,1,2}) && terminal && std::holds_alternative<XeenManualEventCompleted>(*terminal) &&
				std::get<XeenManualEventCompleted>(*terminal).instructionCount==cellRecords.size(),"removed cell replayed collection");
		};
		input(flow,sdl,{{SDLK_SPACE,InteractionAction{},repeat}});
		input(flow,sdl,{{SDLK_RIGHT,NavigationAction::TurnRight,[&](const IndexedFrame &){unchanged();check(camera.direction==XeenDirection::East,"harvest navigation did not recover");}}});
		camera=start;flow.refresh();const auto persistent=flow.frame();
		camera={1,1,14,XeenDirection::West};flow.refresh();camera=start;
		check(flow.refresh().pixels==persistent.pixels,"leave/return lost scene");
		// Each cache separately, then jointly, keeping every session owner alive.
		for(int cache=0;cache<5;++cache) {
			const auto oldMaps=mapLoads,oldObjects=objectLoads,oldScripts=scriptLoads,oldTexts=textLoads;
			const auto oldSprites=assets.spriteLoadCount();
			if(cache==0 || cache==4)world.discardMapCache();
			if(cache==1 || cache==4)events.discardScriptCache();
			if(cache==2 || cache==4)events.discardTextCache();
			if(cache==3 || cache==4)assets.discardSpriteCache();
			flow.refresh(true);input(flow,sdl,{{SDLK_SPACE,InteractionAction{},repeat}});
			if(cache==0 || cache==4)check(mapLoads>oldMaps && objectLoads>oldObjects,"map/object cache not rebuilt");
			if(cache==1 || cache==4)check(scriptLoads>oldScripts,"script cache not rebuilt");
			if(cache==3 || cache==4)check(assets.spriteLoadCount()>oldSprites,"sprite cache not rebuilt");
			if(cache==2 || cache==4) {
				// Removed events have no text. Use the existing original Castle question
				// in this same session to exercise the discarded text provider.
				castle=true;camera={1,8,8,XeenDirection::West};flow.refresh();
				input(flow,sdl,{{SDLK_SPACE,InteractionAction{},[&](const IndexedFrame &){check(flow.blocksGameplay(),"Castle text absent");}},
					{SDLK_n,NoAction{},[&](const IndexedFrame &){check(!flow.blocksGameplay(),"Castle No failed");}}});
				check(textLoads>oldTexts,"text cache not rebuilt");castle=false;camera=start;flow.refresh();
			}
			unchanged();check(flow.frame().pixels==persistent.pixels,"reconstruction restored stale scene");
		}
		camera={1,1,14,XeenDirection::West};flow.refresh();camera=start;flow.refresh();
		input(flow,sdl,{{SDLK_SPACE,InteractionAction{},repeat}});
		visual_remove_test::save(flow.frame(),output/"rebuilt.bmp");
	}
	unchanged();
	std::cout<<(cancel?"Fresh session + cancellation/retry":"Collection + repeat/reconstruction")
		<<" passed ("<<(sdl?"SDL":"direct")<<"): selected index "<<selected<<", roster "<<+party.party.activeRosterIds()[selected]
		<<"; providers maps="<<mapLoads<<" objects="<<objectLoads<<" scripts="<<scriptLoads<<" texts="<<textLoads<<'\n';
	return base;
}
}
int main(int argc,char **argv) {
	try {
		check(argc==3 || (argc==4 && std::string(argv[3])=="sdl"),"usage: mmodern_who_will_smoke <game-directory> <output-directory> [sdl]");
		std::filesystem::create_directories(argv[2]);
		const auto installation=XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(),"Clouds installation unavailable");
		XeenAssetSource assets(*installation,320,200);
		std::vector<std::vector<std::uint8_t>> original;
		const std::vector<std::string> resources{"maze.pty","maze.chr","maze0020.evt","maze0020.mob"};
		for(const auto &name:resources)original.push_back(assets.readInitialResource(name));
		const auto base=checkpoint(assets,argv[2],false,argc==4);
		// Constructs new party/world/EventSystem/presentation owners after collection.
		checkpoint(assets,argv[2],true,argc==4,&base);
		for(std::size_t i=0;i<resources.size();++i)check(original[i]==assets.readInitialResource(resources[i]),"original bytes changed");
		std::cout<<"M18B original acceptance OK\n";return 0;
	}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
