#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_INTERPRETER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_INTERPRETER_H

#include "games/xeen/XeenEventDecoder.h"
#include "games/xeen/XeenEventScript.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenParty.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>

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
	InstructionLimitExceeded
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

class XeenEventInterpreter {
public:
	static constexpr std::size_t kMaximumInstructions = 1024;
	static constexpr std::size_t kMaximumCallDepth = 64;

	using ScriptProvider = std::function<XeenEventScript(std::uint16_t mapId)>;

	XeenEventExecutionResult execute(const XeenCamera &initialCamera,
		const XeenPartyState &partyState, const XeenGameFlags &gameFlags,
		XeenWorld &world, const ScriptProvider &scriptProvider) const;
};

} // namespace mmodern

#endif
