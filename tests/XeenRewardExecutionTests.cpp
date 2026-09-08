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
		record(1,1,1,9,{20,7,99}),record(1,1,1,0x2c,{70,37,0,1}),record(1,1,1,12,{21,99,0,0}),
		record(1,1,1,12,{104,2,0,0})}){
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
int main(){try{terminals();continuations();publication();std::cout<<"Reward interpreter terminal, continuation and publication matrix passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
