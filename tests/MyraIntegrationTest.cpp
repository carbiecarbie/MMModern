#include "XeenCheckpointTestSupport.h"
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
void checkpoint(XeenAssetSource &assets, const std::filesystem::path &output, int roots,
		bool requested, SDL_Keycode dismiss, bool sdl, int lossFixture=0) {
	auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
	check(party.questItems.at(17)==0,"fresh original party inherited a Root");
	const auto originalParty=assets.readInitialResource("maze.pty");
	for(int i=0;i<30;++i)check(party.questFlags.isSet(i)==bool(originalParty.at(739+i/8)&(1u<<(i%8))),"original quest-flag loading");
	check(!party.questFlags.isSet(2),"fresh session inherited request");
	for(int i=0;i<roots;++i)check(party.questItems.increment(17),"controlled Root setup");
	if(requested)party.questFlags.set(2); // Controlled initial state, never the tested result.
	auto expectedQuestFlags=party.questFlags.values();
	auto counts=party.questItems.counts();auto expectedParty=party;
	bool returning=counts[17]>0;unsigned delivered=0;
	for(auto id:party.party.activeRosterIds()) {
		check(party.roster.at(id).canAct(),"original delivery fixture eligibility");
		for(const auto &item:party.roster.at(id).miscellaneous)check(item.id==0,"original delivery fixture capacity");
	}
	// Focused original-script loss fixtures, outside the full initial-state matrix.
	if(lossFixture) {
		for(auto id:party.party.activeRosterIds()) {
			auto &c=party.roster.at(id);
			if(lossFixture==1)c.weapons.back()=c.armor.back()=c.accessories.back()=c.miscellaneous.back()={255,254,253,252};
			else c.conditions[static_cast<std::size_t>(XeenCondition::Dead)]=1;
		}
		expectedParty=party;
	}
	auto expectDelivery=[&]{
		--counts[17];expectedQuestFlags[2]=false;
		if(lossFixture)return;
		for(unsigned i=0;i<5;++i,++delivered) {
			const auto owner=expectedParty.party.activeRosterIds().at(delivered/9);
			expectedParty.roster.at(owner).miscellaneous[delivered%9]={10,37,1,0};
		}
	};
	auto flags=XeenGameFlagsLoader().loadInitialCloudsFlags(assets);const auto flagsBefore=flags.values();
	XeenCamera camera=checkpoint_test::myra;const auto start=camera;
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
	check(!XeenEventScript(original).findInstructionIndex(9,11,XeenDirection::West,15),"original sequential successor must be absent");
	check(text.strings.at(0).size()==259 && text.strings.at(1).size()==22 && text.strings.at(3).size()==58,"original text characteristics");
	const auto geometry=geometrySnapshot(world.map(23).geometry);const auto objectFile=world.objectFile(23);
	const XeenFontFormat font(assets.readArchiveResource("fnt"));const CloudsMapComposer composer;
	const XeenCharacterRulesContext rules{kCloudsInitialYear};std::uint64_t time=0;unsigned random=0;
	auto compose=[&]{return composer.compose(assets,world,party,camera,rules);};
	bool failDraw=false;
	auto draw=[&](IndexedFrame &f,std::uint8_t portrait,std::size_t index){
		if(failDraw)throw std::runtime_error("injected NPC asset failure");assets.drawNpc(f,portrait,index);};
	XeenEventPresenter::Clock clock;
	if(!sdl)clock=[&]{return time;};
	XeenEventFlow flow(world,events,party,camera,flags,font,compose,draw,clock,[&]{return random++%4;});
	const auto base=flow.frame();const std::string name=(roots?"root-"+std::to_string(roots):"request")+
		std::string(requested?"-q1":"-q0")+(lossFixture==1?"-full":lossFixture==2?"-ineligible":"")+(dismiss==SDLK_SPACE?"-space":dismiss==SDLK_RETURN?"-enter":"-escape");
	bool capture=true;
	auto save=[&](const IndexedFrame&f,const std::string&suffix){
		if(capture && (!requested || lossFixture) && dismiss==SDLK_RETURN && roots<2)visual_remove_test::save(f,output/(name+"-"+suffix+".bmp"));};
	save(base,"before");
	std::optional<XeenEventExecutionSuspended> pending;
	std::optional<XeenEventExecutionError> terminal;
	bool completed=false;
	int presentations=0,npcs=0,receipts=0,warnings=0;
	flow.reportText=[](const std::string &s){throw std::runtime_error(s);};
	auto report=[&](const XeenManualEventResult&r){
		if(const auto*p=std::get_if<XeenEventExecutionSuspended>(&r)){
			pending=*p;++presentations;const auto&q=p->request;
			if(q.kind==XeenPresentationKind::NpcAcknowledgment) {
				++npcs;
				check(q.source.line==(returning?7:4) && q.source.fileOffset==(returning?244:217) &&
					p->state.instructionCount==(returning?2:3) && q.title==text.strings.at(1) &&
					q.text==text.strings.at(returning?3:0) && q.npc && q.npc->portraitId==17,"ordinary line-0 NPC frontier");
			} else if(q.kind==XeenPresentationKind::RewardReceipt) {
				++receipts;check(returning && receipts==1 && p->state.instructionCount==9 &&
					p->state.rewardReceipt.delivered==(lossFixture?0U:5U) && p->state.rewardReceipt.lost==(lossFixture?5U:0U) &&
					p->state.rewardReceipt.overflow==0 && !p->state.pendingRewards.hasWork(),"original return receipt/count");
				if(!warnings)expectDelivery();
			} else if(q.kind==XeenPresentationKind::RewardWarning) {
				++warnings;check(returning && lossFixture==1 && warnings==1 &&
					p->state.instructionCount==9 && p->state.pendingRewards.size()==5 &&
					!p->state.rewardReceipt.delivered,"original capacity warning phase");
				expectDelivery(); // Quest mutations precede warning; prepared items remain unchanged.
			} else throw std::runtime_error("unexpected Myra presentation kind");
		}else if(const auto*e=std::get_if<XeenEventExecutionError>(&r))terminal=*e;
		else if(const auto*c=std::get_if<XeenManualEventCompleted>(&r)){
			check(c->instructionCount==(returning?9U:5U) && !c->cameraChanged && !c->flagsChanged,"Myra completion count/publication");
			completed=true;if(!returning)expectedQuestFlags[2]=true;
		}else throw std::runtime_error("unexpected Myra dispatch result");};
	flow.reportManual=report;
	auto unchanged=[&]{
		checkPartyQuestState(party,counts,expectedQuestFlags);
		check(partySnapshot(party)==partySnapshot(expectedParty) && flags.values()==flagsBefore,"party/game flag mutation");
		check(camera.mapId==start.mapId && camera.x==start.x && camera.y==start.y && camera.direction==start.direction,"camera mutation");
		check(world.sessionState().disabledObjectCount()==0 && world.sessionState().disabledEventCount()==0,"world mutation");
		check(geometrySnapshot(world.map(23).geometry)==geometry,"geometry mutation");sameEntities(objectFile.entities,world.objectFile(23).entities);
		for(std::size_t i=0;i<original.records.size();++i)check(sameRecord(original.records[i],world.effectiveEvent({23,i},original.records[i])),"effective event mutation");};
	auto frontier=[&]{
		check(completed && !terminal && npcs==1 && receipts==(returning?1:0) && warnings==(returning && lossFixture==1?1:0) &&
			party.questFlags.isSet(2)==!returning,"original Myra phase/completion result");
		check(!flow.blocksGameplay() && !flow.presentationGeneration(),"terminal Myra remained pending");
		check(flow.frame().pixels==base.pixels,"NPC layer survived final acknowledgment");unchanged();save(flow.frame(),"dismissed");};
	auto rebuild=[&]{
		const auto gen=flow.presentationGeneration();const auto timing=flow.presenter().npcTiming();const auto pixels=flow.frame().pixels;
		const auto oldMaps=maps,oldObjects=objects;const auto oldSprites=assets.spriteLoadCount();
		world.discardMapCache();events.discardScriptCache();events.discardTextCache();assets.discardSpriteCache();flow.refresh(true);
		check(maps>oldMaps && objects>oldObjects && assets.spriteLoadCount()>oldSprites,"pending providers did not reload");
		const auto &after=flow.presenter().npcTiming();
		check(flow.presentationGeneration()==gen && flow.frame().pixels==pixels && after.displayedFrame==timing.displayedFrame &&
			after.nextFrame==timing.nextFrame && after.phase==timing.phase && after.remaining==timing.remaining && after.deadline==timing.deadline,
			"pending reconstruction reset presentation");unchanged();save(flow.frame(),gen?"rebuilt":"completed-rebuilt");};
	auto resetReport=[&]{terminal.reset();pending.reset();completed=false;presentations=0;npcs=receipts=warnings=0;returning=counts[17]>0;};
	// Failure and abandonment must be checked BEFORE the first successful write.
	failDraw=true;flow.handle(InteractionAction{});
	check(terminal && terminal->kind==XeenEventExecutionErrorKind::PresentationFailed && terminal->source &&
		terminal->source->line==(returning?7:4) && !completed && !flow.blocksGameplay() && flow.frame().pixels==base.pixels,
		"original NPC failure did not clean up");unchanged();failDraw=false;resetReport();
	if(sdl){
		int actions=0;bool queued=false;
		const bool ok=SdlWindow().showInteractive(base,"Myra pending quit",[&](const PlayerAction&a)->std::optional<IndexedFrame>{
			++actions;auto f=flow.handle(a);check(flow.blocksGameplay(),"quit fixture did not suspend");
			SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);q={};q.type=SDL_KEYDOWN;q.key.keysym.sym=SDLK_RETURN;SDL_PushEvent(&q);return f;
		},[&]{return flow.handlesEscape();},[&]()->std::optional<IndexedFrame>{
			if(!queued){queued=true;SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_SPACE;SDL_PushEvent(&e);}return flow.updatePresentation();});
		check(ok && actions==1 && !completed && !terminal && flow.blocksGameplay(),"SDL_QUIT acknowledged original NPC");
	}else flow.handle(InteractionAction{});
	flow.abandonPresentation();check(!completed && !terminal && !flow.blocksGameplay() && flow.frame().pixels==base.pixels,"abandonment acknowledged");
	unchanged();resetReport();
	const PlayerAction acknowledgment=dismiss==SDLK_SPACE?PlayerAction(InteractionAction{}):
		dismiss==SDLK_RETURN?PlayerAction(AcknowledgeAction{}):PlayerAction(CancelInteractionAction{});
	auto handle=[&](const PlayerAction &action){
		const bool intermediate=flow.blocksGameplay() && flow.presenter().pageIndex()+1<flow.presenter().pageCount();
		const auto beforeFlags=party.questFlags.values();const auto beforeCounts=party.questItems.counts();
		const auto beforeMembers=partySnapshot(party);const auto generation=flow.presentationGeneration();
		const auto page=flow.presenter().pageIndex();auto frame=flow.handle(action);
		if(intermediate && (std::holds_alternative<InteractionAction>(action) ||
			std::holds_alternative<AcknowledgeAction>(action) || std::holds_alternative<CancelInteractionAction>(action)))
			check(flow.blocksGameplay() && flow.presentationGeneration()==generation && flow.presenter().pageIndex()==page+1 &&
				!completed && !terminal && party.questFlags.values()==beforeFlags && party.questItems.counts()==beforeCounts && partySnapshot(party)==beforeMembers,"intermediate page acknowledged/wrote state");
		return frame;
	};
	std::optional<std::uint64_t> previousGeneration;
	auto freshPresentation=[&]{
		const auto generation=flow.presentationGeneration();
		check(generation && generation!=previousGeneration && flow.presenter().npcTiming().displayedFrame==0 &&
			flow.presenter().npcTiming().phase==0,"independent dispatch inherited generation/timing");
		previousGeneration=generation;
	};
	auto interact=[&](bool animate){
	capture=animate;
	resetReport();
	if(sdl){
		int stage=0;std::uint32_t started=0;bool changed=false;std::size_t page=0;
		auto push=[](SDL_Keycode key,int repeat=0){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;e.key.repeat=repeat;SDL_PushEvent(&e);};
		auto quit=[] {SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);};
		const bool ok=SdlWindow().showInteractive(base,"21C Myra acceptance",[&](const PlayerAction&a)->std::optional<IndexedFrame>{
			auto f=handle(a);unchanged();
			if(stage==1 && pending){freshPresentation();save(f,"page-0");stage=2;}
			if(terminal || completed){frontier();stage=4;quit();push(SDLK_RETURN);}
			return f;},[&]{return flow.handlesEscape();},[&]()->std::optional<IndexedFrame>{
			if(!started)started=SDL_GetTicks();
			if(SDL_GetTicks()-started>5000){quit();return {};}
			if(stage==0){stage=1;push(SDLK_SPACE);push(SDLK_SPACE,1);}
			auto f=flow.updatePresentation();unchanged();
			if(stage==2 && f){changed=true;save(*f,"idle-"+std::to_string(SDL_GetTicks()-started)+"ms");rebuild();
				push(SDLK_UP);push(SDLK_y);push(SDLK_n);push(SDLK_F1);stage=3;}
			else if(stage==3){save(flow.frame(),std::string(pending->request.kind==XeenPresentationKind::RewardReceipt?"receipt-":pending->request.kind==XeenPresentationKind::RewardWarning?"warning-":"page-")+std::to_string(page++));push(dismiss);push(dismiss,1);}
			return f;});
		check(ok && stage==4 && changed && presentations==(returning?(lossFixture==1?3:2):1),"SDL original idle/input/repeat phases");
		flow.abandonPresentation();
	}else{
		handle(InteractionAction{});check(flow.blocksGameplay() && pending,"original NPC absent");
		freshPresentation();
		const auto gen=flow.presentationGeneration();const auto pages=flow.presenter().pageCount();
		std::cout<<name<<": pages="<<pages<<", initial speech counter="<<flow.presenter().npcTiming().remaining<<'\n';
		std::set<unsigned> shown;save(flow.frame(),"page-0");
		for(unsigned tick=0;animate && tick<1000;++tick){
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
		if(animate)check(shown.size()==4 && flow.presenter().npcTiming().remaining==0 && flow.presenter().npcTiming().displayedFrame==0,"four original frames/rest");
		if(animate)std::cout<<name<<": resting frame 0 at injected "<<time<<"ms\n";
		save(flow.frame(),"rest");rebuild();
		unsigned acknowledgments=0;
		while(flow.blocksGameplay()){
			check(++acknowledgments<100,"Myra phase acknowledgment bound");
			std::cout<<name<<": page "<<flow.presenter().pageIndex()<<", speech counter "<<flow.presenter().npcTiming().remaining<<'\n';
			save(flow.frame(),std::string(pending->request.kind==XeenPresentationKind::RewardReceipt?"receipt-":pending->request.kind==XeenPresentationKind::RewardWarning?"warning-":"page-")+std::to_string(flow.presenter().pageIndex()));
			handle(acknowledgment);unchanged();
		}
		frontier();check(!flow.respond(*gen,XeenPresentationResponse::Acknowledged),"old response replayed");
	}
	};
	interact(true);
	// Completed-state reconstruction followed by the original revisit, also via SDL.
	rebuild();const auto oldScripts=scripts,oldStrings=strings;interact(false);
	check(scripts>oldScripts && strings>oldStrings,"independent dispatch did not reload scripts/text");frontier();
	while(counts[17]){interact(false);rebuild();}
	interact(false); // Exhausted Roots request again; accumulated items remain.
	resetReport();flow.handle(InteractionAction{});flow.abandonPresentation();
	check(!terminal && !completed && !flow.blocksGameplay() && flow.frame().pixels==base.pixels,"later abandonment acknowledged");unchanged();
	// Actual new EventSystem/Flow owners, retaining authoritative party/world owners.
	XeenEventSystem newEvents(scriptProvider,textProvider);
	XeenEventFlow fresh(world,newEvents,party,camera,flags,font,compose,draw,[&]{return time;},[]{return 1;});
	fresh.reportManual=report;fresh.reportText=flow.reportText;
	check(!fresh.blocksGameplay() && !fresh.presentationGeneration() && !fresh.updatePresentation(),"new flow inherited pending work");
	camera={1,1,14,XeenDirection::West};fresh.refresh();camera=start;fresh.refresh();
	resetReport();fresh.handle(InteractionAction{});check(fresh.blocksGameplay() && fresh.presenter().npcTiming().displayedFrame==0,"new owner dispatch");
	unsigned freshAcks=0;while(fresh.blocksGameplay()){check(++freshAcks<100,"new owner acknowledgment bound");fresh.handle(acknowledgment);}
	check(completed && !terminal && fresh.frame().pixels==base.pixels && !fresh.presentationGeneration(),"new owner did not finish original path");frontier();unchanged();
	std::cout<<name<<": "<<(roots?"nine-instruction returns exhausted Roots; five records per return":"five-instruction request; Q2=true")
		<<"; revisit, abandon/failure, reconstruction/fresh owners; "<<(sdl?"SDL":"direct")<<" OK; maps="<<maps<<" objects="<<objects<<" scripts="<<scripts<<" texts="<<strings<<'\n';
}
}
int main(int argc,char **argv){try{
	check(argc==3 || (argc==4 && std::string(argv[3])=="sdl"),"usage: mmodern_myra_smoke <game-directory> <output-directory> [sdl]");
	std::filesystem::create_directories(argv[2]);const auto installation=XeenInstallationDetector().detect(argv[1]);
	check(installation && installation->hasXeen(),"Clouds installation unavailable");XeenAssetSource assets(*installation,320,200);
	const std::vector<std::string> names{"maze.pty","maze.chr","maze0023.evt","maze0023.mob"};
	std::vector<std::vector<std::uint8_t>> bytes;for(const auto&name:names)bytes.push_back(assets.readInitialResource(name));
	for(int roots:{0,1,3})for(bool requested:{false,true})for(auto key:{SDLK_SPACE,SDLK_RETURN,SDLK_ESCAPE})
		checkpoint(assets,argv[2],roots,requested,key,argc==4);
	for(int loss:{1,2})checkpoint(assets,argv[2],1,true,SDLK_RETURN,argc==4,loss);
	for(std::size_t i=0;i<names.size();++i)check(bytes[i]==assets.readInitialResource(names[i]),"original resource bytes changed");
	std::cout<<"21C original Myra request/return matrix OK (18 matrix cases plus two loss fixtures and revisits per mode)\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
