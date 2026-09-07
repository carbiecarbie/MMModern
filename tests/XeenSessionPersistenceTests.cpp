#include "XeenRemoveTestSupport.h"
#include "games/xeen/XeenEventSystem.h"

#include <iostream>
#include <map>

using namespace mmodern;
using namespace remove_test;

namespace {

XeenObjectFile objects(XeenMapIdentity id) {
	XeenObjectFile file{id, "synthetic.mob", true, {}};
	file.entities.objectTable.fill(0xff);
	file.entities.objectTable[0] = 111;
	file.entities.objectTable[1] = 112;
	file.entities.objects.push_back({8, 2, 0, 0, 111});
	file.entities.objects.push_back({9, 9, 1, 0, 112});
	return file;
}

std::vector<XeenEventRecord> mutationRecords() {
	return {record(8, 2, 0, 0x0e, {}, 4, 10),
		record(8, 2, 1, 0x12, {}, 0, 16),
		record(4, 4, 0, 0x12, {}, 4, 22)};
}

std::vector<XeenEventRecord> presentationRecords() {
	return {record(9, 9, 0, 0x19, {7, 8, 0}, 4, 30),
		record(9, 9, 1, 0x12, {}, 4, 38),
		record(7, 8, 0, 0x01, {0}, 4, 44),
		record(7, 8, 1, 0x1a, {}, 4, 51)};
}

struct Lifecycle {
	XeenPartyState party;
	std::map<XeenMapIdentity, int> mapLoads, objectLoads, scriptLoads, textLoads;
	XeenWorld world;
	XeenEventSystem events;

