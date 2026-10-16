#include "XeenSaveTestSupport.h"
#include "games/xeen/XeenSkills.h"
#include "XeenTestInstallation.h"
#include "formats/xeen/XeenAssetSource.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iterator>

using namespace mmodern;
using namespace save_test;

namespace {

void roundTrip(const XeenSaveSnapshot &s) {
	const auto bytes = XeenSaveFormat::encode(s);
	const auto decoded = XeenSaveFormat::decode(bytes);
	sameSnapshot(s, decoded);
	check(bytes == XeenSaveFormat::encode(decoded), "encoding is not deterministic");
}

void headFlagsWire() {
 auto s=currentWireSnapshot();s.headCache=XeenHeadCache{s.camera.mapId,{}};
 for(unsigned i=0;i<256;++i)s.headCache->overrides[i]=XeenHeadIndicators{static_cast<std::uint8_t>(i),static_cast<std::uint8_t>(255-i)};
 for(unsigned i=0;i<128;++i)s.journey->worldFlags->values[i]=(i%3)==0;
 s.journey->worldFlags->values[16]=true;s.journey->worldFlags->values[23]=true;roundTrip(s);
 auto wrong=s;wrong.headCache->mapId=28;rejects([&]{XeenSaveFormat::encode(wrong);},"primary map");
 const auto wire=XeenSaveFormat::encode(s);const auto head=wire.size()-(1+3+2+256*3+1);
 check(wire[head]==1 && wire[head+1]==0 && wire[head+2]==23 && wire[head+4]==0 && wire[head+5]==1,"Literal v11 primary HED cache");
 for(unsigned offset:{0u,1u,2u,4u,5u,9u}) {
  auto invalid=wire;invalid[head+offset]=255;fixIndependentEnvelope(invalid);rejects([&]{XeenSaveFormat::decode(invalid);});
 }
}
void worldEffectsWire() {
 auto s=currentWireSnapshot();s.characters[29].awards[9]=255;s.characters[3].awards[126]=15;
 s.map(23).objects={{0,-128,-1,true},{1,127,0,false}};
 roundTrip(s);
 for(auto bad:s.map(23).objects) {
  auto invalid=s;bad.x=128;invalid.map(23).objects={bad};rejects([&]{XeenSaveFormat::encode(invalid);});
 }
 auto invalid=s;invalid.map(23).objects.push_back(s.map(23).objects[0]);rejects([&]{XeenSaveFormat::encode(invalid);});
 invalid=s;invalid.map(23).objects[0].y=-128;rejects([&]{XeenSaveFormat::encode(invalid);},"object override value"); // A removed object has no flip.
 invalid=s;invalid.characters[29].awards[8]=16;rejects([&]{XeenSaveFormat::encode(invalid);});
 const auto wire=XeenSaveFormat::encode(s);
 // From the end: absent name 1, absent head cache 1, then the per-map table:
 // count 2 and map 23's entry: identity 3, monster flag 1, counts 4, 19 slots
 // of 7, two objects of 5 after their count 2, then wall-item, cell and Event
 // counts 2 each and the explored flag 1; before it, 30 tagged award arrays.
 const std::size_t entry=3+1+4+19*7+2+2*5+2+2+2+1;
 const auto tail=wire.size()-(3871+2+entry+1+1);
 const auto objects=tail+3871+2+3+1+4+19*7;
 check(wire.at(tail)==30 && wire.at(tail+1)==0 && wire.at(tail+3871)==1 && wire.at(tail+3871+2+1)==23 &&
  wire.at(objects)==2 && wire.at(objects+2)==0 && wire.at(objects+4)==0x80 && wire.at(objects+5)==0xff && wire.at(objects+6)==1,
  "Literal v13 owner extent and map 23 object overrides (record, x -128, y -1, flip)");
 for(auto offset:{tail,tail+1,objects+6}) {
  auto bad=wire;bad[offset]=255;fixIndependentEnvelope(bad);rejects([&]{XeenSaveFormat::decode(bad);});
 }
}

void skillExplorationWire() {
 auto s=currentWireSnapshot();
 for(unsigned owner=0;owner<30;++owner)for(unsigned skill=0;skill<18;++skill)
  s.characters[owner].skills[skill]=static_cast<std::uint8_t>(owner*18+skill);
 s.map(23).explored.emplace();s.map(28).explored.emplace();(*s.map(23).explored)[0]=1;(*s.map(28).explored)[31]=128;
 roundTrip(s);
 for(unsigned value:{254u,255u,0u}) {s.characters[29].skills[4]=value;roundTrip(s);}
 s.characters[29].skills[4]=254;
 for(unsigned expected:{255u,0u}) {
  check(xeenGrantSkill(s.characters[29],4),"grant failed");
  check(s.characters[29].skills[4]==expected,"live wrapping skill");roundTrip(s);
 }
 auto maximum=s;save_test::clearExplored(maximum);
 for(unsigned tile=1;tile<=512;++tile)maximum.map(static_cast<std::uint16_t>(tile)).explored.emplace();
 roundTrip(maximum);
 auto darkside=s;darkside.map({XeenSide::Darkside,23}).explored.emplace();roundTrip(darkside); // Representable, not loadable.
 for(unsigned bad=0;bad<5;++bad) {
  auto invalid=s;
  switch(bad) {
  case 0:invalid.maps[0].id=0;break;
  case 1:invalid.maps[0].id.side=static_cast<XeenSide>(2);break;
  case 2:invalid.maps[1].id=23;break;
  case 3:std::swap(invalid.maps[0],invalid.maps[1]);break;
  case 4:invalid=maximum;invalid.map(513).explored.emplace();break;
  }
  rejects([&]{XeenSaveFormat::encode(invalid);});
 }
 // From the end: absent name 1 and head cache 1, then map 28's entry (side,
 // number, no monsters, four empty override counts, explored flag and 32
 // bits); map 23's explored bits end just before it.
 auto wire=XeenSaveFormat::encode(s);const auto at=wire.size()-(1+1+3+1+8+1+32);
 check(wire[at]==0 && wire[at+1]==28 && wire[at+2]==0 && wire[at+3]==0 && wire[at+12]==1 && wire[at+13+31]==128 &&
  wire[at-33]==1 && wire[at-32]==1,"literal physical explored tile wire layout");
 for(unsigned field:{0u,2u,3u,12u}) {
  auto corrupt=wire;corrupt[at+field]=255;fixIndependentEnvelope(corrupt);rejects([&]{XeenSaveFormat::decode(corrupt);});
 }
 auto duplicate=wire;duplicate[at+1]=23;fixIndependentEnvelope(duplicate);rejects([&]{XeenSaveFormat::decode(duplicate);},"strictly ordered");
}

void dailyStateRoundTrips() {
 auto s=currentWireSnapshot();s.food=90;
 auto &time=*s.journey->context;time.year=611;time.day=0;time.minutes=299;
 time.newDay=true;time.rested=true;time.effects[0]=9;time.lightAndResistances[1]=23;
 auto &r=*s.journey->supplements[29].inputs.resistances;
 r.firePermanent=17;r.fireTemporary=255;r.energyPermanent=128;r.energyTemporary=254;
 r.magicPermanent=127;r.magicTemporary=1;
 s.characters[29].temporaryAge=31;
 for(unsigned value=0;value<256;++value) {
  for(auto &counter:s.characters[29].conditions)counter=value;
  roundTrip(s);
 }
 for(unsigned food:{0u,1u,90u,65535u}){s.food=food;roundTrip(s);}
}

void saveNames() {
 auto s=currentWireSnapshot();roundTrip(s);
 for(unsigned c=0;c<256;++c) {
  s.name=std::string(1,char(c));
  if(c>=0x21 && c<=0x7e)roundTrip(s);
  else rejects([&]{XeenSaveFormat::encode(s);},"name");
 }
 for(const auto &name:std::vector<std::string>{""," leading",std::string(20,' '),std::string(21,'a')}) {
  s.name=name;rejects([&]{XeenSaveFormat::encode(s);},"name");
 }
 s.name="Case + spaces ! ~  ";roundTrip(s);
 s.name=std::string(20,'a');auto wire=XeenSaveFormat::encode(s);roundTrip(s);
 for(unsigned c:{0u,31u,127u,128u,255u}) {
  auto bad=wire;bad.back()=c;fixIndependentEnvelope(bad);
  rejects([&]{XeenSaveFormat::decode(bad);},"name");
 }
 auto leading=wire;leading[leading.size()-20]=' ';fixIndependentEnvelope(leading);
 rejects([&]{XeenSaveFormat::decode(leading);},"name");
 std::fill(leading.end()-20,leading.end(),' ');fixIndependentEnvelope(leading);
 rejects([&]{XeenSaveFormat::decode(leading);},"name");
 auto bad=wire;bad[bad.size()-21]=21;fixIndependentEnvelope(bad);
 rejects([&]{XeenSaveFormat::decode(bad);},"name length");
 bad=wire;bad.resize(bad.size()-20);fixIndependentEnvelope(bad);
 rejects([&]{XeenSaveFormat::decode(bad);},"name length");
 wire.back()^=1;rejects([&]{XeenSaveFormat::decode(wire);},"checksum");
}

// An independent minimal v1 fixture: fixed offsets and a bitwise CRC oracle.
// No production encoder or field-writing helper constructs this payload.
Bytes legacyGolden() {
	Bytes bytes(4957, 0);
	const Bytes prefix{'M', 'M', 'M', 'S', 'A', 'V', 'E', 0, 1, 0, 0, 0, 0x49, 0x13, 0, 0};
	std::copy(prefix.begin(), prefix.end(), bytes.begin());
	for (int i = 0; i < 12; ++i) bytes[20 + i] = static_cast<std::uint8_t>(i + 1);
	bytes[46] = 23; bytes[49] = 15; bytes[50] = 3;
	bytes[52] = 30;
	for (unsigned i = 0; i < 30; ++i) bytes[53 + 149 * i] = static_cast<std::uint8_t>(i);
	std::uint32_t crc = 0xffffffffU;
	for (std::size_t i = 20; i < bytes.size(); ++i) {
		crc ^= bytes[i];
		for (int bit = 0; bit < 8; ++bit)
			crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320U : 0);
	}
	crc ^= 0xffffffffU;
	for (unsigned i = 0; i < 4; ++i) bytes[16 + i] = static_cast<std::uint8_t>(crc >> (8 * i));
	return bytes;
}

