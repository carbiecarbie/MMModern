#include "XeenRemoveTestSupport.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenQuestItemFormat.h"
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
using Bytes = std::vector<std::uint8_t>;
using Kind = XeenEventExecutionErrorKind;
XeenPartyState party(std::size_t count = 6, bool reversed = false) {
	Bytes roster(XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize);
	Bytes bytes(XeenQuestItemFormat::kRequiredSize);
	std::fill(bytes.begin() + 2, bytes.begin() + 10, 255);
	bytes[0] = bytes[1] = static_cast<std::uint8_t>(count);
	const Bytes ids = reversed ? Bytes{6,1,11,14,18,0} : Bytes{0,18,14,11,1,6};
	for (std::size_t i = 0; i < count; ++i) bytes[2+i] = ids[i];
	auto result = XeenPartyLoader().loadFromResources(roster, bytes);
	for (std::size_t i = 0; i < count; ++i) {
		auto &c = result.roster.at(ids[i]);
		c.name = "Member " + std::to_string(i);
		c.currentSp = static_cast<std::int16_t>(10+i*10);
	}
	return result;
}
Bytes fontBytes() {
	Bytes bytes(XeenFontFormat::kMinimumSize);
	for (int c=0; c<128; ++c) {
		bytes[0x1000+c]=6; bytes[0x1080+c]=3;
		for (int y=0; y<8; ++y) bytes[c*16+y*2]=0x55;
	}
	return bytes;
}
struct Fixture {
	XeenPartyState members = party();
	std::map<XeenMapIdentity,XeenEventScript> scripts;
	XeenEventTextFile text{1,"synthetic.txt",true,{"Fixture", "Message"}};
	int textLoads = 0;
	bool automaticCell = true;
	XeenWorld world{[&](XeenMapIdentity id) { auto m=map(id);m.geometry.cells[17].rawAttributes=automaticCell?0x10:0;return m; },
		[](XeenMapIdentity id) { XeenObjectFile f{id,"synthetic.mob",true,{}};
			f.entities.objects={{1,1,0,0,111}};return f; }};
	XeenEventSystem events{[&](XeenMapIdentity id) { return scripts.at(id); },
		[&](XeenMapIdentity) { ++textLoads; return text; }};
	XeenEventInterpreter interpreter;
	XeenCamera camera{1,1,1,XeenDirection::North};
	XeenGameFlags flags;
	XeenFontFormat font{fontBytes()};
	IndexedFrame base;
	Fixture() { base.width=320;base.height=200;base.pixels.resize(64000); }
	void set(std::vector<XeenEventRecord> records, XeenMapIdentity id=1) {
		scripts.insert_or_assign(id,script(id,std::move(records)));
	}
	XeenEventExecutionStepResult begin(int line=0) {
		return interpreter.begin(camera,members,flags,world,
			[&](XeenMapIdentity id) { return scripts.at(id); },
			[&](XeenMapIdentity) { ++textLoads;return text; },static_cast<std::uint8_t>(line));
	}
	XeenEventExecutionStepResult resume(XeenEventExecutionState state, XeenPresentationResponse response) {
		return interpreter.resume(std::move(state),response,members,world,
			[&](XeenMapIdentity id) { return scripts.at(id); },
			[&](XeenMapIdentity) { ++textLoads;return text; });
	}
};
XeenEventExecutionState pending(const XeenEventExecutionStepResult &r) {
	check(std::holds_alternative<XeenEventExecutionSuspended>(r),"expected suspension");
	return std::get<XeenEventExecutionSuspended>(r).state;
}
void complete(const XeenEventExecutionStepResult &r) {
	check(std::holds_alternative<XeenEventExecutionCompleted>(r),"expected completion");
}
void error(const XeenEventExecutionStepResult &r, Kind kind) {
	const auto *e=std::get_if<XeenEventExecutionError>(&r);
	check(e && e->kind==kind,"unexpected WhoWill error");
}

