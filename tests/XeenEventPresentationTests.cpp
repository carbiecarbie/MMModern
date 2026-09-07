#include "games/xeen/XeenEventDecoder.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenWorld.h"

#include <array>
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
		std::size_t offset = 1) {
	XeenEventRecord result;
	result.fileOffset = offset;
	result.lengthField = static_cast<std::uint8_t>(5 + parameters.size());
	result.x = x;
	result.y = y;
	result.direction = kXeenEventDirectionAll;
	result.line = line;
	result.opcode = opcode;
	result.parameters = std::move(parameters);
	return result;
}

XeenEventScript script(mmodern::XeenMapIdentity mapId, std::vector<XeenEventRecord> records) {
	XeenEventFile file;
	file.mapId = mapId;
	file.resourceName = "maze" + std::to_string(mapId.number) + ".evt";
	file.resourcePresent = true;
	file.records = std::move(records);
	return XeenEventScript(std::move(file));
}

XeenMap map(mmodern::XeenMapIdentity id) {
	XeenMap result;
	result.geometry.id = id.number;
	result.side = id.side;
	result.geometry.flags2 = 0x8000;
	return result;
}

XeenEventTextFile text(mmodern::XeenMapIdentity mapId, std::vector<std::string> strings,
		bool present = true) {
	return {mapId, XeenEventTextLoader::resourceNameForMap(mapId), present,
		std::move(strings)};
}

class Fixture {
public:
	Fixture() : world([this](mmodern::XeenMapIdentity id) { return maps.at(id); }) {}

	XeenEventInterpreter::ScriptProvider scriptsProvider() {
		return [this](mmodern::XeenMapIdentity id) { return scripts.at(id); };
	}

	XeenEventInterpreter::TextProvider textsProvider() {
		return [this](mmodern::XeenMapIdentity id) {
			const auto found = texts.find(id);
			return found == texts.end() ? text(id, {}, false) : found->second;
		};
	}

	XeenEventExecutionStepResult begin(XeenGameFlags flags = {}) {
		return interpreter.begin({1, 1, 1, XeenDirection::North}, {}, flags,
			world, scriptsProvider(), textsProvider());
	}

	XeenEventExecutionStepResult resume(const XeenEventExecutionSuspended &pending,
			XeenPresentationResponse response) {
		return interpreter.resume(pending.state, response, {}, world,
			scriptsProvider(), textsProvider());
	}

	std::map<mmodern::XeenMapIdentity, XeenMap> maps;
	std::map<mmodern::XeenMapIdentity, XeenEventScript> scripts;
	std::map<mmodern::XeenMapIdentity, XeenEventTextFile> texts;
	XeenWorld world;
	XeenEventInterpreter interpreter;
};

XeenEventExecutionSuspended suspended(const XeenEventExecutionStepResult &result) {
	const auto *value = std::get_if<XeenEventExecutionSuspended>(&result);
	check(value != nullptr, "expected suspended execution");
	return *value;
}

XeenEventExecutionCompleted completed(const XeenEventExecutionStepResult &result) {
	const auto *value = std::get_if<XeenEventExecutionCompleted>(&result);
	check(value != nullptr, "expected completed execution");
	return *value;
}

XeenEventExecutionError failed(const XeenEventExecutionStepResult &result,
		XeenEventExecutionErrorKind kind) {
	const auto *value = std::get_if<XeenEventExecutionError>(&result);
	check(value && value->kind == kind, "unexpected execution error");
	return *value;
}

void testDisplayDecoding() {
	struct Case { std::uint8_t opcode; XeenEventDisplayKind kind; };
	const std::array<Case, 7> cases{{
		{0x01, XeenEventDisplayKind::Centered},
		{0x02, XeenEventDisplayKind::DoorLabelReduced},
		{0x03, XeenEventDisplayKind::DoorLabelNormal},
		{0x04, XeenEventDisplayKind::SignLabel},
		{0x29, XeenEventDisplayKind::BottomWindow},
		{0x31, XeenEventDisplayKind::BottomWindowTwoLines},
		{0x35, XeenEventDisplayKind::MainWindow}
	}};
	for (const Case &item : cases) {
		const auto parameters = item.opcode == 0x31 ?
			std::vector<std::uint8_t>{0x7e, 19} : std::vector<std::uint8_t>{19};
		const auto decoded = XeenEventDecoder::decode(record(1, 1, 0,
			item.opcode, parameters));
		const auto *instruction = std::get_if<XeenDecodedEventInstruction>(&decoded);
		check(instruction != nullptr, "display opcode should decode");
		const auto *display = std::get_if<XeenEventDisplay>(&instruction->operation);
		check(display && display->kind == item.kind && display->textIndex == 19,
			"display semantic and text index");
		check((item.opcode == 0x31) == display->layoutValue.has_value(),
			"only two-line display retains leading layout byte");
		if (display->layoutValue)
			check(*display->layoutValue == 0x7e, "two-line leading byte preserved");

		for (const auto &bad : item.opcode == 0x31 ?
				std::vector<std::vector<std::uint8_t>>{{19}, {1, 19, 2}} :
				std::vector<std::vector<std::uint8_t>>{{}, {19, 2}}) {
			const auto malformed = XeenEventDecoder::decode(
				record(1, 1, 0, item.opcode, bad));
			const auto *error = std::get_if<XeenEventDecodeError>(&malformed);
			check(error && error->kind == XeenEventDecodeErrorKind::MalformedInstruction,
				"display opcode strict parameter size");
		}
	}
}

