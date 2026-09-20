#include "XeenSaveTestSupport.h"
#include <iostream>
using namespace save_test;
int main() {
	try {
		XeenSaveSnapshot s;s.resources.clouds={1,2};s.resources.darkside=XeenArchiveFingerprint{3,4};s.camera={23,8,11,XeenDirection::West};
		s.journey.emplace();auto &j=*s.journey;j.schema=j.contract=3;j.initializedMap=23;j.originalActorCount=19;j.skeletonSeed=0;
		j.context=XeenGameplayContext{};j.context->day=42;j.context->year=611;j.context->minutes=1000;j.context->ctr24=23;
		j.random=XeenJourneyRandomState{1,0xf1234567,0x123456789ULL};
		for (unsigned i=0;i<30;++i) {j.supplements[i].owner=i;j.supplements[i].inputs.luck=XeenAttributeValue{17,3};}
		for (unsigned i=0;i<19;++i) j.actors.push_back({{23,i},int(i%16),int(i/16),int(i+1),bool(i%2),XeenActorLifecycle::Present,XeenActorStatus::Physical,false});
		const auto bytes=XeenSaveFormat::encode(s);auto base=s;base.journey.reset();const auto offset=XeenSaveFormat::encode(base).size();
		check(bytes.size()-offset==1651,"Schema-3 exact suffix length");
		check(bytes[offset]==3 && bytes[offset+1]==3 && bytes[offset+3]==3 && bytes[offset+5]==1 && bytes[offset+39]==30,"Schema-3 literal prefix");
		check(bytes[offset+1270]==1 && bytes[offset+1271]==0x67 && bytes[offset+1274]==0xf1 && bytes[offset+1275]==0x89,"Schema-3 literal random fields");
		check(bytes[offset+1283]==0 && bytes[offset+1284]==23 && bytes[offset+1286]==19 && bytes[offset+1288]==19,"Schema-3 literal map and counts");
		for(unsigned i=0;i<19;++i)check(bytes[offset+1290+19*i]==0 && bytes[offset+1291+19*i]==23 && bytes[offset+1293+19*i]==i,"Complete ordered wire identities");
		check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes))==bytes,"Complete representation roundtrip");
		const auto badByte=[&](std::size_t at,unsigned value){auto b=bytes;b[offset+at]=value;fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});};
		for(unsigned value:{0U,1U,2U,4U,255U}) {badByte(1,value);badByte(3,value);}
		for(unsigned value:{0U,1U,18U,20U,107U,108U,255U}) {badByte(1286,value);badByte(1288,value);}
		for(auto at:{5U,37U,38U})badByte(at,2);
		badByte(6,1);badByte(7,2);badByte(39,29);badByte(1270,2);badByte(1283,1);badByte(1284,20);
		for(unsigned i=0;i<30;++i) {badByte(40+41*i,31);badByte(40+41*i+34,1);}
		for(unsigned i=0;i<19;++i) {
			const auto at=1290+19*i;
			badByte(at,1);badByte(at+1,20);badByte(at+3,19);
			badByte(at+15,2);badByte(at+16,4);badByte(at+17,2);badByte(at+18,2);
		}
		for(std::size_t size=offset;size<bytes.size();++size) {auto b=bytes;b.resize(size);fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});}
		auto extra=bytes;extra.push_back(0);fixIndependentEnvelope(extra);rejects([&]{XeenSaveFormat::decode(extra);});
		auto wrong=s;wrong.journey->skeletonSeed=1;rejects([&]{XeenSaveFormat::encode(wrong);});
		wrong=s;wrong.journey->supplements[0].inputs.luck.reset();rejects([&]{XeenSaveFormat::encode(wrong);});

  // Explicit artificial wire fixture, not a production treasure witness.
  auto four=s;four.journey->schema=four.journey->contract=4;four.journey->treasure.emplace();
  auto &treasure=*four.journey->treasure;treasure.gold=0x12345678;treasure.gems=0x87654321;
  for(unsigned i=0;i<30;++i) four.journey->supplements[i].inputs.resistances=XeenCombatResistances{std::uint8_t(i),std::uint8_t(i+30),std::uint8_t(i+60),std::uint8_t(i+90)};
  auto fourBytes=XeenSaveFormat::encode(four);
  check(fourBytes.size()-offset==1820 && fourBytes[offset+1651]==30,"Schema-4 empty suffix length/count");
  for(unsigned i=0;i<30;++i)for(unsigned k=0;k<5;++k)check(fourBytes[offset+1652+i*5+k]==(k?i+30*(k-1):i),"Schema-4 ordered resistance bytes");
  check(fourBytes[offset+1802]==0x78 && fourBytes[offset+1805]==0x12 && fourBytes[offset+1806]==0x21 && fourBytes[offset+1809]==0x87,"Schema-4 literal purse endian");
  sameSnapshot(four,XeenSaveFormat::decode(fourBytes));
  treasure.pendingMask=(1u<<3)|(1u<<9);treasure.pendingGold=20;
  treasure.weapons[0]={9,{0,30,0,0}};treasure.armor[0]={3,{0,2,0,0}};
  for(unsigned id:{3u,9u}) {auto &a=four.journey->actors[id];a.x=a.y=-128;a.hp=0;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;}
  four.characters[0].conditions[3]=2;four.characters[1].conditions[8]=1;
  fourBytes=XeenSaveFormat::encode(four);
  check(fourBytes.size()-offset==1830,"Schema-4 populated suffix length");
  const std::array<unsigned,12> tail{1,1,9,0,30,0,0,3,0,2,0,0};
  for(unsigned i=0;i<tail.size();++i)check(fourBytes[offset+1818+i]==tail[i],"Schema-4 category/source ordered records");
  sameSnapshot(four,XeenSaveFormat::decode(fourBytes));
  check(XeenSaveFormat::encode(XeenSaveFormat::decode(fourBytes))==fourBytes,"Schema-4 exact roundtrip");
  const auto badFour=[&](unsigned at,unsigned value){auto b=fourBytes;b[offset+at]=value;fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});};
  for(unsigned at:{1651u,1652u,1818u,1819u})badFour(at,31);
  for(unsigned at:{1821u,1823u,1824u,1826u,1828u,1829u})badFour(at,1);
  badFour(1820,3);badFour(1822,34);badFour(1827,8);badFour(1810,0);badFour(1814,10);
  for(std::size_t size=offset;size<fourBytes.size();++size){auto b=fourBytes;b.resize(size);fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});}
  for(unsigned mode=0;mode<5;++mode){auto v=four;if(mode==0)v.journey->schema=3;if(mode==1)v.journey->contract=3;if(mode==2)v.journey->treasure.reset();if(mode==3)v.journey->supplements[29].inputs.resistances.reset();if(mode==4)v.journey->actors[9].accounted=false;rejects([&]{XeenSaveFormat::encode(v);});}
  auto maximum=four;auto &maximumTreasure=*maximum.journey->treasure;maximumTreasure.pendingMask=4095;maximumTreasure.pendingGold=120;
  maximumTreasure.weapons={};maximumTreasure.armor={};
  for(unsigned id=0;id<12;++id){auto &a=maximum.journey->actors[id];a.x=a.y=-128;a.hp=0;a.activated=false;a.accounted=true;a.lifecycle=XeenActorLifecycle::Defeated;
   if(id<10)maximumTreasure.weapons[id]={std::uint8_t(id),{0,30,0,0}};else maximumTreasure.armor[id-10]={std::uint8_t(id),{0,2,0,0}};}
  const auto maxBytes=XeenSaveFormat::encode(maximum);check(maxBytes.size()-offset==1880,"Schema-4 maximum 12-source suffix length");sameSnapshot(maximum,XeenSaveFormat::decode(maxBytes));
  auto extraFour=fourBytes;extraFour.push_back(0);fixIndependentEnvelope(extraFour);rejects([&]{XeenSaveFormat::decode(extraFour);});

		std::cout << "Schema-3 exact layout, full coverage and malformed-wire controls passed\n";
		return 0;
	} catch(const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
}
