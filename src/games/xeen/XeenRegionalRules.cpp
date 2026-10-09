#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenTraining.h"
#include <stdexcept>
#include <limits>
#include "formats/xeen/XeenMapFormat.h"
#include "formats/xeen/XeenEventFormat.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenEventDecoder.h"

// Adapted from the ScummVM developers' GPL-3.0-or-later Xeen combat.cpp
// at 6814ee9ba54582f5b5adcffab49efbbd8f589edd: canMonsterMove/stopAttack.
namespace mmodern {
namespace {
std::vector<XeenActor> regionalMove(const std::vector<XeenActor> &before,const XeenCamera &camera,
 const XeenActorApproach::Terrain &terrain,const XeenActorApproach::BeforeMovement &observe,
 XeenActorOpportunityContext context) {
 // Keep ordinary movement's existing external entry and replay probe. An
 // intra-translation-unit call from the new overload cannot be linker-wrapped.
 if(!context.sleeping && context.movementEnabled && !context.charactersShooting)
  return XeenActorApproach::move(before,camera,terrain,true,observe);
 return XeenActorApproach::move(before,camera,terrain,true,observe,context);
}
}
std::optional<std::size_t> xeenRegionalEvent(const XeenEventFile &events,const XeenCamera &camera) {
	if (!events.resourcePresent || (events.mapId!=XeenMapIdentity(23) && events.mapId!=XeenMapIdentity(28)) || camera.mapId!=events.mapId || static_cast<unsigned>(camera.direction)>3)
		throw std::invalid_argument("Invalid regional event lookup");
	for (std::size_t i=0;i<events.records.size();++i) {
		const auto &r=events.records[i];
		if (r.x==camera.x && r.y==camera.y && !r.line && (r.direction==4 || r.direction==unsigned(camera.direction))) return i;
	}
	return {};
}
bool xeenRegionalSign(const XeenEventFile &events,const XeenCamera &camera) {
	const auto record=xeenRegionalEvent(events,camera);
	if (!record || camera.x!=5 || camera.y!=9 || camera.direction!=XeenDirection::North) return false;
	const auto &r=events.records[*record];
	if (r.lengthField!=6 || r.line!=0 || r.opcode!=4 || r.parameters!=std::vector<std::uint8_t>{16}) return false;
	for (const auto &next:events.records) if (next.x==r.x && next.y==r.y && next.line==1 && (next.direction==unsigned(camera.direction) || next.direction==4)) return false;
	return true;
}
void xeenValidateRegionalManifest(const XeenMap &map, const XeenObjectFile &objects, const XeenEventFile &events,
		const std::vector<XeenMonsterRecord> &statistics, const std::vector<std::uint8_t> &dat,
		const std::vector<std::uint8_t> &mob, const std::vector<std::uint8_t> &evt) {
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
	if (map.identity()!=XeenMapIdentity(23) || objects.entities.monsters.empty() ||
		objects.entities.monsters.size()>XeenActorApproach::kCapacity)
		throw std::invalid_argument("Regional resource topology is unsupported");
	for (const auto &monster:objects.entities.monsters) {
		if (!monster.hasResource() || unsigned(monster.resourceId)>=statistics.size() ||
			!statistics[monster.resourceId].supportsGroundMovement() ||
			!statistics[monster.resourceId].supportsAdmittedMechanicsRendering())
			throw std::invalid_argument("Regional monster mechanics are unsupported");
		statistics[monster.resourceId].validateAttackCapabilities();
	}
	for(const auto &r:events.records) if(r.direction>4)
		throw std::invalid_argument("Regional event direction is unsupported");

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
  const auto *l=xeenGetIf<XeenOutdoorLayers>(&map.geometry.cells[y*16+x].geometry);
  if(!l) throw std::invalid_argument("Shoot requires outdoor geometry");
  const auto m=l->middle;
  if(m==1 || m==3 || m==6 || m==7 || m==9 || m==10 || m==12) return row;
 }
 return 4;
}
unsigned xeenPlayerRayRows(XeenWorld &world,const XeenCamera &camera) {
 const auto &map=world.map(camera.mapId);
 if(map.geometry.isOutdoors())return xeenPlayerRayRows(map,camera);
 const auto samples=XeenIndoorScene().sampleWalls(world,camera);
 constexpr unsigned queries[]{2,7,14};
 for(unsigned row=0;row<3;++row) {
  const auto &wall=samples[queries[row]];
  if(!wall.wallValue)return row+1;
  // mazeData() is the primary active map; getCell only resolves the wall tile.
  if(*wall.wallValue>=map.geometry.difficulties[0])return row+1;
 }
 return 4;
}
bool xeenIndoorRangedRay(XeenWorld &world,const XeenCamera &camera,const XeenActor &actor) {
 if(camera.mapId!=actor.id.mapId || world.map(camera.mapId).geometry.isOutdoors() ||
    unsigned(camera.direction)>3 || camera.x<0 || camera.x>=32 || camera.y<0 || camera.y>=32 ||
    !world.sampleCell(camera.mapId,camera.x,camera.y) ||
    actor.x<0 || actor.x>=32 || actor.y<0 || actor.y>=32)
  throw std::invalid_argument("Invalid indoor ranged geometry");
 if(actor.x!=camera.x && actor.y!=camera.y)return false;
 const int dx=(actor.x>camera.x)-(actor.x<camera.x),dy=(actor.y>camera.y)-(actor.y<camera.y);
 // stopAttack samples destination cells, with asymmetric directional masks.
 const unsigned mask=dx>0?0x8:dx<0?0x800:dy<0?0x8000:0x80;
 int x=camera.x,y=camera.y;
 while(x!=actor.x || y!=actor.y) {
  x+=dx;y+=dy;
  const auto cell=world.sampleCell(camera.mapId,x,y);
  if(!cell)return false;
  if(cell->geometry->isOutdoors() || !xeenHolds<XeenIndoorWalls>(cell->cell->geometry))
   throw std::invalid_argument("Invalid indoor ray cell");
  if(cell->cell->rawWord&mask)return false;
 }
 return true;
}
XeenMonsterTerrain xeenRegionalActorTerrain(const XeenMap &map, const XeenActor &a, int x, int y) {
	if (!local(x,y) || map.side!=XeenSide::Clouds || !map.geometry.isOutdoors() ||
		map.identity()!=a.id.mapId || !a.statistics || !a.statistics->supportsGroundMovement() || a.original.resourceId==59)
		return XeenMonsterTerrain::Unsupported;
	const auto *l=xeenGetIf<XeenOutdoorLayers>(&map.geometry.cells[y*16+x].geometry);
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
XeenMonsterTerrain xeenIndoorActorTerrain(XeenWorld &world,const XeenActor &a,int x,int y) {
	if(!a.statistics || !a.statistics->supportsGroundMovement() ||
		a.original.resourceId==59)return XeenMonsterTerrain::Unsupported;
	if(a.statistics->raw[32]) {
		try {a.statistics->validateAttackCapabilities();}
		catch(const std::invalid_argument &) {return XeenMonsterTerrain::Unsupported;}
	}
	if(x<0 || x>=32 || y<0 || y>=32)return XeenMonsterTerrain::Blocked;
	const int dx=x-a.x,dy=y-a.y;
	if(std::abs(dx)+std::abs(dy)>1)return XeenMonsterTerrain::Unsupported;
	if(!dx && !dy)return XeenMonsterTerrain::Allowed;
	const auto cell=world.sampleCell(a.id.mapId,a.x,a.y);
	if(!cell || !cell->geometry || !xeenHolds<XeenIndoorWalls>(cell->cell->geometry))
		return XeenMonsterTerrain::Unsupported;
	const auto direction=dx>0?XeenDirection::East:dx<0?XeenDirection::West:
		dy>0?XeenDirection::North:XeenDirection::South;
	return wallAt(*cell->cell,direction)<=world.map(a.id.mapId).geometry.difficulties[0] ?
		XeenMonsterTerrain::Allowed:XeenMonsterTerrain::Blocked;
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
		const auto *l=xeenGetIf<XeenOutdoorLayers>(&cell.geometry);
		if (!l) throw std::invalid_argument("Invalid outdoor ray cell");
		if (dx>0 ? (cell.rawWord&8)!=0 : !rayMiddle(l->middle)) return false;
	} while (x!=a.x || y!=a.y);
	return true;
}
void xeenValidateRegionalActors(const XeenMap &map, const XeenObjectFile &mob, const std::vector<XeenActor> &actors,
		const std::set<XeenMonsterIdentity> &accounted, const std::vector<XeenMonsterRecord> &statistics) {
	if (map.identity()!=XeenMapIdentity(23) || mob.mapId!=map.identity() || !mob.resourcePresent ||
		actors.empty() || actors.size()>XeenActorApproach::kCapacity || actors.size()!=mob.entities.monsters.size()) throw std::invalid_argument("Incomplete regional actor collection");
	for (auto id:accounted) if (id.mapId!=map.identity() || id.recordIndex>=actors.size())
		throw std::invalid_argument("Invalid regional accounting identity");
	for (unsigned i=0;i<actors.size();++i) {
		const auto &a=actors[i];const auto &original=mob.entities.monsters[i];
		if (!(a.id==XeenMonsterIdentity{23,i}) || a.original.x!=original.x || a.original.y!=original.y ||
			a.original.tableIndex!=original.tableIndex || a.original.direction!=original.direction || a.original.resourceId!=original.resourceId ||
			!a.statistics || !a.statistics->supportsGroundMovement() || !a.statistics->supportsAdmittedMechanicsRendering() || a.status!=XeenActorStatus::Physical)
			throw std::invalid_argument("Regional actor immutable identity/profile changed");
		const auto type=a.original.resourceId;
		if (!a.original.hasResource() || unsigned(type)>=statistics.size() || a.statistics->raw!=statistics[type].raw)
			throw std::invalid_argument("Changed regional statistics source");
		if (a.lifecycle==XeenActorLifecycle::Defeated) {
			if (a.hp || a.x!=-128 || a.y!=-128 || a.activated || !accounted.count(a.id))
				throw std::invalid_argument("Noncanonical regional defeated actor");
		} else if (a.lifecycle!=XeenActorLifecycle::Present || a.hp<1 || a.hp>a.statistics->baseHp() ||
			// Sleeping movement can leave an unseen actor away from its spawn.
			// Immutable identity, HP, accounting, occupancy and terrain closure
			// still validate it at every publication and restore boundary.
			accounted.count(a.id) || !local(a.x,a.y) ||
			!xeenActorClosure(map,a)[a.y*16+a.x]) throw std::invalid_argument("Invalid regional live actor value");
	}
	for (auto count:XeenActorApproach::occupancy(actors)) if (count>3)
		throw std::invalid_argument("Regional occupancy exceeded");
}
}

