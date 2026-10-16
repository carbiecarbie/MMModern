#ifndef MMODERN_XEEN_EVENT_PUBLICATION_H
#define MMODERN_XEEN_EVENT_PUBLICATION_H
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenEventContinuation.h"
#include "games/xeen/XeenEventValues.h"
#include <limits>
namespace mmodern {
// A stack-bound publication capability, issued only to the live Journey continuation.
// It anticipates known effects in retained guard storage, never adopts callback state.
class XeenEventPublication {
public:
 mutable std::array<std::optional<unsigned>,30> damageEffects{};
 mutable std::uint32_t portraitEffectOwners=0; // Published spellFX recipients, presentation only.
	~XeenEventPublication() {if(retainedHistory)retainedHistory->swap(dispatchedRecords);}
	XeenEventPublication(const XeenEventPublication &) = delete;
	XeenEventPublication &operator=(const XeenEventPublication &) = delete;
	void check() const { authority(); guard.check(); }
    unsigned npcAnimationFrame() const {
        check();
        auto &world=const_cast<XeenWorld &>(guard.w);
        if(!world._sessionState._journeyRandom)integrity("NPC Journey random continuation is absent");
        XeenCombatRandom random(*world._sessionState._journeyRandom);
        const auto frame=random.draw(0,3);
        if(!frame)integrity("NPC frame conversion was rejected");
        const auto ending=random.continuation();
        guard.s._journeyRandom=ending;
        world._sessionState._journeyRandom=ending;
        guard.adoptMutationBoundary();
        return *frame;
    }
	void publishCandidate(XeenWorld &world,XeenPartyState &party,XeenCamera &camera,XeenGameFlags &flags,
			XeenWorld &candidateWorld,const XeenRestoreGuard &candidate) const {
		check();candidate.check();
		if(&world!=&guard.w || &party!=&guard.p || &camera!=&guard.c || &flags!=&guard.f ||
			&candidateWorld!=&candidate.w || !candidateWorld._detachedEventCandidate ||
			candidateWorld._sessionState._entry!=XeenEncounterEntry::Ordinary)
			integrity("Event publication owner identity changed");
		// All allocation and provider work precede the callback-free stores.
		auto presentation=world.prepareSpawnPresentation(candidateWorld);
  for(unsigned owner=0;owner<30;++owner) {
   const auto &effect=candidateWorld.scenePresentation().portraits[owner];
   if(effect.damageTicks && !effect.damageDeadline)damageEffects[owner]=effect.damageFrame;
   if(effect.spellFrame<4 && !effect.spellDeadline)portraitEffectOwners|=1u<<owner;
  }
		const XeenCloudsQuestFlags questFlags(candidate.questFlags);
		check();candidate.check();
		guard.prepareVertigoPublication(candidate);
		world.publishTransition(candidateWorld);
		world.publishEventParty(party,candidate.p);
		party.questFlags=questFlags;
		camera=candidate.cameraValue;flags=XeenGameFlags(candidate.flagValues);
		world.scenePresentation()=std::move(presentation);
		guard.adoptMutationBoundary();
	}
	void publishPrelude(XeenGameFlags &flags,const XeenEventExecutionState &state,
			const XeenEventContinuation &continuation) const {
		check();continuation.check(state);script(state.currentScript->file());
		if(&flags!=&guard.f)integrity("Event prelude flag owner changed");
		guard.prepareVertigoPrelude(state.workingGameFlags);
		flags=state.workingGameFlags;guard.adoptMutationBoundary();
	}
	void publishSequencePrefix(XeenWorld &world,XeenPartyState &party,const XeenRestoreGuard &candidate) const {
		check();candidate.check();
		if(&world!=&guard.w || &party!=&guard.p || !candidate.w.detachedEventCandidate())
			integrity("Event sequence publication owner changed");
		const XeenCloudsQuestItems quests(candidate.quests);const XeenCloudsQuestFlags flags(candidate.questFlags);
		guard.prepareEventSequencePrefix(candidate);
		world.publishEventParty(party,candidate.p);party.questItems=quests;party.questFlags=flags;
		world._sessionState._journeyRandom=candidate.s._journeyRandom;
		guard.adoptMutationBoundary();
		for(unsigned owner=0;owner<30;++owner) {
			auto &effect=const_cast<XeenWorld &>(candidate.w).scenePresentation().portraits[owner];
			if(effect.damageTicks && !effect.damageDeadline){damageEffects[owner]=effect.damageFrame;effect.damageTicks=0;}
			if(effect.spellFrame<4 && !effect.spellDeadline){portraitEffectOwners|=1u<<owner;effect.spellFrame=4;}
		}
	}
	void script(const XeenEventFile &file) const {
		check();
		if (file.mapId != original.mapId || file.resourceName!=original.resourceName || file.resourcePresent != original.resourcePresent || file.records.size() != original.records.size())
			integrity("Journey event topology changed");
		for (std::size_t i=0;i<file.records.size();++i) {
			const auto &a=file.records[i]; const auto &b=original.records[i];
			if (a.x!=b.x || a.y!=b.y || a.direction!=b.direction || a.line!=b.line || a.opcode!=b.opcode || a.parameters!=b.parameters || a.fileOffset!=b.fileOffset || a.lengthField!=b.lengthField)
				integrity("Journey event record changed");
		}
	}
	void continuation(const XeenEventExecutionState &state,const XeenEventContinuation &retained,bool detachedTravel=false) const {
        check();
        try {
            retained.check(state);
            // A detached Continue may suspend in its destination script. The
            // interpreter-issued preimage binds that entire script and state;
            // the live publication still validates its original entry resource.
            if(!detachedTravel || state.currentScript->file().mapId==original.mapId)
                script(state.currentScript->file());
        }
		catch(const std::logic_error &) {integrity("Event continuation or resource preimage changed");}
		// Remove may hide the initially selected object before treasure is shown.
		// Its identity belongs to the checked continuation, not a fresh selection.
		selectedObject=state.selectedObject;
	}
	void execution(const XeenEventExecutionState &state) const {
		check();
		{
			const auto interaction=xeenRegionalInteraction(original,guard.cameraValue);
            const auto &address=state.logicalAddress;
            const auto terminalSite=state.currentScript && address.x>=0 && address.x<=255 &&
                address.y>=0 && address.y<=255 && address.line>=0 && address.line<=255 ?
                state.currentScript->findInstructionIndex(address.x,address.y,state.lookupDirection,address.line):std::nullopt;
            if(terminalSite && guard.w.effectiveEvent({original.mapId,*terminalSite},
                state.currentScript->records()[*terminalSite]).opcode==0x11) {
                // The Flow retained and checked the exact interpreter-issued
                // suspension before this terminate-only settlement. A prelude
                // may have called subroutines; cmdDoTownEvent exits all of them.
                if(!xeen_state::sameCamera(state.workingCamera,guard.cameraValue) ||
                    state.workingGameFlags.values()!=guard.flagValues || address.mapId!=XeenMapIdentity(28) ||
                    !state.instructionCount ||
                    state.instructionCount>XeenEventInterpreter::kMaximumInstructions || state.pendingRewards.hasWork() ||
                    (state.pendingPresentation && state.pendingPresentation->continuation!=XeenEventPendingContinuation::Terminate))
                    integrity("Service terminal continuation changed");
                script(state.currentScript->file());currentSite=terminalSite;return;
            }
			if (interaction==XeenRegionalInteraction::None || !xeen_state::sameCamera(state.workingCamera,guard.cameraValue) ||
				state.workingGameFlags.values()!=guard.flagValues || state.logicalAddress.mapId!=original.mapId ||
				state.logicalAddress.x!=guard.cameraValue.x || state.logicalAddress.y!=guard.cameraValue.y ||
				state.logicalAddress.line<0 || state.logicalAddress.line>255 ||
				state.lookupDirection!=guard.cameraValue.direction || state.instructionCount>XeenEventInterpreter::kMaximumInstructions ||
				state.instructionCount!=dispatchedRecords.size() || !state.callStack.empty() ||
				!(state.selectedObject==selectedObject) || !state.currentScript)
				integrity("Regional event continuation changed");
			script(state.currentScript->file());
			const auto site=state.currentScript->findInstructionIndex(static_cast<std::uint8_t>(state.logicalAddress.x),
				static_cast<std::uint8_t>(state.logicalAddress.y),state.lookupDirection,static_cast<std::uint8_t>(state.logicalAddress.line));
			if(!site && state.missingInstructionPolicy!=XeenEventMissingInstructionPolicy::NaturalCompletion)
				integrity("Regional event closure is not natural");
			if(state.rewardPhase==XeenRewardPhase::Running || state.rewardPhase==XeenRewardPhase::Warning)
				validateRewardPrefix(state.pendingRewards,dispatchedRecords.size());
			currentSite=site;
		}
	}
	// Interpreter-issued dispatch, after source decoding and before any side effect.
	// Flow retains this history across suspensions; callback state cannot issue records.
	void dispatched(const XeenEventExecutionState &state,const XeenDecodedEventInstruction &decoded) const {
		check();
		if(!currentSite || decoded.source.recordIndex!=currentSite || state.instructionCount!=dispatchedRecords.size()+1 ||
			state.instructionCount>XeenEventInterpreter::kMaximumInstructions)
			integrity("Event dispatch history changed");
		// Retain the effective dispatch, before a later Remove can disable its
		// resource record. A None dispatch contributes to instruction count only.
		dispatchedRecords.push_back({std::holds_alternative<XeenEventNone>(decoded.operation)?std::nullopt:currentSite,
			decoded.source.opcode});
	}

