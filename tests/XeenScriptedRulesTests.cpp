#include "games/xeen/XeenScriptedRules.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenItemRewards.h"
#include "games/xeen/XeenEventInterpreter.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class T> void finish(T &candidate,XeenCombatRandom &random) {
 for(unsigned n=0;n<1000;++n){XeenConsequenceDraw draw{random,1,{}};if(candidate.service(draw))return;}
 throw std::runtime_error("Scripted rule did not finish");
}
void damage() {
 check(xeenPhysicalAttackDamage(100,100,34,0,0)==100,"Slayer Sword bypass");
 check(xeenPhysicalAttackDamage(100,100,35,0,0)==0,"IDs above 34 must not bypass");
 check(xeenPhysicalAttackDamage(100,100,0,3,6)==100 && xeenPhysicalAttackDamage(100,100,0,0,6)==0,
  "The spell selector bypass requires range");
 XeenCharacter c;c.characterClass=XeenCharacterClass::Knight;c.permanentLevel=1;c.weapons[0]={0,1,5,1};
 XeenCombatInputs inputs;inputs.accuracy={100,0};inputs.might={15,0};
 XeenMonsterRecord monster;monster.raw[33]=0;
 XeenCombatRandom ordinary(7),lord(7);
 XeenPhysicalPlayerCandidate base(c,inputs,monster,88,610,false,XeenDifficulty::Warrior);
 XeenPhysicalPlayerCandidate bonus(c,inputs,monster,89,610,false,XeenDifficulty::Warrior);
 finish(base,ordinary);finish(bonus,lord);
 check(base.damage>0 && bonus.damage==base.damage+100 && ordinary.position()==lord.position(),"Lord bonus follows effectiveness without a draw");
 monster.raw[40]=100;c.weapons[0]={0,31,0,4};
 XeenCombatRandom normalShot(7),lordShot(7);
 XeenPhysicalPlayerCandidate shot(c,inputs,monster,88,610,true,XeenDifficulty::Warrior,XeenActorStatus::Sleep);
 XeenPhysicalPlayerCandidate refused(c,inputs,monster,89,610,true,XeenDifficulty::Warrior,XeenActorStatus::Sleep);
 finish(shot,normalShot);finish(refused,lordShot);
 check(refused.hit && !refused.damage && !refused.impact && refused.status==XeenActorStatus::Sleep && normalShot.position()==lordShot.position()+1,
  "DOS Lord Shoot zero exit removes exactly the save draw and does not wake");
 XeenCombatRandom darkRandom(7);
 XeenPhysicalPlayerCandidate dark(c,inputs,monster,89,610,true,XeenDifficulty::Warrior,XeenActorStatus::Sleep,false,false);
 finish(dark,darkRandom);check(darkRandom.position()==normalShot.position(),"Clouds immunity leaked to Darkside");
 c.weapons[0]={0,34,0,13};XeenCombatRandom swordRandom(7);
 XeenPhysicalPlayerCandidate sword(c,inputs,monster,89,610,false,XeenDifficulty::Warrior);finish(sword,swordRandom);
 check(sword.damage>100,"Sword must hurt Lord Xeen through physical immunity");
}
XeenPartyState party() {
 XeenPartyState p;p.party=XeenParty::fromRosterIds({5,2,7});
 for(auto id:p.party.activeRosterIds())p.roster.at(id).rosterId=id;
 p.roster.at(5).conditions[unsigned(XeenCondition::Dead)]=1;p.questFlags.set(23);return p;
}
void deaths() {
 auto p=party();XeenGameFlags flags;XeenActor actor;actor.id={41,28};XeenCamera camera{41,1,1,XeenDirection::North};
 auto wrong=actor;wrong.id.recordIndex=20;
 auto noHook=xeenPrepareScriptedDeath(p,flags,camera,wrong);check(!noHook.flags.isSet(11),"ScummVM's Tiger Mole slot must not complete the quest");
 xeenPrepareScriptedDeath(p,flags,camera,actor).publish(p,flags);check(flags.isSet(11),"Clan King flag");
 camera.mapId=78;actor.id={78,8};actor.original.resourceId=89;
 XeenCloudsQuestItems::Counts counts{};counts[8]=255;p.questItems=XeenCloudsQuestItems(counts);
 xeenPrepareScriptedDeath(p,flags,camera,actor).publish(p,flags);
 check(flags.isSet(60) && !p.questFlags.isSet(23) && p.questItems.at(8)==0,"Lord flags and byte-wrapping Scepter increment");
 for(auto id:p.party.activeRosterIds())check(p.roster.at(id).awards[42]==1,"Award 42 includes the dead first member");
 camera.mapId=29;actor.id={29,27};actor.original.resourceId=86;
 xeenPrepareScriptedDeath(p,flags,camera,actor).publish(p,flags);check(flags.isSet(104),"Draco flag");
 camera.mapId={XeenSide::Darkside,41};actor.id={camera.mapId,28};XeenGameFlags other;
 check(!xeenPrepareScriptedDeath(p,other,camera,actor).flags.isSet(11),"Clouds death hook leaked sides");
}
void scepter() {
 XeenTreasureStaging staged;staged.weapons[0]={1,2,3,4};staged.weapons[1]={2,3,4,0};
 XeenMonsterRecord monster;XeenMonsterDropCandidate drop(staged,monster,78,0,89);XeenCombatRandom random(7);finish(drop,random);
 check(drop.treasure.weapons[0].id==90 && !drop.treasure.weapons[0].material && !drop.treasure.weapons[0].state &&
  drop.treasure.weapons[0].frame==4 && drop.treasure.weapons[1].id==3 && drop.treasure.ready && random.position()==0,"Scepter overwrites only slot zero without a draw");
 auto p=party();XeenTreasureDelivery delivery;xeenBeginTreasureDelivery(delivery,drop.treasure,p,{});
 while(delivery.phase!=XeenRewardPhase::Receipt)xeenAdvanceTreasureDelivery(delivery,p,{});
 check(delivery.receipt.entries[0].quest && delivery.receipt.entries[0].owner==5,"Ordinary delivery credits the dead first member");
 XeenMonsterDropCandidate arena(staged,monster,85,0,89);check(arena.treasure==staged,"Arena must not stage a Scepter");
 // Search deterministic seeds for an ordinary weapon drop; staging the Scepter
 // must leave that kill's requests unchanged and put the random weapon after it.
 monster.raw[45]=4;bool weapon=false;
 for(unsigned seed=1;seed<=100 && !weapon;++seed) {
  XeenCombatRandom a(seed),b(seed);XeenMonsterDropCandidate normal(XeenTreasureStaging{},monster,78,0,88),boss(XeenTreasureStaging{},monster,78,0,89);
  finish(normal,a);finish(boss,b);
  check(a.position()==b.position(),"Scepter staging changed ordinary drop requests");
  if(normal.outcome==XeenMonsterDropOutcome::Item && normal.category==XeenRewardCategory::Weapons) {
   check(boss.treasure.weapons[0].id==90 && boss.treasure.weapons[1].id==normal.generated.id,"Ordinary drop follows the Scepter");weapon=true;
  }
 }
 check(weapon,"No deterministic ordinary weapon-drop witness");
}
void trolls() {
 std::vector<XeenActor> actors(3);XeenMonsterRecord monster;monster.raw[20]=90;
 for(auto &a:actors){a.lifecycle=XeenActorLifecycle::Present;a.original.resourceId=53;a.statistics=monster;a.hp=2;}
 actors[1].original.resourceId=52;actors[2].lifecycle=XeenActorLifecycle::Defeated;actors[2].hp=0;
 xeenRegenerateIceTrolls(actors,{XeenSide::Darkside,8});check(actors[0].hp==2,"Troll side test");
 xeenRegenerateIceTrolls(actors,8);check(actors[0].hp==90 && actors[1].hp==2 && !actors[2].hp,"Only present Ice Trolls regenerate");
}
void maps() {
 XeenGameFlags flags;for(unsigned n:{63,39,64,65,66,67,68,69,49,43,25})flags.set(n);
 const auto view=xeenMapRuleView(flags);const unsigned maps[]{7,4,12,11,15,10,16,15,19},slots[]{5,6,11,10,11,9,9,14,14};
 XeenMapEntity original;original.x=original.y=10;original.resourceId=118;
 for(unsigned n=0;n<9;++n){const auto object=xeenMapRuleObject(original,{maps[n],slots[n]},std::uint16_t(1u<<n));check(object.x==-128 && object.y==-128,"Quest-done object table");}
 check(xeenMapRuleObject(original,{15,11},view).x==-128 && xeenMapRuleObject(original,{15,14},view).x==10,"Flag 66 suppresses flag 69 on map 15");
 check(xeenMapRuleObject(original,{15,14},std::uint16_t(view&~(1u<<4))).x==-128,"Flag 69 applies when flag 66 is clear");
 check(xeenMapRuleObject(original,{12,0},view).resourceId==85 && xeenMapRuleObject(original,{12,0},0).resourceId==118,"Castle sprite flag");
 check(xeenMapRuleObject(original,{{XeenSide::Darkside,12},0},view).resourceId==118,"Castle sprite side");
 for(unsigned n:{42,43,44})check(xeenDrainedCave(n,view),"Cave drain table");
 check(!xeenDrainedCave(4,view) && !xeenDrainedCave(42,0),"Cave drain must exclude map 4 and require flag 25");
 XeenWorld world([](auto id){XeenMap m;m.side=id.side;m.geometry.id=id.number;for(auto &c:m.geometry.cells){c.rawAttributes=0x5f;c.surfaceIndex=7;c.geometry=XeenIndoorWalls{};}return m;},
  [](auto id){XeenObjectFile file{id,"synthetic.mob",true,{}};XeenMapEntity object;object.resourceId=118;file.entities.objects.push_back(object);return file;});
 world.applyMainMapRules(42,flags);const auto cell=world.sampleCell(42,2,2);
 check(cell && cell->cell->rawAttributes==0x58 && !cell->cell->surfaceIndex && world.map(42).geometry.cells[34].rawAttributes==0x5f,"Drain clears only effective low bits, retaining immutable resources");
 world.applyMainMapRules(12,flags);check(world.effectiveObject({12,0}).resourceId==85 && world.objectFile(12).entities.objects[0].resourceId==118 && !world.sessionState().mapState(12),
  "Castle view must derive from the flag without a saved object override");
 static_cast<void>(world.map(50));check(!world.sessionState().mapState(50),"A resource-only neighbour read must not apply main-map rules");
 world.applyMainMapRules(50,flags);const auto *state=world.sessionState().mapState(50);
 check(!state || !state->explored,"Clouds map 50 must stay unstepped at load");
 XeenExploredTile visited{50,{}};visited.cells[3]=4;world.restoreExploredTiles({visited});
 world.applyMainMapRules(50,flags);check(world.sessionState().exploredTiles()==std::vector<XeenExploredTile>{visited},"Map 50 must preserve exactly the existing stepped bits");
 std::vector<XeenActor> dragons(3);for(auto &a:dragons)a.x=a.y=-128;
 check(xeenLakeDragonsGone(dragons),"All three off-map dragon slots");dragons[2].x=dragons[2].y=5;
 check(!xeenLakeDragonsGone(dragons),"One remaining dragon blocks completion");
}
void ending() {
 XeenEventRecord record;record.x=record.y=1;record.direction=4;record.opcode=30;record.lengthField=5;
 const auto decoded=XeenEventDecoder::decode(record);
 check(std::holds_alternative<XeenDecodedEventInstruction>(decoded) &&
  std::holds_alternative<XeenEventEndClouds>(std::get<XeenDecodedEventInstruction>(decoded).operation),"Opcode 30 is known");
 XeenWorld world([](auto id){XeenMap m;m.side=id.side;m.geometry.id=id.number;return m;});
 auto p=party();const XeenCamera camera{1,1,1,XeenDirection::North};
 const auto scripts=[&](auto id){return XeenEventScript(XeenEventFile{id,"synthetic.evt",true,{record}});};
 XeenEventInterpreter interpreter;auto step=interpreter.begin(camera,p,{},world,scripts,{});
 const auto *done=std::get_if<XeenEventExecutionCompleted>(&step);
 check(done && done->ending==XeenEndingRequest::Clouds,"Opcode 30 requests the live Flow tail, without a clock or notice");
 check(!p.cloudsCompleted && !done->finalGameFlags.isSet(75) && done->finalCamera.mapId==camera.mapId,
  "Detached interpreter must not run the ending tail");
}
}
int main(){try{damage();deaths();scepter();trolls();maps();ending();std::cout<<"Clouds scripted rule controls passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
