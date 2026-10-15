// M68 Part A: DOS monster damage sequence, every secondary effect, the mixed
// ranged volley, the legacy medal and the developer fixture MOB. Pure rules
// with explicit draw tapes; no original data.
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenMonsterFixture.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenJourneyContent.h"
#include "formats/xeen/XeenMapFormat.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool v,const char *m) { if (!v) throw std::runtime_error(m); }
template<class F> void rejects(F f,const char *m) { bool refused=false;try { f(); } catch (const std::exception &) { refused=true; } check(refused,m); }
using Draw=XeenCombatRandom::Draw;
// Level-3 members, Luck 15 (+2): Physical save v = 5, interval 1..25.
XeenConsequenceCharacters characters() {
	XeenConsequenceCharacters result;
	for (unsigned i=0;i<6;++i) { auto &c=result[i];c.rosterId=kXeenCombatOwners[i];c.permanentLevel=3;c.birthYear=592;c.currentHp=50;c.currentSp=30;c.intellect=c.personality=c.endurance={15,0}; }
	result[0].characterClass=XeenCharacterClass::Cleric;
	return result;
}
XeenConsequenceInputs inputs(int fire=0) {
	XeenConsequenceInputs result;
	for (auto &i:result) { i.might=i.speed=i.accuracy={15,0};i.luck=XeenAttributeValue{15,0};i.resistances=XeenCombatResistances{};i.resistances->firePermanent=std::uint8_t(fire);i.poisonResistance=XeenAttributeValue{0,0}; }
	return result;
}
// One attack, one strike of d10, AC 5, hit parameter 5; hatred 3 prefers the Cleric.
XeenMonsterRecord profile(unsigned type,unsigned special,unsigned hates=3) {
	XeenMonsterRecord m;m.raw[20]=20;m.raw[24]=1;m.raw[22]=5;m.raw[25]=hates;m.raw[26]=1;m.raw[28]=10;m.raw[29]=type;m.raw[30]=special;m.raw[31]=5;return m;
}
template<class C> void finish(C &c,XeenCombatRandom &rng,unsigned budget=1) {
	for (unsigned service=0;service<1000;++service) { XeenConsequenceDraw draw{rng,budget,{}};if (c.service(draw)) return; }
	throw std::runtime_error("Candidate did not complete bounded service");
}
XeenEnemyAttackCandidate attack(const XeenMonsterRecord &m,std::vector<Draw> tape,const XeenConsequenceCharacters &c,const XeenConsequenceInputs &i,unsigned *position=nullptr) {
	XeenCombatRandom rng(tape);XeenEnemyAttackCandidate candidate(c,i,m,610,0x3f);finish(candidate,rng);
	if (position) *position=unsigned(rng.position());
	return candidate;
}
void dosSequence() {
	const auto c=characters();const auto i=inputs();
	// A nonphysical attack skips the d20 and the hit parameter; the initial save
	// draws 1..25 (Physical selector), and the element loop draws once even at zero.
	unsigned position=0;
	auto zero=attack(profile(2,0),{{1,10,1},{1,25,5},{1,40,40}},c,i,&position);
	check(position==3 && zero.result.injuryCount==1 && zero.result.injuries[0].amount==0 && zero.characters[0].currentHp==50 &&
		zero.result.attackOutcome==XeenCombatAttackOutcome::HitZeroDamage,"Fire: no hit roll, Physical initial save, one loop draw at zero damage");
	// With fire resistance 10 the loop interval is 1..50; successes halve 8 -> 4 -> 2 -> 1 -> 0 and the loop stops at zero.
	auto loop=attack(profile(2,0),{{1,10,8},{1,25,25},{1,50,1},{1,50,1},{1,50,1},{1,50,1}},c,inputs(10),&position);
	check(position==6 && loop.result.injuries[0].amount==0 && loop.characters[0].currentHp==50,"Element loop halves while it succeeds and damage stays nonzero");
	auto partial=attack(profile(2,0),{{1,10,8},{1,25,25},{1,50,1},{1,50,50}},c,inputs(10),&position);
	check(position==4 && partial.result.injuries[0].amount==4 && partial.characters[0].currentHp==46,"Element loop stops at the first failed save");
	auto halvedTwice=attack(profile(2,0),{{1,10,8},{1,25,5},{1,50,10},{1,50,11}},c,inputs(10),&position);
	check(position==4 && halvedTwice.characters[0].currentHp==48,"Initial Physical save then one element success: 8 -> 4 -> 2");
	// Poison correction: the initial save is the Physical selector, not poison resistance.
	auto poison=attack(profile(5,0),{{1,10,6},{1,25,5},{1,40,40}},c,i,&position);
	check(position==3 && poison.characters[0].currentHp==47 && poison.characters[0].conditions[3]==0,"Poison-type attack: Physical initial save halves 6 to 3; no condition without a special");
	// Secondary save only for positive damage.
	auto noSecondary=attack(profile(2,5),{{1,10,1},{1,25,1},{1,40,40}},c,i,&position);
	check(position==3 && noSecondary.characters[0].conditions[3]==0 && noSecondary.characters[0].currentHp==50,"Zero damage skips the secondary save");
	auto secondary=attack(profile(2,5),{{1,10,4},{1,25,25},{1,40,40},{1,25,25}},c,i,&position);
	check(position==4 && secondary.characters[0].conditions[3]==1 && secondary.characters[0].currentHp==46,"Positive damage draws the secondary save; failure poisons");
	auto saved=attack(profile(2,5),{{1,10,4},{1,25,25},{1,40,40},{1,25,5}},c,i,&position);
	check(position==4 && saved.characters[0].conditions[3]==0 && saved.characters[0].currentHp==46,"A successful secondary save leaves the condition alone, damage applies");
	// Physical: natural 1 misses without further draws; natural 20 applies damage, then the parameter path.
	auto one=attack(profile(0,5),{{1,20,1}},c,i,&position);
	check(position==1 && !one.result.injuryCount,"Natural 1 ends the attack");
	auto twenty=attack(profile(0,5),{{1,20,20},{1,10,2},{1,25,25},{1,5,1},{1,10,3},{1,25,25}},c,i,&position);
	check(position==6 && twenty.result.injuryCount==2 && twenty.characters[0].currentHp==45 && twenty.characters[0].conditions[3]==2,"Natural 20 applies twice with a secondary save each");
	// A sleeping target skips the hit roll and wakes before the application.
	auto asleep=characters();asleep[0].conditions[8]=1;
	auto woke=attack(profile(0,0),{{1,10,7}},asleep,i,&position);
	check(position==1 && woke.characters[0].currentHp==43 && !woke.characters[0].conditions[8],"Sleeping target: direct application and wake");
	// Fresh dice per recipient: hatred 15, Cold, each member draws dice, the initial save and one element save.
	std::vector<Draw> party;for (unsigned n=0;n<6;++n) party.insert(party.end(),{{1,10,unsigned(n+1)},{1,25,25},{1,40,40}});
	auto all=attack(profile(4,0,15),party,c,i,&position);
	check(position==18 && all.result.injuryCount==6,"Whole-party nonphysical attack draws three per member");
	for (unsigned n=0;n<6;++n) check(all.characters[n].currentHp==50-int(n+1),"Each recipient keeps its own dice");
}
XeenConsequenceCharacters equipped() {
	auto c=characters();
	c[0].weapons[0]={0,1,0,1};c[0].weapons[1]={0,34,0,0};c[0].weapons[2]={0,2,0,0};
	c[0].armor[0]={0,1,0,3};c[0].accessories[0]={42,5,0,8};c[0].miscellaneous[0]={3,8,7,0};
	return c;
}
bool same(const XeenItem &a,unsigned material,unsigned id,unsigned state,unsigned frame) {
	return a.material==material && a.id==id && a.state==state && a.frame==frame;
}
void secondaryEffects() {
	const auto i=inputs();
	const std::vector<Draw> hit{{1,20,19},{1,5,5},{1,10,4},{1,25,25}};
	const auto apply=[&](unsigned special,const XeenConsequenceCharacters &c,unsigned *position=nullptr) { return attack(profile(0,special),hit,c,i,position); };
	unsigned position=0;
	// Condition specials increment their byte and saturate at 255.
	const std::pair<unsigned,unsigned> conditions[]{{5,3},{7,4},{8,5},{9,8},{13,0},{14,11},{15,12},{16,10},{18,2}};
	for (const auto &[special,condition]:conditions) {
		auto r=apply(special,characters(),&position);
		check(position==4 && r.characters[0].conditions[condition]==1 && r.characters[0].currentHp==46,"Condition special increments its byte");
		// Saturation (DOS inc/jnz/0xff) on the pure effect: a 255 counter also
		// changes stats, saves and targetability, so the candidate path is not used here.
		XeenCharacter full=characters()[0];full.conditions[condition]=255;
		xeenApplySpecialAttack(full,special);
		check(full.conditions[condition]==255,"Condition special saturates at 255");
	}
	// Specials with no DOS handler still draw the secondary save and change nothing.
	for (unsigned special:{1u,2u,3u,4u,6u,11u}) {
		auto r=apply(special,equipped(),&position);
		check(position==4 && r.characters[0].currentHp==46 && r.result.armorCount==0 && r.characters[0].currentSp==30,"No-effect special draws the save only");
		for (unsigned n=0;n<16;++n) check(r.characters[0].conditions[n]==0,"No-effect special sets no condition");
	}
	check(attack(profile(0,0),{{1,20,19},{1,5,5},{1,10,4}},characters(),i,&position).characters[0].currentHp==46 && position==3,"Special 0 draws no secondary save");
	// Drain SP.
	auto drained=apply(12,characters());
	check(drained.characters[0].currentSp==0 && drained.result.injuries[0].beforeSp==30 && drained.result.injuries[0].afterSp==0,"Drain SP zeroes SP and observes it");
	// Curse Item: every slot of every category, weapon ID 34 untouched, frames stay.
	auto cursed=apply(10,equipped());
	{
		const auto &c=cursed.characters[0];
		check(same(c.weapons[0],0,1,0x40,1) && same(c.weapons[1],0,34,0,0) && same(c.weapons[2],0,2,0x40,0) && same(c.weapons[3],0,0,0x40,0),"Curse Item weapons");
		check(same(c.armor[0],0,1,0x40,3) && same(c.armor[8],0,0,0x40,0) && same(c.accessories[0],42,5,0x40,8) && same(c.miscellaneous[0],3,8,0x47,0) && same(c.miscellaneous[5],0,0,0x40,0),"Curse Item other categories, frames kept");
		check(cursed.result.armorCount==35,"Curse Item observes 35 changed slots");
	}
	// Break Weapon: equipped, nonempty weapons below ID 34 break and unequip.
	auto broken=apply(17,equipped());
	{
		const auto &c=broken.characters[0];
		check(same(c.weapons[0],0,1,0x80,0) && same(c.weapons[1],0,34,0,0) && same(c.weapons[2],0,2,0,0) && same(c.armor[0],0,1,0,3),"Break Weapon unequips only the equipped ordinary weapon");
		check(broken.result.armorCount==1 && broken.result.armor[0].category==0 && broken.result.armor[0].slot==0 && broken.result.armor[0].after.frame==0,"Break Weapon observation");
	}
	auto slayer=equipped();slayer[0].weapons[1]={0,34,0,1};slayer[0].weapons[0].frame=0;
	check(same(apply(17,slayer).characters[0].weapons[1],0,34,0,1),"Break Weapon leaves the equipped Slayer Sword");
	// Eradicate: broken bit everywhere (empty slots too), weapon/armor/accessory frames
	// cleared except weapon ID 34, miscellaneous frames kept, HP zeroed before the subtraction.
	auto eradicated=apply(19,equipped());
	{
		const auto &c=eradicated.characters[0];const auto &injury=eradicated.result.injuries[0];
		check(c.conditions[15]==1 && c.currentHp==-4 && c.conditions[12]==1 && injury.beforeHp==50 && injury.afterHp==-4,"Eradicate zeroes positive HP before subtracting");
		check(same(c.weapons[0],0,1,0x80,0) && same(c.weapons[1],0,34,0,0) && same(c.weapons[2],0,2,0x80,0) && same(c.weapons[8],0,0,0x80,0),"Eradicate weapons");
		check(same(c.armor[0],0,1,0x80,0) && same(c.armor[3],0,0,0x80,0) && same(c.accessories[0],42,5,0x80,0) && same(c.miscellaneous[0],3,8,0x87,0) && same(c.miscellaneous[7],0,0,0x80,0),"Eradicate armor, accessories and miscellaneous");
		check(eradicated.result.armorCount==35 && injury.beforeAc==4 && injury.afterAc==2,"Eradicate observes every slot and the AC without the unequipped armor");
	}
	auto eradicatedSlayer=equipped();eradicatedSlayer[0].weapons[1]={0,34,0,1};eradicatedSlayer[0].weapons[0].frame=0;
	check(same(apply(19,eradicatedSlayer).characters[0].weapons[1],0,34,0,1),"Eradicate skips the Slayer Sword slot entirely");
	// Aging: byte increment, 255 wraps to 0.
	auto aged=apply(20,characters());
	check(aged.characters[0].temporaryAge==1 && aged.result.injuries[0].beforeAge==0 && aged.result.injuries[0].afterAge==1,"Aging increments the temporary age");
	auto old=characters();old[0].temporaryAge=255;
	check(apply(20,old).characters[0].temporaryAge==0,"Aging wraps the byte at 255");
	// Death and Stone: condition plus zero HP before the subtraction.
	auto dead=apply(21,characters());
	check(dead.characters[0].conditions[13]==1 && dead.characters[0].currentHp==-4,"Death zeroes HP then subtracts");
	auto stoned=apply(22,characters());
	check(stoned.characters[0].conditions[14]==1 && stoned.characters[0].currentHp==-4,"Stone zeroes HP then subtracts");
	// Sleep special on a sleeping target: wake, then the special re-sleeps.
	auto asleep=characters();asleep[0].conditions[8]=1;
	auto resleep=attack(profile(0,9),{{1,10,4},{1,25,25}},asleep,i,&position);
	check(position==2 && resleep.characters[0].conditions[8]==1 && resleep.characters[0].currentHp==46,"Sleep special after the wake");
}
void capabilities() {
	XeenMonsterRecord m=profile(0,0);
	for (unsigned type=0;type<=6;++type) for (unsigned special=0;special<=22;++special) for (unsigned ranged=0;ranged<=1;++ranged) {
		m.raw[29]=type;m.raw[30]=special;m.raw[32]=ranged;m.validateAttackCapabilities();
	}
	m=profile(0,0);m.raw[29]=7;rejects([&]{m.validateAttackCapabilities();},"damage type 7");
	m=profile(0,0);m.raw[30]=23;rejects([&]{m.validateAttackCapabilities();},"special 23");
	m=profile(0,0);m.raw[32]=2;rejects([&]{m.validateAttackCapabilities();},"ranged 2");
	m=profile(0,0);m.raw[28]=0;rejects([&]{m.validateAttackCapabilities();},"zero die");
	m=profile(0,0);m.raw[31]=0;rejects([&]{m.validateAttackCapabilities();},"physical zero hit parameter");
	m=profile(2,0);m.raw[31]=0;m.validateAttackCapabilities(); // nonphysical never draws the hit parameter
	m=profile(0,0);m.raw[24]=0;m.raw[26]=0;m.validateAttackCapabilities(); // no attacks: dice unused
}
void mixedVolley() {
	XeenMap map;map.geometry.id=23;map.geometry.flags2=0x8000;map.geometry.surfaceTypes[2]=1;
	for (auto &cell:map.geometry.cells) cell.geometry=XeenOutdoorLayers{2,0,0,0};
	const XeenCamera camera{23,8,8,XeenDirection::North};
	std::vector<XeenActor> actors;
	const auto source=[&](unsigned index,int x,int y,unsigned type) {
		XeenActor a;a.id={23,index};a.original.resourceId=6;a.x=x;a.y=y;a.original.x=x;a.original.y=y;
		a.hp=20;a.activated=true;a.lifecycle=XeenActorLifecycle::Present;
		a.statistics=profile(type,0);a.statistics->raw[32]=1;actors.push_back(a);
	};
	source(0,8,11,0);source(1,11,8,2);
	auto c=characters();const auto i=inputs();
	XeenRegionalOpportunityCandidate opportunity(map,actors,camera,c,i,610,0x3f);
	check(opportunity.shotCount==2 && opportunity.shots[0].source.recordIndex!=opportunity.shots[1].source.recordIndex,"Both ranged sources queue once");
	// The queue follows the approach scan order; each source then draws its own type's sequence.
	const std::vector<Draw> physical{{1,20,19},{1,5,5},{1,10,4}},fire{{1,10,8},{1,25,25},{1,40,40}};
	std::vector<Draw> tape;
	for (unsigned n=0;n<2;++n) { const auto &part=opportunity.shots[n].source.recordIndex==0 ? physical : fire;tape.insert(tape.end(),part.begin(),part.end()); }
	XeenCombatRandom rng(tape);
	finish(opportunity,rng);
	const unsigned physicalShot=opportunity.shots[0].source.recordIndex==0 ? 0 : 1;
	check(rng.position()==6 && opportunity.shots[physicalShot].attack.damage==4 && opportunity.shots[1-physicalShot].attack.damage==8 && opportunity.characters[0].currentHp==38,
		"Each source applies its own damage type in queue order: physical hit roll, Fire saves");
}
void legacyMedal() {
	XeenCharacter c;c.rosterId=1;c.characterClass=XeenCharacterClass::Knight;c.accessories[0]={42,5,0,8};
	xeenValidateCompletedEquipment(c);
	c.accessories[0].state=0x40;xeenValidateCompletedEquipment(c);
	for (unsigned state:{0x80u,0xc0u,0x01u}) { c.accessories[0].state=std::uint8_t(state);rejects([&]{xeenValidateCompletedEquipment(c);},"legacy medal admits only the plain and cursed states"); }
	c.accessories[0].state=0x40;c.rosterId=0;rejects([&]{xeenValidateCompletedEquipment(c);},"cursed medal in a ring frame is still refused on other members");
	c.rosterId=1;c.accessories[0].material=43;rejects([&]{xeenValidateCompletedEquipment(c);},"legacy medal identity unchanged");
}
std::vector<std::uint8_t> syntheticMob(std::vector<std::uint8_t> monsterTable,std::vector<std::array<std::uint8_t,4>> monsters) {
	std::vector<std::uint8_t> bytes(48,0xff);
	bytes[0]=7;
	for (unsigned n=0;n<16;++n) bytes[16+n]=monsterTable.at(n);
	const std::uint8_t object[]{9,11,0,0},terminator[]{0xff,0xff,0xff,0xff};
	bytes.insert(bytes.end(),object,object+4);bytes.insert(bytes.end(),terminator,terminator+4);
	for (const auto &m:monsters) bytes.insert(bytes.end(),m.begin(),m.end());
	bytes.insert(bytes.end(),terminator,terminator+4);
	bytes.insert(bytes.end(),terminator,terminator+4);
	return bytes;
}
void fixtureMob() {
	std::vector<XeenMonsterRecord> statistics(16);
	for (unsigned n=0;n<16;++n) { auto &r=statistics[n];r.raw[0]='A'+char(n);r.raw[20]=20;r.raw[24]=1;r.raw[25]=16;r.raw[26]=1;r.raw[28]=4;r.raw[31]=5;r.raw[47]=8; }
	statistics[12].raw[46]=1; // flyer
	statistics[13].raw[48]=1; // animation loop
	statistics[14].raw[30]=23; // unsupported special
	std::vector<std::uint8_t> table(16,0xff);table[0]=8;table[1]=3;
	const auto original=syntheticMob(table,{{1,1,0,0},{2,2,1,0},{3,3,0,0}});
	const auto parsed=XeenMapFormat::parseMob(original);
	check(parsed.monsters.size()==3 && parsed.monsters[1].resourceId==3,"synthetic MOB parses");
	XeenMonsterFixtureDescription description;
	const auto bytes=xeenMonsterFixtureMob(original,{9,1},statistics,&description);
	const auto fixture=XeenMapFormat::parseMob(bytes);
	check(fixture.monsters.size()==3 && fixture.monsters[1].resourceId==9 && fixture.monsters[0].resourceId==8 && fixture.monsters[2].resourceId==8 &&
		fixture.monsters[1].x==2 && fixture.monsters[1].y==2 && fixture.monsterTable[2]==9 && fixture.monsterTable[0]==8 && fixture.monsterTable[1]==3 &&
		description.name=="J" && description.x==2 && description.y==2 && description.tableIndex==2,"Fixture appends the species to a free trailing slot");
	const auto reuse=XeenMapFormat::parseMob(xeenMonsterFixtureMob(original,{3,0},statistics));
	check(reuse.monsters[0].resourceId==3 && reuse.monsterTable==parsed.monsterTable,"Fixture reuses an existing table entry");
	check(bytes.size()==original.size(),"Fixture keeps the MOB size");
	for (unsigned n=0;n<bytes.size();++n) if (n!=18 && n!=48+8+4*1+2) check(bytes[n]==original[n],"Only the table slot and the record's table index change");
	rejects([&]{xeenMonsterFixtureMob(original,{16,0},statistics);},"species outside the catalogue");
	rejects([&]{xeenMonsterFixtureMob(original,{12,0},statistics);},"flyer refused in Part A");
	rejects([&]{xeenMonsterFixtureMob(original,{13,0},statistics);},"animation loop refused by the mainland admission");
	rejects([&]{xeenMonsterFixtureMob(original,{14,0},statistics);},"unsupported attack refused");
	rejects([&]{xeenMonsterFixtureMob(original,{9,3},statistics);},"record outside the placements");
	const auto disabled=syntheticMob(table,{{1,1,0,0},{std::uint8_t(0x80),std::uint8_t(0x80),1,0}});
	rejects([&]{xeenMonsterFixtureMob(disabled,{9,1},statistics);},"disabled record refused");
	std::vector<std::uint8_t> full(16);for (unsigned n=0;n<16;++n) full[n]=std::uint8_t(n<9 ? n : 10);
	rejects([&]{xeenMonsterFixtureMob(syntheticMob(full,{{1,1,0,0}}),{9,0},statistics);},"full table refused");
	const auto fullReuse=XeenMapFormat::parseMob(xeenMonsterFixtureMob(syntheticMob(full,{{1,1,0,0}}),{10,0},statistics));
	check(fullReuse.monsters[0].resourceId==10 && fullReuse.monsters[0].tableIndex==9,"Full table still admits a present species");
	// Species 59 (Water Dragon) walks by the record's bytes but the runtime refuses
	// its movement until Part B: the fixture and the capability agree.
	std::vector<XeenMonsterRecord> sixty(60);
	for (auto &r:sixty) { r=statistics[0]; }
	check(xeenMonsterMovementSupported(sixty[59],58) && !xeenMonsterMovementSupported(sixty[59],59) && !xeenMonsterMovementSupported(statistics[12],5),"Movement capability: ground species yes, species 59 and flyers no");
	bool partB=false;
	try { xeenMonsterFixtureMob(original,{59,1,{}},sixty); } catch (const std::invalid_argument &e) { partB=std::string(e.what()).find("Part B")!=std::string::npos; }
	check(partB,"Species 59 refused by the fixture with the Part B reason");
}
// Synthetic F3: grass everywhere, water in column 0, a tree (middle 1) where a
// test wants a hidden line, objects and spawns as placed.
XeenMap fixtureMap() {
	XeenMap m;m.geometry.id=23;m.side=XeenSide::Clouds;m.geometry.flags2=0x8000;
	for (unsigned i=0;i<16;++i) m.geometry.surfaceTypes[i]=std::uint8_t(i);
	for (auto &c:m.geometry.cells) { c.rawWord=1;c.surfaceIndex=1;c.geometry=XeenOutdoorLayers{1,0,0,0}; }
	for (unsigned y=0;y<16;++y) { auto &c=m.geometry.cells[y*16];c.surfaceIndex=0;c.geometry=XeenOutdoorLayers{0,0,0,0}; }
	return m;
}
void tree(XeenMap &m,int x,int y) { m.geometry.cells[y*16+x].geometry=XeenOutdoorLayers{1,1,0,0}; }
void fixturePlacement() {
	auto map=fixtureMap();tree(map,6,9);map.geometry.cells[8*16+8].rawAttributes|=kXeenAutomaticEventFlag;
	XeenMapEntities e;
	e.monsters.push_back({6,11,0,0,8});e.monsters.push_back({12,11,0,0,8});e.monsters.push_back({-128,-128,0,0,8});
	e.objects.push_back({5,11,0,0,7});
	// Default: distance 3 first, the record nearest the entry (9,11) first (record 0
	// at (6,11) before record 1 at (12,11): 3 against 3, ties by index), facings
	// North, East, South, West. North is hidden by the tree at (6,9), East by the
	// object at (5,11); South is clear.
	const auto chosen=xeenMonsterFixturePlacement(map,e,{9,{},{}});
	check(chosen.record==0 && chosen.start.mapId==XeenMapIdentity(23) && chosen.start.x==6 && chosen.start.y==14 &&
		chosen.start.direction==XeenDirection::South && chosen.distance==3 && chosen.inView && chosen.monsterX==6 && chosen.monsterY==11,
		"Default placement: first clear line of sight at distance 3");
	const auto second=xeenMonsterFixturePlacement(map,e,{9,1,{}});
	check(second.record==1 && second.start.x==12 && second.start.y==8 && second.start.direction==XeenDirection::North && second.distance==3,
		"--record keeps the placement and chooses its own start");
	// Distance 2 is the fallback when no distance-3 start is legal: block every row-3 start.
	auto near=map;tree(near,6,14);tree(near,3,11);tree(near,8,11);tree(near,6,8);
	const auto two=xeenMonsterFixturePlacement(near,e,{9,0,{}});
	check(two.record==0 && two.distance==2 && two.start.x==6 && two.start.y==13 && two.start.direction==XeenDirection::South,"Distance 2 fallback");
	// --start: validated, then the record in view (or --record, or record 0 out of view).
	const auto south=xeenMonsterFixturePlacement(map,e,{9,{},XeenCamera{23,6,14,XeenDirection::South}});
	check(south.record==0 && south.inView && south.distance==3 && south.start.x==6 && south.start.y==14,"--start takes the record straight ahead");
	const auto away=xeenMonsterFixturePlacement(map,e,{9,{},XeenCamera{23,6,14,XeenDirection::North}});
	check(away.record==0 && !away.inView && away.distance==0,"--start with nothing ahead falls back to record 0 out of view");
	const auto fixed=xeenMonsterFixturePlacement(map,e,{9,1,XeenCamera{23,6,14,XeenDirection::South}});
	check(fixed.record==1 && !fixed.inView,"--start with --record keeps the record even out of view");
	const auto adjacent=xeenMonsterFixturePlacement(map,e,{9,{},XeenCamera{23,6,12,XeenDirection::South}});
	check(adjacent.record==0 && adjacent.inView && adjacent.distance==1,"--start one cell away still sees the record");
	// Refusals carry the reason.
	const auto refusal=[&](const XeenCamera &c) { const auto r=xeenMonsterFixtureStartRefusal(map,e,c);return r ? *r : std::string(); };
	check(refusal({23,0,5,XeenDirection::North}).find("not walkable")!=std::string::npos,"water start refused");
	check(refusal({23,6,11,XeenDirection::North}).find("monster spawns")!=std::string::npos,"spawn start refused");
	check(refusal({23,8,8,XeenDirection::North}).find("automatic Event")!=std::string::npos,"automatic Event start refused");
	check(refusal({28,6,14,XeenDirection::North}).find("map 23")!=std::string::npos,"other map refused");
	check(refusal({23,16,0,XeenDirection::North}).find("outside")!=std::string::npos,"out-of-range start refused");
	check(refusal({23,6,9,XeenDirection::North}).find("not walkable")!=std::string::npos,"tree start refused");
	check(refusal(xeenJourneyContent().entry).empty(),"the Journey entry is a legal start");
	rejects([&]{xeenMonsterFixturePlacement(map,e,{9,{},XeenCamera{23,0,5,XeenDirection::North}});},"placement refuses an illegal --start");
	rejects([&]{xeenMonsterFixturePlacement(map,e,{9,2,{}});},"disabled record refused");
	rejects([&]{xeenMonsterFixturePlacement(map,e,{9,3,{}});},"record outside the placements refused");
	// No clear line from any side: the choice fails with a reason instead of a hidden monster.
	auto boxed=fixtureMap();XeenMapEntities one;one.monsters.push_back({8,8,0,0,8});
	for (const auto cell:{std::pair<int,int>{8,9},{8,7},{7,8},{9,8}}) tree(boxed,cell.first,cell.second);
	rejects([&]{xeenMonsterFixturePlacement(boxed,one,{9,{},{}});},"no placement in view refused");
	rejects([&]{xeenMonsterFixtureMob(syntheticMob(std::vector<std::uint8_t>(16,0xff),{{1,1,0,0}}),{9,{},{}},std::vector<XeenMonsterRecord>(16));},"MOB builder needs a chosen record");
}
}
int main() {
	const std::pair<const char *,void(*)()> sections[]{{"dosSequence",dosSequence},{"secondaryEffects",secondaryEffects},{"capabilities",capabilities},
		{"mixedVolley",mixedVolley},{"legacyMedal",legacyMedal},{"fixtureMob",fixtureMob},{"fixturePlacement",fixturePlacement}};
	for (const auto &[name,run]:sections) {
		try { run(); } catch (const std::exception &e) { std::cerr<<name<<": "<<e.what()<<'\n';return 1; }
	}
	std::cout<<"Monster ability rules passed\n";return 0;
}
