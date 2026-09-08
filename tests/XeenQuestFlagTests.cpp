#include "XeenRemoveTestSupport.h"
#include "XeenPartySnapshotTestSupport.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include <algorithm>
#include <iostream>
#include <limits>

using namespace mmodern;
using namespace remove_test;
namespace {
using Bytes=std::vector<std::uint8_t>;
using Kind=XeenEventExecutionErrorKind;
Bytes resource(int members=1) {
	Bytes b(XeenQuestItemFormat::kRequiredSize);
	b[0]=b[1]=members;std::fill(b.begin()+2,b.begin()+10,255);
	for(int i=0;i<members;++i)b[2+i]=i;
	return b;
}
XeenPartyState party(int count=1) {
	return XeenPartyLoader().loadFromResources(Bytes(30*XeenCharacter::kSerializedSize),resource(count));
}
template<class F> void rejects(F f) {
	bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"expected rejection");
}
XeenEventRecord set(int line=0,int index=2,int x=1,int y=1) {
	return record(x,y,line,0x0c,{0,0,104,static_cast<std::uint8_t>(index)});
}
struct Fixture {
	XeenPartyState members=party();XeenCamera camera{1,1,1,XeenDirection::North};XeenGameFlags flags;
	std::map<XeenMapIdentity,XeenEventScript> scripts;
	XeenWorld world{[](XeenMapIdentity id){auto m=map(id);m.geometry.cells[17].rawAttributes=0x10;return m;},
		[](XeenMapIdentity id){XeenObjectFile f{id,"synthetic.mob",true,{}};f.entities.objects={{1,1,0,0,111},{2,2,0,0,111}};return f;}};
	XeenEventSystem events{[&](XeenMapIdentity id){return scripts.at(id);},
		[](XeenMapIdentity id){return XeenEventTextFile{id,"synthetic.txt",true,{"Title","Body words"}};}};
	XeenEventInterpreter interpreter;
	XeenFontFormat font{[] {Bytes b(XeenFontFormat::kMinimumSize);for(int i=0;i<128;++i)b[0x1000+i]=6;return b;}()};
	IndexedFrame base{[] {IndexedFrame b;b.width=320;b.height=200;b.pixels.resize(64000);return b;}()};
	void scriptAt(std::vector<XeenEventRecord> rs,XeenMapIdentity id=1){scripts.insert_or_assign(id,script(id,std::move(rs)));events.discardScriptCache();}
	XeenEventExecutionStepResult begin(int line=0){return interpreter.begin(camera,members,flags,world,
		[&](XeenMapIdentity id){return scripts.at(id);},{},line);}
	XeenEventExecutionStepResult resume(XeenEventExecutionState s){return interpreter.resume(std::move(s),XeenPresentationResponse::Acknowledged,
		members,world,[&](XeenMapIdentity id){return scripts.at(id);},{});}
	XeenEventFlow flow(){return XeenEventFlow(world,events,members,camera,flags,font,[&]{return base;},
		[](IndexedFrame&,std::uint8_t,std::size_t){},[]{return 0;},[]{return 0;});}
};
void error(const XeenEventExecutionStepResult&r,Kind kind){const auto*e=std::get_if<XeenEventExecutionError>(&r);
	check(e && e->kind==kind,"incorrect quest-flag diagnostic");}
void complete(const XeenEventExecutionStepResult&r){check(std::holds_alternative<XeenEventExecutionCompleted>(r),"set did not complete");}

