#include "XeenSaveTestSupport.h"
#include <iostream>
using namespace save_test;
namespace {
void put(Bytes &b,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)b.push_back(v>>(8*i));}
std::size_t baseSize(const XeenSaveSnapshot &s){
 check(s.barriers.empty(),"Independent suffix fixture requires an empty v5 barrier list");
 std::size_t n=20+25+6+2+s.activeRosterIds.size()+4*s.questItems.size()+s.questFlags.size()+s.gameFlags.size()+8+7*(s.disabledObjects.size()+s.disabledEvents.size())+2;
 for(const auto &c:s.characters)n+=212+c.name.size();return n;
}
Bytes suffix(const XeenSaveSnapshot &s){
 const auto &j=*s.journey;Bytes b{3,9,0,14,0,1,0,0};
 for(unsigned value:{23u,8u,610u,1000u})put(b,value,2);
 b.resize(39);b.push_back(30);
 for(unsigned owner=0;owner<30;++owner){b.push_back(owner);for(unsigned i=0;i<8;++i)put(b,0,4);put(b,17,4);put(b,3,4);}
 b.push_back(1);put(b,0xf1234567,4);put(b,0x123456789ULL,8);
 b.push_back(0);put(b,23,2);put(b,19,2);put(b,19,2);
 const auto actor=[&](const XeenSaveJourneyActor &a){b.push_back(0);put(b,a.id.mapId.number,2);put(b,a.id.recordIndex,4);put(b,std::uint16_t(a.x),2);put(b,std::uint16_t(a.y),2);put(b,std::uint32_t(a.hp),4);b.push_back(a.activated);b.push_back(static_cast<unsigned>(a.lifecycle));b.push_back(static_cast<unsigned>(a.status));b.push_back(a.accounted);};
 for(const auto &a:j.actors)actor(a);
 b.push_back(30);for(unsigned i=0;i<30;++i)for(unsigned v:{i,i,i+30,i+60,i+90})b.push_back(v);
 const auto &t=*j.treasure;for(auto v:{t.gold,t.gems,t.pendingMask,t.pendingGold})put(b,v,4);
 unsigned weapons=0,armor=0;for(const auto &v:t.weapons)weapons+=v.item.id!=0;for(const auto &v:t.armor)armor+=v.item.id!=0;
 b.push_back(weapons);b.push_back(armor);
 for(const auto &items:{t.weapons,t.armor})for(const auto &v:items)if(v.item.id)b.insert(b.end(),{v.source,v.item.material,v.item.id,v.item.state,v.item.frame});
 b.push_back(j.regionalRecovery->worldFlag16);b.push_back(30);
 for(unsigned owner=0;owner<30;++owner){b.push_back(owner);for(unsigned slot=0;slot<39;++slot)b.push_back(owner==2?0:owner==29&&slot==38?255:std::uint8_t(owner*7+slot*3));}
 b.push_back(30);for(unsigned owner=0;owner<30;++owner)b.insert(b.end(),{std::uint8_t(owner),std::uint8_t(owner),std::uint8_t(255-owner)});
 b.push_back(j.vertigoActors.has_value());if(j.vertigoActors){put(b,j.cityOriginalActorCount,2);put(b,j.vertigoActors->size(),2);for(const auto &a:*j.vertigoActors){actor(a);put(b,std::uint16_t(a.spawnedType),2);}}
 b.insert(b.end(),{2,4,4,9});for(const auto &side:j.serviceEconomy->wares.records)for(const auto &shop:side)for(const auto &category:shop)for(const auto &item:category)b.insert(b.end(),{item.material,item.id,item.state,item.frame});
 put(b,0xfedcba98u,4);put(b,0xffffffffu,4);return b;
}
XeenSaveSnapshot sampleCurrent(){
 auto s=currentWireSnapshot();auto &j=*s.journey;s.resources={{1,2},XeenArchiveFingerprint{3,4}};s.camera={23,8,11,XeenDirection::West};
 j.context->minutes=1000;j.context->ctr24=23;j.random=XeenJourneyRandomState{1,0xf1234567,0x123456789ULL};
 j.treasure->gold=0x12345678;j.treasure->gems=0x87654321;
 for(unsigned i=0;i<30;++i){j.supplements[i].inputs.luck=XeenAttributeValue{17,3};j.supplements[i].inputs.resistances=XeenCombatResistances{std::uint8_t(i),std::uint8_t(i+30),std::uint8_t(i+60),std::uint8_t(i+90)};j.supplements[i].inputs.poisonResistance=XeenAttributeValue{int(i),int(255-i)};
  for(unsigned slot=0;slot<39;++slot)s.characters[i].learnedSpells->at(slot)=i==2?0:std::uint8_t(i*7+slot*3);}
 s.characters[29].learnedSpells->at(38)=255;
 for(unsigned i=0;i<19;++i)j.actors[i]={{23,i},int(i%16),int(i/16),int(i+1),bool(i%2),XeenActorLifecycle::Present,XeenActorStatus::Physical,false};return s;
}
void verify(const XeenSaveSnapshot &s){
 const auto bytes=XeenSaveFormat::encode(s),expected=suffix(s);const auto offset=baseSize(s);
 check(bytes.size()==offset+expected.size()&&std::equal(expected.begin(),expected.end(),bytes.begin()+offset),"Independent complete schema9/content14 suffix");
 sameSnapshot(s,XeenSaveFormat::decode(bytes));check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes))==bytes,"Exact current byte continuation");
 for(std::size_t size=offset;size<bytes.size();++size){auto b=bytes;b.resize(size);fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});}
 auto extra=bytes;extra.push_back(0);fixIndependentEnvelope(extra);rejects([&]{XeenSaveFormat::decode(extra);});
}
}
int main(){try{
 auto s=sampleCurrent();verify(s);auto populated=s;auto &t=*populated.journey->treasure;t.pendingMask=(1u<<3)|(1u<<9);t.pendingGold=20;t.weapons[0]={9,{0,30,0,0}};t.armor[0]={3,{0,2,0,0}};
 for(unsigned id:{3u,9u}){auto &a=populated.journey->actors[id];a.x=a.y=-128;a.hp=0;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;}
 populated.characters[0].conditions[3]=2;populated.characters[1].conditions[8]=1;verify(populated);
 auto dormant=populated;dormant.journey->treasure=xeenPrepareMonsterGoldForfeiture(*dormant.journey->treasure);verify(dormant);
 auto later=dormant;later.journey->treasure->pendingMask=1u<<3;later.journey->treasure->pendingGold=10;verify(later);
 auto maximum=populated;auto &mt=*maximum.journey->treasure;mt.pendingMask=4095;mt.pendingGold=120;mt.weapons={};mt.armor={};
 for(unsigned id=0;id<12;++id){auto &a=maximum.journey->actors[id];a.x=a.y=-128;a.hp=0;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;if(id<10)mt.weapons[id]={std::uint8_t(id),{0,30,0,0}};else mt.armor[id-10]={std::uint8_t(id),{0,2,0,0}};}
 verify(maximum);maximum.journey->treasure=xeenPrepareMonsterGoldForfeiture(mt);verify(maximum);
 auto city=populated;city.camera={28,16,2,XeenDirection::North};city.journey->vertigoActors.emplace();
 for(unsigned i=0;i<46;++i)city.journey->vertigoActors->push_back({{28,i},int(i%16),int(i/16),2,false,XeenActorLifecycle::Present,XeenActorStatus::Physical,false});verify(city);
 auto reset=city;reset.disabledEvents={{28,764}};for(unsigned i=46;i<52;++i)reset.journey->vertigoActors->push_back({{28,i},0,0,i<50?0:2,false,i<50?XeenActorLifecycle::Unresolved:XeenActorLifecycle::Present,XeenActorStatus::Physical,false,i<50?std::int16_t(-1):std::int16_t(0)});verify(reset);
 for(unsigned slot:{50u,51u}) {
  auto bad=reset;bad.journey->vertigoActors->at(slot).spawnedType=1;
  rejects([&]{XeenSaveFormat::encode(bad);});
 }
 auto flagged=populated;flagged.journey->regionalRecovery->worldFlag16=true;verify(flagged);
 const auto bytes=XeenSaveFormat::encode(populated);const auto offset=baseSize(populated);
 const auto badByte=[&](unsigned at,unsigned value){auto b=bytes;b[offset+at]=value;fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});};
 for(unsigned at:{5u,37u,38u})badByte(at,2);badByte(6,1);badByte(7,2);badByte(39,29);badByte(1270,2);badByte(1283,1);badByte(1284,20);
 for(unsigned value:{0u,1u,18u,20u,107u,108u,255u}){badByte(1286,value);badByte(1288,value);}
 for(unsigned i=0;i<30;++i){badByte(40+41*i,31);badByte(40+41*i+34,1);}
 for(unsigned i=0;i<19;++i){const auto at=1290+19*i;badByte(at,1);badByte(at+1,20);badByte(at+3,19);badByte(at+15,2);badByte(at+16,4);badByte(at+17,2);badByte(at+18,2);}
 for(unsigned at:{1651u,1652u,1818u,1819u})badByte(at,31);
 for(unsigned at:{1821u,1823u,1824u,1826u,1828u,1829u})badByte(at,1);
 badByte(1820,3);badByte(1822,34);badByte(1827,8);badByte(1810,0);badByte(1814,10);
 badByte(1830,2);badByte(1831,29);badByte(1832,1);badByte(1872,0);badByte(3032,29);badByte(3033,1);badByte(3036,0);badByte(3123,2);
 for(unsigned mode=0;mode<15;++mode){auto bad=dormant;auto &j=*bad.journey;
  switch(mode){case 0:j.skeletonSeed=1;break;case 1:j.supplements[0].inputs.luck.reset();break;case 2:j.treasure.reset();break;case 3:j.supplements[29].inputs.resistances.reset();break;case 4:j.actors[9].accounted=false;break;case 5:j.actors[9].hp=1;break;case 6:j.treasure->pendingGold=10;break;case 7:j.treasure->armor[0].source=9;break;case 8:j.treasure->weapons[0].item.frame=1;break;case 9:j.treasure->weapons[0].source=12;break;case 10:j.regionalRecovery.reset();break;case 11:bad.characters[29].learnedSpells.reset();break;case 12:j.supplements[29].inputs.poisonResistance.reset();break;case 13:j.serviceEconomy.reset();break;case 14:j.random.reset();break;}
  rejects([&]{XeenSaveFormat::encode(bad);});}
 for(const auto &source:{s,city,reset})for(unsigned record:{0u,539u,761u,764u}){
  auto bad=source;if(record==0)bad.disabledObjects={{28,0}};else bad.disabledEvents={{28,record}};
  if(record!=0&&source.journey->vertigoActors)verify(bad);else rejects([&]{XeenSaveFormat::encode(bad);});
 }
 std::cout<<"Current regional literal wire, all city variants, treasure, learned fields and malformed controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
