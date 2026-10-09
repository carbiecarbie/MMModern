#ifndef MMODERN_XEEN_EVENT_PUBLICATION_H
#define MMODERN_XEEN_EVENT_PUBLICATION_H
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenEventContinuation.h"
#include <limits>
namespace mmodern {
// A stack-bound publication capability, issued only to the live Journey continuation.
// It anticipates known effects in retained guard storage, never adopts callback state.
class XeenEventPublication {
public:
 mutable std::uint32_t portraitEffectOwners=0; // Published giveTake(8) recipients, presentation only.
	~XeenEventPublication() {if(retainedHistory)retainedHistory->swap(dispatchedRecords);}
	XeenEventPublication(const XeenEventPublication &) = delete;
	XeenEventPublication &operator=(const XeenEventPublication &) = delete;
	void check() const { authority(); guard.check(); }
	void publishCandidate(XeenWorld &world,XeenPartyState &party,XeenCamera &camera,XeenGameFlags &flags,
			XeenWorld &candidateWorld,const XeenRestoreGuard &candidate) const {
		check();candidate.check();
		if(&world!=&guard.w || &party!=&guard.p || &camera!=&guard.c || &flags!=&guard.f ||
			&candidateWorld!=&candidate.w || !candidateWorld._detachedEventCandidate ||
			candidateWorld._sessionState._entry!=XeenEncounterEntry::Ordinary)
			integrity("Event publication owner identity changed");
		// All allocation and provider work precede the callback-free stores.
		auto presentation=world.prepareSpawnPresentation(candidateWorld);
		const XeenCloudsQuestFlags questFlags(candidate.questFlags);
		check();candidate.check();
		guard.prepareVertigoPublication(candidate);
		world.publishTransition(candidateWorld);
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
	void continuation(const XeenEventExecutionState &state,const XeenEventContinuation &retained) const {
		check();
		try {retained.check(state);script(state.currentScript->file());}
		catch(const std::logic_error &) {integrity("Event continuation or resource preimage changed");}
	}
	void execution(const XeenEventExecutionState &state) const {
		check();
		{
			const auto interaction=xeenRegionalInteraction(original,guard.cameraValue);
			if (interaction==XeenRegionalInteraction::Ironworks || interaction==XeenRegionalInteraction::Training ||
				interaction==XeenRegionalInteraction::Temple) {
				const auto service=xeenRegionalService(original,guard.cameraValue);
				const auto site=xeenRegionalEvent(original,guard.cameraValue);
				if (!xeen_state::sameCamera(state.workingCamera,guard.cameraValue) ||
					state.workingGameFlags.values()!=guard.flagValues || state.logicalAddress.mapId!=XeenMapIdentity(28) ||
					!service || !site || state.logicalAddress.x!=guard.cameraValue.x || state.logicalAddress.y!=guard.cameraValue.y || state.logicalAddress.line!=0 ||
					state.lookupDirection!=guard.cameraValue.direction || state.instructionCount>1 ||
					!state.callStack.empty() || state.pendingRewards.hasWork() || !state.currentScript ||
					(state.pendingPresentation && state.pendingPresentation->continuation!=XeenEventPendingContinuation::Terminate))
					integrity("Service terminal continuation changed");
				script(state.currentScript->file());currentSite=site;return;
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
		dispatchedRecords.push_back(*currentSite);
	}

	void prepareGrant(std::size_t index) const {
		check();
		const auto effect=takeOrGive();
		if (!neutral(effect.first) || effect.second.mode!=21 || effect.second.value!=index+82 || !neutral(effect.third) || index>=guard.quests.size()) integrity("Regional grant operands changed");
		consume();
		if (grant || guard.quests[index]==std::numeric_limits<std::uint32_t>::max())
			throw std::logic_error("Journey grant publication unavailable");
		grant=true; grantIndex=index;
	}
	void text(const XeenEventTextFile &file) const {
		if (file.mapId==XeenMapIdentity(28)) guard.admitVertigoText(file);
		else guard.admitRegionalText(file);
	}
	void granted() const noexcept { ++guard.quests[grantIndex];guard.adoptMutationBoundary(); }
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
		if(effect.first.mode!=21 || effect.first.value!=index+82 || !neutral(effect.second) || !neutral(effect.third) || index>=guard.quests.size() || !guard.quests[index]) integrity("Regional quest take operands changed");
		takeIndex=index;consume();
	}
	void questTaken() const noexcept { --guard.quests[takeIndex];guard.adoptMutationBoundary(); }
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
	void wellFlagWritten() const noexcept { guard.recovery->worldFlag16=true;guard.adoptMutationBoundary(); }
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
	void prepareRemove(const XeenCamera &physical, std::optional<XeenObjectIdentity> selected,
		const XeenEventFile &file) const {
		check(); script(file);
		if(!std::holds_alternative<XeenEventRemove>(decodedOperation()))integrity("Regional Remove operands changed");
		consume();
		if (!xeen_state::sameCamera(physical,guard.cameraValue) || !(selected==selectedObject) || removal)
			throw std::logic_error("Journey Remove publication unavailable");
		objects=guard.s._objects; events=guard.s._events;
		if (selected) objects.insert(*selected);
		for (std::size_t i=0;i<original.records.size();++i)
			if(original.records[i].x==physical.x && original.records[i].y==physical.y) events.insert({physical.mapId,i});
		removal=true;
	}
	void removed() const noexcept { guard.s._objects.swap(objects); guard.s._events.swap(events);guard.adoptMutationBoundary(); }
private:
	static bool neutral(const XeenEventTakeOrGivePair &pair) noexcept {return pair.mode==0 && pair.value==0;}
	XeenDecodedEventOperation decodedOperation() const {
		if(!currentSite || dispatchedRecords.empty() || dispatchedRecords.back()!=*currentSite ||
			guard.s._events.count({original.mapId,*currentSite}))integrity("Event effect has no dispatched source");
		const auto result=XeenEventDecoder::decode(original.records.at(*currentSite),{original.mapId,original.resourceName,*currentSite,true});
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
	void validateRewardPrefix(const XeenPendingRewards &pending,std::size_t before) const {
		std::size_t count=0;
		if(pending.overflow() || pending.invalid())integrity("Regional reward queue is malformed");
		for(std::size_t i=0;i<before;++i) {
			const auto site=dispatchedRecords.at(i);
			if(guard.s._events.count({original.mapId,site}))continue;
			const auto result=XeenEventDecoder::decode(original.records.at(site),{original.mapId,original.resourceName,site,true});
			const auto *instruction=std::get_if<XeenDecodedEventInstruction>(&result);if(!instruction)continue;
			const auto *effect=std::get_if<XeenEventGiveEnchanted>(&instruction->operation);if(!effect)continue;
			if(count>=pending.size())integrity("Regional reward prefix loses dispatched effect");
			const auto &item=pending.at(count++);
			if(item.material!=effect->itemCode-60 || item.id!=effect->specialId || item.state!=1 || item.frame!=0)
				integrity("Regional reward item differs from dispatched operands");
		}
		if(count!=pending.size())integrity("Regional reward prefix adds an undispatched effect");
	}
	[[noreturn]] void integrity(const char *message) const { guard.failed=true; throw std::logic_error(message); }
	friend class XeenEventFlow;
	XeenEventPublication(XeenRestoreGuard &guard, const XeenEventFile &original, std::function<void()> authority,
		std::function<void()> beforeRewardEnqueue={}, std::vector<std::size_t> *history=nullptr) :
		guard(guard), original(original), authority(std::move(authority)),
		beforeRewardEnqueue(std::move(beforeRewardEnqueue)), retainedHistory(history),
		dispatchedRecords(history ? *history : std::vector<std::size_t>{}) {
		check();selectedObject=const_cast<XeenWorld &>(guard.w).selectObject(guard.cameraValue);check();
	}
	XeenRestoreGuard &guard;
	const XeenEventFile &original;
	std::function<void()> authority;
	std::function<void()> beforeRewardEnqueue;
	mutable bool grant=false, removal=false, delivered=false;
	std::vector<std::size_t> *retainedHistory;
	mutable std::vector<std::size_t> dispatchedRecords;
	mutable std::set<std::size_t> consumed;
	std::optional<XeenObjectIdentity> selectedObject;
	mutable std::size_t questFlagIndex=0, takeIndex=0;
	mutable std::size_t grantIndex=18;
	mutable std::optional<std::size_t> currentSite;
	mutable std::set<XeenObjectIdentity> objects;
	mutable std::set<XeenEventIdentity> events;
};
}
#endif