Bytes golden() { auto s=currentWireSnapshot(); s.resources.clouds={0x0807060504030201ULL,0x0c0b0a09U}; s.camera={23,0,15,XeenDirection::West}; return XeenSaveFormat::encode(s); }

void wireContract() {
	auto s=currentWireSnapshot();
	s.resources.clouds = {0x0807060504030201ULL, 0x0c0b0a09U};
	s.camera = {23, 0, 15, XeenDirection::West};
	const auto expected = legacyGolden();
	// The party prefix ends with the game flags at 7319; the v13 Journey
	// block follows directly (no top-level overlay lists).
	Bytes v2(7319, 0);
	std::copy_n(expected.begin(), 53, v2.begin());
	v2[8] = 13;
	for (unsigned i = 0; i < 30; ++i) {v2[53 + 228 * i] = i;v2[53+228*i+227]=255;}
	const auto current=XeenSaveFormat::encode(s);
	check(current[7319]==3,"Journey domain byte follows the game flags");
	v2.insert(v2.end(),current.begin()+7319,current.end());
	fixIndependentEnvelope(v2);
	check(XeenSaveFormat::encode(s) == v2, "encoder differs from independent minimal v2");
	sameSnapshot(s, XeenSaveFormat::decode(v2));
	auto named = s;
	named.characters[0].name = "X";
	named.characters[0].intellect = {-1, std::numeric_limits<int>::min()};
	named.characters[0].currentHp = -32768;
	named.characters[0].currentSp = 32767;
	named.characters[0].conditions[15] = 255;
	named.characters[0].birthYear = 65535;
	const auto bytes = XeenSaveFormat::encode(named);
	check(bytes[54] == 1 && bytes[55] == 'X' && bytes[59] == 255 && bytes[62] == 255 &&
		bytes[63] == 0 && bytes[66] == 128, "name/signed attribute byte layout");
	check(bytes[258] == 0 && bytes[259] == 128 && bytes[260] == 255 && bytes[261] == 127 &&
		bytes[277] == 255 && bytes[278] == 255 && bytes[279] == 255, "v2 HP/SP/condition/year byte layout");
}

