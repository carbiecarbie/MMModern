#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenEventPublication.h"

#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenSkills.h"
#include "games/xeen/XeenCombatRules.h"

#include <limits>
#include <algorithm>
#include <set>
#include <tuple>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mmodern {
namespace {

using MissingInstructionPolicy = XeenEventMissingInstructionPolicy;
using CallFrame = XeenEventCallFrame;

bool validDirection(XeenDirection direction) {
	switch (direction) {
	case XeenDirection::North:
	case XeenDirection::East:
	case XeenDirection::South:
	case XeenDirection::West:
		return true;
	}
	return false;
}

XeenEventExecutionError error(XeenEventExecutionErrorKind kind,
		std::string message, std::size_t instructionCount,
		const XeenEventExecutionAddress &logicalAddress,
		std::optional<XeenEventSourceLocation> source = std::nullopt,
		std::optional<XeenEventExecutionAddress> requestedTarget = std::nullopt) {
	return {kind, std::move(message), instructionCount, logicalAddress,
		std::move(source), std::move(requestedTarget)};
}

XeenEventExecutionCompleted completed(const XeenCamera &camera,
		const XeenGameFlags &gameFlags, std::size_t instructionCount) {
	return {camera, gameFlags, instructionCount};
}

XeenEventExecutionErrorKind executionKind(XeenEventDecodeErrorKind kind) {
	switch (kind) {
	case XeenEventDecodeErrorKind::MalformedInstruction:
		return XeenEventExecutionErrorKind::MalformedInstruction;
	case XeenEventDecodeErrorKind::UnsupportedOpcode:
		return XeenEventExecutionErrorKind::UnsupportedOpcode;
	case XeenEventDecodeErrorKind::UnsupportedOperand:
		return XeenEventExecutionErrorKind::UnsupportedOperand;
	}
	return XeenEventExecutionErrorKind::MalformedInstruction;
}

bool compare(std::uint32_t actual, std::uint32_t expected,
		XeenEventComparison comparison) {
	switch (comparison) {
	case XeenEventComparison::GreaterOrEqual:
		return actual >= expected;
	case XeenEventComparison::Equal:
		return actual == expected;
	case XeenEventComparison::LessOrEqual:
		return actual <= expected;
	}
	return false;
}

bool responseMatches(XeenPresentationResponseRequirement requirement,
		XeenPresentationResponse response) {
	switch (requirement) {
	case XeenPresentationResponseRequirement::WordInput:
		return std::holds_alternative<XeenWordInput>(response.value);
	case XeenPresentationResponseRequirement::CharacterSelection:
		return std::holds_alternative<SelectedCharacter>(response.value) ||
			std::holds_alternative<CharacterSelectionCancelled>(response.value);
	case XeenPresentationResponseRequirement::Presented:
		return response == XeenPresentationResponse::Presented;
	case XeenPresentationResponseRequirement::Acknowledgment:
		return response == XeenPresentationResponse::Acknowledged;
	case XeenPresentationResponseRequirement::YesNo:
		return response == XeenPresentationResponse::Yes ||
			response == XeenPresentationResponse::No;
	}
	return false;
}

XeenPresentationKind presentationKind(XeenEventDisplayKind kind) {
	switch (kind) {
	case XeenEventDisplayKind::Centered:
		return XeenPresentationKind::CenteredMessage;
	case XeenEventDisplayKind::DoorLabelReduced:
		return XeenPresentationKind::SceneLabelReduced;
	case XeenEventDisplayKind::DoorLabelNormal:
		return XeenPresentationKind::SceneLabelNormal;
	case XeenEventDisplayKind::SignLabel:
		return XeenPresentationKind::SceneLabelSign;
	case XeenEventDisplayKind::SignLabelReduced:
		return XeenPresentationKind::SceneLabelSignReduced;
	case XeenEventDisplayKind::BottomWindow:
		return XeenPresentationKind::BottomWindowMessage;
	case XeenEventDisplayKind::BottomWindowTwoLines:
		return XeenPresentationKind::BottomWindowTwoLines;
	case XeenEventDisplayKind::MainWindow:
		return XeenPresentationKind::MainWindowMessage;
	}
	return XeenPresentationKind::CenteredMessage;
}

// Detached city interactions publish only complete supported effect groups.
// Inspect every reachable suffix (including both answers and call returns)
// before a prefix can Spawn, disable an Event or change a flag. This is a
// capability query over resource instructions, never a list of admitted cells.
std::optional<XeenEventExecutionError> preflightCityEvent(XeenWorld &world,
		const XeenEventScript &script, const XeenCamera &camera) {
	using Address=std::tuple<int,int,int>;
	using Path=std::vector<Address>;
	// Include return addresses: a service called from a subroutine exits the
	// entire Event, so its caller's suffix must never be inspected or executed.
	std::vector<Path> pending{{{camera.x,camera.y,0}}};
	std::set<Path> visited;
	while(!pending.empty()) {
		auto path=std::move(pending.back());pending.pop_back();
		if(!visited.insert(path).second)continue;
		if(visited.size()>4096 || path.size()>XeenEventInterpreter::kMaximumCallDepth+1)
			return error(XeenEventExecutionErrorKind::InstructionLimitExceeded,
				"Event preflight exceeds the supported control-flow bound",0,{camera.mapId,camera.x,camera.y,0});
		const auto address=path.back();path.pop_back();
		const auto [x,y,line]=address;
		const auto enqueue=[&](int nx,int ny,int nl) {auto next=path;next.emplace_back(nx,ny,nl);pending.push_back(std::move(next));};
		if(x<0 || x>255 || y<0 || y>255 || line<0 || line>255)
			return error(XeenEventExecutionErrorKind::InvalidJumpTarget,
				"Event preflight address is outside the lookup domain",0,{camera.mapId,x,y,line});
		const auto site=script.findInstructionIndex(x,y,camera.direction,line);
		if(!site)continue; // Runtime distinguishes natural closure from bad jumps.
		const auto effective=world.effectiveEvent({camera.mapId,*site},script.records()[*site]);
		const auto decoded=XeenEventDecoder::decode(effective,
			{camera.mapId,script.file().resourceName,*site,true,effective.opcode!=script.records()[*site].opcode});
		const XeenEventExecutionAddress logical{camera.mapId,x,y,line};
		if(const auto *bad=std::get_if<XeenEventDecodeError>(&decoded))
			return error(executionKind(bad->kind),bad->message,0,logical,bad->source);
		const auto &instruction=std::get<XeenDecodedEventInstruction>(decoded);
		const auto &op=instruction.operation;
		const auto unsupported=[&](const char *message) {
			return error(XeenEventExecutionErrorKind::UnsupportedOperand,message,0,logical,instruction.source);
		};
		if(std::holds_alternative<XeenEventExit>(op))continue;
		if(std::holds_alternative<XeenEventReturn>(op)) {if(!path.empty())pending.push_back(std::move(path));continue;}
		if(const auto *service=std::get_if<XeenEventTownService>(&op)) {
			if(service->action!=0 && service->action!=1 && service->action!=2 && service->action!=3 && service->action!=4 && service->action!=5)
				return unsupported("Dependent service effects are not supported yet");
			continue;
		}
        // Travel resolves dynamically inside the detached candidate. Continue starts
        // a fresh destination run, rather than a suffix of this logical script.
        if(std::holds_alternative<XeenEventTeleportAndExit>(op) ||
           std::holds_alternative<XeenEventTeleportAndContinue>(op))continue;
		if(std::holds_alternative<XeenEventGiveEnchanted>(op) ||
			std::holds_alternative<XeenEventRemove>(op))
			return unsupported("Event effect is not supported yet in this context");
		if(const auto *npc=std::get_if<XeenEventNpc>(&op)) {
			if(npc->confirmationMode>2)return unsupported("NPC mode is not supported yet");
			if(npc->confirmationMode==0)enqueue(x,y,npc->targetLine);
		}
		if(const auto *condition=std::get_if<XeenEventConditional>(&op)) {
			if(condition->action==107 || (condition->action==44 && condition->value>1) ||
				((condition->action==13 || condition->action==99) && condition->value>=18) || (condition->action==20 && condition->value>255) ||
				(condition->action==104 && !XeenCloudsQuestFlags::validIndex(condition->value)))
				return unsupported("Event condition operand is not supported yet");
			enqueue(x,y,condition->targetLine);
		}
		if(const auto *word=std::get_if<XeenEventConfirmWord>(&op))enqueue(x,y,word->targetLine);
		if(const auto *call=std::get_if<XeenEventCallEvent>(&op)) {
			path.emplace_back(x,y,line+1);enqueue(call->x,call->y,call->line);continue;
		}
		if(const auto *jump=std::get_if<XeenEventJumpRandom>(&op))enqueue(x,y,jump->line);
		if(const auto *flag=std::get_if<XeenEventIfMapFlag>(&op))enqueue(x,y,flag->line);
		if(const auto *set=std::get_if<XeenEventSetCharacter>(&op))
			if(static_cast<std::int8_t>(set->selector)>7)return unsupported("SetChar character selection is not supported yet");
		// Statically refused value modes (see the runtime giveTake owners).
		const auto takeSupported=[](const XeenEventTakeOrGivePair &p) {
			if(p.mode && !p.value && p.mode!=104)return false; // HowMuch
			return p.mode!=8 && p.mode!=25 && p.mode!=76 && p.mode!=107;
		};
		const auto giveSupported=[](const XeenEventTakeOrGivePair &p) {
			if(p.mode==13)return p.value<18;
			if(p.mode==21)return p.value<60 || p.value>=82;
			return p.mode!=25 && p.mode!=67 && p.mode!=82 && p.mode!=107;
		};
		if(const auto *give=std::get_if<XeenEventTakeOrGive>(&op)) {
			if(!takeSupported(give->first) || !giveSupported(give->second))
				return unsupported("Dependent TakeOrGive effects are not supported yet");
		}
		if(const auto *test=std::get_if<XeenEventTakeOrGiveTest>(&op)) {
			if(!giveSupported(test->opcode==0x1d?test->third:test->second) || test->first.mode==107)
				return unsupported("Dependent TakeOrGive effects are not supported yet");
		}
		if(const auto *give=std::get_if<XeenEventGiveMulti>(&op)) {
			for(const auto &p:{give->first,give->second,give->third})
				if(!(p.mode==0 && p.value==0) && (p.mode==67 || ((p.mode==66 || p.mode==100 || p.mode==101 || p.mode==106) && !p.value) ||
					(p.mode==66 && p.value>7) || (p.mode!=34 && p.mode!=35 && p.mode!=66 && p.mode!=100 && p.mode!=101 && p.mode!=106 && !giveSupported(p))))
					return unsupported("GiveMulti effect is not supported yet");
			const auto selected=world.selectObject(camera);
			if(selected && selected->recordIndex!=0) {
				const auto sprite=world.effectiveObject(*selected).resourceId;
				if(sprite==16 || sprite==58 || sprite==73)return unsupported("GiveMulti locked chest is not supported yet");
			}
		}
		if(const auto *move=std::get_if<XeenEventMoveObject>(&op)) {
			try {static_cast<void>(world.effectiveObject({camera.mapId,move->record}));}
			catch(const std::exception &) {return unsupported("MoveObj original record is unavailable");}
		}
		if(const auto *spawn=std::get_if<XeenEventSpawn>(&op)) {
			if(spawn->slot>=XeenActorApproach::kCapacity)
				return unsupported("Spawn operands are not supported yet");
		}
		if(const auto *set=std::get_if<XeenEventSetVar>(&op)) {
			if((set->mode==84 && set->value>3) || (set->mode==74 && (set->value&0xff)>1) || (set->mode==25 && (set->value&0xffff)>=1440) ||
				(set->mode==76 && (set->value&0xffff)>=100) || (set->mode>=3 && set->mode<=5))
				return unsupported("SetVar mode is not supported yet");
		}
		enqueue(x,y,line+1);
	}
	return {};
}

} // namespace

