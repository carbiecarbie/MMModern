#include "games/xeen/XeenRestoreGuard.h"
#include "XeenEncounterTestSupport.h"
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
struct Fixture:encounter_test::Fixture {
 XeenPartyState &party=p;
 XeenGameFlags flags;
 Fixture() {
  start();camera={28,15,0,XeenDirection::North};
  party.encounterContext.emplace();party.monsterTreasure.emplace();party.regionalRecovery.emplace();
  party.roster.at(0).learnedSpells.emplace();party.roster.at(0).name="Before";
  auto &actor=const_cast<XeenActor&>(world.sessionState().actors()[0]);
  actor.hp=2;actor.statistics.emplace();
  world.map(28);
 }
};
void require(bool v,const char *message) { if(!v)throw std::runtime_error(message); }
template<class View,class Container,class=void>struct HasRawReferenceConversion:std::false_type{};
template<class View,class Container>struct HasRawReferenceConversion<View,Container,
 std::void_t<decltype(std::declval<View>().operator Container&())>>:std::true_type{};
void rejects(const char *name,const std::function<void(Fixture&)> &mutation) {
 Fixture f;
 XeenRestoreGuard guard(f.world,f.party,f.camera,f.flags);
 require(guard.current(),"initial guard");mutation(f);
 if(guard.current())throw std::runtime_error(std::string("ABA accepted: ")+name);
}
void materializedResources() {
 unsigned missed=0;
 for(bool mob:{false,true}) for(bool reconstruct:{false,true}) for(unsigned field=0;field<=(mob?4u:6u);++field) {
  XeenWorld world([](XeenMapIdentity id) {
   XeenMap m;m.side=id.side;m.geometry.id=id.number;
   m.entities.objects.resize(1);m.entities.monsters.resize(1);m.entities.wallItems.resize(1);
   m.instructions.push_back({0,0,0,0,0,{1,2}});return m;
  },[](XeenMapIdentity id) {
   XeenObjectFile m;m.mapId=id;m.resourcePresent=true;
   m.entities.objects.resize(1);m.entities.monsters.resize(1);m.entities.wallItems.resize(1);return m;
  });
  XeenPartyState party;XeenCamera camera{28,0,0,XeenDirection::North};XeenGameFlags flags;
  if(reconstruct) {if(mob)world.objectFile(28);else world.map(28);}
  XeenRestoreGuard outer(world,party,camera,flags);
  XeenRestoreGuard::Providers outerProviders(outer,world);
  XeenRestoreGuard inner(world,party,camera,flags);
  XeenRestoreGuard::Providers innerProviders(inner,world);
  if(reconstruct)world.discardMapCache();
  // No guard check/refresh between acquisition and ABA: registration must be
  // complete inside the cache insertion, including every nested allocation.
  if(mob) {
   auto &m=const_cast<XeenObjectFile&>(world.objectFile(28));
   if(field==0){m.resourcePresent=false;m.resourcePresent=true;}
   if(field==1){++m.entities.objects[0].x;--m.entities.objects[0].x;}
   if(field==2){++m.entities.monsters[0].x;--m.entities.monsters[0].x;}
   if(field==3){++m.entities.wallItems[0].x;--m.entities.wallItems[0].x;}
  } else {
   auto &m=const_cast<XeenMap&>(world.map(28));
   if(field==0){++m.geometry.flags;--m.geometry.flags;}
   if(field==1){++m.entities.objects[0].x;--m.entities.objects[0].x;}
   if(field==2){++m.entities.monsters[0].x;--m.entities.monsters[0].x;}
   if(field==3){++m.entities.wallItems[0].x;--m.entities.wallItems[0].x;}
   if(field==4){++m.instructions[0].x;--m.instructions[0].x;}
   if(field==5){++m.instructions[0].parameters[0];--m.instructions[0].parameters[0];}
  }
  const bool innerCurrent=inner.current(),outerCurrent=outer.current();
  const bool readOnly=field==(mob?4u:6u);
  if(readOnly ? (!innerCurrent || !outerCurrent) : (innerCurrent || outerCurrent)) {
   ++missed;std::cerr<<(mob?"MOB":"Map")<<(reconstruct?" reconstructed":" cold")<<" ABA missed, field "<<field<<'\n';
  }
 }
 require(missed==0,"new cache storage escaped without observation");
 // A stale observer still inherits new ranges, but insertion/retirement must
 // never renew it. A fresh observer of that same world remains independent.
 for(bool mob:{false,true}) {
  Fixture f;XeenMutationWatch stale;stale.add(&f.world,sizeof(f.world));
  f.world.discardMapCache();
  if(mob) {
   auto &m=const_cast<XeenObjectFile&>(f.world.objectFile(28));
   ++m.entities.objects[0].x;--m.entities.objects[0].x;
  } else {
   auto &m=const_cast<XeenMap&>(f.world.map(28));++m.geometry.flags;--m.geometry.flags;
  }
  XeenMutationWatch fresh;fresh.add(&f.world,sizeof(f.world));
  f.world.discardMapCache();
  if(mob)f.world.objectFile(28);else f.world.map(28);
  require(!stale.current() && fresh.current(),"cache registration renewed stale history or invalidated read");
 }
}
}
int main() {
 try {
  materializedResources();
  static_assert(!HasRawReferenceConversion<XeenReadOnlyVector<XeenActor>,const std::vector<XeenActor>>::value);
  static_assert(!HasRawReferenceConversion<XeenReadOnlySet<XeenEventIdentity>,const std::set<XeenEventIdentity>>::value);
  static_assert(!std::is_convertible_v<XeenReadOnlyVector<XeenActor>,std::vector<XeenActor>&>);
  static_assert(!std::is_convertible_v<XeenReadOnlySet<XeenEventIdentity>,std::set<XeenEventIdentity>&>);
  {
   Fixture f;auto &retained=f.party.roster.at(0).currentHp;
   const auto callback=[&]{++retained;--retained;};
   XeenRestoreGuard guard(f.world,f.party,f.camera,f.flags);
   callback();require(!guard.current(),"retained alias predates guard");
  }
  {
   Fixture f;
   auto &inputs=const_cast<XeenMutableOptional<XeenCombatInputs>&>(f.party.roster.combatInputs(0));
   inputs.emplace();auto &retained=inputs->experience;
   XeenRestoreGuard guard(f.world,f.party,f.camera,f.flags);
   ++retained;--retained;require(!guard.current(),"combat supplement retained alias");
  }
  {
   XeenWorld world([](XeenMapIdentity id){XeenMap m;m.side=id.side;m.geometry.id=id.number;return m;},
    [](XeenMapIdentity id){XeenObjectFile f;f.mapId=id;f.resourcePresent=true;f.entities.objects.resize(1);return f;});
   XeenPartyState party;XeenCamera camera{23,0,0,XeenDirection::North};XeenGameFlags flags;
   world.objectFile(23);
   XeenRestoreGuard guard(world,party,camera,flags);
   world.disableObject({23,0});
   world.restoreSessionState({}, {}, [](XeenMapIdentity id){XeenEventFile f;f.mapId=id;return f;});
   require(!guard.current(),"overlay mutation and restoration");
  }
  rejects("flags",[](auto &f){f.flags.set(9);f.flags.clear(9);});
  rejects("persistent flags",[](auto &f){f.flags.set(9);});
  rejects("joined worker flags ABA",[](auto &f){std::thread worker([&]{f.flags.set(9);f.flags.clear(9);});worker.join();});
  rejects("flag assignment",[](auto &f){const auto before=f.flags;f.flags.set(9);f.flags=before;});
  rejects("camera",[](auto &f){++f.camera.x;--f.camera.x;});
  rejects("camera region",[](auto &f){f.camera.mapId.number=23;f.camera.mapId.number=28;});
  rejects("character",[](auto &f){auto &v=f.party.roster.at(0).currentHp;++v;--v;});
  rejects("character condition",[](auto &f){auto &v=f.party.roster.at(0).conditions[3];v=1;v=0;});
  rejects("character name",[](auto &f){f.party.roster.at(0).name="After";f.party.roster.at(0).name="Before";});
  rejects("inventory",[](auto &f){auto &v=f.party.roster.at(0).miscellaneous[0].state;v=1;v=0;});
  rejects("learned optional",[](auto &f){auto &v=f.party.roster.at(0).learnedSpells;const auto before=v;v.reset();v=before;});
  rejects("calendar",[](auto &f){++f.party.encounterContext->minutes;--f.party.encounterContext->minutes;});
  rejects("effects",[](auto &f){auto &v=f.party.encounterContext->effects[0];v=1;v=0;});
  rejects("context optional",[](auto &f){const auto before=f.party.encounterContext;f.party.encounterContext.reset();f.party.encounterContext=before;});
  rejects("treasure",[](auto &f){++f.party.monsterTreasure->gold;--f.party.monsterTreasure->gold;});
  rejects("recovery",[](auto &f){f.party.regionalRecovery->worldFlag16=true;f.party.regionalRecovery->worldFlag16=false;});
  rejects("quest flags",[](auto &f){f.party.questFlags.set(1);f.party.questFlags.clear(1);});
  rejects("quest counts",[](auto &f){f.party.questItems.increment(1);f.party.questItems.decrement(1);});
  {
   Fixture f;
   XeenCloudsQuestItems::Counts counts{};counts[1]=std::numeric_limits<std::uint32_t>::max();
   f.party.questItems=XeenCloudsQuestItems(counts);
   XeenRestoreGuard guard(f.world,f.party,f.camera,f.flags);
   require(!f.party.questItems.increment(1) && !f.party.questItems.decrement(2),"quest refusal values");
   try {f.flags.set(256);}catch(const std::out_of_range &){}
   try {f.party.questFlags.clear(30);}catch(const std::out_of_range &){}
   require(guard.current(),"refused operations recorded writes without mutation");
  }
  rejects("membership",[](auto &f){const auto before=f.party.party;f.party.party=XeenParty::fromRosterIds({0});f.party.party=before;});
  rejects("metadata",[](auto &f){++f.party.firstSerializedCount;--f.party.firstSerializedCount;});
  rejects("diagnostics",[](auto &f){f.party.diagnostics.push_back("temporary");f.party.diagnostics.clear();});
  auto actor=[](Fixture &f)->XeenActor& {return const_cast<XeenActor&>(f.world.sessionState().actors()[0]);};
  rejects("actor HP",[&](auto &f){++actor(f).hp;--actor(f).hp;});
  rejects("joined worker actor ABA",[&](auto &f){auto &retained=actor(f).hp;std::thread worker([&]{++retained;--retained;});worker.join();});
  rejects("persistent actor HP",[&](auto &f){++actor(f).hp;});
  rejects("actor identity",[&](auto &f){++actor(f).id.recordIndex;--actor(f).id.recordIndex;});
  rejects("actor original",[&](auto &f){++actor(f).original.x;--actor(f).original.x;});
  rejects("actor statistics",[&](auto &f){++actor(f).statistics->raw[0];--actor(f).statistics->raw[0];});
  rejects("map geometry",[](auto &f){auto &map=const_cast<XeenMap&>(f.world.map(28));++map.geometry.flags;--map.geometry.flags;});
  rejects("map array",[](auto &f){auto &map=const_cast<XeenMap&>(f.world.map(28));++map.geometry.wallTypes[0];--map.geometry.wallTypes[0];});
  // These retain the container itself, not just one currently stored element.
  rejects("regional collection replacement",[](auto &f){auto &v=const_cast<XeenSessionWorldState&>(f.world.sessionState());const auto before=v;v={};v=before;});
  rejects("session move assignment",[](auto &f){auto &v=const_cast<XeenSessionWorldState&>(f.world.sessionState());const auto before=v;XeenSessionWorldState empty;v=std::move(empty);v=before;});
  rejects("session swap",[](auto &f){auto &v=const_cast<XeenSessionWorldState&>(f.world.sessionState());XeenSessionWorldState empty;std::swap(v,empty);std::swap(v,empty);});
  rejects("flag storage alias",[](auto &f){auto &v=const_cast<XeenMutableArray<bool,256>&>(f.flags.values());v[9]=true;v[9]=false;});
  rejects("membership element",[](auto &f){auto &v=const_cast<XeenMutable<std::uint8_t>&>(f.party.party.activeRosterIds()[0]);const auto before=v;v=1;v=before;});
  {
   Fixture f;const auto &supplement=f.party.roster.combatInputs(0);
   auto &v=const_cast<XeenMutableOptional<XeenCombatInputs>&>(supplement);v.emplace();v->luck.emplace();
   auto &retained=v->luck;XeenRestoreGuard guard(f.world,f.party,f.camera,f.flags);
   const auto before=retained;retained.reset();retained=before;
   require(!guard.current(),"nested supplement optional ABA");
  }
  {
   Fixture f;f.party.diagnostics.push_back("before");
   XeenRestoreGuard guard(f.world,f.party,f.camera,f.flags);
   static_assert(!std::is_reference_v<decltype(*f.party.diagnostics.begin())>);
   auto detached=*f.party.diagnostics.begin();detached="after";
   std::vector<XeenActor> detachedActors=f.world.sessionState().actors();detachedActors.clear();
   require(guard.current(),"read snapshots mutated live owners");
  }
  {
   Fixture f;auto &map=const_cast<XeenMap&>(f.world.map(28));
   map.instructions.push_back({0,0,0,0,0,{1,2}});
   auto &retained=map.instructions[0].parameters[0];
   XeenRestoreGuard guard(f.world,f.party,f.camera,f.flags);
   retained=2;retained=1;require(!guard.current(),"instruction parameter alias ABA");
  }
  rejects("MOB collection replacement",[](auto &f){auto &v=const_cast<XeenObjectFile&>(f.world.objectFile(20)).entities.objects;const auto before=v;v.clear();v=before;});
  rejects("MOB retained entity",[](auto &f){auto &v=const_cast<XeenMapEntity&>(f.world.objectFile(20).entities.objects[0]);++v.x;--v.x;});
  {
   Fixture f;XeenRestoreGuard guard(f.world,f.party,f.camera,f.flags);
   XeenRestoreGuard::Providers providers(guard,f.world);
   f.world.discardMapCache();require(guard.current(),"cache eviction changed gameplay");
   f.world.map(28);require(guard.current(),"matching cache reconstruction rejected");
   auto &map=const_cast<XeenMap&>(f.world.map(28));++map.geometry.flags;--map.geometry.flags;
   f.world.discardMapCache();require(!guard.current(),"cache retirement erased write history");
  }
  {
   Fixture f;
   for(unsigned id=1;id<=300;++id)f.world.map(static_cast<std::uint16_t>(id));
   XeenRestoreGuard guard(f.world,f.party,f.camera,f.flags);
   require(guard.current(),"legacy resource cache has artificial observer capacity limit");
  }
  // Refreshing one owned publication boundary cannot revive a stale outer guard.
  XeenMutable<int> value=0;XeenMutationWatch outer;outer.add(&value,sizeof(value));
  XeenMutationWatch inner;inner.add(&value,sizeof(value));value=1;
  inner.renew();inner.add(&value,sizeof(value));require(inner.current()&&!outer.current(),"renewal revived outer authority");
  value=0;require(!inner.current()&&!outer.current(),"second mutation lost history");
  std::cout<<"Mutation boundary regressions passed\n";return 0;
 } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
