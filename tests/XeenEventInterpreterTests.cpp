#include "games/xeen/XeenEventInterpreter.h"

#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenWorld.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace mmodern;

namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

XeenEventRecord record(std::uint8_t x, std::uint8_t y, std::uint8_t line,
		std::uint8_t opcode, std::vector<std::uint8_t> parameters = {},
		std::uint8_t direction = kXeenEventDirectionAll,
		std::size_t offset = 1) {
	XeenEventRecord result;
	result.fileOffset = offset;
	result.lengthField = static_cast<std::uint8_t>(5 + parameters.size());
	result.x = x;
	result.y = y;
	result.direction = direction;
	result.line = line;
	result.opcode = opcode;
	result.parameters = std::move(parameters);
	return result;
}

XeenEventScript script(std::uint16_t mapId,
		std::vector<XeenEventRecord> records = {}, bool present = true) {
	XeenEventFile file;
	file.mapId = mapId;
	file.resourceName = "maze" + std::to_string(mapId) + ".evt";
	file.resourcePresent = present;
	file.records = std::move(records);
	return XeenEventScript(std::move(file));
}

XeenMap map(std::uint16_t mapId, bool outdoors = true) {
	XeenMap result;
	result.geometry.id = mapId;
	result.geometry.flags2 = outdoors ? 0x8000 : 0;
	return result;
}

XeenPartyState partyWithSp(std::int16_t sp, std::uint8_t rosterId = 0) {
	std::vector<std::uint8_t> roster(
		XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize, 0);
	const std::size_t recordOffset =
		static_cast<std::size_t>(rosterId) * XeenCharacter::kSerializedSize;
	const std::uint16_t serialized = static_cast<std::uint16_t>(sp);
	roster[recordOffset + 344] = static_cast<std::uint8_t>(serialized & 0xff);
	roster[recordOffset + 345] = static_cast<std::uint8_t>(serialized >> 8);
	std::vector<std::uint8_t> partyBytes(10, 0xff);
	partyBytes[0] = 1;
	partyBytes[1] = 1;
	partyBytes[2] = rosterId;
	return XeenPartyLoader().loadFromResources(roster, partyBytes);
}

XeenPartyState emptyParty() {
	return {};
}

class Fixture {
public:
	Fixture() : world([this](std::uint16_t mapId) {
		if (failingMaps.count(mapId))
			throw std::runtime_error("synthetic map load failure");
		++mapLoads[mapId];
		return map(mapId, mapId != 33);
	}) {}

	XeenEventInterpreter::ScriptProvider provider() {
		return [this](std::uint16_t mapId) {
			++scriptLoads[mapId];
			if (throwingScripts.count(mapId))
				throw std::runtime_error("synthetic script load failure");
			const auto found = scripts.find(mapId);
			return found == scripts.end() ? script(mapId, {}, false) : found->second;
		};
	}

	XeenEventExecutionResult execute(const XeenCamera &camera,
			const XeenPartyState &party, const XeenGameFlags &flags) {
		return interpreter.execute(camera, party, flags, world, provider());
	}

	std::map<std::uint16_t, XeenEventScript> scripts;
	std::map<std::uint16_t, int> scriptLoads;
	std::map<std::uint16_t, int> mapLoads;
	std::map<std::uint16_t, bool> failingMaps;
	std::map<std::uint16_t, bool> throwingScripts;
	XeenWorld world;
	XeenEventInterpreter interpreter;
};

XeenEventExecutionCompleted success(const XeenEventExecutionResult &result) {
	const auto *value = std::get_if<XeenEventExecutionCompleted>(&result);
	check(value != nullptr, "expected successful execution");
	return *value;
}

XeenEventExecutionError failure(const XeenEventExecutionResult &result,
		XeenEventExecutionErrorKind kind) {
	const auto *value = std::get_if<XeenEventExecutionError>(&result);
	check(value && value->kind == kind, "unexpected execution error");
	return *value;
}

void checkCamera(const XeenCamera &camera, std::uint16_t mapId, int x, int y,
		XeenDirection direction, const char *message) {
	check(camera.mapId == mapId && camera.x == x && camera.y == y &&
		camera.direction == direction, message);
}