bool xeenResponseMatches(XeenPresentationResponseRequirement requirement,
		XeenPresentationResponse response) {
	return responseMatches(requirement, response);
}

XeenEventExecutionResult XeenEventInterpreter::execute(
		const XeenCamera &initialCamera, XeenPartyState &partyState,
		const XeenGameFlags &gameFlags, XeenWorld &world,
		const ScriptProvider &scriptProvider) const {
	const XeenEventExecutionStepResult result = begin(initialCamera, partyState,
		gameFlags, world, scriptProvider, {});
	if (const auto *completedResult = std::get_if<XeenEventExecutionCompleted>(&result))
		return *completedResult;
	if (const auto *executionError = std::get_if<XeenEventExecutionError>(&result))
		return *executionError;
	const auto &suspended = std::get<XeenEventExecutionSuspended>(result);
	return error(XeenEventExecutionErrorKind::PresentationRequired,
		"event execution requires a presentation response",
		suspended.state.instructionCount, suspended.state.logicalAddress,
		suspended.request.source);
}

XeenEventExecutionStepResult XeenEventInterpreter::begin(
		const XeenCamera &initialCamera, XeenPartyState &partyState,
		const XeenGameFlags &gameFlags, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider,
		std::uint8_t initialLine, const XeenEventPublication *publication) const {
	if (world.sessionState().journey() && !publication) throw std::logic_error("Journey event requires live continuation authority");
	if (publication) publication->check();
	XeenEventExecutionState state;
	state.logicalAddress = {initialCamera.mapId, initialCamera.x, initialCamera.y, initialLine};
	state.lookupDirection = initialCamera.direction;
	state.workingCamera = initialCamera;
	state.workingGameFlags = gameFlags;
	if (partyState.party.size()) state.activeCharacterIndex = 0;
	if (!scriptProvider) {
		return error(XeenEventExecutionErrorKind::ScriptLoadFailed,
			"script provider is absent", 0, state.logicalAddress);
	}
	const bool city=world.regionalJourney() && initialCamera.mapId==XeenMapIdentity(28);
	if (!world.eventCameraAdmitted(initialCamera)) {
		return error(XeenEventExecutionErrorKind::InvalidInitialCamera,
			"initial camera is outside the supported Xeen map domain", 0,
			state.logicalAddress);
	}

	try {
		static_cast<void>(world.map(initialCamera.mapId));
	} catch (const std::exception &exception) {
		return error(XeenEventExecutionErrorKind::MapLoadFailed,
			std::string("failed to load initial map: ") + exception.what(), 0,
			state.logicalAddress);
	}

	try {
		state.currentScript.emplace(scriptProvider(initialCamera.mapId));
		if (publication) publication->script(state.currentScript->file());
	} catch (const std::exception &exception) {
		return error(XeenEventExecutionErrorKind::ScriptLoadFailed,
			std::string("failed to load initial event script: ") + exception.what(),
			0, state.logicalAddress);
	}
	if (state.currentScript->file().mapId != initialCamera.mapId) {
		return error(XeenEventExecutionErrorKind::ScriptMapMismatch,
			"event script map ID differs from the requested map", 0,
			state.logicalAddress);
	}
	try {
		state.selectedObject = world.selectObject(initialCamera);
	} catch (const std::exception &exception) {
		return error(XeenEventExecutionErrorKind::ObjectLoadFailed,
			std::string("failed to resolve interaction object: ") + exception.what(),
			0, state.logicalAddress);
	}
	if(city) {
		if(const auto refused=preflightCityEvent(world,*state.currentScript,initialCamera))return *refused;
	}
	return run(std::move(state), std::nullopt, partyState, world,
		scriptProvider, textProvider, publication);
}

XeenEventExecutionStepResult XeenEventInterpreter::resume(
		XeenEventExecutionState state, XeenPresentationResponse response,
		XeenPartyState &partyState, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider, const XeenEventPublication *publication) const {
	if (world.sessionState().journey() && !publication) throw std::logic_error("Journey event requires live continuation authority");
	if (publication) publication->check();
	return run(std::move(state), response, partyState, world,
		scriptProvider, textProvider, publication);
}

XeenEventExecutionStepResult XeenEventInterpreter::run(
		XeenEventExecutionState state,
		std::optional<XeenPresentationResponse> response,
		XeenPartyState &partyState, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider, const XeenEventPublication *publication) const {
	if (world.sessionState().journey() && !publication) throw std::logic_error("Journey event requires live continuation authority");
	if (publication) publication->check();
	try {
		auto result = runInstructions(state, response, partyState, world, scriptProvider, textProvider, publication);
		if (auto *failure = std::get_if<XeenEventExecutionError>(&result)) {
			xeenDiscardRewards(state.pendingRewards, state.rewardReceipt, XeenRewardDiscard::ExecutionError);
			failure->rewards = state.rewardReceipt;
		}
		return result;
	} catch (const std::exception &e) {
		xeenDiscardRewards(state.pendingRewards, state.rewardReceipt, XeenRewardDiscard::PresentationFailure);
		auto failure = error(XeenEventExecutionErrorKind::PresentationFailed, e.what(),
			state.instructionCount, state.logicalAddress);
		failure.rewards = state.rewardReceipt;
		return failure;
	}
}

