#include "games/xeen/XeenEventInterpreter.h"

#include "games/xeen/XeenWorld.h"

#include <limits>
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
	case XeenEventDisplayKind::BottomWindow:
		return XeenPresentationKind::BottomWindowMessage;
	case XeenEventDisplayKind::BottomWindowTwoLines:
		return XeenPresentationKind::BottomWindowTwoLines;
	case XeenEventDisplayKind::MainWindow:
		return XeenPresentationKind::MainWindowMessage;
	}
	return XeenPresentationKind::CenteredMessage;
}

} // namespace

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
		std::uint8_t initialLine) const {
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
	if (!initialCamera.mapId || initialCamera.x < 0 || initialCamera.x > 15 ||
			initialCamera.y < 0 || initialCamera.y > 15 ||
			!validDirection(initialCamera.direction)) {
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
	return run(std::move(state), std::nullopt, partyState, world,
		scriptProvider, textProvider);
}

XeenEventExecutionStepResult XeenEventInterpreter::resume(
		XeenEventExecutionState state, XeenPresentationResponse response,
		XeenPartyState &partyState, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider) const {
	return run(std::move(state), response, partyState, world,
		scriptProvider, textProvider);
}

XeenEventExecutionStepResult XeenEventInterpreter::run(
		XeenEventExecutionState state,
		std::optional<XeenPresentationResponse> response,
		XeenPartyState &partyState, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider) const {
	auto &logical = state.logicalAddress;
	auto &workingCamera = state.workingCamera;
	auto &workingGameFlags = state.workingGameFlags;
	auto &script = state.currentScript;
	auto &callStack = state.callStack;
	auto &instructionCount = state.instructionCount;
	auto &missingPolicy = state.missingInstructionPolicy;
	auto &pendingTransferSource = state.pendingTransferSource;

	if (state.pendingPresentation) {
		const XeenEventPendingPresentation pending = *state.pendingPresentation;
		if (!response || !responseMatches(pending.request.response, *response)) {
			return error(XeenEventExecutionErrorKind::InvalidPresentationResponse,
				"response does not match the pending presentation request",
				instructionCount, logical, pending.request.source);
		}
		if (pending.continuation == XeenEventPendingContinuation::WhoWill) {
			if (std::holds_alternative<CharacterSelectionCancelled>(response->value))
				return completed(workingCamera, workingGameFlags, instructionCount);
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
		}
		state.pendingPresentation.reset();
		if (pending.continuation == XeenEventPendingContinuation::Terminate)
			return completed(workingCamera, workingGameFlags, instructionCount);
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
				return completed(workingCamera, workingGameFlags, instructionCount);
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
			{logical.mapId, script->file().resourceName, *recordIndex});
		if (const auto *decodeError = std::get_if<XeenEventDecodeError>(&decodedResult)) {
			return error(executionKind(decodeError->kind), decodeError->message,
				instructionCount, logical, decodeError->source);
		}
		const auto &decoded = std::get<XeenDecodedEventInstruction>(decodedResult);
		if (instructionCount >= kMaximumInstructions) {
			return error(XeenEventExecutionErrorKind::InstructionLimitExceeded,
				"event execution exceeded 1024 dispatched instructions",
				instructionCount, logical, decoded.source);
		}
		++instructionCount;

		if (std::holds_alternative<XeenEventExit>(decoded.operation))
			return completed(workingCamera, workingGameFlags, instructionCount);

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
				++logical.line;
				missingPolicy = MissingInstructionPolicy::NaturalCompletion;
				continue;
			}
			if (who->verbIndex >= 32)
				return error(XeenEventExecutionErrorKind::UnsupportedOperand,
					"WhoWill verb index exceeds 31", instructionCount, logical, decoded.source);
		}
		const auto *display = std::get_if<XeenEventDisplay>(&decoded.operation);
		if (display || who) {
			const auto textIndex = who ? who->textIndex : display->textIndex;
			if (!textProvider) {
				return error(XeenEventExecutionErrorKind::MissingTextResource,
					"event text provider is absent", instructionCount, logical,
					decoded.source);
			}
			XeenEventTextFile textFile;
			try {
				textFile = textProvider(logical.mapId);
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
			request.kind = who ? XeenPresentationKind::CharacterSelection : presentationKind(display->kind);
			request.response = who ? XeenPresentationResponseRequirement::CharacterSelection : display->kind ==
				XeenEventDisplayKind::BottomWindowTwoLines ?
				XeenPresentationResponseRequirement::Acknowledgment :
				XeenPresentationResponseRequirement::Presented;
			request.mapId = logical.mapId;
			request.textIndex = textIndex;
			request.text = *text;
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
				world.applyRemove(workingCamera, state.selectedObject, script->file());
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

		if (const auto *takeOrGive =
				std::get_if<XeenEventTakeOrGive>(&decoded.operation)) {
			const auto neutral = [](const XeenEventTakeOrGivePair &pair) {
				return pair.mode == 0 && pair.value == 0;
			};
			const bool setFlag = neutral(takeOrGive->first) &&
				takeOrGive->second.mode == 20 && neutral(takeOrGive->third);
			const bool clearFlag = takeOrGive->first.mode == 20 &&
				neutral(takeOrGive->second) && neutral(takeOrGive->third);
			const bool grantQuestItem = neutral(takeOrGive->first) &&
				takeOrGive->second.mode == 21 && neutral(takeOrGive->third);
			if (grantQuestItem) {
				const auto itemId = takeOrGive->second.value;
				const std::string detail = "TakeOrGive quest item " + std::to_string(itemId);
				const auto index = XeenCloudsQuestItems::indexForItemId(itemId);
				if (!index)
					return error(XeenEventExecutionErrorKind::UnsupportedOperationMode,
						detail + " is outside Clouds IDs 82..116", instructionCount, logical, decoded.source);
				if (logical.mapId.side != XeenSide::Clouds || workingCamera.mapId.side != XeenSide::Clouds)
					return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
						detail + " requires Clouds logical and physical context", instructionCount, logical, decoded.source);
				if (!partyState.party.size())
					return error(XeenEventExecutionErrorKind::EmptyParty,
						detail + " requires an active party member", instructionCount, logical, decoded.source);
				if (logical.line == 255)
					return error(XeenEventExecutionErrorKind::LineOverflow,
						"quest-item grant sequential line overflow", instructionCount, logical, decoded.source);
				// Party effects are immediate, independent of camera/flag commit.
				if (!partyState.questItems.increment(*index))
					return error(XeenEventExecutionErrorKind::QuestItemOverflow,
						detail + " counter would overflow", instructionCount, logical, decoded.source);
				++logical.line;
				missingPolicy = MissingInstructionPolicy::NaturalCompletion;
				continue;
			}
			if (!setFlag && !clearFlag) {
				return error(XeenEventExecutionErrorKind::UnsupportedOperationMode,
					"TakeOrGive mode combination is outside the interpreter subset",
					instructionCount, logical, decoded.source);
			}
			const std::uint32_t flag = setFlag ? takeOrGive->second.value :
				takeOrGive->first.value;
			if (flag >= XeenGameFlags::kCount) {
				return error(XeenEventExecutionErrorKind::InvalidFlagIndex,
					"TakeOrGive game flag index exceeds 255", instructionCount,
					logical, decoded.source);
			}
			if (setFlag)
				workingGameFlags.set(static_cast<int>(flag));
			else
				workingGameFlags.clear(static_cast<int>(flag));
			if (logical.line == 255) {
				return error(XeenEventExecutionErrorKind::LineOverflow,
					"TakeOrGive sequential event line overflow", instructionCount,
					logical, decoded.source);
			}
			++logical.line;
			missingPolicy = MissingInstructionPolicy::NaturalCompletion;
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
			std::uint32_t actual = 0;
			if (conditional->action == 9) {
				if (!partyState.party.size()) {
					return error(XeenEventExecutionErrorKind::EmptyParty,
						"condition action 9 requires an active party member",
						instructionCount, logical, decoded.source);
				}
				if (!state.activeCharacterIndex || *state.activeCharacterIndex >= partyState.party.size())
					return error(XeenEventExecutionErrorKind::InvalidPresentationResponse,
						"active character context is outside the party", instructionCount, logical, decoded.source);
				actual = static_cast<std::uint32_t>(
					partyState.party.member(partyState.roster, *state.activeCharacterIndex).currentSp);
			} else if (conditional->action == 20) {
				if (conditional->value > 255) {
					return error(XeenEventExecutionErrorKind::InvalidFlagIndex,
						"condition action 20 flag index exceeds 255",
						instructionCount, logical, decoded.source);
				}
				actual = workingGameFlags.isSet(static_cast<int>(conditional->value)) ?
					conditional->value : std::numeric_limits<std::uint32_t>::max();
			} else if (conditional->action == 21) {
				const std::string detail = "condition action 21 item " + std::to_string(conditional->value);
				if (logical.mapId.side != XeenSide::Clouds || workingCamera.mapId.side != XeenSide::Clouds)
					return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
						detail + " requires Clouds logical and physical context", instructionCount, logical, decoded.source);
				const auto index = XeenCloudsQuestItems::indexForItemId(conditional->value);
				if (!index)
					return error(XeenEventExecutionErrorKind::UnsupportedConditionAction,
						detail + " is outside Clouds quest-item IDs 82..116", instructionCount, logical, decoded.source);
				if (!partyState.party.size())
					return error(XeenEventExecutionErrorKind::EmptyParty,
						detail + " requires an active party member", instructionCount, logical, decoded.source);
				actual = partyState.questItems.at(*index) != 0 ? conditional->value :
					std::numeric_limits<std::uint32_t>::max();
			} else {
				return error(XeenEventExecutionErrorKind::UnsupportedConditionAction,
					"condition action is outside the interpreter subset",
					instructionCount, logical, decoded.source);
			}

			if (compare(actual, conditional->value, conditional->comparison)) {
				logical.line = conditional->targetLine;
				missingPolicy = MissingInstructionPolicy::ExplicitJump;
				pendingTransferSource = decoded.source;
			} else {
				if (logical.line == 255) {
					return error(XeenEventExecutionErrorKind::LineOverflow,
						"conditional fallthrough line overflow", instructionCount,
						logical, decoded.source);
				}
				++logical.line;
				missingPolicy = MissingInstructionPolicy::NaturalCompletion;
			}
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

		const auto executeTeleport = [&](std::uint8_t mapId, int x, int y,
				bool continueExecution) -> std::optional<XeenEventExecutionError> {
			XeenEventExecutionAddress target{{workingCamera.mapId.side, mapId}, x, y, 0};
			if (continueExecution && !callStack.empty()) {
				return error(XeenEventExecutionErrorKind::UnsupportedExecutionContext,
					"TeleportAndContinue with an active call stack is unsupported",
					instructionCount, logical, decoded.source, target);
			}
			if (x < 0 || x > 15 || y < 0 || y > 15) {
				return error(XeenEventExecutionErrorKind::UnsupportedTeleportDestination,
					"teleport destination is outside local coordinates 0..15",
					instructionCount, logical, decoded.source, target);
			}
			try {
				static_cast<void>(world.map(target.mapId));
			} catch (const std::exception &exception) {
				return error(XeenEventExecutionErrorKind::MapLoadFailed,
					std::string("failed to load teleport destination: ") + exception.what(),
					instructionCount, logical, decoded.source, target);
			}
			state.selectedObject.reset();
			workingCamera.mapId = target.mapId;
			workingCamera.x = x;
			workingCamera.y = y;
			if (continueExecution) {
				try { state.selectedObject = world.selectObject(workingCamera); }
				catch (const std::exception &exception) {
					return error(XeenEventExecutionErrorKind::ObjectLoadFailed,
						exception.what(), instructionCount, logical, decoded.source, target);
				}
			}
			return std::nullopt;
		};

		if (const auto *teleport =
				std::get_if<XeenEventTeleportAndExit>(&decoded.operation)) {
			if (const auto teleportError = executeTeleport(teleport->mapId,
					teleport->x, teleport->y, false))
				return *teleportError;
			return completed(workingCamera, workingGameFlags, instructionCount);
		}

		if (const auto *teleport =
				std::get_if<XeenEventTeleportAndContinue>(&decoded.operation)) {
			if (const auto teleportError = executeTeleport(teleport->mapId,
					teleport->x, teleport->y, true))
				return *teleportError;
			state.activeCharacterIndex = partyState.party.size() ? std::optional<std::size_t>{0} : std::nullopt;
			logical = {workingCamera.mapId, teleport->x, teleport->y, 0};
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

} // namespace mmodern
