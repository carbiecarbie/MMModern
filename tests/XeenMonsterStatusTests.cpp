#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenScenePresentation.h"
#include "games/xeen/XeenJourneyProgression.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
using S=XeenActorStatus;using Draw=XeenCombatRandom::Draw;
constexpr S controls[]{S::Sleep,S::BeastMaster,S::Hypnotize,S::DragonSleep};
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class C> void finish(C &candidate,XeenCombatRandom &rng,unsigned budget=1){
 for(unsigned n=0;n<1000;++n){XeenConsequenceDraw draw{rng,budget,{}};if(candidate.service(draw))return;}
 throw std::runtime_error("Status candidate did not finish");
}
XeenConsequenceCharacters party(){
 XeenConsequenceCharacters c;
 for(unsigned n=0;n<6;++n){c[n].rosterId=n;c[n].birthYear=592;c[n].permanentLevel=1;c[n].currentHp=100;
  c[n].characterClass=XeenCharacterClass::Knight;c[n].intellect=c[n].personality=c[n].endurance={15,0};}
 return c;
}
XeenConsequenceInputs inputs(){XeenConsequenceInputs i;for(auto &v:i){v.might=v.speed=v.accuracy={15,0};v.luck=XeenAttributeValue{15,0};v.resistances=XeenCombatResistances{};}return i;}
XeenMonsterRecord monster(){XeenMonsterRecord m;m.raw[20]=20;m.raw[22]=10;m.raw[23]=5;m.raw[24]=2;m.raw[25]=1;m.raw[26]=1;m.raw[28]=6;m.raw[31]=5;m.raw[47]=8;return m;}
XeenActor actor(unsigned slot,int x,int y,S status=S::Physical){XeenActor a;a.id={23,slot};a.original={x,y,0,0,6};a.x=x;a.y=y;a.hp=20;a.statistics=monster();a.lifecycle=XeenActorLifecycle::Present;a.activated=true;a.status=status;return a;}
XeenMap map(){XeenMap m;m.geometry.id=23;m.geometry.flags2=0x8000;m.geometry.surfaceTypes[2]=1;for(auto &c:m.geometry.cells)c.geometry=XeenOutdoorLayers{2,0,0,0};return m;}
void valuesAndDamage(){
 auto c=party();auto in=inputs();auto m=monster();
 check(!xeenActorStatusSupported(S::Unsupported) && !xeenActorStatusSupported(static_cast<S>(255)),"Unsupported and invalid status must stay distinct");
 auto a=actor(0,8,8);
 for(auto from:controls)for(auto to:controls){a.status=from;a.status=to;check(a.status==to,"One status must overwrite the previous one");}
 for(auto status:controls){
  XeenCombatRandom empty(std::vector<Draw>{});XeenEnemyAttackCandidate enemy(c,in,m,610,63,{},status);finish(enemy,empty);
  check(!enemy.result.targetedMembers && !enemy.result.injuryCount && !enemy.nextAttack() && !empty.position(),"Disabled attack must skip all attacks without target/damage draws");
  for(bool shoot:{false,true}){
   auto ch=c[0];std::vector<Draw> tape;
   if(shoot){ch.weapons[0]={0,30,0,4};tape={{1,2,1},{1,2,1},{1,2,1}};}
   tape.push_back({1,20,1});if(shoot)tape.push_back({1,56,56});
   auto immune=m;immune.raw[40]=100;
   XeenCombatRandom rng(tape);XeenPhysicalPlayerCandidate attack(ch,in[0],immune,6,610,shoot,XeenDifficulty::Warrior,status);finish(attack,rng);
   check(attack.hit && !attack.damage && attack.status==xeenActorStatusAfterDamage(status,true),"Hit bonus and wake must precede immunity and Shoot save");
   check(rng.position()==tape.size(),"Hit draw count changed");
  }
  // No missile weapon: a successful hit has zero attack2 input and must not wake.
  XeenCombatRandom zero(std::vector<Draw>{{1,20,1}});XeenPhysicalPlayerCandidate noInput(c[0],in[0],m,6,610,true,XeenDifficulty::Warrior,status);finish(noInput,zero);
  check(noInput.hit && !noInput.damage && noInput.status==status && zero.position()==1,"Zero entry damage must not wake or save");
  auto high=m;high.raw[22]=100;
  XeenCombatRandom miss(std::vector<Draw>{{1,20,1}});XeenPhysicalPlayerCandidate absent(c[0],in[0],high,6,610,false,XeenDifficulty::Warrior,status);finish(absent,miss);
  check(!absent.hit && absent.status==status,"Miss must not wake");
  for(auto spell:{XeenLearnedSpell::MagicArrow,XeenLearnedSpell::EnergyBlast,XeenLearnedSpell::Shrapmetal,XeenLearnedSpell::FlyingFist,XeenLearnedSpell::Pain}){
   XeenCombatRandom save(std::vector<Draw>{{1,56,6}});XeenSpellDamageCandidate damage(1,0,0,6,spell,100,status);finish(damage,save);
   check(damage.status==xeenActorStatusAfterDamage(status,true) && save.position()==1,"Existing spell wake/save order");
  }
  XeenCombatRandom gate(std::vector<Draw>{{1,101,1}});XeenSpellDamageCandidate resisted(1,0,100,6,XeenLearnedSpell::MagicArrow,0,status);finish(resisted,gate);
  check(resisted.resisted && resisted.status==status,"Failed magic gate must not wake");
  XeenCombatRandom none(std::vector<Draw>{});XeenSpellDamageCandidate levelZero(0,0,0,6,XeenLearnedSpell::EnergyBlast,0,status);finish(levelZero,none);
  check(levelZero.status==status && !none.position(),"Zero base spell must not wake");
 }
 // A natural 20 still explodes, with the +20 added once after the final die.
 m.raw[22]=35;XeenCombatRandom explode(std::vector<Draw>{{1,20,20},{1,20,1}});
 XeenPhysicalPlayerCandidate attack(c[0],in[0],m,6,610,false,XeenDifficulty::Warrior,S::Sleep);
 XeenConsequenceDraw first{explode,1,{}};check(!attack.service(first) && !attack.hit && attack.status==S::Sleep,"Hit bonus cannot terminate exploding d20");finish(attack,explode);
 check(!attack.hit && explode.position()==2,"Status bonus must be added once after explosion");
 XeenCombatRandom ordinary(std::vector<Draw>{{1,20,1}});m.raw[22]=10;
 XeenPhysicalPlayerCandidate physical(c[0],in[0],m,6,610,false,XeenDifficulty::Warrior);finish(physical,ordinary);check(!physical.hit,"Physical target must not get control bonus");
}
void opportunity(){
 const auto terrain=map();const XeenCamera camera{23,8,8,XeenDirection::North};auto c=party();auto in=inputs();
 std::vector<XeenActor> actors{actor(0,8,8,S::Sleep),actor(1,30,30,S::BeastMaster),actor(2,-128,-128,S::Hypnotize),actor(3,9,8,S::DragonSleep),actor(4,7,8)};
 actors[2].lifecycle=XeenActorLifecycle::Defeated;actors[2].hp=0;actors[2].activated=false;
 // Inject a Dragon Sleep Fire Dragon profile; Part A has no spell producer.
 actors[3].original.resourceId=68;actors[3].statistics->raw[33]=6;
 actors[4].statistics->raw[32]=1;actors[4].statistics->raw[24]=1;
 const auto occupancy=XeenActorApproach::occupancy(actors);check(occupancy[8*32+8]==1,"Controlled actor must occupy its cell");
 check(XeenActorApproach::classify(actors,camera).engaged(),"Controlled front actor must keep contact");
 const auto moved=XeenActorApproach::move(actors,camera,[](const auto &,int,int){return XeenMonsterTerrain::Allowed;},true,[](const auto &,auto){});
 for(unsigned n=0;n<4;++n)check(moved[n].x==actors[n].x && moved[n].y==actors[n].y,"Control must skip movement, including retained dead status");
 XeenRegionalOpportunityCandidate op(terrain,actors,camera,c,in,610,63);op.staged=true;
 check(op.shotCount==1 && op.shots[0].source==actors[4].id,"Controlled sources must not enter ranged queue");
 // Equality recovers; I+1 fails; offscreen and dead slots draw in original order.
 XeenCombatRandom rng(std::vector<Draw>{{1,56,6},{1,56,7},{1,56,6},{0,5,0},{1,20,1}});
 for(unsigned n=0;n<2;++n){XeenConsequenceDraw draw{rng,1,{}};check(!op.service(draw) && !op.travelStarted,"Recovery must precede first travel/shot and resume at the next slot");}
 XeenConsequenceDraw last{rng,1,{}};check(!op.service(last) && op.travelStarted && rng.position()==3,"Recovery must finish before queued travel");
 check(op.actors[0].status==S::Physical && op.actors[1].status==S::BeastMaster && op.actors[2].status==S::Physical && op.actors[3].status==S::DragonSleep,"Recovery boundary/all-slot/Dragon Sleep semantics");
 check(op.shotCount==1,"Recovered monsters cannot retroactively join queue");op.travelPresented=true;finish(op,rng);
 check(rng.position()==5,"Recovery order must precede target and damage requests");
 XeenRegionalOpportunityCandidate rejected(terrain,{actors[0]},camera,c,in,610,63);
 XeenCombatRandom rejection(std::vector<Draw>{{1,56,0,true},{1,56,6}});
 XeenConsequenceDraw raw{rejection,1,{}};check(!rejected.service(raw) && rejected.actors[0].status==S::Sleep,"Rejected conversion must retain the same recovery slot");
 finish(rejected,rejection);check(rejection.position()==2 && rejected.actors[0].status==S::Physical,"Recovery rejection must resume without a duplicate accepted draw");
 for(auto context:{XeenActorOpportunityContext{false,false,false},XeenActorOpportunityContext{false,true,true}}){
  XeenRegionalOpportunityCandidate disabled(terrain,actors,camera,c,in,610,63,{},context);XeenCombatRandom empty(std::vector<Draw>{});finish(disabled,empty);
  check(!disabled.shotCount && disabled.actors[0].status==S::Sleep && !empty.position(),"Disabled opportunity must skip recovery too");
 }
 for(auto lifecycle:{XeenActorLifecycle::Present,XeenActorLifecycle::Disabled,XeenActorLifecycle::Unresolved,XeenActorLifecycle::Defeated})for(auto status:controls){
  auto a=actor(0,1,1,status);a.lifecycle=lifecycle;a.hp=1;a.activated=true;if(lifecycle==XeenActorLifecycle::Unresolved)a.statistics.reset();
  xeenRefillLoadedMonster(a);check(a.status==S::Physical,"Map reload must reset every lifecycle slot");
  auto retained=actor(0,-128,-128,status);retained.lifecycle=lifecycle;retained.hp=0;retained.activated=false;
  if(lifecycle==XeenActorLifecycle::Unresolved)retained.statistics.reset();
  XeenRegionalOpportunityCandidate sleeping(terrain,{retained},camera,c,in,610,63,{},XeenActorOpportunityContext{true});
  XeenCombatRandom recovery(status==S::DragonSleep?std::vector<Draw>{}:std::vector<Draw>{{1,56,6}});finish(sleeping,recovery);
  check(sleeping.actors[0].status==(status==S::DragonSleep?status:S::Physical),"Rest recovery must scan every lifecycle without requiring statistics");
 }
 for(auto status:controls){
  auto a=actor(0,8,8,status);std::array<const XeenCharacter *,6> owners{};for(unsigned n=0;n<6;++n)owners[n]=&c[n];
  check(xeenPrepareJourneyLethal(a,owners,in,{},63).actor.lifecycle==XeenActorLifecycle::Defeated,"Controlled actor must pass kill progression");
 }
}
void animation(){
 for(auto status:controls)for(unsigned frame:{3u,8u,10u,11u}){
  XeenMonsterAnimation a;a.frame=frame;a.postAttackDelay=2;a.effect2=3;
  a.advance(monster(),status);check(a.frame==frame && a.postAttackDelay==2 && a.effect3==1,"Control freezes whole pose block but keeps impact effect alive");
  a.advance(monster(),S::Physical);check(a.frame!=frame || a.postAttackDelay!=2,"Recovery must release frozen pose");
 }
 auto a=actor(0,8,8,S::Hypnotize);XeenScenePresentation scene(1);scene.include({a});
 XeenMonsterAppearance pose;pose.identity=a.id;pose.kind=XeenMonsterSpriteKind::Attack;pose.frame=3;scene.appearance(pose);
 scene.spellImpact(a,XeenLearnedSpell::MagicArrow);scene.advanceAfterDraw(23);
 check(scene.animation(a.id)->frame==11 && scene.animation(a.id)->effect3==1,"Scene must carry live control status into frame-11/FX animation");
 a.status=S::Physical;scene.include({a});scene.advanceAfterDraw(23);
 check(scene.animation(a.id)->postAttackDelay==4,"Scene must refresh status after recovery");
}
}
int main(){try{valuesAndDamage();opportunity();animation();std::cout<<"Monster status core draw tapes passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