void valuesAndLoading() {
	XeenCloudsQuestFlags empty;
	for(int i=0;i<30;++i){check(!empty.isSet(i) && XeenCloudsQuestFlags::validIndex(i),"default/domain");
		auto copy=empty;copy.set(i);copy.set(i);for(int j=0;j<30;++j)check(copy.isSet(j)==(i==j) && !empty.isSet(j),"independent/idempotent flags");}
	for(std::int64_t i:{-1LL,30LL,255LL,4294967296LL,std::numeric_limits<std::int64_t>::min(),std::numeric_limits<std::int64_t>::max()}){
		check(!XeenCloudsQuestFlags::validIndex(i),"wide index accepted");rejects([&]{empty.isSet(i);});rejects([&]{empty.set(i);});}
	check(empty.values()==XeenCloudsQuestFlags::Values{},"failed set changed flags");
	// Every serialized bit individually: adjacent Darkside and padding bits are ignored.
	for(int bit=0;bit<64;++bit){Bytes b(747,0);b[739+bit/8]=1u<<(bit%8);const auto before=b;
		auto flags=XeenQuestFlagFormat::parseClouds(b);
		for(int i=0;i<30;++i)check(flags.isSet(i)==(i==bit),"packed offset/LSB/domain isolation");
		check(b==before,"parser wrote source");}
	Bytes b(747,255);b[739]=0x85;b[740]=0x42;b[741]=0x18;b[742]=0xe0;
	auto flags=XeenQuestFlagFormat::parseClouds(b);
	for(int i=0;i<30;++i)check(flags.isSet(i)==bool(b[739+i/8]&(1u<<(i%8))),"nonzero mixed field");
	b.resize(850,255);check(XeenQuestFlagFormat::parseClouds(b).values()==flags.values(),"tail affects flag domain");
	for(int n:{0,10,738,739,740,741,742,743,744,745,746})rejects([&]{XeenQuestFlagFormat::parseClouds(Bytes(n));});
	Bytes roster(30*XeenCharacter::kSerializedSize);auto full=resource(2);full[0]=1;full[3]=0; // Existing duplicate/count diagnostics.
	std::copy(b.begin()+739,b.begin()+747,full.begin()+739);full[764]=7;
	const auto loaded=XeenPartyLoader().loadFromResources(roster,full);
	check(loaded.questFlags.values()==flags.values() && loaded.questItems.at(17)==7 && loaded.party.size()==2 && loaded.diagnostics.size()>=2,
		"party loading lost flags/counters/membership diagnostics");
	check(XeenCharacterFormat::parsePartyHeader(Bytes(full.begin(),full.begin()+10)).effectiveCount==2,"header-only contract");
	for(int n:{10,746,747,781})rejects([&]{XeenPartyLoader().loadFromResources(roster,Bytes(full.begin(),full.begin()+n));});
	auto copied=loaded;copied.questFlags.set(1);check(!loaded.questFlags.isSet(1),"party copy aliases flags");
	check(!party().questFlags.isSet(2),"fresh resource loading inherited flags");
}

