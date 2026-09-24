#include "XeenJourneyTestSupport.h"
#include "XeenExpeditionTestSupport.h"
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
 check(cursor<tape.size(),"literal tape exhausted");auto d=tape[cursor++];check(d.lo==lo&&d.hi==hi,"literal request endpoints");return d.raw?std::optional<std::uint32_t>{}:d.value;
}
struct Tape { explicit Tape(std::vector<Draw> values){tape=std::move(values);cursor=0;taped=true;}~Tape(){taped=false;} };
using expedition_fixture::terrain; using expedition_fixture::objects; using expedition_fixture::events; using expedition_fixture::monsters;
const XeenSaveResourceSignature signature{{1,2},XeenArchiveFingerprint{3,4}};
struct Domain {
 XeenWorld w{[](auto){return terrain();},[](auto){return objects();}};
 XeenPartyState p;XeenCamera c=xeenJourneyContent(2).entry;XeenGameFlags f;
 XeenEventPresenter::Clock clock=[]{return 0;};std::unique_ptr<XeenEncounterFlow> flow;
 explicit Domain(const std::optional<XeenSaveSnapshot> &saved={}) {
  if(saved){XeenSaveState::Resources r{signature,{},[](auto){return events();},{},{},[]{return monsters();}};XeenSaveState::restoreBeforeGameplay(*saved,r,p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});flow=std::make_unique<XeenEncounterFlow>(w,p,c,f,clock,XeenJourneyRestoreTag{});}
  else {auto bytes=chr();for(unsigned i=0;i<30;++i)bytes[i*354+32]=14;p=XeenPartyLoader().loadFromResources(bytes,pty());auto mon=monsters();auto evt=events();flow=std::make_unique<XeenEncounterFlow>(w,p,c,f,clock,XeenJourneySetup{bytes,XeenGameplayContextFormat::parse(pty()),mon,evt,1,2});}
  present();
 }
 void present(){check(flow->prepareJourneyFrame(flow->ticket(),[] {})&&flow->presentJourney(flow->ticket()),"domain frame");}
 XeenSaveSnapshot save(){return XeenSaveState::capture(signature,p,c,f,w);}
 XeenCombat &engage(){check(flow->journeyAction(flow->ticket(),XeenEncounterAction::Wait).outcome==XeenEncounterOutcome::Engaged,"group contact");check(flow->attachJourney(flow->ticket(),[] {}),"group attachment");return *flow->combat();}
};
void profileBinding(){
 Domain positive;const auto &policy=xeenJourneyContent(2);for(unsigned i=0;i<policy.count;++i){const auto admission=policy.actor(policy.records[i]);const auto &actor=positive.w.sessionState().actors()[admission.record];check(actor.original.resourceId==admission.resourceId&&actor.statistics&&actor.statistics->image()==admission.profileImage,"fresh descriptor profile positive control");}
 const auto freshMismatch=[&](bool zombieAsSkeleton){XeenWorld world([](auto){return terrain();},[](auto){return objects();});auto bytes=chr();for(unsigned i=0;i<30;++i)bytes[i*354+32]=14;XeenPartyState party=XeenPartyLoader().loadFromResources(bytes,pty());XeenCamera camera=policy.entry;XeenGameFlags flags;XeenEventPresenter::Clock clock=[]{return 0;};auto statistics=monsters();if(zombieAsSkeleton)statistics[9]=statistics[8];else statistics[8]=statistics[9];
  rejects([&]{XeenEncounterFlow flow(world,party,camera,flags,clock,XeenJourneySetup{bytes,XeenGameplayContextFormat::parse(pty()),statistics,events(),1,2});});
  check(world.sessionState().encounterMarked()&&!world.sessionState().encounterInitialized()&&world.sessionState().actors().empty()&&world.sessionState().journeyActivity()==XeenJourneyActivity::Failed&&!world.sessionState().journeyRandom()&&!party.roster.combatMarked()&&!party.encounterContext,"fresh profile mismatch leaves only the irreversible failure marker");
 };
 freshMismatch(true);freshMismatch(false);
 auto saved=positive.save();Domain restored(saved);check(restored.w.sessionState().actors()[9].statistics->image()==8&&restored.w.sessionState().actors()[17].statistics->image()==9,"restored descriptor profile positive control");
 const auto restoreMismatch=[&](bool zombieAsSkeleton){XeenWorld world([](auto){return terrain();},[](auto){return objects();});XeenPartyState party;XeenCamera camera;XeenGameFlags flags;XeenEventPresenter::Clock clock=[]{return 0;};auto statistics=monsters();if(zombieAsSkeleton)statistics[9]=statistics[8];else statistics[8]=statistics[9];XeenSaveState::Resources resources{signature,{},[](auto){return events();},{},{},[statistics]{return statistics;}};bool presented=false;
  rejects([&]{XeenSaveState::restoreBeforeGameplay(saved,resources,party,camera,flags,world,[&](auto &,const auto &,const auto &,const auto &){presented=true;});});
  check(!presented&&!world.hasEncounterState()&&!party.roster.combatMarked()&&!party.encounterContext&&xeen_state::sameCamera(camera,XeenCamera{})&&flags.values()==XeenGameFlags().values(),"profile mismatch restoration leaves destination owners unchanged before presentation");
  rejects([&]{XeenEncounterFlow unavailable(world,party,camera,flags,clock,XeenJourneyRestoreTag{});});
 };
 restoreMismatch(true);restoreMismatch(false);
}
XeenSaveSnapshot group(std::initializer_list<unsigned> ids) {
 Domain original;auto s=original.save();s.camera={20,4,14,XeenDirection::East};
 for(auto &a:s.journey->actors){if(std::find(ids.begin(),ids.end(),a.id.recordIndex)!=ids.end()){a.x=5;a.y=14;a.activated=true;}else{a.x=a.y=-128;a.hp=0;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;}}
 for(auto id:kXeenCombatOwners)s.characters[id].currentHp=200;
 return s;
}
XeenCombatResult action(XeenCombat &c,Command cmd){auto r=c.command(c.ticket(),cmd);for(unsigned n=0;c.pending()==Work::Action&&n<100;++n)r=c.service(c.ticket());return r;}
void block(XeenCombat &c){while(c.phase()==Phase::PlayerReady)action(c,Command::Block);check(c.pending()==Work::Enemy,"ordered enemy participant");}
void groups(){for(auto ids:{std::initializer_list<unsigned>{25},{9},{9,25},{17,18},{17,18,25}}){Domain d(group(ids));auto &c=d.engage();unsigned slot=0;for(auto id:ids)check(c.contacts()[slot++]==XeenMonsterIdentity{20,id},"original contact order");for(unsigned row=0;row<slot;++row){auto old=c.ticket();auto rng=*d.w.sessionState().journeyRandom();check(c.selectTarget(old,row).status==Status::Advanced,"select occupied row");check(c.command(old,Command::Attack).status==Status::Stale,"old selection refuses attack");check(rng==*d.w.sessionState().journeyRandom(),"selection consumes no RNG");}check(c.selectTarget(c.ticket(),3).status==Status::Refused,"invalid row");check(!d.flow->journeyQuiet(),"group unsavable");}
 Domain d(group({9,25}));auto &c=d.engage();for(unsigned n=0;n<50&&d.w.sessionState().actors()[9].hp>0;++n){if(c.phase()==Phase::PlayerReady)action(c,Command::Attack);else c.service(c.ticket());}check(d.w.sessionState().actors()[9].hp==0 && c.contacts()[0]==XeenMonsterIdentity{20,25},"lethal compaction");check(c.phase()!=Phase::VictoryAwaitingEnd,"partial lethal is not group End");check(d.w.sessionState().actors()[25].hp==30,"surviving HP preserved");
}
std::vector<Draw> critical(){return {{1,20,20},{1,4,4},{1,4,4},{1,24,24},{1,5,5},{1,4,4},{1,4,4},{1,24,24}};}
void disease(){
 auto s=group({25});s.characters[1].armor={};s.characters[1].accessories={};
 Domain d(s);auto &c=d.engage();block(c);auto values=critical();auto twice=values;twice.insert(twice.end(),values.begin(),values.end());Tape t(twice);
 const auto before=*d.w.sessionState().journeyRandom();auto r=c.service(c.ticket());check(r.damage==16&&r.injuryCount==2&&d.p.roster.at(1).conditions[4]==2&&c.pending()==Work::Enemy,"first Zombie resource attack");check(d.w.sessionState().journeyRandom()->count==before.count+8,"first attack cursor publication");r=c.service(c.ticket());check(r.damage==16&&d.p.roster.at(1).conditions[4]==4&&cursor==16,"two literal Zombie attacks");check(d.p.roster.at(1).currentHp==168,"four applications on same awake target");
 for(auto save:{4u,5u}){auto highAc=s;highAc.journey->supplements[1].inputs.temporaryAc=10;Domain d2(highAc);auto &c2=d2.engage();block(c2);Tape equality({{1,20,20},{1,4,1},{1,4,1},{1,24,save},{1,5,1}});auto r2=c2.service(c2.ticket());check(r2.status!=Status::Failed&&d2.p.roster.at(1).conditions[4]==(save==4?0:1),"physical save equality");}
 s.characters[1].conditions[4]=255;Domain overflow(s);auto &co=overflow.engage();block(co);const auto state=*overflow.w.sessionState().journeyRandom();Tape excess(critical());check(co.service(co.ticket()).status==Status::Failed,"Disease 256 refusal");check(overflow.p.roster.at(1).currentHp==200&&overflow.p.roster.at(1).conditions[4]==255&&*overflow.w.sessionState().journeyRandom()==state,"entire resource attack rejected");
}
void failure(){auto s=group({25});s.characters[1].armor={};s.characters[1].accessories={};Domain d(s);auto &c=d.engage();block(c);Tape t(critical());check(c.service(c.ticket()).status!=Status::Failed,"attack1 publishes");auto state=*d.w.sessionState().journeyRandom();check(c.service(c.ticket()).status==Status::Failed,"attack2 fails");check(d.p.roster.at(1).currentHp==184&&*d.w.sessionState().journeyRandom()==state&&!d.flow->journeyQuiet(),"attack1 retained through attack2 failure");
 for(unsigned field=0;field<3;++field){Domain altered(group({25}));auto &combat=altered.engage();combat.setProbe([&]{auto &v=const_cast<XeenCombatInputs &>(*altered.p.roster.combatInputs(29));if(field==0)v.luck.reset();else if(field==1)++v.luck->permanent;else ++v.luck->temporary;});check(action(combat,Command::Attack).status==Status::Failed,"inactive Luck guarded on callbacks");}
}
void injuriesAndDefeat(){
	// The combat publication callback uses the same owner write boundary even
	// when the visible preimage is restored before the callback returns.
	for(unsigned field=0;field<3;++field) {
		Domain changed(group({25}));auto &combat=changed.engage();
		combat.setProbe([&] {
			if(field==0){auto &hp=changed.p.roster.at(0).currentHp;++hp;--hp;}
			else if(field==1){auto &r=const_cast<XeenMutableOptional<XeenJourneyRandomState>&>(changed.w.sessionState().journeyRandom());++r->count;--r->count;}
			else {auto &s=const_cast<XeenSessionWorldState&>(changed.w.sessionState());const auto before=s;s=XeenSessionWorldState{};s=before;}
		});
		taped=false;
		check(action(combat,Command::Attack).status==Status::Failed && !changed.flow->journeyQuiet(),"combat callback ABA rejected");
	}
 auto s=group({25});s.characters[1].currentHp=1;
 for(auto id:kXeenCombatOwners)if(id!=0&&id!=1){s.characters[id].currentHp=-1;s.characters[id].conditions[12]=1;}
 Domain d(s);auto &c=d.engage();block(c);auto values=critical();values.insert(values.end(),{{0,5,4},{0,0,0},{1,20,1}});Tape t(values);
 auto first=c.service(c.ticket());check(first.status!=Status::Failed&&first.injuryCount==2&&d.p.roster.at(1).currentHp==-15&&!d.p.roster.at(1).canAct(),"critical keeps target through incapacitation");check(first.armorCount>0,"ordered armor breakage at negative HP");auto second=c.service(c.ticket());check(second.status!=Status::Failed&&second.targetOwner==0&&cursor==11,"second attack reselects with singleton fallback request");
 auto only=s;only.characters[0].currentHp=-1;only.characters[0].conditions[12]=1;Domain defeated(only);auto &enemy=defeated.engage();block(enemy);Tape loss(critical());auto before=defeated.p.encounterContext->minutes;check(enemy.service(enemy.ticket()).status==Status::Defeat&&cursor==8&&defeated.p.encounterContext->minutes==before&&!defeated.flow->journeyQuiet(),"defeat suppresses second resource attack and time");
 for(unsigned field=0;field<3;++field){Domain changed(group({25}));auto &combat=changed.engage();auto beforeRandom=*changed.w.sessionState().journeyRandom();combat.setProbe([&]{auto &r=const_cast<XeenMutableOptional<XeenJourneyRandomState>&>(changed.w.sessionState().journeyRandom());if(field==0)++r->state;else if(field==1)++r->count;else r->algorithm=2;});taped=false;check(action(combat,Command::Attack).status==Status::Failed&&changed.w.sessionState().actors()[25].hp==30,"world random fields participate in preimage");}
}

