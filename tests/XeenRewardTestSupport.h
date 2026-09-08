#ifndef MMODERN_REWARD_TEST_SUPPORT_H
#define MMODERN_REWARD_TEST_SUPPORT_H
#include "XeenSaveGameplayTestSupport.h"
namespace mmodern {
// Test-only access: seed fresh typed records in an already owned ordinary
// suspension. No production grant/dispatch/replay entry point is added.
struct XeenRewardTestAccess {
	static void seed(XeenEventFlow &flow, unsigned count = 1) {
		gameplay_test::check(flow._pending && flow._pending->state.rewardPhase == XeenRewardPhase::Running &&
			!flow._pending->state.pendingRewards.hasWork(), "seed requires fresh ordinary suspension");
		for (unsigned i = 0; i < count; ++i)
			flow._pending->state.pendingRewards.enqueue({10, static_cast<std::uint8_t>(37+i), 193, 7});
	}
	static const XeenEventExecutionState &state(const XeenEventFlow &flow) { return flow._pending->state; }
};
}
namespace reward_test {
using namespace gameplay_test;
using remove_test::script;
inline XeenEventRecord pause(int line = 0) { return record(1,1,line,9,{44,1,static_cast<std::uint8_t>(line+1)}); }
inline void full(XeenPartyState &p, bool global) {
	for (auto id : p.party.activeRosterIds()) {
		auto &c=p.roster.at(id); c.miscellaneous.back()={1,2,3,4};
		if(global) c.weapons.back()=c.armor.back()=c.accessories.back()={2,3,4,5};
	}
}
inline XeenEventExecutionState pending(const XeenEventExecutionStepResult &r) {
	if(const auto *e=std::get_if<XeenEventExecutionError>(&r))throw std::runtime_error("Expected suspension: "+e->message);
	check(std::holds_alternative<XeenEventExecutionSuspended>(r),"expected reward suspension");
	return std::get<XeenEventExecutionSuspended>(r).state;
}
struct Execution {
	Fixture f;
	XeenWorld world{[](XeenMapIdentity id){return remove_test::map(id);},
		[](XeenMapIdentity id){return XeenObjectFile{id,"test.mob",true,{}};}};
	XeenEventInterpreter interpreter;
	XeenCamera camera{1,1,1,XeenDirection::North}; XeenGameFlags flags;
	XeenEventInterpreter::ScriptProvider scripts = [&](XeenMapIdentity id){return script(id,f.scripts.at(id));};
	XeenEventInterpreter::TextProvider texts = [&](XeenMapIdentity id){auto t=f.text;t.mapId=id;return t;};
	XeenEventExecutionState seed(std::vector<XeenEventRecord> records, unsigned count=1) {
		f.scripts[1]=std::move(records);
		auto s=pending(interpreter.begin(camera,f.initial,flags,world,scripts,texts));
		for(unsigned i=0;i<count;++i)s.pendingRewards.enqueue({10,static_cast<std::uint8_t>(37+i),193,7});
		return s;
	}
	XeenEventExecutionStepResult resume(XeenEventExecutionState state,
		XeenPresentationResponse response=XeenPresentationResponse::Acknowledged) {
		return interpreter.resume(std::move(state),response,f.initial,world,scripts,texts);
	}
};
}
#endif
