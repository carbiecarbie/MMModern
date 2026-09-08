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
	check(XeenSaveFormat::encode(s) == expected, "encoder differs from independent v1 bytes");
	sameSnapshot(s, XeenSaveFormat::decode(expected));
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
	check(bytes[181] == 0 && bytes[182] == 128 && bytes[183] == 255 && bytes[184] == 127 &&
		bytes[200] == 255 && bytes[201] == 255 && bytes[202] == 255, "HP/SP/condition/year byte layout");
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
	for (int *field : {&c.intellect.permanent, &c.intellect.temporary, &c.personality.permanent,
			&c.personality.temporary, &c.endurance.permanent, &c.endurance.temporary,
			&c.permanentLevel, &c.temporaryLevel, &c.temporaryAge})
		for (int value : {std::numeric_limits<int>::min(), -2, 0, 255, 256, 70000,
				std::numeric_limits<int>::max()}) {
			*field = value; roundTrip(s);
		}
	for (auto *items : {&c.weapons, &c.armor, &c.accessories})
		for (auto &item : *items) {
			item = {0, 255, 0}; roundTrip(s);
			item = {255, 0, 255}; roundTrip(s);
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
	bad = good; bad[8] = 2; rejects([&] { XeenSaveFormat::decode(bad); }, "unsupported version");
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

} // namespace

int main() {
	try {
		wireContract(); completeRoundTrips(); numericDomains(); malformedBytes(); invalidValuesAndLimits(); fingerprints();
		std::cout << "M20A save format: wire contract, all modeled values, domains, malformed input and fingerprints passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n'; return 1;
	}
}