void asymmetricBase() {
	// Independent offsets: three members, 14 name bytes, 228 fixed bytes/character.
	Bytes bytes(7319 + 3 + 14, 0);
	const Bytes prefix{'M','M','M','S','A','V','E',0,13,0,0,0};
	std::copy(prefix.begin(), prefix.end(), bytes.begin());
	bytes[20] = 99; bytes[28] = 88; bytes[46] = 23;
	bytes[51] = 3; bytes[52] = 18; bytes[53] = 0; bytes[54] = 18; bytes[55] = 30;
	auto expected=currentWireSnapshot();
	expected.resources.clouds = {99, 88}; expected.camera = {23, 0, 0, XeenDirection::North};
	expected.activeRosterIds = {18, 0, 18};
	std::size_t base = 56;
	for (unsigned i = 0; i < 30; ++i) {
		auto &c = expected.characters[i];
		c.name = i == 0 ? "A" : i == 18 ? "Owner" : i == 29 ? "Inactive" : "";
		bytes[base] = i; bytes[base + 1] = c.name.size();
		std::copy(c.name.begin(), c.name.end(), bytes.begin() + base + 2);
		const auto itemsStart = base + 60 + c.name.size();
		unsigned category = 0;
		for (auto *items : {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous}) {
			for (unsigned slot = 0; slot < 9; ++slot) {
				const auto offset = itemsStart + category * 36 + slot * 4;
				const auto material = static_cast<std::uint8_t>(255 - i - category * 19 - slot);
				const auto id = static_cast<std::uint8_t>(slot % 3 == 1 ? 0 : 1 + i * 7 + category * 11 + slot);
				const auto state = static_cast<std::uint8_t>(i * 13 + category * 23 + slot);
				const auto frame = static_cast<std::uint8_t>(255 - i * 3 - category * 9 - slot);
				bytes[offset] = material; bytes[offset + 1] = id;
				bytes[offset + 2] = state; bytes[offset + 3] = frame;
				(*items)[slot] = {material, id, state, frame};
			}
			++category;
		}
		const auto after = itemsStart + 144;
		c.currentHp = -static_cast<std::int16_t>(300 + i); c.currentSp = 400 + i;
		bytes[after] = static_cast<std::uint8_t>(c.currentHp); bytes[after + 1] = 254;
		bytes[after + 2] = static_cast<std::uint8_t>(c.currentSp); bytes[after + 3] = 1;
		c.conditions[15] = 200 + i; bytes[after + 19] = c.conditions[15];
		c.birthYear = 592 + i; bytes[after + 20] = c.birthYear & 255; bytes[after + 21] = 2;
		bytes[after+22]=c.quickOption;bytes[after+23]=c.currentSpell;
		base += 228 + c.name.size();
	}
	expected.questItems[34] = 0x12345678; put32(bytes, base + 34 * 4, 0x12345678);
	expected.questFlags[29] = true; bytes[base + 169] = 1;
	expected.gameFlags[255] = true; bytes[base + 425] = 1;
	const auto current=XeenSaveFormat::encode(expected);
	bytes.insert(bytes.end(),current.begin()+bytes.size(),current.end());
	fixIndependentEnvelope(bytes);
	check(XeenSaveFormat::encode(expected) == bytes, "asymmetric independent v2 byte order");
	sameSnapshot(expected, XeenSaveFormat::decode(bytes));
	check(bytes == XeenSaveFormat::encode(XeenSaveFormat::decode(bytes)), "v2 deterministic oracle");
	// Each legal item byte can change without becoming structural corruption.
	for (unsigned field = 0; field < 144; ++field) {
		auto changed = bytes; changed[117 + field] ^= 255; fixIndependentEnvelope(changed);
		check(XeenSaveFormat::encode(XeenSaveFormat::decode(changed)) == changed, "legal item byte rejected or normalized");
	}
	for (std::size_t size = 20; size < bytes.size(); ++size) {
		Bytes truncated(bytes.begin(), bytes.begin() + size); fixIndependentEnvelope(truncated);
		rejects([&] { XeenSaveFormat::decode(truncated); });
	}
	for (const auto offset : {117U, 152U, 189U, 260U, 56U + 228U * 29U + 6U + 60U + 8U}) {
		auto removed = bytes; removed.erase(removed.begin() + offset); fixIndependentEnvelope(removed);
		rejects([&] { XeenSaveFormat::decode(removed); });
		auto inserted = bytes; inserted.insert(inserted.begin() + offset, 255); fixIndependentEnvelope(inserted);
		rejects([&] { XeenSaveFormat::decode(inserted); });
	}
}

void completeRoundTrips() {
	auto s = sample();
	for (const auto &ids : std::vector<std::vector<std::uint8_t>>{
			{}, {0}, {29}, {0, 18, 14, 11, 1, 6}, {18, 0, 18, 23, 1, 6}}) {
		s.activeRosterIds = ids;
		roundTrip(s);
	}
	s.resources.darkside.reset();
	roundTrip(s);
	const auto before = s;
	auto copy = XeenSaveFormat::decode(XeenSaveFormat::encode(s));
	copy.characters[0].name = "Changed";
	copy.questItems.fill(0); copy.questFlags.fill(false); copy.gameFlags.fill(false);
	copy.maps.clear(); copy.activeRosterIds.clear();
	sameSnapshot(s, before);
}

void numericDomains() {
	auto s = sample();
	for (const int value : {-32768, -1, 0, 32767}) {
		s.characters[0].currentHp = static_cast<std::int16_t>(value);
		s.characters[29].currentSp = static_cast<std::int16_t>(value);
		roundTrip(s);
	}
	for (std::size_t i = 0; i < 35; ++i)
		for (const std::uint32_t value : {0U, 1U, 255U, 256U, 0xffffffffU}) {
			s.questItems.fill(0); s.questItems[i] = value; roundTrip(s);
		}
	for (std::size_t i = 0; i < 16; ++i)
		for (const int value : {0, 1, 255}) {
			s.characters[0].conditions.fill(0);
			s.characters[0].conditions[i] = static_cast<std::uint8_t>(value);
			roundTrip(s);
		}
	for (std::size_t i = 0; i < 30; ++i) {
		s.questFlags.fill(false); s.questFlags[i] = true; roundTrip(s);
	}
	for (std::size_t i = 0; i < 256; ++i) {
		s.gameFlags.fill(false); s.gameFlags[i] = true; roundTrip(s);
	}
	auto &c = s.characters[0];
	for (auto *field : {&c.intellect.permanent, &c.intellect.temporary, &c.personality.permanent,
			&c.personality.temporary, &c.endurance.permanent, &c.endurance.temporary,
			&c.permanentLevel, &c.temporaryLevel, &c.temporaryAge})
		for (int value : {std::numeric_limits<int>::min(), -2, 0, 255, 256, 70000,
				std::numeric_limits<int>::max()}) {
			*field = value; roundTrip(s);
		}
	for (auto *items : {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous})
		for (auto &item : *items) {
			item = {0, 0, 255, 0}; roundTrip(s);
			item = {255, 255, 0, 255}; roundTrip(s);
		}
	c.birthYear = 0; roundTrip(s);
	c.birthYear = 65535; roundTrip(s);
}

