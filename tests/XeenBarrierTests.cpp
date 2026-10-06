#include "games/xeen/XeenBarrierRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "XeenSaveTestSupport.h"
#include <iostream>
using namespace mmodern;
namespace {
void check(bool v,const char *m){if(!v)throw std::runtime_error(m);}
template<class F> void rejects(F f){bool bad=false;try{f();}catch(const std::exception &){bad=true;}check(bad,"Expected barrier refusal");}
XeenMap tile(XeenMapIdentity id) {
	XeenMap m;m.geometry.id=id.number;m.side=id.side;
	if(id==28)m.geometry.neighbors=std::array<std::uint16_t,4>{110,109,0,0};
	if(id==109)m.geometry.neighbors=std::array<std::uint16_t,4>{111,0,0,28};
	if(id==110)m.geometry.neighbors=std::array<std::uint16_t,4>{0,111,28,0};
	if(id==111)m.geometry.neighbors=std::array<std::uint16_t,4>{0,0,109,110};
	m.geometry.difficulties=std::array<int,8>{7,4,20,0,0,40,40,0};m.geometry.trapDamage=11;
	for(auto &cell:m.geometry.cells) {cell.geometry=XeenIndoorWalls{{9,9,9,9}};cell.rawWord=0x9999;cell.rawAttributes=1;cell.surfaceIndex=1;}
	return m;
}
struct Party {
	XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;XeenGameplayContext context;
	Party() {
		std::vector<std::uint8_t> bytes(30*XeenCharacter::kSerializedSize);
		for(unsigned n=0;n<30;++n) {auto *p=bytes.data()+n*XeenCharacter::kSerializedSize;
			for(unsigned a=0;a<7;++a)p[20+2*a]=15;
			p[35]=3;p[39]=1;p[342]=100;p[346]=592&255;p[347]=592>>8;
		}
		const auto roster=XeenCharacterFormat::parseRoster(bytes);
		for(unsigned n=0;n<6;++n){characters[n]=roster.at(n);inputs[n]=XeenCharacterFormat::parseCombatInputs(bytes,n,true,true,true);}
		context.year=610;context.day=8;context.minutes=480;
	}
};
void finish(XeenBarrierCandidate &c,XeenCombatRandom &rng) {
	for(unsigned n=0;n<200;++n){XeenConsequenceDraw draw{rng,1,{}};if(c.service(draw))return;}
	throw std::runtime_error("Barrier continuation did not finish");
}
void unlocks() {
	Party p;XeenWorld w(tile);
	for(unsigned type=0;type<7;++type) {
		XeenBarrierCandidate c(w,{28,15,15,XeenDirection::North},p.characters,p.inputs,{1,77,0},p.context,false);
		check(c.selection && c.handled,"Closed grate requires WhoWill");c.choose(0);
		std::vector<XeenCombatRandom::Draw> tape{{1,4,1},{0,6,type}};
		if(type)tape.push_back({1,40,40});tape.push_back({1,20,20});
		XeenCombatRandom rng(tape);finish(c,rng);
		check(c.opened && c.characters[0].currentHp==89 && c.inputs[0].experience==p.inputs[0].experience+60 &&
			rng.position()==tape.size() && p.characters[0].currentHp==100,"Trap must precede unlock XP with exact draws, detached injuries");
	}
	XeenBarrierCandidate fail(w,{28,2,2,XeenDirection::East},p.characters,p.inputs,{1,77,0},p.context,false);
	fail.characters[0].characterClass=XeenCharacterClass::Knight;fail.choose(0);
	XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,4,4},{1,20,1}});finish(fail,rng);
	check(!fail.opened && fail.characters[0].currentHp==100 && fail.inputs[0].experience==p.inputs[0].experience,"Failed roll has no opening/XP; no-trap branch has no damage draw");
	XeenBarrierCandidate staged(w,{28,2,2,XeenDirection::East},p.characters,p.inputs,{1,77,0},p.context,false);staged.deferInjury=true;staged.choose(0);
	XeenCombatRandom traps(std::vector<XeenCombatRandom::Draw>{{1,4,1},{0,6,0},{1,20,20}});
	XeenConsequenceDraw draw{traps};check(!staged.service(draw) && staged.injury->injuryReady && traps.position()==2 && staged.characters[0].currentHp==100,"Portrait must precede HP and Thievery draw");
	staged.injury->injuryAcknowledged=true;
	for(unsigned n=0;n<10 && !staged.damagePauseReady;++n) {XeenConsequenceDraw next{traps};check(!staged.service(next),"Trap must hold before the unlock roll");}
	check(staged.damagePauseReady && traps.position()==2 && staged.characters[0].currentHp==89,"Trap pause must retain injury without drawing unlock");
	staged.damagePauseAcknowledged=true;finish(staged,traps);
	check(staged.opened && traps.position()==3,"Acknowledged trap must not replay draws");
}
void bash() {
	Party p;p.characters[0].conditions[8]=1;p.characters[1].conditions[11]=1;XeenWorld w(tile);
	XeenWorld outdoor([](auto id){auto m=tile(id);m.geometry.flags2=0x8000;for(auto &cell:m.geometry.cells)cell.geometry=XeenOutdoorLayers{};return m;});
	XeenBarrierCandidate outdoors(outdoor,{23,2,2,XeenDirection::North},p.characters,p.inputs,{1,77,0},p.context,true);
	check(outdoors.handled && outdoors.done && !outdoors.portraitMask && !outdoors.opened && outdoors.characters[2].currentHp==100,"Outdoor Bash returns after its charge without injury or a roll");
	XeenBarrierCandidate c(w,{28,15,15,XeenDirection::East},p.characters,p.inputs,{1,77,0},p.context,true);
	XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{1,30,10}});finish(c,rng);
	check(c.opened && c.targetWall==3 && c.portraitMask==12 && c.characters[2].currentHp==98 && c.characters[3].currentHp==98 &&
		c.characters[0].currentHp==100 && rng.position()==1,"Bash uses first two eligible members, two HP and shared Might roll");
	XeenBarrierCandidate fail(w,{28,2,2,XeenDirection::North},p.characters,p.inputs,{1,77,0},p.context,true);
	XeenCombatRandom low(std::vector<XeenCombatRandom::Draw>{{1,30,1}});finish(fail,low);check(!fail.opened && fail.characters[2].currentHp==98,"Failed Bash still costs HP");
	for(unsigned wall:{7u,14u,15u}) {
		XeenWorld solid([&](auto id){auto m=tile(id);for(auto &cell:m.geometry.cells)xeenGet<XeenIndoorWalls>(cell.geometry).walls[0]=wall;return m;});
		XeenBarrierCandidate c(solid,{28,2,2,XeenDirection::North},p.characters,p.inputs,{1,77,0},p.context,true);
		XeenCombatRandom empty(std::vector<XeenCombatRandom::Draw>{});finish(c,empty);
		check(c.done && !c.opened && c.animation==wall && empty.position()==0 && c.characters[2].currentHp==98,"Solid wall Bash pays HP without roll or free passage");
	}
}
void overrides() {
	Party p;XeenWorld source(tile);source.markEncounterSession(XeenEncounterEntry::Journey);
	XeenWorld distinct([](auto id){auto m=tile(id);if(id==110){m.geometry.difficulties[2]=70;m.geometry.difficulties[5]=90;m.geometry.trapDamage=19;}return m;});
	XeenBarrierCandidate seam(distinct,{28,15,16,XeenDirection::North},p.characters,p.inputs,{1,77,0},p.context,false);
	XeenBarrierCandidate seamBash(distinct,{28,15,16,XeenDirection::North},p.characters,p.inputs,{1,77,0},p.context,true);
	check(seam.threshold==20 && seam.trapDamage==11 && seamBash.threshold==40,"Barrier difficulty/trap damage must follow primary map at tile seams");
	XeenWorld dark([](auto id){auto m=tile(XeenMapIdentity{std::uint16_t(id.number)});m.side=id.side;m.geometry.wallKind=id.number==28?2:0;
		for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{{6,6,6,6}};return m;});
	XeenBarrierCandidate blocked(dark,{{XeenSide::Darkside,28},15,16,XeenDirection::North},p.characters,p.inputs,{1,77,0},p.context,false);
	check(!blocked.handled,"Darkside wall-kind predicate must use the primary map, not the physical tile");
	for(unsigned direction=0;direction<4;++direction) {
		auto world=source.transitionCandidate();const XeenCamera camera{28,15,15,XeenDirection(direction)};
		world->setBarrier(camera,6,true);
		const auto &values=world->sessionState().barriers();check(values.size()==2,"Both physical cells must be retained");
		const auto sampled=world->sampleCell(28,15,15);
		check(wallAt(*sampled->cell,camera.direction)==6 && (sampled->cell->flags&0x80) &&
			wallAt(source.map(28).geometry.cells[255],camera.direction)==9,"Effective wall/flags must not mutate original resource");
		XeenWorld restored(tile);restored.restoreBarriers(values);auto next=camera;
		check(XeenMovement().apply(restored,next,NavigationAction::MoveForward)==XeenMovementResult::Moved,"Restored opening remains walkable at tile seams");
		auto snapshot=save_test::currentWireSnapshot();snapshot.barriers=values;
		snapshot.journey->vertigoActors.emplace();snapshot.journey->cityOriginalActorCount=0;
		check(XeenSaveFormat::decode(XeenSaveFormat::encode(snapshot)).barriers==values,"Save v5 preserves canonical override bytes");
		XeenPartyState party;XeenGameFlags flags;auto before=camera;XeenRestoreGuard guard(*world,party,before,flags);
		world->setBarrier(camera,9,true);check(!guard.current(),"Changed barrier invalidates retained preimage");
		world->setBarrier(camera,6,true);check(!guard.current(),"Equal values cannot revive stale barrier guard");
		XeenRestoreGuard direct(*world,party,before,flags);
		auto &face=const_cast<XeenBarrierOverride &>(world->sessionState().barriers()[0]);const auto mask=std::uint8_t(face.mask);
		face.mask=15;face.mask=mask;check(!direct.current(),"Tracked barrier field ABA must invalidate retained tickets");
		auto broken=values;broken.pop_back();XeenWorld bad(tile);rejects([&]{bad.restoreBarriers(broken);});
		broken=values;broken[0].originalWord^=1;rejects([&]{bad.restoreBarriers(broken);});
		const auto toggle=source.transitionCandidate();toggle->setBarrier(camera,6,true);
		XeenBarrierCandidate close(*toggle,camera,p.characters,p.inputs,{1,77,0},p.context,false);
		check(close.handled && close.done && close.opened && !close.selection && close.targetWall==9,"Open grate closes without selection or RNG");
		XeenBarrierCandidate reopen(*world,camera,p.characters,p.inputs,{1,77,0},p.context,false);
		check(reopen.handled && !reopen.selection,"Unlocked grate does not repeat trap/unlock");
		world->setBarrier(camera,3,false);
		check(xeenPlayerRayRows(*world,camera)>=2,"Bashed wall override still blocks player ray");
		constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
		XeenActor enemy;enemy.id={28,0};enemy.x=15+dx[direction];enemy.y=15+dy[direction];
		check(xeenIndoorRangedRay(*world,camera,enemy),"Bashed wall override still blocks enemy wall-bit ray");
		check(!unsupportedManualSpecialInteraction(*world->sampleCell(28,15,15)->cell,camera.direction),"Post-Bash Event lookup must use effective wall rather than original closed grate");
	}
}
}
int main(){try{unlocks();bash();overrides();std::cout<<"Barrier rules, typed traps, seams and v5 overrides passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