XeenEventExecutionStepResult XeenEventInterpreter::runInstructions(
		XeenEventExecutionState &state,
		std::optional<XeenPresentationResponse> response,
		XeenPartyState &partyState, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider, const XeenEventPublication *publication) const {
	if (world.sessionState().journey() && !publication) throw std::logic_error("Journey event requires live continuation authority");
	if (publication) publication->check();
	auto &logical = state.logicalAddress;
	auto &workingCamera = state.workingCamera;
	auto &workingGameFlags = state.workingGameFlags;
	auto &script = state.currentScript;
	auto &callStack = state.callStack;
	auto &instructionCount = state.instructionCount;
	auto &missingPolicy = state.missingInstructionPolicy;
	auto &pendingTransferSource = state.pendingTransferSource;
	unsigned generationBudget=64;
    const auto recipients=[&] {
        if(!partyState.party.size() || (!state.entireParty && (!state.activeCharacterIndex || *state.activeCharacterIndex>=partyState.party.size())))
            throw std::invalid_argument("Portrait effect has no selected recipient");
        return state.entireParty?(1u<<partyState.party.size())-1:1u<<*state.activeCharacterIndex;
    };
    const auto portraitRequest=[&](XeenEventPortraitStage stage,unsigned owner,unsigned frame,const XeenEventSourceLocation &source) -> XeenEventExecutionStepResult {
        XeenPresentationRequest request;request.kind=XeenPresentationKind::PortraitEffect;
        request.response=XeenPresentationResponseRequirement::Presented;request.mapId=logical.mapId;request.source=source;
        request.portrait=XeenEventPortraitEffect{stage,owner,frame};
        state.pendingPresentation=XeenEventPendingPresentation{request,XeenEventPendingContinuation::PortraitEffect,{}};
        return XeenEventExecutionSuspended{state,request};
    };

	const auto finalize = [&]() -> XeenEventExecutionStepResult {
		if (publication) publication->execution(state);
		state.pendingPresentation.reset();
		if (!state.pendingRewards.hasWork() && state.rewardPhase == XeenRewardPhase::Running)
			return completed(workingCamera, workingGameFlags, instructionCount);
		XeenPresentationRequest request;
		request.response = XeenPresentationResponseRequirement::Acknowledgment;
		request.mapId = workingCamera.mapId;
		request.kind=XeenPresentationKind::RewardReceipt;
		bool foundLine=false;
		if(state.rewardPhase==XeenRewardPhase::Receipt) {
			if(publication && partyState.worldFlags)publication->creditRewardCurrency(state,partyState);
			else xeenCreditRewardCurrency(state.pendingRewards,state.rewardReceipt,partyState);
			state.rewardPhase = XeenRewardPhase::Completed;
			return completed(workingCamera, workingGameFlags, instructionCount);
		}
		if(state.rewardPhase==XeenRewardPhase::Running) {
			state.rewardReceipt=xeenPrepareRewardReceipt(state.pendingRewards);state.rewardCursor=0;
			if(xeenPacksGloballyFull(partyState)) {
				state.rewardPhase=XeenRewardPhase::Warning;request.kind=XeenPresentationKind::RewardWarning;
				if(!world.detachedEventCandidate())request.text="Reward capacity warning\nAll four category tails are full across active members.\n"
					"Remaining items will be lost. Gold and gems are still credited.\nSpace / Enter / Escape: continue";
			}
		} else if(state.rewardPhase==XeenRewardPhase::Blank) {
			const auto &entry=state.rewardReceipt.entries[state.rewardCursor];
			if(publication && partyState.worldFlags)publication->sortRewardEntry(entry,partyState);
			else xeenSortRewardEntry(entry,partyState);
			state.rewardPhase=XeenRewardPhase::Found;request.rewardWaitTicks=10;
			foundLine=true;
		} else if(state.rewardPhase==XeenRewardPhase::Found)++state.rewardCursor;
		if(request.kind!=XeenPresentationKind::RewardWarning && !foundLine) {
			while(state.rewardCursor<state.rewardReceipt.count) {
				if(publication && partyState.worldFlags)publication->deliverRewardEntry(state,partyState);
				else xeenDeliverRewardEntry(state.rewardReceipt,state.rewardCursor,partyState,state.preferredRewardRecipient,false);
				if(state.rewardReceipt.entries[state.rewardCursor].owner) {
					state.rewardPhase=XeenRewardPhase::Blank;request.rewardWaitTicks=1;break;
				}
				++state.rewardCursor;
			}
			if(state.rewardCursor==state.rewardReceipt.count)state.rewardPhase=XeenRewardPhase::Receipt;
		}
		if(!world.detachedEventCandidate() && state.rewardPhase==XeenRewardPhase::Receipt)
			request.text=xeenRewardReceiptText(state.rewardReceipt,partyState.roster);
		state.pendingPresentation = XeenEventPendingPresentation{request, XeenEventPendingContinuation::RewardFinalization, {}};
		return XeenEventExecutionSuspended{state, std::move(request)};
	};

	if (state.pendingPresentation) {
		const XeenEventPendingPresentation pending = *state.pendingPresentation;
		if (!response || !responseMatches(pending.request.response, *response)) {
			return error(XeenEventExecutionErrorKind::InvalidPresentationResponse,
				"response does not match the pending presentation request",
				instructionCount, logical, pending.request.source);
		}
		if (pending.continuation == XeenEventPendingContinuation::RewardFinalization) return finalize();
        if(pending.continuation==XeenEventPendingContinuation::ConfirmWord) {
            const auto &word=*pending.request.word;
            const auto &input=std::get<XeenWordInput>(response->value).text;
            if(input.size()>30 || !std::all_of(input.begin(),input.end(),[](unsigned char c){return c>=32 && c<=127;}))
                return error(XeenEventExecutionErrorKind::InvalidPresentationResponse,"Invalid word input",instructionCount,logical,pending.request.source);
            const auto equal=[](const std::string &a,const std::string &b) {
                if(a.size()!=b.size())return false;
                const auto upper=[](unsigned char c){return c>='a' && c<='z'?c-32:c;};
                for(std::size_t i=0;i<a.size();++i)if(upper(a[i])!=upper(b[i]))return false;
                return true;
            };
            unsigned result=0;
            state.mirrorDestination.reset();
            if(!input.empty()) {
                // Maintainer decision, 2026-10-16: opcode 21 alternate-only
                // matches take ordinary success. DOS passes an uninitialized
                // scratch marker here; residue 55 instead selects operand 3.
                // See docs/original-quirks.md (ConfirmWord marker).
                if(word.inputType)result=equal(input,pending.request.text) || equal(input,pending.request.alternate);
                else {
                    const unsigned first=workingCamera.mapId.side==XeenSide::Clouds && workingCamera.mapId.number>32?0x26:0;
                    const unsigned bound=std::min<unsigned>(pending.request.mirrors.size(),workingCamera.mapId.side==XeenSide::Clouds?63:pending.request.mirrors.size());
                    for(unsigned i=first;i<bound;++i)if(equal(input,pending.request.mirrors[i].name)) {
                        result=i+1;state.mirrorDestination=pending.request.mirrors[i];break;
                    }
                }
            }
            if(!word.inputType && workingCamera.mapId.side==XeenSide::Clouds &&
                (result==40 || result==60 || result==61 || result==62))
                return error(XeenEventExecutionErrorKind::UnsupportedOperand,"Ending mirror code is not supported yet",instructionCount,logical,pending.request.source);
            if(!word.inputType && workingCamera.mapId.side==XeenSide::Clouds && result==59 &&
                state.pendingRewards.count(XeenRewardCategory::Weapons)<XeenPendingRewards::kCapacity) {
                state.pendingRewards.enqueue(XeenItem{0,34,0,0},XeenRewardCategory::Weapons);
                return finalize();
            }
            state.pendingPresentation.reset();
            if(result) {
                logical.line=word.targetLine;missingPolicy=MissingInstructionPolicy::ExplicitJump;
                pendingTransferSource=pending.request.source;
            } else {
                if(logical.line==255)return error(XeenEventExecutionErrorKind::LineOverflow,"ConfirmWord fallthrough line overflow",instructionCount,logical,pending.request.source);
                ++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;
            }
        }
		if (pending.continuation == XeenEventPendingContinuation::WhoWill) {
			if (std::holds_alternative<CharacterSelectionCancelled>(response->value))
				return finalize();
			const auto index = std::get<SelectedCharacter>(response->value).partyIndex;
			const auto &members = pending.request.members;
			bool identityMatches = members.size() == partyState.party.size();
			for (std::size_t i = 0; identityMatches && i < members.size(); ++i)
				identityMatches = members[i].partyIndex == i &&
					members[i].rosterId == partyState.party.activeRosterIds()[i];
			if (!identityMatches || index >= members.size())
				return error(XeenEventExecutionErrorKind::InvalidPresentationResponse,
					"WhoWill party identity or selected index changed", instructionCount,
					logical, pending.request.source);
			const auto &character = partyState.party.member(partyState.roster, index);
			if (!character.canAct()) {
				auto &request = state.pendingPresentation->request;
				request.refusal = character.name + " is in no condition to act.";
				for (auto &member : request.members) {
					const auto &live = partyState.party.member(partyState.roster, member.partyIndex);
					member.name = live.name;
					member.eligible = live.canAct();
				}
				return XeenEventExecutionSuspended{state, request};
			}
			state.activeCharacterIndex = index;
			state.entireParty=false;
			if (partyState.party.size() > 1) state.preferredRewardRecipient = index;
		}
		state.pendingPresentation.reset();
		if (pending.continuation == XeenEventPendingContinuation::Terminate)
			return finalize();
		if (pending.continuation == XeenEventPendingContinuation::NpcChoice) {
			if (*response == XeenPresentationResponse::Yes) {
				logical.line = pending.request.npc->targetLine;
				missingPolicy = MissingInstructionPolicy::ExplicitJump;
				pendingTransferSource = pending.request.source;
			} else {
				if (logical.line == 255)
					return error(XeenEventExecutionErrorKind::LineOverflow,
						"NPC fallthrough line overflow", instructionCount, logical, pending.request.source);
				++logical.line;
				missingPolicy = MissingInstructionPolicy::NaturalCompletion;
			}
		}
		if (pending.continuation == XeenEventPendingContinuation::ConditionalAction44) {
			const std::uint32_t actual = pending.request.response ==
				XeenPresentationResponseRequirement::Acknowledgment ? 1 :
				(*response == XeenPresentationResponse::Yes ? 0 : 2);
			const auto &conditional = *pending.conditional;
			if (compare(actual, conditional.value, conditional.comparison)) {
				// Only an acknowledged Action 44 to its numeric next line may
				// complete when lookup finds no successor. Present records still
				// pass through normal decoding, dispatch and instruction limits.
				const bool adjacentAcknowledgment = conditional.action == 44 &&
					conditional.value == 1 && pending.request.response ==
						XeenPresentationResponseRequirement::Acknowledgment &&
					logical.line < 255 && conditional.targetLine == logical.line + 1;
				logical.line = conditional.targetLine;
				missingPolicy = adjacentAcknowledgment ? MissingInstructionPolicy::NaturalCompletion :
					MissingInstructionPolicy::ExplicitJump;
				pendingTransferSource = pending.request.source;
			} else {
				if (logical.line == 255)
					return error(XeenEventExecutionErrorKind::LineOverflow,
						"conditional fallthrough line overflow", instructionCount,
						logical, pending.request.source);
				++logical.line;
				missingPolicy = MissingInstructionPolicy::NaturalCompletion;
			}
		}
	} else if (response) {
		return error(XeenEventExecutionErrorKind::InvalidPresentationResponse,
			"execution has no pending presentation request", instructionCount, logical);
	}

	for (;;) {
		if (publication) publication->execution(state);
		if (logical.x < 0 || logical.x > 255 || logical.y < 0 || logical.y > 255 ||
				logical.line < 0 || logical.line > 255) {
			const auto kind = missingPolicy == MissingInstructionPolicy::ExplicitCall ?
				XeenEventExecutionErrorKind::InvalidCallTarget :
				XeenEventExecutionErrorKind::InvalidJumpTarget;
			return error(kind, "logical event address is outside the lookup domain",
				instructionCount, logical, pendingTransferSource, logical);
		}

		const auto recordIndex = script->findInstructionIndex(
			static_cast<std::uint8_t>(logical.x),
			static_cast<std::uint8_t>(logical.y), state.lookupDirection,
			static_cast<std::uint8_t>(logical.line));
		if (!recordIndex) {
			if (missingPolicy == MissingInstructionPolicy::NaturalCompletion)
				return finalize();
			const auto kind = missingPolicy == MissingInstructionPolicy::ExplicitCall ?
				XeenEventExecutionErrorKind::InvalidCallTarget :
				XeenEventExecutionErrorKind::InvalidJumpTarget;
			return error(kind,
				missingPolicy == MissingInstructionPolicy::ExplicitCall ?
					"explicit CallEvent target does not exist" :
					"explicit conditional jump target does not exist",
				instructionCount, logical, pendingTransferSource, logical);
		}
		pendingTransferSource.reset();

		const XeenEventRecord effective = world.effectiveEvent(
			{logical.mapId, *recordIndex}, script->records()[*recordIndex]);
		const XeenEventDecodeResult decodedResult = XeenEventDecoder::decode(effective,
			{logical.mapId, script->file().resourceName, *recordIndex,
				world.regionalJourney(),effective.opcode!=script->records()[*recordIndex].opcode});
		// Town dispatch requires the Journey publication boundary.
		if (effective.opcode==0x11 && !publication && !world.detachedEventCandidate()) {
            const auto source=std::visit([](const auto &value){return value.source;},decodedResult);
            return error(XeenEventExecutionErrorKind::UnsupportedOpcode,
                "opcode 17 is outside the supported decoder subset",instructionCount,logical,source);
        }
        if (const auto *decodeError = std::get_if<XeenEventDecodeError>(&decodedResult)) {
			return error(executionKind(decodeError->kind), decodeError->message,
				instructionCount, logical, decodeError->source);
		}
		const auto &decoded = std::get<XeenDecodedEventInstruction>(decodedResult);
		const bool resumedEffect=state.generatingReward || state.portraitProgress || state.giveTake || state.lock;
		if (!resumedEffect && instructionCount >= kMaximumInstructions) {
			return error(XeenEventExecutionErrorKind::InstructionLimitExceeded,
				"event execution exceeded 1024 dispatched instructions",
				instructionCount, logical, decoded.source);
		}
		// Journey Events run on a detached candidate. The live Journey world only
		// settles a terminal service continuation, which dispatches nothing.
		if(world.sessionState().journey() && !world.detachedEventCandidate())
			return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
				"Live Journey Event dispatch is not supported",instructionCount,logical,decoded.source);
		if(!resumedEffect) {
			++instructionCount;
			if(publication)publication->dispatched(state,decoded);
		}

		if (std::holds_alternative<XeenEventExit>(decoded.operation))
			return finalize();
		if (const auto *service=std::get_if<XeenEventTownService>(&decoded.operation)) {
			const bool smith=service->action==1,training=service->action==5,temple=service->action==4;
			const bool tavern=service->action==3,bank=service->action==0,guild=service->action==2;
			if ((!publication && !world.detachedEventCandidate()) || (!smith && !training && !temple && !tavern && !bank && !guild) ||
				logical.mapId!=XeenMapIdentity(28) || workingCamera.mapId!=logical.mapId || state.pendingRewards.hasWork())
				return error(XeenEventExecutionErrorKind::UnsupportedOperand,
					"Town service is outside the admitted service Event",instructionCount,logical,decoded.source);
			XeenPresentationRequest request;
			request.kind=guild?XeenPresentationKind::GuildService:bank?XeenPresentationKind::BankService:tavern?XeenPresentationKind::TavernService:temple?XeenPresentationKind::TempleService:
				training?XeenPresentationKind::TrainingService:XeenPresentationKind::ArmorRepairService;
			request.response=XeenPresentationResponseRequirement::Acknowledgment;
			request.mapId=logical.mapId;request.source=decoded.source;
			state.pendingPresentation=XeenEventPendingPresentation{request,XeenEventPendingContinuation::Terminate,{}};
			return XeenEventExecutionSuspended{state,request};
		}

        if(const auto *word=std::get_if<XeenEventConfirmWord>(&decoded.operation)) {
            if(!textProvider)return error(XeenEventExecutionErrorKind::MissingTextResource,"Word input text provider is absent",instructionCount,logical,decoded.source);
            const auto texts=textProvider(logical.mapId);
            if(texts.mapId!=logical.mapId)return error(XeenEventExecutionErrorKind::TextMapMismatch,"Word input text identity changed",instructionCount,logical,decoded.source);
            if(!texts.resourcePresent)return error(XeenEventExecutionErrorKind::MissingTextResource,"Word input text is missing",instructionCount,logical,decoded.source);
            XeenPresentationRequest request;request.kind=XeenPresentationKind::WordInput;
            request.response=XeenPresentationResponseRequirement::WordInput;request.mapId=logical.mapId;
            request.source=decoded.source;request.word=*word;
            // DOS clears all 280 offset slots before indexing the resource.
            // An absent byte-indexed string aliases offset zero (text[0]).
            const auto textAt=[&](unsigned index) {
                const auto *text=texts.stringAt(index);
                return text?*text:texts.strings.empty()?std::string{}:texts.strings.front();
            };
            request.text=textAt(word->inputType);
            request.alternate=word->alternateText?textAt(word->alternateText):request.text;
            if(word->titleText)request.title=textAt(word->titleText);
            request.mirrors=texts.mirrors;
            if(!word->inputType && request.mirrors.empty())return error(XeenEventExecutionErrorKind::MissingTextResource,"Mirror destination table is missing",instructionCount,logical,decoded.source);
            state.pendingPresentation=XeenEventPendingPresentation{request,XeenEventPendingContinuation::ConfirmWord,{}};
            return XeenEventExecutionSuspended{state,request};
        }

		const auto *who = std::get_if<XeenEventWhoWill>(&decoded.operation);
		if (who) {
			if (!partyState.party.size())
				return error(XeenEventExecutionErrorKind::EmptyParty,
					"WhoWill requires an active party member", instructionCount, logical, decoded.source);
			if (logical.line == 255)
				return error(XeenEventExecutionErrorKind::LineOverflow,
					"WhoWill sequential line overflow", instructionCount, logical, decoded.source);
			if (partyState.party.size() == 1) {
				state.activeCharacterIndex = 0;
				state.entireParty=false;
				++logical.line;
				missingPolicy = MissingInstructionPolicy::NaturalCompletion;
				continue;
			}
			if (who->verbIndex >= 32)
				return error(XeenEventExecutionErrorKind::UnsupportedOperand,
					"WhoWill verb index exceeds 31", instructionCount, logical, decoded.source);
		}
		const auto *npc = std::get_if<XeenEventNpc>(&decoded.operation);
		if (npc) {
			if (npc->confirmationMode > 2)
				return error(XeenEventExecutionErrorKind::UnsupportedOperand,
					"NPC confirmation mode " + std::to_string(npc->confirmationMode) + " is unsupported",
					instructionCount, logical, decoded.source);
			if (logical.mapId.side != XeenSide::Clouds || workingCamera.mapId.side != XeenSide::Clouds)
				return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
					"NPC requires Clouds logical and physical context", instructionCount, logical, decoded.source);
		}
		const auto *display = std::get_if<XeenEventDisplay>(&decoded.operation);
		if (display || who || npc) {
			const auto textIndex = npc ? npc->bodyTextIndex : who ? who->textIndex : display->textIndex;
			if (!textProvider) {
				return error(XeenEventExecutionErrorKind::MissingTextResource,
					"event text provider is absent", instructionCount, logical,
					decoded.source);
			}
			XeenEventTextFile textFile;
			try {
				if (publication) publication->check();
				textFile = textProvider(logical.mapId);
				if (publication) publication->check();
			} catch (const std::exception &exception) {
				return error(XeenEventExecutionErrorKind::MissingTextResource,
					std::string("failed to load event text resource: ") + exception.what(),
					instructionCount, logical, decoded.source);
			}
			if (textFile.mapId != logical.mapId) {
				return error(XeenEventExecutionErrorKind::TextMapMismatch,
					"event text identity differs from requested map", instructionCount,
					logical, decoded.source);
			}
			if (!textFile.resourcePresent) {
				return error(XeenEventExecutionErrorKind::MissingTextResource,
					"event text resource is missing", instructionCount, logical,
					decoded.source);
			}
			const std::string *text = textFile.stringAt(textIndex);
			if (!text) {
				return error(XeenEventExecutionErrorKind::InvalidTextIndex,
					"event text index is outside the map text table", instructionCount,
					logical, decoded.source);
			}

			XeenPresentationRequest request;
			request.kind = npc ? XeenPresentationKind::NpcAcknowledgment : who ? XeenPresentationKind::CharacterSelection : presentationKind(display->kind);
			request.response = npc ? (npc->confirmationMode==2 ? XeenPresentationResponseRequirement::Presented :
				npc->confirmationMode==0 ? XeenPresentationResponseRequirement::YesNo : XeenPresentationResponseRequirement::Acknowledgment) : who ? XeenPresentationResponseRequirement::CharacterSelection : display->kind ==
				XeenEventDisplayKind::BottomWindowTwoLines ?
				XeenPresentationResponseRequirement::Acknowledgment :
				XeenPresentationResponseRequirement::Presented;
			request.mapId = logical.mapId;
			request.textIndex = textIndex;
			request.text = *text;
			if (npc) {
				const auto *title = textFile.stringAt(npc->titleTextIndex);
				if (!title)
					return error(XeenEventExecutionErrorKind::InvalidTextIndex,
						"NPC title index is outside the map text table", instructionCount, logical, decoded.source);
				request.npc = *npc;
				request.cdSpeech = state.cdSpeech;
				request.title = *title;
			}
			if (display) request.layoutValue = display->layoutValue;
			if (who) {
				request.verbIndex = who->verbIndex;
				for (std::size_t i = 0; i < partyState.party.size(); ++i) {
					const auto &member = partyState.party.member(partyState.roster, i);
					request.members.push_back({i, partyState.party.activeRosterIds()[i],
						member.name, member.canAct()});
				}
			}
			request.source = decoded.source;

			XeenEventPendingPresentation pending;
			pending.request = request;
			if (who) pending.continuation = XeenEventPendingContinuation::WhoWill;
			if (display && display->kind == XeenEventDisplayKind::BottomWindowTwoLines) {
				pending.continuation = XeenEventPendingContinuation::Terminate;
			} else if (npc && npc->confirmationMode == 0) {
				// LocationMessage with buttons (0x256cd): Yes jumps, No continues.
				pending.continuation = XeenEventPendingContinuation::NpcChoice;
			} else {
				if (logical.line == 255) {
					return error(XeenEventExecutionErrorKind::LineOverflow,
						"display sequential event line overflow", instructionCount,
						logical, decoded.source);
				}
				++logical.line;
				missingPolicy = MissingInstructionPolicy::NaturalCompletion;
			}
			state.pendingPresentation = pending;
			return XeenEventExecutionSuspended{state, request};
		}

		if (std::holds_alternative<XeenEventRemove>(decoded.operation)) {
			try {
				// Calls change logical X/Y, never the physical mutation cell.
				// Current supported transfers keep the script on the physical map.
				world.applyRemove(workingCamera, state.selectedObject, script->file(), publication);
			} catch (const std::exception &exception) {
				return error(XeenEventExecutionErrorKind::InvalidRemoveContext,
					exception.what(), instructionCount, logical, decoded.source);
			}
			logical.line = 0;
			missingPolicy = MissingInstructionPolicy::NaturalCompletion;
			continue;
		}

		if (std::holds_alternative<XeenEventNone>(decoded.operation)) {
			if (logical.line == 255) {
				return error(XeenEventExecutionErrorKind::LineOverflow,
					"sequential event line overflow", instructionCount, logical,
					decoded.source);
			}
			++logical.line;
			missingPolicy = MissingInstructionPolicy::NaturalCompletion;
			continue;
		}
		if (std::holds_alternative<XeenEventVoiceCue>(decoded.operation) ||
			std::holds_alternative<XeenEventSoundCue>(decoded.operation) ||
			std::holds_alternative<XeenEventCdSpeech>(decoded.operation)) {
			if (logical.line==255)
				return error(XeenEventExecutionErrorKind::LineOverflow,
					"audio sequential event line overflow",instructionCount,logical,decoded.source);
			if (publication) publication->deferredAudio();
			if(const auto *cue=std::get_if<XeenEventCdSpeech>(&decoded.operation))state.cdSpeech=*cue;
			XeenPresentationRequest request;
			request.kind=XeenPresentationKind::DeferredAudio;
			request.mapId=logical.mapId;
			request.source=decoded.source;
			request.text=std::holds_alternative<XeenEventCdSpeech>(decoded.operation) ?
				"CD speech not supported yet" : std::holds_alternative<XeenEventSoundCue>(decoded.operation) ?
				"Sound effects not supported yet" : "Voice audio not supported yet";
			// Existing Presented response is automatic: no input, clock or RNG.
			++logical.line;
			missingPolicy=MissingInstructionPolicy::NaturalCompletion;
			state.pendingPresentation=XeenEventPendingPresentation{request,XeenEventPendingContinuation::Advance,{}};
			return XeenEventExecutionSuspended{state,request};
		}

		if (const auto *enchanted = std::get_if<XeenEventGiveEnchanted>(&decoded.operation)) {
			if (logical.mapId.side != XeenSide::Clouds || workingCamera.mapId.side != XeenSide::Clouds)
				return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
					"GiveEnchanted requires Clouds logical and physical context", instructionCount, logical, decoded.source);
			if (!partyState.party.size())
				return error(XeenEventExecutionErrorKind::EmptyParty,
					"GiveEnchanted requires an active party member", instructionCount, logical, decoded.source);
			if (logical.line == 255)
				return error(XeenEventExecutionErrorKind::LineOverflow,
					"GiveEnchanted sequential line overflow", instructionCount, logical, decoded.source);
			XeenItem item{};
			item.material = enchanted->itemCode - 60;
			item.id = enchanted->specialId;
			item.state = 1;
			if (publication && partyState.worldFlags) publication->prepareRewardEnqueue(state,item);
			state.pendingRewards.enqueue(item);
			++logical.line;
			missingPolicy = MissingInstructionPolicy::NaturalCompletion;
			continue;
		}

  if(const auto *heads=std::get_if<XeenEventAlterHeads>(&decoded.operation)) {
   world.alterHeads(workingCamera,{heads->left,heads->right});
   ++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
  }
  // Typed damage with its portrait steps, shared by Damage, RndDamage and a
  // lock's trap. nullopt once the damage hold has been presented.
  const auto serviceDamage=[&]() -> std::optional<XeenEventExecutionStepResult> {
   using P=XeenEventPortraitProgress;
   auto &progress=*state.portraitProgress;
   if(progress.phase==P::Phase::DamageDone) {
       state.portraitProgress.reset();state.portraitDamage.reset();state.portraitRandom.reset();
       return std::nullopt;
   }
   if(progress.phase==P::Phase::DamageApply) {state.portraitDamage->injuryAcknowledged=true;progress.phase=P::Phase::Recipients;}
   for(;;) {
       XeenConsequenceDraw draw{*state.portraitRandom,generationBudget,{}};
       const bool ready=state.portraitDamage->service(draw);generationBudget=draw.remaining;
       world.publishEventDamagePrefix(partyState,*state.portraitDamage,state.portraitRandom->continuation());
       if(ready) {
           progress.phase=P::Phase::DamageDone;
           return portraitRequest(XeenEventPortraitStage::DamageHold,0,state.portraitDamage->portraitFrame,decoded.source);
       }
       if(state.portraitDamage->injuryReady) {
           progress.phase=P::Phase::DamageApply;
           return portraitRequest(XeenEventPortraitStage::DamageOverlay,state.portraitDamage->impactOwner,state.portraitDamage->portraitFrame,decoded.source);
       }
       if(state.portraitDamage->injuryApplied)continue;
       XeenPresentationRequest request;request.kind=XeenPresentationKind::RewardGeneration;
       request.response=XeenPresentationResponseRequirement::Presented;request.mapId=logical.mapId;request.source=decoded.source;
       state.pendingPresentation=XeenEventPendingPresentation{request,XeenEventPendingContinuation::PortraitEffect,{}};
       return XeenEventExecutionStepResult(XeenEventExecutionSuspended{state,request});
   }
  };
  const auto *fixedDamage=std::get_if<XeenEventDamage>(&decoded.operation);
  const auto *randomDamage=std::get_if<XeenEventRandomDamage>(&decoded.operation);
  if(fixedDamage || randomDamage) {
   using P=XeenEventPortraitProgress;
   if(!state.portraitProgress) {
       const auto type=static_cast<XeenDamageType>(fixedDamage?fixedDamage->type:randomDamage->type);
       unsigned amount=fixedDamage?fixedDamage->amount:0;
       if(randomDamage) {
           // 0x285cc..0x2860b: one 1..maximum draw before the ordinary typed damage.
           if(randomDamage->maximum<1)return error(XeenEventExecutionErrorKind::UnsupportedOperand,
               "RndDamage maximum below one divides by zero",instructionCount,logical,decoded.source);
           if(!world.detachedEventCandidate() || !world.sessionState().journeyRandom())
               return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
                   "RndDamage draws require a detached Event",instructionCount,logical,decoded.source);
           XeenCombatRandom random(*world.sessionState().journeyRandom());
           std::optional<std::uint32_t> drawn;while(!drawn)drawn=random.draw(1,static_cast<std::uint32_t>(randomDamage->maximum));
           world.retainEventRandom(random.continuation());amount=*drawn;
       }
       state.portraitProgress=P{recipients(),0,P::Phase::Recipients};
       state.portraitDamage=world.prepareEventDamage(partyState,amount,type,state.activeCharacterIndex,state.entireParty);
       state.portraitDamage->deferInjury=true;state.portraitRandom.emplace(*world.sessionState().journeyRandom());
   }
   if(const auto step=serviceDamage())return *step;
   ++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
  }
		if (std::holds_alternative<XeenEventProtectionCheck>(decoded.operation)) {
			if (logical.mapId!=XeenMapIdentity(28))
				return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
					"protection check is outside the admitted profile",instructionCount,logical,decoded.source);
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
		}
		if (const auto *spawn=std::get_if<XeenEventSpawn>(&decoded.operation)) {
			world.applySpawn(logical.mapId,spawn->slot,spawn->x,spawn->y,spawn->unused);
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
		}
		if(const auto *move=std::get_if<XeenEventMoveWallObject>(&decoded.operation)) {
			world.applyMoveWallObject({logical.mapId,move->record},move->x,move->y);
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
		}
		if(std::holds_alternative<XeenEventMakeNothingHere>(decoded.operation)) {
			world.applyMakeNothingHere(workingCamera,script->file());
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
		}
		if(const auto *alter=std::get_if<XeenEventAlterMap>(&decoded.operation)) {
			world.applyAlterMap(logical.mapId,alter->x,alter->y,alter->direction,alter->wall);
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
		}
		if(const auto *alter=std::get_if<XeenEventAlterCellFlag>(&decoded.operation)) {
			world.applyAlterCellFlag(logical.mapId,alter->x,alter->y,alter->surface);
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
		}
		if (const auto *alter=std::get_if<XeenEventAlterEvent>(&decoded.operation)) {
			world.applyAlterEvent(workingCamera,alter->line,alter->replacement,script->file());
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
		}
		XeenEventValueOwners owners{partyState,workingGameFlags,workingCamera,state.scriptValues};
		const auto advance=[&](const char *what) -> std::optional<XeenEventExecutionError> {
			if(logical.line==255)return error(XeenEventExecutionErrorKind::LineOverflow,
				std::string(what)+" sequential line overflow",instructionCount,logical,decoded.source);
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;return std::nullopt;
		};
		const auto jump=[&](int line) {
			logical.line=line;missingPolicy=MissingInstructionPolicy::ExplicitJump;pendingTransferSource=decoded.source;
		};
		// A sequential write or draw on line 255 is refused before it changes anything.
		const auto lastLine=[&](const char *what) -> std::optional<XeenEventExecutionError> {
			if(logical.line!=255 || state.giveTake || state.lock)return std::nullopt;
			return error(XeenEventExecutionErrorKind::LineOverflow,
				std::string(what)+" sequential line overflow",instructionCount,logical,decoded.source);
		};
		const auto refuse=[&](XeenEventExecutionErrorKind kind,const std::string &reason) {
			return error(kind,reason,instructionCount,logical,decoded.source);
		};
		using K=XeenEventExecutionErrorKind;
		const auto refusalKind=[](XeenEventRefusal refusal,K mode) {
			return refusal==XeenEventRefusal::Index?K::InvalidFlagIndex:refusal==XeenEventRefusal::Context?
				K::UnsupportedExecutionContext:refusal==XeenEventRefusal::Member?K::EmptyParty:mode;
		};
		// Value operations read and write one side's banks; a script on another side is not supported.
		const bool sideMismatch=logical.mapId.side!=workingCamera.mapId.side;
		// The script's selection: 0 is every member (loops at 0x27205, 0x275f4, 0x284ba),
		// otherwise one member. Selection 8 (all but the combat target) has no Clouds producer.
		const auto members=[&]() {
			std::vector<std::size_t> result;
			if(state.entireParty) {for(std::size_t i=0;i<partyState.party.size();++i)result.push_back(i);}
			// Without a member, party-wide modes still run (the value routines refuse the others).
			if(result.empty())result.push_back(state.entireParty?0:state.activeCharacterIndex.value_or(0));
			return result;
		};
		// One getRandomNumber(lo,hi) on the Journey generator (0x45b9a), at its original place.
		const auto randomDraw=[&](std::uint32_t lo,std::uint32_t hi) -> std::uint32_t {
			if(!world.detachedEventCandidate() || !world.sessionState().journeyRandom())
				throw std::invalid_argument("Event draws require a detached Journey RNG");
			XeenCombatRandom random(*world.sessionState().journeyRandom());
			std::optional<std::uint32_t> value;
			while(!value)value=random.draw(lo,hi);
			world.retainEventRandom(random.continuation());
			return *value;
		};
		const auto canDraw=[&] {return world.detachedEventCandidate() && world.sessionState().journeyRandom();};

		if (const auto *setVar=std::get_if<XeenEventSetVar>(&decoded.operation)) {
			// setValue (0x23945) for the selection; mode 84 writes the facing.
			if(sideMismatch)return refuse(K::UnsupportedExecutionContext,"SetVar requires one side");
			if(const auto e=lastLine("SetVar"))return *e;
			for(const auto member:members()) {
				const auto written=xeenEventSetValue(owners,setVar->mode,setVar->value,member);
				if(written.status!=XeenEventWriteStatus::Applied)
					return refuse(refusalKind(written.refusal,K::UnsupportedOperationMode),written.reason);
			}
			if(const auto e=advance("SetVar"))return *e;
			continue;
		}

		if(const auto *move=std::get_if<XeenEventMoveObject>(&decoded.operation)) {
			if(!world.detachedEventCandidate())return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
				"MoveObj publication is not supported yet in this context",instructionCount,logical,decoded.source);
			world.applyMoveObject({logical.mapId,move->record},move->x,move->y);
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
		}
		if(const auto *set=std::get_if<XeenEventSetCharacter>(&decoded.operation)) {
			// 0x279ef: signed selector; 0 selects all, 1..6 only name the combat
			// target, 7 draws a member, 8..127 ask WhoWill.
			const int selector=static_cast<std::int8_t>(set->selector);
			if(const auto e=lastLine("SetChar"))return *e;
			if(selector<=6) {
				if(selector==0)state.entireParty=true;
				state.scriptValues.combatTarget=set->selector;
			} else if(selector==7) {
				if(!partyState.party.size())return refuse(XeenEventExecutionErrorKind::EmptyParty,"SetChar 7 requires an active party member");
				if(!canDraw())return refuse(XeenEventExecutionErrorKind::UnsupportedExecutionContext,"SetChar 7 draws require a detached Event");
				state.activeCharacterIndex=randomDraw(1,static_cast<std::uint32_t>(partyState.party.size()))-1;
				state.entireParty=false;state.scriptValues.combatTarget=1;
			} else return refuse(XeenEventExecutionErrorKind::UnsupportedOperand,"SetChar character selection is not supported yet");
			if(const auto e=advance("SetChar"))return *e;
			continue;
		}
		if(std::holds_alternative<XeenEventSelectRandomCharacter>(decoded.operation)) {
			// 0x287d8..0x287eb: one draw of 1..party size, no reroll.
			if(!partyState.party.size())return refuse(XeenEventExecutionErrorKind::EmptyParty,"SelectRandomChar requires an active party member");
			if(const auto e=lastLine("SelectRandomChar"))return *e;
			if(!canDraw())return refuse(XeenEventExecutionErrorKind::UnsupportedExecutionContext,"SelectRandomChar draws require a detached Event");
			state.activeCharacterIndex=randomDraw(1,static_cast<std::uint32_t>(partyState.party.size()))-1;
			state.entireParty=false;
			if(const auto e=advance("SelectRandomChar"))return *e;
			continue;
		}
		if(const auto *type=std::get_if<XeenEventItemType>(&decoded.operation)) {
			state.scriptValues.itemType=type->itemType; // 0x289d7
			if(const auto e=advance("ItemType"))return *e;
			continue;
		}
		if(const auto *jumpRandom=std::get_if<XeenEventJumpRandom>(&decoded.operation)) {
			// 0x28131..0x2815d: draw 1..range once; jump only when it equals the match.
			if(jumpRandom->range<1)return refuse(XeenEventExecutionErrorKind::UnsupportedOperand,"JumpRnd range below one divides by zero");
			if(!canDraw())return refuse(XeenEventExecutionErrorKind::UnsupportedExecutionContext,"JumpRnd draws require a detached Event");
			if(static_cast<int>(randomDraw(1,static_cast<std::uint32_t>(jumpRandom->range)))==jumpRandom->match)jump(jumpRandom->line);
			else if(const auto e=advance("JumpRnd"))return *e;
			continue;
		}
		if(const auto *mapFlag=std::get_if<XeenEventIfMapFlag>(&decoded.operation)) {
			// 0x28765..0x287d5: a monster counts as present while both position
			// bytes are below 32. Continue if present, else jump.
			const auto table=world.sessionState().monsters(workingCamera.mapId);
			const auto &file=world.objectFile(workingCamera.mapId);
			if(table.size()!=file.entities.monsters.size())
				return refuse(XeenEventExecutionErrorKind::UnsupportedExecutionContext,"IfMapFlag requires the staged monster table");
			const auto present=[](const XeenActor &actor) {
				return static_cast<std::uint8_t>(int(actor.x))<32 && static_cast<std::uint8_t>(int(actor.y))<32;
			};
			bool found=false;
			if(mapFlag->monster==0xff) {for(const auto &actor:table)found=found || present(actor);}
			else {
				if(mapFlag->monster>=table.size())return refuse(XeenEventExecutionErrorKind::UnsupportedOperand,
					"IfMapFlag monster is beyond the map's monster count");
				found=present(table[mapFlag->monster]);
			}
			if(found) {if(const auto e=advance("IfMapFlag"))return *e;}
			else jump(mapFlag->line);
			continue;
		}
		// giveTake (0x2200c..0x23315) for one member. Returns a step result to
		// suspend or fail; otherwise progress.flag is the original return value.
		using GiveTakePhase=XeenEventGiveTakeProgress::Phase;
		const auto sparkle=[&](unsigned owner) -> std::optional<XeenEventExecutionStepResult> {
			if(owner>=XeenRoster::kCharacterCount || (state.sparkledOwners&(1u<<owner)))return std::nullopt;
			state.sparkledOwners|=1u<<owner;
			return portraitRequest(XeenEventPortraitStage::Sparkle,owner,0,decoded.source);
		};
		const auto giveTakeMember=[&](std::uint8_t takeMode,std::uint32_t takeValue,std::uint8_t giveMode,std::uint32_t giveValue,
				std::size_t member,XeenEventGiveTakeProgress &progress) -> std::optional<XeenEventExecutionStepResult> {
			const auto fail=[&](const std::string &reason) {
				return std::optional<XeenEventExecutionStepResult>(refuse(XeenEventExecutionErrorKind::UnsupportedOperationMode,reason));
			};
			const unsigned owner=member<partyState.party.size()?unsigned(partyState.party.activeRosterIds()[member]):unsigned(XeenRoster::kCharacterCount);
			const auto done=[&](bool flag) {progress.flag=flag;progress.phase=GiveTakePhase::AfterMember;};
			if(progress.phase==GiveTakePhase::BeforeHp) {
				// 0x226c5: the sparkle preceded this HP grant.
				xeenEventGive(owners,8,giveValue,member);done(false);return std::nullopt;
			}
			if(sideMismatch)return std::optional<XeenEventExecutionStepResult>(refuse(K::UnsupportedExecutionContext,"TakeOrGive requires one side"));
			if(takeMode && !takeValue && takeMode!=104)return fail("TakeOrGive HowMuch is not supported yet");
			if(takeMode==giveMode && takeValue==giveValue) {
				// 0x22074..0x220af: equal pairs take a random 1..value instead.
				// The draw takes the low word (0x22091) and sign-extends its result (0x2209e).
				if(giveValue) {
					const auto bound=xeenEventWordDrawBound(giveValue);
					if(!bound)return fail("TakeOrGive random take of a zero word divides by zero");
					if(!canDraw())return fail("TakeOrGive random take is not supported in this context");
					takeValue=xeenEventSignedWord(randomDraw(1,*bound));
				}
				giveMode=0;giveValue=0;
			}
			const auto taken=xeenEventTake(owners,takeMode,takeValue,member);
			if(taken.status==XeenEventWriteStatus::Unsupported)
				return std::optional<XeenEventExecutionStepResult>(refuse(refusalKind(taken.refusal,K::UnsupportedOperationMode),taken.reason));
			if(taken.status==XeenEventWriteStatus::NotEnough) {
				// The refusal message, then the script ends (0x27887, 0x278ff).
				XeenPresentationRequest request;
				request.kind=XeenPresentationKind::CenteredMessage;
				request.response=XeenPresentationResponseRequirement::Acknowledgment;
				request.mapId=logical.mapId;request.source=decoded.source;request.notEnough=taken.consumable;
				state.giveTake.reset();
				state.pendingPresentation=XeenEventPendingPresentation{request,XeenEventPendingContinuation::Terminate,{}};
				return std::optional<XeenEventExecutionStepResult>(XeenEventExecutionSuspended{state,request});
			}
			if(taken.status==XeenEventWriteStatus::Failed) {done(true);return std::nullopt;}
			switch(giveMode) {
			case 8:
				if(owner<XeenRoster::kCharacterCount && !(state.sparkledOwners&(1u<<owner))) {
					progress.phase=GiveTakePhase::BeforeHp;
					return sparkle(owner);
				}
				xeenEventGive(owners,8,giveValue,member);done(false);return std::nullopt;
			case 21: {
				// 0x22883: weapons, armor and accessories by ID into treasure;
				// quest items count first, then show as weapons.
				XeenRewardCategory category=XeenRewardCategory::Weapons;XeenItem item{};
				if(giveValue>=82) {
					const auto index=XeenCloudsQuestItems::indexForItemId(giveValue);
					if(workingCamera.mapId.side!=XeenSide::Clouds)
						return std::optional<XeenEventExecutionStepResult>(refuse(K::UnsupportedExecutionContext,"Event quest item is outside the Clouds items"));
					if(!index)return fail("Event quest item is outside the Clouds items");
					if(member>=partyState.party.size())
						return std::optional<XeenEventExecutionStepResult>(refuse(K::EmptyParty,"Event quest item requires an active party member"));
					partyState.questItems.eventGrant(*index);
					if(state.pendingRewards.count(category)==XeenPendingRewards::kCapacity) {done(true);return std::nullopt;}
					state.pendingRewards.enqueueQuest(static_cast<std::uint8_t>(giveValue));
					done(false);return std::nullopt;
				}
				if(giveValue>=60)return fail("Event Misc item grant is not supported yet");
				if(giveValue<35)item.id=static_cast<std::uint8_t>(giveValue);
				else if(giveValue<49) {category=XeenRewardCategory::Armor;item.id=static_cast<std::uint8_t>(giveValue-35);}
				else {category=XeenRewardCategory::Accessories;item.id=static_cast<std::uint8_t>(giveValue-49);}
				if(!item.id || state.pendingRewards.count(category)==XeenPendingRewards::kCapacity) {done(true);return std::nullopt;}
				state.pendingRewards.enqueue(item,category);
				done(false);return std::nullopt;
			}
			case 66: {
				// 0x23006: a generated item; ItemType makes it a special-event item.
				if(giveValue<1 || giveValue>7 || !canDraw())return fail("Event item generation is not supported in this context");
				if(state.scriptValues.itemType && state.pendingRewards.count(XeenRewardCategory::Miscellaneous)==XeenPendingRewards::kCapacity) {
					done(true);return std::nullopt;
				}
				XeenItemCandidate candidate(giveValue,state.scriptValues.itemType?std::optional<int>(state.scriptValues.itemType):std::nullopt);
				XeenCombatRandom random(*world.sessionState().journeyRandom());
				for(;;) {XeenConsequenceDraw draw{random,64,{}};if(candidate.service(draw))break;}
				world.retainEventRandom(random.continuation());
				const auto category=static_cast<XeenRewardCategory>(candidate.category());
				if(state.pendingRewards.count(category)==XeenPendingRewards::kCapacity) {done(true);return std::nullopt;}
				state.pendingRewards.enqueue(candidate.item(),category);
				done(false);return std::nullopt;
			}
			case 76: {
				// 0x23218: addTime(days*1440) on the script clock; a 16-bit minutes argument.
				const std::uint64_t minutes=std::uint64_t(giveValue)*1440;
				if(minutes>0x7fff || !canDraw() || !partyState.encounterContext || partyState.party.size()!=6)
					return fail("Event time advance is not supported in this context");
				XeenConsequenceCharacters characters{};XeenConsequenceInputs inputs{};
				for(unsigned i=0;i<6;++i) {
					const auto id=partyState.party.activeRosterIds()[i];characters[i]=partyState.roster.at(id);
					if(!partyState.roster.combatInputs(id))return fail("Event time advance has no character inputs");
					inputs[i]=*partyState.roster.combatInputs(id);
				}
				XeenConditionTimeCandidate time(*partyState.encounterContext,minutes,characters,inputs,
					partyState.serviceEconomy?&*partyState.serviceEconomy:nullptr,XeenTimeMode::Script,XeenTimeCall::Add);
				XeenCombatRandom random(*world.sessionState().journeyRandom());
				for(;;) {XeenConsequenceDraw draw{random,64,{}};if(time.service(draw))break;}
				world.retainEventRandom(random.continuation());
				world.publishEventTime(partyState,time);
				done(false);return std::nullopt;
			}
			case 85:
				if(!world.detachedEventCandidate() || !partyState.encounterContext)return fail("Event year change requires a detached Event");
				world.applyEventYear(partyState,workingGameFlags,giveValue);
				done(false);return std::nullopt;
			case 100: {
				// 0x232bc: party gold, a random amount from the operand's low word, no sparkle.
				const auto bound=xeenEventWordDrawBound(giveValue);
				if(!bound)return fail("Event random gold of a zero word divides by zero");
				if(!canDraw() || !partyState.monsterTreasure)return fail("Event random gold is not supported in this context");
				partyState.monsterTreasure->gold=partyState.monsterTreasure->gold+xeenEventSignedWord(randomDraw(1,*bound));
				done(false);return std::nullopt;
			}
			case 25: case 82: return fail("Event give mode is not supported yet");
			default: break;
			}
			const auto given=xeenEventGive(owners,giveMode,giveValue,member);
			if(given.status!=XeenEventWriteStatus::Applied)
				return std::optional<XeenEventExecutionStepResult>(refuse(refusalKind(given.refusal,K::UnsupportedOperationMode),given.reason));
			done(false);
			if(given.sparkle)return sparkle(owner);
			return std::nullopt;
		};

		if(const auto *give=std::get_if<XeenEventGiveMulti>(&decoded.operation)) {
			if(!world.detachedEventCandidate())return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
				"GiveMulti publication is not supported yet in this context",instructionCount,logical,decoded.source);
			if(const auto e=lastLine("GiveMulti"))return *e;
			// The selected member, or the first for a whole-party selection (0x27e71).
			const std::size_t recipient=state.entireParty || !state.activeCharacterIndex ? 0 : *state.activeCharacterIndex;
			// giveExt (0x2358e): a locked chest is picked first (0x235ac..0x23726),
			// unless this run already picked one or the selected object is index 0.
			if(!state.generatingReward && !state.lock && !state.scriptValues.lockCounter && state.selectedObject &&
				state.selectedObject->recordIndex!=0) {
				const auto object=world.effectiveObject(*state.selectedObject);
				// The sprite table is 15 (Darkside only), 16, 58, 73 (0x235c2); the
				// following test against 20 reads the same byte, so it never skips.
				if(object.resourceId==16 || object.resourceId==58 || object.resourceId==73 ||
					(object.resourceId==15 && workingCamera.mapId.side==XeenSide::Darkside)) {
					if(!canDraw() || !partyState.encounterContext || recipient>=partyState.party.size())
						return refuse(XeenEventExecutionErrorKind::UnsupportedExecutionContext,"GiveMulti lock picking requires a detached Event");
					state.lock=XeenEventLockProgress{XeenEventLockProgress::Phase::Trap,recipient,*state.selectedObject,false};
					world.objectFrameDrawn(*state.selectedObject,1);
				}
			}
			if(state.lock) {
				using L=XeenEventLockProgress::Phase;
				auto &lock=*state.lock;
				const auto &geometry=world.map(workingCamera.mapId).geometry;
				if(lock.phase==L::Trap && !state.portraitProgress) {
					// 0x23618..0x23644: one chance in four of the map's trap damage, of a random type.
					if(randomDraw(1,4)==1) {
						const auto type=randomDraw(0,6);
						using P=XeenEventPortraitProgress;
						state.portraitProgress=P{1u<<lock.member,0,P::Phase::Recipients};
						state.portraitDamage=world.prepareEventDamage(partyState,geometry.trapDamage,static_cast<XeenDamageType>(type),lock.member,false);
						state.portraitDamage->deferInjury=true;state.portraitRandom.emplace(*world.sessionState().journeyRandom());
					} else lock.phase=L::Attempt;
				}
				if(lock.phase==L::Trap) {
					if(const auto step=serviceDamage())return *step;
					lock.phase=L::Attempt;
				}
				if(lock.phase==L::Attempt) {
					// 0x23647..0x2369e: thievery + 1..20 against the map's box difficulty;
					// success adds level * difficulty * 10 experience as a 16-bit product.
					const auto owner=partyState.party.activeRosterIds()[lock.member];
					auto &member=partyState.roster.at(owner);
					const int box=static_cast<std::uint8_t>(geometry.difficulties[3]);
					lock.picked=XeenCharacterRules::thievery(member)+int(randomDraw(1,20))>=box;
					if(lock.picked) {
						state.scriptValues.lockCounter=1;
						auto &inputs=XeenEventValueAccess::inputs(partyState,owner);
						if(!inputs)return refuse(XeenEventExecutionErrorKind::UnsupportedExecutionContext,"Lock picking has no character inputs");
						const int level=std::max(0,int(member.permanentLevel)+int(member.temporaryLevel));
						inputs->experience=inputs->experience+static_cast<std::uint16_t>(level*box*10);
					} else world.objectFrameDrawn(lock.object,0);
					lock.phase=L::Message;
					XeenPresentationRequest request;
					request.kind=XeenPresentationKind::CenteredMessage;
					request.response=XeenPresentationResponseRequirement::Acknowledgment;
					request.mapId=logical.mapId;request.source=decoded.source;
					request.lockPicked=lock.picked;request.title=member.name;
					// A failed pick returns 1 and ends the Event (0x2371b, 0x27eaf..0x27ebb).
					state.pendingPresentation=XeenEventPendingPresentation{request,lock.picked?
						XeenEventPendingContinuation::Advance:XeenEventPendingContinuation::Terminate,{}};
					return XeenEventExecutionSuspended{state,request};
				}
				state.lock.reset();
			}
			const XeenEventTakeOrGivePair pairs[]={give->first,give->second,give->third};
			// One shared random stream (0x23786, 0x237b3, 0x237db): a new GiveMulti starts
			// from the current Journey state, after any lock or trap draws; its generator
			// lives only across suspensions of this instruction's generation.
			if(!state.generatingReward) {state.rewardPair=0;state.generatingReward=true;state.rewardRandom.reset();}
			for(;state.rewardPair<3;++state.rewardPair) {
				const auto &pair=pairs[state.rewardPair];
				if(state.giveTake) {state.giveTake.reset();continue;} // A suspended default give completed.
				if(pair.mode==0 && pair.value==0)continue;
				if(pair.mode==34 || pair.mode==35) {state.pendingRewards.credit(pair.mode,pair.value);continue;}
				if(pair.mode!=66 && pair.mode!=100 && pair.mode!=101 && pair.mode!=106) {
					// Every other mode is giveTake(0,0,mode,value) (0x238b3); a failure ends the Event.
					if(pair.mode==67)return refuse(XeenEventExecutionErrorKind::UnsupportedOperationMode,"GiveMulti effect is not supported yet");
					XeenEventGiveTakeProgress progress;
					if(const auto step=giveTakeMember(0,0,pair.mode,pair.value,recipient,progress)) {
						if(std::holds_alternative<XeenEventExecutionSuspended>(*step) && progress.phase==GiveTakePhase::AfterMember) {
							state.giveTake=progress;auto suspended=std::get<XeenEventExecutionSuspended>(*step);
							suspended.state=state;return suspended;
						}
						if(std::holds_alternative<XeenEventExecutionSuspended>(*step))
							return refuse(XeenEventExecutionErrorKind::UnsupportedOperationMode,"GiveMulti HP grant is not supported yet");
						return *step;
					}
					state.rewardRandom.reset(); // A give's own draws continue from the retained Journey state.
					if(progress.flag) {state.generatingReward=false;return finalize();}
					continue;
				}
				// Currency and food draw from the operand's low word (xeenEventWordDrawBound).
				const auto bound=pair.mode==66?std::optional<std::uint32_t>(pair.value):xeenEventWordDrawBound(pair.value);
				if(!pair.value || !bound || (pair.mode==66 && pair.value>7))
					return error(XeenEventExecutionErrorKind::UnsupportedOperationMode,
						"GiveMulti effect is not supported yet",instructionCount,logical,decoded.source);
				if(!state.rewardRandom) {
					if(!world.sessionState().journeyRandom())throw std::invalid_argument("Event treasure has no Journey RNG");
					state.rewardRandom.emplace(*world.sessionState().journeyRandom());
				}
				XeenConsequenceDraw draw{*state.rewardRandom,generationBudget,{}};
				bool ready=false;std::optional<unsigned> amount;
				if(pair.mode==66) {
					if(state.scriptValues.itemType && state.pendingRewards.count(XeenRewardCategory::Miscellaneous)==XeenPendingRewards::kCapacity) {
						state.generatingReward=false;state.rewardRandom.reset();return finalize();
					}
					if(!state.rewardItem)state.rewardItem.emplace(pair.value,state.scriptValues.itemType?
						std::optional<int>(state.scriptValues.itemType):std::nullopt);
					ready=state.rewardItem->service(draw);
				} else {amount=draw.draw(1,*bound);ready=amount.has_value();}
				generationBudget=draw.remaining;
				world.retainEventRandom(state.rewardRandom->continuation());
				if(!ready) {
					XeenPresentationRequest request;request.kind=XeenPresentationKind::RewardGeneration;
					request.response=XeenPresentationResponseRequirement::Presented;request.mapId=logical.mapId;request.source=decoded.source;
					state.pendingPresentation=XeenEventPendingPresentation{request,XeenEventPendingContinuation::GiveMulti,{}};
					return XeenEventExecutionSuspended{state,request};
				}
				// 0x23770, 0x237c5: treasure gold and gems; 0x2379d: food straight to the party.
				if(amount && pair.mode==106)partyState.food=static_cast<std::uint16_t>(partyState.food+*amount);
				else if(amount)state.pendingRewards.credit(pair.mode==100?34:35,xeenEventSignedWord(*amount));
				else {
					const auto category=static_cast<XeenRewardCategory>(state.rewardItem->category());
					const bool full=state.pendingRewards.count(category)==XeenPendingRewards::kCapacity;
					if(!full)state.pendingRewards.enqueue(state.rewardItem->item(),category);
					state.rewardItem.reset();
					if(full) {state.generatingReward=false;state.rewardRandom.reset();return finalize();}
				}
			}
			state.generatingReward=false;state.rewardPair=0;state.rewardRandom.reset();
			++logical.line;missingPolicy=MissingInstructionPolicy::NaturalCompletion;continue;
		}
		if(const auto *test=std::get_if<XeenEventTakeOrGiveTest>(&decoded.operation)) {
			// TakeOrGive_2/_4 (28, 38) test the first pair (==, <=), TakeOrGive_3 (29)
			// both; the give is giveTake(0,0,mode2 or mode3,...) (0x275f4..0x277f3).
			const bool both=test->opcode==0x1d;
			if(const auto e=lastLine("TakeOrGive"))return *e;
			const auto comparison=test->opcode==0x26?XeenEventComparison::LessOrEqual:XeenEventComparison::Equal;
			const auto &given=both?test->third:test->second;
			const auto selection=members();
			auto progress=state.giveTake.value_or(XeenEventGiveTakeProgress{});
			// Resumed after a give, the test frame no longer holds the message copy's SI:DI.
			XeenEventResidue residue{state.giveTake?XeenEventResidue::Kind::Unknown:XeenEventResidue::Kind::RecordOffset,
				static_cast<std::uint32_t>(effective.fileOffset)};
			for(;progress.member<selection.size();++progress.member,progress.phase=GiveTakePhase::Member) {
				const auto member=selection[progress.member];
				if(progress.phase==GiveTakePhase::Member) {
					bool pass=true;
					for(const auto *pair:{&test->first,both?&test->second:nullptr}) {
						if(!pair || !pass)continue;
						std::string reason;
						XeenEventRefusal refusal=XeenEventRefusal::Mode;
						const auto probe=xeenEventProbe(owners,pair->mode,pair->value,member,reason,&refusal);
						if(!probe)return refuse(refusalKind(refusal,K::UnsupportedConditionAction),
							reason+" (action "+std::to_string(pair->mode)+", value "+std::to_string(pair->value)+")");
						const auto result=xeenEventCompare(*probe,residue,pair->value,
							both?XeenEventComparison::Equal:comparison);
						if(!result)return refuse(XeenEventExecutionErrorKind::UnsupportedConditionAction,
							"condition depends on an uninitialised original value");
						residue=xeenEventAfterProbe(*probe,residue);pass=*result;
					}
					if(!pass)continue;
					// The give's frame replaces the test residue for any later member.
					residue={XeenEventResidue::Kind::Unknown,0};
				}
				if(progress.phase!=GiveTakePhase::AfterMember) {
					if(const auto step=giveTakeMember(0,0,given.mode,given.value,member,progress)) {
						if(std::holds_alternative<XeenEventExecutionSuspended>(*step)) {
							state.giveTake=progress;auto suspended=std::get<XeenEventExecutionSuspended>(*step);
							suspended.state=state;return suspended;
						}
						return *step;
					}
				}
				if(given.mode==82)break;
			}
			state.giveTake.reset();
			if(const auto e=advance("TakeOrGive"))return *e;
			continue;
		}

		if (const auto *takeOrGive =
				std::get_if<XeenEventTakeOrGive>(&decoded.operation)) {
			if(const auto e=lastLine("TakeOrGive"))return *e;
			// The original loop (0x277f9..0x2790d). Selection 0 runs every member;
			// a failed take or give ends it, and some modes end the whole Event.
			const bool loop=state.entireParty;
			const auto selection=members();
			auto progress=state.giveTake.value_or(XeenEventGiveTakeProgress{GiveTakePhase::Member,0,takeOrGive->first.mode,false});
			for(;progress.member<selection.size();++progress.member,progress.phase=GiveTakePhase::Member) {
				const auto member=selection[progress.member];
				if(progress.phase!=GiveTakePhase::AfterMember) {
					if(const auto step=giveTakeMember(progress.takeMode,takeOrGive->first.value,takeOrGive->second.mode,
							takeOrGive->second.value,member,progress)) {
						if(std::holds_alternative<XeenEventExecutionSuspended>(*step) &&
							std::get<XeenEventExecutionSuspended>(*step).request.kind==XeenPresentationKind::PortraitEffect) {
							state.giveTake=progress;auto suspended=std::get<XeenEventExecutionSuspended>(*step);
							suspended.state=state;return suspended;
						}
						return *step;
					}
				}
				const bool flag=progress.flag;
				if(!loop) {
					// One member: a failure ends the Event (0x278fb).
					state.giveTake.reset();
					if(flag)return finalize();
					break;
				}
				const auto mode2=takeOrGive->second.mode;
				const auto endsOnSuccess=[](unsigned mode) {
					return mode==21 || mode==34 || mode==35 || mode==65 || mode==66 || mode==100 || mode==101 || mode==106;
				};
				bool stop=false;
				switch(progress.takeMode) {
				case 8: case 21: case 66:
					if(progress.takeMode==8)progress.takeMode=0;
					if(!flag) {stop=true;break;}
					if(mode2==82)progress.takeMode=0;
					if(endsOnSuccess(mode2) || mode2==82)stop=!flag;
					break;
				case 34: case 35: case 65: case 100: case 101: case 106:
					if(flag) {state.giveTake.reset();return finalize();}
					stop=true;break;
				default:
					if(mode2==82)progress.takeMode=0;
					if(endsOnSuccess(mode2) || mode2==82)stop=!flag;
					break;
				}
				if(stop)break;
			}
			state.giveTake.reset();
			if(const auto e=advance("TakeOrGive"))return *e;
			continue;
		}

		if (const auto *conditional =
				std::get_if<XeenEventConditional>(&decoded.operation)) {
			if (conditional->action == 44) {
				if (conditional->value > 1) {
					return error(XeenEventExecutionErrorKind::UnsupportedConditionAction,
						"condition action 44 supports only values 0 and 1",
						instructionCount, logical, decoded.source);
				}
				XeenPresentationRequest request;
				request.kind = XeenPresentationKind::Confirmation;
				request.response = conditional->value == 0 ?
					XeenPresentationResponseRequirement::YesNo :
					XeenPresentationResponseRequirement::Acknowledgment;
				request.mapId = logical.mapId;
				request.source = decoded.source;
				XeenEventPendingPresentation pending;
				pending.request = request;
				pending.continuation =
					XeenEventPendingContinuation::ConditionalAction44;
				pending.conditional = *conditional;
				state.pendingPresentation = pending;
				return XeenEventExecutionSuspended{state, request};
			}
			// 0x2712d..0x272ab: one member, or every member OR-ed (action 44 always uses one).
			if(sideMismatch)return refuse(K::UnsupportedExecutionContext,"Condition requires one side");
			const auto selection=members();
			XeenEventResidue residue{XeenEventResidue::Kind::RecordOffset,static_cast<std::uint32_t>(effective.fileOffset)};
			bool any=false,unknown=false;
			for(const auto member:selection) {
				std::string reason;
				XeenEventRefusal refusal=XeenEventRefusal::Mode;
				const auto probe=xeenEventProbe(owners,conditional->action,conditional->value,member,reason,&refusal);
				if(!probe)return refuse(refusalKind(refusal,K::UnsupportedConditionAction),
					(reason.empty()?std::string("condition action is outside the interpreter subset"):reason)+
					" (action "+std::to_string(conditional->action)+", value "+std::to_string(conditional->value)+")");
				const auto result=xeenEventCompare(*probe,residue,conditional->value,conditional->comparison);
				residue=xeenEventAfterProbe(*probe,residue);
				if(!result)unknown=true;else any=any || *result;
			}
			if(!any && unknown)return refuse(XeenEventExecutionErrorKind::UnsupportedConditionAction,
				"condition depends on an uninitialised original value");
			if(any)jump(conditional->targetLine);
			else if(const auto e=advance("conditional fallthrough"))return *e;
			continue;
		}

		if (const auto *call = std::get_if<XeenEventCallEvent>(&decoded.operation)) {
			XeenEventExecutionAddress target{logical.mapId, call->x, call->y,
				call->line};
			if (call->x < 0 || call->y < 0) {
				return error(XeenEventExecutionErrorKind::InvalidCallTarget,
					"negative CallEvent target has no raw lookup mapping",
					instructionCount, logical, decoded.source, target);
			}
			if (callStack.size() >= kMaximumCallDepth) {
				return error(XeenEventExecutionErrorKind::CallStackOverflow,
					"event call stack exceeded 64 pending calls", instructionCount,
					logical, decoded.source, target);
			}
			if (logical.line == 255) {
				return error(XeenEventExecutionErrorKind::LineOverflow,
					"CallEvent return line overflow", instructionCount, logical,
					decoded.source, target);
			}
			callStack.push_back({{logical.mapId, logical.x, logical.y,
				logical.line + 1}});
			logical = target;
			missingPolicy = MissingInstructionPolicy::ExplicitCall;
			pendingTransferSource = decoded.source;
			continue;
		}

		if (std::holds_alternative<XeenEventReturn>(decoded.operation)) {
			if (callStack.empty()) {
				return error(XeenEventExecutionErrorKind::InvalidReturn,
					"Return requires a pending CallEvent", instructionCount,
					logical, decoded.source);
			}
			logical = callStack.back().returnAddress;
			callStack.pop_back();
			missingPolicy = MissingInstructionPolicy::NaturalCompletion;
			pendingTransferSource.reset();
			continue;
		}

        const auto executeTeleport = [&](std::uint8_t mapId,int x,int y,bool continueExecution) -> std::optional<XeenEventExecutionError> {
            XeenCamera destination{{workingCamera.mapId.side,mapId},x,y,workingCamera.direction};
            if(!mapId) {
                if(!state.mirrorDestination)return error(XeenEventExecutionErrorKind::UnsupportedOperand,"Mirror teleport requires a ConfirmWord selection",instructionCount,logical,decoded.source);
                const auto &mirror=*state.mirrorDestination;
                destination.mapId.number=mirror.map;destination.x=mirror.x;destination.y=mirror.y;
                if(mirror.direction!=-1)destination.direction=static_cast<XeenDirection>(mirror.direction);
                if(!destination.x && !destination.y)destination.x=-1;
            }
            try {
                const auto transition=world.prepareEventTransition(destination,destination.x==-1);
                if(!transition)return error(XeenEventExecutionErrorKind::UnsupportedTeleportDestination,transition.reason,instructionCount,logical,decoded.source,
                    XeenEventExecutionAddress{destination.mapId,destination.x,destination.y,0});
                destination=transition.destination;
                if(workingCamera.mapId!=destination.mapId && world.detachedEventCandidate())
                    world.loadEventDestination(workingCamera.mapId,destination.mapId);
                workingCamera=destination;
                world.scenePresentation().markLightingStep();
                state.selectedObject.reset();
                if(continueExecution)state.selectedObject=world.selectObject(workingCamera);
            } catch(const std::exception &e) {
                return error(XeenEventExecutionErrorKind::MapLoadFailed,std::string("Failed teleport destination: ")+e.what(),instructionCount,logical,decoded.source);
            }
            return {};
        };

		if (const auto *teleport =
				std::get_if<XeenEventTeleportAndExit>(&decoded.operation)) {
			if (const auto teleportError = executeTeleport(teleport->mapId,
					teleport->x, teleport->y, false))
				return *teleportError;
			return finalize();
		}

		if (const auto *teleport =
				std::get_if<XeenEventTeleportAndContinue>(&decoded.operation)) {
			if (const auto teleportError = executeTeleport(teleport->mapId,
					teleport->x, teleport->y, true))
				return *teleportError;
   callStack.clear();state.scriptValues.damageType=0;
   state.entireParty=false;state.lookupDirection=workingCamera.direction;
   state.activeCharacterIndex = partyState.party.size() ? std::optional<std::size_t>{0} : std::nullopt;
			state.continuedAtDestination=true; // DAT 0x2711f..0x27127: redraw, then restart.
            // Continue prepares each intermediate scene before restarting its
            // Event chain, even when no text suspends there for a visible frame.
            if(partyState.encounterContext)
                partyState.encounterContext->lightAndResistances[0]=world.lightAfterScenePreparation(partyState,workingCamera);
            world.scenePresentation().completeLightingDraw();
			logical = {workingCamera.mapId, workingCamera.x, workingCamera.y, 0};
			try {
				script.emplace(scriptProvider(logical.mapId));
			} catch (const std::exception &exception) {
				return error(XeenEventExecutionErrorKind::ScriptLoadFailed,
					std::string("failed to load destination event script: ") +
						exception.what(), instructionCount, logical, decoded.source,
					logical);
			}
			if (script->file().mapId != logical.mapId) {
				return error(XeenEventExecutionErrorKind::ScriptMapMismatch,
					"destination event script map ID differs from requested map",
					instructionCount, logical, decoded.source, logical);
			}
			missingPolicy = MissingInstructionPolicy::NaturalCompletion;
			pendingTransferSource.reset();
			continue;
		}

		return error(XeenEventExecutionErrorKind::UnsupportedOpcode,
			"decoded operation is outside the interpreter subset",
			instructionCount, logical, decoded.source);
	}
}