void malformedBytes() {
	const auto good = golden();
	for (std::size_t size = 0; size < good.size(); ++size) {
		Bytes truncated(good.begin(), good.begin() + size);
		rejects([&] { XeenSaveFormat::decode(truncated); });
		// Reach inner field/count readers too, rather than only testing the envelope.
		if (size >= 20) {
			fixEnvelope(truncated);
			rejects([&] { XeenSaveFormat::decode(truncated); });
		}
	}
	auto bad = good;
	bad[0] ^= 1; rejects([&] { XeenSaveFormat::decode(bad); }, "unrecognized format");
	bad = good; bad[8] = 14; rejects([&] { XeenSaveFormat::decode(bad); }, "newer or unsupported");
	bad = good; bad[8] = 3; rejects([&] { XeenSaveFormat::decode(bad); }); // v1 payload is not v3.
	bad = good; bad[8] = 2; rejects([&] { XeenSaveFormat::decode(bad); }); // v1 payload is not v2.
	bad = good; bad[8] = 0; rejects([&] { XeenSaveFormat::decode(bad); }, "newer or unsupported");
	bad = good; bad[10] = 1; rejects([&] { XeenSaveFormat::decode(bad); }, "unsupported game side");
	bad = good; bad[11] = 1; rejects([&] { XeenSaveFormat::decode(bad); }, "reserved");
	bad = good; bad[16] ^= 1; rejects([&] { XeenSaveFormat::decode(bad); }, "checksum");
	bad = good; put32(bad, 12, 0xffffffffU); rejects([&] { XeenSaveFormat::decode(bad); }, "length");
	bad = good; bad.push_back(0); fixEnvelope(bad);
	rejects([&] { XeenSaveFormat::decode(bad); }, "suffix length");
	// The golden table: count 2 and map 23's entry (identity 3, monster flag 1,
	// counts 4, 19 slots of 7, four empty override counts and the explored
	// flag), then the head-cache flag and the name length.
	const auto table = good.size() - (2 + 3 + 1 + 4 + 19 * 7 + 4 * 2 + 1) - 1 - 1;
	rejects([&] { XeenSaveFormat::decode(Bytes(XeenSaveFormat::kMaximumSize + 1)); }, "oversized");
	for (const auto offset : {32U, 112U, 7033U, 7063U}) {
		bad = good; bad[offset] = 2; fixEnvelope(bad);
		rejects([&] { XeenSaveFormat::decode(bad); }, "boolean");
	}
	bad = good; bad[33] = 1; fixEnvelope(bad);
	rejects([&] { XeenSaveFormat::decode(bad); }, "absent archive");
	for (const auto offset : {table, table + 2 + 3 + 1 + 2}) { // Map count, then map 23's monster count.
		bad = good; bad[offset] = 0xff; bad[offset + 1] = 0xff; fixEnvelope(bad);
		rejects([&] { XeenSaveFormat::decode(bad); }, "count");
	}
	bad = good; bad[table + 2 + 3] = 2; fixEnvelope(bad); rejects([&] { XeenSaveFormat::decode(bad); }, "boolean");
	for (const auto offset : {45U, 47U, 48U, 49U, 50U, 51U, 52U, 53U, 54U}) {
		bad = good; bad[offset] = 255; fixEnvelope(bad);
		rejects([&] { XeenSaveFormat::decode(bad); });
	}
	auto s = sample();
	bad = XeenSaveFormat::encode(s); bad[52] = 30; fixEnvelope(bad);
	rejects([&] { XeenSaveFormat::decode(bad); }, "active roster");
	// Six membership bytes move the first record to 59; name starts at 61.
	bad = XeenSaveFormat::encode(s); bad[61] = 0; fixEnvelope(bad);
	rejects([&] { XeenSaveFormat::decode(bad); }, "character name");
	// Map identities end the payload strictly ordered: sample() holds maps 1
	// (one removed object, 18 bytes), 2 (a removed object and an Event, 21),
	// 3 (an Event, 16), 23 (19 monsters, 150) and 9999 (21). A duplicate or
	// descending identity is rejected.
	const auto second = XeenSaveFormat::encode(s).size() - 1 - 1 - (18 + 21 + 16 + 150 + 21) + 18;
	for (const auto entry : {second, second + 21}) { // Map 2 duplicates map 1; map 3 descends to 1.
		bad = XeenSaveFormat::encode(s);
		check(bad[second] == 0 && bad[second + 1] == 2 && bad[second + 21 + 1] == 3, "literal map identities");
		bad[entry + 1] = 1;
		fixEnvelope(bad); rejects([&] { XeenSaveFormat::decode(bad); }, "ordered");
	}
}

void invalidValuesAndLimits() {
	const auto good = sample();
	for (int mode = 0; mode < 15; ++mode) {
		auto s = good;
		switch (mode) {
		case 0: s.camera.x = -1; break;
		case 1: s.camera.y = 32; break; // Map-agnostic: the indoor capacity bounds the camera.
		case 2: s.camera.direction = static_cast<XeenDirection>(4); break;
		case 3: s.camera.mapId = 0; break;
		case 4: s.camera.mapId = 10000; break;
		case 5: s.camera.mapId.side = static_cast<XeenSide>(2); break;
		case 6: s.activeRosterIds.push_back(0); break;
		case 7: s.activeRosterIds[0] = 30; break;
		case 8: s.characters[29].rosterId = 0; break;
		case 9: s.characters[0].name = std::string(17, 'a'); break;
		case 10: s.characters[0].name = std::string("a\0b", 3); break;
		case 11: s.map(1).objects.push_back(s.map(1).objects[0]); break;
		case 12: std::swap(s.maps[0], s.maps[1]); break;
		case 13: s.maps[0].id.side = static_cast<XeenSide>(255); break;
		case 14: s.map(5); break; // An empty entry is not canonical.
		}
		rejects([&] { XeenSaveFormat::encode(s); });
	}
	// Darkside identities are representable; restore rejects them (decision 6).
	auto darkside = good; darkside.camera.mapId.side = XeenSide::Darkside; darkside.headCache.reset(); roundTrip(darkside);
	auto maximum = good;
	maximum.maps.clear();
	auto &first = maximum.map(1);
	for (std::size_t i = 0; i < XeenSaveFormat::kMaximumOverrides; ++i) {
		first.objects.push_back({static_cast<std::uint16_t>(i), -128, -128, false});
		first.events.push_back({static_cast<std::uint16_t>(i), static_cast<std::uint8_t>(i)});
	}
	for (unsigned number = 2; number <= XeenSaveFormat::kMaximumMaps; ++number)
		maximum.map(static_cast<std::uint16_t>(number)).explored.emplace();
	maximum.map(23).monsters = monsterTable(XeenSaveFormat::kMaximumMonsters);
	roundTrip(maximum);
	auto tooMany = maximum; tooMany.map(1).objects.push_back({65535, -128, -128, false});
	rejects([&] { XeenSaveFormat::encode(tooMany); }, "object override");
	tooMany = maximum; tooMany.map(600).explored.emplace();
	rejects([&] { XeenSaveFormat::encode(tooMany); }, "too many map states");
	tooMany = maximum; tooMany.map(23).monsters->slots.emplace_back();
	rejects([&] { XeenSaveFormat::encode(tooMany); }, "monster table counts");
	tooMany = maximum; for (unsigned cell = 0; cell < 257; ++cell) tooMany.map(2).cells.push_back({std::uint8_t(cell), 1, 0, 0, 0});
	rejects([&] { XeenSaveFormat::encode(tooMany); });
}

