#include "XeenVisualRemoveTestSupport.h"
#include "XeenPartySnapshotTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <iostream>
#include <set>

using namespace mmodern;
using namespace remove_test;
namespace {
void checkpoint(XeenAssetSource &assets, const std::filesystem::path &output, int roots, bool sdl) {
	auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
	check(party.questItems.at(17)==0,"fresh original party inherited a Root");
	for(int i=0;i<roots;++i)check(party.questItems.increment(17),"controlled Root setup");
	const auto counts=party.questItems.counts();const auto members=partySnapshot(party);
	auto flags=XeenGameFlagsLoader().loadInitialCloudsFlags(assets);const auto flagsBefore=flags.values();
	XeenCamera camera{23,9,11,XeenDirection::West};const auto start=camera;
	const XeenMapLoader mapLoader;int maps=0,objects=0,scripts=0,strings=0;
	XeenWorld world([&](XeenMapIdentity id){++maps;return mapLoader.loadGeometryMap(assets,id);},
		[&](XeenMapIdentity id){++objects;return mapLoader.loadObjects(assets,id);});
	XeenEventLoader loader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>>{
		if(!assets.hasInitialResource(name))return {};return assets.readInitialResource(name);});
	XeenEventTextLoader texts([&](const std::string &name)->std::optional<std::vector<std::uint8_t>>{
		if(!assets.hasArchiveResource(name))return {};return assets.readArchiveResource(name);});
	auto scriptProvider=[&](XeenMapIdentity id){++scripts;return XeenEventScript(loader.load(id));};
	auto textProvider=[&](XeenMapIdentity id){++strings;return texts.load(id);};
	XeenEventSystem events(scriptProvider,textProvider);
	const auto original=loader.load(23);const auto text=texts.load(23);
	check(original.records.size()==170,"original map-23 record count");
	const std::vector<int> offsets{182,191,200,211,217,228,238,244,255,265,275,285,295,305,315};
	const std::vector<int> opcodes{9,8,5,0x12,5,0x0c,0x12,5,0x0c,0x0c,0x2c,0x2c,0x2c,0x2c,0x2c};
	const std::vector<std::vector<std::uint8_t>> operands{{21,99,7},{9,0,4},{1,2,17,1,3},{},
		{1,0,17,1,5},{0,0,104,2},{},{1,3,17,1,8},{21,99,0,0},{104,2,0,0},
		{70,37,0,1},{70,37,0,1},{70,37,0,1},{70,37,0,1},{70,37,0,1}};
	for(std::size_t i=0;i<offsets.size();++i){const auto&r=original.records.at(21+i);
		check(r.x==9 && r.y==11 && r.line==i && r.fileOffset==offsets[i] && r.opcode==opcodes[i] &&
			r.parameters==operands[i],"original Myra records changed");}
	check(text.strings.at(0).size()==259 && text.strings.at(1).size()==22 && text.strings.at(3).size()==58,"original text characteristics");
	const auto geometry=geometrySnapshot(world.map(23).geometry);const auto objectFile=world.objectFile(23);
	const XeenFontFormat font(assets.readArchiveResource("fnt"));const CloudsMapComposer composer;
	const XeenCharacterRulesContext rules{kCloudsInitialYear};std::uint64_t time=0;unsigned random=0;
	auto compose=[&]{return composer.compose(assets,world,party,camera,rules);};
	auto draw=[&](IndexedFrame &f,std::uint8_t portrait,std::size_t index){assets.drawNpc(f,portrait,index);};
	XeenEventPresenter::Clock clock;
	if(!sdl)clock=[&]{return time;};
	XeenEventFlow flow(world,events,party,camera,flags,font,compose,draw,clock,[&]{return random++%4;});
	const auto base=flow.frame();const std::string name=roots?"root-"+std::to_string(roots):"request";
	auto save=[&](const IndexedFrame&f,const std::string&suffix){visual_remove_test::save(f,output/(name+"-"+suffix+".bmp"));};
	save(base,"before");
	std::optional<XeenEventExecutionSuspended> pending;
	std::optional<XeenEventExecutionError> terminal;
	int presentations=0;
	flow.reportText=[](const std::string &s){throw std::runtime_error(s);};
	flow.reportManual=[&](const auto&r){
		if(const auto*p=std::get_if<XeenEventExecutionSuspended>(&r)){
			pending=*p;++presentations;const auto&q=p->request;
			check(q.kind==XeenPresentationKind::NpcAcknowledgment && q.source.line==(roots?7:4) &&
				q.source.fileOffset==(roots?244:217) && p->state.instructionCount==(roots?2:3) &&
				q.title==text.strings.at(1) && q.text==text.strings.at(roots?3:0) && q.npc && q.npc->portraitId==17,
				"ordinary line-0 NPC frontier");
		}else if(const auto*e=std::get_if<XeenEventExecutionError>(&r))terminal=*e;
		else throw std::runtime_error("M19A unexpectedly completed Myra");};
	auto unchanged=[&]{
		check(party.questItems.counts()==counts && partySnapshot(party)==members && flags.values()==flagsBefore,"party/game flag mutation");
		check(camera.mapId==start.mapId && camera.x==start.x && camera.y==start.y && camera.direction==start.direction,"camera mutation");
		check(world.sessionState().disabledObjectCount()==0 && world.sessionState().disabledEventCount()==0,"world mutation");
		check(geometrySnapshot(world.map(23).geometry)==geometry,"geometry mutation");sameEntities(objectFile.entities,world.objectFile(23).entities);
		for(std::size_t i=0;i<original.records.size();++i)check(sameRecord(original.records[i],world.effectiveEvent({23,i},original.records[i])),"effective event mutation");};
	auto frontier=[&]{
		check(terminal && terminal->kind==XeenEventExecutionErrorKind::UnsupportedOperationMode && terminal->source &&
			terminal->source->line==(roots?8:5) && terminal->source->fileOffset==(roots?255:228) &&
			terminal->instructionCount==(roots?3:4) && !flow.blocksGameplay() && !flow.presentationGeneration(),"wrong M19A unsupported boundary");
		check(flow.frame().pixels==base.pixels,"NPC layer survived final acknowledgment");unchanged();save(flow.frame(),"dismissed");};
	auto rebuild=[&]{
		const auto gen=flow.presentationGeneration();const auto timing=flow.presenter().npcTiming();const auto pixels=flow.frame().pixels;
		const auto oldMaps=maps,oldObjects=objects;const auto oldSprites=assets.spriteLoadCount();
		world.discardMapCache();events.discardScriptCache();events.discardTextCache();assets.discardSpriteCache();flow.refresh(true);
		check(maps>oldMaps && objects>oldObjects && assets.spriteLoadCount()>oldSprites,"pending providers did not reload");
		const auto &after=flow.presenter().npcTiming();
		check(flow.presentationGeneration()==gen && flow.frame().pixels==pixels && after.displayedFrame==timing.displayedFrame &&
			after.nextFrame==timing.nextFrame && after.phase==timing.phase && after.remaining==timing.remaining && after.deadline==timing.deadline,
			"pending reconstruction reset presentation");unchanged();save(flow.frame(),"rebuilt");};
	if(sdl){
		int stage=0;std::uint32_t started=0;bool changed=false;std::size_t page=0;
		auto push=[](SDL_Keycode key,int repeat=0){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;e.key.repeat=repeat;SDL_PushEvent(&e);};
		auto quit=[] {SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);};
		const bool ok=SdlWindow().showInteractive(base,"M19A Myra acceptance",[&](const PlayerAction&a)->std::optional<IndexedFrame>{
			auto f=flow.handle(a);unchanged();
			if(stage==1 && pending){save(f,"page-0");stage=2;}
			if(terminal){frontier();stage=4;quit();push(SDLK_RETURN);}
			return f;},[&]{return flow.handlesEscape();},[&]()->std::optional<IndexedFrame>{
			if(!started)started=SDL_GetTicks();
			if(SDL_GetTicks()-started>5000){quit();return {};}
			if(stage==0){stage=1;push(SDLK_SPACE);push(SDLK_SPACE,1);}
			auto f=flow.updatePresentation();unchanged();
			if(stage==2 && f){changed=true;save(*f,"idle-"+std::to_string(SDL_GetTicks()-started)+"ms");rebuild();
				push(SDLK_UP);push(SDLK_y);push(SDLK_n);push(SDLK_F1);stage=3;}
			else if(stage==3){save(flow.frame(),"page-"+std::to_string(page++));push(SDLK_ESCAPE);push(SDLK_ESCAPE,1);}
			return f;});
		check(ok && stage==4 && changed && presentations==1,"SDL original idle/input/repeat frontier");
		flow.abandonPresentation();
	}else{
		flow.handle(InteractionAction{});check(flow.blocksGameplay() && pending,"original NPC absent");
		const auto gen=flow.presentationGeneration();const auto pages=flow.presenter().pageCount();
		std::cout<<name<<": pages="<<pages<<", initial speech counter="<<flow.presenter().npcTiming().remaining<<'\n';
		std::set<unsigned> shown;save(flow.frame(),"page-0");
		for(unsigned tick=0;tick<1000;++tick){
			const auto&t=flow.presenter().npcTiming();
			if(shown.insert(t.displayedFrame).second)save(flow.frame(),"portrait-"+std::to_string(t.displayedFrame));
			if(tick<8)save(flow.frame(),"time-"+std::to_string(time)+"ms");
			if(!t.remaining && !t.nextFrame && !t.displayedFrame)break;
			const auto before=flow.frame().pixels;
			time+=150;flow.updatePresentation();unchanged();check(flow.presentationGeneration()==gen,"tick consumed generation");
			for(int y=0;y<200;++y)for(int x=0;x<320;++x)
				if(before[y*320+x]!=flow.frame().pixels[y*320+x])
					check(x>=23 && x<55 && y>=22 && y<54,"portrait tick changed text/scene/HUD");
		}
		check(shown.size()==4 && flow.presenter().npcTiming().remaining==0 && flow.presenter().npcTiming().displayedFrame==0,"four original frames/rest");
		std::cout<<name<<": resting frame 0 at injected "<<time<<"ms\n";
		save(flow.frame(),"rest");rebuild();
		while(flow.blocksGameplay()){
			std::cout<<name<<": page "<<flow.presenter().pageIndex()<<", speech counter "<<flow.presenter().npcTiming().remaining<<'\n';
			save(flow.frame(),"page-"+std::to_string(flow.presenter().pageIndex()));
			flow.handle(AcknowledgeAction{});unchanged();
		}
		frontier();check(!flow.respond(*gen,XeenPresentationResponse::Acknowledged),"old response replayed");
		// A second independent interaction must load discarded scripts/text, with fresh timing.
		const auto oldScripts=scripts,oldStrings=strings;terminal.reset();flow.handle(InteractionAction{});
		check(scripts>oldScripts && strings>oldStrings && flow.presenter().npcTiming().displayedFrame==0 &&
			flow.presenter().npcTiming().phase==0 && flow.presentationGeneration()!=gen,"independent dispatch/cache reload");
		while(flow.blocksGameplay())flow.handle(CancelInteractionAction{});frontier();
		terminal.reset();flow.handle(InteractionAction{});flow.abandonPresentation();
		check(!terminal && !flow.blocksGameplay() && flow.frame().pixels==base.pixels,"abandonment acknowledged");unchanged();
		// Actual new EventSystem/Flow owners, retaining authoritative party/world owners.
		XeenEventSystem newEvents(scriptProvider,textProvider);
		XeenEventFlow fresh(world,newEvents,party,camera,flags,font,compose,draw,[&]{return time;},[]{return 1;});
		check(!fresh.blocksGameplay() && !fresh.presentationGeneration() && !fresh.updatePresentation(),"new flow inherited pending work");
		camera={1,1,14,XeenDirection::West};fresh.refresh();camera=start;fresh.refresh();
		fresh.handle(InteractionAction{});check(fresh.blocksGameplay() && fresh.presenter().npcTiming().displayedFrame==0,"new owner dispatch");
		fresh.abandonPresentation();unchanged();
	}
	std::cout<<name<<": "<<(roots?"0 -> 7 -> 8 / offset 255 / 3 instructions":"0 -> 1 -> 4 -> 5 / offset 228 / 4 instructions")
		<<"; unchanged state; "<<(sdl?"SDL":"direct")<<" OK; maps="<<maps<<" objects="<<objects<<" scripts="<<scripts<<" texts="<<strings<<'\n';
}
}
int main(int argc,char **argv){try{
	check(argc==3 || (argc==4 && std::string(argv[3])=="sdl"),"usage: mmodern_myra_smoke <game-directory> <output-directory> [sdl]");
	std::filesystem::create_directories(argv[2]);const auto installation=XeenInstallationDetector().detect(argv[1]);
	check(installation && installation->hasXeen(),"Clouds installation unavailable");XeenAssetSource assets(*installation,320,200);
	const std::vector<std::string> names{"maze.pty","maze.chr","maze0023.evt","maze0023.mob"};
	std::vector<std::vector<std::uint8_t>> bytes;for(const auto&name:names)bytes.push_back(assets.readInitialResource(name));
	for(int roots:{0,1,3})checkpoint(assets,argv[2],roots,argc==4);
	for(std::size_t i=0;i<names.size();++i)check(bytes[i]==assets.readInitialResource(names[i]),"original resource bytes changed");
	std::cout<<"M19A original Myra acceptance OK (quest-state modeling intentionally absent)\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