std::vector<std::uint8_t> setFlag(std::uint8_t flag) {
	return {0, 0, 20, flag};
}

std::vector<std::uint8_t> clearFlag(std::uint8_t flag) {
	return {20, flag};
}

void testNaturalFlowAndFirstMatch() {
	const XeenCamera camera{1, 1, 1, XeenDirection::East};
	const auto party = partyWithSp(2);
	XeenGameFlags flags;
	flags.set(100);

	Fixture absent;
	const auto absentResult = success(absent.execute(camera, party, flags));
	check(absentResult.instructionCount == 0, "missing initial event");
	checkCamera(absentResult.finalCamera, 1, 1, 1, XeenDirection::East,
		"missing initial event camera");
	check(absentResult.finalGameFlags.values() == flags.values(),
		"natural success returns unchanged owned flags");

	Fixture sequential;
	sequential.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x00), record(1, 1, 1, 0x12)
	}));
	check(success(sequential.execute(camera, party, flags)).instructionCount == 2,
		"None advances to Exit");

	Fixture natural;
	natural.scripts.emplace(1, script(1, {record(1, 1, 0, 0x00)}));
	check(success(natural.execute(camera, party, flags)).instructionCount == 1,
		"missing sequential line is natural completion");

	Fixture duplicate;
	duplicate.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x12, {}, kXeenEventDirectionAll, 10),
		record(1, 1, 0, 0x07, {2, 3, 4}, kXeenEventDirectionAll, 20)
	}));
	const auto first = success(duplicate.execute(camera, party, flags));
	check(first.instructionCount == 1 && first.finalCamera.mapId == 1,
		"interpreter preserves First Match Wins");

	Fixture overflow;
	overflow.scripts.emplace(1, script(1, {record(1, 1, 255, 0x00)}));
	const XeenCamera overflowCamera{1, 1, 1, XeenDirection::North};
	// Enter line 255 through an explicit call from line zero.
	overflow.scripts.insert_or_assign(1, script(1, {
		record(1, 1, 0, 0x19, {75, 75, 255}),
		record(75, 75, 255, 0x00)
	}));
	const auto overflowError = failure(overflow.execute(overflowCamera, party, flags),
		XeenEventExecutionErrorKind::LineOverflow);
	check(overflowError.instructionCount == 2, "line 255 overflow count");
}

void testConditions() {
	const XeenCamera camera{1, 1, 1, XeenDirection::North};
	const auto party = partyWithSp(2, 18);
	XeenGameFlags clearFlags;

	Fixture spTrue;
	spTrue.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x08, {9, 2, 2}),
		record(1, 1, 1, 0x07, {2, 1, 1}),
		record(1, 1, 2, 0x12)
	}));
	check(success(spTrue.execute(camera, party, clearFlags)).instructionCount == 2,
		"SP GreaterOrEqual true branch");

	Fixture spFalse;
	spFalse.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0a, {9, 1, 2}),
		record(1, 1, 1, 0x12), record(1, 1, 2, 0x07, {2, 1, 1})
	}));
	check(success(spFalse.execute(camera, party, clearFlags)).instructionCount == 2,
		"SP LessOrEqual false falls through");

	Fixture negativeSp;
	negativeSp.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x08, {9, 255, 1}), record(1, 1, 1, 0x12)
	}));
	check(success(negativeSp.execute(camera, partyWithSp(-1), clearFlags))
		.instructionCount == 2, "negative SP converts to uint32");

	Fixture empty;
	empty.scripts.emplace(1, script(1, {record(1, 1, 0, 0x09, {9, 0, 1})}));
	failure(empty.execute(camera, emptyParty(), clearFlags),
		XeenEventExecutionErrorKind::EmptyParty);

	for (const auto comparison : {std::uint8_t{0x08}, std::uint8_t{0x09},
			std::uint8_t{0x0a}}) {
		for (const bool enabled : {false, true}) {
			Fixture fixture;
			fixture.scripts.emplace(1, script(1, {
				record(1, 1, 0, comparison, {20, 25, 2}),
				record(1, 1, 1, 0x12),
				record(1, 1, 2, 0x07, {2, 3, 4})
			}));
			XeenGameFlags flags;
			if (enabled)
				flags.set(25);
			const bool expectedTrue = comparison == 0x08 || enabled;
			const auto result = fixture.execute(camera, party, flags);
			const auto completed = success(result);
			check(completed.instructionCount == 2 &&
				(completed.finalCamera.mapId == 2) == expectedTrue,
				"game flag comparison branch");
		}
	}

	Fixture boundaryFlags;
	boundaryFlags.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x09, {20, 0, 1}),
		record(1, 1, 1, 0x09, {20, 255, 2}),
		record(1, 1, 2, 0x12)
	}));
	XeenGameFlags flags;
	flags.set(0);
	flags.set(255);
	check(success(boundaryFlags.execute(camera, party, flags)).instructionCount == 3,
		"game flag indices 0 and 255");

	Fixture unsupported;
	unsupported.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x09, {21, 1, 1})
	}));
	failure(unsupported.execute(camera, party, flags),
		XeenEventExecutionErrorKind::UnsupportedConditionAction);

	Fixture missingJump;
	missingJump.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x09, {9, 2, 8})
	}));
	failure(missingJump.execute(camera, party, flags),
		XeenEventExecutionErrorKind::InvalidJumpTarget);

	Fixture untakenMissingJump;
	untakenMissingJump.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x09, {9, 99, 8})
	}));
	check(success(untakenMissingJump.execute(camera, party, flags)).instructionCount == 1,
		"untaken missing target is irrelevant");
}

