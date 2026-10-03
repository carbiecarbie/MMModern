#include "XeenRegionalJourneyTestSupport.h"
#include "games/xeen/XeenRestoreGuard.h"
#include <iostream>
using namespace mmodern;
using combat_test::check;
namespace {
struct Fixture {
 XeenPartyState party=XeenPartyLoader().loadFromResources(regional_test::characterBytes(),regional_test::partyBytes());
 XeenCamera camera{23,9,11,XeenDirection::West};XeenGameFlags flags;
 bool altered=false,wrongIdentity=false;
 XeenWorld world{[&](auto id){auto m=regional_test::map();m.geometry.id=wrongIdentity?99:static_cast<unsigned>(id.number);
  if(altered)m.geometry.trapDamage=231;return m;},
  [&](auto id){auto o=regional_test::objects();o.mapId=wrongIdentity?XeenMapIdentity(99):id;
   if(altered)o.entities.monsters[0].x=99;return o;}};
};
void admission() {
 for(bool objects:{false,true})for(bool strict:{false,true}) {
  Fixture f;XeenRestoreGuard retained(f.world,f.party,f.camera,f.flags,strict);
  {XeenRestoreGuard callback(f.world,f.party,f.camera,f.flags);
   XeenRestoreGuard::Providers providers(callback,f.world);
   if(objects)f.world.objectFile(21);else f.world.map(21);
   check(callback.current() && retained.current(),"Every retained guard admits provider cache growth");
  }
  if(objects)f.world.objectFile(22);else f.world.map(22);
  check(retained.current(),"Unwrapped world provider admission");
  const auto count=f.world.cachedMapCount();f.world.map(22);
  check(f.world.cachedMapCount()==(objects?count+1:count),"Cache hit has no extra insertion");
  check(retained.current(),"Matching map/object admission remains current");
 }
}
void corruption() {
 for(bool objects:{false,true})for(bool revert:{false,true}) {
  Fixture f;XeenRestoreGuard retained(f.world,f.party,f.camera,f.flags);
  if(objects) {auto &v=const_cast<XeenObjectFile &>(f.world.objectFile(21)).entities.monsters[0].x;
   const int old=v;v=99;if(revert)v=old;}
  else {auto &v=const_cast<XeenMap &>(f.world.map(21)).geometry.trapDamage;
   const unsigned old=v;v=231;if(revert)v=old;}
  check(!retained.current(),"Admitted content mutation/reversion invalidates history");
  f.world.discardMapCache();
  if(objects)f.world.objectFile(21);else f.world.map(21);
  check(!retained.current(),"Eviction and reconstruction cannot erase mutation history");
 }
 for(bool objects:{false,true}) {
  Fixture f;XeenRestoreGuard retained(f.world,f.party,f.camera,f.flags);
  if(objects)f.world.objectFile(21);else f.world.map(21);
  XeenRestoreGuard second(f.world,f.party,f.camera,f.flags);
  f.world.discardMapCache();f.altered=true;
  bool refused=false;try {if(objects)f.world.objectFile(21);else f.world.map(21);}catch(const std::exception &){refused=true;}
  check(refused && !retained.current() && !second.current(),"Changed provider reconstruction is refused");
  f.altered=false;if(objects)f.world.objectFile(21);else f.world.map(21);
  check(!retained.current(),"Matching later provider cannot reopen failed guard");
 }
}
void identitiesAndOwners() {
 for(bool objects:{false,true}) {
  Fixture f;XeenRestoreGuard first(f.world,f.party,f.camera,f.flags);
  if(objects)f.world.objectFile(21);else f.world.map(21);
  XeenRestoreGuard moved(std::move(first));
  f.world.discardMapCache();f.wrongIdentity=true;
  bool refused=false;try {if(objects)f.world.objectFile(21);else f.world.map(21);}catch(const std::exception &){refused=true;}
  check(refused && !moved.current(),"Moved guard retains identity binding");
  f.wrongIdentity=false;if(objects)f.world.objectFile(21);else f.world.map(21);
  check(!moved.current(),"Identity reversion cannot reopen admission");
 }
 {Fixture f;XeenRestoreGuard strict(f.world,f.party,f.camera,f.flags,true);
  f.world.map(21);check(strict.current(),"Strict guard admits growth");
  f.world.discardMapCache();f.world.map(21);
  check(!strict.current(),"Admission does not erase strict destination eviction detection");}
 for(bool objects:{false,true}) {
  Fixture f;XeenRestoreGuard retained(f.world,f.party,f.camera,f.flags);f.wrongIdentity=true;
  bool refused=false;try {if(objects)f.world.objectFile(21);else f.world.map(21);}catch(const std::exception &){refused=true;}
  check(refused && !f.world.cachedMapCount() && !f.world.cachedObjectFileCount(),"Wrong provider identity never enters cache");
 }
 for(unsigned owner=0;owner<3;++owner) {
  Fixture f;XeenRestoreGuard retained(f.world,f.party,f.camera,f.flags);f.world.map(21);
  if(owner==0){++f.party.roster.at(0).currentHp;--f.party.roster.at(0).currentHp;}
  if(owner==1){++f.camera.x;--f.camera.x;}
  if(owner==2)f.world.disableObject({23,0});
  f.world.map(22);check(!retained.current(),"Admission never renews owner mutation history");
 }
 regional_journey_test::Fixture f;
 XeenRestoreGuard before(f.w,f.p,f.camera,f.flags);f.engage();
 check(!before.current(),"Combat owner transition still invalidates prior owner snapshot");
 auto *combat=f.flow->combat();const auto ticket=combat->ticket();
 ++f.p.roster.at(0).currentHp;--f.p.roster.at(0).currentHp;
 check(!combat->current(ticket),"Combat retained guard still detects reverted party writes");
}
}
int main(){try{admission();corruption();identitiesAndOwners();
 std::cout<<"Immutable cache admission, identity, reconstruction and owner history passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
