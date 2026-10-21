#include "XeenTreasureDeliveryTestSupport.h"
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
		s.treasure.whoWill=2;
		auto r=e.resume(s);
		if(terminal==3)r=e.resume(pending(r),CharacterSelectionCancelled{});
		if(terminal==4)r=e.resume(pending(r),XeenPresentationResponse::Acknowledged);
		s=pending(r);
		check(s.reward.phase==XeenRewardPhase::Receipt&&s.treasure.pending()&&s.reward.receipt.delivered==1,"successful terminal omitted delivery");
		check(s.reward.receipt.entries[0].owner==1&&s.treasure.whoWill==2,"cancel changed preference");
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
	check(xeenPrepareRewardReceipt(s.treasure).count==1&&s.reward.phase==XeenRewardPhase::Running&&s.callStack.empty()&&s.instructionCount==4,"Call/Return delivered early or lost staging");
	s=pending(e.resume(s));check(s.reward.receipt.delivered==1&&s.instructionCount==5,"Call/Return finalization count");
	for(auto bad:std::vector<XeenEventRecord>{record(1,1,1,0x1a),record(1,1,1,0x19,{99,99,0}),
		record(1,1,1,9,{20,7,99}),record(1,1,1,0x2c,{69,37,0,1}),record(1,1,1,12,{21,117,0,0}),
		record(1,1,1,12,{104,30,0,0})}){
		Execution x;auto state=x.seed({pause(),bad});state.workingGameFlags.set(7);
		auto result=x.resume(state);const auto &error=std::get<XeenEventExecutionError>(result);
		check(error.rewards.discarded==1&&error.rewards.discardReason==XeenRewardDiscard::ExecutionError&&
			!x.f.initial.roster.at(0).miscellaneous[0].id&&!x.flags.isSet(7),"error delivered or lost accounting");
	}
	Execution who;auto state=who.seed({record(1,1,0,0x20,{0,1}),record(1,1,1,0x12)});
	check(!state.treasure.whoWill&&state.activeCharacterIndex==0,"initial reward preference present");
	state=pending(who.resume(state,SelectedCharacter{1}));check(state.reward.receipt.entries[0].owner==1,"WhoWill reward preference");
	Execution single;single.f.initial.party=XeenParty::fromRosterIds({0});
	state=single.seed({pause(),record(1,1,1,0x20,{0,1}),record(1,1,2,0x12)});state.treasure.whoWill=6;
	state=pending(single.resume(state));check(state.treasure.whoWill==6&&state.reward.receipt.entries[0].owner==0,"single WhoWill overwrote preference");
	Execution reordered;state=reordered.seed({record(1,1,0,0x20,{0,1}),pause(1),record(1,1,2,0x12)});
	state=pending(reordered.resume(state,SelectedCharacter{1}));
	reordered.f.initial.party=XeenParty::fromRosterIds({1,0,1});
	state=pending(reordered.resume(state));check(state.reward.receipt.entries[0].owner==0,"preferred index did not resolve current membership");
	Execution immediate;state=immediate.seed({pause(),record(1,1,1,12,{0,0,21,99}),record(1,1,2,12,{0,0,104,2}),record(1,1,3,0xff)});
	const auto failed=std::get<XeenEventExecutionError>(immediate.resume(state));
	check(failed.rewards.discarded==2&&immediate.f.initial.questItems.at(17)==1&&immediate.f.initial.questFlags.isSet(2),"pre-delivery error rolled back prior immediate grants");
}
XeenEventExecutionStepResult begin(Execution &e,std::vector<XeenEventRecord> records) {
	e.f.scripts[1]=std::move(records);
	auto result=e.interpreter.begin(e.camera,e.f.initial,e.flags,e.world,e.scripts,e.texts);
	for(unsigned i=0;i<8*XeenTreasureStaging::kSlots;++i) {
		auto *s=std::get_if<XeenEventExecutionSuspended>(&result);
		if(!s || (s->state.reward.phase!=XeenRewardPhase::Blank && s->state.reward.phase!=XeenRewardPhase::Found))return result;
		result=e.resumeOne(std::move(s->state));
	}
	throw std::runtime_error("Reward initial hold bound");
}
void production(){
	for(int n : {1,5,11}) {
		Execution e;std::vector<XeenEventRecord> records;
		for(int i=0;i<n;++i) records.push_back(record(1,1,i,0x2c,
			{static_cast<std::uint8_t>(70+i%2),static_cast<std::uint8_t>(1+i),255,193}));
		records.push_back(pause(n));
		auto s=pending(begin(e,records));
		check(s.instructionCount==n+1 && xeenPrepareRewardReceipt(s.treasure).count==std::min(n,10) && s.treasure.full(XeenRewardCategory::Miscellaneous)==(n>=10),"real production staging/overflow");
		for(int i=0;i<std::min(n,10);++i) {
			const auto item=s.treasure.miscellaneous[i];
			check(item.material==10+i%2 && item.id==1+i && item.state==1 && item.frame==0,"deterministic fields/order/suffix");
		}
		check(!e.f.initial.roster.at(0).miscellaneous[0].id,"production delivered before finalization");
		s=pending(e.resume(s));check(s.reward.receipt.delivered==std::min(n,10),"real natural finalizer");
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
			failure.instructionCount==10 && failure.source && failure.source->line==10 && failure.rewards.discarded==10 &&
			!e.f.initial.roster.at(0).miscellaneous[0].id,"full staging operand validation/accounting");
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
		check(failure.kind==kind && failure.rewards.discarded==0 && !e.f.initial.roster.at(0).miscellaneous[0].id,"production guard staged");
	}
	{
		Execution e;e.f.initial.questItems.increment(17);
		std::vector<XeenEventRecord> records{record(1,1,0,12,{21,99})};
		for(int i=1;i<=11;++i)records.push_back(record(1,1,i,0x2c,{70,37}));
		auto s=pending(begin(e,records));
		check(e.f.initial.questItems.at(17)==0 && s.reward.receipt.delivered==10 &&
			s.instructionCount==12,"overflow refunded Root or stopped execution");
	}
	{
		Execution e;e.f.initial.questItems.increment(17);
		// A quest take at zero wraps the byte counter (0x22339) and the script continues.
		const auto result=begin(e,{record(1,1,0,12,{21,99}),
			record(1,1,1,0x2c,{70,37}),record(1,1,2,12,{21,99}),record(1,1,3,12,{0,0,104,2})});
		check(!std::holds_alternative<XeenEventExecutionError>(result) && e.f.initial.questItems.at(17)==255 &&
			e.f.initial.questFlags.isSet(2),"quest take at zero stopped or did not wrap");
	}
	Execution call;auto s=pending(begin(call,{record(1,1,0,0x2c,{70,37,0,1}),record(1,1,1,0x19,{2,2,0}),
		pause(2),record(2,2,0,0x2c,{71,73,123}),record(2,2,1,0x1a)}));
	check(xeenPrepareRewardReceipt(s.treasure).count==2 && s.callStack.empty() && s.instructionCount==5,"real production Call/Return");
	s=pending(call.resume(s));check(s.reward.receipt.delivered==2 && s.reward.receipt.entries[1].item.material==11,"call order/lifecycle");
	for(int outcome=0;outcome<4;++outcome) {
		Execution e;e.f.initial.questItems.increment(17);e.f.initial.questFlags.set(2);
		auto s=pending(begin(e,{record(1,1,0,12,{21,99}),record(1,1,1,12,{104,2}),
			record(1,1,2,0x2c,{70,37,0,1}),pause(3),record(1,1,4,outcome==0?0xff:0x12)}));
		check(e.f.initial.questItems.at(17)==0 && !e.f.initial.questFlags.isSet(2) && xeenPrepareRewardReceipt(s.treasure).count==1,"exchange effects not immediate");
		if(outcome==0){const auto failure=std::get<XeenEventExecutionError>(e.resume(s));check(failure.rewards.discarded==1,"real production failure discard");}
		else {
			if(outcome==1)full(e.f.initial,false);
			if(outcome==2)for(auto id:e.f.initial.party.activeRosterIds())e.f.initial.roster.at(id).conditions[static_cast<std::size_t>(XeenCondition::Dead)]=1;
			if(outcome==3)for(auto id:e.f.initial.party.activeRosterIds())e.f.initial.roster.at(id).currentSp=-1;
			s=pending(e.resume(s));check(s.reward.receipt.lost==0 && s.reward.receipt.delivered==1,"Live category replacement and unrestricted dead fallback");
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
			while(s.reward.phase!=XeenRewardPhase::Receipt) {
				r=events.resumeAutomaticEvent(s,XeenPresentationResponse::Acknowledged,e.world,e.f.initial,e.camera,e.flags);
				s=std::get<XeenEventExecutionSuspended>(r).state;
				check(e.camera.mapId==XeenMapIdentity(1)&&!e.flags.isSet(7),"intermediate reward hold published camera/flags");
			}
			r=events.resumeAutomaticEvent(s,XeenPresentationResponse::Acknowledged,e.world,e.f.initial,e.camera,e.flags);
			check(std::holds_alternative<XeenAutomaticEventCompleted>(r),"automatic final ACK");
		}else{auto r=events.resumeManualEvent(s,XeenPresentationResponse::Acknowledged,e.world,e.f.initial,e.camera,e.flags);
			check(std::holds_alternative<XeenEventExecutionSuspended>(r)&&e.camera.mapId==XeenMapIdentity(1)&&!e.flags.isSet(7),"manual published early");
			s=std::get<XeenEventExecutionSuspended>(r).state;
			while(s.reward.phase!=XeenRewardPhase::Receipt) {
				r=events.resumeManualEvent(s,XeenPresentationResponse::Acknowledged,e.world,e.f.initial,e.camera,e.flags);
				s=std::get<XeenEventExecutionSuspended>(r).state;
				check(e.camera.mapId==XeenMapIdentity(1)&&!e.flags.isSet(7),"intermediate reward hold published camera/flags");
			}
			r=events.resumeManualEvent(s,XeenPresentationResponse::Acknowledged,e.world,e.f.initial,e.camera,e.flags);
			check(std::holds_alternative<XeenManualEventCompleted>(r),"manual final ACK");}
		check(e.camera.mapId==XeenMapIdentity(2)&&e.flags.isSet(7)&&e.f.initial.roster.at(1).miscellaneous[0].id==46,"final publication/items");
	}
}
void individualPublication() {
 Execution e;e.f.initial.purse=XeenPurse{};
 auto state=e.seed({pause(),record(1,1,1,0x12)},2);state.treasure.gold+=31u;
 state=pending(e.resumeOne(state));
 check(state.reward.phase==XeenRewardPhase::Blank && state.reward.cursor==0 && state.reward.receipt.delivered==1 &&
  state.pendingPresentation->request.rewardWaitTicks==1 && e.f.initial.roster.at(0).miscellaneous.back().id==37 &&
  !e.f.initial.roster.at(0).miscellaneous.front().id && !e.f.initial.purse->gold,
  "First item did not publish before the blank one-tick hold");
 state=pending(e.resumeOne(state));
 check(state.reward.phase==XeenRewardPhase::Found && state.reward.receipt.delivered==1 &&
  state.pendingPresentation->request.rewardWaitTicks==10 && e.f.initial.roster.at(0).miscellaneous.front().id==37,
  "Skipping blank did not stop at found/sort/ten-tick boundary");
 state=pending(e.resumeOne(state));
 check(state.reward.phase==XeenRewardPhase::Blank && state.reward.cursor==1 && state.reward.receipt.delivered==2,
  "Found completion did not publish exactly the next item");
 state=pending(e.resumeOne(state));state=pending(e.resumeOne(state));
 check(state.reward.phase==XeenRewardPhase::Receipt && !state.pendingPresentation->request.rewardWaitTicks &&
  state.treasure.pending() && !e.f.initial.purse->gold,"Item skips manufactured final currency acknowledgment");
 check(std::holds_alternative<XeenEventExecutionCompleted>(e.resumeOne(state)) && e.f.initial.purse->gold==31,
  "Final currency settlement failed");
 for(bool packsFull:{false,true}) {
  Execution quest;if(packsFull)full(quest.f.initial,true);
  auto q=quest.seed({pause(),record(1,1,1,12,{0,0,21,99}),record(1,1,2,0x12)},0);
  const auto inventory=xeenInventoryInspection(quest.f.initial);q=pending(quest.resumeOne(q));
  check(quest.f.initial.questItems.at(17)==1 && xeenPrepareRewardReceipt(q.treasure).entries[0].quest,"Quest grant did not join Weapons treasure staging");
  if(packsFull){check(q.reward.phase==XeenRewardPhase::Warning,"Quest full-pack warning missing");q=pending(quest.resumeOne(q));}
  else {check(q.reward.phase==XeenRewardPhase::Blank && q.reward.receipt.entries[0].quest,"Quest blank display obligation missing");q=pending(quest.resume(q));}
  check(q.reward.phase==XeenRewardPhase::Receipt && quest.f.initial.questItems.at(17)==1 &&
   q.reward.receipt.delivered==(packsFull?0u:1u),"Quest display re-granted or ignored full-pack rule");
  quest.resumeOne(q);check(quest.f.initial.questItems.at(17)==1,"Final quest acknowledgment granted twice");
  check(xeenInventoryInspection(quest.f.initial).substr(xeenInventoryInspection(quest.f.initial).find("\nActive order:"))==
   inventory.substr(inventory.find("\nActive order:")),"Quest presentation inserted an inventory item");
 }
}
// M81 Part B: Event treasure on the running program's staging (0x26774). Each run
// stashes pending currency/readiness with the arrays, stages Event items in the
// same arrays, delivers them after the Events and restores the stash.
void sharedStaging(){
	const XeenItem weapon{0,30,0,0};
	const auto run=[](Execution &e,std::vector<XeenEventRecord> records) {
		auto result=begin(e,std::move(records));
		for(unsigned i=0;i<4 && std::holds_alternative<XeenEventExecutionSuspended>(result);++i)
			result=e.resume(std::get<XeenEventExecutionSuspended>(result).state);
		return result;
	};
	const auto weapons30=[&](const XeenPartyState &p) {
		unsigned n=0;for(auto id:p.party.activeRosterIds())for(const auto &item:p.roster.at(id).weapons)n+=item.id==30;return n;
	};
	{
		// No Event treasure: the stash returns exactly; the owner is not even republished.
		Execution e;XeenTreasureStaging staged;staged.gold=10;staged.ready=true;staged.weapons[0]=weapon;
		e.world.treasureStaging().publish(staged);const auto revision=e.world.treasureStaging().revision();
		check(std::holds_alternative<XeenEventExecutionCompleted>(run(e,{record(1,1,0,0x12)})) &&
			e.world.treasureStaging().value()==staged && e.world.treasureStaging().revision()==revision,
			"An Event without treasure changed pending monster loot");
	}
	{
		// Dormant monster items (no currency or readiness) deliver with the Event's own.
		Execution e;e.f.initial.purse=XeenPurse{};XeenTreasureStaging dormant;dormant.weapons[0]=weapon;
		e.world.treasureStaging().publish(dormant);
		auto s=pending(begin(e,{record(1,1,0,0x2c,{70,37}),record(1,1,1,0x12)}));
		check(s.reward.phase==XeenRewardPhase::Receipt && s.reward.receipt.count==2 &&
			s.reward.receipt.entries[0].category==XeenRewardCategory::Weapons && s.reward.receipt.entries[0].item.id==30 &&
			s.reward.receipt.entries[1].item.id==37,"Dormant monster items were not delivered first with the Event treasure");
		check(std::holds_alternative<XeenEventExecutionCompleted>(e.resume(s)) && weapons30(e.f.initial)==1 &&
			e.world.treasureStaging().value()==XeenTreasureStaging{},"Delivery did not empty the shared staging");
	}
	{
		// Pending monster currency: the Event delivers the monster items with its own,
		// then the stash returns them with their currency for ordinary delivery.
		Execution e;e.f.initial.purse=XeenPurse{};XeenTreasureStaging staged;staged.gold=10;staged.gems=2;staged.ready=true;staged.weapons[0]=weapon;
		e.world.treasureStaging().publish(staged);
		auto s=pending(begin(e,{record(1,1,0,0x2c,{70,37}),record(1,1,1,0x12)}));
		check(s.stashed && s.stash==staged && !s.treasure.gold && s.reward.receipt.count==2 && !s.reward.receipt.gold,
			"The Event did not stash the monster currency and readiness");
		check(std::holds_alternative<XeenEventExecutionCompleted>(e.resume(s)) && e.world.treasureStaging().value()==staged &&
			weapons30(e.f.initial)==1 && !e.f.initial.purse->gold,"The stash was not restored over the delivered staging");
		const auto again=xeenTestDeliverTreasure(e.world.treasureStaging().value(),e.f.initial);
		check(again.count==1 && weapons30(e.f.initial)==2 && e.f.initial.purse->gold==10 && e.f.initial.purse->gems==2,
			"Ordinary delivery after the Event must hand the monster items over a second time, currency once");
	}
	{
		// Event capacity counts the monster items: a full Misc array refuses the item.
		Execution e;XeenTreasureStaging dormant;for(auto &item:dormant.miscellaneous)item={3,8,7,0};
		e.world.treasureStaging().publish(dormant);
		check(std::holds_alternative<XeenEventExecutionCompleted>(run(e,{record(1,1,0,0x2c,{70,37}),record(1,1,1,0x12)})) &&
			e.world.treasureStaging().value()==dormant && !e.f.initial.roster.at(0).miscellaneous[0].id,
			"A full Misc array staged or delivered an Event item");
		Execution give;XeenTreasureStaging weapons;for(auto &item:weapons.weapons)item=weapon;
		give.world.treasureStaging().publish(weapons);
		const auto result=run(give,{record(1,1,0,12,{0,0,21,7}),record(1,1,1,0x2c,{70,37}),record(1,1,2,0x12)});
		check(std::holds_alternative<XeenEventExecutionCompleted>(result) && give.world.treasureStaging().value()==weapons &&
			!give.f.initial.roster.at(0).miscellaneous[0].id,"A full weapon array did not end the Event at the refused give");
	}
	{
		// An aborted Event leaves the owner at its entry snapshot, monster loot included.
		Execution e;XeenTreasureStaging staged;staged.gold=10;staged.ready=true;staged.weapons[0]=weapon;
		e.world.treasureStaging().publish(staged);const auto revision=e.world.treasureStaging().revision();
		const auto result=begin(e,{record(1,1,0,0x2c,{70,37}),record(1,1,1,0xff)});
		const auto *failure=std::get_if<XeenEventExecutionError>(&result);
		check(failure && failure->rewards.discarded==1 && e.world.treasureStaging().value()==staged &&
			e.world.treasureStaging().revision()==revision && !weapons30(e.f.initial),"An aborted Event did not restore its snapshot");
	}
}
int main(){try{terminals();sharedStaging();continuations();production();publication();individualPublication();std::cout<<"Reward interpreter individual publication matrix passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