void testCallsReturnsAndStack() {
	const XeenCamera camera{1, 1, 1, XeenDirection::South};
	const auto party = partyWithSp(2);
	const XeenGameFlags flags;

	Fixture calls;
	calls.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {75, 102, 0}),
		record(75, 102, 0, 0x1a), record(1, 1, 1, 0x12)
	}));
	check(success(calls.execute(camera, party, flags)).instructionCount == 3,
		"CallEvent logical address and Return");

	Fixture nested;
	nested.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {75, 76, 0}),
		record(75, 76, 0, 0x19, {102, 102, 0}),
		record(102, 102, 0, 0x1a), record(75, 76, 1, 0x1a),
		record(1, 1, 1, 0x12)
	}));
	check(success(nested.execute(camera, party, flags)).instructionCount == 5,
		"nested calls and returns");

	Fixture returnNatural;
	returnNatural.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {75, 75, 0}), record(75, 75, 0, 0x1a)
	}));
	check(success(returnNatural.execute(camera, party, flags)).instructionCount == 2,
		"missing sequential line after Return is natural completion");

	Fixture invalidReturn;
	invalidReturn.scripts.emplace(1, script(1, {record(1, 1, 0, 0x1a)}));
	failure(invalidReturn.execute(camera, party, flags),
		XeenEventExecutionErrorKind::InvalidReturn);

	Fixture missingTarget;
	missingTarget.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {75, 102, 9})
	}));
	const auto missing = failure(missingTarget.execute(camera, party, flags),
		XeenEventExecutionErrorKind::InvalidCallTarget);
	check(missing.requestedTarget && missing.requestedTarget->x == 75 &&
		missing.requestedTarget->y == 102, "missing CallEvent target metadata");

	Fixture negative;
	negative.scripts.emplace(1, script(1, {record(1, 1, 0, 0x19, {0xff, 1, 0})}));
	const auto negativeError = failure(negative.execute(camera, party, flags),
		XeenEventExecutionErrorKind::InvalidCallTarget);
	check(negativeError.requestedTarget && negativeError.requestedTarget->x == -1,
		"negative CallEvent target is not wrapped");

	Fixture returnOverflow;
	returnOverflow.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {75, 75, 255}),
		record(75, 75, 255, 0x19, {76, 76, 0})
	}));
	failure(returnOverflow.execute(camera, party, flags),
		XeenEventExecutionErrorKind::LineOverflow);

	Fixture stack;
	stack.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {75, 75, 0}),
		record(75, 75, 0, 0x19, {75, 75, 0})
	}));
	const auto stackError = failure(stack.execute(camera, party, flags),
		XeenEventExecutionErrorKind::CallStackOverflow);
	check(stackError.instructionCount == 65, "64 pending calls allowed, 65th rejected");
}