void decoderAndCardinality() {
	const auto r=record(1,2,3,0x20,{0,3},4,77);
	const auto decoded=XeenEventDecoder::decode(r,{20,"fixture.evt",4});
	const auto &d=std::get<XeenDecodedEventInstruction>(decoded);
	const auto op=std::get<XeenEventWhoWill>(d.operation);
	check(op.verbIndex==0 && op.textIndex==3 && d.source.fileOffset==77 &&
		d.source.mapId==20 && d.source.recordIndex==4 && d.source.line==3 &&
		d.source.x==1 && d.source.y==2 && d.source.direction==4 &&
		d.source.opcode==0x20 && d.source.resourceName=="fixture.evt","A01 source/operands");
	for (const Bytes bytes : std::vector<Bytes>{{},{0},{0,3,0}}) {
		const auto result=XeenEventDecoder::decode(record(1,2,3,0x20,bytes,4,77),{20,"fixture.evt",4});
		const auto &e=std::get<XeenEventDecodeError>(result);
		check(e.kind==XeenEventDecodeErrorKind::MalformedInstruction && e.expectedParameterSize==2 &&
			e.actualParameterSize==bytes.size() && e.source.fileOffset==77 && e.source.recordIndex==4,"A02 malformed metadata");
		for (int count : {0,1,6}) {
			Fixture f;f.members=party(count);f.set({record(1,1,0,0x20,bytes),record(1,1,1,0x0c,{0,0,21,100})});
			error(f.begin(),Kind::MalformedInstruction);check(f.members.questItems.at(18)==0,"malformed ran grant");
		}
	}
	Fixture empty;empty.members={};empty.set({record(1,1,0,0x20,{0,0})});
	error(empty.begin(),Kind::EmptyParty);check(empty.textLoads==0,"empty read text");
	empty.set({record(1,1,0,0x12)});complete(empty.begin());
	for (bool incapacitated : {false,true}) {
		Fixture f;f.members=party(1);f.members.roster.at(0).conditions[15]=incapacitated;
		f.set({record(1,1,0,0x20,{255,255}),record(1,1,1,9,{9,10,3}),record(1,1,2,0xff),record(1,1,3,0x12)});
		complete(f.begin());check(f.textLoads==0,"one member consulted unused text/verb");
	}
	Fixture invalid;invalid.set({record(1,1,0,0x20,{32,0})});
	error(invalid.begin(),Kind::UnsupportedOperand);check(invalid.textLoads==0,"bad verb read text");
	for (int mode=0;mode<4;++mode) {
		Fixture f;f.set({record(1,1,0,0x20,{0,0})});
		if(mode==0)f.text.resourcePresent=false;
		if(mode==1)f.text.mapId=2;
		if(mode==2)f.text.strings.clear();
		if(mode==3)f.text.strings={""};
		const auto result=f.begin();
		if(mode==3)check(pending(result).pendingPresentation->request.text.empty(),"empty text rejected");
		else error(result,mode==0?Kind::MissingTextResource:mode==1?Kind::TextMapMismatch:Kind::InvalidTextIndex);
	}
}

