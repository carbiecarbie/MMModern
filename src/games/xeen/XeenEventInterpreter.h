#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_INTERPRETER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_INTERPRETER_H

#include "games/xeen/XeenEventDecoder.h"
#include "games/xeen/XeenEventScript.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenParty.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mmodern {

class XeenWorld;

struct XeenEventExecutionAddress {
	std::uint16_t mapId = 0;
	int x = 0;
	int y = 0;
	int line = 0;
};

struct XeenEventExecutionCompleted {
	XeenCamera finalCamera;
	XeenGameFlags finalGameFlags;
	std::size_t instructionCount = 0;
};

enum class XeenPresentationKind {
	CenteredMessage,
	SceneLabelReduced,
	SceneLabelNormal,
	SceneLabelSign,
	BottomWindowMessage,
	BottomWindowTwoLines,
	MainWindowMessage,
	Confirmation
};

enum class XeenPresentationResponseRequirement {
	Presented,
	Acknowledgment,
	YesNo
};

enum class XeenPresentationResponse {
	Presented,
	Acknowledged,
	Yes,
	No
};

struct XeenPresentationRequest {
	XeenPresentationKind kind = XeenPresentationKind::CenteredMessage;
	XeenPresentationResponseRequirement response =
		XeenPresentationResponseRequirement::Presented;
	std::uint16_t mapId = 0;
	std::optional<std::uint8_t> textIndex;
	std::string text;
	std::optional<std::uint8_t> layoutValue;
	XeenEventSourceLocation source;
};

enum class XeenEventMissingInstructionPolicy {
	NaturalCompletion,
	ExplicitJump,
	ExplicitCall
};

enum class XeenEventPendingContinuation {
	Advance,
	Terminate,
	ConditionalAction44
};

struct XeenEventPendingPresentation {
	XeenPresentationRequest request;
	XeenEventPendingContinuation continuation = XeenEventPendingContinuation::Advance;
	std::optional<XeenEventConditional> conditional;
};

struct XeenEventCallFrame {
	XeenEventExecutionAddress returnAddress;
};

struct XeenEventExecutionState {
	XeenEventExecutionAddress logicalAddress;
	XeenDirection lookupDirection = XeenDirection::North;
	XeenCamera workingCamera;
	XeenGameFlags workingGameFlags;
	std::optional<XeenEventScript> currentScript;
	std::vector<XeenEventCallFrame> callStack;
	std::size_t instructionCount = 0;
	XeenEventMissingInstructionPolicy missingInstructionPolicy =
		XeenEventMissingInstructionPolicy::NaturalCompletion;
	std::optional<XeenEventSourceLocation> pendingTransferSource;
	std::optional<XeenEventPendingPresentation> pendingPresentation;
};

struct XeenEventExecutionSuspended {
	XeenEventExecutionState state;
	XeenPresentationRequest request;
};

enum class XeenEventExecutionErrorKind {
	InvalidInitialCamera,
	ScriptLoadFailed,
	ScriptMapMismatch,
	MalformedInstruction,
	UnsupportedOpcode,
	UnsupportedOperand,
	LineOverflow,
	UnsupportedConditionAction,
	UnsupportedOperationMode,
	EmptyParty,
	InvalidFlagIndex,
	InvalidJumpTarget,
	InvalidCallTarget,
	InvalidReturn,
	CallStackOverflow,
	UnsupportedTeleportDestination,
	MapLoadFailed,
	UnsupportedExecutionContext,
	InstructionLimitExceeded,
	MissingTextResource,
	InvalidTextIndex,
	InvalidPresentationResponse,
	PresentationRequired
};

struct XeenEventExecutionError {
	XeenEventExecutionErrorKind kind =
		XeenEventExecutionErrorKind::InvalidInitialCamera;
	std::string message;
	std::size_t instructionCount = 0;
	XeenEventExecutionAddress logicalAddress;
	std::optional<XeenEventSourceLocation> source;
	std::optional<XeenEventExecutionAddress> requestedTarget;
};

using XeenEventExecutionResult = std::variant<
	XeenEventExecutionCompleted,
	XeenEventExecutionError>;

using XeenEventExecutionStepResult = std::variant<
	XeenEventExecutionCompleted,
	XeenEventExecutionSuspended,
	XeenEventExecutionError>;

class XeenEventInterpreter {
public:
	static constexpr std::size_t kMaximumInstructions = 1024;
	static constexpr std::size_t kMaximumCallDepth = 64;

	using ScriptProvider = std::function<XeenEventScript(std::uint16_t mapId)>;
	using TextProvider = std::function<XeenEventTextFile(std::uint16_t mapId)>;

	XeenEventExecutionResult execute(const XeenCamera &initialCamera,
		const XeenPartyState &partyState, const XeenGameFlags &gameFlags,
		XeenWorld &world, const ScriptProvider &scriptProvider) const;

	XeenEventExecutionStepResult begin(const XeenCamera &initialCamera,
		const XeenPartyState &partyState, const XeenGameFlags &gameFlags,
		XeenWorld &world, const ScriptProvider &scriptProvider,
		const TextProvider &textProvider) const;

	XeenEventExecutionStepResult resume(XeenEventExecutionState state,
		XeenPresentationResponse response, const XeenPartyState &partyState,
		XeenWorld &world, const ScriptProvider &scriptProvider,
		const TextProvider &textProvider) const;

private:
	XeenEventExecutionStepResult run(XeenEventExecutionState state,
		std::optional<XeenPresentationResponse> response,
		const XeenPartyState &partyState, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider) const;
};

} // namespace mmodern

#endif