void testTeleportsAndTransactionalResult() {
	const XeenCamera camera{1, 1, 1, XeenDirection::West};
	const auto party = partyWithSp(2);
	const XeenGameFlags flags;

	Fixture exit;
	exit.scripts.emplace(1, script(1, {record(1, 1, 0, 0x07, {33, 4, 5})}));
	const auto indoor = success(exit.execute(camera, party, flags));
	checkCamera(indoor.finalCamera, 33, 4, 5, XeenDirection::West,
		"TeleportAndExit to interior");
	check(exit.scriptLoads[33] == 0, "TeleportAndExit does not load destination script");

	Fixture outdoor;
	outdoor.scripts.emplace(1, script(1, {record(1, 1, 0, 0x07, {2, 6, 7})}));
	checkCamera(success(outdoor.execute(camera, party, flags)).finalCamera,
		2, 6, 7, XeenDirection::West, "TeleportAndExit to exterior");

	Fixture invalidCoordinates;
	invalidCoordinates.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x07, {2, 16, 1})
	}));
	failure(invalidCoordinates.execute(camera, party, flags),
		XeenEventExecutionErrorKind::UnsupportedTeleportDestination);
	checkCamera(camera, 1, 1, 1, XeenDirection::West,
		"invalid teleport preserves input camera");

	Fixture mapFailure;
	mapFailure.failingMaps[2] = true;
	mapFailure.scripts.emplace(1, script(1, {record(1, 1, 0, 0x07, {2, 1, 1})}));
	failure(mapFailure.execute(camera, party, flags),
		XeenEventExecutionErrorKind::MapLoadFailed);

	Fixture continuation;
	continuation.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x1f, {2, 3, 4})
	}));
	continuation.scripts.emplace(2, script(2, {record(3, 4, 0, 0x12)}));
	const auto continued = success(continuation.execute(camera, party, flags));
	checkCamera(continued.finalCamera, 2, 3, 4, XeenDirection::West,
		"TeleportAndContinue final camera");
	check(continued.instructionCount == 2, "TeleportAndContinue dispatch count");

	Fixture continuationNatural;
	continuationNatural.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x1f, {2, 3, 4})
	}));
	const auto noDestinationEvent = success(
		continuationNatural.execute(camera, party, flags));
	check(noDestinationEvent.instructionCount == 1 &&
		noDestinationEvent.finalCamera.mapId == 2,
		"TeleportAndContinue without destination event");

	Fixture activeStack;
	activeStack.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {75, 75, 0}),
		record(75, 75, 0, 0x1f, {2, 3, 4})
	}));
	failure(activeStack.execute(camera, party, flags),
		XeenEventExecutionErrorKind::UnsupportedExecutionContext);

	Fixture mirror;
	mirror.scripts.emplace(1, script(1, {record(1, 1, 0, 0x07, {0})}));
	failure(mirror.execute(camera, party, flags),
		XeenEventExecutionErrorKind::UnsupportedOperand);

	Fixture malformed;
	malformed.scripts.emplace(1, script(1, {record(1, 1, 0, 0x12, {1})}));
	failure(malformed.execute(camera, party, flags),
		XeenEventExecutionErrorKind::MalformedInstruction);

	Fixture unsupported;
	unsupported.scripts.emplace(1, script(1, {record(1, 1, 0, 0x06)}));
	failure(unsupported.execute(camera, party, flags),
		XeenEventExecutionErrorKind::UnsupportedOpcode);
}