void XeenWorld::applyEventYear(XeenPartyState &party,XeenGameFlags &flags,unsigned years) {
 if(!detachedEventCandidate() || !party.encounterContext)throw std::logic_error("Year advance requires a detached Event");
 auto &context=*party.encounterContext;
 if(years>65535u-context.year)throw std::overflow_error("Event year overflow");
 context.year+=years;
 for(int flag:{55,155,222,231})flags.clear(flag);
 for(auto owner:party.party.activeRosterIds()) {
  if(!party.roster._combatInputs[owner])throw std::invalid_argument("Year reset has no character inputs");
  xeenResetCharacterTemps(party.roster._characters[owner],*party.roster._combatInputs[owner]);
 }
 xeenResetPartyTemps(context);context.rested=true;
}
void XeenWorld::publishEventTime(XeenPartyState &party,const XeenConditionTimeCandidate &time) {
 if(!detachedEventCandidate() || !party.encounterContext || party.party.size()!=6)
  throw std::logic_error("Event time requires a detached Event");
 for(unsigned i=0;i<6;++i) {
  const auto owner=party.party.activeRosterIds()[i];
  party.roster._characters[owner]=time.characters[i];
  party.roster._combatInputs[owner]=time.inputs[i];
 }
 *party.encounterContext=time.context;
 if(time.economy && party.serviceEconomy)*party.serviceEconomy=*time.economy;
}
XeenTypedDamageCandidate XeenWorld::prepareEventDamage(const XeenPartyState &party,unsigned damage,XeenDamageType type,
 std::optional<std::size_t> member,bool entireParty) const {
 if(!detachedEventCandidate() || !party.encounterContext || !_sessionState._journeyRandom ||
  !party.party.size() || (!entireParty && (!member || *member>=party.party.size())))
  throw std::invalid_argument("Event damage has no selected owner or RNG");
 XeenConsequenceCharacters characters{};XeenConsequenceInputs inputs{};
 for(unsigned i=0;i<party.party.size();++i) {
  const auto owner=party.party.activeRosterIds()[i];characters[i]=party.roster.at(owner);
  if(!party.roster.combatInputs(owner))throw std::invalid_argument("Event damage inputs are absent");
  inputs[i]=*party.roster.combatInputs(owner);
 }
 const auto &context=*party.encounterContext;XeenDamageProtection protection;
 for(unsigned i=0;i<4;++i)protection.resistances[i]=context.lightAndResistances[i+2];
 protection.powerShield=context.effects[XeenPartyEffect::PowerShield];protection.blessed=context.effects[XeenPartyEffect::Bless];
 return XeenTypedDamageCandidate(characters,inputs,damage,type,context.year,
  entireParty?(1u<<party.party.size())-1:1u<<*member,protection);
 }
