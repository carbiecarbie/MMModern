#include "games/xeen/XeenMonsterFixture.h"
#include "formats/xeen/XeenMapFormat.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenJourneyContent.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenRegionalRules.h"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace mmodern {
namespace {
bool local(int x,int y) { return x>=0 && x<16 && y>=0 && y<16; }
// Forward deltas by facing, as XeenActorApproach moves the party: North is +y.
constexpr int kDx[]{0,1,0,-1},kDy[]{1,0,-1,0};
bool monsterAt(const XeenMapEntities &entities,int x,int y,std::optional<unsigned> except={}) {
	for (unsigned n=0;n<entities.monsters.size();++n) {
		const auto &m=entities.monsters[n];
		if (m.isActive() && m.x==x && m.y==y && !(except && *except==n)) return true;
	}
	return false;
}
bool objectAt(const XeenMapEntities &entities,int x,int y) {
	for (const auto &o:entities.objects) if (o.isActive() && o.x==x && o.y==y) return true;
	return false;
}
// The monster at distance `k` straight ahead is in view when the ray helper
// clears every row up to it (middle layers that hide a row) and no object or
// other monster stands on the cells between or on the monster's own cell.
bool inView(const XeenMap &map,const XeenMapEntities &entities,const XeenCamera &start,unsigned record,unsigned k) {
	if (k<1 || k>3) return false;
	if (xeenPlayerRayRows(map,start)<=k) return false;
	const auto d=unsigned(start.direction);
	for (unsigned j=1;j<=k;++j) {
		const int x=start.x+kDx[d]*int(j),y=start.y+kDy[d]*int(j);
		if (!local(x,y) || objectAt(entities,x,y) || (j<k && monsterAt(entities,x,y,record))) return false;
	}
	return true;
}
// The monster terrain test for a walker or a flyer of an ordinary species.
XeenMonsterTerrain monsterTerrain(const XeenMap &map,int x,int y,bool flying) {
	if (!local(x,y)) return XeenMonsterTerrain::Unsupported;
	const auto *l=xeenGetIf<XeenOutdoorLayers>(&map.geometry.cells[y*16+x].geometry);
	if (!l || l->surface>=16) return XeenMonsterTerrain::Unsupported;
	return xeenOutdoorMonsterTerrain(l->middle,map.geometry.surfaceTypes[l->surface],map.geometry.difficulties[0],flying,0,map.side);
}
// Water or Deep Water under a middle value whose surface decides: a walker
// cannot enter it, a flyer can.
bool water(const XeenMap &map,int x,int y) {
	const auto *l=local(x,y) ? xeenGetIf<XeenOutdoorLayers>(&map.geometry.cells[y*16+x].geometry) : nullptr;
	if (!l || l->surface>=16) return false;
	const auto surface=map.geometry.surfaceTypes[l->surface];
	return (surface==0 || surface==8) && monsterTerrain(map,x,y,false)==XeenMonsterTerrain::Blocked &&
		monsterTerrain(map,x,y,true)==XeenMonsterTerrain::Allowed;
}
XeenMonsterFixturePlacement waterEdgePlacement(const XeenMap &map,const XeenMapEntities &entities,const XeenMonsterFixture &fixture) {
	const auto &entry=xeenJourneyContent().entry;
	std::vector<std::pair<int,int>> cells;
	for (int y=0;y<16;++y) for (int x=0;x<16;++x) cells.push_back({x,y});
	std::stable_sort(cells.begin(),cells.end(),[&](const auto &a,const auto &b) {
		return std::abs(a.first-entry.x)+std::abs(a.second-entry.y)<std::abs(b.first-entry.x)+std::abs(b.second-entry.y);
	});
	for (const auto &cell:cells) for (unsigned d=0;d<4;++d) {
		const XeenCamera start{23,cell.first,cell.second,static_cast<XeenDirection>(d)};
		if (!water(map,start.x+kDx[d],start.y+kDy[d]) || xeenMonsterFixtureStartRefusal(map,entities,start)) continue;
		for (unsigned k=3;k>=2;--k) {
			const int x=start.x+kDx[d]*int(k),y=start.y+kDy[d]*int(k);
			if (monsterTerrain(map,x,y,false)!=XeenMonsterTerrain::Allowed || monsterAt(entities,x,y) || objectAt(entities,x,y) ||
				xeenPlayerRayRows(map,start)<=k) continue;
			unsigned record=0;
			if (fixture.record) record=*fixture.record;
			else {
				bool found=false;int best=0;
				for (unsigned n=0;n<entities.monsters.size();++n) {
					if (!entities.monsters[n].isActive()) continue;
					const int distance=std::abs(entities.monsters[n].x-x)+std::abs(entities.monsters[n].y-y);
					if (!found || distance<best) { record=n;best=distance;found=true; }
				}
				if (!found) throw std::invalid_argument("Monster fixture: map 23 has no active placement");
			}
			bool clear=true;
			for (unsigned j=1;j<k;++j) {
				const int bx=start.x+kDx[d]*int(j),by=start.y+kDy[d]*int(j);
				clear=clear && !objectAt(entities,bx,by) && !monsterAt(entities,bx,by,record);
			}
			if (!clear) continue;
			XeenMonsterFixturePlacement p;p.record=record;p.start=start;p.monsterX=x;p.monsterY=y;
			p.distance=k;p.inView=true;p.relocated=true;
			return p;
		}
	}
	throw std::invalid_argument("Monster fixture: no legal start faces water with a free cell beyond it; use --start");
}
}
std::optional<std::string> xeenMonsterFixtureStartRefusal(const XeenMap &map,const XeenMapEntities &entities,
		const XeenCamera &start) {
	if (start.mapId!=XeenMapIdentity(23) || map.identity()!=XeenMapIdentity(23)) return "the fixture start must be on F3 (map 23)";
	if (unsigned(start.direction)>3) return "invalid facing";
	if (!local(start.x,start.y)) return "the start cell is outside the 16x16 map";
	if (!XeenMovement::component(map,xeenJourneyContent().entry.x,xeenJourneyContent().entry.y,xeenJourneyContent().traversal)[start.y*16+start.x])
		return "the start cell is not walkable from the Journey entry (water, mountain or unreachable)";
	if (hasAutomaticTrigger(map.geometry,start.x,start.y)) return "the start cell has an automatic Event";
	if (monsterAt(entities,start.x,start.y)) return "a monster spawns on the start cell";
	return std::nullopt;
}
XeenMonsterFixturePlacement xeenMonsterFixturePlacement(const XeenMap &map,const XeenMapEntities &entities,
		const XeenMonsterFixture &fixture) {
	if (fixture.record && (*fixture.record>=entities.monsters.size() || !entities.monsters[*fixture.record].isActive()))
		throw std::invalid_argument("Monster fixture record is not an active original placement");
	const auto placement=[&](unsigned record,const XeenCamera &start,unsigned distance,bool view) {
		XeenMonsterFixturePlacement p;p.record=record;p.start=start;
		p.monsterX=entities.monsters[record].x;p.monsterY=entities.monsters[record].y;p.distance=distance;p.inView=view;
		return p;
	};
	if (fixture.start && fixture.waterEdge)
		throw std::invalid_argument("Monster fixture: --water-edge chooses the start; do not combine it with --start");
	if (fixture.waterEdge) return waterEdgePlacement(map,entities,fixture);
	if (fixture.start) {
		if (const auto refusal=xeenMonsterFixtureStartRefusal(map,entities,*fixture.start))
			throw std::invalid_argument("Monster fixture --start refused: "+*refusal);
		const auto &start=*fixture.start;
		const auto d=unsigned(start.direction);
		const auto ahead=[&](unsigned record) -> unsigned {
			const auto &m=entities.monsters[record];
			for (unsigned k=1;k<=3;++k)
				if (m.x==start.x+kDx[d]*int(k) && m.y==start.y+kDy[d]*int(k)) return inView(map,entities,start,record,k) ? k : 0;
			return 0;
		};
		if (fixture.record) { const auto k=ahead(*fixture.record);return placement(*fixture.record,start,k,k!=0); }
		for (unsigned n=0;n<entities.monsters.size();++n)
			if (entities.monsters[n].isActive()) if (const auto k=ahead(n)) return placement(n,start,k,true);
		return placement(0,start,0,false);
	}
	// Records nearest the Journey entry first (ties by index), so the default
	// start stays close to the familiar route.
	std::vector<unsigned> order;
	for (unsigned n=0;n<entities.monsters.size();++n) if (entities.monsters[n].isActive()) order.push_back(n);
	const auto &entry=xeenJourneyContent().entry;
	const auto nearness=[&](unsigned n) { return std::abs(entities.monsters[n].x-entry.x)+std::abs(entities.monsters[n].y-entry.y); };
	std::stable_sort(order.begin(),order.end(),[&](unsigned a,unsigned b) { return nearness(a)<nearness(b); });
	for (unsigned k=3;k>=2;--k)
		for (const unsigned n:order) {
			if (fixture.record && *fixture.record!=n) continue;
			for (unsigned d=0;d<4;++d) {
				XeenCamera start{23,entities.monsters[n].x-kDx[d]*int(k),entities.monsters[n].y-kDy[d]*int(k),static_cast<XeenDirection>(d)};
				if (!local(start.x,start.y) || xeenMonsterFixtureStartRefusal(map,entities,start) || !inView(map,entities,start,n,k)) continue;
				return placement(n,start,k,true);
			}
		}
	throw std::invalid_argument(fixture.record ? "Monster fixture: record "+std::to_string(*fixture.record)+" has no legal start cell with it in view; use --start"
		: "Monster fixture: no placement has a legal start cell with it in view; use --start");
}
std::vector<std::uint8_t> xeenMonsterFixtureMob(const std::vector<std::uint8_t> &original,
		const XeenMonsterFixture &fixture, const std::vector<XeenMonsterRecord> &statistics,
		XeenMonsterFixtureDescription *description) {
	if (fixture.species>=statistics.size())
		throw std::invalid_argument("Monster fixture species is outside the xeen.mon catalogue");
	if (!fixture.record) throw std::invalid_argument("Monster fixture record is not chosen");
	const unsigned record=*fixture.record;
	const auto &species=statistics[fixture.species];
	if (!xeenMonsterMovementSupported(species,int(fixture.species)))
		throw std::invalid_argument("Monster fixture species '"+species.name()+"' has no HP; it is not a live monster");
	if (!species.supportsAdmittedMechanicsRendering())
		throw std::invalid_argument("Monster fixture species '"+species.name()+"' uses an animation loop or effect the mainland does not admit");
	species.validateAttackCapabilities();
	const auto entities=XeenMapFormat::parseMob(original);
	if (record>=entities.monsters.size() || !entities.monsters[record].isActive())
		throw std::invalid_argument("Monster fixture record is not an active original placement");
	// Resolve the compact table index the record will use: an existing entry for
	// the species, or a free trailing slot. Replacing an interior FF would shift
	// every later compact index and retarget unrelated records.
	std::vector<std::uint8_t> table(entities.monsterTable.begin(),entities.monsterTable.end());
	unsigned compact=0;bool found=false;unsigned index=0;
	for (unsigned slot=0;slot<16 && !found;++slot) {
		if (table[slot]==0xff) continue;
		if (table[slot]==fixture.species) { index=compact;found=true; }
		++compact;
	}
	if (!found) {
		unsigned last=16;
		for (unsigned slot=0;slot<16;++slot) if (table[slot]!=0xff) last=slot;
		const unsigned free=last==16 ? 0 : last+1;
		if (free>=16) throw std::invalid_argument("Monster fixture: the map-23 monster table has no free trailing slot");
		table[free]=static_cast<std::uint8_t>(fixture.species);index=compact;
	}
	// Record layout (XeenMapFormat::parseMob): three 16-byte tables, the object
	// records (dummy + terminator when empty, otherwise records + terminator),
	// then the monster records of four bytes: x, y, table index, direction.
	const std::size_t objectBytes=entities.objects.empty() ? 8 : 4*(entities.objects.size()+1);
	const std::size_t offset=48+objectBytes+4*record;
	if (offset+4>original.size()) throw std::invalid_argument("Monster fixture record offset is outside the MOB resource");
	auto bytes=original;
	for (unsigned slot=0;slot<16;++slot) bytes[16+slot]=table[slot];
	bytes[offset+2]=static_cast<std::uint8_t>(index);
	if (fixture.spawn) {
		if (!local(fixture.spawn->first,fixture.spawn->second))
			throw std::invalid_argument("Monster fixture spawn is outside the 16x16 map");
		bytes[offset]=static_cast<std::uint8_t>(fixture.spawn->first);bytes[offset+1]=static_cast<std::uint8_t>(fixture.spawn->second);
	}
	const int x=fixture.spawn ? fixture.spawn->first : int(entities.monsters[record].x);
	const int y=fixture.spawn ? fixture.spawn->second : int(entities.monsters[record].y);
	// The result must parse to the same placements with one retargeted species
	// (moved only to the requested spawn).
	const auto parsed=XeenMapFormat::parseMob(bytes);
	if (parsed.monsters.size()!=entities.monsters.size() || parsed.monsters[record].resourceId!=int(fixture.species) ||
		parsed.monsters[record].x!=x || parsed.monsters[record].y!=y)
		throw std::logic_error("Monster fixture bytes do not parse to the intended placement");
	for (unsigned n=0;n<parsed.monsters.size();++n) if (n!=record &&
		(parsed.monsters[n].resourceId!=entities.monsters[n].resourceId || parsed.monsters[n].x!=entities.monsters[n].x || parsed.monsters[n].y!=entities.monsters[n].y))
		throw std::logic_error("Monster fixture retargeted an unrelated placement");
	if (description) {
		description->name=species.name();
		description->x=x;description->y=y;
		description->tableIndex=index;
	}
	return bytes;
}
}