void testTakeOrGiveFlagOperations() {
	const XeenCamera camera{1, 1, 1, XeenDirection::North};
	const auto party = partyWithSp(2);

	Fixture set;
	set.scripts.emplace(1, script(1, {record(1, 1, 0, 0x0c, setFlag(25))}));
	XeenGameFlags clearInput;
	clearInput.set(42);
	const auto setResult = success(set.execute(camera, party, clearInput));
	check(setResult.instructionCount == 1 && setResult.finalGameFlags.isSet(25) &&
		setResult.finalGameFlags.isSet(42) && !clearInput.isSet(25),
		"SET returns owned flags and preserves unrelated/input flags");

	Fixture idempotentSet;
	idempotentSet.scripts.emplace(1, script(1,
		{record(1, 1, 0, 0x0c, setFlag(25))}));
	XeenGameFlags alreadySet;
	alreadySet.set(25);
	check(success(idempotentSet.execute(camera, party, alreadySet))
		.finalGameFlags.isSet(25), "SET is idempotent");

	Fixture clear;
	clear.scripts.emplace(1, script(1,
		{record(1, 1, 0, 0x0c, clearFlag(25))}));
	const auto clearResult = success(clear.execute(camera, party, alreadySet));
	check(!clearResult.finalGameFlags.isSet(25) && alreadySet.isSet(25),
		"CLEAR returns owned flags and preserves input");
	XeenGameFlags alreadyClear;
	check(!success(clear.execute(camera, party, alreadyClear))
		.finalGameFlags.isSet(25), "CLEAR is idempotent");

	Fixture boundaries;
	boundaries.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(0)),
		record(1, 1, 1, 0x0c, setFlag(255)),
		record(1, 1, 2, 0x0c, clearFlag(0))
	}));
	const auto boundaryResult = success(boundaries.execute(camera, party, {}));
	check(boundaryResult.instructionCount == 3 &&
		!boundaryResult.finalGameFlags.isSet(0) &&
		boundaryResult.finalGameFlags.isSet(255),
		"flag boundaries and multiple mutations");

	XeenGameFlags ownedInput;
	Fixture owned;
	owned.scripts.emplace(1, script(1, {record(1, 1, 0, 0x0c, setFlag(7))}));
	const auto ownedResult = success(owned.execute(camera, party, ownedInput));
	ownedInput.set(8);
	check(ownedResult.finalGameFlags.isSet(7) &&
		!ownedResult.finalGameFlags.isSet(8), "Completed owns finalGameFlags by value");

	for (const auto &parameters : std::vector<std::vector<std::uint8_t>>{
			{}, {20, 1, 20, 2}, {9, 1}, {0, 7, 20, 1},
			{0, 0, 20, 1, 1, 1}, {0, 0}}) {
		Fixture unsupported;
		unsupported.scripts.emplace(1, script(1,
			{record(1, 1, 0, 0x0c, parameters)}));
		const auto result = failure(unsupported.execute(camera, party, {}),
			XeenEventExecutionErrorKind::UnsupportedOperationMode);
		check(result.instructionCount == 1,
			"unsupported TakeOrGive combination counts as dispatched");
	}

	Fixture malformed;
	malformed.scripts.emplace(1, script(1, {record(1, 1, 0, 0x0c, {20})}));
	check(failure(malformed.execute(camera, party, {}),
		XeenEventExecutionErrorKind::MalformedInstruction).instructionCount == 0,
		"malformed TakeOrGive does not count");

	Fixture overflow;
	overflow.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {75, 75, 255}),
		record(75, 75, 255, 0x0c, setFlag(25))
	}));
	XeenGameFlags overflowInput;
	const auto overflowError = failure(overflow.execute(camera, party, overflowInput),
		XeenEventExecutionErrorKind::LineOverflow);
	check(overflowError.instructionCount == 2 && !overflowInput.isSet(25),
		"TakeOrGive line overflow rolls back flags");
}

