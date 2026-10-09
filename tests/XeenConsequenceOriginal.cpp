#include "XeenTestInstallation.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
// Read-only original resources with explicitly artificial rare-rule arrangements.
// This executable is additional evidence, never a replacement for gameplay routes.
#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenSaveState.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "XeenSaveTestSupport.h"
#include "XeenCombatGameplayTestSupport.h"
#include "XeenJourneyResourceTestSupport.h"
#include "XeenTrainingTestSupport.h"
#include <iostream>
#include "platform/XeenSaveFile.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "XeenInitialResourceOracle.h"
#include "XeenM40Evidence.h"
using namespace mmodern;
using save_test::check;
namespace mmodern {
struct XeenCombatRotationTestAccess {
 static void arm(XeenCombat &c) {c.countdownForScheduling().arm();}
 static void acquired(XeenCombat &c) {c.rangedPresented(c.ticket(),true);}
};
}
namespace consequence_controls {
std::vector<XeenCombatRandom::Draw> tape;unsigned cursor=0;bool taped=false;
std::optional<std::uint32_t> realDraw(XeenCombatRandom *,std::uint32_t,std::uint32_t) asm("__real__ZN7mmodern16XeenCombatRandom4drawEjj");
std::optional<std::uint32_t> wrappedDraw(XeenCombatRandom *,std::uint32_t,std::uint32_t) asm("__wrap__ZN7mmodern16XeenCombatRandom4drawEjj");
std::optional<std::uint32_t> wrappedDraw(XeenCombatRandom *r,std::uint32_t lo,std::uint32_t hi) {
 const auto result=realDraw(r,lo,hi);if(!taped)return result;
 check(cursor<tape.size(),"Rare-rule literal tape exhausted");const auto d=tape[cursor++];
 if(d.lo!=lo || d.hi!=hi)throw std::runtime_error("Rare-rule interval mismatch requested "+std::to_string(lo)+","+std::to_string(hi)+" expected "+std::to_string(d.lo)+","+std::to_string(d.hi));
 return d.raw?std::nullopt:std::optional<std::uint32_t>{d.value};
}
// Artificial zero-damage operand at the pure resolver boundary only. World
// profiles/resources remain original; no genuine route uses this wrapper mode.
bool zeroResistanceFixture=false;
#define PLAYER_CTOR "_ZN7mmodern27XeenPhysicalPlayerCandidateC1ERKNS_13XeenCharacterERKNS_16XeenCombatInputsERKNS_17XeenMonsterRecordEjjbNS_14XeenDifficultyE"
void realPlayer(XeenPhysicalPlayerCandidate *,const XeenCharacter &,const XeenCombatInputs &,const XeenMonsterRecord &,unsigned,unsigned,bool,XeenDifficulty) asm("__real_" PLAYER_CTOR);
void wrappedPlayer(XeenPhysicalPlayerCandidate *,const XeenCharacter &,const XeenCombatInputs &,const XeenMonsterRecord &,unsigned,unsigned,bool,XeenDifficulty) asm("__wrap_" PLAYER_CTOR);
void wrappedPlayer(XeenPhysicalPlayerCandidate *self,const XeenCharacter &c,const XeenCombatInputs &i,const XeenMonsterRecord &m,unsigned type,unsigned year,bool shoot,XeenDifficulty difficulty){
 auto operand=m;if(zeroResistanceFixture && shoot)operand.raw[40]=100;realPlayer(self,c,i,operand,type,year,shoot,difficulty);
}
struct Source:training_test::Inputs {
 std::vector<XeenMonsterRecord> &mon=statistics;XeenEventFile &evt=mainland;
 explicit Source(const GameInstallation &i):Inputs(i){}
 XeenRegionalManifest manifest(){return regional();}
 auto resources(){auto r=Inputs::resources();r.loadEvents=[this](auto id){return id==XeenMapIdentity(23)?evt:city;};return r;}
};
auto currentServices(combat_gameplay_test::Harness &h,Source &source){
 auto services=h.services();auto r=source.resources();r.loadInitialParty=services.resources.loadInitialParty;r.loadInitialContext=services.resources.loadInitialContext;
 r.loadInitialPurse=[&source]{return XeenCharacterFormat::parseMonsterPurse(source.pty);};
 r.loadInitialRegionalRecovery=[&source]{return XeenQuestFlagFormat::parseRegionalRecovery(source.pty);};
 r.loadInitialBankBalances=[&source]{return XeenCharacterFormat::parseBankBalances(source.pty);};services.resources=r;services.texts=r.loadRegionalText;return services;
}
struct Domain {
 Source &source;XeenWorld world;XeenPartyState party;XeenCamera camera{23,9,11,XeenDirection::West};XeenGameFlags flags;
 std::uint64_t now=0;XeenEventPresenter::Clock clock=[this]{return now;};std::unique_ptr<XeenEncounterFlow> flow;
 Domain(Source &s,const std::optional<XeenSaveSnapshot> &saved={},XeenWorld::MapLoader fixture={}):source(s),world(fixture?fixture:[&](auto id){return s.maps.loadGeometryMap(s.assets,id);},[&](auto id){return s.maps.loadObjects(s.assets,id);}) {
  auto state=saved?*saved:s.base();
  if(!saved)state.journey->random=XeenJourneyRandomState{1,1,0};
  XeenSaveState::restoreBeforeGameplay(state,s.resources(),party,camera,flags,world,[](auto &,const auto &,const auto &,const auto &){});
  flow=std::make_unique<XeenEncounterFlow>(world,party,camera,flags,clock,XeenJourneyRestoreTag{});
  present();
 }
 void present(){check(flow->prepareJourneyFrame(flow->ticket(),[]{}) && flow->presentJourney(flow->ticket()),"Artificial control presentation");}
 XeenSaveSnapshot save(){return XeenSaveState::capture(source.signature,party,camera,flags,world);}
};
void attack(XeenCombat &combat,std::vector<XeenCombatRandom::Draw> draws) {
 XeenCombatRandom expected(combat.random().continuation());
 for(const auto &draw:draws)realDraw(&expected,draw.lo,draw.hi);
 tape=std::move(draws);cursor=0;taped=true;
 auto r=combat.service(combat.ticket());taped=false;
 check(cursor==tape.size(),"Every rare-rule literal draw consumed");check(r.status==XeenCombatStatus::Advanced,"Artificial resource attack published");
 check(combat.random().continuation()==expected.continuation(),"Exact attack RNG continuation publication");
}
std::vector<XeenCombatRandom::Draw> toadTape(const XeenPartyState &p,bool awake,unsigned roll,unsigned parameter,bool sleepFirst) {
 std::vector<XeenCombatRandom::Draw> draws{{0,5,0}};
 const unsigned i=0;const auto id=kXeenCombatOwners[i];
  if(awake){draws.push_back({1,20,roll});if(roll==1)return draws;draws.push_back({1,8,parameter});}
  for(unsigned n=0;n<3;++n)draws.push_back({1,8,1});
  const auto &c=p.roster.at(id);const auto &in=*p.roster.combatInputs(id);
  const auto v=XeenCharacterRules::physicalBonus(XeenCharacterRules::effectiveLuck(c,in))+c.currentLevel();
  constexpr unsigned intervals[]{23,24,22,27,24,25};if(unsigned(v+20)!=intervals[i])throw std::runtime_error("Original save interval owner "+std::to_string(i)+" value "+std::to_string(v+20));
  draws.push_back({1,intervals[i],i==0 && !sleepFirst?1:intervals[i]});
 return draws;
}

void appearanceResources(Source &s) {
 const auto crc=[](const auto &bytes){std::uint32_t v=0xffffffffu;for(auto b:bytes){v^=b;for(unsigned i=0;i<8;++i)v=(v>>1)^((v&1)?0xedb88320u:0u);}return ~v;};
 struct Image {unsigned id,monSize,attSize;std::uint32_t monCrc,attCrc;};
 for(const auto &v:std::array<Image,5>{{{3,19145,13045,0xc9df78af,0x279fe4ac},{6,19022,15639,0x0b8c61d5,0x975cca6f},{8,35946,22004,0x1d238d62,0xf73997b9},{9,22076,14892,0xceb119e6,0xe968a6c6},{13,10856,19479,0xc3aabd05,0x1402fd81}}}) {
  const auto m=s.assets.readArchiveResource(XeenAssetSource::normalMonsterResource(v.id)),a=s.assets.readArchiveResource(XeenAssetSource::attackMonsterResource(v.id));
  check(m.size()==v.monSize && crc(m)==v.monCrc && a.size()==v.attSize && crc(a)==v.attCrc,"Original MON/ATT literal bytes and CRC");
  s.assets.validateNormalMonster(v.id);s.assets.validateAttackMonster(v.id);
 }
 for(bool enemy:{false,true}){const auto b=s.assets.readArchiveResource(enemy?"pow12.icn":"pow11.icn");check(b.size()==(enemy?358u:451u) && crc(b)==(enemy?0xd977e765u:0x67f7f690u) && b[0]==3 && b[1]==0,"Original POW literal bytes, CRC and frame count");s.assets.validateProjectile(enemy);}
 Domain d(s);
 constexpr int bases[]{124,95,76,53};
 constexpr int xs[4][6]{{72,72,93,51,97,47},{72,72,85,59,89,55},{72,72,77,67,81,63},{72,72,69,75,73,71}};
 constexpr int ys[4][6]{{43,43,48,48,36,36},{48,48,53,53,41,41},{53,53,58,58,47,47},{58,58,63,63,53,53}};
 for(bool enemy:{false,true})for(unsigned row=0;row<4;++row)for(unsigned lane=0;lane<6;++lane){
  XeenMonsterAppearance appearance{0};appearance.projectile=XeenProjectileAppearance{enemy,row,lane,3,XeenMonsterIdentity{23,9}};
  const auto commands=XeenOutdoorScene().build(d.world,d.camera,nullptr,nullptr,{},appearance);
  unsigned found=0;for(const auto &c:commands)if(const auto *p=c.projectile()){
   ++found;const auto o=c.drawOptions();check(c.originalOrder==bases[row]+int(lane) && c.x==xs[row][lane] && c.y==ys[row][lane] && o.scaleIndex==4*row+(enemy?3:0) && o.horizontalFlip==bool(lane%2) && o.sceneClipped,"Literal projectile row/lane/order/scale/clip");
  }check(found==1,"One disposable projectile command");
 }
 std::cout<<"Five MON/ATT and both POW exact resources; 48 projectile placements PASS\n";
}

void combatPublicationFaults(Source &source) {
 for(bool toad:{false,true}) {
  Domain original(source);auto saved=original.save();
  for(auto id:kXeenCombatOwners)saved.characters[id].currentHp=1000;
  auto &a=saved.journey->actors[toad?14:9];a.x=8;a.y=11;a.activated=true;if(!toad)a.hp=1;
  const auto ready=[&](Domain &d){
   d.flow->journeyAction(d.flow->ticket(),XeenEncounterAction::Forward);
   if(d.flow->state().phase()==XeenEncounterPhase::Exploring)d.flow->journeyPulse(d.flow->ticket());
   check(d.flow->attachJourney(d.flow->ticket(),[]{}),"Fault fixture attaches");
   auto &combat=*d.flow->combat();
   if(!toad){unsigned limit=0;while(combat.phase()!=XeenCombatPhase::PlayerReady){check(++limit<20,"Fault player ready");combat.service(combat.ticket());}combat.command(combat.ticket(),XeenCombatCommand::Attack);}
  };
  unsigned calls=0;
  {Domain d(source,saved);ready(d);auto &c=*d.flow->combat();c.setProbe([&]{++calls;});
   while(c.service(c.ticket()).status==XeenCombatStatus::Pending){}
   check(calls>3,"Observe preparation/draw/publication boundaries");
  }
  for(unsigned failAt=1;failAt<=calls;++failAt){
   Domain d(source,saved);ready(d);auto &c=*d.flow->combat();
   const auto chars=d.party.roster.characters();const std::vector<XeenActor> actors=d.world.sessionState().actors();
   const auto random=d.world.sessionState().journeyRandom();const auto purse=d.party.monsterTreasure;const auto context=d.party.encounterContext;
   unsigned n=0;c.setProbe([&]{if(++n==failAt)throw std::bad_alloc();});
   XeenCombatResult result;do{result=c.service(c.ticket());}while(result.status==XeenCombatStatus::Pending);
   check(n==failAt && result.status==XeenCombatStatus::Failed,"Every injected preparation boundary fails closed");
   for(unsigned i=0;i<30;++i)check(xeen_state::sameCharacter(chars[i],d.party.roster.at(i)),"No partial party-wide or lethal publication");
   for(unsigned i=0;i<actors.size();++i)check(xeen_state::sameActor(actors[i],d.world.sessionState().actors()[i]),"Unpublished actor candidate unchanged");
   check(d.world.sessionState().journeyRandom()==random && d.party.monsterTreasure==purse && d.party.encounterContext==context,"Failed unit preserves RNG/purse/time predecessor");
   check(!XeenSaveState::canCapture(d.party,d.camera,d.world),"Failure latch excludes capture");
  }
  std::cout<<"ARTIFICIAL "<<(toad?"party-wide Toad":"lethal Orc")<<" per-callback failure controls PASS count="<<calls<<'\n';

  for(unsigned fault=0;fault<7;++fault){
   Domain d(source,saved);ready(d);auto &c=*d.flow->combat();const auto old=c.ticket();
   const auto chars=d.party.roster.characters();const auto rng=d.world.sessionState().journeyRandom();
   if(fault==0)--d.party.roster.at(29).currentHp;
   if(fault==1){bool denied=false;try{auto replacement=d.party;d.party=replacement;}catch(const std::logic_error &){denied=true;}check(denied,"Marked party replacement refused before ABA");continue;}
   if(fault==2){bool denied=false;try{auto replacement=d.party.roster;d.party.roster=replacement;}catch(const std::logic_error &){denied=true;}check(denied,"Marked roster replacement refused before ABA");continue;}
   if(fault==3)++d.party.monsterTreasure->gold;
   if(fault==4)++const_cast<XeenCombatInputs &>(*d.party.roster.combatInputs(29)).resistances->coldPermanent;
   if(fault==5)const_cast<XeenMap &>(d.world.map(23)).geometry.runX^=1;
   if(fault==6){c.preparePresentation(old,[&]{check(c.service(old).status==XeenCombatStatus::Refused,"Reentrant combat service refused");});continue;}
   const auto result=c.service(old);check(result.status==XeenCombatStatus::Failed || result.status==XeenCombatStatus::Stale,"Mutable/owner/resource preimage rejects prepared work");
   check(rng==d.world.sessionState().journeyRandom(),"Rejected owner does not publish RNG");
   for(unsigned i=0;i<29;++i)check(xeen_state::sameCharacter(chars[i],d.party.roster.at(i)),"Rejected owner does not publish physical consequences");
   check(!XeenSaveState::canCapture(d.party,d.camera,d.world),"Changed authority excludes capture");
  }
  // A published predecessor survives compatible presentation failure and retry.
  {Domain d(source,saved);ready(d);auto &c=*d.flow->combat();while(c.service(c.ticket()).status==XeenCombatStatus::Pending){}
   const auto chars=d.party.roster.characters();const std::vector<XeenActor> actors=d.world.sessionState().actors();const auto rng=d.world.sessionState().journeyRandom();
   bool threw=false;try{c.preparePresentation(c.ticket(),[]{throw std::bad_alloc();});}catch(const std::bad_alloc &){threw=true;}check(threw,"Injected post-publication I/O failure");
   c.preparePresentation(c.ticket(),[]{});
   for(unsigned i=0;i<30;++i)check(xeen_state::sameCharacter(chars[i],d.party.roster.at(i)),"Presentation retry retains published party");
   for(unsigned i=0;i<actors.size();++i)check(xeen_state::sameActor(actors[i],d.world.sessionState().actors()[i]),"Presentation retry retains published actor");
   check(rng==d.world.sessionState().journeyRandom(),"Presentation retry never rerolls");
  }
 }
}

void restoreConsequences(Source &source,const std::optional<std::filesystem::path> &fixture) {
 Domain original(source);const auto saved=original.save();
 for(unsigned mode=0;mode<8;++mode){auto bad=saved;
  if(mode==0)bad.characters[0].conditions[0]=1;
  if(mode==1)bad.characters[0].conditions[12]=1;
  if(mode==2)bad.characters[0].currentHp=0;
  if(mode==3)bad.characters[0].conditions[8]=2;
  if(mode==4)bad.journey->actors[9].hp=26;
  if(mode==5)for(auto id:kXeenCombatOwners)bad.characters[id].conditions[13]=2;
  if(mode==6)bad.journey->context->minutes=1260;
  if(mode==7){auto &a=bad.journey->actors[9];a.x=a.y=-128;a.hp=0;a.lifecycle=XeenActorLifecycle::Defeated;a.activated=false;a.accounted=true;bad.journey->treasure->pendingMask=512;bad.journey->treasure->pendingGold=10;}
  const bool canonical=mode==0 || mode==3 || mode==6;
  bool failed=false;try{Domain d(source,bad);if(canonical)save_test::sameSnapshot(bad,d.save());}catch(const std::exception &){failed=true;}
  check(failed!=canonical,"Canonical conditions/night restore; malformed/currently collectable state refused");
 }
 auto sleep=saved;for(auto id:kXeenCombatOwners)sleep.characters[id].conditions[8]=1;
 Domain sleeping(source,sleep);save_test::sameSnapshot(sleep,sleeping.save());
 check(sleeping.flow->handle(ShootAction{}),"All-asleep Shoot produces explicit empty-eligibility refusal");sleeping.flow->holdJourneyFrame();sleeping.present();save_test::sameSnapshot(sleep,sleeping.save());
 check(sleeping.flow->journeyRefusal().find("no awake eligible")!=std::string::npos,"Empty eligibility has readable refusal and no time/RNG charge");
 auto pending=saved;auto &dead=pending.journey->actors[9];dead.x=dead.y=-128;dead.hp=0;dead.activated=false;dead.lifecycle=XeenActorLifecycle::Defeated;dead.accounted=true;
 auto &survivor=pending.journey->actors[8];survivor.x=7;survivor.y=11;survivor.activated=true;
 pending.journey->treasure->pendingMask=512;pending.journey->treasure->pendingGold=10;pending.journey->treasure->armor[0]={9,{0,2,0,0}};
 Domain delayed(source,pending);save_test::sameSnapshot(pending,delayed.save());
 check(XeenSaveFormat::encode(pending)==XeenSaveFormat::encode(delayed.save()),"Artificial pending item/provenance exact bytes");
 if(fixture)XeenSaveFile::write(*fixture,delayed.save());
 std::cout<<"ARTIFICIAL malformed consequence restores, all-asleep Quiet and selected-survivor treasure PASS\n";
}
void shootOrder(Source &source) {
 Domain original(source);auto saved=original.save();
 for(unsigned n=0;n<3;++n){auto &a=saved.journey->actors[9-n];a.x=8-n;a.y=11;a.activated=true;}
 saved.characters[0].weapons[8]={0,30,0,4};
 Domain d(source,saved);
 tape.clear();for(unsigned shot=0;shot<4;++shot){for(unsigned die=0;die<3;++die)tape.push_back({1,2,1});tape.push_back({1,20,shot<2?1u:19u});if(shot>=2)tape.push_back({1,56,56});}
 cursor=0;taped=true;check(d.flow->handle(ShootAction{}),"Artificial mixed-target Shoot starts through typed action");d.present();
 const auto finishVisuals=[](Domain &v,unsigned distance){
  std::vector<std::vector<std::pair<unsigned,unsigned>>> seen;
  for(unsigned n=0;n<100 && v.party.encounterContext->minutes==480;++n){
   std::vector<std::pair<unsigned,unsigned>> frame;
   for(const auto &p:v.flow->appearance().projectiles){check(!p.enemy&&!p.source,"Player projectile never fabricates monster identity");frame.emplace_back(p.lane,p.row);}
   seen.push_back(frame);
   check(!v.flow->canSave(),"In-flight/impact Shoot exposed save boundary");
   v.now+=100;v.flow->idle();
   if(v.flow->appearance().kind==XeenMonsterSpriteKind::Attack) {
    XeenRestoreGuard held(v.world,v.party,v.camera,v.flags);
    check(!v.flow->idle() && held.current(),"Unacknowledged Shoot impact replayed RNG/HP/reward");
    check(v.flow->journeyAction(v.flow->ticket(),XeenEncounterAction::Left).outcome==XeenEncounterOutcome::Refused && held.current(),"Unacknowledged Shoot impact accepted another action");
   }
   v.flow->holdJourneyFrame();v.present();}
  // combat.cpp:1916/1963/2017/2071 draw each depth; misses do not
  // call attack2. Both hit draws advance *all* remaining visual lanes.
  const std::vector<std::vector<std::pair<unsigned,unsigned>>> expected=distance==3 ?
   std::vector<std::vector<std::pair<unsigned,unsigned>>>{{{0,0},{2,0}},{{0,1},{2,1}},{{0,2},{2,2}},{{0,3},{2,3}},{}} :
   std::vector<std::vector<std::pair<unsigned,unsigned>>>{{{0,0},{2,0}},{{0,1},{2,1}},{{0,2},{2,2}},{{0,3},{2,3}},{},{},{},{}};
  if(seen!=expected) {std::cerr<<"Ordered frames:";for(const auto &f:seen){std::cerr<<" [";for(const auto &p:f)std::cerr<<p.first<<':'<<p.second<<',';std::cerr<<']';}std::cerr<<'\n';}
  check(seen==expected,"Ordered multi-shooter reference draw/lane oracle differs");
  check(v.party.encounterContext->minutes==490&&v.flow->state().pending()==3,"Visuals preserve charge and owed opportunity");
 };
 finishVisuals(d,2);taped=false;
 check(cursor==18 && d.world.sessionState().journeyRandom()->count==18,"Two misses then two hits in active-member order");
 check(d.world.sessionState().actors()[9].hp==25 && d.world.sessionState().actors()[8].hp==7 && d.world.sessionState().actors()[7].hp==25,"Miss advances original target order; each successful shooter spends once");
 check(d.party.encounterContext->minutes==490 && !d.flow->canSave(),"Volley charges after every impact; capture remains excluded for owed movement");
 Domain misses(source,saved);tape.clear();for(unsigned shot=0;shot<6;++shot){for(unsigned die=0;die<3;++die)tape.push_back({1,2,1});tape.push_back({1,20,1});}
 cursor=0;taped=true;check(misses.flow->handle(ShootAction{}),"All-miss volley begins");misses.present();
 finishVisuals(misses,3);taped=false;check(cursor==24 && misses.world.sessionState().journeyRandom()->count==24,"Both shooters traverse all three rows on misses");
 for(unsigned id:{7u,8u,9u})check(misses.world.sessionState().actors()[id].hp==25,"All-miss volley leaves actor wounds unchanged");
 check(misses.world.sessionState().journeyRandom()->count==24,"All-miss visuals consume no RNG");
 std::cout<<"ARTIFICIAL two-shooter miss continuation, all-miss rows and spent-hit target ordering PASS\n";
}
void shootLethalPreparation(Source &source) {
 Domain original(source);auto saved=original.save();
 for(auto &other:saved.journey->actors)if(other.id.recordIndex!=9) {
  other.x=other.y=-128;other.hp=0;other.activated=false;other.lifecycle=XeenActorLifecycle::Defeated;other.accounted=true;
 }
 auto &a=saved.journey->actors[9];a.x=8;a.y=11;a.activated=true;a.hp=1;
 for(unsigned mode=0;mode<3;++mode) {
  const bool overflow=mode==2,lethal=mode!=0;
  auto input=saved;if(!lethal)input.journey->actors[9].hp=25;
  if(overflow)input.journey->supplements[0].inputs.experience=UINT32_MAX;
  Domain d(source,input);const auto rng=d.world.sessionState().journeyRandom();
  const auto chars=d.party.roster.characters();const auto purse=d.party.monsterTreasure;
  tape={{1,2,1},{1,2,1},{1,2,1},{1,20,19},{1,56,56},{1,100,100}};
  cursor=0;taped=true;check(d.flow->handle(ShootAction{}),"Lethal Shoot starts");d.present();
  d.now+=100;d.flow->idle();
  check(d.flow->appearance().projectiles.size()==1 && d.flow->appearance().projectiles[0].row==1 && cursor==0,"Ordered depth1 arrival must precede damage draws");
  d.flow->holdJourneyFrame();d.present();d.now+=100;
  bool failed=false;try{d.flow->idle();}catch(const std::overflow_error &){failed=true;}
  if(overflow) {
   check(failed && d.world.sessionState().actors()[9].hp==1 &&
    d.world.sessionState().journeyRandom()==rng && d.party.monsterTreasure==purse &&
    !d.world.sessionState().accountedMonsters().count({23,9}),"Failed lethal preparation published HP/RNG/reward");
   for(unsigned n=0;n<30;++n)check(xeen_state::sameCharacter(chars[n],d.party.roster.at(n)),"Failed Shoot lethal preparation changed party");
  }else {
   check(!failed,"Lethal Shoot preparation failed");
   // Last shooter: depth1 arrival advances to row2, then drawScene clears
   // _charsShooting before animate3d. Repeated row2 frames are intentional.
   for(unsigned frame=0;frame<(lethal?3u:2u);++frame) {
    auto appearance=d.flow->appearance();check(appearance.projectiles.size()==1 && appearance.projectiles[0].lane==2 && appearance.projectiles[0].row==2,"Ordered last-shooter lane frame differs");
    const auto &actor=d.world.sessionState().actors()[9];
    check(frame==0 ? actor.hp==(lethal?1:25) : frame==1 ? actor.hp==(lethal?0:16) && actor.lifecycle==XeenActorLifecycle::Present : actor.lifecycle==XeenActorLifecycle::Defeated,"Shoot HP/frame/removal ordering differs");
    for(unsigned n=0;n<6;++n)check(d.party.roster.combatInputs(kXeenCombatOwners[n])->experience==
      input.journey->supplements[kXeenCombatOwners[n]].inputs.experience+(frame==2?66u:0u),"Shoot frame credited XP before removal");
    const auto before=d.world.sessionState().journeyRandom();
    check(!d.flow->idle() && d.world.sessionState().journeyRandom()==before,"Unacknowledged lethal frame advanced");
    d.flow->holdJourneyFrame();bool rejected=false;
    rejected=!d.flow->prepareJourneyFrame(d.flow->ticket(),[]{throw std::bad_alloc();});
    check(rejected && d.flow->appearance().projectiles.size()==appearance.projectiles.size() && d.world.sessionState().journeyRandom()==before,"Failed Shoot composition advanced lanes/RNG");
    d.flow->holdJourneyFrame();d.present();d.now+=100;d.flow->idle();
   }
  }
  check(cursor==(lethal?6u:5u),"Ordered Shoot frame sequence introduced random draws");taped=false;
 }
 std::cout<<"ARTIFICIAL Shoot lethal preparation overflow preserves HP/RNG; ordered lethal frames PASS\n";
}
void indoorShoot(Source &source) {
 constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
 for(bool original:{false,true})for(unsigned facing=0;facing<(original?1u:4u);++facing)
 for(unsigned depth=1;depth<=3;++depth)for(unsigned mode=0;mode<5;++mode) {
  // Real CHR/MON/MOB owners with explicitly artificial actor arrangements.
  // Original north entrance geometry is also exercised without substitution.
  auto saved=source.service();saved.camera={28,15,original?0:15,XeenDirection(facing)};
  saved.journey->random=XeenJourneyRandomState{1,1,0};
  for(auto &a:*saved.journey->vertigoActors) {
   a.x=a.y=-128;a.hp=0;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
  }
  const bool empty=mode==0,miss=mode==1,lethal=mode==2 || mode==3,overflow=mode==3;
  const unsigned targetId=mode==4?44:35;
  auto &target=saved.journey->vertigoActors->at(targetId);
  if(!empty) {
   target.x=saved.camera.x+dx[facing]*int(depth);target.y=saved.camera.y+dy[facing]*int(depth);
   target.hp=mode==4?20:1;target.activated=true;target.lifecycle=XeenActorLifecycle::Present;target.accounted=false;
  }
  // Two simultaneous lanes must travel even when all their attacks miss.
  saved.characters[0].weapons[8]={0,30,0,4};
  if(overflow)saved.journey->supplements[0].inputs.experience=UINT32_MAX;
  const XeenWorld::MapLoader maps=original?XeenWorld::MapLoader{}:XeenWorld::MapLoader{[&](auto id) {
   auto m=source.maps.loadGeometryMap(source.assets,id);
   if(!m.geometry.isOutdoors())for(auto &cell:m.geometry.cells){cell.geometry=XeenIndoorWalls{};cell.rawWord=0;}
   return m;
  }};
  Domain d(source,saved,maps);const auto before=d.world.sessionState().journeyRandom();
  const auto mainland=std::vector<XeenActor>(d.world.sessionState().actors());
  const auto chars=d.party.roster.characters();
  tape.clear();
  if(!empty)for(unsigned shooter=0;shooter<(miss || mode==4?2u:1u);++shooter) {
   for(unsigned die=0;die<3;++die)tape.push_back({1,2,1});
   tape.push_back({1,20,miss?1u:19u});if(!miss)tape.push_back({1,mode==4?123u:50u,mode==4?123u:50u});
  }
  cursor=0;taped=true;check(d.flow->handle(ShootAction{}),"Indoor typed Shoot starts");d.present();
  unsigned hitFrames=0,hpFrames=0,removedFrames=0;bool failed=false;
  for(unsigned n=0;n<50 && d.party.encounterContext->minutes==saved.journey->context->minutes;++n) {
   check(!d.flow->canSave(),"Indoor volley exposed a quiet save boundary");
   d.now+=100;
   try{d.flow->idle();}catch(const std::overflow_error &){failed=true;break;}
   const auto &live=d.world.sessionState().regionalActors(28).at(targetId);
   if(!empty && lethal && live.lifecycle==XeenActorLifecycle::Present && live.hp==1 &&
      d.flow->appearance().kind==XeenMonsterSpriteKind::Attack)++hitFrames;
   if(!empty && live.lifecycle==XeenActorLifecycle::Present && live.hp==0) {
    ++hpFrames;check(!d.world.sessionState().accountedMonsters().count({28,targetId}),"Indoor HP frame credited XP/removal early");
   }
   if(!empty && live.lifecycle==XeenActorLifecycle::Defeated)++removedFrames;
   if(!overflow)for(auto owner:kXeenCombatOwners) {
    // Slime awards 50 / six eligible owners, doubled below level 15.
    const unsigned award=lethal && live.lifecycle==XeenActorLifecycle::Defeated?
     (chars[owner].permanentLevel<15?16u:8u):0u;
    check(d.party.roster.combatInputs(owner)->experience==saved.journey->supplements[owner].inputs.experience+award,
     "Indoor hit/HP frames awarded XP early, or removal awarded it more than once");
   }
   const auto rng=d.world.sessionState().journeyRandom();
   const auto appearance=d.flow->appearance();
   if(!appearance.projectiles.empty() || appearance.impactSnapshot || appearance.kind==XeenMonsterSpriteKind::Attack)
    check(!d.flow->idle() && rng==d.world.sessionState().journeyRandom(),"Unacknowledged indoor draw advanced RNG");
   d.flow->holdJourneyFrame();
   check(!d.flow->prepareJourneyFrame(d.flow->ticket(),[]{throw std::bad_alloc();}) &&
     rng==d.world.sessionState().journeyRandom(),"Indoor composition failure published RNG");
   d.flow->holdJourneyFrame();d.present();
  }
  taped=false;
  if(overflow) {
   check(failed && d.world.sessionState().regionalActors(28).at(35).hp==1 &&
    before==d.world.sessionState().journeyRandom() && !d.world.sessionState().accountedMonsters().count({28,35}),
    "Indoor lethal preparation failure published HP/RNG/accounting");
   for(unsigned n=0;n<30;++n)check(xeen_state::sameCharacter(chars[n],d.party.roster.at(n)),"Indoor failed preparation changed party");
  }else {
   check(!failed && d.party.encounterContext->minutes==saved.journey->context->minutes+1 &&
    d.flow->state().pending()==3,"Indoor Shoot must charge exactly one minute after settlement");
   check(cursor==(empty?0u:miss?8u:mode==4?10u:5u) && d.world.sessionState().journeyRandom()->count==cursor,
    "Indoor visual/impact work added RNG draws");
   if(!empty && lethal)check(hitFrames==1 && hpFrames==1 && removedFrames>=1 &&
     d.world.sessionState().accountedMonsters().count({28,35}),"Indoor lethal hit/HP/removal sequence differs");
   if(miss)check(d.world.sessionState().regionalActors(28).at(35).hp==1,"Indoor miss damaged target");
   if(mode==4)check(d.world.sessionState().regionalActors(28).at(targetId).hp<20 &&
    d.world.sessionState().regionalActors(28).at(targetId).hp>0 && !d.world.sessionState().accountedMonsters().count({28,targetId}),
    "Indoor nonlethal hit changed lifecycle/accounting");
  }
  for(unsigned n=0;n<mainland.size();++n)check(xeen_state::sameActor(mainland[n],d.world.sessionState().actors()[n]),
   "Indoor Shoot published into mainland collection");
 }
 std::cout<<"Indoor Shoot original/artificial geometry, four facings, every distant row, miss/empty, lethal/failure and one-minute charge PASS\n";
}
void indoorChangedShoot(Source &source) {
 constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
 XeenWorld original(source.mapLoader());original.markEncounterSession(XeenEncounterEntry::Journey);
 std::optional<XeenCamera> gate;
 for(int y=1;y<31 && !gate;++y)for(int x=1;x<31 && !gate;++x)for(unsigned direction=0;direction<4;++direction) {
  const XeenCamera c{28,x,y,XeenDirection(direction)};const auto cell=original.sampleCell(28,x,y);
  if(wallAt(*cell->cell,c.direction)==9) {gate=c;break;}
 }
 check(bool(gate),"Original Shoot gate fixture absent");
 for(unsigned wall:{9u,3u,6u}) {
  auto saved=source.service();saved.camera=*gate;saved.journey->random=XeenJourneyRandomState{1,1,0};
  for(auto &a:*saved.journey->vertigoActors) {
   a.x=a.y=-128;a.hp=0;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
  }
  auto &a=saved.journey->vertigoActors->at(35);a.x=gate->x+dx[unsigned(gate->direction)];
  a.y=gate->y+dy[unsigned(gate->direction)];a.hp=1;a.activated=true;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
  if(wall!=9) {auto candidate=original.transitionCandidate();candidate->setBarrier(*gate,wall,wall==6);
   saved.barriers=candidate->sessionState().barriers();}
  Domain d(source,saved);tape={{1,2,1},{1,2,1},{1,2,1},{1,20,19},{1,50,50}};cursor=0;taped=true;
  check(d.flow->handle(ShootAction{}),"Original changed-wall Shoot begins");d.present();
  for(unsigned n=0;n<50 && d.party.encounterContext->minutes==saved.journey->context->minutes;++n) {
   d.now+=100;d.flow->idle();d.flow->holdJourneyFrame();d.present();
  }
  taped=false;
  check(d.party.encounterContext->minutes==saved.journey->context->minutes+1 && cursor==(wall==9?0u:5u),
   "Original closed/Bashed/unlocked Shoot charge or RNG differs");
  check(d.world.sessionState().regionalActors(28).at(35).hp==(wall==9?1:0),
   "Original Shoot ignored effective barrier geometry");
  if(wall!=9) {
   for(unsigned n=0;n<50 && !d.flow->canSave();++n) {d.now+=100;d.flow->idle();d.flow->holdJourneyFrame();d.present();}
   check(d.flow->canSave(),"Settled indoor lethal volley did not return to quiet save");
   const auto settled=d.save();Domain restored(source,settled);
   check(XeenSaveFormat::encode(settled)==XeenSaveFormat::encode(restored.save()),
    "Settled indoor ranged save/load replayed HP, XP, barriers, time or RNG");
  }
 }
 std::cout<<"Original closed/Bashed/unlocked geometry Shoot and settled save round-trip PASS\n";
}
void blockReset(Source &source) {
 Domain original(source);auto saved=original.save();
 // Isolate the two original Ogres for Block/turn bookkeeping; these are
 // explicitly artificial rule arrangements, never paid-route evidence.
 for(auto &a:saved.journey->actors)if(a.id.recordIndex!=14 && a.id.recordIndex!=15) {
  a.x=a.y=-128;a.hp=0;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
 }
 for(auto id:kXeenCombatOwners){saved.characters[id].currentHp=1000;saved.characters[id].conditions[8]=id!=0;}
 auto &a=saved.journey->actors[14],&b=saved.journey->actors[15];a.x=8;a.y=11;a.activated=true;b.x=7;b.y=11;b.activated=true;
 Domain d(source,saved);
 d.flow->journeyAction(d.flow->ticket(),XeenEncounterAction::Forward);
 if(d.flow->state().phase()==XeenEncounterPhase::Exploring)d.flow->journeyPulse(d.flow->ticket());
 check(d.flow->state().phase()==XeenEncounterPhase::Engaged,"Artificial first Toad contact");
 check(d.flow->attachJourney(d.flow->ticket(),[]{}),"Artificial grouped combat attaches");auto &combat=*d.flow->combat();
 attack(combat,toadTape(d.party,true,1,0,false));
 check(combat.pending()==XeenCombatWork::Round,"Owed movement before ready");combat.service(combat.ticket());
 check(combat.phase()==XeenCombatPhase::PlayerReady && combat.participant()==0 && combat.contacts()[1],"Second Toad joins before first player frame");
 combat.command(combat.ticket(),XeenCombatCommand::Block);
 attack(combat,toadTape(d.party,true,19,8,true));
 for(auto id:kXeenCombatOwners)check(d.party.roster.at(id).conditions[8]==1,"All asleep after Block");
 const auto minutes=d.party.encounterContext->minutes;const auto rng=d.world.sessionState().journeyRandom();
 check(combat.service(combat.ticket()).status==XeenCombatStatus::Pending,"Bounded all-asleep inner service");
 check(d.party.encounterContext->minutes==minutes && d.world.sessionState().journeyRandom()==rng,"Inner reset has no time/movement draws");
 attack(combat,toadTape(d.party,false,0,0,false));
 check(!d.party.roster.at(0).conditions[8] && combat.pending()==XeenCombatWork::Enemy,"Wake before following enemy");
 const auto ac=XeenCharacterRules::combatArmorClass(d.party.roster.at(0),*d.party.roster.combatInputs(0),{610});
 const unsigned parameter=8,roll=13;check(ac==13,"Literal original AC discriminator");
 auto miss=toadTape(d.party,true,roll,parameter,true);miss.resize(3);
 const auto hp=d.party.roster.at(0).currentHp;attack(combat,miss);check(d.party.roster.at(0).currentHp==hp,"Block survives Sleep/inner reset/wake");
 check(combat.phase()==XeenCombatPhase::PlayerReady,"Awake player resumes");
 const auto turnTime=d.party.encounterContext;const auto turnRandom=combat.random().continuation();
 XeenCombatRotationTestAccess::arm(combat);
 check(combat.rotate(combat.ticket(),NavigationAction::TurnLeft).status==XeenCombatStatus::Pending,"Blocked player's turn flush starts");
 check(combat.service(combat.ticket()).status==XeenCombatStatus::Advanced && combat.participant()==0 && d.party.encounterContext==turnTime && combat.random().continuation()==turnRandom,"Turn flush charged or consumed blocked player");
 check(combat.rotate(combat.ticket(),NavigationAction::TurnRight).status==XeenCombatStatus::Advanced,"Blocked player's facing returns");
 check(combat.result().blockedMembers==1,"Combat rotations cleared the retained Block flag");
 combat.command(combat.ticket(),XeenCombatCommand::Block);
 combat.service(combat.ticket()); // ordinary reset clears Block before fast enemy
 attack(combat,toadTape(d.party,true,roll,parameter,false));check(d.party.roster.at(0).currentHp==hp-3,"Ordinary round clears Block; identical threshold hits");
 std::cout<<"ARTIFICIAL Block -> Sleep -> inner reset -> wake retained Block; ordinary-round clearing PASS AC="<<ac<<" roll="<<roll<<" parameter=8\n";
}
// Reuse the original archive owner: nested asset runtimes share SearchMan.
XeenGameplayServices disengagementServices(Source &,combat_gameplay_test::Harness &);
#include "XeenConsequenceReviewControls.h"
#include "XeenConsequencePhysicalControls.h"
#include "XeenDisengagementTestControls.h"
#ifndef MMODERN_M45_COLD_CACHE
#include "XeenFreshStartControls.h"
#endif
#ifdef MMODERN_M45_COLD_CACHE
#include "XeenResourceColdOriginalControls.h"
#endif
}
using namespace consequence_controls;
#ifdef MMODERN_M45_COLD_CACHE
int main(int argc,char **argv){try{
 check(argc==2,"usage: mmodern_resource_cold_original <installation>");
 const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation");
 Source source(*installation);source.signature=XeenSaveFile::fingerprint(*installation);
 disengagementFinishPresentation(source,false);disengagementFinishPresentation(source,true);coldVertigo(source);
 std::cout<<"Cold north-edge combat, occupied Run and Vertigo PASS\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
#else
int main(int argc,char **argv){try{
 check(argc==2 || argc==3,"usage: mmodern_consequence_original <installation> [artificial-pending-item-save]");
 const auto i=xeenTestInstallationDetector().detect(argv[1]);check(bool(i),"Original installation");
 Source source(*i);source.signature=XeenSaveFile::fingerprint(*i);
 freshSaveControls(source);titlePublicationControls(source);freshFailureControls(source);freshDifficultyHits(source);
 if(std::getenv("MMODERN_M49_IMPACT_ONLY")) {
  chargedWait(source);stagedVolleyDefeat(source);stagedRotationVolley(source);
  zeroHitVolley(source);shootOrder(source);shootLethalPreparation(source);indoorShoot(source);indoorChangedShoot(source);blockReset(source);
  std::cout<<"M49 original-resource impact/rotation controls PASS\n";return 0;
 }
 if(std::getenv("MMODERN_M34_FINISH_PRESENTATION_ONLY")){disengagementFinishPresentation(source);return 0;}
 reviewControls(source);disengagementControls(source);appearanceResources(source);shootOrder(source);shootLethalPreparation(source);blockReset(source);
 combatPublicationFaults(source);restoreConsequences(source,argc==3?std::optional<std::filesystem::path>{XeenSaveFile::resolve(argv[2],argv[1])}:std::nullopt);
 journey_resources_test::run([&]{return XeenPartyLoader().loadFromResources(source.chr,source.pty);},
 source.setup(),
 [&](auto id){return source.maps.loadGeometryMap(source.assets,id);},[&](auto id){return source.maps.loadObjects(source.assets,id);},source.signature,source.resources());
 std::cout<<"312 content-14 fresh/restored retained-resource controls PASS\n";shootRevalidation(source);physicalPresentationControls(source,std::filesystem::path(argv[1]));return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

#endif
