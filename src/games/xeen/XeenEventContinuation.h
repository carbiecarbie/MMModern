#ifndef MMODERN_XEEN_EVENT_CONTINUATION_H
#define MMODERN_XEEN_EVENT_CONTINUATION_H
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenStateEquality.h"
#include <tuple>
namespace mmodern {
// Value preimage of an interpreter-issued suspension. Flow owns the lease and
// generation; this guard binds every continuation field before a resume.
class XeenEventContinuation {
public:
	explicit XeenEventContinuation(const XeenEventExecutionState &state,bool regionalRewards=false) : expected(state) {
		if(!state.currentScript || !state.pendingPresentation || (!regionalRewards && (state.pendingRewards.hasWork() ||
			state.rewardPhase!=XeenRewardPhase::Running || state.rewardReceipt.count ||
			state.rewardReceipt.delivered || state.rewardReceipt.lost || state.rewardReceipt.overflow ||
			state.rewardReceipt.invalid || state.rewardReceipt.discarded ||
			state.rewardReceipt.discardReason!=XeenRewardDiscard::None)) ||
			state.instructionCount>XeenEventInterpreter::kMaximumInstructions ||
			state.callStack.size()>XeenEventInterpreter::kMaximumCallDepth)
			throw std::logic_error("Unsupported detached Event continuation");
	}
	void check(const XeenEventExecutionState &state) const {
		const auto &a=state;const auto &b=expected;
		bool same=queue(a.pendingRewards,b.pendingRewards) && a.rewardPhase==b.rewardPhase &&
			a.preferredRewardRecipient==b.preferredRewardRecipient && address(a.logicalAddress,b.logicalAddress) &&
			a.lookupDirection==b.lookupDirection && xeen_state::sameCamera(a.workingCamera,b.workingCamera) &&
			a.selectedObject==b.selectedObject && a.activeCharacterIndex==b.activeCharacterIndex &&
			a.workingGameFlags.values()==b.workingGameFlags.values() &&
			a.instructionCount==b.instructionCount && a.missingInstructionPolicy==b.missingInstructionPolicy &&
			optional(a.pendingTransferSource,b.pendingTransferSource,source) &&
			a.currentScript && sameScript(a.currentScript->file(),b.currentScript->file()) &&
			a.callStack.size()==b.callStack.size() &&
			optional(a.pendingPresentation,b.pendingPresentation,presentation);
		for(unsigned i=0;same && i<a.callStack.size();++i)same=address(a.callStack[i].returnAddress,b.callStack[i].returnAddress);
		const auto &r=a.rewardReceipt;const auto &s=b.rewardReceipt;
		same=same && r.count==s.count && r.delivered==s.delivered && r.lost==s.lost &&
			r.overflow==s.overflow && r.invalid==s.invalid && r.discarded==s.discarded && r.discardReason==s.discardReason;
		for(unsigned i=0;same && i<r.entries.size();++i) {
			const auto &x=r.entries[i];const auto &y=s.entries[i];
			same=x.owner==y.owner && x.loss==y.loss && x.item.material==y.item.material &&
				x.item.id==y.item.id && x.item.state==y.item.state && x.item.frame==y.item.frame;
		}
		if(!same)throw std::logic_error("Detached Event continuation preimage changed");
	}
	static bool sameScript(const XeenEventFile &a,const XeenEventFile &b) {
		if(a.mapId!=b.mapId || a.resourceName!=b.resourceName || a.resourcePresent!=b.resourcePresent || a.records.size()!=b.records.size())return false;
		for(unsigned i=0;i<a.records.size();++i) {
			const auto &x=a.records[i];const auto &y=b.records[i];
			if(std::tie(x.x,x.y,x.direction,x.line,x.opcode,x.parameters,x.fileOffset,x.lengthField)!=
				std::tie(y.x,y.y,y.direction,y.line,y.opcode,y.parameters,y.fileOffset,y.lengthField))return false;
		}
		return true;
	}
private:
	XeenEventExecutionState expected;
	static bool queue(const XeenPendingRewards &a,const XeenPendingRewards &b) {
		if(a.size()!=b.size() || a.overflow()!=b.overflow() || a.invalid()!=b.invalid())return false;
		for(std::size_t i=0;i<a.size();++i) {
			const auto &x=a.at(i),&y=b.at(i);
			if(std::tie(x.material,x.id,x.state,x.frame)!=std::tie(y.material,y.id,y.state,y.frame))return false;
		}
		return true;
	}
	static bool address(const XeenEventExecutionAddress &a,const XeenEventExecutionAddress &b) {
		return std::tie(a.mapId,a.x,a.y,a.line)==std::tie(b.mapId,b.x,b.y,b.line);
	}
	static bool source(const XeenEventSourceLocation &a,const XeenEventSourceLocation &b) {
		return std::tie(a.mapId,a.resourceName,a.fileOffset,a.x,a.y,a.direction,a.line,a.opcode,a.recordIndex)==
			std::tie(b.mapId,b.resourceName,b.fileOffset,b.x,b.y,b.direction,b.line,b.opcode,b.recordIndex);
	}
	template<class T,class Equal> static bool optional(const std::optional<T> &a,const std::optional<T> &b,Equal equal) {
		return bool(a)==bool(b) && (!a || equal(*a,*b));
	}
	static bool conditional(const XeenEventConditional &a,const XeenEventConditional &b) {
		return std::tie(a.comparison,a.action,a.value,a.targetLine)==std::tie(b.comparison,b.action,b.value,b.targetLine);
	}
	static bool presentation(const XeenEventPendingPresentation &a,const XeenEventPendingPresentation &b) {
		const auto &x=a.request;const auto &y=b.request;
		if(a.continuation!=b.continuation || !optional(a.conditional,b.conditional,conditional) ||
			std::tie(x.kind,x.response,x.mapId,x.textIndex,x.text,x.layoutValue,x.verbIndex,x.refusal,x.title)!=
			std::tie(y.kind,y.response,y.mapId,y.textIndex,y.text,y.layoutValue,y.verbIndex,y.refusal,y.title) ||
			!source(x.source,y.source) || x.members.size()!=y.members.size() ||
			!optional(x.npc,y.npc,[](const auto &p,const auto &q) {
				return std::tie(p.titleTextIndex,p.bodyTextIndex,p.portraitId,p.confirmationMode,p.targetLine)==
					std::tie(q.titleTextIndex,q.bodyTextIndex,q.portraitId,q.confirmationMode,q.targetLine);
			}))return false;
		for(unsigned i=0;i<x.members.size();++i) {
			const auto &p=x.members[i];const auto &q=y.members[i];
			if(std::tie(p.partyIndex,p.rosterId,p.name,p.eligible)!=std::tie(q.partyIndex,q.rosterId,q.name,q.eligible))return false;
		}
		return true;
	}
};
}
#endif