void eligibilityAndProtocol() {
	for (int condition=0;condition<=16;++condition) {
		XeenCharacter c;if(condition<16)c.conditions[condition]=1;
		const bool eligible=condition!=8 && (condition<11 || condition>15);
		check(c.canAct()==eligible,"worst condition helper");
	}
	XeenCharacter c;c.conditions[8]=1;c.conditions[10]=1;
	check(c.canAct(),"Asleep+Confused precedence");
	for (int count : {2,3,4,5,6}) for (int index=0;index<count;++index) {
		Fixture f;f.members=party(count);
		f.set({record(1,1,0,0x20,{0,0}),record(1,1,1,9,{9,static_cast<std::uint8_t>(10+10*index),3}),
			record(1,1,2,0xff),record(1,1,3,0x12)});
		auto state=pending(f.begin());
		check(state.activeCharacterIndex==0 && state.pendingPresentation->request.members.size()==count,"initial context/metadata");
		complete(f.resume(state,SelectedCharacter{static_cast<std::size_t>(index)}));
		for (auto response : {XeenPresentationResponse{XeenPresentationResponse::Yes},
			XeenPresentationResponse{XeenPresentationResponse::No},XeenPresentationResponse{XeenPresentationResponse::Acknowledged},
			XeenPresentationResponse{XeenPresentationResponse::Presented},XeenPresentationResponse{SelectedCharacter{99}}})
			error(f.resume(state,response),Kind::InvalidPresentationResponse);
		complete(f.resume(state,CharacterSelectionCancelled{}));
	}
	Fixture f;f.set({record(1,1,0,0x20,{0,0}),record(1,1,1,0xff)});
	for (int condition : {8,11,12,13,14,15}) {
		f.members.roster.at(18).conditions={};
		f.members.roster.at(18).conditions[condition]=1;
		auto refused=pending(f.resume(pending(f.begin()),SelectedCharacter{1}));
		check(!refused.pendingPresentation->request.refusal.empty() && refused.instructionCount==1,"blocking condition failed to refuse/retry");
		complete(f.resume(refused,CharacterSelectionCancelled{}));
	}
	f.members.roster.at(18).conditions={};
	f.members.roster.at(18).conditions[8]=1;f.members.roster.at(18).conditions[10]=1;
	error(f.resume(pending(f.begin()),SelectedCharacter{1}),Kind::UnsupportedOpcode);
	auto state=pending(f.begin());
	for(auto id:f.members.party.activeRosterIds())f.members.roster.at(id).conditions[15]=1;
	for(int index=0;index<6;++index) {
		state=pending(f.resume(state,SelectedCharacter{static_cast<std::size_t>(index)}));
		check(state.activeCharacterIndex==0 && state.instructionCount==1 &&
			state.pendingPresentation->request.refusal.find("Member "+std::to_string(index))!=std::string::npos,"refusal/retry");
	}
	complete(f.resume(state,CharacterSelectionCancelled{}));
	f.members.roster.at(18).conditions={};
	error(f.resume(state,SelectedCharacter{1}),Kind::UnsupportedOpcode); // live recovery accepted
	for(int mutation=0;mutation<3;++mutation) {
		Fixture changed;changed.set({record(1,1,0,0x20,{0,0}),record(1,1,1,0x0c,{0,0,21,100})});
		auto old=pending(changed.begin());
		changed.members=mutation==0?party(2):mutation==1?party(6,true):XeenPartyState{};
		error(changed.resume(old,SelectedCharacter{0}),Kind::InvalidPresentationResponse);
		check(changed.members.questItems.at(18)==0,"changed party ran grant");
	}
}

