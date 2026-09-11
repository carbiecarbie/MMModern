#ifndef MMODERN_GAMES_XEEN_WORLD_H
#define MMODERN_GAMES_XEEN_WORLD_H

#include "games/xeen/XeenMap.h"
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenEventFile.h"
#include "games/xeen/XeenActor.h"
#include "games/xeen/XeenEncounterEntry.h"
#include <stdexcept>
#include <set>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace mmodern {

struct XeenCellSample {
	XeenMapIdentity mapId = 0;
	int x = 0;
	int y = 0;
	const XeenMapGeometry *geometry = nullptr;
	const XeenMapCell *cell = nullptr;
};

// Session-owned overlays and explicit encounter authority. Original records remain untouched.
class XeenSessionWorldState {
public:
	bool isObjectDisabled(XeenObjectIdentity id) const { return _objects.count(id) != 0; }
	bool isEventDisabled(XeenEventIdentity id) const { return _events.count(id) != 0; }
	std::size_t disabledObjectCount() const { return _objects.size(); }
	std::size_t disabledEventCount() const { return _events.size(); }
	const std::set<XeenObjectIdentity> &disabledObjects() const { return _objects; }
	const std::set<XeenEventIdentity> &disabledEvents() const { return _events; }
	bool encounterMarked() const { return _encounterMarked; }
	XeenEncounterEntry encounterEntry() const noexcept { return _entry; }
	bool encounterInitialized() const { return _encounterInitialized; }
	bool encounterTerminal() const { return _encounterTerminal; }
	const std::vector<XeenActor> &actors() const { return _actors; }
private:
	friend class XeenWorld;
	friend class XeenActorApproach;
	friend class XeenCombat;
	const void *_combatOwner = nullptr;
	const void *_combatApproachState = nullptr;
	bool _diagnostic27 = false, _combatEntered = false, _combatAccounted = false;
	XeenEncounterEntry _entry = XeenEncounterEntry::Ordinary;
	bool _encounterMarked = false, _encounterInitialized = false, _encounterTerminal = false;
	std::uint64_t _encounterRevision = 0;
	std::vector<XeenActor> _actors;
	std::set<XeenObjectIdentity> _objects;
	std::set<XeenEventIdentity> _events;
};

class XeenWorld {
public:
	using MapLoader = std::function<XeenMap(XeenMapIdentity)>;

	using ObjectLoader = std::function<XeenObjectFile(XeenMapIdentity)>;
	using EventLoader = std::function<XeenEventFile(XeenMapIdentity)>;
	explicit XeenWorld(MapLoader loader, ObjectLoader objectLoader = {});
	const XeenObjectFile &objectFile(XeenMapIdentity mapId);
	bool isObjectDisabled(XeenObjectIdentity id);
	bool isEventDisabled(XeenEventIdentity id) const { return _sessionState.isEventDisabled(id); }
	std::optional<XeenObjectIdentity> selectObject(const XeenCamera &camera);
	XeenEventRecord effectiveEvent(XeenEventIdentity id, const XeenEventRecord &base) const;
	void disableObject(XeenObjectIdentity id);
	void disableEventsAtCell(const XeenCamera &physical, const XeenEventFile &events);
	void applyRemove(const XeenCamera &physical, std::optional<XeenObjectIdentity> selected,
		const XeenEventFile &events);
	std::size_t cachedObjectFileCount() const { return _objects.size(); }
	XeenWorld(const XeenWorld &) = delete;
	XeenWorld &operator=(const XeenWorld &) = delete;
	const XeenSessionWorldState &sessionState() const { return _sessionState; }
	// Irreversible safety marker, including failed preparation. No clear/reset API.
	void markEncounterSession() noexcept { _sessionState._encounterMarked = true; }
	void markEncounterSession(XeenEncounterEntry entry) {
		if (entry == XeenEncounterEntry::Ordinary ||
			(_sessionState._entry != XeenEncounterEntry::Ordinary && _sessionState._entry != entry) ||
			(_sessionState._encounterMarked && _sessionState._entry == XeenEncounterEntry::Ordinary))
			throw std::logic_error("Encounter entry cannot be replaced");
		_sessionState._entry = entry;
		_sessionState._encounterMarked = true;
	}
	bool hasEncounterState() const {
		return _sessionState._encounterMarked || _sessionState._encounterInitialized ||
			!_sessionState._actors.empty();
	}
	// For unpublished startup owners only. Validates every original identity
	// before replacing either set; no script execution or cell expansion.
	void restoreSessionState(const std::vector<XeenObjectIdentity> &objects,
		const std::vector<XeenEventIdentity> &events, const EventLoader &eventLoader);
	// Invalidates map/cell/object-file references, not the session state.
	void discardMapCache() { _maps.clear(); _objects.clear(); }

	const XeenMap &map(XeenMapIdentity mapId);
	std::optional<XeenCellSample> sampleCell(XeenMapIdentity mapId, int x, int y);
	std::size_t cachedMapCount() const { return _maps.size(); }

private:
	friend class XeenSaveState;
	friend class XeenCombat;
	friend class XeenActorApproach;
	void swapPreparedState(XeenWorld &candidate) noexcept;
	// Diagnostic27 checks retained authority after fallible resource providers.
	std::function<void()> _combatCheck;
	// Retained Diagnostic27 authorization, separate from domain validity.
	std::function<bool()> _combatAuthorized;
	XeenSessionWorldState _sessionState;
	MapLoader _loader;
	ObjectLoader _objectLoader;
	std::map<XeenMapIdentity, XeenObjectFile> _objects;
	void validateObject(XeenObjectIdentity id);
	void validateEventCell(const XeenCamera &physical, const XeenEventFile &events);
	std::map<XeenMapIdentity, XeenMap> _maps;
};

} // namespace mmodern

#endif
