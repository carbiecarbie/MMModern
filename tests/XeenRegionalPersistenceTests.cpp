#include "XeenTreasureDeliveryTestSupport.h"
#include "XeenSaveTestSupport.h"
#include "games/xeen/XeenItemRewards.h"
#include <iostream>
#include <map>
using namespace save_test;
namespace {
void put(Bytes &b,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)b.push_back(v>>(8*i));}
std::size_t baseSize(const XeenSaveSnapshot &s){
 // Header 20, fingerprints 25, Inn flag 1, camera 6, member and roster counts 2, then the
 // members, quest counters and flags and game flags; v13 writes no top-level
 // overlay list before the Journey block.
 std::size_t n=20+25+1+6+2+s.activeRosterIds.size()+4*s.questItems.size()+s.questFlags.size()+s.gameFlags.size()+4;
 // v19 adds seven roster bytes after the four mutable beacon bytes per character before the Journey block.
 for(const auto &c:s.characters)n+=239+c.name.size();return n;
}
Bytes suffix(const XeenSaveSnapshot &s){
 const auto &j=*s.journey;Bytes b{3,9,0,14,0,1,0,0};
 for(unsigned value:{23u,8u,610u,1000u})put(b,value,2);
 b.resize(39);put(b,0xbeef,2);b.push_back(30);
 for(unsigned owner=0;owner<30;++owner){b.push_back(owner);for(unsigned i=0;i<8;++i)put(b,0,4);put(b,17,4);put(b,3,4);}
 b.push_back(1);put(b,0xf1234567,4);put(b,0x123456789ULL,8);
 // v13 has no Journey actor table here; the monster tables follow the awards.
 b.push_back(30);for(unsigned i=0;i<30;++i)for(unsigned v:{i,i,i+30,i+60,i+90,i+120,i+150,i+180,i+210,255-i,254-i})b.push_back(v);
 // v17: the purse alone; pending treasure is never saved.
 put(b,j.purse->gold,4);put(b,j.purse->gems,4);
 // Independent v21 play-time/completion bytes after the purse.
 put(b,0xfedcba98u,4);b.push_back(1);
 for(bool flag:j.worldFlags->values)b.push_back(flag);b.push_back(30);
 for(unsigned owner=0;owner<30;++owner){b.push_back(owner);for(unsigned slot=0;slot<39;++slot)b.push_back(owner==2?0:owner==29&&slot==38?255:std::uint8_t(owner*7+slot*3));}
 b.push_back(30);for(unsigned owner=0;owner<30;++owner)b.insert(b.end(),{std::uint8_t(owner),std::uint8_t(owner),std::uint8_t(255-owner)});
 b.insert(b.end(),{2,4,4,9});for(const auto &side:j.serviceEconomy->wares.records)for(const auto &shop:side)for(const auto &category:shop)for(const auto &item:category)b.insert(b.end(),{item.material,item.id,item.state,item.frame});
 put(b,0xfedcba98u,4);put(b,0xffffffffu,4);
 b.push_back(30);for(unsigned owner=0;owner<30;++owner){b.push_back(owner);for(const auto value:s.characters[owner].awards)b.push_back(value);}
 // v14 per-map table in identity order. These fixtures hold monster tables,
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
  b.push_back(m.tavernTips.has_value());if(m.tavernTips)b.push_back(*m.tavernTips);
 }
 check(!s.name && !s.headCache,"Independent loose-save fixture requires absent head cache and name");
 b.push_back(0);b.push_back(0);return b;
}
XeenSaveSnapshot sampleCurrent(){
 auto s=currentWireSnapshot();auto &j=*s.journey;s.resources={{1,2},XeenArchiveFingerprint{3,4}};s.camera={23,8,11,XeenDirection::West};s.food=0xbeef;
 j.context->minutes=1000;j.context->ctr24=23;j.random=XeenJourneyRandomState{1,0xf1234567,0x123456789ULL};
 j.purse->gold=0x12345678;j.purse->gems=0x87654321;
 j.playTimeSeconds=0xfedcba98u;j.cloudsCompleted=true;
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
 const auto &s=fixture;
 const auto bytes=XeenSaveFormat::encode(s),expected=suffix(s);const auto offset=baseSize(s);
 check(bytes.size()==offset+expected.size()&&std::equal(expected.begin(),expected.end(),bytes.begin()+offset),"Independent complete v21 schema9/content14 suffix and absent name");
 sameSnapshot(s,XeenSaveFormat::decode(bytes));check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes))==bytes,"Exact current byte continuation");
 const std::size_t stride=exhaustive?1:97;
 for(std::size_t size=offset;size<bytes.size();size+=stride){auto b=bytes;b.resize(size);fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});}
 {auto b=bytes;b.resize(bytes.size()-1);fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});}
 auto extra=bytes;extra.push_back(0);fixIndependentEnvelope(extra);rejects([&]{XeenSaveFormat::decode(extra);});
}
void rejected(const XeenSaveSnapshot &fixture){rejects([&]{XeenSaveFormat::encode(fixture);});}
}
int main(){try{
 auto s=sampleCurrent();verify(s,true);auto populated=s;
 for(unsigned id:{3u,9u})defeat(populated,id);
 populated.characters[0].conditions[3]=2;populated.characters[1].conditions[8]=1;verify(populated,true);
 // Literal v21 purse (gold, then gems), play time/completion and world flags.
 {const auto wire=XeenSaveFormat::encode(populated);const auto at=baseSize(populated)+1616;
  const Bytes literal{0x78,0x56,0x34,0x12, 0x21,0x43,0x65,0x87, 0x98,0xba,0xdc,0xfe,1,0};
  check(std::equal(literal.begin(),literal.end(),wire.begin()+at),"Literal v21 purse, play time and completion without pending treasure");}
 // Delivered items and the credited purse survive the round-trip without replay.
 auto delivered=populated;std::array<XeenCharacter,6> recipients;
 for(unsigned i=0;i<6;++i){recipients[i]=delivered.characters[i];recipients[i].weapons.fill({0,1,0,0});recipients[i].armor.fill({0,1,0,0});recipients[i].miscellaneous.fill({1,1,1,0});recipients[i].accessories={};}
 recipients[0].weapons.back().frame=4;recipients[0].armor.back().frame=3;recipients[0].miscellaneous.back().frame=7;
 XeenTreasureStaging staged;staged.gold=10;staged.ready=true;staged.miscellaneous[0]={3,8,7,0};
 const auto deliveredCharacters=[](const XeenTreasureStaging &staged,const std::array<XeenCharacter,6> &c) {
  XeenPartyState party;std::vector<std::uint8_t> ids;
  for(const auto &v:c){party.roster.at(v.rosterId)=v;ids.push_back(v.rosterId);}
  party.party=XeenParty::fromRosterIds(ids);xeenTestDeliverTreasure(staged,party);
  std::array<XeenCharacter,6> result;for(unsigned i=0;i<6;++i)result[i]=party.roster.at(c[i].rosterId);return result;
 };
 const auto delivery=deliveredCharacters(staged,recipients);
 for(unsigned i=0;i<6;++i)delivered.characters[i]=delivery[i];
 delivered.journey->purse=xeenCreditTreasure(*delivered.journey->purse,staged);
 check(delivered.journey->purse->gold==0x12345682u,"Delivery credit");verify(delivered);
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
 const auto bytes=XeenSaveFormat::encode(populated);const auto offset=baseSize(populated);
 const auto badByte=[&](unsigned at,unsigned value){auto b=bytes;b[offset+at]=value;fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});};
 // Literal v21 suffix offsets for `populated`: context marker 5, profile 6,
 // difficulty 7, rested/newDay 37-38, supplement count 41 and owners at
 // 42+41*i, RNG algorithm 1272, resistance count 1285 and owner 1286, purse
 // 1616, play time 1624, completion 1628, world flags 1629, learned spells
 // 1757, poison 2958, economy 3049, awards 4213 and the per-map table at
 // 8084 (map 23's slots from 8094).
 for(unsigned at:{5u,37u,38u})badByte(at,2);badByte(6,1);badByte(7,2);badByte(41,29);badByte(1272,2);badByte(1285,1);badByte(1286,20);
 for(unsigned i=0;i<30;++i){badByte(42+41*i,31);badByte(42+41*i+34,1);}
 for(unsigned at:{1285u,1286u})badByte(at,31);
 badByte(1628,2);badByte(1629,2);badByte(1757,29);badByte(1758,1);badByte(1798,0);badByte(2958,29);badByte(2959,1);badByte(2962,0);badByte(3049,3);
 badByte(8084,2);badByte(8087,0);badByte(8089,2);badByte(8090,20);badByte(8092,108);
 for(unsigned i=0;i<19;++i){const auto at=8094+7*i;badByte(at+2,4);badByte(at+3,2);badByte(at+4,2);badByte(at+5,0);if(i!=3 && i!=9)badByte(at+4,1);}
 for(unsigned mode=0;mode<9;++mode){auto bad=populated;auto &j=*bad.journey;
  switch(mode){case 0:j.skeletonSeed=1;break;case 1:j.supplements[0].inputs.luck.reset();break;case 2:j.purse.reset();break;case 3:j.supplements[29].inputs.resistances.reset();break;
   case 4:j.worldFlags.reset();break;case 5:bad.characters[29].learnedSpells.reset();break;case 6:j.supplements[29].inputs.poisonResistance.reset();break;case 7:j.serviceEconomy.reset();break;case 8:j.random.reset();break;}
  rejected(bad);}
 // The format is map-agnostic: city object and Event overrides need no
 // retained city table (restore binds them to the city's MOB and EVT).
 for(const auto &source:{s,city,reset})for(unsigned record:{0u,539u,761u,764u}){
  auto bad=source;if(record==0)save_test::setRemovedObjects(bad,{{28,0}});else save_test::setDisabledEvents(bad,{{28,record}});
  verify(bad);
 }
 std::cout<<"Current regional literal wire, all city variants, purse, learned fields and malformed controls passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