	Lifecycle() :
		world([this](XeenMapIdentity id) {
			++mapLoads[id]; return map(id);
		}, [this](XeenMapIdentity id) {
			++objectLoads[id]; return objects(id);
		}),
		events([this](XeenMapIdentity id) {
			++scriptLoads[id];
			return script(id, id.number == 23 ? mutationRecords() : presentationRecords());
		}, [this](XeenMapIdentity id) {
			++textLoads[id];
			return XeenEventTextFile{id, "synthetic.txt", true, {"persisted presentation"}};
		}) {}
};

void checkMutation(const Lifecycle &lifecycle, XeenMapIdentity id, bool expected) {
	check(lifecycle.world.sessionState().isObjectDisabled({id, 0}) == expected,
		"object mutation state mismatch");
	for (std::size_t i = 0; i < 2; ++i)
		check(lifecycle.world.isEventDisabled({id, i}) == expected,
			"event mutation state mismatch");
	check(!lifecycle.world.isEventDisabled({id, 2}), "unrelated event changed");
}

void sessionLifecycle() {
	const XeenMapIdentity clouds23{XeenSide::Clouds, 23};
	const XeenMapIdentity clouds24{XeenSide::Clouds, 24};
	const XeenMapIdentity clouds25{XeenSide::Clouds, 25};
	const XeenMapIdentity dark23{XeenSide::Darkside, 23};
	Lifecycle session;
	const auto *owner = &session.world.sessionState();
	const XeenCamera plant{clouds23, 8, 2, XeenDirection::North};
	const XeenEventScript base = script(clouds23, mutationRecords());

	check(session.world.selectObject(plant) == XeenObjectIdentity{clouds23, 0},
		"fresh session selection failed");
	session.world.applyRemove(plant, XeenObjectIdentity{clouds23, 0}, base.file());
	checkMutation(session, clouds23, true);
	checkMutation(session, dark23, false);
	check(session.world.selectObject(plant) == std::nullopt,
		"disabled object remained selectable");

	// Leave and return using map loads rather than retaining a cached map reference.
	session.world.map(clouds24);
	session.world.map(clouds23);
	check(owner == &session.world.sessionState() && session.mapLoads[clouds24] == 1,
		"map transition replaced the session owner");
	checkMutation(session, clouds23, true);

	// Normal interaction begins at line zero and traverses the two effective None records.
	XeenCamera camera = plant;
	XeenGameFlags flags;
	const auto repeated = session.events.runManualEvent(session.world, session.party, camera, flags);
	check(std::get<XeenManualEventCompleted>(repeated).instructionCount == 2,
		"normal interaction replayed the removed branch");
	check(session.scriptLoads[clouds23] == 1, "initial script cache load missing");

	// Independent map/object reconstruction retains mutations and reloads base data.
	const int oldMapLoads = session.mapLoads[clouds23];
	const int oldObjectLoads = session.objectLoads[clouds23];
	session.world.discardMapCache();
	check(session.world.cachedMapCount() == 0 && session.world.cachedObjectFileCount() == 0,
		"map/object cache was not discarded");
	check(session.world.map(clouds23).identity() == clouds23 &&
		session.world.objectFile(clouds23).entities.objects[0].resourceId == 111,
		"map/object base reconstruction failed");
	check(session.mapLoads[clouds23] == oldMapLoads + 1 &&
		session.objectLoads[clouds23] == oldObjectLoads + 1,
		"map/object providers were not called again");
	checkMutation(session, clouds23, true);

	// Independent script reconstruction reloads immutable opcodes but effective state remains None.
	session.events.discardScriptCache();
	check(session.events.cachedScriptCount() == 0, "script cache was not discarded");
	const auto scriptReload = session.events.runManualEvent(session.world, session.party, camera, flags);
	check(std::get<XeenManualEventCompleted>(scriptReload).instructionCount == 2 &&
		session.scriptLoads[clouds23] == 2,
		"reloaded script did not observe session-effective events");
	const auto pristine = mutationRecords();
	check(pristine[0].opcode == 0x0e && pristine[1].opcode == 0x12,
		"base script opcodes were modified");

	// Prime an unrelated text entry so its later load proves text-cache reconstruction.
	XeenCamera textCamera{clouds25, 9, 9, XeenDirection::North};
	check(std::holds_alternative<XeenEventExecutionSuspended>(
		session.events.runManualEvent(session.world, session.party, textCamera, flags)),
		"text-cache priming presentation missing");
	const int oldScript25 = session.scriptLoads[clouds25];
	const int oldText25 = session.textLoads[clouds25];

	// Suspend inside a call, mutate its physical cell, discard every cache, and
	// resume. Return reaches a newly effective None through the owned script copy.
	XeenCamera presentationCamera{clouds24, 9, 9, XeenDirection::North};
	auto pending = std::get<XeenEventExecutionSuspended>(
		session.events.runManualEvent(session.world, session.party, presentationCamera, flags));
	check(pending.state.logicalAddress.x == 7 && pending.state.workingCamera.x == 9 &&
		pending.state.callStack.size() == 1 &&
		pending.state.selectedObject == XeenObjectIdentity{clouds24, 1},
		"suspended state lost logical/physical/selection/call values");
	const int oldMap24 = session.mapLoads[clouds24];
	const int oldObject24 = session.objectLoads[clouds24];
	const int oldScript24 = session.scriptLoads[clouds24];
	const int oldText24 = session.textLoads[clouds24];
	const XeenEventScript presentationBase = script(clouds24, presentationRecords());
	session.world.applyRemove(presentationCamera, pending.state.selectedObject,
		presentationBase.file());
	check(session.world.sessionState().isObjectDisabled({clouds24, 1}) &&
		session.world.isEventDisabled({clouds24, 0}) &&
		session.world.isEventDisabled({clouds24, 1}),
		"suspended mutation was not applied to physical state");
	session.world.discardMapCache();
	session.events.discardScriptCache();
	session.events.discardTextCache();
	check(session.events.cachedScriptCount() == 0 && session.events.cachedTextCount() == 0,
		"event caches were not discarded during suspension");
	session.world.map(clouds24);
	session.world.objectFile(clouds24);
	check(session.mapLoads[clouds24] == oldMap24 + 1 &&
		session.objectLoads[clouds24] == oldObject24 + 1,
		"suspended map/object reconstruction did not occur");
	const auto resumed = session.events.resumeManualEvent(pending.state,
		XeenPresentationResponse::Presented, session.world, session.party, presentationCamera, flags);
	check(std::holds_alternative<XeenManualEventCompleted>(resumed) &&
		presentationCamera.mapId == clouds24 && presentationCamera.x == 9 &&
		session.scriptLoads[clouds24] == oldScript24 &&
		session.textLoads[clouds24] == oldText24,
		"owned suspended script failed to observe effective None after reconstruction");
	checkMutation(session, clouds23, true);
	check(session.world.sessionState().isObjectDisabled({clouds24, 1}) &&
		session.world.isEventDisabled({clouds24, 1}),
		"suspended reconstruction restored world state");

	// The next map-24 interaction reloads its script and traverses effective None.
	presentationCamera = {clouds24, 9, 9, XeenDirection::North};
	const auto rebuiltInteraction = session.events.runManualEvent(
		session.world, session.party, presentationCamera, flags);
	check(session.scriptLoads[clouds24] == oldScript24 + 1 &&
		session.textLoads[clouds24] == oldText24 &&
		std::holds_alternative<XeenManualEventCompleted>(rebuiltInteraction),
		"script provider did not reload effective state after discard");
	// Re-entering the previously cached unrelated presentation reloads its text.
	textCamera = {clouds25, 9, 9, XeenDirection::North};
	pending = std::get<XeenEventExecutionSuspended>(
		session.events.runManualEvent(session.world, session.party, textCamera, flags));
	check(session.scriptLoads[clouds25] == oldScript25 + 1 &&
		session.textLoads[clouds25] == oldText25 + 1 &&
		pending.request.text == "persisted presentation",
		"script/text providers did not reload after discard");

	// Mutate the colliding synthetic side, reconstruct, and prove two-way isolation.
	const XeenEventScript darkBase = script(dark23, mutationRecords());
	const XeenCamera darkPlant{dark23, 8, 2, XeenDirection::North};
	session.world.applyRemove(darkPlant, XeenObjectIdentity{dark23, 0}, darkBase.file());
	checkMutation(session, clouds23, true);
	checkMutation(session, dark23, true);
	session.world.discardMapCache();
	session.world.map(clouds23);
	session.world.objectFile(clouds23);
	session.world.map(dark23);
	session.world.objectFile(dark23);
	checkMutation(session, clouds23, true);
	checkMutation(session, dark23, true);
	check(session.world.sessionState().disabledObjectCount() == 3 &&
		session.world.sessionState().disabledEventCount() == 6,
		"side-colliding identities merged");

	// A genuinely new owner restores the immutable base state and has no pending execution.
	Lifecycle fresh;
	check(&fresh.world.sessionState() != owner &&
		fresh.world.sessionState().disabledObjectCount() == 0 &&
		fresh.world.sessionState().disabledEventCount() == 0,
		"new session inherited mutation state");
	check(fresh.world.selectObject(plant) == XeenObjectIdentity{clouds23, 0},
		"new session did not restore object selection");
	checkMutation(fresh, clouds23, false);
	check(fresh.events.cachedScriptCount() == 0 && fresh.events.cachedTextCount() == 0,
		"new session inherited event caches or pending execution");
}

} // namespace

int main() {
	try {
		sessionLifecycle();
		std::cout << "M15C cache reconstruction, isolation and new-session restoration OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