void testTakeOrGiveVisibilityAcrossFlow() {
	const XeenCamera camera{1, 1, 1, XeenDirection::East};
	const auto party = partyWithSp(2);

	Fixture setBranch;
	setBranch.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(25)),
		record(1, 1, 1, 0x09, {20, 25, 3}),
		record(1, 1, 2, 0x07, {2, 2, 2}),
		record(1, 1, 3, 0x07, {3, 3, 3})
	}));
	const auto setResult = success(setBranch.execute(camera, party, {}));
	check(setResult.finalCamera.mapId == 3 && setResult.finalGameFlags.isSet(25) &&
		setResult.instructionCount == 3, "condition sees preceding SET");

	Fixture clearBranch;
	clearBranch.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, clearFlag(25)),
		record(1, 1, 1, 0x09, {20, 25, 3}),
		record(1, 1, 2, 0x07, {2, 2, 2}),
		record(1, 1, 3, 0x07, {3, 3, 3})
	}));
	XeenGameFlags setInput;
	setInput.set(25);
	const auto clearResult = success(clearBranch.execute(camera, party, setInput));
	check(clearResult.finalCamera.mapId == 2 &&
		!clearResult.finalGameFlags.isSet(25) && setInput.isSet(25),
		"condition sees preceding CLEAR");

	Fixture call;
	call.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {75, 75, 0}),
		record(1, 1, 1, 0x09, {20, 25, 3}),
		record(1, 1, 2, 0x07, {2, 2, 2}),
		record(1, 1, 3, 0x07, {3, 3, 3}),
		record(75, 75, 0, 0x0c, setFlag(25)),
		record(75, 75, 1, 0x1a)
	}));
	const auto callResult = success(call.execute(camera, party, {}));
	check(callResult.finalCamera.mapId == 3 &&
		callResult.finalGameFlags.isSet(25) && callResult.instructionCount == 5,
		"flag mutation survives CallEvent and Return");

	Fixture continuation;
	continuation.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(25)),
		record(1, 1, 1, 0x1f, {2, 3, 4})
	}));
	continuation.scripts.emplace(2, script(2, {
		record(3, 4, 0, 0x09, {20, 25, 2}),
		record(3, 4, 1, 0x07, {3, 3, 3}),
		record(3, 4, 2, 0x07, {4, 4, 4})
	}));
	const auto continuationResult = success(
		continuation.execute(camera, party, {}));
	check(continuationResult.finalCamera.mapId == 4 &&
		continuationResult.finalGameFlags.isSet(25) &&
		continuationResult.instructionCount == 4,
		"flag mutation survives TeleportAndContinue");
}

void testTakeOrGiveRollback() {
	const XeenCamera camera{1, 1, 1, XeenDirection::South};
	const auto party = partyWithSp(2);
	XeenGameFlags clearInput;

	Fixture unsupportedOpcode;
	unsupportedOpcode.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(25)), record(1, 1, 1, 0x06)
	}));
	failure(unsupportedOpcode.execute(camera, party, clearInput),
		XeenEventExecutionErrorKind::UnsupportedOpcode);
	check(!clearInput.isSet(25), "unsupported opcode rolls back SET");

	Fixture unsupportedMode;
	unsupportedMode.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, clearFlag(25)), record(1, 1, 1, 0x0c)
	}));
	XeenGameFlags setInput;
	setInput.set(25);
	failure(unsupportedMode.execute(camera, party, setInput),
		XeenEventExecutionErrorKind::UnsupportedOperationMode);
	check(setInput.isSet(25), "unsupported operation rolls back CLEAR");

	Fixture loadFailure;
	loadFailure.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(25)),
		record(1, 1, 1, 0x1f, {2, 3, 4})
	}));
	loadFailure.throwingScripts[2] = true;
	failure(loadFailure.execute(camera, party, clearInput),
		XeenEventExecutionErrorKind::ScriptLoadFailed);
	check(!clearInput.isSet(25), "script load failure rolls back SET");

	Fixture limit;
	limit.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x0c, setFlag(25)),
		record(1, 1, 1, 0x09, {20, 25, 1})
	}));
	const auto limitError = failure(limit.execute(camera, party, clearInput),
		XeenEventExecutionErrorKind::InstructionLimitExceeded);
	check(limitError.instructionCount == XeenEventInterpreter::kMaximumInstructions &&
		!clearInput.isSet(25) && party.party.member(party.roster, 0).currentSp == 2,
		"instruction limit rolls back flags and preserves Party");
	checkCamera(camera, 1, 1, 1, XeenDirection::South,
		"TakeOrGive errors preserve input camera");
}

