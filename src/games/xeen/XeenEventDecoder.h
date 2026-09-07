#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_DECODER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_DECODER_H

#include "formats/xeen/XeenEventFormat.h"

#include "games/xeen/XeenMapIdentity.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace mmodern {

struct XeenEventDecodeContext {
	std::optional<XeenMapIdentity> mapId;
	std::optional<std::string> resourceName;
};

struct XeenEventSourceLocation {
	std::optional<XeenMapIdentity> mapId;
	std::optional<std::string> resourceName;
	std::size_t fileOffset = 0;
	std::uint8_t x = 0;
	std::uint8_t y = 0;
	std::uint8_t direction = 0;
	std::uint8_t line = 0;
	std::uint8_t opcode = 0;
};

struct XeenEventNone {};
struct XeenEventExit {};
struct XeenEventReturn {};

enum class XeenEventDisplayKind {
	Centered,
	DoorLabelReduced,
	DoorLabelNormal,
	SignLabel,
	BottomWindow,
	BottomWindowTwoLines,
	MainWindow
};

struct XeenEventDisplay {
	XeenEventDisplayKind kind = XeenEventDisplayKind::Centered;
	std::uint8_t textIndex = 0;
	// Only DisplayBottomTwoLines has a leading layout byte. It is retained
	// independently and is never interpreted as another text index.
	std::optional<std::uint8_t> layoutValue;
};

struct XeenEventTeleportAndExit {
	std::uint8_t mapId = 0;
	int x = 0;
	int y = 0;
};

struct XeenEventTeleportAndContinue {
	std::uint8_t mapId = 0;
	int x = 0;
	int y = 0;
};

struct XeenEventCallEvent {
	int x = 0;
	int y = 0;
	std::uint8_t line = 0;
};

enum class XeenEventComparison {
	GreaterOrEqual,
	Equal,
	LessOrEqual
};

struct XeenEventConditional {
	XeenEventComparison comparison = XeenEventComparison::Equal;
	std::uint8_t action = 0;
	std::uint32_t value = 0;
	std::uint8_t targetLine = 0;
};

struct XeenEventTakeOrGivePair {
	std::uint8_t mode = 0;
	std::uint32_t value = 0;
};

struct XeenEventTakeOrGive {
	XeenEventTakeOrGivePair first;
	XeenEventTakeOrGivePair second;
	XeenEventTakeOrGivePair third;
};

using XeenDecodedEventOperation = std::variant<
	XeenEventNone,
	XeenEventExit,
	XeenEventReturn,
	XeenEventDisplay,
	XeenEventTeleportAndExit,
	XeenEventTeleportAndContinue,
	XeenEventCallEvent,
	XeenEventConditional,
	XeenEventTakeOrGive>;

struct XeenDecodedEventInstruction {
	XeenEventSourceLocation source;
	XeenDecodedEventOperation operation;
};

enum class XeenEventDecodeErrorKind {
	MalformedInstruction,
	UnsupportedOpcode,
	UnsupportedOperand
};

struct XeenEventDecodeError {
	XeenEventDecodeErrorKind kind = XeenEventDecodeErrorKind::MalformedInstruction;
	XeenEventSourceLocation source;
	std::string message;
	std::optional<std::size_t> expectedParameterSize;
	std::size_t actualParameterSize = 0;
};

using XeenEventDecodeResult = std::variant<
	XeenDecodedEventInstruction,
	XeenEventDecodeError>;

// Strictly decodes the supported MMModern subset. Unlike the original runtime,
// supported direct forms must consume exactly all bytes in record.parameters.
class XeenEventDecoder {
public:
	static XeenEventDecodeResult decode(const XeenEventRecord &record,
		XeenEventDecodeContext context = {});
};

} // namespace mmodern

#endif
