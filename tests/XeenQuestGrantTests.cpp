#include "XeenRemoveTestSupport.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include "games/xeen/XeenPartyLoader.h"
#include <algorithm>
#include <iostream>
#include <limits>

using namespace mmodern;
using namespace remove_test;
namespace {
using Bytes = std::vector<std::uint8_t>;
XeenPartyState party(int members = 1) {
	Bytes roster(XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize);
	Bytes bytes(XeenQuestItemFormat::kRequiredSize);
	std::fill(bytes.begin()+2, bytes.begin()+10, 255);
	bytes[0]=bytes[1]=members;
	for(int i=0;i<members;++i) bytes[2+i]=i;
	return XeenPartyLoader().loadFromResources(roster,bytes);
}
Bytes fontBytes() {
	Bytes b(XeenFontFormat::kMinimumSize);
	for(int c=0;c<128;++c){b[0x1000+c]=6;b[0x1080+c]=3;
		for(int y=0;y<8;++y){b[c*16+y*2]=0x55;b[0x800+c*16+y*2]=0x55;}}
	return b;
}
struct Fixture {
	XeenPartyState members=party();
	std::map<XeenMapIdentity,XeenEventScript> scripts;
	int maps=0,objects=0,loads=0,texts=0;
	std::string message="Text";
	XeenWorld world{[&](XeenMapIdentity id){++maps;auto m=map(id);m.geometry.cells[17].rawAttributes=0x10;return m;},
		[&](XeenMapIdentity id){++objects;XeenObjectFile f{id,"synthetic.mob",true,{}};
			f.entities.objects={{1,1,0,0,111},{2,2,0,0,111}};return f;}};
	XeenEventSystem events{[&](XeenMapIdentity id){++loads;return scripts.at(id);},
		[&](XeenMapIdentity id){++texts;return XeenEventTextFile{id,"text",true,{message}};}};
	XeenEventInterpreter interpreter;
	XeenCamera camera{1,1,1,XeenDirection::North};
	XeenGameFlags flags;
	void set(std::vector<XeenEventRecord> records,XeenMapIdentity id=1){scripts.insert_or_assign(id,script(id,std::move(records)));}
	XeenEventExecutionStepResult begin(int line=0){return interpreter.begin(camera,members,flags,world,
		[&](XeenMapIdentity id){return scripts.at(id);},{},line);}
	XeenEventExecutionStepResult resume(XeenEventExecutionState state){return interpreter.resume(std::move(state),
		XeenPresentationResponse::Acknowledged,members,world,[&](XeenMapIdentity id){return scripts.at(id);},{});}
};
void error(const XeenEventExecutionStepResult &r,XeenEventExecutionErrorKind kind) {
	const auto *e=std::get_if<XeenEventExecutionError>(&r);
	check(e && e->kind==kind,"wrong grant error");
}
void complete(const XeenEventExecutionStepResult &r) {
	check(std::holds_alternative<XeenEventExecutionCompleted>(r),"grant did not complete");
}
XeenEventExecutionState pending(const XeenEventExecutionStepResult &r){return std::get<XeenEventExecutionSuspended>(r).state;}

void operandsAndCounts() {
	for(int size:{1,6})for(int id:{82,99,116})for(bool third:{false,true}) {
		Fixture f;f.members=party(size);Bytes payload{0,0,21,static_cast<std::uint8_t>(id)};
		if(third)payload.insert(payload.end(),{0,0});
		const auto decoded=XeenEventDecoder::decode(record(1,1,0,0x0c,payload));
		const auto &op=std::get<XeenEventTakeOrGive>(std::get<XeenDecodedEventInstruction>(decoded).operation);
		check(op.first.mode==0 && op.first.value==0 && op.second.mode==21 && op.second.value==id &&
			op.third.mode==0 && op.third.value==0,"Phirna payload decode");
		f.set({record(1,1,0,0x0c,payload),record(1,1,1,0x0c,payload)});
		complete(f.begin());
		for(int i=0;i<35;++i)check(f.members.questItems.at(i)==(i==id-82?2:0),"grant quantity/domain/member count");
	}
	for(const Bytes bytes:std::vector<Bytes>{{0,0,21},{0,0,21,99,0},{0,0,21,99,0,0,0}}){
		Fixture f;f.set({record(1,1,0,0x0c,bytes)});error(f.begin(),XeenEventExecutionErrorKind::MalformedInstruction);
		check(f.members.questItems.at(17)==0,"malformed grant mutated");
	}
	for(const Bytes bytes:std::vector<Bytes>{{0,1,21,99},{20,7,21,99},{21,99},{21,99,21,99},
		{0,0,21,99,0,1},{0,0,21,99,20,7},{0,0,22,99},{0,0,21,81},{0,0,21,117},{0,0,21,255}}){
		Fixture f;f.set({record(1,1,0,0x0c,bytes)});error(f.begin(),XeenEventExecutionErrorKind::UnsupportedOperationMode);
		check(f.members.questItems.counts()==XeenCloudsQuestItems::Counts{} && !f.flags.isSet(7),"invalid combination mutated");
	}
	for(std::uint32_t start:{255u,std::numeric_limits<std::uint32_t>::max()-1,std::numeric_limits<std::uint32_t>::max()}){
		Fixture f;XeenCloudsQuestItems::Counts counts{};counts[17]=start;counts[0]=8;
		f.members.questItems=XeenCloudsQuestItems(counts);f.set({record(1,1,0,0x0c,{0,0,21,99})});
		if(start==std::numeric_limits<std::uint32_t>::max())error(f.begin(),XeenEventExecutionErrorKind::QuestItemOverflow);
		else{complete(f.begin());++counts[17];}
		check(f.members.questItems.counts()==counts,"wide count or overflow");
	}
	auto first=party(),second=party();check(first.questItems.increment(17),"increment failed");
	check(first.questItems.at(17)==1 && second.questItems.at(17)==0,"party instances share counters");
	bool rejected=false;try{first.questItems.increment(35);}catch(const std::out_of_range &){rejected=true;}
	check(rejected && first.questItems.at(17)==1,"unbounded increment");
}

void prevalidationAndVisibility() {
	for(int mode=0;mode<6;++mode){
		Fixture f;f.set({record(1,1,0,9,{44,1,1}),record(1,1,1,0x0c,{0,0,21,99})});
		auto s=pending(f.begin());auto kind=XeenEventExecutionErrorKind::UnsupportedExecutionContext;
		if(mode==0)s.logicalAddress.mapId.side=XeenSide::Darkside;
		if(mode==1)s.workingCamera.mapId.side=XeenSide::Darkside;
		if(mode==2){f.members=party(0);kind=XeenEventExecutionErrorKind::EmptyParty;}
		if(mode==3){s.instructionCount=1024;kind=XeenEventExecutionErrorKind::InstructionLimitExceeded;}
		if(mode==4){s.selectedObject.reset();s.logicalAddress.line=254;s.pendingPresentation->conditional->targetLine=255;
			s.currentScript=script(1,{record(1,1,255,0x0c,{0,0,21,99})});kind=XeenEventExecutionErrorKind::LineOverflow;}
		if(mode==5){s.pendingPresentation->conditional->targetLine=2;s.currentScript=script(1,{record(1,1,2,0x0c,{0,0,21,117})});kind=XeenEventExecutionErrorKind::UnsupportedOperationMode;}
		error(f.resume(s),kind);check(f.members.questItems.counts()==XeenCloudsQuestItems::Counts{},"failed validation granted");
	}
	Fixture earlier;earlier.set({record(1,1,0,0x0c,{0,0,21,99}),record(1,1,1,0x0c,{0,0,21,117})});
	error(earlier.begin(),XeenEventExecutionErrorKind::UnsupportedOperationMode);
	check(earlier.members.questItems.at(17)==1,"later invalid grant rolled back earlier grant");
	Fixture call;call.set({record(1,1,0,0x0c,{0,0,21,99}),record(1,1,1,9,{21,99,3}),record(1,1,2,0xff),
		record(1,1,3,0x19,{7,8,0}),record(1,1,4,9,{21,99,6}),record(1,1,5,0xff),record(1,1,6,0x12),
		record(7,8,0,9,{21,99,2}),record(7,8,1,0xff),record(7,8,2,0x1a)});
	complete(call.begin());check(call.members.questItems.at(17)==1,"calls lost/doubled grant");
}

void immediateErrors() {
	for(bool automatic:{false,true})for(int mode=0;mode<3;++mode){
		Fixture f;f.set({record(1,1,0,0x1f,{2,1,1})});
		// Logical call is distinct from physical Remove: restart sees owned root
		// and reaches the fault without replaying the grant.
		f.set({record(1,1,0,0x19,{7,8,0}),record(1,1,1,0x12),record(7,8,0,9,{21,99,5}),
			record(7,8,1,0x0c,{0,0,20,7}),record(7,8,2,0x0c,{0,0,21,99}),
			record(7,8,3,mode==0?0xff:mode==1?9:0x0e,mode==1?Bytes{44,1,4}:Bytes{}),
			record(7,8,4,0x0e),record(7,8,5,0xff)},2);
		XeenFontFormat font(fontBytes());IndexedFrame base;base.width=320;base.height=200;base.pixels.resize(64000);
		XeenEventFlow flow(f.world,f.events,f.members,f.camera,f.flags,font,[&]{return base;});
		std::optional<XeenEventExecutionState> suspended;
		bool failed=false;auto report=[&](const auto &r){
			if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&r))suspended=pending->state;
			if(const auto *e=std::get_if<XeenEventExecutionError>(&r)){
			failed=e->kind==(mode==1?XeenEventExecutionErrorKind::InvalidRemoveContext:XeenEventExecutionErrorKind::UnsupportedOpcode);
			if(automatic)throw std::runtime_error("automatic failure");}};
		flow.reportManual=report;flow.reportAutomatic=report;
		bool propagated=false;
		try{
			automatic?flow.initial():flow.handle(InteractionAction{});
			if(mode==1){
				check(suspended.has_value(),"grant did not suspend before Remove");
				suspended->selectedObject=XeenObjectIdentity{2,999};
				if(automatic)flow.acceptAutomatic(f.events.resumeAutomaticEvent(*suspended,
					XeenPresentationResponse::Acknowledged,f.world,f.members,f.camera,f.flags));
				else flow.acceptManual(f.events.resumeManualEvent(*suspended,
					XeenPresentationResponse::Acknowledged,f.world,f.members,f.camera,f.flags));
			}
		}catch(const std::runtime_error &e){propagated=std::string(e.what())=="automatic failure";}
		check(f.members.questItems.at(17)==1 && f.camera.mapId==1 && !f.flags.isSet(7),"party/world transaction policy");
		check(failed && propagated==automatic && !flow.blocksGameplay(),"manual/automatic error policy");
		check(f.world.isObjectDisabled({2,0})==(mode==2) && !f.world.isObjectDisabled({2,1}) && !f.world.isObjectDisabled({1,0}),"world mutation persistence/isolation");
		check(f.world.sessionState().disabledEventCount()==(mode==2?2:0),"failed Remove changed events");
	}
}

