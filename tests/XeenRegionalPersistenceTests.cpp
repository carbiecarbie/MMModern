#include "XeenSaveTestSupport.h"
#include <iostream>
#include <map>
using namespace save_test;
namespace {
void put(Bytes &b,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)b.push_back(v>>(8*i));}
std::size_t baseSize(const XeenSaveSnapshot &s){
 // Header 20, fingerprints 25, camera 6, member and roster counts 2, then the
 // members, quest counters and flags and game flags; v13 writes no top-level
 // overlay list before the Journey block.
 std::size_t n=20+25+6+2+s.activeRosterIds.size()+4*s.questItems.size()+s.questFlags.size()+s.gameFlags.size();
 for(const auto &c:s.characters)n+=228+c.name.size();return n;
}
// The fixtures below use source slot r for F3 record r, as the drop rules
// produce F3's Orcs live; v13 saves order source identities instead.
// Names each referenced slot's F3 monster (the live form).
XeenMonsterTreasure ids(XeenMonsterTreasure t){
 t.sources={};const auto used=t.referencedSources();
 for(unsigned r=0;r<12;++r)if(used&(1u<<r))t.sources[r]={23,r};
 return t;
}
// Converts a record-source fixture to its canonical saved form.
XeenSaveSnapshot withSources(XeenSaveSnapshot s){
 if(!s.journey || !s.journey->treasure)return s;
 auto &t=*s.journey->treasure;std::uint32_t records=t.pendingMask;
 for(const auto *items:{&t.weapons,&t.armor,&t.miscellaneous})for(const auto &v:*items)if(v.item.id)records|=1u<<v.source;
 std::map<unsigned,std::uint8_t> index;t.sources={};
 for(unsigned r=0;r<32;++r)if(records&(1u<<r)){index[r]=std::uint8_t(index.size());if(index[r]<12)t.sources[index[r]]={23,r};}
 std::uint32_t mask=0;for(unsigned r=0;r<32;++r)if(t.pendingMask&(1u<<r))mask|=1u<<index.at(r);t.pendingMask=mask;
 for(auto *items:{&t.weapons,&t.armor,&t.miscellaneous})for(auto &v:*items)if(v.item.id)v.source=index.at(v.source);
 return s;
}
Bytes suffix(const XeenSaveSnapshot &s){
 const auto &j=*s.journey;Bytes b{3,9,0,14,0,1,0,0};
 for(unsigned value:{23u,8u,610u,1000u})put(b,value,2);
 b.resize(39);put(b,0xbeef,2);b.push_back(30);
 for(unsigned owner=0;owner<30;++owner){b.push_back(owner);for(unsigned i=0;i<8;++i)put(b,0,4);put(b,17,4);put(b,3,4);}
 b.push_back(1);put(b,0xf1234567,4);put(b,0x123456789ULL,8);
 // v13 has no Journey actor table here; the monster tables follow the awards.
 b.push_back(30);for(unsigned i=0;i<30;++i)for(unsigned v:{i,i,i+30,i+60,i+90,i+120,i+150,i+180,i+210,255-i,254-i})b.push_back(v);
 const auto &t=*j.treasure;for(auto v:{t.gold,t.gems,t.pendingMask,t.pendingGold})put(b,v,4);
 // v13: the source identities (side, number, record) that bits and items index.
 unsigned sources=0;while(sources<12 && t.sources[sources].mapId)++sources;b.push_back(std::uint8_t(sources));
 for(unsigned k=0;k<sources;++k){const auto id=t.sources[k];b.push_back(static_cast<unsigned>(XeenSide(id.mapId.side)));put(b,id.mapId.number,2);put(b,id.recordIndex,2);}
 unsigned weapons=0,armor=0;for(const auto &v:t.weapons)weapons+=v.item.id!=0;for(const auto &v:t.armor)armor+=v.item.id!=0;
 b.push_back(weapons);b.push_back(armor);
 for(const auto &items:{t.weapons,t.armor})for(const auto &v:items)if(v.item.id)b.insert(b.end(),{v.source,v.item.material,v.item.id,v.item.state,v.item.frame});
 unsigned miscellaneous=0;for(const auto &v:t.miscellaneous)miscellaneous+=v.item.id!=0;
 b.push_back(miscellaneous);
 for(const auto &v:t.miscellaneous)if(v.item.id)b.insert(b.end(),{v.source,v.item.material,v.item.id,v.item.state,v.item.frame});
 for(bool flag:j.worldFlags->values)b.push_back(flag);b.push_back(30);
 for(unsigned owner=0;owner<30;++owner){b.push_back(owner);for(unsigned slot=0;slot<39;++slot)b.push_back(owner==2?0:owner==29&&slot==38?255:std::uint8_t(owner*7+slot*3));}
 b.push_back(30);for(unsigned owner=0;owner<30;++owner)b.insert(b.end(),{std::uint8_t(owner),std::uint8_t(owner),std::uint8_t(255-owner)});
 b.insert(b.end(),{2,4,4,9});for(const auto &side:j.serviceEconomy->wares.records)for(const auto &shop:side)for(const auto &category:shop)for(const auto &item:category)b.insert(b.end(),{item.material,item.id,item.state,item.frame});
 put(b,0xfedcba98u,4);put(b,0xffffffffu,4);
 b.push_back(30);for(unsigned owner=0;owner<30;++owner){b.push_back(owner);for(const auto value:s.characters[owner].awards)b.push_back(value);}
 // v13 per-map table in identity order. These fixtures hold monster tables,
 // removed objects, MakeNothingHere records and explored bits only.
 put(b,s.maps.size(),2);
 for(const auto &m:s.maps){
  b.push_back(static_cast<unsigned>(XeenSide(m.id.side)));put(b,m.id.number,2);b.push_back(m.monsters.has_value());
  if(m.monsters){put(b,m.monsters->originalCount,2);put(b,m.monsters->slots.size(),2);
   for(const auto &a:m.monsters->slots)b.insert(b.end(),{std::uint8_t(a.x),std::uint8_t(a.y),std::uint8_t(a.lifecycle),std::uint8_t(a.status),std::uint8_t(a.accounted),std::uint8_t(a.spawnedType),std::uint8_t(std::uint16_t(a.spawnedType)>>8)});}
  check(m.wallItems.empty() && m.cells.empty(),"Independent suffix fixture has no wall-item or cell overrides");
  put(b,m.objects.size(),2);for(const auto &v:m.objects){put(b,v.record,2);b.insert(b.end(),{std::uint8_t(v.x),std::uint8_t(v.y),std::uint8_t(v.flipped)});}
  put(b,0,2);put(b,0,2);
  put(b,m.events.size(),2);for(const auto &v:m.events){put(b,v.record,2);b.push_back(v.opcode);}
  b.push_back(m.explored.has_value());if(m.explored)b.insert(b.end(),m.explored->begin(),m.explored->end());
 }
 check(!s.name && !s.headCache,"Independent loose-save fixture requires absent head cache and name");
 b.push_back(0);b.push_back(0);return b;
}
XeenSaveSnapshot sampleCurrent(){
 auto s=currentWireSnapshot();auto &j=*s.journey;s.resources={{1,2},XeenArchiveFingerprint{3,4}};s.camera={23,8,11,XeenDirection::West};s.food=0xbeef;
 j.context->minutes=1000;j.context->ctr24=23;j.random=XeenJourneyRandomState{1,0xf1234567,0x123456789ULL};
 j.treasure->gold=0x12345678;j.treasure->gems=0x87654321;
 for(unsigned i=0;i<30;++i){j.supplements[i].inputs.luck=XeenAttributeValue{17,3};j.supplements[i].inputs.resistances=XeenCombatResistances{std::uint8_t(i),std::uint8_t(i+30),std::uint8_t(i+60),std::uint8_t(i+90),std::uint8_t(i+120),std::uint8_t(i+150),std::uint8_t(i+180),std::uint8_t(i+210),std::uint8_t(255-i),std::uint8_t(254-i)};j.supplements[i].inputs.poisonResistance=XeenAttributeValue{int(i),int(255-i)};
  for(unsigned slot=0;slot<39;++slot)s.characters[i].learnedSpells->at(slot)=i==2?0:std::uint8_t(i*7+slot*3);}
 s.characters[29].learnedSpells->at(38)=255;
 for(unsigned i=0;i<19;++i)s.map(23).monsters->slots[i]={int(i%16),int(i/16),XeenActorLifecycle::Present,XeenActorStatus::Physical,false,-1};return s;
}
void defeat(XeenSaveSnapshot &s,unsigned record){auto &a=s.map(23).monsters->slots[record];a.x=a.y=-128;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;}
// Truncation at every suffix byte is rejected: checked exhaustively once per
// distinct suffix layout (exhaustive=true), and at a coprime stride over the
// variants that only change field values, so no layout loses that coverage
// while the test stops decoding tens of thousands of truncated saves.
void verify(const XeenSaveSnapshot &fixture,bool exhaustive=false){
 const auto s=withSources(fixture);
 const auto bytes=XeenSaveFormat::encode(s),expected=suffix(s);const auto offset=baseSize(s);
 check(bytes.size()==offset+expected.size()&&std::equal(expected.begin(),expected.end(),bytes.begin()+offset),"Independent complete v13 schema9/content14 suffix and absent name");
 sameSnapshot(s,XeenSaveFormat::decode(bytes));check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes))==bytes,"Exact current byte continuation");
 const std::size_t stride=exhaustive?1:97;
 for(std::size_t size=offset;size<bytes.size();size+=stride){auto b=bytes;b.resize(size);fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});}
 {auto b=bytes;b.resize(bytes.size()-1);fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});}
 auto extra=bytes;extra.push_back(0);fixIndependentEnvelope(extra);rejects([&]{XeenSaveFormat::decode(extra);});
}
void rejected(const XeenSaveSnapshot &fixture){rejects([&]{XeenSaveFormat::encode(withSources(fixture));});}
}
int main(){try{
 auto s=sampleCurrent();verify(s,true);auto populated=s;auto &t=*populated.journey->treasure;t.pendingMask=(1u<<3)|(1u<<9);t.pendingGold=20;t.weapons[0]={9,{0,30,0,0}};t.armor[0]={3,{0,2,0,0}};
 for(unsigned id:{3u,9u})defeat(populated,id);
 populated.characters[0].conditions[3]=2;populated.characters[1].conditions[8]=1;verify(populated,true);
 // Literal sources: F3 records 3 and 9 (side 0, map 23 little-endian, record),
 // bits 0 and 1, the weapon from #9 (index 1) and the armor from #3 (index 0).
 {const auto wire=XeenSaveFormat::encode(withSources(populated));const auto at=baseSize(populated)+1616;
  const Bytes literal{0x78,0x56,0x34,0x12, 0x21,0x43,0x65,0x87, 3,0,0,0, 20,0,0,0, 2, 0,23,0,3,0, 0,23,0,9,0, 1,1, 1,0,30,0,0, 0,0,2,0,0, 0};
  check(std::equal(literal.begin(),literal.end(),wire.begin()+at),"Literal v13 treasure block with its source identities");}
 auto dormant=populated;dormant.journey->treasure=xeenPrepareMonsterGoldForfeiture(ids(*dormant.journey->treasure));verify(dormant);
 auto misc=populated;
 for(unsigned id:{1u,2u})defeat(misc,id);
 misc.journey->treasure->miscellaneous[0]={1,{1,1,1,0}};
 misc.journey->treasure->miscellaneous[1]={2,{9,15,8,0}};verify(misc,true);
 misc.journey->treasure=xeenPrepareMonsterGoldForfeiture(ids(*misc.journey->treasure));verify(misc);
 check(misc.journey->treasure->dormant(),"Restorable Misc retains dormant state");
 for(unsigned mode=0;mode<13;++mode){auto bad=misc;auto &entry=bad.journey->treasure->miscellaneous[0];
  switch(mode){case 0:entry.source=9;break;case 1:entry.source=12;break;case 2:entry.item.material=0;break;case 3:entry.item.material=10;break;
   case 4:entry.item.id=0;break;case 5:entry.item.id=16;break;case 6:entry.item.state=0;break;case 7:entry.item.state=9;break;case 8:entry.item.frame=1;break;
   case 9:entry={};break;case 10:bad.map(23).monsters->slots[1].accounted=false;break;
   case 11:bad.map(23).monsters->slots[1]={5,5,XeenActorLifecycle::Present,XeenActorStatus::Physical,false,-1};break; // A live source.
   case 12:bad.journey->treasure->miscellaneous[1].source=1;break;}
  rejected(bad);}
 // Sources 1, 2, 3, 9 (four identities) precede the two weapon/armor records;
 // the Misc count follows them, then each Misc record's source index.
 const auto miscWire=XeenSaveFormat::encode(withSources(misc));const auto miscAt=baseSize(misc)+1632+1+4*5+2+2*5;
 check(miscWire[miscAt]==2 && miscWire[miscAt+1]==0 && miscWire[miscAt+6]==1,"Literal Misc section extent/source order");
 for(const auto mutation:std::vector<std::pair<unsigned,unsigned>>{{0,11},{1,9},{1,12},{2,0},{2,10},{3,0},{3,16},{4,0},{4,9},{5,1},{6,0}}) {
  auto bad=miscWire;bad[miscAt+mutation.first]=mutation.second;fixIndependentEnvelope(bad);rejects([&]{XeenSaveFormat::decode(bad);});
 }
 auto fullMisc=misc;auto &fm=*fullMisc.journey->treasure;fm.weapons={};fm.armor={};fm.miscellaneous={};
 for(unsigned id=0;id<12;++id){defeat(fullMisc,id);
  if(id<10)fm.miscellaneous[id]={std::uint8_t(id),{9,15,8,0}};else if(id==10)fm.weapons[0]={10,{0,33,0,0}};else fm.armor[0]={11,{0,7,0,0}};}
 verify(fullMisc,true);
 auto excess=XeenSaveFormat::encode(withSources(fullMisc));excess[baseSize(fullMisc)+1632+1+12*5+2+2*5]=11;fixIndependentEnvelope(excess);rejects([&]{XeenSaveFormat::decode(excess);});
 // Delivery results survive the generic current-format round-trip without replay.
 auto delivered=populated;std::array<XeenCharacter,6> recipients;
 for(unsigned i=0;i<6;++i){recipients[i]=delivered.characters[i];recipients[i].weapons.fill({0,1,0,0});recipients[i].armor.fill({0,1,0,0});recipients[i].miscellaneous.fill({1,1,1,0});recipients[i].accessories={};}
 recipients[0].weapons.back().frame=4;recipients[0].armor.back().frame=3;recipients[0].miscellaneous.back().frame=7;
 defeat(delivered,1);
 delivered.journey->treasure->miscellaneous[0]={1,{3,8,7,0}};
 const auto delivery=xeenPrepareMonsterDelivery(ids(*delivered.journey->treasure),recipients);
 for(unsigned i=0;i<6;++i)delivered.characters[i]=delivery.characters[i];
 delivered.journey->treasure=xeenPrepareMonsterGoldCredit(delivery.treasure);
 const auto deliveredBytes=XeenSaveFormat::encode(withSources(delivered));sameSnapshot(withSources(delivered),XeenSaveFormat::decode(deliveredBytes));
 check(XeenSaveFormat::encode(XeenSaveFormat::decode(deliveredBytes))==deliveredBytes,"Delivery and currency round-trip without replay");
 auto later=dormant;later.journey->treasure->pendingMask=1u<<3;later.journey->treasure->pendingGold=10;verify(later);
 auto maximum=populated;auto &mt=*maximum.journey->treasure;mt.pendingMask=4095;mt.pendingGold=120;mt.weapons={};mt.armor={};
 for(unsigned id=0;id<12;++id){defeat(maximum,id);if(id<10)mt.weapons[id]={std::uint8_t(id),{0,30,0,0}};else mt.armor[id-10]={std::uint8_t(id),{0,2,0,0}};}
 verify(maximum);maximum.journey->treasure=xeenPrepareMonsterGoldForfeiture(ids(mt));verify(maximum);
 auto city=populated;city.camera={28,16,2,XeenDirection::North};city.map(28).monsters=XeenSaveMonsterTable{46,{}};
 for(unsigned i=0;i<46;++i)city.map(28).monsters->slots.push_back({int(i%16),int(i/16),XeenActorLifecycle::Present,XeenActorStatus::Physical,false,-1});verify(city,true);
 auto reset=city;save_test::setDisabledEvents(reset,{{28,764}});
 for(unsigned i=46;i<52;++i)reset.map(28).monsters->slots.push_back({0,0,i<50?XeenActorLifecycle::Unresolved:XeenActorLifecycle::Present,XeenActorStatus::Physical,false,i<50?std::int16_t(-1):std::int16_t(0)});
 verify(reset,true);
 for(unsigned slot:{50u,51u}) {
  auto bad=reset;bad.map(28).monsters->slots.at(slot).spawnedType=-1;
  rejected(bad);
 }
 auto flagged=populated;flagged.journey->worldFlags->values[16]=true;verify(flagged);
 const auto bytes=XeenSaveFormat::encode(withSources(populated));const auto offset=baseSize(populated);
 const auto badByte=[&](unsigned at,unsigned value){auto b=bytes;b[offset+at]=value;fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});};
 // Literal v13 suffix offsets for `populated` (two sources): context marker 5,
 // profile 6, difficulty 7, rested/newDay 37-38, supplement count 41 and
 // owners at 42+41*i, RNG algorithm 1272, resistance count 1285 and owner
 // 1286, mask 1624, pending gold 1628, sources 1632.., weapon/armor counts
 // 1643/1644, weapon 1645-1649, armor 1650-1654, Misc count 1655, world
 // flags 1656, learned spells 1784, poison 2985, economy 3076, awards 4240
 // and the per-map table at 8111 (map 23's slots from 8121).
 for(unsigned at:{5u,37u,38u})badByte(at,2);badByte(6,1);badByte(7,2);badByte(41,29);badByte(1272,2);badByte(1285,1);badByte(1286,20);
 for(unsigned i=0;i<30;++i){badByte(42+41*i,31);badByte(42+41*i+34,1);}
 for(unsigned at:{1285u,1286u,1643u,1644u})badByte(at,31);
 for(unsigned at:{1646u,1648u,1649u,1651u,1653u,1654u})badByte(at,1);
 badByte(1645,0);badByte(1647,34);badByte(1652,8);badByte(1624,0);badByte(1628,10);
 badByte(1632,3);badByte(1633,1);badByte(1634,24);badByte(1636,4); // Source count, side, map and record.
 badByte(1655,2);badByte(1784,29);badByte(1785,1);badByte(1825,0);badByte(2985,29);badByte(2986,1);badByte(2989,0);badByte(3076,3);
 badByte(8111,2);badByte(8114,0);badByte(8116,2);badByte(8117,20);badByte(8119,108);
 for(unsigned i=0;i<19;++i){const auto at=8121+7*i;badByte(at+2,4);badByte(at+3,2);badByte(at+4,2);badByte(at+5,0);if(i!=3 && i!=9)badByte(at+4,1);}
 for(unsigned mode=0;mode<15;++mode){auto bad=dormant;auto &j=*bad.journey;
  switch(mode){case 0:j.skeletonSeed=1;break;case 1:j.supplements[0].inputs.luck.reset();break;case 2:j.treasure.reset();break;case 3:j.supplements[29].inputs.resistances.reset();break;
   case 4:bad.map(23).monsters->slots[9].accounted=false;break;case 5:bad.map(23).monsters->slots[9]={5,5,XeenActorLifecycle::Present,XeenActorStatus::Physical,false,-1};break;
   case 6:j.treasure->pendingGold=10;break;case 7:j.treasure->armor[0].source=9;break;case 8:j.treasure->weapons[0].item.frame=1;break;case 9:j.treasure->weapons[0].source=12;break;case 10:j.worldFlags.reset();break;case 11:bad.characters[29].learnedSpells.reset();break;case 12:j.supplements[29].inputs.poisonResistance.reset();break;case 13:j.serviceEconomy.reset();break;case 14:j.random.reset();break;}
  rejected(bad);}
 // The format is map-agnostic: city object and Event overrides need no
 // retained city table (restore binds them to the city's MOB and EVT).
 for(const auto &source:{s,city,reset})for(unsigned record:{0u,539u,761u,764u}){
  auto bad=source;if(record==0)save_test::setRemovedObjects(bad,{{28,0}});else save_test::setDisabledEvents(bad,{{28,record}});
  verify(bad);
 }
 std::cout<<"Current regional literal wire, all city variants, treasure sources, learned fields and malformed controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
