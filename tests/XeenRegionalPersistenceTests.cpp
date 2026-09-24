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

  // Artificial 5/5 codec controls; retained item provenance is independent of gold.
  auto five=four;five.journey->schema=five.journey->contract=5;
  five.journey->treasure=xeenPrepareMonsterGoldForfeiture(*five.journey->treasure,5);
  five.characters[0].currentHp=15;five.characters[0].conditions[13]=7;
  five.characters[1].conditions[12]=1;five.characters[1].conditions[13]=2;
  auto fiveBytes=XeenSaveFormat::encode(five);
  check(fiveBytes.size()-offset==1830 && fiveBytes[offset+1]==5 && fiveBytes[offset+3]==5,"Schema-5 exact discriminator and unchanged extent");
  for(unsigned n=1810;n<1818;++n)check(fiveBytes[offset+n]==0,"Dormant wire has no gold obligations");
  sameSnapshot(five,XeenSaveFormat::decode(fiveBytes));
  check(XeenSaveFormat::encode(XeenSaveFormat::decode(fiveBytes))==fiveBytes,"Schema-5 complete exact roundtrip");
  auto legacyDormant=five;legacyDormant.journey->schema=legacyDormant.journey->contract=4;rejects([&]{XeenSaveFormat::encode(legacyDormant);});
  for(unsigned mode=0;mode<8;++mode){auto bad=five;
   if(mode==0)bad.journey->schema=4;
   if(mode==1)bad.journey->contract=4;
   if(mode==2)bad.journey->actors[9].accounted=false;
   if(mode==3)bad.journey->actors[9].hp=1;
   if(mode==4)bad.journey->treasure->pendingGold=10;
   if(mode==5)bad.journey->treasure->armor[0].source=9;
   if(mode==6)bad.journey->treasure->weapons[0].item.frame=1;
   if(mode==7)bad.journey->treasure->weapons[0].source=12;
   rejects([&]{XeenSaveFormat::encode(bad);});
  }
  const auto badFive=[&](unsigned at,unsigned value){auto b=fiveBytes;b[offset+at]=value;fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});};
  for(unsigned value:{0u,1u,2u,3u,4u,6u,255u}){badFive(1,value);badFive(3,value);}
  for(unsigned at:{1651u,1652u,1818u,1819u})badFive(at,31);
  for(unsigned at:{1821u,1823u,1824u,1826u,1828u,1829u})badFive(at,1);
  badFive(1820,3);badFive(1822,34);badFive(1827,8);badFive(1814,10);
  auto dormantMaximum=maximum;dormantMaximum.journey->schema=dormantMaximum.journey->contract=5;
  dormantMaximum.journey->treasure=xeenPrepareMonsterGoldForfeiture(*dormantMaximum.journey->treasure,5);
  const auto maxFive=XeenSaveFormat::encode(dormantMaximum);
  check(maxFive.size()-offset==1880,"Schema-5 maximum 12 dormant sources extent");
  sameSnapshot(dormantMaximum,XeenSaveFormat::decode(maxFive));
  auto later=five;later.journey->treasure->pendingMask=1u<<3;later.journey->treasure->pendingGold=10;
  sameSnapshot(later,XeenSaveFormat::decode(XeenSaveFormat::encode(later)));
  for(std::size_t size=offset;size<fiveBytes.size();++size){auto b=fiveBytes;b.resize(size);fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});}
  auto extraFive=fiveBytes;extraFive.push_back(0);fixIndependentEnvelope(extraFive);rejects([&]{XeenSaveFormat::decode(extraFive);});
	  auto six=five;six.journey->schema=six.journey->contract=6;six.journey->regionalRecovery=XeenRegionalRecoveryState{false};
	  const auto sixBytes=XeenSaveFormat::encode(six);
	  check(sixBytes.size()-offset==1831 && sixBytes[offset+1]==6 && sixBytes[offset+3]==6 && sixBytes.back()==0,
		  "Schema-6 discriminator, extent and flag");
	  sameSnapshot(six,XeenSaveFormat::decode(sixBytes));
	  six.journey->regionalRecovery->worldFlag16=true;
	  const auto flagged=XeenSaveFormat::encode(six);
	  check(flagged.back()==1 && flagged.size()==sixBytes.size(),"Schema-6 canonical flag byte");
	  sameSnapshot(six,XeenSaveFormat::decode(flagged));
	  auto badSix=flagged;badSix.back()=2;fixIndependentEnvelope(badSix);rejects([&]{XeenSaveFormat::decode(badSix);});
	  badSix=flagged;badSix.pop_back();fixIndependentEnvelope(badSix);rejects([&]{XeenSaveFormat::decode(badSix);});
	  badSix=flagged;badSix.push_back(0);fixIndependentEnvelope(badSix);rejects([&]{XeenSaveFormat::decode(badSix);});
	  auto missing=six;missing.journey->regionalRecovery.reset();rejects([&]{XeenSaveFormat::encode(missing);});
	  auto crossed=six;crossed.journey->contract=5;rejects([&]{XeenSaveFormat::encode(crossed);});
	  crossed=six;crossed.journey->schema=5;rejects([&]{XeenSaveFormat::encode(crossed);});
	  auto seven=six;seven.journey->schema=seven.journey->contract=7;
	  for(unsigned owner=0;owner<30;++owner) {
	   XeenCharacter::XeenLearnedSpells book{};
	   if(owner!=2)for(unsigned slot=0;slot<39;++slot)book[slot]=std::uint8_t((owner*7+slot*3)&255);
	   seven.characters[owner].learnedSpells=book;
	  }
	  seven.characters[29].learnedSpells->at(38)=255;
	  const auto sevenBytes=XeenSaveFormat::encode(seven);
	  check(sevenBytes.size()-offset==3032 && sevenBytes[offset+1]==7 && sevenBytes[offset+3]==7,
	   "Schema-7 exact suffix extent and discriminator");
	  check(std::equal(sevenBytes.begin()+20,sevenBytes.begin()+offset,sixBytes.begin()+20),
	   "Schema-7 changed the common character wire prefix");
	  constexpr unsigned knowledge=1831;
	  check(sevenBytes[offset+knowledge]==30 && sevenBytes[offset+knowledge+1]==0 &&
	   sevenBytes[offset+knowledge+2]==0 && sevenBytes[offset+knowledge+2+38]==114 &&
	   sevenBytes[offset+knowledge+1+40*2]==2 && sevenBytes[offset+knowledge+2+40*2]==0 &&
	   sevenBytes[offset+knowledge+1+40*29]==29 && sevenBytes.back()==255,
	   "Schema-7 literal owner sequence and raw learned bytes");
	  sameSnapshot(seven,XeenSaveFormat::decode(sevenBytes));
	  check(XeenSaveFormat::encode(XeenSaveFormat::decode(sevenBytes))==sevenBytes,
	   "Schema-7 exact byte continuation");
	  auto wrongBooks=six;wrongBooks.characters[0].learnedSpells=XeenCharacter::XeenLearnedSpells{};
	  rejects([&]{XeenSaveFormat::encode(wrongBooks);});
	  wrongBooks=seven;wrongBooks.characters[29].learnedSpells.reset();rejects([&]{XeenSaveFormat::encode(wrongBooks);});
	  wrongBooks=seven;wrongBooks.journey->schema=6;rejects([&]{XeenSaveFormat::encode(wrongBooks);});
	  wrongBooks=seven;wrongBooks.journey->contract=6;rejects([&]{XeenSaveFormat::encode(wrongBooks);});
	  const auto badSeven=[&](unsigned at,unsigned value){auto wire=sevenBytes;wire[offset+at]=value;fixIndependentEnvelope(wire);rejects([&]{XeenSaveFormat::decode(wire);});};
	  badSeven(knowledge,29);badSeven(knowledge+1,1);badSeven(knowledge+1+40,0);
	  badSeven(1,6);badSeven(3,6);
	  for(std::size_t size=offset+knowledge;size<sevenBytes.size();++size){
	   auto wire=sevenBytes;wire.resize(size);fixIndependentEnvelope(wire);rejects([&]{XeenSaveFormat::decode(wire);});
	  }
	  auto extraSeven=sevenBytes;extraSeven.push_back(0);fixIndependentEnvelope(extraSeven);rejects([&]{XeenSaveFormat::decode(extraSeven);});
	  // Artificial schema-8 wire controls. The expected suffix is written here
	  // independently of the production encoder and its decoded output.
	  auto eight=seven;eight.journey->schema=eight.journey->contract=8;
	  for(unsigned owner=0;owner<30;++owner)
	   eight.journey->supplements[owner].inputs.poisonResistance=XeenAttributeValue{int(owner),int(255-owner)};
	  auto expectedEight=sevenBytes;expectedEight[offset+1]=8;expectedEight[offset+3]=8;
	  expectedEight.push_back(30);
	  for(unsigned owner=0;owner<30;++owner){expectedEight.push_back(owner);expectedEight.push_back(owner);expectedEight.push_back(255-owner);}
	  expectedEight.push_back(0);fixIndependentEnvelope(expectedEight);
	  auto eightBytes=XeenSaveFormat::encode(eight);
	  check(eightBytes==expectedEight && eightBytes.size()-offset==3124,"Schema-8 unvisited literal wire and extent");
	  const auto badEight=[&](const Bytes &source,unsigned at,unsigned value){auto wire=source;wire[offset+at]=value;fixIndependentEnvelope(wire);rejects([&]{XeenSaveFormat::decode(wire);});};
	  constexpr unsigned poison=3032;
	  badEight(eightBytes,poison,29);badEight(eightBytes,poison+1,1);
	  badEight(eightBytes,poison+4,0);badEight(eightBytes,poison+91,2);
	  auto missingPoison=eight;missingPoison.journey->supplements[29].inputs.poisonResistance.reset();rejects([&]{XeenSaveFormat::encode(missingPoison);});
	  auto city=eight;
	  city.journey->vertigoActors.emplace();
	  for(unsigned i=0;i<46;++i)city.journey->vertigoActors->push_back({{28,i},int(i%16),int(i/16),2,false,XeenActorLifecycle::Present,XeenActorStatus::Physical,false});
	  city.camera={28,16,2,XeenDirection::North};
	  auto expectedCity=expectedEight;expectedCity.back()=1;
	  const auto push16=[&](Bytes &b,unsigned value){b.push_back(value&255);b.push_back((value>>8)&255);};
	  const auto push32=[&](Bytes &b,unsigned value){for(unsigned n=0;n<4;++n)b.push_back((value>>(8*n))&255);};
	  push16(expectedCity,46);push16(expectedCity,46);
	  for(unsigned i=0;i<46;++i){expectedCity.push_back(0);push16(expectedCity,28);push32(expectedCity,i);
	   push16(expectedCity,i%16);push16(expectedCity,i/16);push32(expectedCity,2);
	   expectedCity.insert(expectedCity.end(),{0,0,0,0});}
	  expectedCity[46]=28;expectedCity[47]=0;expectedCity[48]=16;expectedCity[49]=2;expectedCity[50]=0;
	  fixIndependentEnvelope(expectedCity);
	  auto cityBytes=XeenSaveFormat::encode(city);
	  check(cityBytes==expectedCity && cityBytes.size()-offset==4002,"Schema-8 visited-46 literal wire and extent");
	  badEight(cityBytes,poison+92,47);badEight(cityBytes,poison+94,45);
	  auto reset=city;
	  for(unsigned i=46;i<52;++i)reset.journey->vertigoActors->push_back({{28,i},0,0,i<50?0:2,false,
	   i<50?XeenActorLifecycle::Unresolved:XeenActorLifecycle::Present,XeenActorStatus::Physical,false});
	  auto expectedReset=expectedCity;expectedReset[offset+poison+94]=52;
	  for(unsigned i=46;i<52;++i){expectedReset.push_back(0);push16(expectedReset,28);push32(expectedReset,i);
	   push16(expectedReset,0);push16(expectedReset,0);push32(expectedReset,i<50?0:2);
	   expectedReset.insert(expectedReset.end(),{0,std::uint8_t(i<50?2:0),0,0});}
	  fixIndependentEnvelope(expectedReset);
	  auto resetBytes=XeenSaveFormat::encode(reset);
	  check(resetBytes==expectedReset && resetBytes.size()-offset==4116,"Schema-8 visited-52 literal wire and extent");
	  badEight(resetBytes,poison+94,51);
	  for(const auto &wire:{eightBytes,cityBytes,resetBytes}){
	   check(XeenSaveFormat::encode(XeenSaveFormat::decode(wire))==wire,"Schema-8 exact byte continuation");
	   auto truncated=wire;truncated.pop_back();fixIndependentEnvelope(truncated);rejects([&]{XeenSaveFormat::decode(truncated);});
	   auto appended=wire;appended.push_back(0);fixIndependentEnvelope(appended);rejects([&]{XeenSaveFormat::decode(appended);});
	  }
	  auto mixed=eight;mixed.journey->contract=7;rejects([&]{XeenSaveFormat::encode(mixed);});
	  // Forge the base overlay records directly, with an independent repaired
	  // envelope: semantic rejection must not depend on the trusted encoder.
	  const auto overlayWire=[&](Bytes wire, bool object, unsigned record) {
	   const auto insertion=offset-(object?4u:0u);
	   wire.at(offset-(object?8u:4u))=1;
	   Bytes identity{0,28,0};push32(identity,record);
	   wire.insert(wire.begin()+insertion,identity.begin(),identity.end());
	   fixIndependentEnvelope(wire);return wire;
	  };
	  for(unsigned record:{539u,761u}) {
	   const auto forged=overlayWire(cityBytes,false,record);
	   rejects([&]{XeenSaveFormat::decode(forged);});
	  }
	  for(const auto &source:{cityBytes,resetBytes,eightBytes}) {
	   const auto object=overlayWire(source,true,0);
	   rejects([&]{XeenSaveFormat::decode(object);});
	  }
	  const auto absentOverlay=overlayWire(eightBytes,false,764);
	  rejects([&]{XeenSaveFormat::decode(absentOverlay);});
	  for(const auto &source:{cityBytes,resetBytes}) {
	   const auto valid=overlayWire(source,false,764);
	   const auto decoded=XeenSaveFormat::decode(valid);
	   check(decoded.disabledEvents==std::vector<XeenEventIdentity>{{28,764}},"exact city protection overlay rejected");
	  }
	  mixed=eight;mixed.journey->schema=7;rejects([&]{XeenSaveFormat::encode(mixed);});
		std::cout << "Schema-3 exact layout, full coverage and malformed-wire controls passed\n";
		return 0;
	} catch(const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
}