void presentationNoReplay() {
	for(bool automatic:{false,true})for(bool yes:{false,true}){
		Fixture f;f.message=std::string(1600,'A');
		f.set({record(1,1,0,0x0c,{0,0,21,99}),record(1,1,1,0x01,{0}),record(1,1,2,9,{44,1,3}),
			record(1,1,3,9,{44,0,6}),record(1,1,4,9,{21,99,8}),record(1,1,5,0xff),
			record(1,1,6,9,{21,99,8}),record(1,1,7,0xff),record(1,1,8,0x12)});
		XeenFontFormat font(fontBytes());IndexedFrame base;base.width=320;base.height=200;base.pixels.resize(64000);
		XeenEventFlow flow(f.world,f.events,f.members,f.camera,f.flags,font,[&]{return base;});
		int line=-1;bool completed=false;auto report=[&](const auto &r){
			if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&r))line=s->request.source.line;
			else{check(!std::holds_alternative<XeenEventExecutionError>(r),"presentation continuation error");completed=true;}};
		flow.reportManual=report;flow.reportAutomatic=report;
		automatic?flow.initial():flow.handle(InteractionAction{});
		check(f.members.questItems.at(17)==1 && flow.blocksGameplay(),"grant before presentation");
		flow.handle(AcknowledgeAction{});const auto page=flow.frame();
		for(int i=0;i<4;++i)flow.handle(NavigationAction::TurnRight);
		check(flow.frame().pixels==page.pixels && f.camera.direction==XeenDirection::North,"pending input changed page/camera");
		f.world.discardMapCache();f.events.discardScriptCache();f.events.discardTextCache();
		check(flow.refresh(true).pixels==page.pixels,"cache rebase lost page");
		int pages=1;while(line!=2 && pages++<100){flow.handle(InteractionAction{});check(f.members.questItems.at(17)==1,"page replayed grant");}
		check(pages>2 && pages<100 && flow.blocksGameplay(),"multipage did not reach acknowledgment");
		flow.handle(AcknowledgeAction{});check(line==3 && flow.blocksGameplay(),"ack did not reach Yes/No");
		flow.handle(AcknowledgeAction{});check(flow.blocksGameplay(),"ack answered Yes/No");
		flow.handle(yes?PlayerAction{YesAction{}}:PlayerAction{NoAction{}});
		check(completed && !flow.blocksGameplay() && f.members.questItems.at(17)==1,"resume lost/doubled grant");
		flow.handle(NavigationAction::TurnRight);check(f.camera.direction==XeenDirection::East,"input did not recover");
		check(f.loads==2 && f.maps>=2,"cache providers did not reload");
	}
}
}
int main(){try{operandsAndCounts();prevalidationAndVisibility();immediateErrors();presentationNoReplay();
	std::cout<<"M17B grants, validation, immediate effects and shared-flow no-replay passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