class ObservedStream : public std::stringbuf {
public:
	explicit ObservedStream(const std::string &data, bool fail = false) : std::stringbuf(data), _fail(fail) {}
	unsigned calls = 0;
	std::streamsize maximumRequest = 0;
protected:
	std::streamsize xsgetn(char *data, std::streamsize count) override {
		++calls; maximumRequest = std::max(maximumRequest, count);
		if (_fail && calls == 2) throw std::runtime_error("injected archive read error");
		return std::stringbuf::xsgetn(data, count);
	}
private:
	bool _fail;
};

void fingerprints() {
	std::istringstream known("123456789");
	check(XeenSaveFormat::fingerprint(known) == XeenArchiveFingerprint{9, 0xcbf43926U}, "CRC32 standard vector");
	std::istringstream empty;
	check(XeenSaveFormat::fingerprint(empty) == XeenArchiveFingerprint{}, "empty stream fingerprint");
	ObservedStream buffer(std::string(200000, 'a'));
	std::istream stream(&buffer);
	const auto signature = XeenSaveFormat::fingerprint(stream);
	check(signature.size == 200000 && buffer.calls == 4 && buffer.maximumRequest == 65536,
		"archive hashing is not bounded/chunked");
	ObservedStream failing(std::string(200000, 'a'), true);
	std::istream broken(&failing);
	rejects([&] { XeenSaveFormat::fingerprint(broken); });
	std::istringstream bad("x"); bad.setstate(std::ios::failbit);
	rejects([&] { XeenSaveFormat::fingerprint(bad); }, "readable");
	std::istringstream throwing("123456789"); throwing.exceptions(std::ios::badbit | std::ios::failbit);
	check(XeenSaveFormat::fingerprint(throwing) == XeenArchiveFingerprint{9, 0xcbf43926U}, "EOF exception handling");
}

// Synthetic wire-only owners: no original resources or gameplay injection.

// Bytes of the minimal Journey suffix of currentWireSnapshot(), from the domain
// byte to the end: the fixed v13 Journey block (6 domain/schema/content/context
// marker, 36 context and food, 1230 supplements, 13 RNG, 331 resistances, 20
// treasure counts, 128 world flags, 1201 learned spells, 91 poison, 1164
// economy, 3871 awards), the per-map table (count 2 and map 23's 150-byte
// entry with 19 slots), the head-cache flag and the name length.
constexpr std::size_t kMinimalSuffix = 8091 + 2 + 150 + 1 + 1;
// The economy block (shape 4, wares 1152, bank 8) to the end of the minimal save.
constexpr std::size_t kMinimalEconomyTail = 1164 + 3871 + 2 + 150 + 1 + 1;
// A map entry is 13 bytes without a monster table; a table adds 4 + 7 per slot.
constexpr std::size_t mapEntryBytes(std::size_t monsters) { return 13 + 4 + 7 * monsters; }

XeenSaveMonsterTable cityTable(unsigned count) {
	auto table = monsterTable(count);table.originalCount = 46;
	for (unsigned i = 0; i < count; ++i) { auto &a = table.slots[i]; a.x = int(i % 32); a.y = int(i / 32); if (i >= 46) a.spawnedType = 0; }
	return table;
}

void rejectedVersions() {
 // v12 and every earlier envelope are recognizable older formats: no legacy
 // reader, the existing clear rejection.
 for(unsigned version:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u,12u,14u,65535u}) {
  auto bytes=golden();bytes[8]=version;bytes[9]=version>>8;
  // Only recognizable earlier envelopes are "older"; others may be newer builds.
  rejects([&]{XeenSaveFormat::decode(bytes);},version>=1 && version<=12?"no longer supported":"newer or unsupported");
 }
 {
  auto v12=golden();v12[8]=12;fixIndependentEnvelope(v12);
  try{XeenSaveFormat::decode(v12);throw std::runtime_error("v12 save accepted");}
  catch(const XeenUnsupportedSave &e){check(e.recognizableOlder,"v12 save must be a recognizable older save");}
 }
 rejects([&]{XeenSaveFormat::decode(legacyGolden());},"no longer supported");
 rejects([&]{XeenSaveFormat::encode(XeenSaveSnapshot{});},"no longer supported");
 const auto good=currentWireSnapshot();const auto wire=XeenSaveFormat::encode(good);const auto start=wire.size()-kMinimalSuffix;
 check(wire[start]==3,"literal minimal v13 Journey suffix");
 for(unsigned schema=0;schema<=11;++schema)for(unsigned content=0;content<=15;++content) {
  if(schema==9 && content==14)continue;
  auto s=good;s.journey->schema=schema;s.journey->content=content;
  const char *message=((schema>=1 && schema<=8 && schema==content) || (schema==8 && (content==9 || content==10)) || (schema==9 && content>=11 && content<=13))?"no longer supported":"newer or unsupported";
  rejects([&]{XeenSaveFormat::validate(s);},message);
  rejects([&]{XeenSaveFormat::encode(s);},message);
  auto bytes=wire;bytes[start+1]=schema;bytes[start+3]=content;fixIndependentEnvelope(bytes);
  rejects([&]{XeenSaveFormat::decode(bytes);},message);
 }
}
void regionalCityWire() {
 auto city=currentWireSnapshot();city.map(28).monsters=cityTable(46);
 for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x) {
  city.camera={28,static_cast<int>(x),static_cast<int>(y),XeenDirection::North};
  roundTrip(city);
 }
 city.camera={28,8,4,XeenDirection::West};
 for(unsigned mode=0;mode<7;++mode) {
  auto invalid=city;
  switch(mode){case 0:invalid.journey->context->year=611;break;case 1:invalid.journey->context->minutes=299;break;
   case 2:invalid.journey->context->minutes=1260;break;case 3:invalid.journey->context->effects[0]=1;break;
   case 4:invalid.journey->context->rested=true;break;case 5:invalid.journey->context->newDay=true;break;
   case 6:invalid.journey->context->difficulty=XeenDifficulty::Warrior;break;}
  roundTrip(invalid);
 }
 auto invalidDifficulty=city;invalidDifficulty.journey->context->difficulty=static_cast<XeenDifficulty>(2);
 rejects([&]{XeenSaveFormat::encode(invalidDifficulty);},"context enum");
 for(unsigned i=46;i<52;++i){XeenSaveMonster a;
  if(i<50)a.lifecycle=XeenActorLifecycle::Unresolved;
  else a.spawnedType=0;
  city.map(28).monsters->slots.push_back(a);}
 roundTrip(city);
 auto corruptType=city;corruptType.map(28).monsters->slots.at(50).spawnedType=-1;
 rejects([&]{XeenSaveFormat::encode(corruptType);},"script-slot MON type");
 corruptType=city;corruptType.map(28).monsters->slots.at(46).spawnedType=0;
 rejects([&]{XeenSaveFormat::encode(corruptType);},"script-slot MON type");
 corruptType=city;corruptType.map(28).monsters->slots.at(3).spawnedType=0;
 rejects([&]{XeenSaveFormat::encode(corruptType);},"bind to MOB");
 city.map(28).events.push_back({764,0});roundTrip(city);
}

