#ifndef MMODERN_XEEN_MONSTER_FIXTURE_H
#define MMODERN_XEEN_MONSTER_FIXTURE_H
#include "formats/xeen/XeenMonsterFormat.h"
#include "games/xeen/XeenMap.h"
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mmodern {
// Developer fixture (M68): the original F3 (map 23) MOB with one placed
// monster record retargeted to a chosen original Clouds species at its own,
// legal spawn. Nothing is written to the commercial files or to saves; the
// fixture bytes feed parsing, the regional manifest, reconstruction and renewal
// exactly as the original resource would.
struct XeenMonsterFixture {
	unsigned species = 0;               // xeen.mon record index
	std::optional<unsigned> record;     // map-23 MOB monster record (placement) to retarget; chosen when absent
	std::optional<XeenCamera> start;    // party start on F3; chosen when absent
	bool waterEdge = false;             // start facing water with the record moved across it (not with start)
	std::optional<std::pair<int,int>> spawn; // the record's new cell, set from a water-edge placement
};
struct XeenMonsterFixtureDescription {
	std::string name;
	int x = 0, y = 0;
	unsigned tableIndex = 0;
};
// The chosen placement: the retargeted record, the party's start camera, the
// monster's cell and whether it stands 1-3 cells straight ahead on a clear line.
// `relocated` marks a water-edge placement whose cell is not the record's own.
struct XeenMonsterFixturePlacement {
	unsigned record = 0;
	XeenCamera start;
	int monsterX = 0, monsterY = 0;
	unsigned distance = 0;
	bool inView = false;
	bool relocated = false;
};
// Why a start camera is not legal on F3 (outside the walkable component of the
// Journey entry, an automatic Event cell, a monster spawn, or not map 23), or
// nullopt when it is legal.
std::optional<std::string> xeenMonsterFixtureStartRefusal(const XeenMap &map, const XeenMapEntities &entities,
	const XeenCamera &start);
// Chooses the record and start. Without --start: the first (distance 3, then 2;
// records nearest the Journey entry first, ties by index; facings North, East,
// South, West) legal start that looks straight at the record's spawn over a
// clear line (no blocking middle layer, object or other monster between). With
// --water-edge (no original F3 spawn faces a reachable shore across water): the
// first legal start (nearest the Journey entry, ties by y then x; facings North,
// East, South, West) whose cell right ahead is Water or Deep Water that walkers
// cannot enter, with a free land cell 3 (then 2) cells ahead on a clear line;
// --record or the active record nearest that cell (ties by index) moves there.
// With --start: validates it and takes --record or the first record in view.
// Throws std::invalid_argument with the reason when nothing qualifies.
XeenMonsterFixturePlacement xeenMonsterFixturePlacement(const XeenMap &map, const XeenMapEntities &entities,
	const XeenMonsterFixture &);
// Validates the species against the admission the runtime uses (movement
// capability, admitted rendering, attack capabilities) and the record against the
// original placements, then returns the fixture MOB bytes, with the record
// moved to `spawn` when set. Throws std::invalid_argument with the reason
// otherwise. The record must be chosen.
std::vector<std::uint8_t> xeenMonsterFixtureMob(const std::vector<std::uint8_t> &originalMob,
	const XeenMonsterFixture &, const std::vector<XeenMonsterRecord> &statistics,
	XeenMonsterFixtureDescription *description = nullptr);
}
#endif
