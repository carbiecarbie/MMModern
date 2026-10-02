#include "XeenSaveTestSupport.h"

#include <algorithm>
#include <iostream>
#include <sstream>

using namespace mmodern;
using namespace save_test;

namespace {

void roundTrip(const XeenSaveSnapshot &s) {
	const auto bytes = XeenSaveFormat::encode(s);
	const auto decoded = XeenSaveFormat::decode(bytes);
	sameSnapshot(s, decoded);
	check(bytes == XeenSaveFormat::encode(decoded), "encoding is not deterministic");
}

// An independent minimal v1 fixture: fixed offsets and a bitwise CRC oracle.
// No production encoder or field-writing helper constructs this payload.
Bytes golden() {
	Bytes bytes(4957, 0);
	const Bytes prefix{'M', 'M', 'M', 'S', 'A', 'V', 'E', 0, 1, 0, 0, 0, 0x49, 0x13, 0, 0};
	std::copy(prefix.begin(), prefix.end(), bytes.begin());
	for (int i = 0; i < 12; ++i) bytes[20 + i] = static_cast<std::uint8_t>(i + 1);
	bytes[46] = 1; bytes[49] = 15; bytes[50] = 3;
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

void wireContract() {
	XeenSaveSnapshot s;
	s.resources.clouds = {0x0807060504030201ULL, 0x0c0b0a09U};
	s.camera = {1, 0, 15, XeenDirection::West};
	const auto expected = golden();
	s.itemState = XeenSaveItemState::LegacyV1MissingFields;
	sameSnapshot(s, XeenSaveFormat::decode(expected));
	XeenSaveFormat::validate(s);
	rejects([&] { XeenSaveFormat::encode(s); }, "unresolved legacy");
	s.itemState = XeenSaveItemState::Complete;
	Bytes v2(6847, 0);
	std::copy_n(expected.begin(), 53, v2.begin());
	v2[8] = 2;
	for (unsigned i = 0; i < 30; ++i) v2[53 + 212 * i] = i;
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
	check(bytes[244] == 0 && bytes[245] == 128 && bytes[246] == 255 && bytes[247] == 127 &&
		bytes[263] == 255 && bytes[264] == 255 && bytes[265] == 255, "v2 HP/SP/condition/year byte layout");
}

void asymmetricV2() {
	// Independent offsets: three members, 14 name bytes, 212 fixed bytes/character.
	Bytes bytes(6847 + 3 + 14, 0);
	const Bytes prefix{'M','M','M','S','A','V','E',0,2,0,0,0};
	std::copy(prefix.begin(), prefix.end(), bytes.begin());
	bytes[20] = 99; bytes[28] = 88; bytes[46] = 1;
	bytes[51] = 3; bytes[52] = 18; bytes[53] = 0; bytes[54] = 18; bytes[55] = 30;
	XeenSaveSnapshot expected;
	expected.resources.clouds = {99, 88}; expected.camera = {1, 0, 0, XeenDirection::North};
	expected.activeRosterIds = {18, 0, 18};
	std::size_t base = 56;
	for (unsigned i = 0; i < 30; ++i) {
		auto &c = expected.characters[i];
		c.name = i == 0 ? "A" : i == 18 ? "Owner" : i == 29 ? "Inactive" : "";
		bytes[base] = i; bytes[base + 1] = c.name.size();
		std::copy(c.name.begin(), c.name.end(), bytes.begin() + base + 2);
		const auto itemsStart = base + 46 + c.name.size();
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
		base += 212 + c.name.size();
	}
	expected.questItems[34] = 0x12345678; put32(bytes, base + 34 * 4, 0x12345678);
	expected.questFlags[29] = true; bytes[base + 169] = 1;
	expected.gameFlags[255] = true; bytes[base + 425] = 1;
	fixIndependentEnvelope(bytes);
	check(XeenSaveFormat::encode(expected) == bytes, "asymmetric independent v2 byte order");
	sameSnapshot(expected, XeenSaveFormat::decode(bytes));
	check(bytes == XeenSaveFormat::encode(XeenSaveFormat::decode(bytes)), "v2 deterministic oracle");
	// Each legal item byte can change without becoming structural corruption.
	for (unsigned field = 0; field < 144; ++field) {
		auto changed = bytes; changed[103 + field] ^= 255; fixIndependentEnvelope(changed);
		check(XeenSaveFormat::encode(XeenSaveFormat::decode(changed)) == changed, "legal item byte rejected or normalized");
	}
	for (std::size_t size = 20; size < bytes.size(); ++size) {
		Bytes truncated(bytes.begin(), bytes.begin() + size); fixIndependentEnvelope(truncated);
		rejects([&] { XeenSaveFormat::decode(truncated); });
	}
	for (const auto offset : {103U, 138U, 175U, 246U, 56U + 212U * 29U + 6U + 46U + 8U}) {
		auto removed = bytes; removed.erase(removed.begin() + offset); fixIndependentEnvelope(removed);
		rejects([&] { XeenSaveFormat::decode(removed); });
		auto inserted = bytes; inserted.insert(inserted.begin() + offset, 255); fixIndependentEnvelope(inserted);
		rejects([&] { XeenSaveFormat::decode(inserted); });
	}
}

void v3WireContract() {
	auto expectedSnapshot = completedSample();
	auto ordinary = expectedSnapshot; ordinary.completedEncounter.reset();
	auto expected = XeenSaveFormat::encode(ordinary);
	const auto base = expected.size();
	auto u8=[&](std::uint8_t v){expected.push_back(v);};
	auto u16=[&](std::uint16_t v){u8(v);u8(v>>8);};
	auto u32=[&](std::uint32_t v){for(unsigned i=0;i<4;++i)u8(v>>(8*i));};
	u8(1);u8(2);u8(1);u8(1);u8(0);u16(20);u32(5);u8(0);u8(0);
	const auto &context=expectedSnapshot.completedEncounter->context;
	u16(context.ctr24);u16(context.day);u16(context.year);u16(context.minutes);
	for(auto v:context.effects)u8(v);for(auto v:context.lightAndResistances)u16(v);
	u8(context.rested);u8(context.newDay);u8(6);
	for(const auto &record:expectedSnapshot.completedEncounter->supplements) {
		u8(record.owner);
		for(int v:{record.inputs.might.permanent,record.inputs.might.temporary,
			record.inputs.speed.permanent,record.inputs.speed.temporary,
			record.inputs.accuracy.permanent,record.inputs.accuracy.temporary,record.inputs.temporaryAc})u32(static_cast<std::uint32_t>(v));
		u32(record.inputs.experience);
	}
	check(expected.size()==base+243,"independent v3 extension is not 243 bytes");
	expected[8]=3;fixIndependentEnvelope(expected);
	check(XeenSaveFormat::encode(expectedSnapshot)==expected,"encoder differs from independent v3 suffix oracle");
	sameSnapshot(expectedSnapshot,XeenSaveFormat::decode(expected));
	auto lowerBound=expectedSnapshot;lowerBound.completedEncounter->context.ctr24=0;
	lowerBound.completedEncounter->context.minutes=491;roundTrip(lowerBound);
	check(expected[base+44]==6&&expected[base+45]==0&&expected[base+78]==1&&expected[base+111]==6&&
		expected[base+144]==11&&expected[base+177]==14&&expected[base+210]==18,"v3 fixed owner offsets differ");

	// Every new compared field, including presence and explicit zero, has a negative control.
	auto changed=expectedSnapshot;changed.completedEncounter.reset();
	check(!sameCompleted(expectedSnapshot.completedEncounter,changed.completedEncounter),"completion presence mismatch was ignored");
	for(unsigned record=0;record<6;++record)for(unsigned field=0;field<9;++field) {
		changed=expectedSnapshot;auto &r=changed.completedEncounter->supplements[record];
		if(field==0)r.owner^=1;
		else if(field==1)r.inputs.might.permanent^=1;else if(field==2)r.inputs.might.temporary^=1;
		else if(field==3)r.inputs.speed.permanent^=1;else if(field==4)r.inputs.speed.temporary^=1;
		else if(field==5)r.inputs.accuracy.permanent^=1;else if(field==6)r.inputs.accuracy.temporary^=1;
		else if(field==7)r.inputs.temporaryAc^=1;else r.inputs.experience^=1;
		check(!sameCompleted(expectedSnapshot.completedEncounter,changed.completedEncounter),"supplement mismatch was ignored");
	}
	auto mismatch=[&](auto mutate){changed=expectedSnapshot;mutate(*changed.completedEncounter);
		check(!sameCompleted(expectedSnapshot.completedEncounter,changed.completedEncounter),"completion/context mismatch was ignored");};
	mismatch([](auto &e){e.entry=XeenEncounterEntry::Diagnostic26;});
	mismatch([](auto &e){e.victory=false;});mismatch([](auto &e){e.accountingConsumed=false;});
	mismatch([](auto &e){e.monster.mapId.side=XeenSide::Darkside;});
	mismatch([](auto &e){e.monster.mapId.number++;});mismatch([](auto &e){e.monster.recordIndex++;});
	mismatch([](auto &e){e.context.profile=static_cast<XeenBehaviorProfile>(1);});
	mismatch([](auto &e){e.context.difficulty=XeenDifficulty::Warrior;});
	mismatch([](auto &e){e.context.ctr24--;});mismatch([](auto &e){e.context.day++;});
	mismatch([](auto &e){e.context.year++;});mismatch([](auto &e){e.context.minutes--;});
	for(unsigned i=0;i<9;++i)mismatch([i](auto &e){e.context.effects[i]=1;});
	for(unsigned i=0;i<6;++i)mismatch([i](auto &e){e.context.lightAndResistances[i]=1;});
	mismatch([](auto &e){e.context.rested=true;});mismatch([](auto &e){e.context.newDay=true;});

	for(std::size_t size=base;size<expected.size();++size) {
		Bytes truncated(expected.begin(),expected.begin()+size);fixIndependentEnvelope(truncated);
		rejects([&]{XeenSaveFormat::decode(truncated);});
	}
	auto malformed=expected;
	for(auto value:{0,27,30,255}){malformed=expected;malformed[base+44]=static_cast<std::uint8_t>(value);fixIndependentEnvelope(malformed);rejects([&]{XeenSaveFormat::decode(malformed);});}
	for(auto offset:{0U,1U,2U,3U,4U,5U,7U,11U,12U,42U,43U}) {
		malformed=expected;malformed[base+offset]=static_cast<std::uint8_t>(malformed[base+offset]+1);fixIndependentEnvelope(malformed);rejects([&]{XeenSaveFormat::decode(malformed);});
	}
	for(auto offset:{0U,1U,2U,3U}){malformed=expected;malformed[base+offset]=0;fixIndependentEnvelope(malformed);rejects([&]{XeenSaveFormat::decode(malformed);});}
	for(auto offset:{42U,43U}){malformed=expected;malformed[base+offset]=2;fixIndependentEnvelope(malformed);rejects([&]{XeenSaveFormat::decode(malformed);},"boolean");}
	for(unsigned record=0;record<6;++record){malformed=expected;malformed[base+45+33*record]^=1;fixIndependentEnvelope(malformed);rejects([&]{XeenSaveFormat::decode(malformed);});}
	malformed=expected;put32(malformed,base+46,0xffffffffU);fixIndependentEnvelope(malformed);rejects([&]{XeenSaveFormat::decode(malformed);},"range");
	malformed=expected;put32(malformed,base+46,256);fixIndependentEnvelope(malformed);rejects([&]{XeenSaveFormat::decode(malformed);},"range");
	malformed=expected;malformed.push_back(0);fixIndependentEnvelope(malformed);rejects([&]{XeenSaveFormat::decode(malformed);});
	malformed=XeenSaveFormat::encode(ordinary);malformed[8]=3;fixIndependentEnvelope(malformed);rejects([&]{XeenSaveFormat::decode(malformed);},"extension");
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
	copy.disabledObjects.clear(); copy.disabledEvents.clear(); copy.activeRosterIds.clear();
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
	bad = good; bad[8] = 5; rejects([&] { XeenSaveFormat::decode(bad); }, "unsupported version");
	bad = good; bad[8] = 3; rejects([&] { XeenSaveFormat::decode(bad); }); // v1 payload is not v3.
	bad = good; bad[8] = 2; rejects([&] { XeenSaveFormat::decode(bad); }); // v1 payload is not v2.
	bad = good; bad[8] = 0; rejects([&] { XeenSaveFormat::decode(bad); }, "unsupported version");
	bad = good; bad[10] = 1; rejects([&] { XeenSaveFormat::decode(bad); }, "unsupported game side");
	bad = good; bad[11] = 1; rejects([&] { XeenSaveFormat::decode(bad); }, "reserved");
	bad = good; bad[16] ^= 1; rejects([&] { XeenSaveFormat::decode(bad); }, "checksum");
	bad = good; put32(bad, 12, 0xffffffffU); rejects([&] { XeenSaveFormat::decode(bad); }, "length");
	bad = good; bad.push_back(0); fixEnvelope(bad);
	rejects([&] { XeenSaveFormat::decode(bad); }, "trailing");
	rejects([&] { XeenSaveFormat::decode(Bytes(XeenSaveFormat::kMaximumSize + 1)); }, "oversized");
	for (const auto offset : {32U, 94U, 95U, 96U, 97U, 98U, 4663U, 4693U}) {
		bad = good; bad[offset] = 2; fixEnvelope(bad);
		rejects([&] { XeenSaveFormat::decode(bad); }, "boolean");
	}
	bad = good; bad[33] = 1; fixEnvelope(bad);
	rejects([&] { XeenSaveFormat::decode(bad); }, "absent archive");
	for (const auto offset : {4949U, 4953U}) {
		bad = good; put32(bad, offset, 0xffffffffU); fixEnvelope(bad);
		rejects([&] { XeenSaveFormat::decode(bad); }, "count");
	}
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
	// Original identity arrays end the payload and must reject duplicates/order.
	for (bool events : {false, true}) {
		bad = XeenSaveFormat::encode(s);
		const auto first = events ? bad.size() - 21 : bad.size() - 46;
		std::copy_n(bad.begin() + first, 7, bad.begin() + first + 7);
		fixEnvelope(bad); rejects([&] { XeenSaveFormat::decode(bad); }, "ordered");
	}
}

void invalidValuesAndLimits() {
	const auto good = sample();
	for (int mode = 0; mode < 14; ++mode) {
		auto s = good;
		switch (mode) {
		case 0: s.camera.x = -1; break;
		case 1: s.camera.y = 16; break;
		case 2: s.camera.direction = static_cast<XeenDirection>(4); break;
		case 3: s.camera.mapId = 0; break;
		case 4: s.camera.mapId = 10000; break;
		case 5: s.camera.mapId.side = XeenSide::Darkside; break;
		case 6: s.activeRosterIds.push_back(0); break;
		case 7: s.activeRosterIds[0] = 30; break;
		case 8: s.characters[29].rosterId = 0; break;
		case 9: s.characters[0].name = std::string(17, 'a'); break;
		case 10: s.characters[0].name = std::string("a\0b", 3); break;
		case 11: s.disabledObjects[1] = s.disabledObjects[0]; break;
		case 12: std::swap(s.disabledEvents[0], s.disabledEvents[1]); break;
		case 13: s.disabledEvents[0].mapId.side = static_cast<XeenSide>(255); break;
		}
		rejects([&] { XeenSaveFormat::encode(s); });
	}
	if (std::numeric_limits<std::size_t>::max() > 0xffffffffULL) {
		auto s = good; s.disabledObjects[0].recordIndex = static_cast<std::size_t>(0x100000000ULL);
		rejects([&] { XeenSaveFormat::encode(s); }, "representation");
	}
	auto maximum = good;
	maximum.disabledObjects.clear(); maximum.disabledEvents.clear();
	for (std::size_t i = 0; i < XeenSaveFormat::kMaximumObjects; ++i) maximum.disabledObjects.push_back({1, i});
	for (std::size_t i = 0; i < XeenSaveFormat::kMaximumEvents; ++i) maximum.disabledEvents.push_back({1, i});
	roundTrip(maximum);
	maximum.disabledObjects.push_back({1, XeenSaveFormat::kMaximumObjects});
	rejects([&] { XeenSaveFormat::encode(maximum); }, "too many");
	maximum.disabledObjects.pop_back();
	maximum.disabledEvents.push_back({1, XeenSaveFormat::kMaximumEvents});
	rejects([&] { XeenSaveFormat::encode(maximum); }, "too many");
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
XeenServiceEconomy literalMerchantEconomy() {
	// Independent possible output: all twenty calls choose Weapons; calls after
	// the eighth discard. First eight levels follow each shop's literal bands.
	XeenServiceEconomy e;
	const std::array<std::array<std::uint8_t,8>,8> materials{{
		{{0,0,0,0,0,0,0,0}},{{0,0,0,0,0,37,37,37}},
		{{0,0,0,0,0,37,37,37}},{{0,0,0,0,0,37,37,37}},
		{{0,0,0,0,0,0,0,0}},{{0,0,0,0,0,37,37,37}},
		{{40,40,40,40,40,41,41,41}},{{39,39,39,39,39,40,40,40}}
	}};
	for (unsigned shop=0;shop<8;++shop) for(unsigned slot=0;slot<8;++slot)
		e.wares[shop/4][shop%4][0][slot]={materials[shop][slot],1,0,0};
	e.bank.gold=0xfedcba98u;e.bank.gems=0xffffffffu;
	return e;
}

XeenSaveSnapshot journeyWire(std::uint16_t contract) {
	XeenSaveSnapshot s;
	s.camera = {23,9,11,XeenDirection::West};
	s.journey.emplace();
	auto &j = *s.journey;
	j.schema = xeenJourneyContent(contract).schema();
	j.contract = contract;
	j.context.emplace();
	j.context->day = contract == 1 ? 1 : 8;
	j.context->year = 610;
	j.context->minutes = 577;
	for (unsigned i=0; i<30; ++i) {
		j.supplements[i].owner = i;
		auto &input = j.supplements[i].inputs;
		if (contract >= 2) input.luck.emplace();
		if (contract >= 4) input.resistances.emplace();
		if (contract >= 8) input.poisonResistance.emplace();
		if (contract >= 7) s.characters[i].learnedSpells.emplace();
	}
	if (contract == 1) j.skeletonSeed = 1;
	else j.random.emplace();
	j.initializedMap = contract >= 3 ? 23 : 20;
	j.originalActorCount = contract >= 3 ? 19 : 27;
	for (unsigned i=0; i<(contract >= 3 ? 19u : contract == 2 ? 4u : 1u); ++i) {
		XeenSaveJourneyActor actor;
		actor.id = {j.initializedMap, contract >= 3 ? i : xeenJourneyContent(contract).records[i]};
		j.actors.push_back(actor);
	}
	if (contract >= 4) j.treasure.emplace();
	if (contract >= 6) j.regionalRecovery.emplace();
	if (contract >= 11 && contract <= 14) j.serviceEconomy=literalMerchantEconomy();
	return s;
}

void ironworksWireContract() {
	const auto legacy = XeenSaveFormat::encode(journeyWire(8));
	const auto successor = XeenSaveFormat::encode(journeyWire(9));
	const auto start = successor.size() - 3114;
	check(successor.size() == legacy.size() && successor[8] == 4 &&
		successor[start] == 3 && successor[start+1] == 8 && successor[start+2] == 0 &&
		successor[start+3] == 9 && successor[start+4] == 0 && successor[start+5] == 1,
		"Ironworks must retain v4/schema8 and exact absent-city suffix extent");
	auto expected = legacy;
	expected[start+3] = 9;
	fixIndependentEnvelope(expected);
	check(expected == successor, "8/9 changed schema8 representation beyond content selector");
	for (unsigned count=1; count<=12; ++count) {
		auto pending = journeyWire(9);
		pending.journey->treasure->gold = 0xffffffffU;
		pending.journey->treasure->gems = 0xffffffffU;
		for (unsigned source=0; source<count; ++source) {
			auto &actor = pending.journey->actors[source];
			actor.accounted = true; actor.lifecycle = XeenActorLifecycle::Defeated;
			actor.x = actor.y = -128;
			auto &entry = source<10 ? pending.journey->treasure->weapons[source] : pending.journey->treasure->armor[source-10];
			entry.source = source; entry.item.id = 1;
		}
		const auto bytes = XeenSaveFormat::encode(pending);
		check(bytes.size() == successor.size()+5*count, "Ironworks pending-item wire extent");
		const auto restored = XeenSaveFormat::decode(bytes);
		check(restored.journey->treasure->gold == 0xffffffffU && restored.journey->treasure->dormant() &&
			XeenSaveFormat::encode(restored) == bytes, "Ironworks full-u32 purse/dormant treasure changed");
	}
	for (unsigned contract=1; contract<=14; ++contract) {
		const auto bytes = XeenSaveFormat::encode(journeyWire(contract));
		const auto decoded = XeenSaveFormat::decode(bytes);
		check(decoded.journey->contract == contract && decoded.journey->schema == xeenJourneyContent(contract).schema(),
			"supported pair changed on decode");
		check(XeenSaveFormat::encode(decoded) == bytes, "legacy/successor bytes changed on recapture");
	}
	for (unsigned schema=0; schema<=11; ++schema) for (unsigned contract=0; contract<=14; ++contract) {
		const bool supported = (schema >= 1 && schema <= 8 && schema == contract) || (schema == 8 && (contract == 9 || contract == 10)) || (schema == 9 && (contract >= 11 && contract <= 14));
		if (supported) continue;
		auto invalid = journeyWire(9);
		invalid.journey->schema = schema; invalid.journey->contract = contract;
		rejects([&] { XeenSaveFormat::encode(invalid); }, "schema/contract");
		auto bytes = successor;
		bytes[start+1] = schema; bytes[start+3] = contract; fixIndependentEnvelope(bytes);
		rejects([&] { XeenSaveFormat::decode(bytes); }, "schema/contract");
	}
	auto city = journeyWire(9);
	city.journey->vertigoActors.emplace();
	for (unsigned i=0; i<46; ++i) {
		XeenSaveJourneyActor actor; actor.id = {28,i};
		city.journey->vertigoActors->push_back(actor);
	}
	city.camera = {28,8,4,XeenDirection::West};
	for (unsigned day=8; day<=10; ++day) {
		city.journey->context->day = day;
		const auto bytes = XeenSaveFormat::encode(city);
		check(bytes.size() == start+3992, "Ironworks 46-slot suffix extent");
		check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes)) == bytes, "Ironworks city/date recapture");
	}
	for (unsigned day : {0u,7u,11u,99u}) {
		auto invalid = city; invalid.journey->context->day = day;
		rejects([&] { XeenSaveFormat::encode(invalid); }, "calendar");
	}
	for (unsigned day : {9u,10u}) {
		auto invalid = journeyWire(9); invalid.journey->context->day = day;
		rejects([&] { XeenSaveFormat::encode(invalid); }, "retained city");
	}
	for (unsigned mode=0; mode<7; ++mode) {
		auto invalid = city;
		switch (mode) {
		case 0: invalid.journey->context->year = 611; break;
		case 1: invalid.journey->context->minutes = 299; break;
		case 2: invalid.journey->context->minutes = 1260; break;
		case 3: invalid.journey->context->effects[0] = 1; break;
		case 4: invalid.journey->context->rested = true; break;
		case 5: invalid.journey->context->newDay = true; break;
		case 6: invalid.journey->context->difficulty = XeenDifficulty::Warrior; break;
		}
		rejects([&] { XeenSaveFormat::encode(invalid); }, "calendar");
	}
	city.journey->context->day = 8;
	for (unsigned y=0; y<16; ++y) for (unsigned x=0; x<32; ++x) {
		city.camera.x = x; city.camera.y = y;
		const bool admitted = (x==15 && y<=4) || (x==16 && y>=1 && y<=4) || (y==4 && x>=8 && x<=14);
		if (admitted) XeenSaveFormat::encode(city);
		else rejects([&] { XeenSaveFormat::encode(city); });
	}
	city.camera = {28,8,4,XeenDirection::West};
	for (unsigned i=46; i<52; ++i) {
		XeenSaveJourneyActor actor; actor.id = {28,i}; city.journey->vertigoActors->push_back(actor);
	}
	rejects([&] { XeenSaveFormat::encode(city); }, "protection overlay");
	city.disabledEvents.push_back({28,764});
	const auto reset = XeenSaveFormat::encode(city);
	check(reset.size() == start+7+4106, "Ironworks reset suffix extent");
	check(XeenSaveFormat::encode(XeenSaveFormat::decode(reset)) == reset, "Ironworks reset recapture");
}

