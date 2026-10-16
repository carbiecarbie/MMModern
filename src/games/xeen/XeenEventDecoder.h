#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_DECODER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_DECODER_H

#include "formats/xeen/XeenEventFormat.h"

#include "games/xeen/XeenMapIdentity.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mmodern {

struct XeenEventDecodeContext {
	std::optional<XeenMapIdentity> mapId;
	std::optional<std::string> resourceName;
	std::optional<std::size_t> recordIndex;
	bool templeRecovery = false;
	// AlterEvent changes the opcode, retaining its original operand storage.
	bool replacedOpcode = false;
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
	std::optional<std::size_t> recordIndex;
};

struct XeenEventNone {};
struct XeenEventRemove {};
struct XeenEventExit {};
struct XeenEventTownService { std::uint8_t action = 0; };
struct XeenEventReturn {};
struct XeenEventNpc {
	std::uint8_t titleTextIndex, bodyTextIndex, portraitId, confirmationMode, targetLine;
};
struct XeenEventWhoWill {
	std::uint8_t verbIndex = 0;
	std::uint8_t textIndex = 0;
};

enum class XeenEventDisplayKind {
	Centered,
	DoorLabelReduced,
	DoorLabelNormal,
	SignLabel,
	SignLabelReduced,
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

struct XeenEventGiveEnchanted {
	std::uint8_t itemCode = 0;
	std::uint8_t specialId = 0;
	// Owned diagnostic bytes; no gameplay meaning in the bounded misc branch.
	std::vector<std::uint8_t> suffix;
};
struct XeenEventVoiceCue { std::uint8_t index = 0; };
struct XeenEventSoundCue { std::uint8_t index = 0; };
// Presentation-only CD speech operands; the DOS encoding is MMSSFF (75 Hz).
struct XeenEventCdSpeech { std::uint8_t track = 0; std::uint16_t start = 0, finish = 0; };
struct XeenEventSpawn { std::uint8_t slot=0; int x=0,y=0; std::uint8_t unused=0; };
struct XeenEventAlterEvent { std::uint8_t line=0, replacement=0; };
struct XeenEventMoveWallObject { std::uint8_t record=0; int x=0,y=0; };
struct XeenEventMakeNothingHere {};
struct XeenEventAlterMap { int x=0,y=0; std::uint8_t direction=0,wall=0; };
struct XeenEventAlterCellFlag { int x=0,y=0; std::uint8_t surface=0; };
// SetVar keeps the mode-specific operand width (see XeenEventValues.h).
struct XeenEventSetVar { std::uint8_t mode=0; std::uint32_t value=0; };
struct XeenEventProtectionCheck {};
struct XeenEventAlterHeads { std::uint8_t left=0,right=0; };
struct XeenEventDamage { std::uint16_t amount=0;std::uint8_t type=0; };
struct XeenEventMoveObject {std::uint8_t record=0;int x=0,y=0;};
struct XeenEventSetCharacter {std::uint8_t selector=0;};

struct XeenEventTakeOrGivePair {
	std::uint8_t mode = 0;
	std::uint32_t value = 0;
};

struct XeenEventTakeOrGive {
	XeenEventTakeOrGivePair first;
	XeenEventTakeOrGivePair second;
	XeenEventTakeOrGivePair third;
};
struct XeenEventGiveMulti : XeenEventTakeOrGive {};
// TakeOrGive_2/_3/_4 (opcodes 28, 29, 38): ifProc test(s), then a give-only giveTake.
struct XeenEventTakeOrGiveTest : XeenEventTakeOrGive {
	std::uint8_t opcode = 28;
};
// JumpRnd (23): draw 1..range, jump when it equals match. Operands are signed bytes.
struct XeenEventJumpRandom { int range = 0, match = 0, line = 0; };
// RndDamage (33): typed damage of 1..maximum. The third byte is not read.
struct XeenEventRandomDamage { std::uint8_t type = 0; int maximum = 0; std::optional<std::uint8_t> unused; };
// IfMapFlag (42): continue while the monster (or any, 0xff) is on the map, else jump.
struct XeenEventIfMapFlag { std::uint8_t monster = 0; int line = 0; };
struct XeenEventSelectRandomCharacter {};
// ItemType (45): the special-event item for the run's later item generation.
struct XeenEventItemType { int itemType = 0; };

using XeenDecodedEventOperation = std::variant<
	XeenEventNone,
	XeenEventRemove,
	XeenEventExit,
	XeenEventTownService,
	XeenEventReturn,
	XeenEventNpc,
	XeenEventWhoWill,
	XeenEventDisplay,
	XeenEventTeleportAndExit,
	XeenEventTeleportAndContinue,
	XeenEventCallEvent,
	XeenEventConditional,
	XeenEventTakeOrGive,
	XeenEventGiveMulti,
	XeenEventMoveObject,
	XeenEventSetCharacter,
	XeenEventGiveEnchanted,
	XeenEventVoiceCue,
	XeenEventSoundCue,
	XeenEventCdSpeech,
	XeenEventSpawn,
	XeenEventAlterEvent,
	XeenEventMoveWallObject,
	XeenEventMakeNothingHere,
	XeenEventAlterMap,
	XeenEventAlterCellFlag,
	XeenEventSetVar,
	XeenEventProtectionCheck,
	XeenEventAlterHeads,
	XeenEventDamage,
	XeenEventTakeOrGiveTest,
	XeenEventJumpRandom,
	XeenEventRandomDamage,
	XeenEventIfMapFlag,
	XeenEventSelectRandomCharacter,
	XeenEventItemType>;

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
// supported direct forms account for all bytes in record.parameters. The bounded
// GiveEnchanted form owns up to two unused suffix bytes for diagnostics.
class XeenEventDecoder {
public:
	static XeenEventDecodeResult decode(const XeenEventRecord &record,
		XeenEventDecodeContext context = {});
};

} // namespace mmodern

#endif