void contextLifetime() {
	Fixture preceding;
	preceding.set({record(1,1,0,0x01,{1}),record(1,1,1,0x20,{0,0}),
		record(1,1,2,0x19,{7,8,0}),record(1,1,3,9,{9,20,5}),record(1,1,4,0xff),record(1,1,5,0x12),
		record(7,8,0,9,{9,60,2}),record(7,8,1,0xff),record(7,8,2,0x20,{0,0}),record(7,8,3,0x1a)});
	auto first=pending(preceding.resume(pending(preceding.begin()),XeenPresentationResponse::Presented));
	check(first.activeCharacterIndex==0,"display before choice lost default context");
	auto called=pending(preceding.resume(first,SelectedCharacter{5}));
	check(called.activeCharacterIndex==5 && called.callStack.size()==1,"callee did not see prior choice");
	complete(preceding.resume(called,SelectedCharacter{1}));
	for (int opcode : {8,9,10}) for (bool called : {false,true}) {
		Fixture f;
		std::vector<XeenEventRecord> records{
			record(1,1,0,9,{9,10,2}),record(1,1,1,0xff),
			record(1,1,2,called?0x19:0x20,called?Bytes{7,8,0}:Bytes{0,0}),
			record(1,1,3,0x01,{1}),record(1,1,4,opcode,{9,20,6}),record(1,1,5,0xff),
			record(1,1,6,0x12),record(7,8,0,0x20,{0,0}),record(7,8,1,0x1a)};
		f.set(records);auto state=pending(f.begin());
		state=pending(f.resume(state,SelectedCharacter{1}));
		check(state.activeCharacterIndex==1 && state.callStack.empty(),"selection lost through return/display");
		complete(f.resume(state,XeenPresentationResponse::Presented));
		check(pending(f.begin()).activeCharacterIndex==0,"new dispatch leaked selection");
	}
	Fixture beforeCall;beforeCall.set({record(1,1,0,0x20,{0,0}),record(1,1,1,0x19,{7,8,0}),
		record(1,1,2,9,{9,20,4}),record(1,1,3,0xff),record(1,1,4,0x12),
		record(7,8,0,9,{9,20,2}),record(7,8,1,0xff),record(7,8,2,0x1a)});
	complete(beforeCall.resume(pending(beforeCall.begin()),SelectedCharacter{1}));
	Fixture t;t.set({record(1,1,0,0x20,{0,0}),record(1,1,1,0x1f,{2,1,1})});
	t.set({record(1,1,0,9,{9,10,2}),record(1,1,1,0xff),record(1,1,2,0x12)},2);
	const auto done=t.resume(pending(t.begin()),SelectedCharacter{5});complete(done);
	check(std::get<XeenEventExecutionCompleted>(done).instructionCount==4,"teleport budget reset");
	Fixture neg;neg.members.roster.at(18).currentSp=-1;
	neg.set({record(1,1,0,0x20,{0,0}),record(1,1,1,8,{9,255,3}),record(1,1,2,0xff),record(1,1,3,0x12)});
	complete(neg.resume(pending(neg.begin()),SelectedCharacter{1}));
	Fixture limit;limit.set({record(1,1,255,0x20,{0,0})});error(limit.begin(255),Kind::LineOverflow);
	Fixture acknowledgment;acknowledgment.set({record(1,1,0,9,{44,1,1}),record(1,1,1,0xff)});
	for (auto response : {XeenPresentationResponse{SelectedCharacter{0}},XeenPresentationResponse{CharacterSelectionCancelled{}}})
		error(acknowledgment.resume(pending(acknowledgment.begin()),response),Kind::InvalidPresentationResponse);
	limit.set({record(1,1,0,0x20,{0,0}),record(1,1,1,0x12)});
	auto state=pending(limit.begin());state.instructionCount=XeenEventInterpreter::kMaximumInstructions;
	error(limit.resume(state,SelectedCharacter{0}),Kind::InstructionLimitExceeded);
}

void integratedTextErrors() {
	for (bool mismatch : {false,true}) for (int opcode : {0x20,0x04}) {
		Fixture f;
		f.set({record(1,1,0,opcode,opcode==0x20?Bytes{0,0}:Bytes{0}),
			record(1,1,1,0x0c,{0,0,21,100}),record(1,1,2,0x0e)});
		f.text.mapId=mismatch?2:1;f.text.resourcePresent=mismatch;
		for (int attempt=0;attempt<2;++attempt) {
			const auto result=f.events.runManualEvent(f.world,f.members,f.camera,f.flags);
			const auto *e=std::get_if<XeenEventExecutionError>(&result);
			check(e && e->kind==(mismatch?Kind::TextMapMismatch:Kind::MissingTextResource),
				"integrated text diagnostic lost");
			check(e->instructionCount==1 && e->source && e->source->line==0 &&
				f.members.questItems.at(18)==0 && !f.world.isObjectDisabled({1,0}),
				"text error executed subsequent instruction");
		}
		if(mismatch)check(f.events.cachedTextCount()==0 && f.textLoads==2,"mismatched text cached");
	}
}