void testLimitsLoadingAndInputPreservation() {
	const XeenCamera camera{1, 1, 1, XeenDirection::North};
	const auto party = partyWithSp(2);
	XeenGameFlags flags;
	flags.set(25);
	const auto originalFlags = flags.values();

	Fixture loop;
	loop.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x08, {9, 0, 0})
	}));
	const auto limit = failure(loop.execute(camera, party, flags),
		XeenEventExecutionErrorKind::InstructionLimitExceeded);
	check(limit.instructionCount == 1024, "1024 dispatched instructions allowed");
	checkCamera(camera, 1, 1, 1, XeenDirection::North,
		"instruction limit preserves input camera");
	check(flags.values() == originalFlags && party.party.size() == 1,
		"success and errors do not mutate flags or Party");

	Fixture badInitial;
	failure(badInitial.execute({0, -1, 16, static_cast<XeenDirection>(9)},
		party, flags), XeenEventExecutionErrorKind::InvalidInitialCamera);

	Fixture scriptFailure;
	scriptFailure.throwingScripts[1] = true;
	failure(scriptFailure.execute(camera, party, flags),
		XeenEventExecutionErrorKind::ScriptLoadFailed);

	Fixture failureAfterWorkingTeleport;
	failureAfterWorkingTeleport.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x1f, {2, 3, 4})
	}));
	failureAfterWorkingTeleport.throwingScripts[2] = true;
	failure(failureAfterWorkingTeleport.execute(camera, party, flags),
		XeenEventExecutionErrorKind::ScriptLoadFailed);
	checkCamera(camera, 1, 1, 1, XeenDirection::North,
		"error after working teleport preserves input camera");

	Fixture mismatch;
	mismatch.scripts.emplace(1, script(2, {}));
	failure(mismatch.execute(camera, party, flags),
		XeenEventExecutionErrorKind::ScriptMapMismatch);

	// A stateful provider makes the 1024th dispatched instruction Exit, proving
	// that exactly the limit can still complete successfully.
	XeenWorld world([](std::uint16_t id) { return map(id); });
	std::size_t loads = 0;
	const auto provider = [&loads](std::uint16_t id) {
		++loads;
		if (loads == XeenEventInterpreter::kMaximumInstructions)
			return script(id, {record(1, 1, 0, 0x12)});
		return script(id, {record(1, 1, 0, 0x1f,
			{static_cast<std::uint8_t>(id), 1, 1})});
	};
	const auto exact = success(XeenEventInterpreter().execute(camera, party,
		flags, world, provider));
	check(exact.instructionCount == 1024, "execution may complete on instruction 1024");

	XeenWorld loopWorld([](std::uint16_t id) { return map(id); });
	const auto endlessProvider = [](std::uint16_t id) {
		return script(id, {record(1, 1, 0, 0x1f,
			{static_cast<std::uint8_t>(id), 1, 1})});
	};
	const auto continuationLimit = failure(XeenEventInterpreter().execute(
		camera, party, flags, loopWorld, endlessProvider),
		XeenEventExecutionErrorKind::InstructionLimitExceeded);
	check(continuationLimit.instructionCount == 1024,
		"TeleportAndContinue loop respects global instruction limit");
}

XeenEventExecutionError makeOwnedDiagnostic() {
	Fixture fixture;
	fixture.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x06, {}, kXeenEventDirectionAll, 987)
	}));
	return failure(fixture.execute({1, 1, 1, XeenDirection::North},
		partyWithSp(2), XeenGameFlags{}),
		XeenEventExecutionErrorKind::UnsupportedOpcode);
}

void testOwnedDiagnosticLifetime() {
	const auto diagnostic = makeOwnedDiagnostic();
	check(diagnostic.source && diagnostic.source->fileOffset == 987 &&
		diagnostic.source->resourceName == "maze1.evt" &&
		diagnostic.logicalAddress.mapId == 1,
		"diagnostic owns source values after scripts are destroyed");
}

} // namespace

int main() {
	try {
		testNaturalFlowAndFirstMatch();
		testConditions();
		testCallsReturnsAndStack();
		testTeleportsAndTransactionalResult();
		testTakeOrGiveFlagOperations();
		testTakeOrGiveVisibilityAcrossFlow();
		testTakeOrGiveRollback();
		testLimitsLoadingAndInputPreservation();
		testOwnedDiagnosticLifetime();
		std::cout << "Headless transactional Xeen event interpreter OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
