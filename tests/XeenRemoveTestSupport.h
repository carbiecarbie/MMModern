#ifndef MMODERN_TESTS_XEEN_REMOVE_SUPPORT_H
#define MMODERN_TESTS_XEEN_REMOVE_SUPPORT_H

#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenEventScript.h"
#include <sstream>
#include <stdexcept>

namespace remove_test {
using namespace mmodern;
inline void check(bool value, const char *message) {
	if (!value) throw std::runtime_error(message);
}
inline bool sameObject(const XeenMapEntity &a, const XeenMapEntity &b) {
	return a.x == b.x && a.y == b.y && a.tableIndex == b.tableIndex &&
		a.direction == b.direction && a.resourceId == b.resourceId;
}
inline bool sameRecord(const XeenEventRecord &a, const XeenEventRecord &b, bool opcode = true) {
	return a.fileOffset == b.fileOffset && a.lengthField == b.lengthField &&
		a.x == b.x && a.y == b.y && a.direction == b.direction && a.line == b.line &&
		a.parameters == b.parameters && (!opcode || a.opcode == b.opcode);
}
inline void sameEntities(const XeenMapEntities &a, const XeenMapEntities &b) {
	check(a.objectTable == b.objectTable && a.monsterTable == b.monsterTable &&
		a.wallItemTable == b.wallItemTable, "MOB resource tables changed");
	for (auto pair : {std::make_pair(&a.objects, &b.objects),
		std::make_pair(&a.monsters, &b.monsters), std::make_pair(&a.wallItems, &b.wallItems)}) {
		check(pair.first->size() == pair.second->size(), "MOB record count changed");
		for (std::size_t i = 0; i < pair.first->size(); ++i)
			check(sameObject((*pair.first)[i], (*pair.second)[i]), "MOB base record changed");
	}
}
inline std::string geometrySnapshot(const XeenMapGeometry &g) {
	std::ostringstream s;
	auto value = [&](auto x) { s << +x << ','; };
	value(g.id); value(g.flags); value(g.flags2);
	for (auto x : g.neighbors) value(x);
	for (auto x : g.wallTypes) value(x);
	for (auto x : g.surfaceTypes) value(x);
	for (auto x : g.difficulties) value(x);
	value(g.floorType); value(g.runX); value(g.runY); value(g.trapDamage);
	value(g.wallKind); value(g.tavernTips);
	for (const auto &c : g.cells) {
		value(c.rawWord); value(c.rawAttributes); value(c.surfaceIndex); value(c.flags);
		value(c.seen); value(c.stepped); value(c.geometry.index());
		if (const auto *w = std::get_if<XeenIndoorWalls>(&c.geometry)) {
			for (auto x : w->walls) value(x);
		} else {
			const auto &o = std::get<XeenOutdoorLayers>(c.geometry);
			value(o.surface); value(o.middle); value(o.top); value(o.overlay);
		}
	}
	return s.str();
}
inline XeenEventRecord record(int x, int y, int line, int opcode,
		std::vector<std::uint8_t> params = {}, int direction = 4, std::size_t offset = 100) {
	return {offset, static_cast<std::uint8_t>(5 + params.size()),
		static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y),
		static_cast<std::uint8_t>(direction), static_cast<std::uint8_t>(line),
		static_cast<std::uint8_t>(opcode), std::move(params)};
}
inline XeenMap map(XeenMapIdentity id) {
	XeenMap m; m.side = id.side; m.geometry.id = id.number; m.geometry.flags2 = 0x8000;
	for (auto &c : m.geometry.cells) c.geometry = XeenOutdoorLayers{};
	return m;
}
inline XeenEventScript script(XeenMapIdentity id, std::vector<XeenEventRecord> records) {
	return XeenEventScript({id, "synthetic.evt", true, std::move(records)});
}
} // namespace remove_test
#endif
