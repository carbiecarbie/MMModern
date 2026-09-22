#include "games/xeen/XeenRegionalRules.h"
#include <stdexcept>
#include "formats/xeen/XeenMapFormat.h"
#include "formats/xeen/XeenEventFormat.h"
#include "games/xeen/XeenStateEquality.h"
#include <zlib.h>

// Adapted from the ScummVM developers' GPL-3.0-or-later Xeen combat.cpp
// at 6814ee9ba54582f5b5adcffab49efbbd8f589edd: canMonsterMove/stopAttack.
namespace mmodern {
std::optional<std::size_t> xeenRegionalEvent(const XeenEventFile &events,const XeenCamera &camera) {
	if (!events.resourcePresent || events.mapId!=XeenMapIdentity(23) || camera.mapId!=events.mapId || static_cast<unsigned>(camera.direction)>3)
		throw std::invalid_argument("Invalid regional event lookup");
	for (std::size_t i=0;i<events.records.size();++i) {
		const auto &r=events.records[i];
		if (r.x==camera.x && r.y==camera.y && !r.line && (r.direction==4 || r.direction==unsigned(camera.direction))) return i;
	}
	return {};
}
bool xeenRegionalSign(const XeenEventFile &events,const XeenCamera &camera) {
	const auto record=xeenRegionalEvent(events,camera);
	if (!record || *record!=56 || camera.x!=5 || camera.y!=9 || camera.direction!=XeenDirection::North) return false;
	const auto &r=events.records[56];
	if (r.fileOffset!=495 || r.lengthField!=6 || r.direction!=0 || r.line!=0 || r.opcode!=4 || r.parameters!=std::vector<std::uint8_t>{16}) return false;
	for (const auto &next:events.records) if (next.x==5 && next.y==9 && next.line==1 && (next.direction==0 || next.direction==4)) return false;
	return true;
}
void xeenValidateRegionalManifest(const XeenMap &map, const XeenObjectFile &objects, const XeenEventFile &events,
		const std::vector<XeenMonsterRecord> &statistics, const std::vector<std::uint8_t> &dat,
		const std::vector<std::uint8_t> &mob, const std::vector<std::uint8_t> &evt) {
	const auto crc=[](const auto &bytes) { return crc32(0,bytes.data(),static_cast<uInt>(bytes.size())); };
	if (dat.size()!=892 || crc(dat)!=0x8f3e28ee || mob.size()!=220 || crc(mob)!=0xce08a8c8 ||
		evt.size()!=1440 || crc(evt)!=0xe3128711)
		throw std::invalid_argument("Regional DAT/MOB/EVT manifest mismatch");
	XeenMap expected;expected.geometry=XeenMapFormat::parseDat(dat);
	if (!xeen_state::sameMap(map,expected) || objects.mapId!=XeenMapIdentity(23) || !objects.resourcePresent ||
		!xeen_state::sameEntities(objects.entities,XeenMapFormat::parseMob(mob)) || events.mapId!=XeenMapIdentity(23) || !events.resourcePresent)
		throw std::invalid_argument("Regional typed resources differ from manifest");
	const auto records=XeenEventFormat::parse(evt);
	if (records.size()!=events.records.size()) throw std::invalid_argument("Regional event count changed");
	for (unsigned i=0;i<records.size();++i) {
		const auto &a=records[i],&b=events.records[i];
		if (a.fileOffset!=b.fileOffset || a.lengthField!=b.lengthField || a.x!=b.x || a.y!=b.y ||
			a.direction!=b.direction || a.line!=b.line || a.opcode!=b.opcode || a.parameters!=b.parameters)
			throw std::invalid_argument("Regional typed event changed");
	}
	constexpr unsigned types[]{3,6,8,9,13};
	constexpr std::uint32_t checksums[]{0x7f7e3f71,0xeb3b54b1,0xe36833c6,0x5002c318,0x5636ae25};
	for (unsigned i=0;i<5;++i)
		if (statistics.size()<=types[i] || crc(statistics[types[i]].raw)!=checksums[i])
			throw std::invalid_argument("Regional monster statistics manifest mismatch");
}
namespace {
bool local(int x, int y) { return x>=0 && x<16 && y>=0 && y<16; }
bool rayMiddle(unsigned middle) {
	switch (middle) {
	case 0:case 2:case 4:case 5:case 8:case 11:case 13:case 14:return true;
	default:return false;
	}
}
}
unsigned xeenPlayerRayRows(const XeenMap &map,const XeenCamera &camera) {
 constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};const auto d=unsigned(camera.direction);
 if(d>3 || !local(camera.x,camera.y)) throw std::invalid_argument("Invalid player ray origin");
 for(unsigned row=1;row<4;++row) {
  const int x=camera.x+dx[d]*int(row),y=camera.y+dy[d]*int(row);
  if(!local(x,y)) return row;
  const auto *l=std::get_if<XeenOutdoorLayers>(&map.geometry.cells[y*16+x].geometry);
  if(!l) throw std::invalid_argument("Shoot requires outdoor geometry");
  const auto m=l->middle;
  if(m==1 || m==3 || m==6 || m==7 || m==9 || m==10 || m==12) return row;
 }
 return 4;
}
XeenMonsterTerrain xeenRegionalActorTerrain(const XeenMap &map, const XeenActor &a, int x, int y) {
	if (!local(x,y) || map.side!=XeenSide::Clouds || !map.geometry.isOutdoors() ||
		map.identity()!=a.id.mapId || !a.statistics || !a.statistics->supportsGroundMovement() || a.original.resourceId==59)
		return XeenMonsterTerrain::Unsupported;
	const auto *l=std::get_if<XeenOutdoorLayers>(&map.geometry.cells[y*16+x].geometry);
	if (!l || l->surface>=16 || l->middle>=16) return XeenMonsterTerrain::Unsupported;
	const auto surface=map.geometry.surfaceTypes[l->surface];
	switch (l->middle) {
	case 0:case 2:case 3:case 4:case 5:case 6:case 8:case 11:case 13:case 14:
		if (surface>=16) return XeenMonsterTerrain::Unsupported;
		return surface==0 || surface==8 || surface==15 ? XeenMonsterTerrain::Blocked : XeenMonsterTerrain::Allowed;
	default:
		return l->middle<=map.geometry.difficulties[0] ? XeenMonsterTerrain::Allowed : XeenMonsterTerrain::Blocked;
	}
}
std::bitset<256> xeenActorClosure(const XeenMap &map, const XeenActor &a) {
	if (!local(a.original.x,a.original.y)) throw std::invalid_argument("Invalid original actor spawn");
	std::bitset<256> result;
	std::array<int,256> queue{};unsigned begin=0,end=0;
	queue[end++]=a.original.y*16+a.original.x;result.set(queue[0]);
	while (begin!=end) {
		const int index=queue[begin++],x=index%16,y=index/16;
		for (const auto d:{std::pair<int,int>{1,0},{-1,0},{0,1},{0,-1}}) {
			const int nx=x+d.first,ny=y+d.second;
			if (!local(nx,ny)) continue;
			const auto terrain=xeenRegionalActorTerrain(map,a,nx,ny);
			if (terrain==XeenMonsterTerrain::Unsupported) throw std::invalid_argument("Unsupported actor closure terrain");
			if (terrain==XeenMonsterTerrain::Allowed && !result[ny*16+nx]) {
				result.set(ny*16+nx);queue[end++]=ny*16+nx;
			}
		}
	}
	return result;
}
bool xeenOutdoorRangedRay(const XeenMap &map, const XeenCamera &camera, const XeenActor &a) {
	if (camera.mapId!=map.identity() || a.id.mapId!=map.identity() || !map.geometry.isOutdoors() ||
		map.side!=XeenSide::Clouds || !local(camera.x,camera.y) || !local(a.x,a.y))
		throw std::invalid_argument("Invalid outdoor ranged geometry");
	if (a.x!=camera.x && a.y!=camera.y) return false;
	const int dx=(a.x>camera.x)-(a.x<camera.x),dy=(a.y>camera.y)-(a.y<camera.y);
	if (!dx && !dy) return true;
	int x=camera.x,y=camera.y;
	do {
		x+=dx;y+=dy;
		const auto &cell=map.geometry.cells[y*16+x];
		const auto *l=std::get_if<XeenOutdoorLayers>(&cell.geometry);
		if (!l) throw std::invalid_argument("Invalid outdoor ray cell");
		if (dx>0 ? (cell.rawWord&8)!=0 : !rayMiddle(l->middle)) return false;
	} while (x!=a.x || y!=a.y);
	return true;
}
void xeenValidateRegionalActors(const XeenMap &map, const XeenObjectFile &mob, const std::vector<XeenActor> &actors,
		const std::set<XeenMonsterIdentity> &accounted) {
	if (map.identity()!=XeenMapIdentity(23) || mob.mapId!=map.identity() || !mob.resourcePresent ||
		actors.size()!=19 || mob.entities.monsters.size()!=19) throw std::invalid_argument("Incomplete regional actor collection");
	for (auto id:accounted) if (id.mapId!=map.identity() || id.recordIndex>=19)
		throw std::invalid_argument("Invalid regional accounting identity");
	for (unsigned i=0;i<actors.size();++i) {
		const auto &a=actors[i];const auto &original=mob.entities.monsters[i];
		if (!(a.id==XeenMonsterIdentity{23,i}) || a.original.x!=original.x || a.original.y!=original.y ||
			a.original.tableIndex!=original.tableIndex || a.original.direction!=original.direction || a.original.resourceId!=original.resourceId ||
			!a.statistics || !a.statistics->supportsGroundMovement() || !a.statistics->supportsRendering() || a.status!=XeenActorStatus::Physical)
			throw std::invalid_argument("Regional actor immutable identity/profile changed");
		const auto type=a.original.resourceId;
		std::uint32_t expected=0;
		switch (type) { case 3:expected=0x7f7e3f71;break;case 6:expected=0xeb3b54b1;break;case 8:expected=0xe36833c6;break;
		case 9:expected=0x5002c318;break;case 13:expected=0x5636ae25;break;default:throw std::invalid_argument("Unknown regional species"); }
		if (crc32(0,a.statistics->raw.data(),60)!=expected) throw std::invalid_argument("Changed regional statistics");
		if (a.lifecycle==XeenActorLifecycle::Defeated) {
			if (a.hp || a.x!=-128 || a.y!=-128 || a.activated || !accounted.count(a.id))
				throw std::invalid_argument("Noncanonical regional defeated actor");
		} else if (a.lifecycle!=XeenActorLifecycle::Present || a.hp<1 || a.hp>a.statistics->baseHp() ||
			accounted.count(a.id) || !local(a.x,a.y) || (!a.activated && (a.x!=original.x || a.y!=original.y)) ||
			!xeenActorClosure(map,a)[a.y*16+a.x]) throw std::invalid_argument("Invalid regional live actor value");
	}
	for (auto count:XeenActorApproach::occupancy(actors)) if (count>3)
		throw std::invalid_argument("Regional occupancy exceeded");
}
}