void serviceEconomyWireContract() {
	for(unsigned content:{11u,12u,13u,14u}) {
	auto s=journeyWire(content);
	for(unsigned owner=0;owner<30;++owner) {
		(*s.characters[owner].learnedSpells)[owner%39]=255-owner;
		s.journey->supplements[owner].inputs.poisonResistance=XeenAttributeValue{int(owner),int(255-owner)};
	}
	const auto bytes=XeenSaveFormat::encode(s);const auto start=bytes.size()-4278;
	check(bytes[8]==4 && bytes[start+1]==9 && bytes[start+3]==content,"exact v4/schema9/content selectors");
	auto inherited=s;inherited.journey->schema=8;inherited.journey->contract=10;inherited.journey->serviceEconomy.reset();
	auto expected=XeenSaveFormat::encode(inherited);expected[start+1]=9;expected[start+3]=content;
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
	fixIndependentEnvelope(expected);
	check(expected==bytes,"M40 exact inherited suffix then shape/stock/bank wire");
	const auto decoded=XeenSaveFormat::decode(expected);sameSnapshot(s,decoded);
	check(XeenSaveFormat::encode(decoded)==expected,"M40 exact schema9 round trip");
	const auto economyOffset=bytes.size()-1164;
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
	for(std::size_t cut=1;cut<=1164;++cut) {
		auto bad=bytes;bad.resize(bad.size()-cut);fixIndependentEnvelope(bad);
		rejects([&]{XeenSaveFormat::decode(bad);});
	}
	auto bad=bytes;bad.push_back(0);fixIndependentEnvelope(bad);rejects([&]{XeenSaveFormat::decode(bad);});
	bad=bytes;bad[economyOffset+4]^=1;rejects([&]{XeenSaveFormat::decode(bad);},"checksum");
	auto missing=s;missing.journey->serviceEconomy.reset();rejects([&]{XeenSaveFormat::encode(missing);},"economy presence");
	for(unsigned contract=1;contract<=10;++contract) {
		auto legacy=journeyWire(contract);legacy.journey->serviceEconomy=s.journey->serviceEconomy;
		rejects([&]{XeenSaveFormat::encode(legacy);},"economy presence");
	}
	for(unsigned cityCount:{0u,46u,52u})for(unsigned pending=0;pending<=12;++pending) {
		auto state=s;
		if(cityCount) {
			state.journey->vertigoActors.emplace();
			for(unsigned owner=0;owner<cityCount;++owner) {XeenSaveJourneyActor a;a.id={28,owner};state.journey->vertigoActors->push_back(a);}
			state.camera={28,8,4,XeenDirection::West};
			if(cityCount==52)state.disabledEvents.push_back({28,764});
		}
		for(unsigned source=0;source<pending;++source) {
			auto &actor=state.journey->actors[source];actor.accounted=true;actor.lifecycle=XeenActorLifecycle::Defeated;actor.x=actor.y=-128;
			auto &entry=source<10?state.journey->treasure->weapons[source]:state.journey->treasure->armor[source-10];entry.source=source;entry.item.id=1;
		}
		const auto wire=XeenSaveFormat::encode(state);
		check(wire.size()==start+(cityCount==0?4278:cityCount==46?5156:5270)+5*pending+(cityCount==52?7:0),"M40 exact city/pending suffix extent");
		sameSnapshot(state,XeenSaveFormat::decode(wire));
	}
	for(unsigned day=8;day<=99;++day) {
		auto state=s;state.journey->context->day=day;
		if(day>8) {state.journey->vertigoActors.emplace();for(unsigned owner=0;owner<46;++owner){XeenSaveJourneyActor a;a.id={28,owner};state.journey->vertigoActors->push_back(a);}}
		const auto wire=XeenSaveFormat::encode(state);sameSnapshot(state,XeenSaveFormat::decode(wire));
	}
	for(unsigned day:{0u,7u,100u,65535u}) {auto state=s;state.journey->context->day=day;rejects([&]{XeenSaveFormat::encode(state);},"calendar");}
	for(unsigned day:{9u,11u,99u}) {auto state=s;state.journey->context->day=day;rejects([&]{XeenSaveFormat::encode(state);},"retained city");}
	}
}
void purchaseDepletedWireContract() {
	auto state=journeyWire(13);state.camera={28,8,4,XeenDirection::West};state.journey->vertigoActors.emplace();
	for(unsigned owner=0;owner<46;++owner){XeenSaveJourneyActor a;a.id={28,owner};state.journey->vertigoActors->push_back(a);}
	const auto complete=XeenSaveFormat::encode(state);
	const auto economyOffset=complete.size()-1164,start=complete.size()-5156;
	check(complete[8]==4 && complete[start+1]==9 && complete[start+3]==13,"purchase selector/extent differs");
	// Literal eight L1 Weapon source from twenty Weapon calls. Removing one
	// inserted plain record leaves seven; old generated-only meaning rejects it.
	state.journey->serviceEconomy->wares[0][0][0][7]={};
	rejects([&]{XeenSaveFormat::encode(state);}); // Quiet saved day8 must be complete.
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
		auto rejected=state;rejected.journey->contract=legacy;
		rejects([&]{XeenSaveFormat::encode(rejected);});
		auto bytes=depleted;bytes[start+3]=legacy;fixIndependentEnvelope(bytes);
		rejects([&]{XeenSaveFormat::decode(bytes);});
	}
	auto day8=depleted;day8[dayOffset]=8;fixIndependentEnvelope(day8);
	rejects([&]{XeenSaveFormat::decode(day8);});
	for(unsigned category:{2u,3u})for(unsigned slot=0;slot<9;++slot) {
		auto bad=depleted;bad[economyOffset+4+category*36+slot*4+1]=255;fixIndependentEnvelope(bad);
		rejects([&]{XeenSaveFormat::decode(bad);});
	}
}