void scheduler(){
 for(unsigned minute:{948u,949u}){
  auto saved=group({25});saved.journey->context->minutes=minute;
  for(auto id:kXeenCombatOwners){saved.journey->supplements[id].inputs.speed={1,0};saved.characters[id].accessories={};}
  Domain d(saved);auto &c=d.engage();check(c.phase()==Phase::PendingEnemy&&c.participant()==6,"enemy-first attachment");
  Tape misses({{1,20,1},{1,20,1},{1,20,1},{1,20,1}});
  c.service(c.ticket());c.service(c.ticket());check(c.phase()==Phase::PlayerReady,"enemy ordinal before player");
  while(c.phase()==Phase::PlayerReady)action(c,Command::Block);
  check(c.pending()==Work::Round,"round exhausted");auto before=*d.w.sessionState().journeyRandom();auto r=c.service(c.ticket());
  if(minute==949){check(r.status==Status::SupportStopped&&*d.w.sessionState().journeyRandom()==before&&cursor==2,"959 stops before new-round selection draws");}
  else {check(c.pending()==Work::Enemy&&cursor==2,"958 selects enemy before movement");c.service(c.ticket());c.service(c.ticket());check(cursor==4&&d.p.encounterContext->minutes==958&&c.pending()==Work::Round,"new-round attacks publish before charge");auto published=*d.w.sessionState().journeyRandom();c.setProbe([]{throw std::runtime_error("Round observation failure");});check(c.service(c.ticket()).status==Status::Failed&&*d.w.sessionState().journeyRandom()==published&&d.p.encounterContext->minutes==958,"Round failure retains selection attacks");}
 }
 // A final-player lethal skips Round even with a live off-contact survivor.
 auto saved=group({9,17});saved.journey->actors[1].x=7;saved.journey->actors[1].y=14;saved.characters[6].permanentLevel=24;saved.journey->supplements[6].inputs.might={50,0};
 Domain d(saved);auto &c=d.engage();for(unsigned i=0;i<5;++i)action(c,Command::Block);
 check(c.participant()==5,"last player selected");std::vector<Draw> hits;for(unsigned i=0;i<4;++i){hits.push_back({1,3,3});hits.push_back({1,20,10});}Tape lethal(hits);
 auto r=action(c,Command::Attack);check(r.status!=Status::Failed&&c.phase()==Phase::VictoryAwaitingEnd,"final-player lethal skips Round");auto minutes=d.p.encounterContext->minutes;auto survivor=d.w.sessionState().actors()[17];check(c.service(c.ticket()).status==Status::Victory&&d.p.encounterContext->minutes==minutes+1&&xeen_state::sameActor(survivor,d.w.sessionState().actors()[17]),"End does not move off-contact survivor");
}
void prefixes(){
 Domain d(group({25}));auto &c=d.engage();std::vector<Draw> values{{1,2,2},{1,2,2},{1,2,2},{1,2,2}};
 for(unsigned i=0;i<70;++i)values.push_back({1,20,20});values.push_back({1,20,1});Tape t(values);
 auto before=*d.w.sessionState().journeyRandom();check(c.command(c.ticket(),Command::Attack).status==Status::Pending,"retained action");check(c.service(c.ticket()).status==Status::Pending&&cursor==64&&*d.w.sessionState().journeyRandom()==before,"64-draw prefix uncommitted");check(c.service(c.ticket()).status!=Status::Failed&&cursor==75&&d.w.sessionState().journeyRandom()->count==before.count+75,"prefix continues without reroll");
 auto saved=group({25});saved.journey->random->count=std::numeric_limits<std::uint64_t>::max()-3;Domain exhausted(saved);auto &e=exhausted.engage();taped=false;auto previous=*exhausted.w.sessionState().journeyRandom();check(action(e,Command::Attack).status==Status::Failed&&*exhausted.w.sessionState().journeyRandom()==previous&&exhausted.w.sessionState().actors()[25].hp==30,"count exhaustion within attack rolls back candidate");
}
void derived(){
 Domain d;auto c=d.p.roster.at(1);auto input=*d.p.roster.combatInputs(1);c.permanentLevel=3;c.endurance={16,0};c.personality={20,0};c.intellect={10,0};c.hasSpells=true;c.race=XeenRace::Human;c.armor={};c.accessories={};c.weapons={};
 const auto speed=XeenCharacterRules::effectivePhysical(c,input,XeenCharacterRules::PhysicalAttribute::Speed,{610});
 for(unsigned severity:{0u,1u,2u,4u,255u}){c.conditions[4]=severity;check(XeenCharacterRules::effectivePhysical(c,input,XeenCharacterRules::PhysicalAttribute::Speed,{610})==speed&&XeenCharacterRules::effectiveLuck(c,input)==14,"Disease does not alter physical inputs");check(c.canAct()&&xeenCombatXpEligible(c.worstCondition()),"Disease eligibility");}
 c.conditions[4]=2;c.currentHp=12;c.currentSp=21;auto hp=XeenCharacterRules::maxHp(c,{610});check(hp==18&&c.currentHp==12&&c.currentSp==21,"Disease maximum with historical current values");c.conditions[13]=1;check(XeenCharacterRules::maxHp(c,{610})==21&&!c.canAct(),"Dead suppresses Disease maximum modifier");input.luck->permanent=std::numeric_limits<int>::max();input.luck->temporary=1;rejects([&]{XeenCharacterRules::effectiveLuck(c,input);});
}

