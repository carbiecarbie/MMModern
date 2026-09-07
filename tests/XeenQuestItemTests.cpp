#include "XeenRemoveTestSupport.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenEventSystem.h"

#include <algorithm>
#include <iostream>
#include <limits>

using namespace mmodern;
using namespace remove_test;

namespace {
using Bytes = std::vector<std::uint8_t>;

template<class F> void rejects(F f) {
	bool rejected = false;
	try { f(); } catch (const std::exception &) { rejected = true; }
	check(rejected, "expected bounded access/parser rejection");
}

XeenPartyState party() {
	Bytes roster(XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize, 0);
	Bytes bytes(XeenQuestItemFormat::kRequiredSize, 0);
	std::fill(bytes.begin() + 2, bytes.begin() + 10, 0xff);
	bytes[0] = bytes[1] = 1; bytes[2] = 0;
	return XeenPartyLoader().loadFromResources(roster, bytes);
}

void valueAndLoading() {
	const XeenCloudsQuestItems empty;
	for (std::size_t i = 0; i < 35; ++i) {
		check(empty.at(i) == 0, "default count not zero");
		check(XeenCloudsQuestItems::indexForItemId(82 + i) == i, "item ID mapping");
	}
	for (std::int64_t id : {-1LL, 0LL, 81LL, 117LL, 4294967296LL,
		std::numeric_limits<std::int64_t>::min(), std::numeric_limits<std::int64_t>::max()})
		check(!XeenCloudsQuestItems::indexForItemId(id), "unsupported item mapped to a counter");
	rejects([&] { empty.at(35); });
	rejects([&] { empty.at(static_cast<std::size_t>(-1)); });
	XeenCloudsQuestItems::Counts wide{};
	wide[17] = std::numeric_limits<std::uint32_t>::max();
	check(XeenCloudsQuestItems(wide).at(17) == wide[17] && empty.at(17) == 0,
		"count narrowed or value instances share storage");

	Bytes bytes(782, 0xa5);
	for (std::size_t i = 0; i < 35; ++i) bytes[747 + i] = static_cast<std::uint8_t>(i + 1);
	bytes[764] = 255;
	const auto original = bytes;
	const auto counts = XeenQuestItemFormat::parseClouds(bytes);
	for (std::size_t i = 0; i < 35; ++i)
		check(counts.at(i) == (i == 17 ? 255 : i + 1), "serialized counter offset/width");
	check(bytes == original, "parser modified its input");
	bytes.resize(850, 0x5a);
	check(XeenQuestItemFormat::parseClouds(bytes).counts() == counts.counts(), "tail interpreted as Clouds counts");
	for (std::size_t size : {0, 10, 746, 747, 781})
		rejects([&] { XeenQuestItemFormat::parseClouds(Bytes(size, 0)); });

	Bytes roster(XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize, 0);
	bytes[0] = bytes[1] = 1; std::fill(bytes.begin() + 2, bytes.begin() + 10, 0xff); bytes[2] = 3;
	const auto loaded = XeenPartyLoader().loadFromResources(roster, bytes);
	check(loaded.questItems.counts() == counts.counts() && loaded.party.activeRosterIds() == Bytes{3},
		"full party loader lost counters or member order");
	rejects([&] { XeenPartyLoader().loadFromResources(roster, Bytes(bytes.begin(), bytes.begin() + 781)); });
	check(XeenCharacterFormat::parsePartyHeader(Bytes(bytes.begin(), bytes.begin() + 10)).effectiveCount == 1,
		"header-only parser contract changed");
}

struct Fixture {
	std::map<XeenMapIdentity, XeenEventScript> scripts;
	int mapLoads = 0, scriptLoads = 0, textLoads = 0;
	XeenWorld world{[&](XeenMapIdentity id) { ++mapLoads; return map(id); }};
	XeenPartyState members = party();
	XeenEventInterpreter interpreter;
	XeenEventSystem events{[&](XeenMapIdentity id) { ++scriptLoads; return scripts.at(id); },
		[&](XeenMapIdentity id) { ++textLoads; return XeenEventTextFile{id, "synthetic.txt", true, {"Text"}}; }};
	XeenEventExecutionStepResult begin(int line = 0, XeenMapIdentity id = 1) {
		return interpreter.begin({id, 1, 1, XeenDirection::North}, members, {}, world,
			[&](XeenMapIdentity key) { return scripts.at(key); }, {}, static_cast<std::uint8_t>(line));
	}
	XeenEventExecutionStepResult resume(XeenEventExecutionState state,
		XeenPresentationResponse response = XeenPresentationResponse::Acknowledged) {
		return interpreter.resume(std::move(state), response, members, world,
			[&](XeenMapIdentity id) { return scripts.at(id); }, {});
	}
};

XeenEventExecutionState pending(const XeenEventExecutionStepResult &r) {
	const auto *s = std::get_if<XeenEventExecutionSuspended>(&r);
	check(s != nullptr, "expected acknowledgment suspension"); return s->state;
}
XeenEventExecutionCompleted done(const XeenEventExecutionStepResult &r) {
	if (const auto *e = std::get_if<XeenEventExecutionError>(&r)) throw std::runtime_error(e->message);
	check(std::holds_alternative<XeenEventExecutionCompleted>(r), "expected completion");
	return std::get<XeenEventExecutionCompleted>(r);
}
XeenEventExecutionError failed(const XeenEventExecutionStepResult &r, XeenEventExecutionErrorKind kind) {
	const auto *e = std::get_if<XeenEventExecutionError>(&r);
	check(e && e->kind == kind, "unexpected error kind"); return *e;
}

void conditions() {
	for (int count : {0, 1, 7}) for (int op : {8, 9, 10}) for (int id : {82, 99, 116}) {
		Fixture f;
		XeenCloudsQuestItems::Counts counts{};
		counts[1] = 14; counts[id - 82] = count;
		f.members.questItems = XeenCloudsQuestItems(counts);
		f.scripts.emplace(1, script(1, {record(1,1,0,op,{21,static_cast<std::uint8_t>(id),2}),
			record(1,1,1,0x12), record(1,1,2,0x0c,{0,0,20,7})}));
		const auto result = done(f.begin());
		check(result.finalGameFlags.isSet(7) == (op == 8 || count != 0), "possession comparison used bool/raw count");
		check(f.members.questItems.counts() == counts, "condition mutated ownership");
	}
	for (int id : {0,81,117,255}) {
		Fixture f; f.scripts.emplace(1, script(1,{record(1,1,0,9,{21,static_cast<std::uint8_t>(id),1})}));
		const auto e = failed(f.begin(), XeenEventExecutionErrorKind::UnsupportedConditionAction);
		check(e.source && e.source->fileOffset == 100 && e.message.find(std::to_string(id)) != std::string::npos,
			"unsupported item diagnostic lost source/value");
	}
	Fixture empty; empty.members = {};
	empty.scripts.emplace(1, script(1,{record(1,1,0,9,{21,99,1})}));
	failed(empty.begin(), XeenEventExecutionErrorKind::EmptyParty);
	Fixture dark; const XeenMapIdentity darkId{XeenSide::Darkside,1};
	dark.scripts.emplace(darkId, script(darkId,{record(1,1,0,9,{21,99,1})}));
	failed(dark.begin(0,darkId), XeenEventExecutionErrorKind::UnsupportedExecutionContext);
	Fixture physical;
	physical.scripts.emplace(1,script(1,{record(1,1,0,9,{44,1,1}),record(1,1,1,9,{21,99,2})}));
	auto state = pending(physical.begin()); state.workingCamera.mapId = darkId;
	failed(physical.resume(state), XeenEventExecutionErrorKind::UnsupportedExecutionContext);
	state = pending(physical.begin()); state.logicalAddress.mapId = darkId;
	failed(physical.resume(state), XeenEventExecutionErrorKind::UnsupportedExecutionContext);

	Fixture call;
	XeenCloudsQuestItems::Counts held{}; held[17]=2; call.members.questItems=XeenCloudsQuestItems(held);
	call.scripts.emplace(1,script(1,{record(1,1,0,0x19,{7,8,0}), record(1,1,1,0x1f,{2,1,1}),
		record(7,8,0,9,{21,99,2}),record(7,8,1,0xff),record(7,8,2,0x1a)}));
	call.scripts.emplace(2,script(2,{record(1,1,0,9,{21,99,2}),record(1,1,1,0xff),record(1,1,2,0x12)}));
	check(done(call.begin()).finalCamera.mapId == XeenMapIdentity{2} && call.members.questItems.at(17)==2,
		"call/transfer lost party ownership");
	Fixture grant; grant.scripts.emplace(1,script(1,{record(1,1,0,0x0c,{0,0,21,99})}));
	check(done(grant.begin()).instructionCount==1 && grant.members.questItems.at(17)==1,
		"17B grant did not update party ownership");
}

void acknowledgmentBoundaries() {
	Fixture adjacent; adjacent.scripts.emplace(1,script(1,{record(1,1,0,9,{44,1,1})}));
	auto state = pending(adjacent.begin());
	check(done(adjacent.resume(state)).instructionCount==1,"absent successor counted as dispatch");
	failed(adjacent.resume(state,XeenPresentationResponse::Yes),XeenEventExecutionErrorKind::InvalidPresentationResponse);
	state.instructionCount=XeenEventInterpreter::kMaximumInstructions;
	check(done(adjacent.resume(state)).instructionCount==1024,"missing instruction consumed budget");
	for (int op : {0,0x12,0x01,0xff}) {
		Fixture f; f.scripts.emplace(1,script(1,{record(1,1,0,9,{44,1,1}),record(1,1,1,op)}));
		auto p=pending(f.begin());
		if(op==0 || op==0x12) check(done(f.resume(p)).instructionCount==2,"present successor skipped");
		else failed(f.resume(p),op==0x01?XeenEventExecutionErrorKind::MalformedInstruction:XeenEventExecutionErrorKind::UnsupportedOpcode);
		if(op==0) { p.instructionCount=1024; failed(f.resume(p),XeenEventExecutionErrorKind::InstructionLimitExceeded); }
	}
	Fixture effective;
	effective.scripts.emplace(1,script(1,{record(1,1,0,9,{44,1,1}),record(1,1,1,0x0c,{0,0,21,99})}));
	auto p=pending(effective.begin());
	effective.world.disableEventsAtCell({1,1,1,XeenDirection::North},effective.scripts.at(1).file());
	check(done(effective.resume(p)).instructionCount==2,"effective None considered absent");
	Fixture far; far.scripts.emplace(1,script(1,{record(1,1,0,9,{44,1,2})}));
	failed(far.resume(pending(far.begin())),XeenEventExecutionErrorKind::InvalidJumpTarget);
	Fixture yes; yes.scripts.emplace(1,script(1,{record(1,1,0,9,{44,0,1})}));
	failed(yes.resume(pending(yes.begin()),XeenPresentationResponse::Yes),XeenEventExecutionErrorKind::InvalidJumpTarget);
	Fixture ordinary; ordinary.scripts.emplace(1,script(1,{record(1,1,0,8,{20,7,1})}));
	failed(ordinary.begin(),XeenEventExecutionErrorKind::InvalidJumpTarget);
	Fixture missingCall; missingCall.scripts.emplace(1,script(1,{record(1,1,0,0x19,{7,8,0})}));
	failed(missingCall.begin(),XeenEventExecutionErrorKind::InvalidCallTarget);
	Fixture high; high.scripts.emplace(1,script(1,{record(1,1,254,9,{44,1,255}),record(1,1,255,9,{44,1,0})}));
	auto next=high.resume(pending(high.begin(254)));
	check(pending(next).logicalAddress.line==255,"existing line255 not executed");
	failed(high.resume(pending(next)),XeenEventExecutionErrorKind::InvalidJumpTarget);
	Fixture end; end.scripts.emplace(1,script(1,{record(1,1,254,9,{44,1,255})}));
	check(done(end.resume(pending(end.begin(254)))).instructionCount==1,"missing line255 not terminal");
	Fixture wrap; wrap.scripts.emplace(1,script(1,{record(1,1,255,9,{44,1,0}),record(1,1,0,0x12)}));
	check(done(wrap.resume(pending(wrap.begin(255)))).instructionCount==2,"valid backward jump changed");

	// Logical called cell controls lookup; a physical next-line record must not match.
	Fixture call; call.scripts.emplace(1,script(1,{record(1,1,0,0x19,{7,8,0}),record(1,1,1,0xff),
		record(7,8,0,9,{44,1,1}),record(7,8,1,0xff,{},1)}));
	auto called=pending(call.begin());
	check(called.callStack.size()==1 && called.logicalAddress.x==7 && called.workingCamera.x==1,"call location fixture");
	check(done(call.resume(called)).instructionCount==2,"terminal call invented return or used wrong direction/cell");
	called.lookupDirection=XeenDirection::East;
	failed(call.resume(called),XeenEventExecutionErrorKind::UnsupportedOpcode);
}

void cachedSuspensionAndCommit() {
	Fixture f;
	f.scripts.emplace(1,script(1,{record(1,1,0,0x0c,{0,0,20,7}), record(1,1,1,0x19,{7,8,0}),
		record(1,1,2,0xff), record(7,8,0,0x01,{0}),record(7,8,1,9,{44,1,2})}));
	XeenCamera camera{1,1,1,XeenDirection::North}; XeenGameFlags flags;
	auto result=f.events.runManualEvent(f.world,f.members,camera,flags);
	check(!flags.isSet(7),"suspension committed flags early");
	auto s=std::get<XeenEventExecutionSuspended>(result);
	result=f.events.resumeManualEvent(s.state,XeenPresentationResponse::Presented,f.world,f.members,camera,flags);
	s=std::get<XeenEventExecutionSuspended>(result);
	f.events.discardScriptCache(); f.events.discardTextCache(); f.world.discardMapCache();
	result=f.events.resumeManualEvent(s.state,XeenPresentationResponse::Acknowledged,f.world,f.members,camera,flags);
	check(std::holds_alternative<XeenManualEventCompleted>(result) && flags.isSet(7),"terminal suspension did not commit working flags");
	check(std::get<XeenManualEventCompleted>(result).instructionCount==4,"call terminal instruction accounting");
	f.events.runManualEvent(f.world,f.members,camera,flags);
	check(f.scriptLoads==2 && f.textLoads==2 && f.mapLoads==2,"cache discard did not reload providers");
}
}

int main() {
	try { valueAndLoading(); conditions(); acknowledgmentBoundaries(); cachedSuspensionAndCommit();
		std::cout << "M17A quest prefix, possession and terminal acknowledgment boundaries OK\n";
		return 0;
	} catch(const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
