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
#include <iostream>
#include "platform/XeenSaveFile.h"
#include "games/xeen/XeenOutdoorScene.h"
using namespace mmodern;
using save_test::check;
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
#define PLAYER_CTOR "_ZN7mmodern27XeenPhysicalPlayerCandidateC1ERKNS_13XeenCharacterERKNS_16XeenCombatInputsERKNS_17XeenMonsterRecordEjjb"
void realPlayer(XeenPhysicalPlayerCandidate *,const XeenCharacter &,const XeenCombatInputs &,const XeenMonsterRecord &,unsigned,unsigned,bool) asm("__real_" PLAYER_CTOR);
void wrappedPlayer(XeenPhysicalPlayerCandidate *,const XeenCharacter &,const XeenCombatInputs &,const XeenMonsterRecord &,unsigned,unsigned,bool) asm("__wrap_" PLAYER_CTOR);
void wrappedPlayer(XeenPhysicalPlayerCandidate *self,const XeenCharacter &c,const XeenCombatInputs &i,const XeenMonsterRecord &m,unsigned type,unsigned year,bool shoot){
 auto operand=m;if(zeroResistanceFixture && shoot)operand.raw[40]=100;realPlayer(self,c,i,operand,type,year,shoot);
}
struct Source {
 XeenAssetSource assets;XeenMapLoader maps;
 std::vector<std::uint8_t> chr,pty;std::vector<XeenMonsterRecord> mon;XeenEventFile evt;
 XeenSaveResourceSignature signature{{1,2},XeenArchiveFingerprint{3,4}};
 explicit Source(const GameInstallation &i):assets(i) {
  chr=assets.readInitialResource("maze.chr");pty=assets.readInitialResource("maze.pty");mon=XeenMonsterFormat::parse(*assets.readCloudsMonsterStatisticsFromDarkArchive());
  XeenEventLoader loader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>>{if(!assets.hasInitialResource(name))return {};return assets.readInitialResource(name);});evt=loader.load(23);
 }
 XeenRegionalManifest manifest(){return [&](const auto &m,const auto &o,const auto &e,const auto &s){xeenValidateRegionalManifest(m,o,e,s,assets.readInitialResource("maze0023.dat"),assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));};}
};
struct Domain {
 Source &source;XeenWorld world;XeenPartyState party;XeenCamera camera{23,9,11,XeenDirection::West};XeenGameFlags flags;
 std::uint64_t now=0;XeenEventPresenter::Clock clock=[this]{return now;};std::unique_ptr<XeenEncounterFlow> flow;
 Domain(Source &s,const std::optional<XeenSaveSnapshot> &saved={}):source(s),world([&](auto id){return s.maps.loadGeometryMap(s.assets,id);},[&](auto id){return s.maps.loadObjects(s.assets,id);}) {
  if(saved) {
   XeenSaveState::Resources r{s.signature,{},[&](auto){return s.evt;},{},{},[&]{return s.mon;},s.manifest()};
   XeenSaveState::restoreBeforeGameplay(*saved,r,party,camera,flags,world,[](auto &,const auto &,const auto &,const auto &){});
   flow=std::make_unique<XeenEncounterFlow>(world,party,camera,flags,clock,XeenJourneyRestoreTag{});
  }else{
   party=XeenPartyLoader().loadFromResources(s.chr,s.pty);
   flow=std::make_unique<XeenEncounterFlow>(world,party,camera,flags,clock,XeenJourneySetup{s.chr,XeenGameplayContextFormat::parse(s.pty),s.mon,s.evt,1,4,s.manifest(),XeenCharacterFormat::parseMonsterPurse(s.pty)});
  }
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
 std::vector<XeenCombatRandom::Draw> draws;
 for(unsigned i=0;i<6;++i){const auto id=kXeenCombatOwners[i];
  if(i==0 && awake){draws.push_back({1,20,roll});if(roll==1)continue;draws.push_back({1,8,parameter});}
  for(unsigned n=0;n<3;++n)draws.push_back({1,8,1});
  const auto &c=p.roster.at(id);const auto &in=*p.roster.combatInputs(id);
  const auto v=XeenCharacterRules::physicalBonus(XeenCharacterRules::effectiveLuck(c,in))+c.currentLevel();
  constexpr unsigned intervals[]{23,24,22,27,24,25};if(unsigned(v+20)!=intervals[i])throw std::runtime_error("Original save interval owner "+std::to_string(i)+" value "+std::to_string(v+20));
  draws.push_back({1,intervals[i],i==0 && !sleepFirst?1:intervals[i]});
 }
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
   const auto chars=d.party.roster.characters();const auto actors=d.world.sessionState().actors();
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
   const auto chars=d.party.roster.characters();const auto actors=d.world.sessionState().actors();const auto rng=d.world.sessionState().journeyRandom();
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
  bool failed=false;try{Domain d(source,bad);}catch(const std::exception &){failed=true;}check(failed,"Malformed/currently collectable consequence restore refused");
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
 for(unsigned service=0;cursor<tape.size() && service<100;++service){d.now+=100;d.flow->idle();d.present();}
 taped=false;
 const auto finishVisuals=[](Domain &v,unsigned distance){
  std::vector<std::pair<unsigned,unsigned>> seen;
  for(unsigned n=0;n<50 && v.party.encounterContext->minutes==480;++n){v.now+=100;v.flow->idle();v.flow->holdJourneyFrame();v.present();
   if(const auto p=v.flow->appearance().projectile){check(!p->enemy&&!p->source,"Player projectile never fabricates monster identity");const auto item=std::make_pair(p->lane,p->row);if(seen.empty()||seen.back()!=item)seen.push_back(item);}}
  std::vector<std::pair<unsigned,unsigned>> expected;for(unsigned lane:{0u,2u})for(unsigned row=0;row<=distance;++row)expected.emplace_back(lane,row);
  check(seen==expected,"Exactly one outward projectile per shooter, independent of miss attempts");
  check(v.party.encounterContext->minutes==490&&v.flow->state().pending()==3,"Visuals preserve charge and owed opportunity");
 };
 check(cursor==18 && d.world.sessionState().journeyRandom()->count==18,"Two misses then two hits in active-member order");
 check(d.world.sessionState().actors()[9].hp==25 && d.world.sessionState().actors()[8].hp==7 && d.world.sessionState().actors()[7].hp==25,"Miss advances original target order; each successful shooter spends once");
 check(d.party.encounterContext->minutes==480 && !d.flow->canSave(),"Volley publishes attempts before owed charge; capture remains excluded");
 finishVisuals(d,2);
 Domain misses(source,saved);tape.clear();for(unsigned shot=0;shot<6;++shot){for(unsigned die=0;die<3;++die)tape.push_back({1,2,1});tape.push_back({1,20,1});}
 cursor=0;taped=true;check(misses.flow->handle(ShootAction{}),"All-miss volley begins");misses.present();
 for(unsigned service=0;cursor<tape.size() && service<100;++service){misses.now+=100;misses.flow->idle();misses.present();}
 taped=false;check(cursor==24 && misses.world.sessionState().journeyRandom()->count==24,"Both shooters traverse all three rows on misses");
 for(unsigned id:{7u,8u,9u})check(misses.world.sessionState().actors()[id].hp==25,"All-miss volley leaves actor wounds unchanged");
 finishVisuals(misses,3);
 check(misses.world.sessionState().journeyRandom()->count==24,"All-miss visuals consume no RNG");
 std::cout<<"ARTIFICIAL two-shooter miss continuation, all-miss rows and spent-hit target ordering PASS\n";
}
void blockReset(Source &source) {
 Domain original(source);auto saved=original.save();
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
 auto miss=toadTape(d.party,true,roll,parameter,true);miss.erase(miss.begin()+2,miss.begin()+6);
 const auto hp=d.party.roster.at(0).currentHp;attack(combat,miss);check(d.party.roster.at(0).currentHp==hp,"Block survives Sleep/inner reset/wake");
 check(combat.phase()==XeenCombatPhase::PlayerReady,"Awake player resumes");combat.command(combat.ticket(),XeenCombatCommand::Block);
 combat.service(combat.ticket()); // ordinary reset clears Block before fast enemy
 attack(combat,toadTape(d.party,true,roll,parameter,false));check(d.party.roster.at(0).currentHp==hp-3,"Ordinary round clears Block; identical threshold hits");
 std::cout<<"ARTIFICIAL Block -> Sleep -> inner reset -> wake retained Block; ordinary-round clearing PASS AC="<<ac<<" roll="<<roll<<" parameter=8\n";
}
#include "XeenConsequenceReviewControls.h"
#include "XeenConsequencePhysicalControls.h"
}
using namespace consequence_controls;
int main(int argc,char **argv){try{check(argc==2 || argc==3,"usage: mmodern_consequence_original <installation> [artificial-pending-item-save]");const auto i=XeenInstallationDetector().detect(argv[1]);check(bool(i),"Original installation");Source source(*i);source.signature=XeenSaveFile::fingerprint(*i);reviewControls(source);appearanceResources(source);shootOrder(source);blockReset(source);combatPublicationFaults(source);restoreConsequences(source,argc==3?std::optional<std::filesystem::path>{XeenSaveFile::resolve(argv[2],argv[1])}:std::nullopt);
 journey_resources_test::run([&]{return XeenPartyLoader().loadFromResources(source.chr,source.pty);},
 XeenJourneySetup{source.chr,XeenGameplayContextFormat::parse(source.pty),source.mon,source.evt,1,4,source.manifest(),XeenCharacterFormat::parseMonsterPurse(source.pty)},
 [&](auto id){return source.maps.loadGeometryMap(source.assets,id);},[&](auto id){return source.maps.loadObjects(source.assets,id);},source.signature);
 std::cout<<"312 contract-4 fresh/restored retained-resource controls PASS\n";shootRevalidation(source,std::filesystem::path(argv[1]));physicalPresentationControls(source,std::filesystem::path(argv[1]));return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
