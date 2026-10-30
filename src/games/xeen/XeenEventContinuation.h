#ifndef MMODERN_XEEN_EVENT_CONTINUATION_H
#define MMODERN_XEEN_EVENT_CONTINUATION_H
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenStateEquality.h"
#include <tuple>
namespace mmodern {
// Flow-owned dispatch history survives suspension and later opcode deletion.
// Keep the effective opcode that actually ran, not a reread of its resource.
struct XeenEventDispatchRecord {
	std::optional<std::size_t> record;
	std::uint8_t opcode=0;
};
// Value preimage of an interpreter-issued suspension. Flow owns the lease and
// generation; this guard binds every continuation field before a resume.
class XeenEventContinuation {
public:
	explicit XeenEventContinuation(const XeenEventExecutionState &state,bool regionalRewards=false) : expected(state) {
		if(!state.currentScript || !state.pendingPresentation || (!regionalRewards && (state.treasureChanged() ||
			state.reward.phase!=XeenRewardPhase::Running || state.reward.receipt.count ||
			state.reward.receipt.delivered || state.reward.receipt.lost || state.reward.receipt.discarded ||
			state.reward.receipt.discardReason!=XeenRewardDiscard::None)) ||
			state.instructionCount>XeenEventInterpreter::kMaximumInstructions ||
			state.callStack.size()>XeenEventInterpreter::kMaximumCallDepth)
			throw std::logic_error("Unsupported detached Event continuation");
	}
	void check(const XeenEventExecutionState &state) const {
		const auto &a=state;const auto &b=expected;
		bool same=a.forceText==b.forceText && a.ending==b.ending && optional(a.portraitDamage,b.portraitDamage,[](const auto &x,const auto &y){return x==y;}) && optional(a.portraitRandom,b.portraitRandom,[](const auto &x,const auto &y){return x==y;}) && a.sparkledOwners==b.sparkledOwners && optional(a.portraitProgress,b.portraitProgress,[](const auto &x,const auto &y){return x==y;}) && a.treasure==b.treasure && a.stash==b.stash && a.stashed==b.stashed && a.reward.phase==b.reward.phase && a.reward.cursor==b.reward.cursor &&
			optional(a.cdSpeech,b.cdSpeech,speech) &&
			optional(a.rewardRandom,b.rewardRandom,[](const auto &x,const auto &y){return x==y;}) &&
			optional(a.rewardItem,b.rewardItem,[](const auto &x,const auto &y){return x==y;}) &&
			a.rewardPair==b.rewardPair && a.generatingReward==b.generatingReward &&
			a.inputClearCount==b.inputClearCount && a.inputClearSite==b.inputClearSite &&
            a.scriptExecuted==b.scriptExecuted && a.otherFacing==b.otherFacing && a.tailDone==b.tailDone &&
            a.redrawDone==b.redrawDone &&
            a.prefixLight==b.prefixLight &&
			address(a.logicalAddress,b.logicalAddress) &&
			a.lookupDirection==b.lookupDirection && xeen_state::sameCamera(a.workingCamera,b.workingCamera) &&
			a.continuedAtDestination==b.continuedAtDestination &&
			a.selectedObject==b.selectedObject && a.activeCharacterIndex==b.activeCharacterIndex && a.entireParty==b.entireParty &&
			a.mirrorDestination==b.mirrorDestination && a.scriptValues==b.scriptValues && a.giveTake==b.giveTake && a.lock==b.lock &&
			a.workingGameFlags.values()==b.workingGameFlags.values() &&
			a.instructionCount==b.instructionCount && a.missingInstructionPolicy==b.missingInstructionPolicy &&
			optional(a.pendingTransferSource,b.pendingTransferSource,source) &&
			a.currentScript && sameScript(a.currentScript->file(),b.currentScript->file()) &&
			a.callStack.size()==b.callStack.size() &&
			optional(a.pendingPresentation,b.pendingPresentation,presentation);
		for(unsigned i=0;same && i<a.callStack.size();++i)same=address(a.callStack[i].returnAddress,b.callStack[i].returnAddress);
		const auto &r=a.reward.receipt;const auto &s=b.reward.receipt;
		same=same && r.count==s.count && r.delivered==s.delivered && r.lost==s.lost && r.gold==s.gold && r.gems==s.gems &&
			r.discarded==s.discarded && r.discardReason==s.discardReason;
		for(unsigned i=0;same && i<r.entries.size();++i) {
			const auto &x=r.entries[i];const auto &y=s.entries[i];
			same=x.owner==y.owner && x.loss==y.loss && x.category==y.category && x.quest==y.quest && x.item.material==y.item.material &&
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
	static bool speech(const XeenEventCdSpeech &a,const XeenEventCdSpeech &b) {
		return std::tie(a.track,a.start,a.finish)==std::tie(b.track,b.start,b.finish);
	}
	XeenEventExecutionState expected;
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
		if(x.forceText!=y.forceText || x.entranceAction!=y.entranceAction || x.arena!=y.arena)return false;
		if(x.mirrors!=y.mirrors || x.alternate!=y.alternate || !optional(x.word,y.word,[](const auto &p,const auto &q){return std::tie(p.inputType,p.targetLine,p.alternateText,p.titleText)==std::tie(q.inputType,q.targetLine,q.alternateText,q.titleText);}) || a.continuation!=b.continuation || x.rewardWaitTicks!=y.rewardWaitTicks || !optional(x.portrait,y.portrait,[](const auto &p,const auto &q){return p==q;}) || !optional(a.conditional,b.conditional,conditional) ||
			!optional(x.cdSpeech,y.cdSpeech,speech) ||
			std::tie(x.kind,x.response,x.mapId,x.textIndex,x.text,x.layoutValue,x.verbIndex,x.refusal,x.title,x.notEnough,x.lockPicked)!=
			std::tie(y.kind,y.response,y.mapId,y.textIndex,y.text,y.layoutValue,y.verbIndex,y.refusal,y.title,y.notEnough,y.lockPicked) ||
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