namespace mmodern {
XeenRegionalOpportunityCandidate::XeenRegionalOpportunityCandidate(XeenWorld &world,
		const std::vector<XeenActor> &before,const XeenCamera &c,const XeenConsequenceCharacters &p,
		const XeenConsequenceInputs &i,unsigned y,unsigned mask,const std::array<bool,6> &b,XeenActorOpportunityContext context) :
		characters(p),camera(c),inputs(i),year(y),participantMask(mask),blocked(b),indoorWorld(&world) {
	if(world.map(c.mapId).geometry.isOutdoors() || mask>0x3f)
		throw std::invalid_argument("Invalid indoor opportunity");
	std::array<bool,107> tested{};
	actors=regionalMove(before,c,[&](const XeenActor &a,int x,int z) {
		return xeenIndoorActorTerrain(world,a,x,z);
	},[&](const std::vector<XeenActor> &current,std::size_t index) {
		const auto &a=current[index];
		if(tested[index] || !a.statistics || !a.statistics->raw[32])return;
		tested[index]=true;
		a.statistics->validateAttackCapabilities();
		if(a.lifecycle!=XeenActorLifecycle::Present || a.status!=XeenActorStatus::Physical ||
			(a.x==c.x && a.y==c.y) || (a.x!=c.x && a.y!=c.y))return;
		const auto contacts=XeenIndoorScene().classifyActors(world,c,current);
		for(unsigned i=0;i<3;++i)if(contacts.slots[i]==a.id)return;
		if(!xeenIndoorRangedRay(world,c,a))return;
		if(shotCount==shots.size())return; // Original _gmonHit has 36 entries.
		auto &shot=shots[shotCount++];shot.source=a.id;shot.x=a.x;shot.y=a.y;
		shot.distance=unsigned(std::abs(a.x-c.x)+std::abs(a.y-c.y));
		shot.direction=a.x>c.x?XeenDirection::East:a.x<c.x?XeenDirection::West:
			a.y>c.y?XeenDirection::North:XeenDirection::South;
	},context);
}
XeenRegionalOpportunityCandidate::XeenRegionalOpportunityCandidate(const XeenMap &map,
		const std::vector<XeenActor> &before,const XeenCamera &c,const XeenConsequenceCharacters &p,
		const XeenConsequenceInputs &i,unsigned y,unsigned mask,const std::array<bool,6> &b,XeenActorOpportunityContext context) :
		characters(p),camera(c),inputs(i),year(y),participantMask(mask),blocked(b) {
	if (mask>0x3f) throw std::invalid_argument("Invalid regional participation mask");
	std::array<bool,107> tested{};
	actors=regionalMove(before,c,[&](const XeenActor &a,int x,int z) {
		return xeenRegionalActorTerrain(map,a,x,z);
	},[&](const std::vector<XeenActor> &current,std::size_t index) {
		const auto &a=current[index];
		if (tested[index] || (!a.activated && !context.sleeping) || a.lifecycle!=XeenActorLifecycle::Present ||
			a.status!=XeenActorStatus::Physical || !a.statistics || !a.statistics->raw[32] ||
			(a.x==c.x && a.y==c.y) || (a.x!=c.x && a.y!=c.y)) return;
		tested[index]=true;
		a.statistics->validateAttackCapabilities();
		if (!xeenOutdoorRangedRay(map,c,a)) return;
		auto &shot=shots.at(shotCount++);shot.source=a.id;shot.x=a.x;shot.y=a.y;
		shot.distance=unsigned(std::abs(a.x-c.x)+std::abs(a.y-c.y));
		shot.direction=a.x>c.x ? XeenDirection::East : a.x<c.x ? XeenDirection::West : a.y>c.y ? XeenDirection::North : XeenDirection::South;
	},context);
}
bool XeenRegionalOpportunityCandidate::service(XeenConsequenceDraw &draw) {
	if(staged && shotCount && !travelPresented) {travelStarted=true;return false;}
	impactSource.reset();impactOwner.reset();impactApplied=false;
	while (cursor<shotCount && draw.remaining) {
		auto &shot=shots[cursor];
		const auto &source=actors.at(shot.source.recordIndex);
		if (!(source.id==shot.source) || !source.statistics) throw std::invalid_argument("Ranged source identity changed");
		if (!attack) attack.emplace(characters,inputs,*source.statistics,year,participantMask,blocked);
		attack->deferInjury=staged;
        if(staged && impactPresented) {attack->injuryAcknowledged=true;impactPresented=false;}
        if (!attack->service(draw)) {
            if(attack->injuryReady) {impactSource=shot.source;impactOwner=attack->impactOwner;}
            if(attack->injuryApplied) {characters=attack->characters;attack->injuryApplied=false;impactApplied=true;portraitPublished=false;}
            return false;
        }
        const auto &part=attack->result;
        // doMonsterTurn returns MODE_INTERACTIVE when a non-party attack has
        // no able target. A completed injury alone does not change sleeping.
        noTargets=noTargets || (source.statistics->attacks() && !part.targetedMembers);
        std::vector<XeenCombatDamage> overflow;
        if(shot.attack.additionalInjuries)overflow=*shot.attack.additionalInjuries;
        for(unsigned i=0;i<part.injuryCount;++i) {
            if(shot.attack.injuryCount<shot.attack.injuries.size())shot.attack.injuries[shot.attack.injuryCount]=part.injury(i);
            else overflow.push_back(part.injury(i));
            ++shot.attack.injuryCount;
        }
        if(!overflow.empty())shot.attack.additionalInjuries=std::make_shared<const std::vector<XeenCombatDamage>>(std::move(overflow));
        const auto damage=std::int64_t(shot.attack.damage)+part.damage;
        if(damage>std::numeric_limits<int>::max())throw std::overflow_error("Ranged source damage overflow");
        shot.attack.damage=static_cast<int>(damage);
        shot.attack.targetedMembers|=part.targetedMembers;shot.attack.targetOwner=part.targetOwner;
        shot.attack.critical=shot.attack.critical || part.critical;
        shot.attack.actingMonster=shot.source;shot.attack.monster=shot.source;
        shot.attack.operation=part.operation;shot.attack.attackOutcome=part.attackOutcome;
        for(unsigned i=0;i<part.armorCount;++i) {
            const auto &change=part.armor[i];bool found=false;
            for(unsigned j=0;j<shot.attack.armorCount;++j)if(shot.attack.armor[j].owner==change.owner && shot.attack.armor[j].slot==change.slot) {shot.attack.armor[j].after=change.after;found=true;break;}
            if(!found)shot.attack.armor.at(shot.attack.armorCount++)=change;
        }
        characters=attack->characters;
        if(attack->nextAttack())continue;
		shot.attack.attackOutcome=!participantMask ? XeenCombatAttackOutcome::NoParticipants :
            !shot.attack.injuryCount ? XeenCombatAttackOutcome::Miss : shot.attack.damage ?
            XeenCombatAttackOutcome::HitPositiveDamage : XeenCombatAttackOutcome::HitZeroDamage;
		attack.reset();++cursor;
	}
	if (cursor<shotCount) return false;
	view=indoorWorld ? XeenIndoorScene().classifyActors(*indoorWorld,camera,actors) :
		XeenActorApproach::classify(actors,camera);
	for (unsigned i=0;i<actors.size();++i) actors[i].activated=actors[i].activated || view.activation[i];
	return true;
}
std::shared_ptr<const XeenRegionalObservation> XeenRegionalOpportunityCandidate::presentation() const {
    auto result=std::make_shared<XeenRegionalObservation>();
    if(!travelPresented) {
        result->stage=XeenRegionalObservation::Stage::Travel;result->count=shotCount;
        for(unsigned i=0;i<shotCount;++i)result->shots[i]=shots[i];
    }else {result->stage=XeenRegionalObservation::Stage::Portrait;result->impactSource=impactSource;result->impactOwner=impactOwner;}
    return result;
}
}
namespace mmodern {
std::optional<std::int16_t> xeenWellHpAfter(std::int16_t before) noexcept {
	const int after=int(before)+25;
	if(after>std::numeric_limits<std::int16_t>::max())return {};
	return static_cast<std::int16_t>(after);
}
std::optional<std::uint8_t> xeenRegionalService(const XeenEventFile &events,const XeenCamera &camera) {
	const auto first=xeenRegionalEvent(events,camera);
	if(!first || events.mapId!=camera.mapId || camera.mapId!=XeenMapIdentity(28))return {};
	const auto &r=events.records[*first];
	if(r.opcode!=0x11 || r.line!=0 || r.parameters.size()!=1)return {};
	const auto action=r.parameters[0];
	if(action!=1 && action!=4 && action!=5)return {};
	return action;
}
XeenRegionalInteraction xeenRegionalInteraction(const XeenEventFile &events,const XeenCamera &camera) {
	const auto first=xeenRegionalEvent(events,camera);
	if (!first) return XeenRegionalInteraction::None;
	if(const auto service=xeenRegionalService(events,camera)) {
		return *service==1?XeenRegionalInteraction::Ironworks:
			*service==4?XeenRegionalInteraction::Temple:XeenRegionalInteraction::Training;
	}
	// Classify supported region transitions from original operands at this
	// physical interaction. Neighbor geometry tiles are never destinations.
	const auto teleportsTo=[&](unsigned map) {
		for(const auto &r:events.records)
			if(r.x==camera.x && r.y==camera.y && (r.direction==4 || r.direction==unsigned(camera.direction)) &&
				r.opcode==0x07 && r.parameters.size()==3 && r.parameters[0]==map)return true;
		return false;
	};
	if(camera.mapId==XeenMapIdentity(28))return teleportsTo(23)?XeenRegionalInteraction::VertigoExit:XeenRegionalInteraction::Event;
	if(camera.mapId==XeenMapIdentity(23) && teleportsTo(28))return XeenRegionalInteraction::VertigoEntrance;
	if (xeenRegionalSign(events,camera)) return XeenRegionalInteraction::Sign;

	// Coordinates are the logical interaction identity; record ordering is transport data.
	// Validate semantic operations at that address, allowing presentation records before them.
	bool npc=false, grant=false, remove=false, hp=false, selection=false;
	for (std::size_t i=0;i<events.records.size();++i) {
		const auto &r=events.records[i];
		if(r.x!=camera.x || r.y!=camera.y || (r.direction!=4 && r.direction!=unsigned(camera.direction)))continue;
		const auto decoded=XeenEventDecoder::decode(r,{events.mapId,events.resourceName,i,true});
		const auto *instruction=std::get_if<XeenDecodedEventInstruction>(&decoded);if(!instruction)continue;
		npc=npc || std::holds_alternative<XeenEventNpc>(instruction->operation);
		remove=remove || std::holds_alternative<XeenEventRemove>(instruction->operation);
		selection=selection || std::holds_alternative<XeenEventWhoWill>(instruction->operation);
		if(const auto *effect=std::get_if<XeenEventTakeOrGive>(&instruction->operation)) {
			grant=grant || effect->second.mode==21;
			hp=hp || effect->second.mode==8;
		}
	}
	if(camera.x==9 && camera.y==11 && npc)return XeenRegionalInteraction::Myra;
	if(camera.x==8 && camera.y==2 && grant && remove)return XeenRegionalInteraction::Phirna;
	if(camera.x==7 && camera.y==7 && hp && selection)return XeenRegionalInteraction::Well;
	return XeenRegionalInteraction::None;
}
}
