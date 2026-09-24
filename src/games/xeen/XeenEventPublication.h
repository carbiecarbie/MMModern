#ifndef MMODERN_XEEN_EVENT_PUBLICATION_H
#define MMODERN_XEEN_EVENT_PUBLICATION_H
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenRegionalRules.h"
#include <limits>
namespace mmodern {
// A stack-bound publication capability, issued only to the live Journey continuation.
// It anticipates known effects in retained guard storage, never adopts callback state.
class XeenEventPublication {
public:
	XeenEventPublication(const XeenEventPublication &) = delete;
	XeenEventPublication &operator=(const XeenEventPublication &) = delete;
	void check() const { authority(); guard.check(); }
	void script(const XeenEventFile &file) const {
		check();
		if (file.mapId != original.mapId || file.resourcePresent != original.resourcePresent || file.records.size() != original.records.size())
			integrity("Journey event topology changed");
		for (std::size_t i=0;i<file.records.size();++i) {
			const auto &a=file.records[i]; const auto &b=original.records[i];
			if (a.x!=b.x || a.y!=b.y || a.direction!=b.direction || a.line!=b.line || a.opcode!=b.opcode || a.parameters!=b.parameters || a.fileOffset!=b.fileOffset || a.lengthField!=b.lengthField)
				integrity("Journey event record changed");
		}
	}
	void execution(const XeenEventExecutionState &state) const {
		check();
		if (guard.s.journeyContract()>=3) {
			const auto interaction=xeenRegionalInteraction(original,guard.cameraValue,guard.s.journeyContract());
			const int end=interaction==XeenRegionalInteraction::Myra ? 15 : interaction==XeenRegionalInteraction::Phirna ? 11 :
				interaction==XeenRegionalInteraction::Well ? 10 : 1;
			const std::size_t first=interaction==XeenRegionalInteraction::Myra ? 21 : interaction==XeenRegionalInteraction::Phirna ? 125 :
				interaction==XeenRegionalInteraction::Well ? 57 : 56;
			const std::size_t last=interaction==XeenRegionalInteraction::Myra ? 35 : interaction==XeenRegionalInteraction::Phirna ? 135 :
				interaction==XeenRegionalInteraction::Well ? 66 : 56;
			const std::size_t object=interaction==XeenRegionalInteraction::Myra ? 1 : interaction==XeenRegionalInteraction::Phirna ? 13 :
				interaction==XeenRegionalInteraction::Well ? 4 : 7;
			if (interaction==XeenRegionalInteraction::None || !xeen_state::sameCamera(state.workingCamera,guard.cameraValue) ||
				state.workingGameFlags.values()!=guard.flagValues || state.logicalAddress.mapId!=XeenMapIdentity(23) ||
				state.logicalAddress.x!=guard.cameraValue.x || state.logicalAddress.y!=guard.cameraValue.y || state.logicalAddress.line<0 || state.logicalAddress.line>end ||
				state.lookupDirection!=guard.cameraValue.direction || state.instructionCount>36 ||
				!state.callStack.empty() ||
				(state.selectedObject && !(*state.selectedObject==XeenObjectIdentity{23,object})) || !state.currentScript)
				integrity("Regional event continuation changed");
			if (interaction==XeenRegionalInteraction::Sign && (state.instructionCount>1 || state.pendingRewards.hasWork()))
				integrity("Regional sign continuation changed");
			if (interaction!=XeenRegionalInteraction::Myra && state.pendingRewards.hasWork())
				integrity("Regional reward producer changed");
			script(state.currentScript->file());
			const auto site=state.currentScript->findInstructionIndex(static_cast<std::uint8_t>(state.logicalAddress.x),
				static_cast<std::uint8_t>(state.logicalAddress.y),state.lookupDirection,static_cast<std::uint8_t>(state.logicalAddress.line));
			if (site && (*site<first || *site>last)) integrity("Regional event escaped admitted records");
			if (!site && state.logicalAddress.line!=end) integrity("Regional event ended outside natural closure");
			if (interaction==XeenRegionalInteraction::Myra &&
				(state.rewardPhase==XeenRewardPhase::Running || state.rewardPhase==XeenRewardPhase::Warning) &&
				state.pendingRewards.hasWork()) {
				if ((site && *site<31) || state.pendingRewards.size()!=rewardProducerCount(site ? *site : 36) ||
					state.pendingRewards.overflow() || state.pendingRewards.invalid())
					integrity("Regional reward production prefix changed");
			}
			currentSite=site;
			return;
		}
		if (!xeen_state::sameCamera(state.workingCamera,guard.cameraValue) || state.workingGameFlags.values()!=guard.flagValues ||
			state.logicalAddress.mapId!=XeenMapIdentity(20) || state.logicalAddress.x!=5 || state.logicalAddress.y!=14 ||
			state.logicalAddress.line<0 || state.logicalAddress.line>5 || !state.callStack.empty() ||
			(state.selectedObject && !(*state.selectedObject==XeenObjectIdentity{20,1})))
			integrity("Journey event continuation changed");
		if (!state.currentScript) integrity("Journey event script missing");
		script(state.currentScript->file());
	}
	void prepareGrant(std::size_t index) const {
		check();
		const bool phirna=(guard.s.journeyContract()>=6 && guard.s.journeyContract()<=8) && currentSite==131 && index==17;
		if (guard.s.journeyContract()>=3 && !phirna) integrity("Regional grant site changed");
		if ((!phirna && index!=18) || grant || guard.quests[index]==std::numeric_limits<std::uint32_t>::max())
			throw std::logic_error("Journey grant publication unavailable");
		grant=true; grantIndex=index;
	}
	void text(const XeenEventTextFile &file) const {
		if (guard.s.journeyContract()==8 && file.mapId==XeenMapIdentity(28)) guard.admitVertigoText(file);
		else if (guard.s.journeyContract()>=6 && guard.s.journeyContract()<=8) guard.admitRegionalText(file);
		else check();
	}
	void granted() const noexcept { ++guard.quests[grantIndex];guard.adoptMutationBoundary(); }
	void prepareQuestFlag(bool value) const {
		check();
		if ((guard.s.journeyContract()<6 || guard.s.journeyContract()>8) || currentSite!=(value ? 26u : 30u)) integrity("Regional quest flag site changed");
	}
	void questFlagWritten(bool value) const noexcept { guard.questFlags[2]=value;guard.adoptMutationBoundary(); }
	void prepareQuestTake(std::size_t index) const {
		check();
		if ((guard.s.journeyContract()<6 || guard.s.journeyContract()>8) || currentSite!=29 || index!=17 || !guard.quests[17]) integrity("Regional quest take site changed");
	}
	void questTaken() const noexcept { --guard.quests[17];guard.adoptMutationBoundary(); }
	void voiceCue(std::uint8_t index) const {
		check();
		if ((guard.s.journeyContract()<6 || guard.s.journeyContract()>8) || currentSite!=58 || index!=2) integrity("Regional voice cue site changed");
	}
	void prepareWellHp(std::uint8_t owner,std::int16_t before,std::int16_t after) const {
		check();
		if ((guard.s.journeyContract()<6 || guard.s.journeyContract()>8) || currentSite!=60 || owner>=30 ||
			guard.characters[owner].currentHp!=before || std::int32_t(before)+25!=after)
			integrity("Regional well HP site or preimage changed");
	}
	void wellHpWritten(std::uint8_t owner,std::int16_t after) const noexcept { guard.characters[owner].currentHp=after;guard.adoptMutationBoundary(); }
	void prepareWellFlag() const {
		check();
		if ((guard.s.journeyContract()<6 || guard.s.journeyContract()>8) || currentSite!=63 || !guard.recovery)
			integrity("Regional well flag site changed");
	}
	void wellFlagWritten() const noexcept { guard.recovery->worldFlag16=true;guard.adoptMutationBoundary(); }
	void prepareRewardEnqueue(const XeenEventExecutionState &state,const XeenItem &item) const {
		check();
		if ((guard.s.journeyContract()<6 || guard.s.journeyContract()>8) || !currentSite || *currentSite<31 || *currentSite>35 ||
			guard.s._events.count({XeenMapIdentity(23),*currentSite}) ||
			state.rewardPhase!=XeenRewardPhase::Running || state.pendingRewards.size()!=rewardProducerCount(*currentSite) ||
			state.pendingRewards.overflow() || state.pendingRewards.invalid() ||
			item.material!=10 || item.id!=37 || item.state!=1 || item.frame!=0)
			integrity("Regional reward production changed");
		for(std::size_t i=0;i<state.pendingRewards.size();++i) if (!sameReward(state.pendingRewards.at(i)))
			integrity("Regional reward production prefix changed");
		if (beforeRewardEnqueue) { beforeRewardEnqueue(); check(); }
	}
	XeenRewardReceipt deliverRewards(XeenPendingRewards &pending,XeenPartyState &party,
		std::optional<std::size_t> preferred) const {
		check();
		if ((guard.s.journeyContract()<6 || guard.s.journeyContract()>8) ||
			xeenRegionalInteraction(original,guard.cameraValue,guard.s.journeyContract())!=XeenRegionalInteraction::Myra ||
			preferred || pending.size()!=rewardProducerCount(36) || pending.overflow() || pending.invalid())
			integrity("Regional reward delivery changed");
		for (std::size_t i=0;i<pending.size();++i) if (!sameReward(pending.at(i)))
			integrity("Regional reward item changed");
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
		const bool phirna=(guard.s.journeyContract()>=6 && guard.s.journeyContract()<=8) && currentSite==132;
		if (guard.s.journeyContract()>=3 && !phirna) integrity("Regional Remove site changed");
		const XeenObjectIdentity expected{phirna ? 23u : 20u,phirna ? 13u : 1u};
		if (!xeen_state::sameCamera(physical,guard.cameraValue) || (selected && !(*selected==expected)) || removal)
			throw std::logic_error("Journey Remove publication unavailable");
		objects=guard.s._objects; events=guard.s._events;
		if (selected) objects.insert(*selected);
		for (std::size_t i=phirna ? 125u : 1u;i<=(phirna ? 135u : 5u);++i) events.insert({physical.mapId,i});
		removal=true;
	}
	void removed() const noexcept { guard.s._objects.swap(objects); guard.s._events.swap(events);guard.adoptMutationBoundary(); }
private:
	bool sameReward(const XeenItem &item) const noexcept {
		return item.material==10 && item.id==37 && item.state==1 && item.frame==0;
	}
	std::size_t rewardProducerCount(std::size_t before) const noexcept {
		std::size_t count=0;
		for(std::size_t site=31;site<before && site<=35;++site)
			count+=!guard.s._events.count({XeenMapIdentity(23),site});
		return count;
	}
	[[noreturn]] void integrity(const char *message) const { guard.failed=true; throw std::logic_error(message); }
	friend class XeenEventFlow;
	XeenEventPublication(XeenRestoreGuard &guard, const XeenEventFile &original, std::function<void()> authority,
		std::function<void()> beforeRewardEnqueue={}) :
		guard(guard), original(original), authority(std::move(authority)),
		beforeRewardEnqueue(std::move(beforeRewardEnqueue)) { check(); }
	XeenRestoreGuard &guard;
	const XeenEventFile &original;
	std::function<void()> authority;
	std::function<void()> beforeRewardEnqueue;
	mutable bool grant=false, removal=false;
	mutable std::size_t grantIndex=18;
	mutable std::optional<std::size_t> currentSite;
	mutable std::set<XeenObjectIdentity> objects;
	mutable std::set<XeenEventIdentity> events;
};
}
#endif