void codec(){
 XeenPartyState ordinary;XeenWorld ordinaryWorld([](auto){return terrain();});XeenCamera ordinaryCamera;XeenGameFlags flags;XeenCombatInputs detached;detached.luck=XeenAttributeValue{14,0};const_cast<XeenMutableOptional<XeenCombatInputs>&>(ordinary.roster.combatInputs(29))=detached;rejects([&]{XeenSaveState::capture(signature,ordinary,ordinaryCamera,flags,ordinaryWorld);});
 Domain d;auto s=d.save();auto bytes=XeenSaveFormat::encode(s);auto base=s;base.journey.reset();auto offset=XeenSaveFormat::encode(base).size();check(bytes.size()-offset==1366,"exact successor suffix length");check(bytes[offset]==3&&bytes[offset+1]==2&&bytes[offset+3]==2&&bytes[offset+39]==30&&bytes[offset+1270]==1&&bytes[offset+1288]==4,"literal wire offsets");
 Bytes expected(1366,0);const auto put=[&](unsigned at,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)expected[at+i]=v>>(8*i);};
 put(0,3,1);put(1,2,2);put(3,2,2);put(5,1,1);put(10,8,2);put(12,610,2);put(14,480,2);put(39,30,1);
 auto raw=chr();constexpr unsigned residual[]{1000,2000,1000,1000,2000,1000};
 for(unsigned owner=0;owner<30;++owner){unsigned b=40+41*owner;put(b,owner,1);unsigned k=1;for(unsigned byte:{20u,21u,28u,29u,30u,31u,34u}){put(b+k,raw[owner*354+byte],4);k+=4;}auto pos=std::find(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),owner);put(b+29,pos==kXeenCombatOwners.end()?0:residual[pos-kXeenCombatOwners.begin()],4);put(b+33,14,4);}
 put(1270,1,1);put(1271,1,4);put(1284,20,2);put(1286,27,2);put(1288,4,2);
 const unsigned ids[]{9,17,18,25},xs[]{6,8,8,1},ys[]{14,15,15,13};for(unsigned i=0;i<4;++i){unsigned b=1290+i*19;put(b+1,20,2);put(b+3,ids[i],4);put(b+7,xs[i],2);put(b+9,ys[i],2);put(b+11,i==0?20:30,4);put(b+15,i==3?1:0,1);}
 check(std::equal(expected.begin(),expected.end(),bytes.begin()+offset),"every schema2 suffix byte independently specified");
 const auto repair=[](Bytes &b){auto size=b.size()-20;auto crc=crc32(0,b.data()+20,size);for(unsigned i=0;i<4;++i){b[12+i]=size>>(8*i);b[16+i]=crc>>(8*i);}};
 for(auto field:{0u,1u,3u,5u,6u,7u,37u,38u,39u,40u,1270u,1283u,1286u,1288u,1290u,1305u,1306u,1307u,1308u}){auto bad=bytes;bad[offset+field]=255;repair(bad);rejects([&]{XeenSaveFormat::decode(bad);});}
 for(unsigned mode=0;mode<3;++mode){auto bad=bytes;if(mode==0)bad.push_back(0);else if(mode==1)bad.pop_back();else bad[offset+1271]^=1;if(mode!=2)repair(bad);rejects([&]{XeenSaveFormat::decode(bad);});}

 for(unsigned field=0;field<13;++field){auto bad=s;switch(field){case 0:bad.journey->schema=1;break;case 1:bad.journey->contract=1;break;case 2:bad.journey->supplements[29].inputs.luck.reset();break;case 3:bad.journey->supplements[0].inputs.luck->temporary=256;break;case 4:bad.journey->random->state=0;break;case 5:bad.journey->random->algorithm=2;break;case 6:bad.journey->skeletonSeed=1;break;case 7:bad.journey->actors.pop_back();break;case 8:bad.journey->actors[1].id=bad.journey->actors[0].id;break;case 9:bad.journey->actors[0].hp=19;break;case 10:bad.disabledEvents.push_back({20,16});break;case 11:bad.disabledObjects.push_back({20,2});break;case 12:bad.journey->context->minutes=960;break;}rejects([&]{Domain invalid(bad);});}
 s.journey->random->count=std::numeric_limits<std::uint64_t>::max();Domain full(s);check(full.save().journey->random==s.journey->random,"maximum cursor quiet roundtrip");
 auto exhausted=group({25});exhausted.journey->random->count=std::numeric_limits<std::uint64_t>::max();Domain max(exhausted);auto &c=max.engage();auto r=action(c,Command::Attack);check(r.status==Status::Failed&&max.w.sessionState().actors()[25].hp==30,"next draw exhaustion before publication");
}
void objectivePersistence(){
 Domain initial;const auto fresh=initial.save();
 // Independent categories include grant-only failure, each partial overlay,
 // removal without possession, and collection with an existing counter.
 for(unsigned mask=0;mask<64;++mask)for(unsigned count:{0u,7u,std::numeric_limits<unsigned>::max()}){
  auto saved=fresh;saved.questItems[100-XeenCloudsQuestItems::kFirstItemId]=count;
  if(mask&1)saved.disabledObjects.push_back({20,1});
  for(unsigned i=1;i<=5;++i)if(mask&(1u<<i))saved.disabledEvents.push_back({20,i});
  Domain restored(XeenSaveFormat::decode(XeenSaveFormat::encode(saved)));
  check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(restored.save()),"independent objective counters and overlay subsets preserve every byte");
  auto base=saved;base.journey.reset();
  check(XeenSaveFormat::encode(saved).size()-XeenSaveFormat::encode(base).size()==1366,"overlay base growth retains schema2 suffix");
  restored.w.discardMapCache();
  XeenActorApproach::validateEnvironment(restored.w,restored.w.sessionState().actors(),events(),2);
  check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(restored.save()),"cache reconstruction preserves all saved values");
  check(restored.flow->journeyAction(restored.flow->ticket(),XeenEncounterAction::Right).outcome==XeenEncounterOutcome::Accepted,"partial overlays retain mutable navigation with surviving actors");
  restored.present();auto after=restored.save();
  auto expectedContext=saved.journey->context;expectedContext->ctr24=(expectedContext->ctr24+1)%24;
  check(after.questItems==saved.questItems&&after.disabledObjects==saved.disabledObjects&&after.disabledEvents==saved.disabledEvents&&after.journey->random==saved.journey->random&&after.journey->context==expectedContext,"turning preserves objective effects and RNG with inherited ctr24 advancement");
 }
 auto removed=fresh;removed.disabledObjects={{20,1}};for(unsigned i=1;i<=5;++i)removed.disabledEvents.push_back({20,i});
 for(unsigned field=0;field<14;++field){
  auto mob=objects();auto evt=events();
  switch(field){case 0:mob.entities.objects[1].resourceId=7;break;case 1:mob.entities.objects[1].tableIndex=0;break;case 2:mob.entities.objects[1].direction=1;break;case 3:mob.entities.objects[1].x=4;break;case 4:mob.entities.objectTable[1]=7;break;case 5:evt.records[1].opcode=0;break;case 6:evt.records[2].parameters={1};break;case 7:evt.records[3].direction=0;break;case 8:evt.records[4].fileOffset=30;break;case 9:evt.records[5].lengthField=6;break;case 10:evt.records[5].line=5;break;case 11:evt.records[0].x=0;evt.records[0].y=14;break;case 12:evt.records.pop_back();break;case 13:mob.entities.objects[0]=mob.entities.objects[1];break;}
  XeenWorld world([](auto){return terrain();},[mob](auto){return mob;});XeenPartyState party;XeenCamera camera;XeenGameFlags flags;
  XeenSaveState::Resources resources{signature,{},[evt](auto){return evt;},{},{},[]{return monsters();}};bool presented=false;
  rejects([&]{XeenSaveState::restoreBeforeGameplay(removed,resources,party,camera,flags,world,[&](auto &,const auto &,const auto &,const auto &){presented=true;});});
  check(!presented&&!world.hasEncounterState()&&world.sessionState().disabledObjectCount()==0&&world.sessionState().disabledEventCount()==0&&!party.encounterContext,"disabled overlays cannot hide invalid immutable topology or publish a partial restore");
 }
}
void restorationGuards(){Domain initial;auto saved=initial.save();
 for(unsigned facing=0;facing<4;++facing){auto objective=group({25});objective.camera={20,5,14,static_cast<XeenDirection>(facing)};objective.journey->actors[3].x=4;Domain d(objective);auto before=XeenSaveFormat::encode(d.save());bool called=false;rejects([&]{d.flow->journeyRead([&]{called=true;});});check(!called&&XeenSaveFormat::encode(d.save())==before,"deferred objective refuses callback in every facing without mutation");}
 for(unsigned field=0;field<6;++field){XeenWorld world([](auto){return terrain();},[](auto){return objects();});XeenPartyState party;XeenCamera camera;XeenGameFlags flags;XeenSaveState::Resources resources{signature,{},[](auto){return events();},{},{},[]{return monsters();}};
  rejects([&]{XeenSaveState::restoreBeforeGameplay(saved,resources,party,camera,flags,world,[&](auto &w,const auto &p,const auto &,const auto &){if(field<3){auto &v=const_cast<XeenCombatInputs &>(*p.roster.combatInputs(29));if(field==0)v.luck.reset();else if(field==1)++v.luck->permanent;else ++v.luck->temporary;}else{auto &r=const_cast<XeenMutableOptional<XeenJourneyRandomState>&>(w.sessionState().journeyRandom());if(field==3)++r->state;else if(field==4)++r->count;else r->algorithm=2;}});});
  check(!world.hasEncounterState()&&!party.roster.combatMarked()&&!party.encounterContext,"mutated successor preflight leaves fresh destinations unchanged");
 }
 for(unsigned record=1;record<=5;++record){auto partial=saved;partial.disabledEvents.push_back({20,record});Domain accepted(partial);check(XeenSaveFormat::encode(partial)==XeenSaveFormat::encode(accepted.save()),"independent objective event overlay roundtrip");}
 for(unsigned variant=0;variant<5;++variant){auto bad=group({25});auto &actor=bad.journey->actors[3];if(variant==0){actor.x=4;actor.y=14;}if(variant==1)actor.activated=false;if(variant==2){actor.x=5;actor.y=15;}if(variant==3)actor.hp=29;if(variant==4)actor.accounted=true;rejects([&]{Domain rejected(bad);});}
}

