#include "games/xeen/XeenEventDecoder.h"

#include <cstdint>
#include <iostream>
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

XeenEventRecord record(std::uint8_t opcode,
		std::vector<std::uint8_t> parameters = {}) {
	XeenEventRecord result;
	result.fileOffset = 1234;
	result.lengthField = static_cast<std::uint8_t>(5 + parameters.size());
	result.x = 201;
	result.y = 202;
	result.direction = 4;
	result.line = 77;
	result.opcode = opcode;
	result.parameters = std::move(parameters);
	return result;
}

XeenDecodedEventInstruction success(const XeenEventDecodeResult &result) {
	const auto *decoded = std::get_if<XeenDecodedEventInstruction>(&result);
	check(decoded != nullptr, "expected successful decode");
	return *decoded;
}

XeenEventDecodeError failure(const XeenEventDecodeResult &result,
		XeenEventDecodeErrorKind kind) {
	const auto *error = std::get_if<XeenEventDecodeError>(&result);
	check(error != nullptr && error->kind == kind, "unexpected decode error kind");
	return *error;
}

template<class Operation>
Operation operation(const XeenEventDecodeResult &result) {
	const auto decoded = success(result);
	const auto *value = std::get_if<Operation>(&decoded.operation);
	check(value != nullptr, "unexpected decoded operation");
	return *value;
}

void testEmptyOperationsAndStrictSizes() {
	check(std::holds_alternative<XeenEventNone>(
		success(XeenEventDecoder::decode(record(0x00))).operation), "None");
	check(std::holds_alternative<XeenEventExit>(
		success(XeenEventDecoder::decode(record(0x12))).operation), "Exit");
	check(std::holds_alternative<XeenEventReturn>(
		success(XeenEventDecoder::decode(record(0x1a))).operation), "Return");
	for (const std::uint8_t opcode : {0x00, 0x12, 0x1a}) {
		const auto &error = failure(XeenEventDecoder::decode(record(opcode, {1})),
			XeenEventDecodeErrorKind::MalformedInstruction);
		check(error.expectedParameterSize == 0 && error.actualParameterSize == 1,
			"empty opcode strict size");
	}
}

void testTeleports() {
	const auto &exit = operation<XeenEventTeleportAndExit>(
		XeenEventDecoder::decode(record(0x07, {31, 4, 9})));
	check(exit.mapId == 31 && exit.x == 4 && exit.y == 9, "direct exit teleport");
	const auto &continuation = operation<XeenEventTeleportAndContinue>(
		XeenEventDecoder::decode(record(0x1f, {42, 0x80, 0xff})));
	check(continuation.mapId == 42 && continuation.x == -128 && continuation.y == -1,
		"direct continuing teleport and signed coordinates");

	for (const auto &payload : std::vector<std::vector<std::uint8_t>>{
		{1, 0x00, 0x7f}, {1, 0x80, 0xff}}) {
		const auto &decoded = operation<XeenEventTeleportAndExit>(
			XeenEventDecoder::decode(record(0x07, payload)));
		check(decoded.x == (payload[1] == 0x80 ? -128 : 0) &&
			decoded.y == (payload[2] == 0xff ? -1 : 127), "signed teleport boundaries");
	}

	for (const auto &payload : std::vector<std::vector<std::uint8_t>>{{}, {1}, {1, 2}, {1, 2, 3, 4}})
		failure(XeenEventDecoder::decode(record(0x07, payload)),
			XeenEventDecodeErrorKind::MalformedInstruction);

	const auto &mirrorShort = failure(XeenEventDecoder::decode(record(0x07, {0})),
		XeenEventDecodeErrorKind::UnsupportedOperand);
	const auto &mirrorLong = failure(XeenEventDecoder::decode(record(0x1f, {0, 1, 2, 3})),
		XeenEventDecodeErrorKind::UnsupportedOperand);
	check(!mirrorShort.expectedParameterSize && mirrorShort.actualParameterSize == 1 &&
		!mirrorLong.expectedParameterSize && mirrorLong.actualParameterSize == 4,
		"mirror is identified before direct-form size validation");
}

void testCallEvent() {
	const auto &logical = operation<XeenEventCallEvent>(
		XeenEventDecoder::decode(record(0x19, {75, 102, 255})));
	check(logical.x == 75 && logical.y == 102 && logical.line == 255,
		"CallEvent logical target and line");
	const auto &boundaries = operation<XeenEventCallEvent>(
		XeenEventDecoder::decode(record(0x19, {0x7f, 0x80, 0xff})));
	check(boundaries.x == 127 && boundaries.y == -128 && boundaries.line == 255,
		"CallEvent signed boundary values");
	const auto &negative = operation<XeenEventCallEvent>(
		XeenEventDecoder::decode(record(0x19, {0xff, 0x00, 1})));
	check(negative.x == -1 && negative.y == 0, "CallEvent -1 and zero");
	failure(XeenEventDecoder::decode(record(0x19, {1, 2})),
		XeenEventDecodeErrorKind::MalformedInstruction);
	failure(XeenEventDecoder::decode(record(0x19, {1, 2, 3, 4})),
		XeenEventDecodeErrorKind::MalformedInstruction);
}

