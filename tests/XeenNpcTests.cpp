#include "XeenRemoveTestSupport.h"
#include "SyntheticXeenArchive.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include "games/xeen/XeenPartyLoader.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <iostream>

using namespace mmodern;
using namespace remove_test;
namespace {
using Bytes = std::vector<std::uint8_t>;
using Kind = XeenEventExecutionErrorKind;
Bytes fontBytes() {
	Bytes b(XeenFontFormat::kMinimumSize);
	for (int c=0;c<128;++c) {
		b[0x1000+c]=6;b[0x1080+c]=3;
		for(int y=0;y<8;++y)b[c*16+y*2]=0x55;
	}
	return b;
}
XeenPartyState party() {
	Bytes r(XeenRoster::kCharacterCount*XeenCharacter::kSerializedSize),p(XeenQuestItemFormat::kRequiredSize);
	std::fill(p.begin()+2,p.begin()+10,255);p[0]=p[1]=2;p[2]=4;p[3]=8;
	return XeenPartyLoader().loadFromResources(r,p);
}
XeenEventRecord npc(int line=0,int x=1,int y=1) { return record(x,y,line,5,{0,1,9,1,99}); }
struct Fixture {
	XeenPartyState members=party();
	XeenCamera camera{1,1,1,XeenDirection::North};
	XeenGameFlags flags;
	XeenFontFormat font{fontBytes()};
	IndexedFrame base;
	std::uint64_t time=0;unsigned random=2;int draws=0,loads=0,composes=0;
	bool failDraw=false;
	XeenEventTextFile text{1,"synthetic.txt",true,{"Title\n\t125Subtitle","one two three four"}};
	std::map<XeenMapIdentity,XeenEventScript> scripts;
	XeenWorld world{[](XeenMapIdentity id){return map(id);},[](XeenMapIdentity id){
		XeenObjectFile f{id,"synthetic.mob",true,{}};f.entities.objects={{1,1,0,0,111}};return f;}};
	XeenEventSystem events{[&](XeenMapIdentity id){return scripts.at(id);},
		[&](XeenMapIdentity){++loads;return text;}};
	XeenEventInterpreter interpreter;
	Fixture(){base.width=320;base.height=200;base.pixels.assign(64000,90);set({npc(),record(1,1,1,0x12)});}
	void set(std::vector<XeenEventRecord> rs){scripts.insert_or_assign(1,script(1,std::move(rs)));events.discardScriptCache();}
	XeenEventPresenter::NpcDraw draw(){return [&](IndexedFrame &f,std::uint8_t id,std::size_t frame){
		check(id==9,"portrait operand lost");++draws;if(failDraw)throw std::runtime_error("synthetic FAC failure");
		f.pixels[23+23*320]=static_cast<std::uint8_t>(40+frame);
		if(frame==2)f.pixels[24+23*320]=55;
	};}
	XeenEventFlow flow(){return XeenEventFlow(world,events,members,camera,flags,font,
		[&]{++composes;return base;},draw(),[&]{return time;},[&]{return random;});}
	XeenEventExecutionStepResult begin(int line=0){return interpreter.begin(camera,members,flags,world,
		[&](XeenMapIdentity id){return scripts.at(id);},[&](XeenMapIdentity){return text;},line);}
	XeenEventExecutionStepResult resume(XeenEventExecutionState state,XeenPresentationResponse response=XeenPresentationResponse::Acknowledged){
		return interpreter.resume(std::move(state),response,members,world,[&](XeenMapIdentity id){return scripts.at(id);},
			[&](XeenMapIdentity){return text;});}
};
XeenEventExecutionState pending(const XeenEventExecutionStepResult &r) {
	check(std::holds_alternative<XeenEventExecutionSuspended>(r),"expected NPC suspension");
	return std::get<XeenEventExecutionSuspended>(r).state;
}
void error(const XeenEventExecutionStepResult &r,Kind kind) {
	const auto *e=std::get_if<XeenEventExecutionError>(&r);check(e && e->kind==kind,"unexpected NPC error");
}
void decoderAndExecution() {
	for(int size=0;size<=7;++size) {
		auto r=npc();r.parameters.resize(size);
		auto d=XeenEventDecoder::decode(r,{1,"test.evt",7});
		if(size==5) {
			const auto &i=std::get<XeenDecodedEventInstruction>(d);const auto &n=std::get<XeenEventNpc>(i.operation);
			check(n.titleTextIndex==0 && n.bodyTextIndex==1 && n.portraitId==9 && n.confirmationMode==1 && n.targetLine==99,"NPC operands");
			check(i.source.recordIndex==7 && i.source.fileOffset==100 && i.source.opcode==5,"NPC provenance");
		} else check(std::get<XeenEventDecodeError>(d).kind==XeenEventDecodeErrorKind::MalformedInstruction,"NPC strict size");
	}
	for(int mode:{0,2,3,255}) {Fixture f;auto r=npc();r.parameters[3]=mode;f.set({r});error(f.begin(),Kind::UnsupportedOperand);}
	for(int missing:{0,1}) {Fixture f;auto r=npc();r.parameters[missing]=250;f.set({r});error(f.begin(),Kind::InvalidTextIndex);}
	for(int fault=0;fault<3;++fault) {
		Fixture f;auto flow=f.flow();std::optional<XeenEventExecutionError> e;
		flow.reportManual=[&](const auto&r){if(auto p=std::get_if<XeenEventExecutionError>(&r))e=*p;};
		if(fault==0)f.text.resourcePresent=false;else if(fault==1)f.text.mapId=2;else f.text.strings.clear();
		flow.handle(InteractionAction{});
		check(e && e->kind==(fault==0?Kind::MissingTextResource:fault==1?Kind::TextMapMismatch:Kind::InvalidTextIndex) &&
			e->source->opcode==5 && !flow.blocksGameplay(),"integrated NPC text error");
	}
	Fixture f;auto s=pending(f.begin());
	check(s.pendingPresentation->request.title==f.text.strings[0] && s.pendingPresentation->request.text==f.text.strings[1] &&
		s.logicalAddress.line==1 && s.instructionCount==1,"owned title/body and fallthrough continuation");
	f.text.strings={"changed","changed"};
	check(s.pendingPresentation->request.title!="changed","cache string borrowed");
	for(auto response:{XeenPresentationResponse(XeenPresentationResponse::Yes),XeenPresentationResponse(XeenPresentationResponse::No),
		XeenPresentationResponse(XeenPresentationResponse::Presented),XeenPresentationResponse(SelectedCharacter{0}),
		XeenPresentationResponse(CharacterSelectionCancelled{})})error(f.resume(s,response),Kind::InvalidPresentationResponse);
	check(std::get<XeenEventExecutionCompleted>(f.resume(s)).instructionCount==2,"mode1 used absent target instead of fallthrough");
	f.set({npc()});check(std::get<XeenEventExecutionCompleted>(f.resume(pending(f.begin()))).instructionCount==1,"natural successor completion");
	f.set({npc(255)});error(f.begin(255),Kind::LineOverflow);
	f.set({npc(),record(1,1,1,0x12)});s=pending(f.begin());s.instructionCount=1024;error(f.resume(s),Kind::InstructionLimitExceeded);
	f.camera.mapId={XeenSide::Darkside,1};f.scripts.emplace(f.camera.mapId,script(f.camera.mapId,{npc()}));error(f.begin(),Kind::UnsupportedExecutionContext);
	Fixture empty;empty.members={};empty.text.strings={"",""};check(std::holds_alternative<XeenEventExecutionSuspended>(empty.begin()),"empty NPC presentation unnecessarily rejected");
}
void contextAndEffects() {
	Fixture f;
	f.members.roster.at(8).currentSp=50;
	f.set({record(1,1,0,0x20,{0,0}),record(1,1,1,0x19,{2,2,0}),
		record(1,1,2,9,{9,50,4}),record(1,1,3,0xff),record(1,1,4,0x12),npc(0,2,2),record(2,2,1,0x1a)});
	auto choice=pending(f.begin());auto s=pending(f.resume(choice,SelectedCharacter{1}));
	check(s.activeCharacterIndex==1 && s.callStack.size()==1 && s.workingCamera.x==1 && s.logicalAddress.x==2 &&
		s.selectedObject==choice.selectedObject,"NPC lost call/character/object context");
	check(std::holds_alternative<XeenEventExecutionCompleted>(f.resume(s)),"NPC call return/action9 failed");
	// Logical transfer resolves the new map's strings while the physical camera
	// remains uncommitted until the resumed event completes.
	for(bool fail:{false,true}) {
		Fixture t;t.set({record(1,1,0,0x1f,{2,2,2})});
		t.scripts.emplace(2,script(2,{npc(0,2,2),record(2,2,1,fail?0xff:0x12)}));
		t.text.mapId=2;t.text.strings={"Map two title","Map two body"};
		auto flow=t.flow();flow.handle(InteractionAction{});
		check(flow.blocksGameplay() && t.camera.mapId==XeenMapIdentity(1),"NPC committed transfer before response");
		flow.handle(AcknowledgeAction{});
		check(!flow.blocksGameplay() && t.camera.mapId==XeenMapIdentity(fail?1:2),"NPC transfer completion/error policy");
	}
	for(bool fail:{false,true}) {
		Fixture a;a.failDraw=fail;auto flow=a.flow();bool completed=false,failed=false;
		flow.reportAutomatic=[&](const auto&r){completed=std::holds_alternative<XeenAutomaticEventCompleted>(r);
			failed=std::holds_alternative<XeenEventExecutionError>(r);};
		flow.acceptAutomatic(std::get<XeenEventExecutionSuspended>(a.begin()));
		if(!fail)flow.handle(AcknowledgeAction{});
		check(!flow.blocksGameplay() && completed==!fail && failed==fail,"automatic NPC completion/failure routing");
	}
	for(bool fail:{false,true}) {
		Fixture a;a.set({record(1,1,0,0x0c,{0,0,21,99}),record(1,1,1,0x0c,{0,0,20,7}),npc(2),record(1,1,3,fail?0xff:0x12)});
		auto flow=a.flow();flow.handle(InteractionAction{});
		check(a.members.questItems.at(17)==1 && !a.flags.isSet(7),"pre-NPC mutation policy");
		flow.handle(CancelInteractionAction{});
		check(!flow.blocksGameplay() && a.members.questItems.at(17)==1 && a.flags.isSet(7)==!fail,"NPC Escape completion/error policy");
	}
}
void presentationAndTiming() {
	Fixture f;f.text.strings[1]=std::string(800,'W')+" end of long body";
	auto flow=f.flow();flow.handle(InteractionAction{});const auto generation=flow.presentationGeneration();
	check(flow.presenter().pageCount()>1 && flow.handlesEscape() && !flow.canCancelInteraction(),"NPC paging/Escape kind");
	auto before=flow.frame();const auto count=f.draws;
	for(auto action:{PlayerAction(YesAction{}),PlayerAction(NoAction{}),PlayerAction(SelectMemberAction{0}),PlayerAction(NavigationAction::TurnLeft)})flow.handle(action);
	check(flow.frame().pixels==before.pixels && flow.presentationGeneration()==generation && f.draws==count && f.camera.direction==XeenDirection::North,"unrelated NPC input");
	for(std::size_t i=1;i<flow.presenter().pageCount();++i) {
		f.time+=300;flow.updatePresentation();const auto timing=flow.presenter().npcTiming();
		flow.handle(i%2?PlayerAction(CancelInteractionAction{}):PlayerAction(AcknowledgeAction{}));
		check(flow.blocksGameplay() && flow.presentationGeneration()==generation && flow.presenter().pageIndex()==i,"page resumed prematurely");
		const auto &after=flow.presenter().npcTiming();
		check(after.phase==timing.phase && after.displayedFrame==timing.displayedFrame && after.nextFrame==timing.nextFrame &&
			after.deadline==f.time+150,"page reset changed phase/frame");
		const auto pixels=flow.frame().pixels;flow.refresh(true);
		check(flow.presenter().pageIndex()==i && flow.frame().pixels==pixels,"noninitial page rebase");
	}
	flow.handle(InteractionAction{});check(!flow.blocksGameplay() && flow.frame().pixels==f.base.pixels,"final NPC cleanup");
	check(!flow.respond(*generation,XeenPresentationResponse::Acknowledged),"completed generation replayed");
	flow.handle(InteractionAction{});check(flow.presentationGeneration()!=generation && flow.presenter().pageIndex()==0,"independent NPC dispatch");
	flow.abandonPresentation();check(!flow.blocksGameplay() && flow.frame().pixels==f.base.pixels,"NPC abandonment");
	Fixture t;t.text.strings={"A B","C D"};auto timed=t.flow();timed.handle(InteractionAction{});
	check(timed.presenter().npcTiming().remaining==4 && timed.presenter().npcTiming().displayedFrame==0,"initial speech state");
	t.time=149;check(!timed.updatePresentation(),"early animation");
	t.time=150;timed.updatePresentation();check(timed.presenter().npcTiming().nextFrame==2 && timed.presenter().npcTiming().remaining==4,"first draw/select order");
	t.time=300;check(timed.updatePresentation().has_value(),"second frame missing");
	check(timed.presenter().npcTiming().displayedFrame==2 && timed.presenter().npcTiming().remaining==3,"alternating speech decrement");
	const auto phase=timed.presenter().npcTiming();const auto gen=timed.presentationGeneration();auto pixels=timed.frame().pixels;
	t.events.discardScriptCache();t.events.discardTextCache();t.world.discardMapCache();timed.refresh(true);
	check(timed.frame().pixels==pixels && timed.presentationGeneration()==gen && timed.presenter().npcTiming().remaining==phase.remaining &&
		timed.presenter().npcTiming().phase==phase.phase && timed.presenter().npcTiming().deadline==phase.deadline,"rebase changed temporal state");
	t.time=9999;timed.updatePresentation();check(timed.presenter().npcTiming().remaining==3,"stall replayed backlog");
	for(int i=0;i<20;++i){t.time+=150;timed.updatePresentation();}
	check(timed.presenter().npcTiming().remaining==0 && timed.presenter().npcTiming().displayedFrame==0 &&
		timed.frame().pixels[24+23*320]!=55,"rest left animated overlay");
	const auto draws=t.draws,composes=t.composes;for(int i=0;i<10;++i){t.time+=150;check(!timed.updatePresentation(),"rest keeps updating");}
	check(t.draws==draws && t.composes==composes,"rest keeps drawing");
	Fixture n;n.set({npc(),npc(1),record(1,1,2,0x12)});auto next=n.flow();next.handle(InteractionAction{});
	const auto old=*next.presentationGeneration();next.handle(CancelInteractionAction{});
	check(next.blocksGameplay() && *next.presentationGeneration()!=old && !next.respond(old,XeenPresentationResponse::Acknowledged),"input consumed next NPC");
	next.respond(*next.presentationGeneration(),XeenPresentationResponse::Acknowledged);check(!next.blocksGameplay(),"direct NPC response");
	Fixture pos;pos.text.strings={"A\n\t100B","body"};auto title=pos.flow();title.handle(InteractionAction{});
	check(title.frame().pixels[30*320+138]==25 && title.frame().pixels[40*320+113]==25 &&
		title.frame().pixels[40*320+138]==153,"positioned title anchor ignored");
	check(title.presenter().diagnostics().empty(),"valid title diagnostics");
	pos.text.strings[0]="\f01A\n\t100B";pos.events.discardTextCache();title.abandonPresentation();title.handle(InteractionAction{});
	check(title.frame().pixels[30*320+138]==8 && title.frame().pixels[40*320+113]==8,
		"title color state lost across positioned lines");
	pos.text.strings[0]="A\n\txB";pos.events.discardTextCache();title.abandonPresentation();title.handle(InteractionAction{});
	check(!title.presenter().diagnostics().empty(),"malformed tab undiagnosed");
	pos.text.strings[0]=std::string(100,'W');pos.events.discardTextCache();title.abandonPresentation();title.handle(InteractionAction{});
	check(!title.presenter().diagnostics().empty(),"clipped long title undiagnosed");
	// Source spans cover the raw string, including spaces trimmed at wraps.
	XeenTextRenderOptions opts;opts.bounds={0,0,24,10};opts.paginate=true;
	auto pages=XeenTextRenderer(pos.font).render(pos.base,"aa  bb cc  dd",opts);
	check(pages.pages.size()==pages.pageSourceEnds.size() && pages.pageSourceEnds.back()==13 &&
		std::is_sorted(pages.pageSourceEnds.begin(),pages.pageSourceEnds.end()),"raw page coverage");
	std::size_t ink=0;for(const auto&p:pages.pages)ink+=std::count(p.pixels.begin(),p.pixels.end(),25);
	check(ink==8*32,"pagination discarded or duplicated body glyphs");
}
void failuresAndOwners() {
	for(int when=0;when<4;++when) {
		Fixture f;f.set({record(1,1,0,0x0c,{0,0,21,99}),npc(1),record(1,1,2,0x0c,{0,0,21,100})});
		if(when==3)f.text.strings[1]=std::string(900,'W');
		auto flow=f.flow();std::optional<XeenEventExecutionError> err;
		flow.reportManual=[&](const auto&r){if(auto e=std::get_if<XeenEventExecutionError>(&r))err=*e;};
		f.failDraw=when==0;flow.handle(InteractionAction{});const auto gen=flow.presentationGeneration();
		f.failDraw=true;
		if(when==1){f.time=150;flow.updatePresentation();}
		if(when==2)flow.refresh(true);
		if(when==3)flow.handle(AcknowledgeAction{});
		check(err && err->kind==Kind::PresentationFailed && err->source->line==1 && !flow.blocksGameplay() &&
			flow.frame().pixels==f.base.pixels && f.members.questItems.at(17)==1 && f.members.questItems.at(18)==0,"NPC failure cleanup/state");
		if(gen)check(!flow.respond(*gen,XeenPresentationResponse::Acknowledged),"failed generation valid");
		f.failDraw=false;flow.handle(InteractionAction{});check(flow.blocksGameplay(),"NPC retry failed");
		flow.abandonPresentation();auto fresh=f.flow();check(!fresh.blocksGameplay() && f.members.questItems.at(17)==2,"fresh flow erased party owner");
	}
	Fixture f;f.set({npc(),record(1,1,1,0x0c,{0,0,21,99})});
	{XeenEventFlow noAssets(f.world,f.events,f.members,f.camera,f.flags,f.font,[&]{return f.base;});
		bool failed=false;noAssets.reportManual=[&](const auto&r){if(auto e=std::get_if<XeenEventExecutionError>(&r))failed=e->kind==Kind::PresentationFailed;};
		noAssets.handle(InteractionAction{});check(failed && !noAssets.blocksGameplay() && noAssets.frame().pixels==f.base.pixels,
			"missing NPC asset callback accepted");}
	{auto flow=f.flow();flow.handle(InteractionAction{});}check(f.members.questItems.at(17)==0,"destruction resumed event");
	{auto flow=f.flow();flow.handle(InteractionAction{});auto old=*flow.presentationGeneration();
		flow.acceptManual(f.events.runManualEvent(f.world,f.members,f.camera,f.flags));
		check(flow.blocksGameplay() && !flow.respond(old,XeenPresentationResponse::Acknowledged) && f.members.questItems.at(17)==0,
			"replaced NPC accepted stale response");flow.abandonPresentation();}
	for(auto response:{XeenPresentationResponse(XeenPresentationResponse::Yes),XeenPresentationResponse(CharacterSelectionCancelled{})}) {
		auto flow=f.flow();std::optional<Kind> errorKind;
		flow.reportManual=[&](const auto&r){if(auto e=std::get_if<XeenEventExecutionError>(&r))errorKind=e->kind;};
		flow.handle(InteractionAction{});flow.respond(*flow.presentationGeneration(),response);
		check(errorKind==Kind::InvalidPresentationResponse && flow.frame().pixels==f.base.pixels,"wrong response cleanup");
	}
	// Retained sign survives the transient NPC, including direct finalization/rebase.
	f.set({record(1,1,0,4,{1}),npc(1),record(1,1,2,0x12)});auto flow=f.flow();flow.handle(InteractionAction{});
	flow.respond(*flow.presentationGeneration(),XeenPresentationResponse::Acknowledged);
	const auto sign=flow.frame();check(sign.pixels!=f.base.pixels && flow.refresh(true).pixels==sign.pixels,"NPC erased retained sign");
}
Bytes fac() {
	using namespace sprite_test;Bytes b;word(b,4);b.resize(18);
	for(int frame=0;frame<4;++frame) {
		setWord(b,2+frame*4,b.size());auto first=cell(0,2,0,1,{4,0,1,static_cast<std::uint8_t>(10+frame),11});b.insert(b.end(),first.begin(),first.end());
		setWord(b,4+frame*4,b.size());auto second=cell(1,1,0,1,{3,0,0,static_cast<std::uint8_t>(20+frame)});b.insert(b.end(),second.begin(),second.end());
	}
	return b;
}
void assets() {
	using namespace sprite_test;
	const auto dir=std::filesystem::temp_directory_path()/"mmodern-npc-assets";std::filesystem::create_directories(dir);
	GameInstallation installation;installation.root=dir;installation.xeenArchive=dir/"xeen.cc";
	for(int bad=0;bad<7;++bad) {
		auto portrait=fac();if(bad==1)portrait.resize(12);if(bad==2)portrait.back()=255;
		if(bad==2)portrait[portrait.size()-3]=255; // Invalid last-frame row command.
		if(bad==3)portrait[0]=3;
		std::map<std::string,Bytes> files{{"face09.fac",portrait},{"frame.fac",sprite(cell(0,1,0,1,{3,0,0,88}))},{"base.raw",Bytes(64000,91)}};
		if(bad==4)files.erase("frame.fac");
		if(bad==5)files.erase("face09.fac");
		if(bad==6)files["frame.fac"].resize(3);
		archive(dir/"xeen.cc",files);
		XeenAssetSource a(installation,320,200);a.loadRawFramebuffer("base.raw");auto base=a.snapshot(),frame=base;
		bool failed=false;try {a.drawNpc(frame,9,0);}catch(const std::exception&){failed=true;}
		if(bad){check(failed && frame.pixels==base.pixels,"invalid FAC not atomic/preflighted");continue;}
		check(!failed && frame.pixels[16*320+16]==88 && frame.pixels[22*320+23]==10 && frame.pixels[22*320+24]==20,"FAC cells/order/anchors");
		check(a.snapshot().pixels==base.pixels,"NPC contaminated scene surface");
		for(int i=0;i<4;++i){frame=base;a.drawNpc(frame,9,i);check(frame.pixels[22*320+23]==10+i && frame.pixels[22*320+24]==20+i,"FAC frame mapping");}
		const auto loads=a.spriteLoadCount();a.discardSpriteCache();frame=base;a.drawNpc(frame,9,0);check(a.spriteLoadCount()==loads+2,"FAC cache reconstruction");
	}
	std::filesystem::remove(dir/"xeen.cc");std::filesystem::remove(dir);
}
void sdl() {
	Fixture f;f.text.strings[1]="one two three four";f.set({npc(),npc(1),record(1,1,2,0x0c,{0,0,21,99})});
	// Production clock + SDL's actual idle callback; no keyboard event starts animation.
	XeenEventFlow flow(f.world,f.events,f.members,f.camera,f.flags,f.font,[&]{return f.base;},f.draw(),{},[]{return 3;});
	flow.handle(InteractionAction{});bool changed=false;int actions=0,idles=0;std::uint32_t started=0;
	auto push=[](SDL_Keycode key,int repeat=0){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;e.key.repeat=repeat;SDL_PushEvent(&e);};
	const bool ok=SdlWindow().showInteractive(flow.frame(),"NPC SDL synthetic",
		[&](const PlayerAction&a)->std::optional<IndexedFrame>{++actions;auto result=flow.handle(a);
			if(actions==1){check(flow.blocksGameplay(),"Escape consumed next NPC");SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);push(SDLK_RETURN);}
			return result;},[&]{return flow.handlesEscape();},[&]()->std::optional<IndexedFrame>{
			++idles;if(!started)started=SDL_GetTicks();auto result=flow.updatePresentation();
			if(result && !changed){changed=true;push(SDLK_ESCAPE);push(SDLK_ESCAPE,1);}
			if(SDL_GetTicks()-started>3000){SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);}
			return result;});
	check(ok && changed && actions==1 && idles>1 && flow.blocksGameplay() && f.members.questItems.at(17)==0,"SDL idle/quit/repeat behavior");
	flow.abandonPresentation();check(!flow.blocksGameplay() && !flow.updatePresentation(),"SDL abandonment");
}
}
int main(int argc,char **argv) {
	try {
		if(argc>1 && std::string(argv[1])=="sdl")sdl();
		else {decoderAndExecution();contextAndEffects();presentationAndTiming();failuresAndOwners();assets();}
		std::cout<<"M19A NPC tests passed\n";return 0;
	}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