void directResponses() {
	for(int mode=0;mode<5;++mode) {
		Fixture direct, input;
		const auto records=std::vector<XeenEventRecord>{record(1,1,0,0x04,{0}),
			mode==3?record(1,1,1,0x31,{1,0}):mode==4?record(1,1,1,0x20,{0,1}):
				record(1,1,1,9,{44,static_cast<std::uint8_t>(mode==2?1:0),2}),
			record(1,1,2,0x12)};
		direct.set(records);input.set(records);
		XeenEventFlow a(direct.world,direct.events,direct.members,direct.camera,direct.flags,direct.font,[&](std::uint64_t){return XeenEventFlow::Composition{direct.base, false};});
		XeenEventFlow b(input.world,input.events,input.members,input.camera,input.flags,input.font,[&](std::uint64_t){return XeenEventFlow::Composition{input.base, false};});
		bool completed=false;
		a.reportManual=[&](const auto &result) {
			check(!std::holds_alternative<XeenEventExecutionError>(result),"direct response execution failed");
			completed=std::holds_alternative<XeenManualEventCompleted>(result);
		};
		XeenEventPresenter retained(direct.font);
		const auto sign=retained.present(direct.base,pending(direct.begin()).pendingPresentation->request).frame;
		a.handle(InteractionAction{});b.handle(InteractionAction{});
		const auto before=a.frame();const auto generation=*a.presentationGeneration();
		const XeenPresentationResponse response=mode==0?XeenPresentationResponse{XeenPresentationResponse::Yes}:
			mode==1?XeenPresentationResponse{XeenPresentationResponse::No}:mode==4?XeenPresentationResponse{SelectedCharacter{1}}:
			XeenPresentationResponse{XeenPresentationResponse::Acknowledged};
		const PlayerAction action=mode==0?PlayerAction{YesAction{}}:mode==1?PlayerAction{NoAction{}}:
			mode==4?PlayerAction{SelectMemberAction{1}}:PlayerAction{AcknowledgeAction{}};
		check(a.respond(generation,response),"direct response rejected");b.handle(action);
		check(completed && !a.blocksGameplay() && !b.blocksGameplay() && !a.respond(generation,response),"direct completion/replay");
		check(a.frame().pixels==b.frame().pixels,"direct response left different presentation");
		check(mode==3?a.frame().pixels==before.pixels:a.frame().pixels!=before.pixels,"consumed layer retention");
		check(a.frame().pixels!=direct.base.pixels,"passive text lost on response");
		if(mode!=3)check(a.frame().pixels==sign.pixels,"consumed UI did not restore exact retained sign");
		direct.base.pixels.assign(64000,3);input.base=direct.base;
		check(a.refresh(true).pixels==b.refresh(true).pixels,"direct response rebase restored consumed layer");
	}
}

void passiveSelectionInputs() {
	Fixture f;f.set({record(1,1,0,0x04,{0}),record(1,1,1,0x12)});
	int compositions=0;
	XeenEventFlow flow(f.world,f.events,f.members,f.camera,f.flags,f.font,[&](std::uint64_t){++compositions;return XeenEventFlow::Composition{f.base, false};});
	flow.handle(InteractionAction{});const auto frame=flow.frame();const auto count=compositions;
	check(!flow.blocksGameplay() && frame.pixels!=f.base.pixels,"passive fixture missing label");
	for(std::size_t index=0;index<6;++index) {
		check(flow.handle(SelectMemberAction{index}).pixels==frame.pixels && compositions==count &&
			!flow.presentationGeneration(),"F1-F6 mutated passive presentation");
	}
	check(flow.refresh(true).pixels==frame.pixels,"F1-F6 lost retained label");
}

