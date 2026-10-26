#include "XeenRemoveTestSupport.h"
#include "SyntheticXeenArchive.h"
#include "XeenDosTextTestSupport.h"
#include "app/XeenEventFlow.h"
#include "app/XeenAudio.h"
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
    mmodern::XeenProgramState programState{mmodern::XeenWallClock::fixed(0)};
	XeenPartyState members=party();
	XeenCamera camera{1,1,1,XeenDirection::North};
	XeenGameFlags flags;
	XeenFontFormat font{fontBytes()};
	IndexedFrame base;
	std::uint64_t time=0;unsigned random=2;int draws=0,loads=0,composes=0;
	bool failDraw=false, ordinary=false;
	unsigned randomReads=0;
	std::vector<std::uint64_t> phases;
	XeenEventTextFile text{1,"synthetic.txt",true,{"Title\n\t125Subtitle","one two three four"}};
	std::map<XeenMapIdentity,XeenEventScript> scripts;
	XeenWorld world{[](XeenMapIdentity id){auto m=map(id);m.geometry.cells[17].rawAttributes=0;return m;},[](XeenMapIdentity id){
		XeenObjectFile f{id,"synthetic.mob",true,{}};f.entities.objects=std::vector<XeenMapEntity>{{1,1,0,0,111}};return f;}};
	XeenEventSystem events{[&](XeenMapIdentity id){return scripts.at(id);},
		[&](XeenMapIdentity){++loads;return text;}};
	XeenEventInterpreter interpreter;
	Fixture(){
        auto options=XeenSoundOptions(members.soundOptions);options.text=true;members.soundOptions=options;
        auto module=dos_test::fixture();
        for(unsigned i=0;i<6;++i){
            module[0x4f10a+i]=0;
            dos_test::u16(module,0x508cc+4*i,0);
            dos_test::u16(module,0x508ce + 4*i,0x481d);
        }
        programState.audio=std::make_shared<XeenAudio>(0,PromusDriver::Bytes{},
            [](const auto &,auto){return Bytes{};},std::move(module),0,false);
        base.width=320;base.height=200;base.pixels.assign(64000,90);set({npc(),record(1,1,1,0x12)});}
	void set(std::vector<XeenEventRecord> rs){scripts.insert_or_assign(1,script(1,std::move(rs)));events.discardScriptCache();}
	XeenEventPresenter::NpcDraw draw(){return [&](IndexedFrame &f,std::uint8_t id,std::size_t frame){
		check(id==9,"portrait operand lost");++draws;if(failDraw)throw std::runtime_error("synthetic FAC failure");
		f.pixels[23+23*320]=static_cast<std::uint8_t>(40+frame);
		if(frame==2)f.pixels[24+23*320]=55;
	};}
	IndexedFrame ordinaryBase(std::uint64_t phase) const {
		auto f=base;
		if(ordinary)for(int y=80;y<100;++y)for(int x=80;x<100;++x)f.pixels[y*320+x]=20+phase%10;
		return f;
	}
	XeenEventFlow flow(){return XeenEventFlow(programState,world,events,members,camera,flags,font,
		[&](std::uint64_t phase){++composes;phases.push_back(phase);return XeenEventFlow::Composition{ordinaryBase(phase), ordinary};},draw(),[&]{return time;},[&]{++randomReads;return random;},nullptr,{},nullptr,{},&dos_test::npcText());}
    void acquire(XeenEventFlow &flow) {flow.framePresented(flow.frame().presentation());}
    void tap(XeenEventFlow &flow,const PlayerAction &action) {
        acquire(flow);
        if(const auto deadline=flow.nextPresentationDeadline())time=std::max(time,PresentationClock::ceilMilliseconds(*deadline));
        flow.handle(action);acquire(flow);flow.updatePresentation();acquire(flow);
        // Part 0: NPC retirement and the outer input's pass have distinct acquisitions.
        for(unsigned n=0;n<4 && !flow.presentationGeneration() && flow.blocksGameplay();++n){flow.updatePresentation();acquire(flow);}
    }
    void tick(XeenEventFlow &flow,std::uint64_t now) {time=now;if(flow.updatePresentation())acquire(flow);}
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
void skillComparisons() {
 for(const auto opcode:{8,9,10})for(const auto all:{false,true})for(unsigned holders=0;holders<=2;++holders) {
  Fixture f;f.members.roster.at(4).skills[4]=holders==2?7:0;
  f.members.roster.at(8).skills[4]=holders>=1?1:0;
  std::vector<XeenEventRecord> records;
  unsigned line=0;if(all)records.push_back(record(1,1,line++,0x0f,{0}));
  records.push_back(record(1,1,line,opcode,{13,4,std::uint8_t(line+2)}));
  records.push_back(record(1,1,line+1,0x12));
  records.push_back(record(1,1,line+2,0x0c,{0,0,20,7}));f.set(records);
  const auto result=f.interpreter.execute(f.camera,f.members,f.flags,f.world,[&](auto id){return f.scripts.at(id);});
  // A failed skill test leaves the record offset (100) in the low word: >= 4 holds, == 4 fails
  // and <= 4 depends on the unknown high word, so it is refused unless another member passes.
  if(opcode==10 && (all?holders==0:holders!=2)) {
   check(std::holds_alternative<XeenEventExecutionError>(result),"residue <= comparison was not refused");
   continue;
  }
  const auto *done=std::get_if<XeenEventExecutionCompleted>(&result);
  check(done && done->finalGameFlags.isSet(7)==(opcode!=9 || (all?holders>0:holders==2)),"Selected/all-member skill13 comparison changed");
 }
 for(const auto opcode:{8,9,10})for(unsigned holders=0;holders<=2;++holders) {
  Fixture f;f.members.roster.at(4).skills[11]=holders>=1?1:0;
  f.members.roster.at(8).skills[11]=holders>=2?255:0;
  f.members.roster.at(4).conditions[static_cast<unsigned>(XeenCondition::Dead)]=1;
  f.set({record(1,1,0,opcode,{99,11,2}),record(1,1,1,0x12),record(1,1,2,0x0c,{0,0,20,7})});
  const auto result=f.interpreter.execute(f.camera,f.members,f.flags,f.world,[&](auto id){return f.scripts.at(id);});
  const auto *done=std::get_if<XeenEventExecutionCompleted>(&result);
  check(done && done->finalGameFlags.isSet(7)==(opcode==8 || holders==2),"Party skill99 Pathfinder count/condition filter changed");
 }
 Fixture unsupported;unsupported.set({record(1,1,0,0x09,{107,1,1}),record(1,1,1,0x12)});
 error(unsupported.begin(),Kind::UnsupportedConditionAction);
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
	{
  Fixture offer;auto r=npc();r.parameters={0,1,9,2};offer.set({r,record(1,1,1,0x12)});
  const auto state=pending(offer.begin());
  check(state.pendingPresentation->request.response==XeenPresentationResponseRequirement::Presented,"Mode2 offer added an acknowledgment/yes-no");
  XeenEventPresenter presenter(offer.font,offer.draw());presenter.originalAssets(&dos_test::npcText(),{});const auto shown=presenter.present(offer.base,state.pendingPresentation->request);
  check(shown.response && *shown.response==XeenPresentationResponse::Presented && !presenter.blocksGameplay() && presenter.hasNpcOffer(),"Mode2 offer did not remain as passive NPC layer");
  check(std::holds_alternative<XeenEventExecutionCompleted>(offer.resume(state,XeenPresentationResponse::Presented)),"Mode2 offer failed sequential continuation");
 }
 for(int mode:{3,255}) {Fixture f;auto r=npc();r.parameters[3]=mode;f.set({r});error(f.begin(),Kind::UnsupportedOperand);}
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
		t.tap(flow,AcknowledgeAction{});
		check(!flow.blocksGameplay() && t.camera.mapId==XeenMapIdentity(fail?1:2),"NPC transfer completion/error policy");
	}
	for(bool fail:{false,true}) {
		Fixture a;a.failDraw=fail;auto flow=a.flow();bool completed=false,failed=false;
		flow.reportAutomatic=[&](const auto&r){completed=std::holds_alternative<XeenAutomaticEventCompleted>(r);
			failed=std::holds_alternative<XeenEventExecutionError>(r);};
		flow.acceptAutomatic(std::get<XeenEventExecutionSuspended>(a.begin()));
		if(!fail)a.tap(flow,AcknowledgeAction{});
		check(!flow.blocksGameplay() && completed==!fail && failed==fail,"automatic NPC completion/failure routing");
	}
	for(bool fail:{false,true}) {
		Fixture a;a.set({record(1,1,0,0x0c,{0,0,21,99}),record(1,1,1,0x0c,{0,0,20,7}),npc(2),record(1,1,3,fail?0xff:0x12)});
		auto flow=a.flow();flow.handle(InteractionAction{});
		check(a.members.questItems.at(17)==1 && !a.flags.isSet(7),"pre-NPC mutation policy");
		a.tap(flow,CancelInteractionAction{});
		if(!fail) {
			check(flow.blocksGameplay() && !a.flags.isSet(7),"Quest display must retain final flag settlement");
			for(unsigned phase=0;phase<3;++phase)a.tap(flow,AcknowledgeAction{});
		}
		check(!flow.blocksGameplay() && a.members.questItems.at(17)==1 && a.flags.isSet(7)==!fail,"NPC Escape completion/error policy");
	}
}
void presentationAndTiming() {
	Fixture f;f.text.strings[1]=std::string(800,'W')+" end of long body";
	auto flow=f.flow();flow.handle(InteractionAction{});const auto generation=flow.presentationGeneration();
	check(flow.presenter().pageCount()>1 && flow.handlesEscape() && !flow.canCancelInteraction(),"NPC paging/Escape kind");
	auto before=flow.frame();const auto count=f.draws;
	for(auto action:{PlayerAction(YesAction{}),PlayerAction(NoAction{}),PlayerAction(SelectMemberAction{0}),PlayerAction(NavigationAction::TurnLeft),PlayerAction(EquipmentInventoryAction{})})flow.handle(action);
	check(flow.frame().pixels==before.pixels && flow.presentationGeneration()==generation && f.draws==count && f.camera.direction==XeenDirection::North,"unrelated NPC input");
	for(std::size_t i=1;i<flow.presenter().pageCount();++i) {
		f.tap(flow,i%2?PlayerAction(CancelInteractionAction{}):PlayerAction(AcknowledgeAction{}));
		check(flow.blocksGameplay() && flow.presentationGeneration()==generation && flow.presenter().pageIndex()==i,"page resumed prematurely");
		const auto &after=flow.presenter().npcTiming();
		check(after.displayedFrame==0 && !after.remaining && after.deadline==PresentationClock::afterMilliseconds(f.time,3),"silent page restarted speech or lost three-tick poll");
		const auto pixels=flow.frame().pixels;flow.refresh(true);
		check(flow.presenter().pageIndex()==i && flow.frame().pixels==pixels,"noninitial page rebase");
	}
	f.tap(flow,InteractionAction{});check(!flow.blocksGameplay() && flow.frame().pixels==f.base.pixels,("final NPC cleanup: blocked="+std::to_string(flow.blocksGameplay())+" pending="+std::to_string(bool(flow.presentationGeneration()))+" base="+std::to_string(flow.frame().pixels==f.base.pixels)).c_str());
	check(!flow.respond(*generation,XeenPresentationResponse::Acknowledged),"completed generation replayed");
	flow.handle(InteractionAction{});check(flow.presentationGeneration()!=generation && flow.presenter().pageIndex()==0,"independent NPC dispatch");
	flow.abandonPresentation();check(!flow.blocksGameplay() && flow.frame().pixels==f.base.pixels,"NPC abandonment");
	Fixture t;t.text.strings={"A B","C D"};
    t.set({record(1,1,0,60,{19,30,0,54,1}),npc(1),record(1,1,2,0x12)});
    auto timed=t.flow();timed.handle(InteractionAction{});t.acquire(timed);
	check(timed.presenter().npcTiming().remaining==3066666667ULL && timed.presenter().npcTiming().displayedFrame==0,"CD span replaces word count");
	t.time=164;check(!timed.updatePresentation(),"early animation");
	t.time=165;timed.updatePresentation();check(timed.presenter().npcTiming().displayedFrame==2 && t.randomReads==1,"eligible CD-busy draw selects one frame");
    t.time=330;check(!timed.updatePresentation() && t.randomReads==1,"Unacquired NPC frame advanced");
    t.acquire(timed);t.time=495;check(timed.updatePresentation().has_value(),"second frame missing");t.acquire(timed);
	check(timed.presenter().npcTiming().displayedFrame==2 && t.randomReads==2,"CD busy sampled independently of words");
	const auto phase=timed.presenter().npcTiming();const auto gen=timed.presentationGeneration();auto pixels=timed.frame().pixels;
	t.events.discardScriptCache();t.events.discardTextCache();t.world.discardMapCache();timed.refresh(true);
	check(timed.frame().pixels==pixels && timed.presentationGeneration()==gen && timed.presenter().npcTiming().remaining==phase.remaining &&
		timed.presenter().npcTiming().phase==phase.phase && timed.presenter().npcTiming().deadline==phase.deadline,"rebase changed temporal state");
	t.time=9999;timed.updatePresentation();t.acquire(timed);check(!timed.presenter().npcTiming().remaining && t.randomReads==2,"late wake sampled completed CD or replayed backlog");
	check(timed.presenter().npcTiming().remaining==0 && timed.presenter().npcTiming().displayedFrame==0 &&
		timed.frame().pixels[24+23*320]!=55,"rest left animated overlay");
	const auto randomReads=t.randomReads;for(int i=0;i<10;++i)t.tick(timed,t.time+165);
	check(t.randomReads==randomReads,"silent NPC draws consumed random requests");
	Fixture n;n.set({npc(),npc(1),record(1,1,2,0x12)});auto next=n.flow();next.handle(InteractionAction{});
	const auto old=*next.presentationGeneration();n.tap(next,CancelInteractionAction{});
	check(next.blocksGameplay() && *next.presentationGeneration()!=old && !next.respond(old,XeenPresentationResponse::Acknowledged),"input consumed next NPC");
	next.respond(*next.presentationGeneration(),XeenPresentationResponse::Acknowledged);
    for(unsigned retry=0;retry<4 && next.blocksGameplay() && !next.presentationGeneration();++retry){n.tick(next,n.time);}
    check(!next.blocksGameplay(),"direct NPC response");
	Fixture pos;pos.text.strings={"A\n\t100B","body"};auto title=pos.flow();title.handle(InteractionAction{});
    const auto expectedTitle=[&] {
        XeenTextRenderOptions options;options.originalControls=true;options.stopAtBottom=true;
        options.drawWindow=true;options.windowBounds={8,8,224,140};options.bounds={16,16,216,132};
        options.x=options.y=16;
        auto frame=XeenTextRenderer(pos.font).render(pos.base,xeenDialogFormat(dos_test::npcText().scalar("NPC_DIALOG"),pos.text.strings),options).pages.front();
        pos.draw()(frame,9,0);return frame;
    };
    check(title.frame().pixels==expectedTitle().pixels,"NPC title/body did not use original writer coordinates");
    pos.text.strings[0]="\f01A\n\t100B";pos.events.discardTextCache();title.abandonPresentation();title.handle(InteractionAction{});
    check(title.frame().pixels==expectedTitle().pixels,"original NPC color/control state lost");
	// Source spans cover the raw string, including spaces trimmed at wraps.
	XeenTextRenderOptions opts;opts.bounds={0,0,24,10};opts.paginate=true;
	auto pages=XeenTextRenderer(pos.font).render(pos.base,"aa  bb cc  dd",opts);
	check(pages.pages.size()==pages.pageSourceEnds.size() && pages.pageSourceEnds.back()==13 &&
		std::is_sorted(pages.pageSourceEnds.begin(),pages.pageSourceEnds.end()),"raw page coverage");
	std::size_t ink=0;for(const auto&p:pages.pages)ink+=std::count(p.pixels.begin(),p.pixels.end(),25);
	check(ink==8*32,"pagination discarded or duplicated body glyphs");
}
void ordinaryCoexistence() {
 for(bool ordinary:{false,true})for(bool npcActive:{false,true}) {
  Fixture f;f.ordinary=ordinary;auto flow=f.flow();
  if(npcActive){flow.handle(InteractionAction{});f.acquire(flow);}
  const auto generation=flow.presentationGeneration();
  unsigned ordinarySteps=0;
  for(std::uint64_t now:{109,110,164,165,219,220,330,9999}) {
   const auto count=f.composes;const auto random=f.randomReads;
   const auto timing=flow.presenter().npcTiming();
   f.time=now;const bool due=!npcActive && (now==110||now==220||now==330||now==9999);
   const bool npcDue=npcActive&&(now==165||now==330||now==9999);
   if(due)++ordinarySteps;
   if(flow.updatePresentation())f.acquire(flow);
   check(f.composes==count+(due?1:0),"ordinary/NPC coalesced composition count");
   check(f.randomReads==random,"silent NPC/ordinary redraw consumed randomness");
   check(flow.presentationGeneration()==generation && flow.blocksGameplay()==npcActive,"idle consumed pending generation");
   if(!npcDue)check(flow.presenter().npcTiming().deadline==timing.deadline && flow.presenter().npcTiming().phase==timing.phase,"ordinary step rearmed NPC");
   else check(flow.presenter().npcTiming().deadline==PresentationClock::afterMilliseconds(now,3),"NPC deadline not independent");
   if(ordinary&&due)check(f.phases.back()==ordinarySteps,"ordinary phase count with NPC");
   const auto after=f.composes;const auto randomAfter=f.randomReads;flow.updatePresentation();
   check(f.composes==after && f.randomReads==randomAfter,"same-now coexistence double advance");
  }
  if(npcActive){f.tap(flow,AcknowledgeAction{});check(!flow.blocksGameplay() && flow.frame().pixels==f.ordinaryBase(ordinarySteps).pixels,"covered animation not revealed at current base");}
 }
 // Noninitial pages and retained labels survive animated rebases.
 Fixture f;f.ordinary=true;f.text.strings[1]=std::string(800,'W')+" words for speech";
 f.set({record(1,1,0,4,{0}),npc(1),record(1,1,2,0x12)});auto flow=f.flow();flow.handle(InteractionAction{});
 f.tap(flow,AcknowledgeAction{});const auto gen=flow.presentationGeneration();const auto page=flow.presenter().pageIndex();
 check(page==1,"animated noninitial NPC page fixture");
 f.time=100;flow.updatePresentation();const auto pixels=flow.frame().pixels;const auto timing=flow.presenter().npcTiming();
 f.world.discardMapCache();f.events.discardScriptCache();f.events.discardTextCache();flow.refresh(true);
 check(flow.frame().pixels==pixels && flow.presenter().pageIndex()==page && flow.presentationGeneration()==gen &&
  flow.presenter().npcTiming().deadline==timing.deadline,"animated noninitial reconstruction lost state");
 while(flow.blocksGameplay())f.tap(flow,AcknowledgeAction{});
 XeenEventPresenter oracle(f.font);XeenPresentationRequest label;label.kind=XeenPresentationKind::SceneLabelSign;label.text=f.text.strings[0];
 const auto afterSpeechPhase=f.phases.back();
 const auto expected=oracle.present(f.ordinaryBase(afterSpeechPhase),label).frame;
 check(flow.frame().pixels==expected.pixels,"idle erased retained label/current base");
 f.time=200;flow.updatePresentation();XeenEventPresenter later(f.font);
 check(flow.frame().pixels==later.present(f.ordinaryBase(afterSpeechPhase+1),label).frame.pixels,"idle erased completed retained label");
 flow.handle(NavigationAction::MoveForward);check(flow.frame().pixels==f.ordinaryBase(afterSpeechPhase+2).pixels,"accepted blocked action retained label");
 // Both consumers call the one stored mutable clock, rather than independent copies.
 Fixture shared;std::vector<unsigned> calls;
 XeenEventPresenter::Clock clock=[n=0U,&calls]() mutable {calls.push_back(++n);return n;};
 XeenEventFlow sharing(shared.programState,shared.world,shared.events,shared.members,shared.camera,shared.flags,shared.font,
  [&](std::uint64_t){return XeenEventFlow::Composition{shared.base,true};},shared.draw(),clock,{},nullptr,{},nullptr,{},&dos_test::npcText());
 sharing.handle(InteractionAction{});sharing.updatePresentation();
 check(calls.size()>=4,"shared clock consumers were not exercised");
 for(unsigned i=0;i<calls.size();++i)check(calls[i]==i+1,"stateful clock copied between Flow and NPC");

}
void speechOff() {
 Fixture f;f.members.soundOptions=XeenSoundOptions{true,true,false,false};
 f.set({record(1,1,0,60,{19,30,0,54,1}),npc(1),record(1,1,2,0x12)});
 auto flow=f.flow();flow.handle(InteractionAction{});f.acquire(flow);
 check(flow.blocksGameplay() && flow.presenter().npcTiming().remaining==0 && !f.programState.audio->cdDeadline(f.time),
       "Speech-off created a CD wait");
 const auto reads=f.randomReads;f.tick(flow,165);
 check(f.randomReads==reads,"Speech-off animated a busy voice");
 flow.abandonPresentation();
 // The disabled command must still replace the retained same-disc operands,
 // including when there is no NPC in the event that issued the command.
 Fixture retained;retained.members.soundOptions=XeenSoundOptions{true,true,false,false};
 retained.set({record(1,1,0,60,{19,30,0,54,1}),record(1,1,1,0x12)});
 auto later=retained.flow();later.handle(InteractionAction{});retained.acquire(later);
 check(!later.blocksGameplay() && !retained.programState.audio->cdDeadline(retained.time) && later.frame().pixels==retained.base.pixels,
       "Disabled op60 drew a speech notice or created a wait");
 check(later.configureSound(XeenSoundOptions{}),"Enable Speech after the silent command");
 retained.set({npc(),record(1,1,1,0x12)});
 later.handle(InteractionAction{});retained.acquire(later);
 check(later.presenter().npcTiming().remaining==3066666667ULL,
       "Speech-off op60 lost the retained cue used by a later same-disc NPC");
 later.abandonPresentation();
}
void cdLifetime() {
 // Mode 2 starts CD and leaves the passive portrait behind WhoWill.
 Fixture f;auto offer=npc(1);offer.parameters={0,1,9,2};
 f.set({record(1,1,0,60,{19,30,0,54,1}),offer,record(1,1,2,0x20,{1,0}),record(1,1,3,0x12)});
 auto flow=f.flow();flow.handle(InteractionAction{});f.acquire(flow);
 check(flow.presenter().hasNpcOffer() && flow.canCancelInteraction() &&
       flow.presenter().npcTiming().remaining==3066666667ULL,"Passive NPC failed to start its CD interval");
 f.tick(flow,165);
 check(f.randomReads==1 && flow.presenter().npcTiming().displayedFrame==2,"WhoWill failed to animate the retained busy NPC");
 flow.respond(*flow.presentationGeneration(),CharacterSelectionCancelled{});
 const auto reads=f.randomReads;f.tick(flow,330);
 check(!flow.presenter().hasNpcOffer() && f.randomReads==reads,"Event completion left passive NPC animation active");
 // A no-op cue preserves the passive call's audio; ordinary final dismissal stops it.
 auto normal=npc(1);normal.parameters[3]=1;
 f.set({record(1,1,0,60,{0,0,0,0,0}),normal,record(1,1,2,0x12)});
 flow.handle(InteractionAction{});f.acquire(flow);
 check(flow.presenter().npcTiming().remaining==0,"WhoWill exit failed to stop CD before the all-zero cue");
 f.tap(flow,CancelInteractionAction{});
 flow.handle(InteractionAction{});f.acquire(flow);
 check(flow.presenter().npcTiming().remaining==0,"Ordinary dismissal failed to stop CD");
 flow.abandonPresentation();
 // Each fresh invocation restarts a retained nonzero cue, but page changes do not.
 Fixture pages;pages.text.strings[1]=std::string(800,'W');
 pages.set({record(1,1,0,60,{19,30,0,54,1}),npc(1),npc(2),record(1,1,3,0x12)});
 auto paged=pages.flow();paged.handle(InteractionAction{});pages.acquire(paged);
 pages.tap(paged,AcknowledgeAction{});
 check(paged.presenter().pageIndex()==1 && paged.presenter().npcTiming().remaining<3066666667ULL,"Page change restarted CD");
 const auto generation=paged.presentationGeneration();
 while(paged.presentationGeneration()==generation)pages.tap(paged,AcknowledgeAction{});
 check(paged.presenter().pageIndex()==0 && paged.presenter().npcTiming().remaining==3066666667ULL,"Fresh NPC invocation did not restart the retained cue");
 paged.abandonPresentation();
 pages.set({npc(),record(1,1,1,0x12)});
 paged.handle(InteractionAction{});pages.acquire(paged);
 check(paged.presenter().npcTiming().remaining==3066666667ULL,"Same-disc Event boundary discarded retained CD operands");
}
void failuresAndOwners() {
	for(int when=0;when<4;++when) {
		Fixture f;f.set({record(1,1,0,0x0c,{0,0,21,99}),npc(1),record(1,1,2,0x0c,{0,0,21,100})});
		if(when==3)f.text.strings[1]=std::string(900,'W');
		auto flow=f.flow();std::optional<XeenEventExecutionError> err;
		flow.reportManual=[&](const auto&r){if(auto e=std::get_if<XeenEventExecutionError>(&r))err=*e;};
		f.failDraw=when==0;flow.handle(InteractionAction{});const auto gen=flow.presentationGeneration();
        if(when!=0)f.acquire(flow);
		f.failDraw=true;
		if(when==1){f.time=165;flow.updatePresentation();}
		if(when==2)flow.refresh(true);
		if(when==3) {
            f.failDraw=false;f.time=165;flow.handle(AcknowledgeAction{});f.acquire(flow);
            f.failDraw=true;flow.updatePresentation();
        }
		for(unsigned retry=0;retry<4 && flow.blocksGameplay() && !flow.presentationGeneration();++retry){f.acquire(flow);flow.updatePresentation();}
		check(err && err->kind==Kind::PresentationFailed && err->source->line==1 && !flow.blocksGameplay() &&
			flow.frame().pixels==f.base.pixels && f.members.questItems.at(17)==1 && f.members.questItems.at(18)==0,"NPC failure cleanup/state");
		if(gen)check(!flow.respond(*gen,XeenPresentationResponse::Acknowledged),"failed generation valid");
		f.failDraw=false;flow.handle(InteractionAction{});check(flow.blocksGameplay(),"NPC retry failed");
		flow.abandonPresentation();auto fresh=f.flow();check(!fresh.blocksGameplay() && f.members.questItems.at(17)==2,"fresh flow erased party owner");
	}
	Fixture f;f.set({npc(),record(1,1,1,0x0c,{0,0,21,99})});
	{XeenEventFlow noAssets(f.programState,f.world,f.events,f.members,f.camera,f.flags,f.font,[&](std::uint64_t){return XeenEventFlow::Composition{f.base, false};});
		bool failed=false;noAssets.reportManual=[&](const auto&r){if(auto e=std::get_if<XeenEventExecutionError>(&r))failed=e->kind==Kind::PresentationFailed;};
		noAssets.handle(InteractionAction{});check(failed && !noAssets.blocksGameplay() && noAssets.frame().pixels==f.base.pixels,
			"missing NPC asset callback accepted");}
	{auto flow=f.flow();flow.handle(InteractionAction{});}check(f.members.questItems.at(17)==0,"destruction resumed event");
	{auto flow=f.flow();flow.handle(InteractionAction{});auto old=*flow.presentationGeneration();
		flow.abandonPresentation();
		flow.acceptManual(f.events.runManualEvent(f.world,f.members,f.camera,f.flags));
		check(flow.blocksGameplay() && !flow.respond(old,XeenPresentationResponse::Acknowledged) && f.members.questItems.at(17)==0,
			"replaced NPC accepted stale response");flow.abandonPresentation();}
	for(auto response:{XeenPresentationResponse(XeenPresentationResponse::Yes),XeenPresentationResponse(CharacterSelectionCancelled{})}) {
		auto flow=f.flow();std::optional<Kind> errorKind;
		flow.reportManual=[&](const auto&r){if(auto e=std::get_if<XeenEventExecutionError>(&r))errorKind=e->kind;};
		flow.handle(InteractionAction{});const auto generation=flow.presentationGeneration();
		const auto pixels=flow.frame().pixels;
		check(!flow.respond(*generation,response) && !errorKind && flow.presentationGeneration()==generation &&
			flow.frame().pixels==pixels,"wrong response consumed pending NPC");
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
	XeenEventFlow flow(f.programState,f.world,f.events,f.members,f.camera,f.flags,f.font,[&](std::uint64_t){return XeenEventFlow::Composition{f.base, false};},f.draw(),{},[]{return 3;},nullptr,{},nullptr,{},&dos_test::npcText());
	flow.handle(InteractionAction{});bool changed=false;int actions=0,idles=0;std::uint32_t started=0;
	auto push=[](SDL_Keycode key,int repeat=0){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;e.key.repeat=repeat;SDL_PushEvent(&e);};
	SdlWindow::FrameUpdateHandler handler=[&](const PlayerAction&a)->std::optional<IndexedFrame>{++actions;auto result=flow.handle(a);
			if(actions==1){check(flow.blocksGameplay(),"Escape consumed next NPC");SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);push(SDLK_RETURN);}
			return result;};
    handler.framePresented=[&](const auto &origin){flow.framePresented(origin);};
    handler.completeInputHandoff=[&](const auto &origin){flow.completeInputHandoff(origin);};
    handler.inputContext=[&](const auto &origin){return flow.inputContext(origin);};
    handler.nextPresentationDeadline=[&]{return flow.nextPresentationDeadline();};
    handler.presentationClock=[&]{return flow.presentationNow();};
    handler.protectAllKeys=true;
	const bool ok=SdlWindow().showInteractive(flow.frame(),"NPC SDL synthetic",handler,
        [&]{return flow.handlesEscape();},[&]()->std::optional<IndexedFrame>{
			++idles;if(!started)started=SDL_GetTicks();auto result=flow.updatePresentation();
			if(result && !changed){changed=true;push(SDLK_ESCAPE);push(SDLK_ESCAPE,1);}
			if(SDL_GetTicks()-started>3000){SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);}
			return result;});
	check(ok && changed && actions==1 && idles>1 && flow.blocksGameplay() && f.members.questItems.at(17)==0,"SDL idle/quit/repeat behavior");
	flow.abandonPresentation();const auto npcDraws=f.draws;flow.updatePresentation();
	check(!flow.blocksGameplay() && !flow.presentationGeneration() && f.draws==npcDraws && f.members.questItems.at(17)==0,"SDL abandonment must retire NPC while ordinary drawing may resume");
}
}
int main(int argc,char **argv) {
	try {
		if(argc>1 && std::string(argv[1])=="sdl")sdl();
		else {skillComparisons();decoderAndExecution();contextAndEffects();presentationAndTiming();ordinaryCoexistence();speechOff();cdLifetime();failuresAndOwners();assets();}
		std::cout<<"M19A NPC tests passed\n";return 0;
	}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