void checkConditional(std::uint8_t opcode, XeenEventComparison comparison,
		std::vector<std::uint8_t> parameters, std::uint8_t action,
		std::uint32_t value, std::uint8_t targetLine) {
	const auto &conditional = operation<XeenEventConditional>(
		XeenEventDecoder::decode(record(opcode, std::move(parameters))));
	check(conditional.comparison == comparison && conditional.action == action &&
		conditional.value == value && conditional.targetLine == targetLine,
		"conditional decode");
}

void testConditionals() {
	checkConditional(0x08, XeenEventComparison::GreaterOrEqual, {9, 0xab, 3}, 9, 0xab, 3);
	checkConditional(0x09, XeenEventComparison::Equal, {20, 25, 2}, 20, 25, 2);
	checkConditional(0x0a, XeenEventComparison::LessOrEqual, {200, 0x7f, 255}, 200, 0x7f, 255);

	for (const std::uint8_t action : {25, 35, 101, 106})
		checkConditional(0x09, XeenEventComparison::Equal,
			{action, 0x34, 0x12, 7}, action, 0x1234, 7);
	checkConditional(0x09, XeenEventComparison::Equal,
		{25, 0xff, 0xff, 8}, 25, 0xffff, 8);

	for (const std::uint8_t action : {16, 34, 100})
		checkConditional(0x08, XeenEventComparison::GreaterOrEqual,
			{action, 0x78, 0x56, 0x34, 0x12, 9}, action, 0x12345678, 9);
	checkConditional(0x0a, XeenEventComparison::LessOrEqual,
		{16, 0x00, 0x00, 0x00, 0x80, 10}, 16, 0x80000000u, 10);
	checkConditional(0x0a, XeenEventComparison::LessOrEqual,
		{100, 0xff, 0xff, 0xff, 0xff, 255}, 100, 0xffffffffu, 255);

	for (const std::uint8_t opcode : {0x08, 0x09, 0x0a}) {
		failure(XeenEventDecoder::decode(record(opcode, {})),
			XeenEventDecodeErrorKind::MalformedInstruction);
		failure(XeenEventDecoder::decode(record(opcode, {9, 1})),
			XeenEventDecodeErrorKind::MalformedInstruction);
		failure(XeenEventDecoder::decode(record(opcode, {9, 1, 2, 3})),
			XeenEventDecodeErrorKind::MalformedInstruction);
		failure(XeenEventDecoder::decode(record(opcode, {25, 1, 2})),
			XeenEventDecodeErrorKind::MalformedInstruction);
		failure(XeenEventDecoder::decode(record(opcode, {25, 1, 2, 3, 4})),
			XeenEventDecodeErrorKind::MalformedInstruction);
		failure(XeenEventDecoder::decode(record(opcode, {16, 1, 2, 3, 4})),
			XeenEventDecodeErrorKind::MalformedInstruction);
		failure(XeenEventDecoder::decode(record(opcode, {16, 1, 2, 3, 4, 5, 6})),
			XeenEventDecodeErrorKind::MalformedInstruction);
	}
}

void checkPair(const XeenEventTakeOrGivePair &pair, std::uint8_t mode,
		std::uint32_t value, const char *message) {
	check(pair.mode == mode && pair.value == value, message);
}