void testPresentationKindsAndProgress() {
	Fixture fixture;
	fixture.maps.emplace(1, map(1));
	const std::string rawCenter("cen\x03ter", 7);
	fixture.texts.emplace(1, text(1, {rawCenter, "", "large", "sign", "bottom", "main"}));
	fixture.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x01, {0}), record(1, 1, 1, 0x02, {1}),
		record(1, 1, 2, 0x03, {2}), record(1, 1, 3, 0x04, {3}),
		record(1, 1, 4, 0x29, {4}), record(1, 1, 5, 0x35, {5}),
		record(1, 1, 6, 0x12)
	}));
	const std::array<XeenPresentationKind, 6> kinds{{
		XeenPresentationKind::CenteredMessage,
		XeenPresentationKind::SceneLabelReduced,
		XeenPresentationKind::SceneLabelNormal,
		XeenPresentationKind::SceneLabelSign,
		XeenPresentationKind::BottomWindowMessage,
		XeenPresentationKind::MainWindowMessage
	}};
	XeenEventExecutionStepResult step = fixture.begin();
	for (std::size_t index = 0; index < kinds.size(); ++index) {
		const auto pending = suspended(step);
		check(pending.request.kind == kinds[index] &&
			pending.request.response == XeenPresentationResponseRequirement::Presented &&
			pending.request.textIndex == index && pending.state.instructionCount == index + 1,
			"distinct presentation kind and cumulative progress");
		if (index == 1)
			check(pending.request.text.empty(), "valid empty text remains valid");
		if (index == 0)
			check(pending.request.text == rawCenter,
				"presentation preserves raw formatting-control bytes");
		step = fixture.resume(pending, XeenPresentationResponse::Presented);
	}
	check(completed(step).instructionCount == 7,
		"continuing displays execute once and reach following instruction");
}

void testTextFailuresAndMapContext() {
	Fixture missing;
	missing.maps.emplace(1, map(1));
	missing.scripts.emplace(1, script(1, {record(1, 1, 0, 0x01, {0})}));
	failed(missing.begin(), XeenEventExecutionErrorKind::MissingTextResource);

	Fixture invalid;
	invalid.maps.emplace(1, map(1));
	invalid.texts.emplace(1, text(1, {"only"}));
	invalid.scripts.emplace(1, script(1, {record(1, 1, 0, 0x04, {1})}));
	failed(invalid.begin(), XeenEventExecutionErrorKind::InvalidTextIndex);

	Fixture crossMap;
	crossMap.maps.emplace(1, map(1));
	crossMap.maps.emplace(2, map(2));
	crossMap.texts.emplace(1, text(1, {"wrong"}));
	crossMap.texts.emplace(2, text(2, {"map two"}));
	crossMap.scripts.emplace(1, script(1, {record(1, 1, 0, 0x1f, {2, 2, 2})}));
	crossMap.scripts.emplace(2, script(2, {record(2, 2, 0, 0x35, {0})}));
	const auto pending = suspended(crossMap.begin());
	check(pending.request.mapId == 2 && pending.request.text == "map two",
		"text resolves from current executing map after transfer");
}

void testTwoLineTermination() {
	Fixture fixture;
	fixture.maps.emplace(1, map(1));
	fixture.texts.emplace(1, text(1, {"unused", "two lines"}));
	fixture.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x31, {77, 1}),
		record(1, 1, 1, 0x0c, {0, 0, 20, 9})
	}));
	const auto pending = suspended(fixture.begin());
	check(pending.request.kind == XeenPresentationKind::BottomWindowTwoLines &&
		pending.request.response == XeenPresentationResponseRequirement::Acknowledgment &&
		pending.request.layoutValue == 77 && pending.request.text == "two lines",
		"two-line request semantics");
	failed(fixture.resume(pending, XeenPresentationResponse::Presented),
		XeenEventExecutionErrorKind::InvalidPresentationResponse);
	const auto result = completed(fixture.resume(pending,
		XeenPresentationResponse::Acknowledged));
	check(result.instructionCount == 1 && !result.finalGameFlags.isSet(9),
		"two-line acknowledgment terminates without following line");
}

