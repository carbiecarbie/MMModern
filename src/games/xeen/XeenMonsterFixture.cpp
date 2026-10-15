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
		throw std::invalid_argument(fixture.species==59 ?
			"Monster fixture species '"+species.name()+"' (59) moves only on water; that movement arrives with flight in Part B and is not supported yet" :
			"Monster fixture species '"+species.name()+"' flies or has no HP; flight is not supported yet");
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
	// The result must parse to the same placements with one retargeted species.
	const auto parsed=XeenMapFormat::parseMob(bytes);
	if (parsed.monsters.size()!=entities.monsters.size() || parsed.monsters[record].resourceId!=int(fixture.species))
		throw std::logic_error("Monster fixture bytes do not parse to the intended placement");
	for (unsigned n=0;n<parsed.monsters.size();++n) if (n!=record &&
		parsed.monsters[n].resourceId!=entities.monsters[n].resourceId)
		throw std::logic_error("Monster fixture retargeted an unrelated placement");
	if (description) {
		description->name=species.name();
		description->x=entities.monsters[record].x;description->y=entities.monsters[record].y;
		description->tableIndex=index;
	}
	return bytes;
}
}