void productionFlow() {
	for(bool automatic : {false,true}) {
		Fixture f;f.text.strings[1]=std::string(1200,'A');
		f.set({record(1,1,0,0x20,{0,0}),record(1,1,1,0x01,{1}),record(1,1,2,9,{44,1,3}),
			record(1,1,3,9,{9,20,5}),record(1,1,4,0xff),record(1,1,5,0x0c,{0,0,21,100}),record(1,1,6,0x12)});
		XeenEventFlow flow(f.world,f.events,f.members,f.camera,f.flags,f.font,[&](std::uint64_t){return XeenEventFlow::Composition{f.base, false};});
		int line=-1;bool done=false;
		auto report=[&](const auto &r) {
			check(!std::holds_alternative<XeenEventExecutionError>(r),"production flow failed");
			if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&r))line=s->request.source.line;
			else done=true;
		};
		flow.reportManual=report;flow.reportAutomatic=report;
		automatic?flow.initial():flow.handle(InteractionAction{});
		const auto generation=*flow.presentationGeneration();const auto frame=flow.frame();
		for(const PlayerAction action : {PlayerAction{SelectMemberAction{6}},PlayerAction{AcknowledgeAction{}},
			PlayerAction{InteractionAction{}},PlayerAction{YesAction{}},PlayerAction{NoAction{}},PlayerAction{NavigationAction::TurnRight},
			PlayerAction{EquipmentInventoryAction{}}})
			flow.handle(action);
		check(flow.presentationGeneration()==generation && flow.frame().pixels==frame.pixels &&
			f.camera.direction==XeenDirection::North && flow.canCancelInteraction(),"ignored choice inputs");
		f.world.discardMapCache();f.events.discardScriptCache();f.events.discardTextCache();
		check(flow.refresh(true).pixels==frame.pixels && flow.presentationGeneration()==generation,"rebase changed generation/frame");
		f.camera.direction=XeenDirection::East;flow.refresh(true);
		check(flow.presentationGeneration()==generation && flow.canCancelInteraction(),"camera rebase discarded pending selection");
		f.camera.direction=XeenDirection::North;
		check(flow.refresh(true).pixels==frame.pixels,"camera rebase lost request");
		f.members.roster.at(18).conditions[8]=1;
		flow.handle(SelectMemberAction{1});
		check(flow.canCancelInteraction() && *flow.presentationGeneration()!=generation,"refusal lost choice");
		const auto refusal=flow.frame();check(refusal.pixels!=frame.pixels,"refusal invisible");
		check(flow.refresh(true).pixels==refusal.pixels,"refusal rebase changed");
		check(!flow.respond(generation,SelectedCharacter{1}),"stale response accepted");
		f.members.roster.at(18).conditions={};const auto retry=*flow.presentationGeneration();
		flow.handle(SelectMemberAction{1});
		check(!flow.respond(retry,SelectedCharacter{1}) && line==1 && !flow.canCancelInteraction(),"response replay/selection acknowledged next page");
		const auto page=flow.frame();flow.handle(SelectMemberAction{1});check(flow.frame().pixels==page.pixels,"F-key acknowledged");
		int pages=0;while(line==1 && ++pages<100)flow.handle(AcknowledgeAction{});
		check(pages>1 && pages<100 && line==2,"pagination failed");
		flow.handle(AcknowledgeAction{});
		check(done && !flow.blocksGameplay() && f.members.questItems.at(18)==1,"selected SP or grant count");
		check(!flow.respond(retry,SelectedCharacter{1}),"completed response replay");
		flow.handle(NavigationAction::TurnRight);check(f.camera.direction==XeenDirection::East,"navigation did not recover");
	}
	for(int mode=0;mode<4;++mode) {
		Fixture f;f.set({record(1,1,0,0x20,{0,0}),record(1,1,1,0x0c,{0,0,21,100})});
		XeenEventFlow flow(f.world,f.events,f.members,f.camera,f.flags,f.font,[&](std::uint64_t){return XeenEventFlow::Composition{f.base, false};});
		bool failed=false;flow.reportManual=[&](const auto &r){failed=std::holds_alternative<XeenEventExecutionError>(r);};
		flow.handle(InteractionAction{});const auto old=*flow.presentationGeneration();
		if(mode==0)flow.handle(CancelInteractionAction{});
		if(mode==1)flow.respond(old,SelectedCharacter{99});
		if(mode==2){f.members=party(6,true);flow.handle(SelectMemberAction{0});}
		if(mode==3){flow.abandonPresentation();flow.acceptManual(f.events.runManualEvent(f.world,f.members,f.camera,f.flags));
			check(!flow.respond(old,SelectedCharacter{0}),"replaced generation accepted");flow.handle(CancelInteractionAction{});}
		check(!flow.blocksGameplay() && flow.frame().pixels==f.base.pixels &&
			failed==(mode==1 || mode==2) && f.members.questItems.at(18)==0 && !flow.respond(old,SelectedCharacter{0}),"cancel/error/replacement policy");
	}
}

