#ifndef MMODERN_XEEN_EVENT_PUBLICATION_H
#define MMODERN_XEEN_EVENT_PUBLICATION_H
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenEventInterpreter.h"
#include <limits>
namespace mmodern {
// A stack-bound publication capability, issued only to the live Journey continuation.
// It anticipates known effects in retained guard storage, never adopts callback state.
class XeenEventPublication {
public:
	XeenEventPublication(const XeenEventPublication &) = delete;
	XeenEventPublication &operator=(const XeenEventPublication &) = delete;
	void check() const { authority(); guard.check(); }
	void script(const XeenEventFile &file) const {
		check();
		if (file.mapId != original.mapId || file.resourcePresent != original.resourcePresent || file.records.size() != original.records.size())
			integrity("Journey event topology changed");
		for (std::size_t i=0;i<file.records.size();++i) {
			const auto &a=file.records[i]; const auto &b=original.records[i];
			if (a.x!=b.x || a.y!=b.y || a.direction!=b.direction || a.line!=b.line || a.opcode!=b.opcode || a.parameters!=b.parameters || a.fileOffset!=b.fileOffset || a.lengthField!=b.lengthField)
				integrity("Journey event record changed");
		}
	}
	void execution(const XeenEventExecutionState &state) const {
		check();
		if (!xeen_state::sameCamera(state.workingCamera,guard.cameraValue) || state.workingGameFlags.values()!=guard.flagValues ||
			state.logicalAddress.mapId!=XeenMapIdentity(20) || state.logicalAddress.x!=5 || state.logicalAddress.y!=14 ||
			state.logicalAddress.line<0 || state.logicalAddress.line>5 || !state.callStack.empty() ||
			(state.selectedObject && !(*state.selectedObject==XeenObjectIdentity{20,1})))
			integrity("Journey event continuation changed");
		if (!state.currentScript) integrity("Journey event script missing");
		script(state.currentScript->file());
	}
	void prepareGrant(std::size_t index) const {
		check();
		if (index!=18 || grant || guard.quests[index]==std::numeric_limits<std::uint32_t>::max())
			throw std::logic_error("Journey grant publication unavailable");
		grant=true;
	}
	void granted() const noexcept { ++guard.quests[18]; }
	void prepareRemove(const XeenCamera &physical, std::optional<XeenObjectIdentity> selected,
		const XeenEventFile &file) const {
		check(); script(file);
		if (!xeen_state::sameCamera(physical,guard.cameraValue) || (selected && !(*selected==XeenObjectIdentity{20,1})) || removal)
			throw std::logic_error("Journey Remove publication unavailable");
		objects=guard.s._objects; events=guard.s._events;
		if (selected) objects.insert(*selected);
		for (std::size_t i=1;i<=5;++i) events.insert({20,i});
		removal=true;
	}
	void removed() const noexcept { guard.s._objects.swap(objects); guard.s._events.swap(events); }
private:
	[[noreturn]] void integrity(const char *message) const { guard.failed=true; throw std::logic_error(message); }
	friend class XeenEventFlow;
	XeenEventPublication(XeenRestoreGuard &guard, const XeenEventFile &original, std::function<void()> authority) :
		guard(guard), original(original), authority(std::move(authority)) { check(); }
	XeenRestoreGuard &guard;
	const XeenEventFile &original;
	std::function<void()> authority;
	mutable bool grant=false, removal=false;
	mutable std::set<XeenObjectIdentity> objects;
	mutable std::set<XeenEventIdentity> events;
};
}
#endif
