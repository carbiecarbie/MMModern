#include "XeenJourneyTestSupport.h"
#include "XeenExpeditionTestSupport.h"
#include "XeenRegionalTestSupport.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "XeenCombatGameplayTestSupport.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenCombatRules.h"
#include <limits>
#include <zlib.h>
#include <iostream>
using namespace journey_test;
namespace expedition_test {
std::vector<Draw> tape;
unsigned cursor=0;
bool taped=false;
std::optional<std::uint32_t> realDraw(XeenCombatRandom *,std::uint32_t,std::uint32_t) asm("__real__ZN7mmodern16XeenCombatRandom4drawEjj");
std::optional<std::uint32_t> wrappedDraw(XeenCombatRandom *,std::uint32_t,std::uint32_t) asm("__wrap__ZN7mmodern16XeenCombatRandom4drawEjj");
std::optional<std::uint32_t> wrappedDraw(XeenCombatRandom *r,std::uint32_t lo,std::uint32_t hi) {
 auto original=realDraw(r,lo,hi);if(!taped)return original;
 check(cursor<tape.size(),"literal tape exhausted");auto d=tape[cursor++];if(d.lo!=lo||d.hi!=hi)throw std::runtime_error("literal request endpoints at "+std::to_string(cursor)+" expected "+std::to_string(d.lo)+".."+std::to_string(d.hi)+" got "+std::to_string(lo)+".."+std::to_string(hi));return d.raw?std::optional<std::uint32_t>{}:d.value;
}
struct Tape { explicit Tape(std::vector<Draw> values){tape=std::move(values);cursor=0;taped=true;}~Tape(){taped=false;} };
const auto signature=regional_test::signature();
auto monsters(){auto m=regional_test::statistics();m[9]=expedition_fixture::monsters()[9];regional_test::fingerprint(m[9],0x5002c318);return m;}
auto objects(XeenMapIdentity id){auto o=regional_test::objects(id);if(id==XeenMapIdentity(23))for(unsigned i:{16u,17u,18u})o.entities.monsters[i].resourceId=9;return o;}
auto resources(){auto r=regional_test::resources();r.loadMonsterStatistics=monsters;return r;}
struct PauseTape {bool old=taped;PauseTape(){taped=false;}~PauseTape(){taped=old;}};
XeenSaveSnapshot initial(){PauseTape pause;
 auto saved=regional_test::snapshot();auto bytes=chr();for(unsigned i=0;i<30;++i)bytes[i*354+32]=14;
 saved.characters=XeenPartyLoader().loadFromResources(bytes,pty()).roster.characters();
 for(unsigned i=0;i<30;++i){saved.characters[i].learnedSpells=XeenCharacterFormat::parseLearnedSpells(bytes,i);saved.journey->supplements[i]={static_cast<std::uint8_t>(i),XeenCharacterFormat::parseCombatInputs(bytes,i,true,true,true)};}
 constexpr unsigned levels[]{3,3,3,4,3,3};for(unsigned i=0;i<6;++i){auto &c=saved.characters[kXeenCombatOwners[i]];c.permanentLevel=levels[i];c.temporaryLevel=0;}
 for(unsigned i:{16u,17u,18u})saved.journey->actors[i].hp=30;
 saved.journey->context->minutes=480;return saved;
}
struct Domain {
 XeenWorld w{regional_test::map,objects};XeenPartyState p;XeenCamera c;XeenGameFlags f;
 XeenEventPresenter::Clock clock=[]{return 0;};std::unique_ptr<XeenEncounterFlow> flow;
 explicit Domain(const std::optional<XeenSaveSnapshot> &saved={}) {PauseTape pause;
  XeenSaveState::restoreBeforeGameplay(saved?*saved:initial(),resources(),p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
  flow=std::make_unique<XeenEncounterFlow>(w,p,c,f,clock,XeenJourneyRestoreTag{});present();
 }
 void present(){if(w.sessionState().journeyActivity()==XeenJourneyActivity::Presentation)check(flow->prepareJourneyFrame(flow->ticket(),[] {})&&flow->presentJourney(flow->ticket()),"regional group frame");}
 XeenSaveSnapshot save(){return XeenSaveState::capture(signature,p,c,f,w);}
 XeenCombat &engage(){check(flow->journeyAction(flow->ticket(),XeenEncounterAction::Wait).outcome==XeenEncounterOutcome::Engaged,"group contact");check(flow->attachJourney(flow->ticket(),[] {}),"group attachment");return *flow->combat();}
};
XeenSaveSnapshot group(std::initializer_list<unsigned> ids) {
 Domain original;auto s=original.save();s.camera={23,4,14,XeenDirection::East};
 for(auto &a:s.journey->actors){if(std::find(ids.begin(),ids.end(),a.id.recordIndex)!=ids.end()){a.x=5;a.y=14;a.activated=true;}else{a.x=a.y=-128;a.hp=0;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;}}
 for(auto id:kXeenCombatOwners)s.characters[id].currentHp=200;
 return s;
}
XeenCombatResult action(XeenCombat &c,Command cmd){auto r=c.command(c.ticket(),cmd);for(unsigned n=0;c.pending()==Work::Action&&n<100;++n)r=c.service(c.ticket());return r;}
void block(XeenCombat &c){while(c.phase()==Phase::PlayerReady)action(c,Command::Block);check(c.pending()==Work::Enemy,"ordered enemy participant");}
void groups(){for(auto ids:{std::initializer_list<unsigned>{16},{9},{9,16},{17,18},{16,17,18}}){Domain d(group(ids));auto &c=d.engage();unsigned slot=0;for(auto id:ids)check(c.contacts()[slot++]==XeenMonsterIdentity{23,id},"original contact order");for(unsigned row=0;row<slot;++row){auto old=c.ticket();auto rng=*d.w.sessionState().journeyRandom();check(c.selectTarget(old,row).status==Status::Advanced,"select occupied row");check(c.command(old,Command::Attack).status==Status::Stale,"old selection refuses attack");check(rng==*d.w.sessionState().journeyRandom(),"selection consumes no RNG");}check(c.selectTarget(c.ticket(),3).status==Status::Refused,"invalid row");check(!d.flow->journeyQuiet(),"group unsavable");}
 Domain d(group({9,16}));auto &c=d.engage();for(unsigned n=0;n<50&&d.w.sessionState().actors()[9].hp>0;++n){if(c.phase()==Phase::PlayerReady)action(c,Command::Attack);else c.service(c.ticket());}check(d.w.sessionState().actors()[9].hp==0 && c.contacts()[0]==XeenMonsterIdentity{23,16},"lethal compaction");check(c.phase()!=Phase::VictoryAwaitingEnd,"partial lethal is not group End");check(d.w.sessionState().actors()[16].hp==30,"surviving HP preserved");
}
std::vector<Draw> critical(){return {{1,20,20},{1,4,4},{1,4,4},{1,24,24},{1,5,5},{1,4,4},{1,4,4},{1,24,24}};}
void disease(){
 auto s=group({16});s.characters[1].armor={};s.characters[1].accessories={};
 Domain d(s);auto &c=d.engage();block(c);auto values=critical();auto twice=values;twice.insert(twice.end(),values.begin(),values.end());Tape t(twice);
 const auto before=*d.w.sessionState().journeyRandom();auto r=c.service(c.ticket());check(r.damage==16&&r.injuryCount==2&&d.p.roster.at(1).conditions[4]==2&&c.pending()==Work::Enemy,"first Zombie resource attack");check(d.w.sessionState().journeyRandom()->count==before.count+8,"first attack cursor publication");r=c.service(c.ticket());check(r.damage==16&&d.p.roster.at(1).conditions[4]==4&&cursor==16,"two literal Zombie attacks");check(d.p.roster.at(1).currentHp==168,"four applications on same awake target");
 for(auto save:{4u,5u}){auto highAc=s;highAc.journey->supplements[1].inputs.temporaryAc=10;Domain d2(highAc);auto &c2=d2.engage();block(c2);Tape equality({{1,20,20},{1,4,1},{1,4,1},{1,24,save},{1,5,1}});auto r2=c2.service(c2.ticket());check(r2.status!=Status::Failed&&d2.p.roster.at(1).conditions[4]==(save==4?0:1),"physical save equality");}
 s.characters[1].conditions[4]=255;Domain overflow(s);auto &co=overflow.engage();block(co);const auto state=*overflow.w.sessionState().journeyRandom();Tape excess(critical());check(co.service(co.ticket()).status==Status::Failed,"Disease 256 refusal");check(overflow.p.roster.at(1).currentHp==200&&overflow.p.roster.at(1).conditions[4]==255&&*overflow.w.sessionState().journeyRandom()==state,"entire resource attack rejected");
}
void failure(){auto s=group({16});s.characters[1].armor={};s.characters[1].accessories={};Domain d(s);auto &c=d.engage();block(c);Tape t(critical());check(c.service(c.ticket()).status!=Status::Failed,"attack1 publishes");auto state=*d.w.sessionState().journeyRandom();check(c.service(c.ticket()).status==Status::Failed,"attack2 fails");check(d.p.roster.at(1).currentHp==184&&*d.w.sessionState().journeyRandom()==state&&!d.flow->journeyQuiet(),"attack1 retained through attack2 failure");
 for(unsigned field=0;field<3;++field){Domain altered(group({16}));auto &combat=altered.engage();combat.setProbe([&]{auto &v=const_cast<XeenCombatInputs &>(*altered.p.roster.combatInputs(29));if(field==0)v.luck.reset();else if(field==1)++v.luck->permanent;else ++v.luck->temporary;});check(action(combat,Command::Attack).status==Status::Failed,"inactive Luck guarded on callbacks");}
}
void injuriesAndDefeat(){
	// The combat publication callback uses the same owner write boundary even
	// when the visible preimage is restored before the callback returns.
	for(unsigned field=0;field<3;++field) {
		Domain changed(group({16}));auto &combat=changed.engage();
		combat.setProbe([&] {
			if(field==0){auto &hp=changed.p.roster.at(0).currentHp;++hp;--hp;}
			else if(field==1){auto &r=const_cast<XeenMutableOptional<XeenJourneyRandomState>&>(changed.w.sessionState().journeyRandom());++r->count;--r->count;}
			else {auto &s=const_cast<XeenSessionWorldState&>(changed.w.sessionState());const auto before=s;s=XeenSessionWorldState{};s=before;}
		});
		taped=false;
		check(action(combat,Command::Attack).status==Status::Failed && !changed.flow->journeyQuiet(),"combat callback ABA rejected");
	}
 auto s=group({16});s.characters[1].currentHp=1;
 for(auto id:kXeenCombatOwners)if(id!=0&&id!=1){s.characters[id].currentHp=-1;s.characters[id].conditions[12]=1;}
 Domain d(s);auto &c=d.engage();block(c);auto values=critical();values.insert(values.end(),{{0,5,4},{0,0,0},{1,20,1}});Tape t(values);
 auto first=c.service(c.ticket());check(first.status!=Status::Failed&&first.injuryCount==2&&d.p.roster.at(1).currentHp==-15&&!d.p.roster.at(1).canAct(),"critical keeps target through incapacitation");check(first.armorCount>0,"ordered armor breakage at negative HP");auto second=c.service(c.ticket());check(second.status!=Status::Failed&&second.targetOwner==0&&cursor==11,"second attack reselects with singleton fallback request");
 auto only=s;only.characters[0].currentHp=-1;only.characters[0].conditions[12]=1;Domain defeated(only);auto &enemy=defeated.engage();block(enemy);Tape loss(critical());auto before=defeated.p.encounterContext->minutes;check(enemy.service(enemy.ticket()).status==Status::Defeat&&cursor==8&&defeated.p.encounterContext->minutes==before&&!defeated.flow->journeyQuiet(),"defeat suppresses second resource attack and time");
 for(unsigned field=0;field<3;++field){Domain changed(group({16}));auto &combat=changed.engage();auto beforeRandom=*changed.w.sessionState().journeyRandom();combat.setProbe([&]{auto &r=const_cast<XeenMutableOptional<XeenJourneyRandomState>&>(changed.w.sessionState().journeyRandom());if(field==0)++r->state;else if(field==1)++r->count;else r->algorithm=2;});taped=false;check(action(combat,Command::Attack).status==Status::Failed&&changed.w.sessionState().actors()[16].hp==30,"world random fields participate in preimage");}
}
void scheduler(){
 for(unsigned minute:{480u}){
  auto saved=group({16});saved.journey->context->minutes=minute;
  for(auto id:kXeenCombatOwners){saved.journey->supplements[id].inputs.speed={1,0};saved.characters[id].accessories={};}
  Domain d(saved);auto &c=d.engage();check(c.phase()==Phase::PendingEnemy&&c.participant()==6,"enemy-first attachment");
  Tape misses({{1,20,1},{1,20,1},{1,20,1},{1,20,1}});
  c.service(c.ticket());c.service(c.ticket());check(c.phase()==Phase::PlayerReady,"enemy ordinal before player");
  while(c.phase()==Phase::PlayerReady)action(c,Command::Block);
  check(c.pending()==Work::Round,"round exhausted");auto before=*d.w.sessionState().journeyRandom();auto r=c.service(c.ticket());
  {check(c.pending()==Work::Enemy&&cursor==2,"New round selects enemy before movement");c.service(c.ticket());c.service(c.ticket());check(cursor==4&&d.p.encounterContext->minutes==490&&c.pending()==Work::Round,"new-round attacks publish before charge");auto published=*d.w.sessionState().journeyRandom();c.setProbe([]{throw std::runtime_error("Round observation failure");});check(c.service(c.ticket()).status==Status::Failed&&*d.w.sessionState().journeyRandom()==published&&d.p.encounterContext->minutes==490,"Round failure retains selection attacks");}
 }
 // A final-player lethal skips Round even with a live off-contact survivor.
 auto saved=group({9,17});saved.journey->actors[17].x=7;saved.journey->actors[17].y=14;saved.characters[6].permanentLevel=24;saved.journey->supplements[6].inputs.might={50,0};
 Domain d(saved);auto &c=d.engage();for(unsigned i=0;i<5;++i)action(c,Command::Block);
 check(c.participant()==5,"last player selected");std::vector<Draw> hits;for(unsigned i=0;i<4;++i){hits.push_back({1,3,3});hits.push_back({1,20,10});}Tape lethal(hits);
 auto r=action(c,Command::Attack);check(r.status!=Status::Failed&&c.phase()==Phase::VictoryAwaitingEnd,"final-player lethal skips Round");auto minutes=d.p.encounterContext->minutes;auto survivor=d.w.sessionState().actors()[17];check(c.service(c.ticket()).status==Status::Victory&&d.p.encounterContext->minutes==minutes+1&&xeen_state::sameActor(survivor,d.w.sessionState().actors()[17]),"End does not move off-contact survivor");
}
void prefixes(){
 Domain d(group({16}));auto &c=d.engage();std::vector<Draw> values{{1,2,2},{1,2,2},{1,2,2},{1,2,2}};
 for(unsigned i=0;i<70;++i)values.push_back({1,20,20});values.push_back({1,20,1});Tape t(values);
 auto before=*d.w.sessionState().journeyRandom();check(c.command(c.ticket(),Command::Attack).status==Status::Pending,"retained action");check(c.service(c.ticket()).status==Status::Pending&&cursor==64&&*d.w.sessionState().journeyRandom()==before,"64-draw prefix uncommitted");check(c.service(c.ticket()).status!=Status::Failed&&cursor==75&&d.w.sessionState().journeyRandom()->count==before.count+75,"prefix continues without reroll");
 auto saved=group({16});saved.journey->random->count=std::numeric_limits<std::uint64_t>::max()-3;Domain exhausted(saved);auto &e=exhausted.engage();taped=false;auto previous=*exhausted.w.sessionState().journeyRandom();check(action(e,Command::Attack).status==Status::Failed&&*exhausted.w.sessionState().journeyRandom()==previous&&exhausted.w.sessionState().actors()[16].hp==30,"count exhaustion within attack rolls back candidate");
}
void derived(){
 Domain d;auto c=d.p.roster.at(1);auto input=*d.p.roster.combatInputs(1);c.permanentLevel=3;c.endurance={16,0};c.personality={23,0};c.intellect={10,0};c.hasSpells=true;c.race=XeenRace::Human;c.armor={};c.accessories={};c.weapons={};
 const auto speed=XeenCharacterRules::effectivePhysical(c,input,XeenCharacterRules::PhysicalAttribute::Speed,{610});
 for(unsigned severity:{0u,1u,2u,4u,255u}){c.conditions[4]=severity;check(XeenCharacterRules::effectivePhysical(c,input,XeenCharacterRules::PhysicalAttribute::Speed,{610})==speed&&XeenCharacterRules::effectiveLuck(c,input)==14,"Disease does not alter physical inputs");check(c.canAct()&&xeenCombatXpEligible(c.worstCondition()),"Disease eligibility");}
 c.conditions[4]=2;c.currentHp=12;c.currentSp=21;auto hp=XeenCharacterRules::maxHp(c,{610});check(hp==18&&c.currentHp==12&&c.currentSp==21,"Disease maximum with historical current values");c.conditions[13]=1;check(XeenCharacterRules::maxHp(c,{610})==21&&!c.canAct(),"Dead suppresses Disease maximum modifier");input.luck->permanent=std::numeric_limits<int>::max();input.luck->temporary=1;rejects([&]{XeenCharacterRules::effectiveLuck(c,input);});
}
}
int main(){using namespace expedition_test;try{groups();disease();failure();injuriesAndDefeat();scheduler();prefixes();derived();std::cout<<"Regional group, Zombie, failure and Luck controls passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