	void prepareGrant(std::size_t index) const {
		check();
		const auto effect=takeOrGive();
		if (!neutral(effect.first) || effect.second.mode!=21 || effect.second.value!=index+82 || !neutral(effect.third) || index>=guard.quests.size()) integrity("Regional grant operands changed");
		consume();
		if (grant)
			throw std::logic_error("Journey grant publication unavailable");
		grant=true; grantIndex=index;
	}
	// Generic live writes of If/SetVar/TakeOrGive value modes: party fields an
	// Event owns, anticipated into the retained preimage after each write.
	void prepareEventWrite() const {
		check();
		const auto operation=decodedOperation();
		if(!std::holds_alternative<XeenEventTakeOrGive>(operation) && !std::holds_alternative<XeenEventTakeOrGiveTest>(operation) &&
			!std::holds_alternative<XeenEventSetVar>(operation) && !std::holds_alternative<XeenEventGiveMulti>(operation))
			integrity("Event write has no dispatched value operation");
	}
	void eventWritten(const XeenPartyState &party) const {
		for(const auto id:guard.membership) {
			xeenEventCopyOwnedCharacter(guard.characters[id],party.roster.at(id));
			guard.inputs[id]=party.roster.combatInputs(id);
		}
		guard.context=party.encounterContext;guard.food=party.food;
		if(guard.treasure && party.monsterTreasure) {guard.treasure->gold=party.monsterTreasure->gold;guard.treasure->gems=party.monsterTreasure->gems;}
		guard.adoptMutationBoundary();
	}
	void text(const XeenEventTextFile &file) const {
		guard.admitEventText(file);
	}
	void granted() const noexcept { guard.quests[grantIndex]=XeenCloudsQuestItems::afterEventGrant(guard.quests[grantIndex]);guard.adoptMutationBoundary(); }
	void prepareQuestFlag(bool value) const {
		check();
		const auto effect=takeOrGive();
		const auto pair=value ? effect.second : effect.first;
		if(pair.mode!=104 || pair.value>=guard.questFlags.size() || !neutral(value ? effect.first : effect.second) || !neutral(effect.third)) integrity("Regional quest flag operands changed");
		questFlagIndex=pair.value;consume();
	}
	void questFlagWritten(bool value) const noexcept { guard.questFlags[questFlagIndex]=value;guard.adoptMutationBoundary(); }
	void prepareQuestTake(std::size_t index) const {
		check();
		const auto effect=takeOrGive();
		if(effect.first.mode!=21 || effect.first.value!=index+82 || !neutral(effect.second) || !neutral(effect.third) || index>=guard.quests.size()) integrity("Regional quest take operands changed");
		takeIndex=index;consume();
	}
	void questTaken() const noexcept { guard.quests[takeIndex]=XeenCloudsQuestItems::afterEventTake(guard.quests[takeIndex]);guard.adoptMutationBoundary(); }
	void deferredAudio() const {
		check();
		const auto operation=decodedOperation();
		if(!std::holds_alternative<XeenEventVoiceCue>(operation) &&
			!std::holds_alternative<XeenEventCdSpeech>(operation))
			integrity("Deferred audio has no dispatched audio instruction");
		consume();
	}
	void prepareWellHp(std::uint8_t owner,std::int16_t before,std::int16_t after) const {
		check();
		const auto effect=takeOrGive();
		if (!neutral(effect.first) || effect.second.mode!=8 || effect.second.value!=25 || !neutral(effect.third) || owner>=30 ||
			guard.characters[owner].currentHp!=before || std::int32_t(before)+25!=after)
			integrity("Regional well HP site or preimage changed");
		consume();
	}
	void wellHpWritten(std::uint8_t owner,std::int16_t after) const noexcept { guard.characters[owner].currentHp=after;guard.adoptMutationBoundary();portraitEffectOwners|=1u<<owner; }
	void prepareWellFlag() const {
		check();
		const auto effect=takeOrGive();
		if (!neutral(effect.first) || effect.second.mode!=103 || effect.second.value!=16 || !neutral(effect.third) || !guard.recovery)
			integrity("Regional well flag site changed");
		consume();
	}
	void wellFlagWritten() const noexcept { guard.recovery->values[16]=true;guard.adoptMutationBoundary(); }
	void prepareRewardEnqueue(const XeenEventExecutionState &state,const XeenItem &item) const {
		check();
		const auto operation=decodedOperation();
		const auto *effect=std::get_if<XeenEventGiveEnchanted>(&operation);
		if(!effect || item.material!=effect->itemCode-60 || item.id!=effect->specialId || item.state!=1 || item.frame!=0 ||
			state.rewardPhase!=XeenRewardPhase::Running || dispatchedRecords.empty())
			integrity("Regional reward production changed");
		validateRewardPrefix(state.pendingRewards,dispatchedRecords.size()-1);
		consume();
		if (beforeRewardEnqueue) { beforeRewardEnqueue(); check(); }
	}
	XeenRewardReceipt deliverRewards(XeenPendingRewards &pending,XeenPartyState &party,
		std::optional<std::size_t> preferred) const {
		check();
		if(preferred || delivered)integrity("Regional reward delivery changed");
		validateRewardPrefix(pending,dispatchedRecords.size());
		delivered=true;
		XeenPartyState detached;
		detached.party=XeenParty::fromRosterIds(guard.membership);
		for (auto id:guard.membership) detached.roster.at(id)=guard.characters[id];
		auto detachedQueue=pending;
		auto receipt=xeenDeliverRewards(detachedQueue,detached,preferred);
		check();
		for (auto id:guard.membership) {
			guard.characters[id].miscellaneous=detached.roster.at(id).miscellaneous;
			party.roster.at(id).miscellaneous=detached.roster.at(id).miscellaneous;
		}
		pending={};
		guard.adoptMutationBoundary();
		return receipt;
	}
	void deliverRewardEntry(XeenEventExecutionState &state,XeenPartyState &party) const {
		check();validateRewardPrefix(state.pendingRewards,dispatchedRecords.size());
		XeenPartyState detached;detached.party=XeenParty::fromRosterIds(guard.membership);
		for(auto id:guard.membership)detached.roster.at(id)=guard.characters[id];
		auto receipt=state.rewardReceipt;xeenDeliverRewardEntry(receipt,state.rewardCursor,detached,state.preferredRewardRecipient,false);
		check();
		for(auto id:guard.membership)for(unsigned category=0;category<4;++category) {
			const auto kind=static_cast<XeenRewardCategory>(category);
			xeenRewardItems(guard.characters[id],kind)=xeenRewardItems(detached.roster.at(id),kind);
			xeenRewardItems(party.roster.at(id),kind)=xeenRewardItems(detached.roster.at(id),kind);
		}
		state.rewardReceipt=receipt;guard.adoptMutationBoundary();
	}
	void sortRewardEntry(const XeenRewardEntry &entry,XeenPartyState &party) const {
		check();if(!entry.owner)return;
		auto items=xeenRewardItems(guard.characters[*entry.owner],entry.category);xeenCompactItems(items);
		check();xeenRewardItems(guard.characters[*entry.owner],entry.category)=items;
		xeenRewardItems(party.roster.at(*entry.owner),entry.category)=items;guard.adoptMutationBoundary();
	}
	void creditRewardCurrency(XeenEventExecutionState &state,XeenPartyState &party) const {
		check();validateRewardPrefix(state.pendingRewards,dispatchedRecords.size());
		if(party.monsterTreasure) {
			guard.treasure->gold+=state.rewardReceipt.gold;guard.treasure->gems+=state.rewardReceipt.gems;
			party.monsterTreasure->gold=guard.treasure->gold;party.monsterTreasure->gems=guard.treasure->gems;
		}
		state.pendingRewards={};guard.adoptMutationBoundary();
	}
	void prepareRemove(const XeenCamera &physical, std::optional<XeenObjectIdentity> selected,
		const XeenEventFile &file) const {
		check(); script(file);
		if(!std::holds_alternative<XeenEventRemove>(decodedOperation()))integrity("Regional Remove operands changed");
		consume();
		if (!xeen_state::sameCamera(physical,guard.cameraValue) || !(selected==selectedObject) || removal)
			throw std::logic_error("Journey Remove publication unavailable");
		// The whole per-map table, with this map's Remove applied.
		mapStates=guard.s._mapStates;
		auto &state=mapStates.state(physical.mapId);
		if (selected) state.removedObjects.insert(*selected);
		for (std::size_t i=0;i<original.records.size();++i)
			if(original.records[i].x==physical.x && original.records[i].y==physical.y) state.disabledEvents.insert({physical.mapId,i});
		state.eventOverrides.erase(std::remove_if(state.eventOverrides.begin(),state.eventOverrides.end(),
			[&](const auto &v){return state.disabledEvents.count(v.id)!=0;}),state.eventOverrides.end());
		removal=true;
	}
	void removed() const noexcept { guard.s._mapStates.swap(mapStates);guard.adoptMutationBoundary(); }
private:
	static bool neutral(const XeenEventTakeOrGivePair &pair) noexcept {return pair.mode==0 && pair.value==0;}
	XeenDecodedEventOperation decodedOperation() const {
		if(!currentSite || dispatchedRecords.empty() || dispatchedRecords.back().record!=*currentSite ||
			guard.s.isEventDisabled({original.mapId,*currentSite}))integrity("Event effect has no dispatched source");
		const auto result=XeenEventDecoder::decode(dispatchedRecord(*currentSite,dispatchedRecords.back().opcode),
			{original.mapId,original.resourceName,*currentSite,true,dispatchedRecords.back().opcode!=original.records.at(*currentSite).opcode});
		const auto *decoded=std::get_if<XeenDecodedEventInstruction>(&result);
		if(!decoded)integrity("Event effect source is unsupported");
		return decoded->operation;
	}
	XeenEventTakeOrGive takeOrGive() const {
		const auto operation=decodedOperation();const auto *effect=std::get_if<XeenEventTakeOrGive>(&operation);
		if(!effect)integrity("Event effect source is not GiveTake");return *effect;
	}
	void consume() const {
		if(!consumed.insert(dispatchedRecords.size()).second)integrity("Event effect already published");
	}
	XeenEventRecord dispatchedRecord(std::size_t site,std::uint8_t opcode) const {
		auto record=original.records.at(site);record.opcode=opcode;return record;
	}
	void validateRewardPrefix(const XeenPendingRewards &pending,std::size_t before) const {
		std::size_t count=0;
		if(pending.overflow() || pending.invalid())integrity("Regional reward queue is malformed");
		for(std::size_t i=0;i<before;++i) {
			const auto &dispatch=dispatchedRecords.at(i);const auto retained=dispatch.record;if(!retained)continue;
			const auto site=*retained;
			const auto result=XeenEventDecoder::decode(dispatchedRecord(site,dispatch.opcode),
				{original.mapId,original.resourceName,site,true,dispatch.opcode!=original.records.at(site).opcode});
			const auto *instruction=std::get_if<XeenDecodedEventInstruction>(&result);if(!instruction)continue;
			const auto *effect=std::get_if<XeenEventGiveEnchanted>(&instruction->operation);
			const auto *give=std::get_if<XeenEventTakeOrGive>(&instruction->operation);
			const bool quest=give && neutral(give->first) && give->second.mode==21 &&
				give->second.value>=82 && neutral(give->third);
			if(!effect && !quest)continue;
			if(count>=pending.size())integrity("Regional reward prefix loses dispatched effect");
			const auto index=count++;const auto &item=pending.at(index);
			if(quest) {
				if(!pending.questAt(index) || pending.categoryAt(index)!=XeenRewardCategory::Weapons ||
					item.id!=give->second.value || item.material || item.state || item.frame)
					integrity("Regional quest reward differs from dispatched operands");
			} else if(pending.questAt(index) || item.material!=effect->itemCode-60 || item.id!=effect->specialId || item.state!=1 || item.frame!=0)
				integrity("Regional reward item differs from dispatched operands");
		}
		if(count!=pending.size())integrity("Regional reward prefix adds an undispatched effect");
	}
	[[noreturn]] void integrity(const char *message) const { guard.failed=true; throw std::logic_error(message); }
	friend class XeenEventFlow;
	XeenEventPublication(XeenRestoreGuard &guard, const XeenEventFile &original, std::function<void()> authority,
		std::function<void()> beforeRewardEnqueue={}, std::vector<XeenEventDispatchRecord> *history=nullptr) :
		guard(guard), original(original), authority(std::move(authority)),
		beforeRewardEnqueue(std::move(beforeRewardEnqueue)), retainedHistory(history),
		dispatchedRecords(history ? *history : std::vector<XeenEventDispatchRecord>{}) {
		check();selectedObject=const_cast<XeenWorld &>(guard.w).selectObject(guard.cameraValue);check();
	}
	XeenRestoreGuard &guard;
	const XeenEventFile &original;
	std::function<void()> authority;
	std::function<void()> beforeRewardEnqueue;
	mutable bool grant=false, removal=false, delivered=false;
	std::vector<XeenEventDispatchRecord> *retainedHistory;
	mutable std::vector<XeenEventDispatchRecord> dispatchedRecords;
	mutable std::set<std::size_t> consumed;
	mutable std::optional<XeenObjectIdentity> selectedObject;
	mutable std::size_t questFlagIndex=0, takeIndex=0;
	mutable std::size_t grantIndex=18;
	mutable std::optional<std::size_t> currentSite;
	mutable XeenMapTable mapStates;
};
}
#endif