void templeWireContract() {
	auto state=journeyWire(14);
	state.journey->vertigoActors.emplace();
	for(unsigned owner=0;owner<46;++owner) {
		XeenSaveJourneyActor actor;actor.id={28,owner};
		state.journey->vertigoActors->push_back(actor);
	}
	for(unsigned y=8;y<=28;++y) {
		state.camera={28,15,static_cast<int>(y),XeenDirection::North};
		const auto bytes=XeenSaveFormat::encode(state);
		const auto decoded=XeenSaveFormat::decode(bytes);
		check(decoded.journey->schema==9 && decoded.journey->contract==14 &&
			decoded.camera.mapId==state.camera.mapId && decoded.camera.x==state.camera.x &&
			decoded.camera.y==state.camera.y && decoded.camera.direction==state.camera.direction &&
			XeenSaveFormat::encode(decoded)==bytes,
			"M43 Temple corridor did not round-trip as schema 9/content 14");
		auto legacy=state;legacy.journey->contract=13;
		rejects([&]{XeenSaveFormat::encode(legacy);});
	}
}

} // namespace

int main() {
	try {
		wireContract(); asymmetricV2(); v3WireContract(); completeRoundTrips(); numericDomains(); malformedBytes(); invalidValuesAndLimits(); fingerprints(); ironworksWireContract(); serviceEconomyWireContract(); purchaseDepletedWireContract(); templeWireContract();
		std::cout << "M20A save format: wire contract, all modeled values, domains, malformed input and fingerprints passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n'; return 1;
	}
}
