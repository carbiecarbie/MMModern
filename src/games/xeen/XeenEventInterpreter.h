#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_INTERPRETER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_INTERPRETER_H

#include "games/xeen/XeenEventDecoder.h"
#include "games/xeen/XeenEventScript.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenRecordIdentity.h"

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
	XeenMapIdentity mapId = 0;
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
	Confirmation,
	NpcAcknowledgment,
	CharacterSelection
};

enum class XeenPresentationResponseRequirement {
	Presented,
	Acknowledgment,
	YesNo,
	CharacterSelection
};

struct SelectedCharacter { std::size_t partyIndex; };
struct CharacterSelectionCancelled {};

struct XeenPresentationResponse {
	enum Signal {
		Presented,
		Acknowledged,
		Yes,
		No
	};
	std::variant<Signal, SelectedCharacter, CharacterSelectionCancelled> value;
	XeenPresentationResponse(Signal signal) : value(signal) {}
	XeenPresentationResponse(SelectedCharacter selection) : value(selection) {}
	XeenPresentationResponse(CharacterSelectionCancelled cancel) : value(cancel) {}
	bool operator==(Signal signal) const {
		const auto *actual = std::get_if<Signal>(&value);
		return actual && *actual == signal;
	}
	bool operator!=(Signal signal) const { return !(*this == signal); }
};

struct XeenCharacterSelectionMember {
	std::size_t partyIndex;
	std::uint8_t rosterId;
	std::string name;
	bool eligible;
};

struct XeenPresentationRequest {
	XeenPresentationKind kind = XeenPresentationKind::CenteredMessage;
	XeenPresentationResponseRequirement response =
		XeenPresentationResponseRequirement::Presented;
	XeenMapIdentity mapId = 0;
	std::optional<std::uint8_t> textIndex;
	std::string text;
	std::optional<std::uint8_t> layoutValue;
	XeenEventSourceLocation source;
	std::vector<XeenCharacterSelectionMember> members;
	std::optional<std::uint8_t> verbIndex;
	std::string refusal;
	std::optional<XeenEventNpc> npc;
	std::string title;
};

enum class XeenEventMissingInstructionPolicy {
	NaturalCompletion,
	ExplicitJump,
	ExplicitCall
};

enum class XeenEventPendingContinuation {
	Advance,
	Terminate,
	ConditionalAction44,
	WhoWill
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
	std::optional<XeenObjectIdentity> selectedObject;
	// Temporary active-party index; independent of roster IDs and call frames.
	std::optional<std::size_t> activeCharacterIndex;
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
	QuestItemOverflow,
	EmptyParty,
	InvalidFlagIndex,
	InvalidJumpTarget,
	InvalidCallTarget,
	InvalidReturn,
	CallStackOverflow,
	UnsupportedTeleportDestination,
	MapLoadFailed,
	ObjectLoadFailed,
	InvalidRemoveContext,
	UnsupportedExecutionContext,
	InstructionLimitExceeded,
	MissingTextResource,
	TextMapMismatch,
	InvalidTextIndex,
	InvalidPresentationResponse,
	PresentationRequired,
	PresentationFailed
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

	using ScriptProvider = std::function<XeenEventScript(XeenMapIdentity mapId)>;
	using TextProvider = std::function<XeenEventTextFile(XeenMapIdentity mapId)>;

	XeenEventExecutionResult execute(const XeenCamera &initialCamera,
		XeenPartyState &partyState, const XeenGameFlags &gameFlags,
		XeenWorld &world, const ScriptProvider &scriptProvider) const;

	// Integration checkpoints may specify a line; all gameplay callers use zero.
	XeenEventExecutionStepResult begin(const XeenCamera &initialCamera,
		XeenPartyState &partyState, const XeenGameFlags &gameFlags,
		XeenWorld &world, const ScriptProvider &scriptProvider,
		const TextProvider &textProvider, std::uint8_t initialLine = 0) const;

	XeenEventExecutionStepResult resume(XeenEventExecutionState state,
		XeenPresentationResponse response, XeenPartyState &partyState,
		XeenWorld &world, const ScriptProvider &scriptProvider,
		const TextProvider &textProvider) const;

private:
	XeenEventExecutionStepResult run(XeenEventExecutionState state,
		std::optional<XeenPresentationResponse> response,
		XeenPartyState &partyState, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider) const;
};

} // namespace mmodern

#endif