void mainlandTopologyCounts() {
	for(unsigned count:{1u,18u,20u,107u}) {
		auto save=currentWireSnapshot();auto &table=*save.map(23).monsters;table=monsterTable(count);
		for(unsigned i=0;i<count;++i){table.slots[i].x=int(i%16);table.slots[i].y=int(i/16);}
		roundTrip(save);
		auto wrong=save;wrong.map(23).monsters->originalCount=count+1;rejects([&]{XeenSaveFormat::encode(wrong);},"monster table counts");
		wrong=save;wrong.map(23).monsters->slots.resize(108);rejects([&]{XeenSaveFormat::encode(wrong);},"monster table counts");
	}
}

void serviceEconomyWireContract() {
	// M71 mutations: reject old charges 1/2, truncate new 9/10, regenerate
	// restored stock, replay pending rewards, or move the saved RNG cursor.
	// Both legacy and corrected records use the same v13/schema9/content14 wire.
	for(unsigned charges:{1u,2u,9u,10u})for(bool pending:{false,true}) {
		auto state=currentWireSnapshot();
		state.journey->serviceEconomy->wares[0][0][3][0]={1,1,std::uint8_t(charges),0};
		auto &actor=state.map(23).monsters->slots[0];
		actor.accounted=true;actor.lifecycle=XeenActorLifecycle::Defeated;actor.x=actor.y=-128;
		auto &loot=*state.journey->treasure;
		loot.sources[0]={23,0};loot.miscellaneous[0]={0,{9,15,std::uint8_t(charges),0}};
		if(pending) {loot.pendingMask=1;loot.pendingGold=10;}
		roundTrip(state);
		const auto wire=XeenSaveFormat::encode(state);
		// Literal v13 positions: stock's first Misc state and treasure's sole
		// Misc state after its five-byte source identity and category counts.
		const auto stockCharge=wire.size()-kMinimalEconomyTail+4+3*9*4+2;
		const auto lootCharge=journeyOffset(state)+6+36+1230+13+331+16+1+5+2+1+3;
		check(wire.at(stockCharge)==charges && wire.at(lootCharge)==charges,"v13 charge wire positions differ");
		for(unsigned invalid:{0u,11u,64u,128u,255u}) {
			auto bad=state;bad.journey->serviceEconomy->wares[0][0][3][0].state=invalid;
			rejects([&]{XeenSaveFormat::encode(bad);});
			bad=state;bad.journey->treasure->miscellaneous[0].item.state=invalid;
			rejects([&]{XeenSaveFormat::encode(bad);});
			for(auto offset:{stockCharge,lootCharge}) {
				auto corrupt=wire;corrupt.at(offset)=invalid;fixIndependentEnvelope(corrupt);
				rejects([&]{XeenSaveFormat::decode(corrupt);});
			}
		}
	}
	for(unsigned content:{14u}) {
	auto s=currentWireSnapshot();
	for(unsigned owner=0;owner<30;++owner) {
		(*s.characters[owner].learnedSpells)[owner%39]=255-owner;
		s.journey->supplements[owner].inputs.poisonResistance=XeenAttributeValue{int(owner),int(255-owner)};
	}
	const auto bytes=XeenSaveFormat::encode(s);const auto start=bytes.size()-kMinimalSuffix;
	check(bytes[8]==13 && bytes[start+1]==9 && bytes[start+3]==content,"exact v13/schema9/content selectors");
	auto expected=bytes;expected.resize(expected.size()-kMinimalEconomyTail);
	expected.insert(expected.end(),{2,4,4,9});
	// Literal wire recipe is independent of the production encoder and validator.
	const std::array<std::array<std::uint8_t,8>,8> materials{{
		{{0,0,0,0,0,0,0,0}},{{0,0,0,0,0,37,37,37}},
		{{0,0,0,0,0,37,37,37}},{{0,0,0,0,0,37,37,37}},
		{{0,0,0,0,0,0,0,0}},{{0,0,0,0,0,37,37,37}},
		{{40,40,40,40,40,41,41,41}},{{39,39,39,39,39,40,40,40}}
	}};
	for(unsigned shop=0;shop<8;++shop) {
		for(unsigned slot=0;slot<8;++slot)expected.insert(expected.end(),{materials[shop][slot],1,0,0});
		expected.insert(expected.end(),112,0); // Zero ninth Weapon and three complete categories.
	}
	expected.insert(expected.end(),{0x98,0xba,0xdc,0xfe,0xff,0xff,0xff,0xff});
	expected.push_back(30);
	for(unsigned owner=0;owner<30;++owner) {expected.push_back(owner);expected.insert(expected.end(),128,0);}
	// v13 per-map table: one entry, map 23 (Clouds side 0), with its monster
	// table of 19 MOB-bound slots (x 0, y 0, Present 0, Physical 0, not
	// accounted, type -1) and no overrides or explored bits.
	expected.insert(expected.end(),{1,0, 0,23,0, 1, 19,0, 19,0});
	for(unsigned slot=0;slot<19;++slot)expected.insert(expected.end(),{0,0,0,0,0,0xff,0xff});
	expected.insert(expected.end(),{0,0, 0,0, 0,0, 0,0, 0});
	expected.push_back(0); // v11 absent head cache.
	expected.push_back(0); // v7 absent-name metadata ends the payload.
	fixIndependentEnvelope(expected);
	check(expected==bytes,"M40 exact inherited suffix then shape/stock/bank/award/per-map wire");
	const auto decoded=XeenSaveFormat::decode(expected);sameSnapshot(s,decoded);
	check(XeenSaveFormat::encode(decoded)==expected,"M40 exact schema9 round trip");
	const auto economyOffset=bytes.size()-kMinimalEconomyTail;
	for(unsigned shape=0;shape<4;++shape)for(unsigned value:{0u,1u,3u,5u,8u,10u,255u}) {
		if(value==bytes[economyOffset+shape])continue;
		auto bad=bytes;bad[economyOffset+shape]=value;fixIndependentEnvelope(bad);
		rejects([&]{XeenSaveFormat::decode(bad);},"stock shape");
	}
	for(unsigned mutation=0;mutation<10;++mutation) {
		auto bad=bytes;const auto item=economyOffset+4;
		switch(mutation) {
		case 0:bad[item]=37;break; // First level-1 call cannot enchant.
		case 1:bad[item+1]=34;break; // Unsupported Weapon ID.
		case 2:bad[item+2]=7;break; // Effectiveness outside 0..6.
		case 3:bad[item+2]=64;break;
		case 4:bad[item+2]=128;break;
		case 5:bad[item+3]=1;break; // Merchant frames are zero.
		case 6:bad[item+4+1]=0;break; // Hole before occupied records.
		case 7:bad[item+8*4+1]=1;break; // Forbidden ninth slot.
		case 8:bad[item+36+0]=1;break; // Empty ID-zero metadata must be zero.
		case 9:std::fill(bad.begin()+item,bad.begin()+item+1152,0);break; // Cannot omit twenty calls.
		}
		fixIndependentEnvelope(bad);rejects([&]{XeenSaveFormat::decode(bad);});
	}
	for(std::size_t cut=1;cut<kMinimalEconomyTail;++cut) {
		auto bad=bytes;bad.resize(bad.size()-cut);fixIndependentEnvelope(bad);
		rejects([&]{XeenSaveFormat::decode(bad);});
	}
	auto bad=bytes;bad.push_back(0);fixIndependentEnvelope(bad);rejects([&]{XeenSaveFormat::decode(bad);});
	bad=bytes;bad[economyOffset+4]^=1;rejects([&]{XeenSaveFormat::decode(bad);},"checksum");
	auto missing=s;missing.journey->serviceEconomy.reset();rejects([&]{XeenSaveFormat::encode(missing);},"economy presence");

	for(unsigned cityCount:{0u,46u,52u})for(unsigned pending=0;pending<=12;++pending) {
		auto state=s;
		if(cityCount) {
			state.map(28).monsters=cityTable(cityCount);
			state.camera={28,8,4,XeenDirection::West};
			if(cityCount==52)state.map(28).events.push_back({764,0});
		}
		for(unsigned source=0;source<pending;++source) {
			auto &actor=state.map(23).monsters->slots[source];actor.accounted=true;actor.lifecycle=XeenActorLifecycle::Defeated;actor.x=actor.y=-128;
			auto &entry=source<10?state.journey->treasure->weapons[source]:state.journey->treasure->armor[source-10];entry.source=source;entry.item.id=1;
			state.journey->treasure->sources[source]={23,source};
		}
		const auto wire=XeenSaveFormat::encode(state);
		// Each pending source adds its five-byte identity and its five-byte item.
		check(wire.size()==start+kMinimalSuffix+(cityCount?mapEntryBytes(cityCount):0)+10*pending+(cityCount==52?3:0),"v13 exact city/pending suffix extent");
		sameSnapshot(state,XeenSaveFormat::decode(wire));
	}
	for(unsigned day=8;day<=99;++day) {
		auto state=s;state.journey->context->day=day;
		if(day>8)state.map(28).monsters=cityTable(46);
		const auto wire=XeenSaveFormat::encode(state);sameSnapshot(state,XeenSaveFormat::decode(wire));
	}
	for(unsigned day:{100u,65535u}) {auto state=s;state.journey->context->day=day;rejects([&]{XeenSaveFormat::encode(state);},"calendar");}
	for(unsigned day:{9u,11u,99u}) {auto state=s;state.journey->context->day=day;roundTrip(state);}
	}
}
void purchaseDepletedWireContract() {
	auto state=currentWireSnapshot();state.camera={28,8,4,XeenDirection::West};state.map(28).monsters=cityTable(46);
	const auto complete=XeenSaveFormat::encode(state);
	// The city entry follows map 23's at the end of the payload.
	const auto economyOffset=complete.size()-(kMinimalEconomyTail+mapEntryBytes(46));
	const auto start=complete.size()-(kMinimalSuffix+mapEntryBytes(46));
	check(complete[8]==13 && complete[start+1]==9 && complete[start+3]==14,"purchase selector/extent differs");
	// Literal eight L1 Weapon source from twenty Weapon calls. Removing one
	// inserted plain record leaves seven; old generated-only meaning rejects it.
	state.journey->serviceEconomy->wares[0][0][0][7]={};
	roundTrip(state); // Depletion remains valid on any canonical day.
	state.journey->context->day=9;
	const auto depleted=XeenSaveFormat::encode(state);
	auto expected=complete;
	// Journey context starts after six domain/schema/content/entry bytes,
	// then profile/difficulty/ctr24 and the little-endian day.
	// Obtain no offsets from the production encoder's internals.
	const auto dayOffset=start+10;
	check(expected[dayOffset]==8 && expected[dayOffset+1]==0,"literal schema9 day offset differs");
	expected[dayOffset]=9;
	std::fill(expected.begin()+economyOffset+4+7*4,expected.begin()+economyOffset+4+8*4,0);
	fixIndependentEnvelope(expected);
	check(expected==depleted,"purchase retained schema9 bytes differ from literal deletion/day recipe");
	sameSnapshot(state,XeenSaveFormat::decode(expected));
	for(unsigned day:{9u,10u,11u,98u,99u}) {
		state.journey->context->day=day;XeenSaveFormat::validate(state);
		const auto bytes=XeenSaveFormat::encode(state);sameSnapshot(state,XeenSaveFormat::decode(bytes));
	}
	for(unsigned legacy:{11u,12u}) {
		auto rejected=state;rejected.journey->content=legacy;
		rejects([&]{XeenSaveFormat::encode(rejected);});
		auto bytes=depleted;bytes[start+3]=legacy;fixIndependentEnvelope(bytes);
		rejects([&]{XeenSaveFormat::decode(bytes);});
	}
	auto day8=depleted;day8[dayOffset]=8;fixIndependentEnvelope(day8);
	check(XeenSaveFormat::encode(XeenSaveFormat::decode(day8))==day8,"Day-8 depleted stock round-trip");
	for(unsigned category:{2u,3u})for(unsigned slot=0;slot<9;++slot) {
		auto bad=depleted;bad[economyOffset+4+category*36+slot*4+1]=255;fixIndependentEnvelope(bad);
		rejects([&]{XeenSaveFormat::decode(bad);});
	}
}