void presentationLayout() {
	Fixture f;f.set({record(1,1,0,0x20,{0,0})});
	const auto request=pending(f.begin()).pendingPresentation->request;
	for (int verb=0;verb<32;++verb) {
		auto shortRequest=request;shortRequest.verbIndex=static_cast<std::uint8_t>(verb);
		XeenEventPresenter shortPresenter(f.font);
		const auto normal=shortPresenter.present(f.base,shortRequest);
		auto longRequest=shortRequest;longRequest.text=std::string(300,'A');
		XeenEventPresenter longPresenter(f.font);
		const auto clipped=longPresenter.present(f.base,longRequest);
		check(!normal.response && !clipped.response && shortPresenter.diagnostics().empty() &&
			longPresenter.diagnostics().empty(),"WhoWill layout produced response/diagnostic");
		bool footerVisible=false;
		for(int y=0;y<200;++y)for(int x=0;x<320;++x) {
			const auto offset=static_cast<std::size_t>(y*320+x);
			if(x<225 || x>=320 || y<74 || y>=154)
				check(clipped.frame.pixels[offset]==f.base.pixels[offset],"WhoWill drew outside panel");
			if(x>=233 && x<312 && y>=104 && y<146)
				check(normal.frame.pixels[offset]==clipped.frame.pixels[offset],"long title displaced question/keys");
			if(x>=233 && x<312 && y>=138 && y<146 && normal.frame.pixels[offset]!=normal.frame.pixels[138*320+233])
				footerVisible=true;
		}
		check(footerVisible,"WhoWill F-key label clipped");
	}
}

void priorEffects() {
	for(bool cancel : {false,true}) for(bool remove : {false,true}) {
		Fixture f;
		f.set({record(1,1,0,0x1f,{2,1,1})});
		f.set({record(1,1,0,0x19,{7,8,0}),record(1,1,1,0xff),
			record(7,8,0,9,{21,100,5}),record(7,8,1,0x0c,{0,0,20,7}),
			record(7,8,2,0x0c,{0,0,21,100}),record(7,8,3,remove?0x0e:0),
			record(7,8,4,0),record(7,8,5,0x20,{0,0}),record(7,8,6,0x0c,{0,0,21,100})},2);
		f.text.mapId=2;
		XeenEventFlow flow(f.world,f.events,f.members,f.camera,f.flags,f.font,[&](std::uint64_t){return XeenEventFlow::Composition{f.base, false};});
		bool failed=false;flow.reportManual=[&](const auto &r){failed=std::holds_alternative<XeenEventExecutionError>(r);};
		flow.handle(InteractionAction{});
		check(f.camera.mapId==1 && !f.flags.isSet(7) && f.members.questItems.at(18)==1,"suspension committed transaction/lost grant");
		const auto generation=*flow.presentationGeneration();
		if(!cancel)check(!flow.respond(generation,XeenPresentationResponse::Yes)&&flow.presentationGeneration()==generation,
			"wrong-kind WhoWill response changed generation");
		flow.respond(generation,cancel?XeenPresentationResponse{CharacterSelectionCancelled{}}:
			XeenPresentationResponse{SelectedCharacter{99}});
		check(!flow.blocksGameplay() && failed!=cancel && f.camera.mapId==(cancel?2:1) &&
			f.flags.isSet(7)==cancel && f.members.questItems.at(18)==1,"cancel/error transaction policy");
		check(f.world.isObjectDisabled({2,0})==remove && f.world.sessionState().disabledEventCount()==(remove?2:0),"prior Remove rolled back");
	}
}