void validation() {
	for(int n:{1,6})for(int index=0;index<30;++index)for(bool third:{false,true}){
		Fixture f;f.members=party(n);f.members.questItems.increment(17);f.flags.set(2);
		const auto chars=partySnapshot(f.members);const auto items=f.members.questItems.counts();
		auto r=set(0,index);if(third)r.parameters.insert(r.parameters.end(),{0,0});auto again=r;again.line=1;
		f.scriptAt({r,again});auto result=f.begin();complete(result);
		XeenCloudsQuestFlags::Values expected{};expected[index]=true;checkPartyQuestState(f.members,items,expected);
		check(partySnapshot(f.members)==chars && f.flags.isSet(2) && std::get<XeenEventExecutionCompleted>(result).instructionCount==2,
			"set applied per member or changed unrelated state");
	}
	for(const Bytes bytes:std::vector<Bytes>{{0,1,104,2},{20,7,104,2},{104,2,0,0},{21,99,0,0},
		{104,2,104,2},{0,0,104,2,0,1},{0,0,104,2,20,7},{0,0,104,2,104,3},{0,0,20,2,104,3}})for(bool initiallySet:{false,true}){
		Fixture f;if(initiallySet)f.members.questFlags.set(2);const auto before=f.members.questFlags.values();f.scriptAt({record(1,1,0,0x0c,bytes)});
		error(f.begin(),Kind::UnsupportedOperationMode);checkPartyQuestState(f.members,{},before);check(!f.flags.isSet(7),"mixed operation partially applied");}
	for(int index:{30,31,255}){Fixture f;f.scriptAt({set(0,index)});auto r=f.begin();error(r,Kind::InvalidFlagIndex);
		check(std::get<XeenEventExecutionError>(r).message.find("quest flag")!=std::string::npos,"ambiguous flag diagnostic");checkPartyQuestState(f.members,{},{});}
	for(const Bytes bytes:std::vector<Bytes>{{0,0,104},{0,0,104,2,0},{0,0,104,2,0,0,0}}){Fixture f;f.scriptAt({record(1,1,0,12,bytes)});
		error(f.begin(),Kind::MalformedInstruction);checkPartyQuestState(f.members,{},{});}
	for(int fault=0;fault<5;++fault){Fixture f;f.scriptAt({record(1,1,0,9,{44,1,1}),set(1)});
		auto s=std::get<XeenEventExecutionSuspended>(f.begin()).state;auto kind=Kind::UnsupportedExecutionContext;
		if(fault==0)s.logicalAddress.mapId.side=XeenSide::Darkside;
		if(fault==1)s.workingCamera.mapId.side=XeenSide::Darkside;
		if(fault==2){f.members=party(0);kind=Kind::EmptyParty;}
		if(fault==3){s.instructionCount=1024;kind=Kind::InstructionLimitExceeded;}
		if(fault==4){s.logicalAddress.line=254;s.pendingPresentation->conditional->targetLine=255;s.currentScript=script(1,{set(255)});kind=Kind::LineOverflow;}
		error(f.resume(s),kind);checkPartyQuestState(f.members,{},{});}
	Fixture condition;condition.scriptAt({record(1,1,0,9,{104,2,1})});error(condition.begin(),Kind::UnsupportedConditionAction);
}

void lifetimeAndCancellation() {
	Fixture f;f.members=party(6);f.scriptAt({record(1,1,0,0x20,{0,0}),record(1,1,1,0x19,{7,8,0}),
		record(1,1,2,0x12),set(0,2,7,8),record(7,8,1,5,{0,1,9,1,99}),record(7,8,2,0x1a)});
	auto flow=f.flow();flow.handle(InteractionAction{});flow.handle(SelectMemberAction{5});
	check(f.members.questFlags.isSet(2) && flow.blocksGameplay(),"selected member/call lost immediate set");
	f.members.questFlags.set(29); // Independent live mutation while suspended must survive resume.
	f.events.discardScriptCache();f.events.discardTextCache();f.world.discardMapCache();flow.refresh(true);
	flow.handle(AcknowledgeAction{});check(f.members.questFlags.isSet(29) && !flow.blocksGameplay(),"continuation restored old party state");
	flow.handle(InteractionAction{});flow.handle(CancelInteractionAction{});check(f.members.questFlags.isSet(2),"later cancellation erased request");
	XeenEventSystem replacement([&](XeenMapIdentity id){return f.scripts.at(id);});
	XeenEventFlow newFlow(f.world,replacement,f.members,f.camera,f.flags,f.font,[&]{return f.base;});
	check(!newFlow.blocksGameplay() && f.members.questFlags.isSet(2) && f.members.questFlags.isSet(29),"new event owner reset party");
	Fixture cancel;cancel.members=party(2);cancel.scriptAt({record(1,1,0,0x1f,{2,1,1})});
	cancel.scriptAt({record(1,1,0,12,{0,0,20,7}),set(1),record(1,1,2,0x20,{0,0}),set(3,29)},2);
	auto c=cancel.flow();c.handle(InteractionAction{});check(cancel.members.questFlags.isSet(2) && !cancel.flags.isSet(7),"pre-cancel policy");
	c.handle(CancelInteractionAction{});check(cancel.camera.mapId==XeenMapIdentity(2) && cancel.flags.isSet(7) &&
		cancel.members.questFlags.isSet(2) && !cancel.members.questFlags.isSet(29),"WhoWill cancellation commit policy");
	for(int outcome=0;outcome<3;++outcome){Fixture a;a.scriptAt({set(),record(1,1,1,5,{0,1,9,1,2}),set(2,29)});
		{auto pending=a.flow();pending.handle(InteractionAction{});if(outcome==0)pending.abandonPresentation();
			if(outcome==1)pending.respond(*pending.presentationGeneration(),XeenPresentationResponse::Yes);}
		check(a.members.questFlags.isSet(2) && !a.members.questFlags.isSet(29),"abandon/wrong-response/destruction changed authoritative flags");}
	// Original no-root branch's unsigned SP>=0 must not invent an alternate request.
	for(int sp:{0,-1}){Fixture a;a.members.roster.at(0).currentSp=sp;a.scriptAt({record(1,1,0,8,{9,0,2}),record(1,1,1,0xff),set(2)});complete(a.begin());}
}

