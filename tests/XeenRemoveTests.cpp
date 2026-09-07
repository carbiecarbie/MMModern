#include "XeenRemoveTestSupport.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventSystem.h"
#include <iostream>

using namespace mmodern;
using namespace remove_test;

namespace {
using Bytes = std::vector<std::uint8_t>;
Bytes mob() {
	Bytes bytes(48, 0xff); bytes[0] = 111;
	Bytes objects{0x80,2,0,0, 8,2,1,0, 8,3,0,0, 8,2,0,1, 8,2,0,2};
	bytes.insert(bytes.end(), objects.begin(), objects.end());
	bytes.insert(bytes.end(), 4 * 4, 0xff); // object terminator, empty monster pair, wall terminator
	return bytes;
}
XeenObjectFile objects(XeenMapIdentity id) {
	auto file = XeenMapLoader().loadObjects([](const std::string &) { return mob(); }, id.number);
	file.mapId = id; // Synthetic side provider; the real adapter remains Clouds-only.
	return file;
}
struct Fixture {
	std::map<XeenMapIdentity, XeenEventScript> scripts;
	int objectLoads = 0;
	XeenWorld world{[](XeenMapIdentity id) { return map(id); },
		[this](XeenMapIdentity id) { ++objectLoads; return objects(id); }};
	XeenEventInterpreter interpreter;
	XeenEventInterpreter::ScriptProvider provider() {
		return [this](XeenMapIdentity id) { return scripts.at(id); };
	}
	XeenEventInterpreter::TextProvider textProvider() {
		return [](XeenMapIdentity id) { return XeenEventTextFile{id, "test.txt", true, {"text"}}; };
	}
	XeenEventExecutionStepResult begin(std::uint8_t line = 0, XeenMapIdentity id = 23) {
		return interpreter.begin({id,8,2,XeenDirection::North}, {}, {}, world,
			provider(), textProvider(), line);
	}
	XeenEventExecutionStepResult resume(XeenEventExecutionState state) {
		return interpreter.resume(std::move(state), XeenPresentationResponse::Presented,
			{}, world, provider(), textProvider());
	}
};

void loadingAndSelection() {
	XeenMapLoader loader;
	const auto missing = loader.loadObjects([](const std::string &name) -> std::optional<Bytes> {
		check(name == "maze0023.mob", "MOB resource naming"); return std::nullopt;
	}, 23);
	check(!missing.resourcePresent && missing.entities.objects.empty(), "missing MOB not distinct");
	const auto empty = loader.loadObjects([](const std::string &) { return Bytes(68, 0xff); }, 23);
	check(empty.resourcePresent && empty.entities.objects.empty(), "valid empty MOB not distinct");
	bool malformed = false, rejected = false;
	try { loader.loadObjects([](const std::string &) { return Bytes{}; }, 23); }
	catch (const std::runtime_error &e) { malformed = std::string(e.what()).find("maze0023.mob") != std::string::npos; }
	int reads = 0;
	try { loader.loadObjects([&](const std::string &) { ++reads; return mob(); }, {XeenSide::Darkside,23}); }
	catch (const std::invalid_argument &) { rejected = true; }
	check(malformed && rejected && reads == 0, "MOB failure or side handling");
	Fixture f;
	f.world.map(23); f.world.map(24);
	check(f.objectLoads == 0, "geometry eagerly loaded objects");
	const XeenCamera camera{23,8,2,XeenDirection::North};
	const auto base = f.world.objectFile(23);
	check(base.entities.objects.size() == 5 && base.entities.objects[0].x == -128 &&
		base.entities.objects[3].direction == 1 && base.entities.objects[4].direction == 2,
		"base order/disabled records not preserved");
	check(f.world.selectObject(camera) == XeenObjectIdentity{23,3}, "first eligible current object");
	f.world.disableObject({23,3});
	check(f.world.selectObject(camera) == XeenObjectIdentity{23,4}, "same sprite conflated identities");
	f.world.disableObject({23,4});
	check(!f.world.selectObject(camera), "selected ahead/base-disabled/invalid-resource object");
	check(f.world.isObjectDisabled({23,0}), "base disabled state ignored");
	const XeenMapIdentity dark{XeenSide::Darkside,23};
	check(f.world.selectObject({dark,8,2,XeenDirection::North}) == XeenObjectIdentity{dark,3},
		"object mutation crossed sides");
	f.world.discardMapCache();
	check(f.world.isObjectDisabled({23,3}) && f.objectLoads == 3, "reload erased identity/mutation");
	sameEntities(base.entities, f.world.objectFile(23).entities);
	XeenWorld wrong([](XeenMapIdentity id) { return map(id); },
		[](XeenMapIdentity) { return objects(24); });
	rejected = false;
	try { wrong.objectFile(23); } catch (const std::runtime_error &) { rejected = true; }
	check(rejected && wrong.cachedObjectFileCount() == 0, "wrong object map cached");
}

void basicRemoveAndEffectiveRecords() {
	Fixture f;
	f.scripts.emplace(23, script(23, {record(8,2,0,0xff,{1,2},4,10),
		record(8,2,0,0x12,{},0,19), record(8,2,1,0x01,{99},4,25),
		record(8,2,2,0x0e,{},4,32), record(8,2,99,0xff,{5},2,38),
		record(9,2,0,0xff,{8},4,45)}));
	const auto original = f.scripts.at(23);
	const auto geometry = geometrySnapshot(f.world.map(23).geometry);
	const auto result = std::get<XeenEventExecutionCompleted>(f.begin(2));
	check(result.instructionCount == 4, "Remove must restart line zero and count three effective None");
	check(f.world.isObjectDisabled({23,3}) && !f.world.isObjectDisabled({23,4}), "wrong object disabled");
	check(f.world.sessionState().disabledEventCount() == 5, "cell mutation filtered direction/line");
	const auto copied = f.scripts.at(23);
	check(copied.findInstructionIndex(8,2,XeenDirection::North,0) == 0, "disabled duplicate first match lost");
	for (std::size_t i=0;i<copied.records().size();++i) {
		check(sameRecord(original.records()[i],copied.records()[i]), "base event altered");
		const auto effective = f.world.effectiveEvent({23,i},copied.records()[i]);
		check(sameRecord(original.records()[i],effective,false) &&
			effective.opcode == (i<5 ? 0 : original.records()[i].opcode), "effective event metadata changed");
		check(!f.world.isEventDisabled({{XeenSide::Darkside,23},i}) &&
			!f.world.isEventDisabled({24,i}), "event mutation crossed identity");
	}
	check(geometry == geometrySnapshot(f.world.map(23).geometry), "geometry changed");
	check(std::get<XeenEventExecutionCompleted>(f.begin()).instructionCount == 3,
		"second interaction replayed original opcodes");
	// Missing objects are a valid no-selection context, not an execution error.
	XeenWorld noObjects([](XeenMapIdentity id) { return map(id); });
	check(std::holds_alternative<XeenEventExecutionCompleted>(f.interpreter.begin(
		{23,8,2,XeenDirection::North}, {}, {}, noObjects, f.provider(), {}, 2)) &&
		noObjects.sessionState().disabledObjectCount() == 0 &&
		noObjects.sessionState().disabledEventCount() == 5, "Remove without selection failed");
}

void suspendedSelectionAndInvalidContext() {
	for (int mode=0;mode<4;++mode) {
		Fixture f;
		f.scripts.emplace(23,script(23,{record(8,2,0,0x01,{0}),record(8,2,1,0x0e)}));
		auto pending=std::get<XeenEventExecutionSuspended>(f.begin());
		check(pending.state.selectedObject == XeenObjectIdentity{23,3}, "suspension selection missing");
		if(mode==0) f.world.disableObject({23,3}); // valid retained selection is now disabled
		if(mode==1) pending.state.selectedObject=XeenObjectIdentity{23,999};
		if(mode==2) pending.state.selectedObject=XeenObjectIdentity{24,3};
		if(mode==3) pending.state.selectedObject=XeenObjectIdentity{{XeenSide::Darkside,23},3};
		f.world.discardMapCache();
		const auto result=f.resume(pending.state);
		if(mode==0) {
			check(std::get<XeenEventExecutionCompleted>(result).instructionCount==4 &&
				!f.world.isObjectDisabled({23,4}), "resume reselected another object");
		} else {
			check(std::get<XeenEventExecutionError>(result).kind==XeenEventExecutionErrorKind::InvalidRemoveContext &&
				f.world.sessionState().disabledObjectCount()==0 &&
				f.world.sessionState().disabledEventCount()==0, "invalid context partially mutated world");
		}
	}
}

void callsTransfersAndLimits() {
	Fixture f;
	f.scripts.emplace(23,script(23,{record(8,2,0,0x19,{7,8,0}),record(8,2,1,0x12),
		record(7,8,0,0x01,{0}),record(7,8,1,0x0e)}));
	auto pending=std::get<XeenEventExecutionSuspended>(f.begin());
	check(pending.state.logicalAddress.x==7 && pending.state.workingCamera.x==8 &&
		pending.state.callStack.size()==1 && pending.state.selectedObject==XeenObjectIdentity{23,3},
		"CallEvent changed physical position or selection");
	pending=std::get<XeenEventExecutionSuspended>(f.resume(pending.state));
	check(pending.state.callStack.size()==1 && f.world.isEventDisabled({23,0}) &&
		f.world.isEventDisabled({23,1}) && !f.world.isEventDisabled({23,2}) &&
		!f.world.isEventDisabled({23,3}), "Remove mutated logical cell or cleared call stack");
	XeenEventExecutionStepResult result=pending;
	while(const auto *s=std::get_if<XeenEventExecutionSuspended>(&result)) result=f.resume(s->state);
	check(std::get<XeenEventExecutionError>(result).kind==XeenEventExecutionErrorKind::InstructionLimitExceeded &&
		std::get<XeenEventExecutionError>(result).instructionCount==1024, "cross-location Remove loop escaped limit");
	// Near the boundary, Remove and its effective None both consume budget.
	Fixture budget;
	budget.scripts.emplace(23,script(23,{record(8,2,0,0x01,{0}),record(8,2,1,0x0e)}));
	auto nearLimit=std::get<XeenEventExecutionSuspended>(budget.begin());
	nearLimit.state.instructionCount=1022;
	const auto limited=std::get<XeenEventExecutionError>(budget.resume(nearLimit.state));
	check(limited.kind==XeenEventExecutionErrorKind::InstructionLimitExceeded &&
		limited.instructionCount==1024 && budget.world.sessionState().disabledEventCount()==2,
		"effective None failed to consume the remaining instruction budget");
	Fixture transfer;
	transfer.scripts.emplace(23,script(23,{record(8,2,0,0x1f,{24,8,2})}));
	transfer.scripts.emplace(24,script(24,{record(8,2,0,0x01,{0}),record(8,2,1,0x0e)}));
	pending=std::get<XeenEventExecutionSuspended>(transfer.begin());
	check(pending.state.selectedObject==XeenObjectIdentity{24,3}, "transfer retained old selection");
	check(std::holds_alternative<XeenEventExecutionCompleted>(transfer.resume(pending.state)) &&
		transfer.world.isObjectDisabled({24,3}) && !transfer.world.isObjectDisabled({23,3}),
		"transfer removed source object");
	// A destination with no eligible object must clear the source selection.
	Fixture empty;
	empty.scripts=transfer.scripts;
	empty.world.disableObject({24,3}); empty.world.disableObject({24,4});
	pending=std::get<XeenEventExecutionSuspended>(empty.begin());
	check(!pending.state.selectedObject, "empty destination retained old selection");
}

void worldEffectsSurviveTransactionFailure() {
	Fixture f;
	// On the called cell, set a working flag, Remove, restart, then take the
	// flag branch to a supported Return with malformed operands.
	f.scripts.emplace(23,script(23,{record(8,2,0,0x1f,{24,8,2})}));
	f.scripts.emplace(24,script(24,{record(8,2,0,0x19,{7,8,0}),record(8,2,1,0x12),
		record(7,8,0,0x09,{20,5,3}), record(7,8,1,0x0c,{0,0,20,5}),
		record(7,8,2,0x0e),record(7,8,3,0x1a,{1})}));
	XeenEventSystem events(f.provider());
	XeenCamera camera{23,8,2,XeenDirection::North}; XeenGameFlags flags;
	const auto result=events.runManualEvent(f.world,{},camera,flags);
	check(std::get<XeenEventExecutionError>(result).kind==XeenEventExecutionErrorKind::MalformedInstruction &&
		camera.mapId==23 && !flags.isSet(5), "camera/flags failed rollback");
	check(f.world.isObjectDisabled({24,3}) && f.world.isEventDisabled({24,0}) &&
		f.world.isEventDisabled({24,1}) && !f.world.isObjectDisabled({23,3}),
		"world effects rolled back with camera/flags");
}
} // namespace

int main() {
	try {
		loadingAndSelection(); basicRemoveAndEffectiveRecords(); suspendedSelectionAndInvalidContext();
		callsTransfersAndLimits(); worldEffectsSurviveTransactionFailure();
		std::cout << "M15B object loading, selection, effective records, Remove and transactions OK\n";
		return 0;
	} catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