void sdlFlow() {
	for (int mode=0; mode<3; ++mode) {
		std::cerr << "SDL scenario " << mode << '\n';
		Fixture f;f.automaticCell=false;f.members=party(2);f.members.roster.at(0).conditions[8]=1;
		f.set({record(1,1,0,0x20,{0,0}),record(1,1,1,9,{44,1,2}),
			record(1,1,2,9,{9,20,4}),record(1,1,3,0xff),record(1,1,4,0x0c,{0,0,21,100})});
		XeenEventFlow flow(f.world,f.events,f.members,f.camera,f.flags,f.font,[&](std::uint64_t){return XeenEventFlow::Composition{f.base, false};});
		flow.reportManual=[](const auto &r) { check(!std::holds_alternative<XeenEventExecutionError>(r),"SDL flow execution error"); };
		std::vector<SDL_Keycode> keys{SDLK_SPACE,SDLK_UP,SDLK_RETURN,SDLK_y,SDLK_n,SDLK_F6,SDLK_F1};
		if (mode==0) keys.insert(keys.end(),{SDLK_F2,SDLK_RETURN,SDLK_RIGHT});
		if (mode==1) keys.insert(keys.end(),{SDLK_ESCAPE,SDLK_RIGHT});
		std::atomic<bool> finished{false};std::exception_ptr senderError;
		std::thread sender([&] {
			try {
				auto push=[&](SDL_Event e) {
					for(int attempt=0;attempt<100 && !finished;++attempt) {
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						if(SDL_PushEvent(&e)==1)return;
					}
					if(!finished)throw std::runtime_error("SDL queue unavailable");
				};
				for(auto key:keys) {
					SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;
					push(e);e.key.repeat=1;push(e);
					e.type=SDL_KEYUP;e.key.repeat=0;push(e);
				}
				SDL_Event exit{};exit.type=mode==2?SDL_QUIT:SDL_KEYDOWN;exit.key.keysym.sym=SDLK_ESCAPE;push(exit);
			} catch(...) { senderError=std::current_exception();SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e); }
		});
		std::size_t calls=0;
		const bool ok=SdlWindow().showInteractive(flow.frame(),"18A synthetic input",
			[&](const PlayerAction &action)->std::optional<IndexedFrame> {
				++calls;const auto frame=flow.handle(action);
				if(calls<=7)check(flow.canCancelInteraction() && f.camera.direction==XeenDirection::North,"SDL ignored input escaped choice/refusal");
				if(mode==0 && calls==8)check(flow.blocksGameplay() && !flow.canCancelInteraction(),"F2 acknowledged next request");
				return frame;
			},[&]{return flow.canCancelInteraction();});
		finished=true;sender.join();if(senderError)std::rethrow_exception(senderError);
		check(ok && calls==keys.size(),"SDL repeats/cancellation delivery");
		check(f.members.questItems.at(18)==(mode==0?1:0),"SDL selected SP or cancellation effects");
		check(flow.blocksGameplay()==(mode==2) && f.camera.direction==(mode==2?XeenDirection::North:XeenDirection::East),"SDL quit/navigation recovery");
	}
}
}
int main(int argc, char **argv) {
	try {
		if(argc==2 && std::string(argv[1])=="text-errors") { integratedTextErrors();return 0; }
		if(argc==2 && std::string(argv[1])=="direct-responses") { directResponses();return 0; }
		if(argc==2 && std::string(argv[1])=="passive-inputs") { passiveSelectionInputs();return 0; }
		if(argc==2 && std::string(argv[1])=="sdl") { sdlFlow();std::cout<<"18A integrated SDL passed\n";return 0; }
		integratedTextErrors();directResponses();passiveSelectionInputs();
		decoderAndCardinality();eligibilityAndProtocol();contextLifetime();productionFlow();presentationLayout();priorEffects();
		std::cout<<"18A decoder, cardinality, eligibility, selected SP, continuations and production generations passed\n";
		return 0;
	} catch(const std::exception &e) { std::cerr<<e.what()<<'\n';return 1; }
}