void errorsAndRemove() {
	for(bool automatic:{false,true})for(int mode=0;mode<3;++mode){Fixture f;
		f.scriptAt({record(1,1,0,0x1f,{2,1,1})});
		f.scriptAt({record(1,1,0,0x19,{7,8,0}),record(1,1,1,0x12),
			record(7,8,0,9,{20,7,5}),record(7,8,1,12,{0,0,20,7}),set(2,2,7,8),
			record(7,8,3,mode==0?0xff:mode==1?9:0x0e,mode==1?Bytes{44,1,4}:Bytes{}),record(7,8,4,0x0e),record(7,8,5,0xff)},2);
		auto flow=f.flow();std::optional<XeenEventExecutionState> suspended;std::optional<Kind> failed;
		auto report=[&](const auto&r){if(auto s=std::get_if<XeenEventExecutionSuspended>(&r))suspended=s->state;
			if(auto e=std::get_if<XeenEventExecutionError>(&r))failed=e->kind;};flow.reportManual=report;flow.reportAutomatic=report;
		automatic?flow.initial():flow.handle(InteractionAction{});
		if(mode==1){check(suspended.has_value(),"missing pre-Remove suspension");suspended->selectedObject=XeenObjectIdentity{2,999};
			flow.abandonPresentation(); // Explicit replacement for this copied fault fixture only.
			if(automatic)flow.acceptAutomatic(f.events.resumeAutomaticEvent(*suspended,XeenPresentationResponse::Acknowledged,f.world,f.members,f.camera,f.flags));
			else flow.acceptManual(f.events.resumeManualEvent(*suspended,XeenPresentationResponse::Acknowledged,f.world,f.members,f.camera,f.flags));}
		check(failed && *failed==(mode==1?Kind::InvalidRemoveContext:Kind::UnsupportedOpcode),"later failure diagnostic");
		check(f.members.questFlags.isSet(2) && !f.flags.isSet(7) && f.camera.mapId==XeenMapIdentity(1),"immediate quest / transactional camera-game flags");
		check(f.world.isObjectDisabled({2,0})==(mode==2) && !f.world.isObjectDisabled({2,1}) &&
			f.world.sessionState().disabledEventCount()==(mode==2?2:0),"Remove persistence/failure scope");}
	Fixture invalid;invalid.scriptAt({set(),set(1,30)});error(invalid.begin(),Kind::InvalidFlagIndex);check(invalid.members.questFlags.isSet(2),"later invalid set rolled back earlier set");
	Fixture target;target.scriptAt({set(),record(1,1,1,0x19,{7,8,9})});error(target.begin(),Kind::InvalidCallTarget);
	check(target.members.questFlags.isSet(2),"bad target rolled back prior set");
}
}
int main(){try{valuesAndLoading();validation();lifetimeAndCancellation();errorsAndRemove();
	std::cout<<"M19B quest flags: loading, bounded set and authoritative lifetime passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