void testTakeOrGive() {
	const auto empty = operation<XeenEventTakeOrGive>(
		XeenEventDecoder::decode(record(0x0c)));
	checkPair(empty.first, 0, 0, "empty TakeOrGive first pair");
	checkPair(empty.second, 0, 0, "empty TakeOrGive second pair");
	checkPair(empty.third, 0, 0, "empty TakeOrGive third pair");

	const auto clear = operation<XeenEventTakeOrGive>(
		XeenEventDecoder::decode(record(0x0c, {20, 25})));
	checkPair(clear.first, 20, 25, "clear flag pair");
	checkPair(clear.second, 0, 0, "omitted second pair");
	checkPair(clear.third, 0, 0, "omitted third pair");

	const auto setOmitted = operation<XeenEventTakeOrGive>(
		XeenEventDecoder::decode(record(0x0c, {0, 0, 20, 255})));
	checkPair(setOmitted.first, 0, 0, "set neutral first pair");
	checkPair(setOmitted.second, 20, 255, "set flag 255 pair");
	checkPair(setOmitted.third, 0, 0, "set omitted third pair");

	const auto setComplete = operation<XeenEventTakeOrGive>(
		XeenEventDecoder::decode(record(0x0c, {0, 0, 20, 0, 0, 0})));
	checkPair(setComplete.first, 0, 0, "complete set first pair");
	checkPair(setComplete.second, 20, 0, "complete set flag zero");
	checkPair(setComplete.third, 0, 0, "explicit zero third pair");

	const auto thirdNonZero = operation<XeenEventTakeOrGive>(
		XeenEventDecoder::decode(record(0x0c, {0, 0, 20, 25, 9, 7})));
	checkPair(thirdNonZero.third, 9, 7, "nonzero third pair is preserved");

	for (const std::uint8_t mode : {std::uint8_t{25}, std::uint8_t{35},
			std::uint8_t{101}, std::uint8_t{106}}) {
		const auto decoded = operation<XeenEventTakeOrGive>(
			XeenEventDecoder::decode(record(0x0c, {mode, 0x34, 0x12})));
		checkPair(decoded.first, mode, 0x1234, "TakeOrGive uint16 LE");
	}
	for (const std::uint8_t mode : {std::uint8_t{16}, std::uint8_t{34},
			std::uint8_t{100}}) {
		const auto decoded = operation<XeenEventTakeOrGive>(
			XeenEventDecoder::decode(record(0x0c,
				{mode, 0x78, 0x56, 0x34, 0x12})));
		checkPair(decoded.first, mode, 0x12345678, "TakeOrGive uint32 LE");
	}
	checkPair(operation<XeenEventTakeOrGive>(XeenEventDecoder::decode(
		record(0x0c, {200, 0xab}))).first, 200, 0xab,
		"unknown TakeOrGive mode remains uint8");

	for (const auto &payload : std::vector<std::vector<std::uint8_t>>{
			{20}, {0}, {0, 0, 20}, {25, 1}, {16, 1, 2, 3},
			{0, 0, 25, 1}, {0, 0, 0, 0, 16, 1, 2, 3},
			{0, 0, 0, 0, 0}}) {
		const auto malformed = failure(XeenEventDecoder::decode(
			record(0x0c, payload)), XeenEventDecodeErrorKind::MalformedInstruction);
		check(!malformed.expectedParameterSize &&
			malformed.actualParameterSize == payload.size(),
			"partial TakeOrGive pair diagnostics");
	}
	const auto extra = failure(XeenEventDecoder::decode(
		record(0x0c, {0, 0, 0, 0, 0, 0, 0})),
		XeenEventDecodeErrorKind::MalformedInstruction);
	check(!extra.expectedParameterSize && extra.actualParameterSize == 7,
		"TakeOrGive rejects bytes after third pair");
}

void checkSource(const XeenEventSourceLocation &source, bool withContext,
		std::uint8_t opcode) {
	check(source.fileOffset == 1234 && source.x == 201 && source.y == 202 &&
		source.direction == 4 && source.line == 77 && source.opcode == opcode,
		"raw source metadata");
	if (withContext) {
		check(source.mapId == 42 && source.resourceName == "maze0042.evt",
			"owned source context");
	} else {
		check(!source.mapId && !source.resourceName, "explicitly absent source context");
	}
}

void testErrorsMetadataAndPurity() {
	const auto unsupportedKnown = XeenEventDecoder::decode(record(0x06, {1, 2}),
		{42, std::string("maze0042.evt")});
	const auto &opcodeError = failure(unsupportedKnown,
		XeenEventDecodeErrorKind::UnsupportedOpcode);
	checkSource(opcodeError.source, true, 0x06);
	check(!opcodeError.expectedParameterSize && opcodeError.actualParameterSize == 2,
		"unsupported opcode does not parse parameters");
	checkSource(failure(XeenEventDecoder::decode(record(0xff)),
		XeenEventDecodeErrorKind::UnsupportedOpcode).source, false, 0xff);
	checkSource(failure(XeenEventDecoder::decode(record(0x12, {1})),
		XeenEventDecodeErrorKind::MalformedInstruction).source, false, 0x12);
	checkSource(failure(XeenEventDecoder::decode(record(0x07, {0})),
		XeenEventDecodeErrorKind::UnsupportedOperand).source, false, 0x07);
	checkSource(success(XeenEventDecoder::decode(record(0x12),
		{42, std::string("maze0042.evt")})).source, true, 0x12);

	auto input = record(0x19, {0xff, 2, 3});
	const auto originalParameters = input.parameters;
	const auto first = XeenEventDecoder::decode(input);
	const auto second = XeenEventDecoder::decode(input);
	check(input.parameters == originalParameters && input.fileOffset == 1234 &&
		operation<XeenEventCallEvent>(first).x == operation<XeenEventCallEvent>(second).x,
		"decoder is deterministic and does not mutate input");
}

XeenEventDecodeResult decodeTemporaryRecord() {
	auto temporary = record(0x19, {0xff, 0x80, 255});
	return XeenEventDecoder::decode(temporary,
		{31, std::string("maze0031.evt")});
}

void testOwnedResultLifetime() {
	const auto result = decodeTemporaryRecord();
	const auto &decoded = operation<XeenEventCallEvent>(result);
	check(decoded.x == -1 && decoded.y == -128 && decoded.line == 255,
		"operation survives input destruction");
	const auto &source = success(result).source;
	check(source.mapId == 31 && source.resourceName == "maze0031.evt" &&
		source.fileOffset == 1234 && source.x == 201 && source.y == 202 &&
		source.direction == 4 && source.line == 77,
		"source context survives input destruction");
}

} // namespace

int main() {
	try {
		testEmptyOperationsAndStrictSizes();
		testTeleports();
		testCallEvent();
		testConditionals();
		testTakeOrGive();
		testErrorsMetadataAndPurity();
		testOwnedResultLifetime();
		std::cout << "Typed EVT decoder and strict validation OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