void XeenWorld::publishEventDamagePrefix(XeenPartyState &party,const XeenTypedDamageCandidate &candidate,const XeenJourneyRandomState &random,bool feedback) {
 if(!detachedEventCandidate())throw std::logic_error("Event damage prefix requires retained private owners");
 for(unsigned i=0;i<party.party.size();++i) {
  auto &live=party.roster.at(party.party.activeRosterIds()[i]);const auto &after=candidate.characters[i];
  live.currentHp=after.currentHp;live.conditions=after.conditions;live.armor=after.armor;
 }
 retainEventRandom(random);
 if(feedback)for(unsigned i=0;i<candidate.result.injuryCount;++i) {
  const auto owner=candidate.result.injuries[i].owner;_scenePresentation.portraitDamage(owner,candidate.portraitFrame);
 }
}
void XeenWorld::applyEventDamage(XeenPartyState &party,unsigned damage,XeenDamageType type,
 std::optional<std::size_t> member,bool entireParty,bool feedback) {
 auto candidate=prepareEventDamage(party,damage,type,member,entireParty);XeenCombatRandom random(*_sessionState._journeyRandom);
 for(;;) {XeenConsequenceDraw draw{random,64,{}};if(candidate.service(draw))break;}
 publishEventDamagePrefix(party,candidate,random.continuation(),feedback);
}

} // namespace mmodern