XeenSaveSnapshot finishGroup(Domain &d){auto &c=d.engage();for(unsigned i=0;i<500&&c.phase()!=Phase::Victory;++i){auto r=c.phase()==Phase::PlayerReady?action(c,Command::Attack):c.service(c.ticket());check(r.status!=Status::Failed&&r.status!=Status::Defeat,"restart combat");}check(c.phase()==Phase::Victory&&d.flow->retireJourney(d.flow->ticket()),"restart End retirement");d.present();return d.save();}
void startupAdmission(const std::filesystem::path &dir){
 Domain d;auto path=dir/"successor.mmsave";XeenSaveFile::write(path,d.save());combat_gameplay_test::Harness harness;auto services=harness.services();unsigned providers=0;
 services.maps=[&](auto){++providers;return terrain();};services.objects=[&](auto){++providers;return objects();};services.resources.signature=signature;services.resources.loadMonsterStatistics=[] {return monsters();};services.resources.loadEvents=[](auto) {return events();};services.resources.loadInitialParty=[&]()->XeenPartyState{throw std::runtime_error("restore must not load initial party");};
 services.show=[&](const auto &,const auto &handler,const auto &,const auto &,const auto &){handler.framePresented(harness.flow->frame().presentation());check(harness.flow->canSave()&&harness.world->sessionState().journeyContract()==2,"schema2 startup frame admission");return true;};
 check(Application().playGameplay(services,{},path,true)==0&&providers>0&&harness.compositions>0,"Application descriptor-driven successor startup");
}
void restart(const std::filesystem::path &exe,const std::filesystem::path &dir){
 for(unsigned variant=0;variant<3;++variant){auto s=group({25});if(variant==1){s.journey->actors[1].x=8;s.journey->actors[1].y=14;s.journey->actors[1].hp=30;s.journey->actors[1].activated=true;s.journey->actors[1].accounted=false;s.journey->actors[1].lifecycle=XeenActorLifecycle::Present;s.characters[1].conditions[4]=2;s.characters[1].currentHp=12;s.characters[1].currentSp=21;}if(variant==2){s.journey->random->state=234;s.journey->random->count=100;s.journey->context->minutes=700;}
  auto input=dir/("restart"+std::to_string(variant)+".mmsave"),output=dir/("result"+std::to_string(variant)+".mmsave"),log=dir/("child"+std::to_string(variant)+".log");XeenSaveFile::write(input,s);
  Domain direct(s);auto expected=finishGroup(direct);
  const auto child=child_test::launch(exe,{L"--resume",input.wstring(),output.wstring()},log);check(child.exit==0,"separate-process continuation");
  check(XeenSaveFormat::encode(expected)==XeenSaveFormat::encode(XeenSaveFile::read(output)),"uninterrupted/process-restart equivalence for all saved fields");
 }
}

}
int main(int argc,char **argv){using namespace expedition_test;try{
 if(argc==4&&std::string(argv[1])=="--resume"){Domain restored(XeenSaveFile::read(std::filesystem::absolute(argv[2])));XeenSaveFile::write(std::filesystem::absolute(argv[3]),finishGroup(restored));return 0;}
 profileBinding();groups();disease();failure();injuriesAndDefeat();scheduler();prefixes();derived();codec();objectivePersistence();restorationGuards();
 auto dir=std::filesystem::temp_directory_path()/("mmodern-m30a-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));std::filesystem::create_directory(dir);startupAdmission(dir);restart(std::filesystem::absolute(argv[0]),dir);
 std::cout<<"Expedition group, literal Zombie, failure, Luck, successor codec, production startup and process controls passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
