#ifndef MMODERN_GAMES_XEEN_WORLD_H
#define MMODERN_GAMES_XEEN_WORLD_H

#include "games/xeen/XeenMap.h"
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenEventFile.h"
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

// Session-owned overlays only. Original object/event records remain untouched.
class XeenSessionWorldState {
public:
	bool isObjectDisabled(XeenObjectIdentity id) const { return _objects.count(id) != 0; }
	bool isEventDisabled(XeenEventIdentity id) const { return _events.count(id) != 0; }
	std::size_t disabledObjectCount() const { return _objects.size(); }
	std::size_t disabledEventCount() const { return _events.size(); }
	const std::set<XeenObjectIdentity> &disabledObjects() const { return _objects; }
	const std::set<XeenEventIdentity> &disabledEvents() const { return _events; }
private:
	friend class XeenWorld;
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
	void swapPreparedState(XeenWorld &candidate) noexcept;
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
