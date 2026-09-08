#include "XeenRewardTestSupport.h"
#include <iostream>
using namespace reward_test;
void terminals(){
	for(int terminal=0;terminal<5;++terminal){Execution e;
		std::vector<XeenEventRecord> records{pause()};
		if(terminal==0)records.push_back(record(1,1,1,0x12));
		if(terminal==2)records.push_back(record(1,1,1,0x07,{2,2,2}));
		if(terminal==3)records.push_back(record(1,1,1,0x20,{0,1}));
		if(terminal==4)records.push_back(record(1,1,1,0x31,{1,0}));
		auto s=e.seed(records);s.workingGameFlags.set(7);
		s.preferredRewardRecipient=1;
		auto r=e.resume(s);
		if(terminal==3)r=e.resume(pending(r),CharacterSelectionCancelled{});
		if(terminal==4)r=e.resume(pending(r),XeenPresentationResponse::Acknowledged);
		s=pending(r);
		check(s.rewardPhase==XeenRewardPhase::Receipt&&!s.pendingRewards.hasWork()&&s.rewardReceipt.delivered==1,"successful terminal omitted delivery");
		check(s.rewardReceipt.entries[0].owner==1&&s.preferredRewardRecipient==1,"cancel changed preference");
		check(e.f.initial.roster.at(1).miscellaneous[0].id==37&&!e.flags.isSet(7),"insertion before receipt ACK");
		const auto count=s.instructionCount;auto done=e.resume(s);
		const auto &c=std::get<XeenEventExecutionCompleted>(done);
		check(c.instructionCount==count&&c.finalGameFlags.isSet(7),"ACK counted instruction or lost working flags");
		check(c.finalCamera.mapId==XeenMapIdentity(terminal==2?2:1),"teleport terminal");
	}
}
void continuations(){
	Execution e;
	auto s=e.seed({pause(),record(1,1,1,0x19,{2,2,0}),pause(2),record(1,1,3,0x12),
		record(2,2,0,0x1a)});
	auto r=e.resume(s);s=pending(r);
	check(s.pendingRewards.size()==1&&s.rewardPhase==XeenRewardPhase::Running&&s.callStack.empty()&&s.instructionCount==4,"Call/Return delivered early or lost queue");
	s=pending(e.resume(s));check(s.rewardReceipt.delivered==1&&s.instructionCount==5,"Call/Return finalization count");
	for(auto bad:std::vector<XeenEventRecord>{record(1,1,1,0x1a),record(1,1,1,0x19,{99,99,0}),
		record(1,1,1,9,{20,7,99}),record(1,1,1,0x2c,{69,37,0,1}),record(1,1,1,12,{21,81,0,0}),
		record(1,1,1,12,{104,2,0,1})}){
		Execution x;auto state=x.seed({pause(),bad});state.workingGameFlags.set(7);
		auto result=x.resume(state);const auto &error=std::get<XeenEventExecutionError>(result);
		check(error.rewards.discarded==1&&error.rewards.discardReason==XeenRewardDiscard::ExecutionError&&
			!x.f.initial.roster.at(0).miscellaneous[0].id&&!x.flags.isSet(7),"error delivered or lost accounting");
	}
	Execution who;auto state=who.seed({record(1,1,0,0x20,{0,1}),record(1,1,1,0x12)});
	check(!state.preferredRewardRecipient&&state.activeCharacterIndex==0,"initial reward preference present");
	state=pending(who.resume(state,SelectedCharacter{1}));check(state.rewardReceipt.entries[0].owner==1,"WhoWill reward preference");
	Execution single;single.f.initial.party=XeenParty::fromRosterIds({0});
	state=single.seed({pause(),record(1,1,1,0x20,{0,1}),record(1,1,2,0x12)});state.preferredRewardRecipient=5;
	state=pending(single.resume(state));check(state.preferredRewardRecipient==5&&state.rewardReceipt.entries[0].owner==0,"single WhoWill overwrote preference");
	Execution reordered;state=reordered.seed({record(1,1,0,0x20,{0,1}),pause(1),record(1,1,2,0x12)});
	state=pending(reordered.resume(state,SelectedCharacter{1}));
	reordered.f.initial.party=XeenParty::fromRosterIds({1,0,1});
	state=pending(reordered.resume(state));check(state.rewardReceipt.entries[0].owner==0,"preferred index did not resolve current membership");
	Execution immediate;state=immediate.seed({pause(),record(1,1,1,12,{0,0,21,99}),record(1,1,2,12,{0,0,104,2}),record(1,1,3,0xff)});
	const auto failed=std::get<XeenEventExecutionError>(immediate.resume(state));
	check(failed.rewards.discarded==1&&immediate.f.initial.questItems.at(17)==1&&immediate.f.initial.questFlags.isSet(2),"pre-delivery error rolled back prior immediate grants");
}
XeenEventExecutionStepResult begin(Execution &e,std::vector<XeenEventRecord> records) {
	e.f.scripts[1]=std::move(records);
	return e.interpreter.begin(e.camera,e.f.initial,e.flags,e.world,e.scripts,e.texts);
}
void production(){
	for(int n : {1,5,11}) {
		Execution e;std::vector<XeenEventRecord> records;
		for(int i=0;i<n;++i) records.push_back(record(1,1,i,0x2c,
			{static_cast<std::uint8_t>(70+i%2),static_cast<std::uint8_t>(1+i),255,193}));
		records.push_back(pause(n));
		auto s=pending(begin(e,records));
		check(s.instructionCount==n+1 && s.pendingRewards.size()==std::min(n,10) && s.pendingRewards.overflow()==(n==11),"real production queue/overflow");
		for(int i=0;i<std::min(n,10);++i) {
			const auto item=s.pendingRewards.at(i);
			check(item.material==10+i%2 && item.id==1+i && item.state==1 && item.frame==0,"deterministic fields/order/suffix");
		}
		check(!e.f.initial.roster.at(0).miscellaneous[0].id,"production delivered before finalization");
		s=pending(e.resume(s));check(s.rewardReceipt.delivered==std::min(n,10) && s.rewardReceipt.overflow==(n==11),"real natural finalizer");
		const auto inventory=xeenInventoryInspection(e.f.initial);
		const auto done=std::get<XeenEventExecutionCompleted>(e.resume(s));
		check(done.instructionCount==n+1 && xeenInventoryInspection(e.f.initial)==inventory,"receipt ACK replayed/count changed");
	}
	// Fullness must not bypass decoder errors; failed decoding adds no dispatch.
	for(const auto &bytes : std::vector<Bytes>{{70},{70,37,1,2,3},{69,37},{70,0},{71,74}}) {
		Execution e;std::vector<XeenEventRecord> records;
		for(int i=0;i<10;++i)records.push_back(record(1,1,i,0x2c,{70,37}));
		records.push_back(record(1,1,10,0x2c,bytes));
		const auto failure=std::get<XeenEventExecutionError>(begin(e,records));
		check(failure.kind==(bytes.size()<2 || bytes.size()>4?XeenEventExecutionErrorKind::MalformedInstruction:XeenEventExecutionErrorKind::UnsupportedOperand) &&
			failure.instructionCount==10 && failure.source && failure.source->line==10 && failure.rewards.discarded==10 && failure.rewards.overflow==0 &&
			!e.f.initial.roster.at(0).miscellaneous[0].id,"full queue operand validation/accounting");
	}
	for(int fault=0;fault<5;++fault) {
		Execution e;auto s=pending(begin(e,{pause(),record(1,1,1,0x2c,{70,37})}));
		auto kind=XeenEventExecutionErrorKind::UnsupportedExecutionContext;
		if(fault==0)s.logicalAddress.mapId.side=XeenSide::Darkside;
		if(fault==1)s.workingCamera.mapId.side=XeenSide::Darkside;
		if(fault==2){e.f.initial.party=XeenParty::fromRosterIds({});kind=XeenEventExecutionErrorKind::EmptyParty;}
		if(fault==3){s.instructionCount=1024;kind=XeenEventExecutionErrorKind::InstructionLimitExceeded;}
		if(fault==4){s.logicalAddress.line=254;s.pendingPresentation->conditional->targetLine=255;s.currentScript=script(1,{record(1,1,255,0x2c,{70,37})});kind=XeenEventExecutionErrorKind::LineOverflow;}
		const auto failure=std::get<XeenEventExecutionError>(e.resume(s));
		check(failure.kind==kind && failure.rewards.discarded==0 && !e.f.initial.roster.at(0).miscellaneous[0].id,"production guard enqueued");
	}
	{
		Execution e;e.f.initial.questItems.increment(17);
		std::vector<XeenEventRecord> records{record(1,1,0,12,{21,99})};
		for(int i=1;i<=11;++i)records.push_back(record(1,1,i,0x2c,{70,37}));
		auto s=pending(begin(e,records));
		check(e.f.initial.questItems.at(17)==0 && s.rewardReceipt.delivered==10 && s.rewardReceipt.overflow==1 &&
			s.instructionCount==12,"overflow refunded Root or stopped execution");
	}
	{
		Execution e;e.f.initial.questItems.increment(17);
		const auto failure=std::get<XeenEventExecutionError>(begin(e,{record(1,1,0,12,{21,99}),
			record(1,1,1,0x2c,{70,37}),record(1,1,2,12,{21,99}),record(1,1,3,12,{0,0,104,2})}));
		check(failure.kind==XeenEventExecutionErrorKind::QuestItemUnderflow && failure.instructionCount==3 &&
			failure.rewards.discarded==1 && e.f.initial.questItems.at(17)==0 && !e.f.initial.questFlags.isSet(2) &&
			!e.f.initial.roster.at(0).miscellaneous[0].id,"underflow rolled back earlier take/delivered queue/continued");
	}
	Execution call;auto s=pending(begin(call,{record(1,1,0,0x2c,{70,37,0,1}),record(1,1,1,0x19,{2,2,0}),
		pause(2),record(2,2,0,0x2c,{71,73,123}),record(2,2,1,0x1a)}));
	check(s.pendingRewards.size()==2 && s.callStack.empty() && s.instructionCount==5,"real production Call/Return");
	s=pending(call.resume(s));check(s.rewardReceipt.delivered==2 && s.rewardReceipt.entries[1].item.material==11,"call order/lifecycle");
	for(int outcome=0;outcome<4;++outcome) {
		Execution e;e.f.initial.questItems.increment(17);e.f.initial.questFlags.set(2);
		auto s=pending(begin(e,{record(1,1,0,12,{21,99}),record(1,1,1,12,{104,2}),
			record(1,1,2,0x2c,{70,37,0,1}),pause(3),record(1,1,4,outcome==0?0xff:0x12)}));
		check(e.f.initial.questItems.at(17)==0 && !e.f.initial.questFlags.isSet(2) && s.pendingRewards.size()==1,"exchange effects not immediate");
		if(outcome==0){const auto failure=std::get<XeenEventExecutionError>(e.resume(s));check(failure.rewards.discarded==1,"real production failure discard");}
		else {
			if(outcome==1)full(e.f.initial,false);
			if(outcome==2)for(auto id:e.f.initial.party.activeRosterIds())e.f.initial.roster.at(id).conditions[static_cast<std::size_t>(XeenCondition::Dead)]=1;
			if(outcome==3)for(auto id:e.f.initial.party.activeRosterIds())e.f.initial.roster.at(id).currentSp=-1;
			s=pending(e.resume(s));check(s.rewardReceipt.lost==(outcome==3?0U:1U) && s.rewardReceipt.delivered==(outcome==3?1U:0U),"live eligibility/capacity loss");
		}
		check(e.f.initial.questItems.at(17)==0 && !e.f.initial.questFlags.isSet(2),"failed/lost exchange refunded quest");
	}
}
void publication(){
	for(bool automatic:{false,true}){Execution e;auto s=e.seed({pause(),record(1,1,1,0x07,{2,2,2})},10);
		s.workingGameFlags.set(7);XeenEventSystem events(e.scripts,e.texts);
		if(automatic){auto r=events.resumeAutomaticEvent(s,XeenPresentationResponse::Acknowledged,e.world,e.f.initial,e.camera,e.flags);
			check(std::holds_alternative<XeenEventExecutionSuspended>(r)&&e.camera.mapId==XeenMapIdentity(1)&&!e.flags.isSet(7),"automatic published early");
			s=std::get<XeenEventExecutionSuspended>(r).state;
			r=events.resumeAutomaticEvent(s,XeenPresentationResponse::Acknowledged,e.world,e.f.initial,e.camera,e.flags);
			check(std::holds_alternative<XeenAutomaticEventCompleted>(r),"automatic final ACK");
		}else{auto r=events.resumeManualEvent(s,XeenPresentationResponse::Acknowledged,e.world,e.f.initial,e.camera,e.flags);
			check(std::holds_alternative<XeenEventExecutionSuspended>(r)&&e.camera.mapId==XeenMapIdentity(1)&&!e.flags.isSet(7),"manual published early");
			s=std::get<XeenEventExecutionSuspended>(r).state;
			r=events.resumeManualEvent(s,XeenPresentationResponse::Acknowledged,e.world,e.f.initial,e.camera,e.flags);
			check(std::holds_alternative<XeenManualEventCompleted>(r),"manual final ACK");}
		check(e.camera.mapId==XeenMapIdentity(2)&&e.flags.isSet(7)&&e.f.initial.roster.at(1).miscellaneous[0].id==46,"final publication/items");
	}
}
int main(){try{terminals();continuations();production();publication();std::cout<<"Reward interpreter terminal, continuation and publication matrix passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