void templeWireContract() {
	auto state=currentWireSnapshot();
	state.map(28).monsters=cityTable(46);
	for(unsigned y=8;y<=28;++y) {
		state.camera={28,15,static_cast<int>(y),XeenDirection::North};
		const auto bytes=XeenSaveFormat::encode(state);
		const auto decoded=XeenSaveFormat::decode(bytes);
		check(decoded.journey->schema==9 && decoded.journey->content==14 &&
			decoded.camera.mapId==state.camera.mapId && decoded.camera.x==state.camera.x &&
			decoded.camera.y==state.camera.y && decoded.camera.direction==state.camera.direction &&
			XeenSaveFormat::encode(decoded)==bytes,
			"M43 Temple corridor did not round-trip as schema 9/content 14");
		auto legacy=state;legacy.journey->content=13;
		rejects([&]{XeenSaveFormat::encode(legacy);});
	}
}

// Every v13 per-map record kind, written byte by byte independently of the
// encoder. Map 28 holds a monster table with a MOB record and a script slot,
// a moved and a removed object, a wall item, a cell, a MakeNothingHere and a
// replaced Event record and explored bits; map 109 (a neighbour tile) holds a
// cell and explored bits only.
void perMapWire() {
	auto s=currentWireSnapshot();s.camera={28,8,4,XeenDirection::West};
	auto &city=s.map(28);
	city.monsters=XeenSaveMonsterTable{1,{{3,-128,XeenActorLifecycle::Disabled,XeenActorStatus::Physical,false,-1},
		{31,30,XeenActorLifecycle::Present,XeenActorStatus::Physical,false,0}}};
	city.objects={{2,20,-1,true},{300,-128,-128,false}};
	city.wallItems={{1,-5,7}};
	city.cells={{255,0x1999,0x80,0x9999,0x00}};
	city.events={{0,0},{764,0x2a}};
	city.explored=std::array<std::uint8_t,32>{};(*city.explored)[0]=0x81;(*city.explored)[31]=0x7f;
	auto &tile=s.map(109);
	tile.cells={{0,0x9993,0x08,0x9999,0x08}};
	tile.explored=std::array<std::uint8_t,32>{};(*tile.explored)[5]=4;
	roundTrip(s);
	const auto wire=XeenSaveFormat::encode(s);
	Bytes table{3,0}; // Three entries: 23, 28 and 109.
	table.insert(table.end(),{0,23,0, 1, 19,0, 19,0});
	for(unsigned slot=0;slot<19;++slot)table.insert(table.end(),{0,0,0,0,0,0xff,0xff});
	table.insert(table.end(),{0,0, 0,0, 0,0, 0,0, 0});
	table.insert(table.end(),{0,28,0, 1, 1,0, 2,0});              // Identity, table present, original count 1 of 2.
	table.insert(table.end(),{3,0x80, 1, 0, 0, 0xff,0xff});       // Record 0: x 3, y -128, Disabled, Physical, MOB-bound.
	table.insert(table.end(),{31,30, 0, 0, 0, 0,0});              // Slot 1: (31,30), Present, spawned MON type 0.
	table.insert(table.end(),{2,0, 2,0, 20,0xff,1, 0x2c,0x01,0x80,0x80,0}); // Objects: #2 at (20,-1) flipped; #300 removed.
	table.insert(table.end(),{1,0, 1,0, 0xfb,7});                 // Wall item #1 at (-5,7).
	table.insert(table.end(),{1,0, 255, 0x99,0x19, 0x80, 0x99,0x99, 0x00}); // Cell 255: word/attributes, then preimage.
	table.insert(table.end(),{2,0, 0,0,0, 0xfc,0x02,0x2a});       // Events: #0 opcode 0; #764 opcode 0x2a.
	table.push_back(1);table.push_back(0x81);table.insert(table.end(),30,0);table.push_back(0x7f);
	table.insert(table.end(),{0,109,0, 0, 0,0, 0,0, 1,0, 0, 0x93,0x99, 0x08, 0x99,0x99, 0x08, 0,0, 1});
	for(unsigned byte=0;byte<32;++byte)table.push_back(byte==5?4:0);
	table.insert(table.end(),{0,0}); // Absent head cache and name.
	check(wire.size()>table.size() && std::equal(table.begin(),table.end(),wire.end()-table.size()),
		"literal v13 per-map table: identity order and every record kind");
	const auto at=wire.size()-table.size();
	const auto city0=at+2+mapEntryBytes(19);
	// Each corrupted field is rejected. Offsets within map 28's entry: identity
	// 0..2, monster flag 3, counts 4..7, slot 0 at 8 (x, y, lifecycle 10,
	// status 11, accounted 12, type 13..14), slot 1 at 15, objects at 22 (the
	// removed #300's flip at 33), wall items at 34, cells at 40, Events at 49
	// (record #0 at 51..52), explored flag 57.
	const std::vector<std::pair<std::size_t,std::uint8_t>> corruptions{
		{at,4},{city0+1,23},{city0+3,2},{city0+10,4},{city0+11,2},{city0+13,0},
		{city0+33,1},{city0+41,1},{city0+52,5},{city0+57,2}};
	for(const auto &[offset,value]:corruptions) {
		auto bad=wire;bad.at(offset)=value;fixIndependentEnvelope(bad);rejects([&]{XeenSaveFormat::decode(bad);});
	}
	// A Darkside entry is representable at the format level (decision 6).
	auto darkside=s;darkside.map({XeenSide::Darkside,28}).explored.emplace();roundTrip(darkside);
	// The existing signed-byte fields also carry original Spawn parking slots.
	auto parked=s;parked.map(28).monsters->slots[1].x=100;parked.map(28).monsters->slots[1].y=100;roundTrip(parked);
}

} // namespace

int main() {
	try {

		const auto run=[](const char *name,auto test){try{test();}catch(const std::exception &e){throw std::runtime_error(std::string(name)+": "+e.what());}};
		run("saveNames",saveNames);
		run("worldEffectsWire",worldEffectsWire);
		run("skillExplorationWire",skillExplorationWire);
		run("dailyStateRoundTrips",dailyStateRoundTrips);
		run("wireContract",wireContract);
		run("asymmetricBase",asymmetricBase);
		run("completeRoundTrips",completeRoundTrips);
		run("numericDomains",numericDomains);
		run("malformedBytes",malformedBytes);
		run("invalidValuesAndLimits",invalidValuesAndLimits);
		run("fingerprints",fingerprints);
		run("rejectedVersions",rejectedVersions);
		run("regionalCityWire",regionalCityWire);
		run("mainlandTopologyCounts",mainlandTopologyCounts);
		run("serviceEconomyWireContract",serviceEconomyWireContract);
		run("purchaseDepletedWireContract",purchaseDepletedWireContract);
		run("templeWireContract",templeWireContract);
		run("perMapWire",perMapWire);

		std::cout << "Current save format: wire content, all modeled values, domains, malformed input and fingerprints passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n'; return 1;
	}
}