void testLimitsCallsAndRollback() {
	Fixture limit;
	limit.maps.emplace(1, map(1));
	limit.texts.emplace(1, text(1, {"yield"}));
	limit.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x01, {0}), record(1, 1, 1, 0x09, {20, 0, 1})
	}));
	XeenGameFlags set;
	set.set(0);
	const auto limitPending = suspended(limit.begin(set));
	const auto limitError = failed(limit.resume(limitPending,
		XeenPresentationResponse::Presented),
		XeenEventExecutionErrorKind::InstructionLimitExceeded);
	check(limitError.instructionCount == XeenEventInterpreter::kMaximumInstructions,
		"instruction limit remains cumulative after suspension");

	Fixture calls;
	calls.maps.emplace(1, map(1));
	calls.texts.emplace(1, text(1, {"inside call"}));
	calls.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x19, {2, 2, 0}), record(1, 1, 1, 0x12),
		record(2, 2, 0, 0x01, {0}), record(2, 2, 1, 0x1a)
	}));
	const auto callPending = suspended(calls.begin());
	check(callPending.state.callStack.size() == 1 &&
		callPending.state.instructionCount == 2, "call stack retained at yield");
	check(completed(calls.resume(callPending,
		XeenPresentationResponse::Presented)).instructionCount == 4,
		"CallEvent and Return survive suspension");

	std::map<mmodern::XeenMapIdentity, XeenMap> maps{{1, map(1)}};
	std::map<mmodern::XeenMapIdentity, XeenEventScript> scripts{{1, script(1, {
		record(1, 1, 0, 0x0c, {0, 0, 20, 9}),
		record(1, 1, 1, 0x01, {0}), record(1, 1, 2, 0x06)
	})}};
	XeenWorld world([&](mmodern::XeenMapIdentity id) { return maps.at(id); });
	XeenEventSystem system([&](mmodern::XeenMapIdentity id) { return scripts.at(id); },
		[](mmodern::XeenMapIdentity id) { return text(id, {"shown"}); });
	XeenCamera camera{1, 1, 1, XeenDirection::North};
	XeenGameFlags flags;
	const XeenManualEventResult started = system.runManualEvent(world, {}, camera, flags);
	const auto *pending = std::get_if<XeenEventExecutionSuspended>(&started);
	check(pending && !flags.isSet(9), "working flag is not committed while pending");
	const auto resumed = system.resumeManualEvent(pending->state,
		XeenPresentationResponse::Presented, world, {}, camera, flags);
	check(std::holds_alternative<XeenEventExecutionError>(resumed) &&
		!flags.isSet(9), "failure after presentation rolls back working flags");
}

void testAction44() {
	auto prepareChoice = [](Fixture &fixture) {
		fixture.maps.emplace(1, map(1));
		fixture.maps.emplace(2, map(2));
		fixture.scripts.emplace(1, script(1, {
			record(1, 1, 0, 0x09, {44, 0, 2}), record(1, 1, 1, 0x12),
			record(1, 1, 2, 0x07, {2, 3, 4})
		}));
	};

	Fixture no;
	prepareChoice(no);
	const auto noRequest = suspended(no.begin());
	check(noRequest.request.kind == XeenPresentationKind::Confirmation &&
		noRequest.request.response == XeenPresentationResponseRequirement::YesNo,
		"Action 44 value zero requests Yes/No");
	check(completed(no.resume(noRequest, XeenPresentationResponse::No))
		.finalCamera.mapId == 1, "No maps to original value 2 and avoids teleport");

	Fixture yes;
	prepareChoice(yes);
	check(completed(yes.resume(suspended(yes.begin()), XeenPresentationResponse::Yes))
		.finalCamera.mapId == 2, "Yes maps to original value zero and takes branch");

	Fixture acknowledgment;
	acknowledgment.maps.emplace(1, map(1));
	acknowledgment.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x09, {44, 1, 2}), record(1, 1, 1, 0x12),
		record(1, 1, 2, 0x0c, {0, 0, 20, 5}), record(1, 1, 3, 0x12)
	}));
	const auto ack = suspended(acknowledgment.begin());
	check(ack.request.response == XeenPresentationResponseRequirement::Acknowledgment,
		"Action 44 value one requests acknowledgment");
	const auto acknowledged = completed(acknowledgment.resume(ack,
		XeenPresentationResponse::Acknowledged));
	check(acknowledged.instructionCount == 3 && acknowledged.finalGameFlags.isSet(5),
		"acknowledgment supplies original comparison value one");

	Fixture unsupported;
	unsupported.maps.emplace(1, map(1));
	unsupported.scripts.emplace(1, script(1, {
		record(1, 1, 0, 0x09, {44, 2, 1})
	}));
	failed(unsupported.begin(), XeenEventExecutionErrorKind::UnsupportedConditionAction);
}

} // namespace

int main() {
	try {
		testDisplayDecoding();
		testPresentationKindsAndProgress();
		testTextFailuresAndMapContext();
		testTwoLineTermination();
		testLimitsCallsAndRollback();
		testAction44();
		std::cout << "Resumable event presentation semantics OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