namespace mmodern {
XeenRegionalOpportunityCandidate::XeenRegionalOpportunityCandidate(const XeenMap &map,
		const std::vector<XeenActor> &before,const XeenCamera &c,const XeenConsequenceCharacters &p,
		const XeenConsequenceInputs &i,unsigned y,unsigned mask,const std::array<bool,6> &b) :
		characters(p),camera(c),inputs(i),year(y),participantMask(mask),blocked(b) {
	if (mask>0x3f) throw std::invalid_argument("Invalid regional participation mask");
	std::array<bool,107> tested{};
	actors=XeenActorApproach::move(before,c,[&](const XeenActor &a,int x,int z) {
		return xeenRegionalActorTerrain(map,a,x,z);
	},true,[&](const std::vector<XeenActor> &current,std::size_t index) {
		const auto &a=current[index];
		if (tested[index] || !a.activated || a.lifecycle!=XeenActorLifecycle::Present ||
			a.status!=XeenActorStatus::Physical || a.original.resourceId!=6 || !a.statistics || a.statistics->raw[32]!=1 ||
			(a.x==c.x && a.y==c.y) || (a.x!=c.x && a.y!=c.y)) return;
		tested[index]=true;
		if (!xeenOutdoorRangedRay(map,c,a)) return;
		auto &shot=shots.at(shotCount++);shot.source=a.id;shot.x=a.x;shot.y=a.y;
		shot.distance=unsigned(std::abs(a.x-c.x)+std::abs(a.y-c.y));
		shot.direction=a.x>c.x ? XeenDirection::East : a.x<c.x ? XeenDirection::West : a.y>c.y ? XeenDirection::North : XeenDirection::South;
	});
}
bool XeenRegionalOpportunityCandidate::service(XeenConsequenceDraw &draw) {
	while (cursor<shotCount && draw.remaining) {
		auto &shot=shots[cursor];
		const auto &source=actors.at(shot.source.recordIndex);
		if (!(source.id==shot.source) || !source.statistics) throw std::invalid_argument("Ranged source identity changed");
		if (!attack) attack.emplace(characters,inputs,*source.statistics,year,participantMask,blocked);
		if (!attack->service(draw)) return false;
		characters.swap(attack->characters);
		shot.attack=attack->result;shot.attack.actingMonster=shot.source;shot.attack.monster=shot.source;
		attack.reset();++cursor;
	}
	if (cursor<shotCount) return false;
	view=XeenActorApproach::classify(actors,camera);
	for (unsigned i=0;i<actors.size();++i) actors[i].activated=actors[i].activated || view.activation[i];
	return true;
}
}