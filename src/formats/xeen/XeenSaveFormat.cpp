#include "formats/xeen/XeenSaveFormat.h"

#include <zlib.h>

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mmodern {
namespace {

constexpr std::array<std::uint8_t, 8> kMagic{'M', 'M', 'M', 'S', 'A', 'V', 'E', 0};
static_assert(std::numeric_limits<int>::digits == 31, "save v1 requires 32-bit character integers");
static_assert(XeenSaveFormat::kMaximumSize <= std::numeric_limits<uInt>::max(),
	"save CRC input must fit zlib's length type");

void require(bool value, const char *message) {
	if (!value) throw std::runtime_error(std::string("MMModern save: ") + message);
}

void validateMap(XeenMapIdentity id) {
	require(id.side == XeenSide::Clouds && id.number >= 1 && id.number <= 9999,
		"invalid Clouds map identity");
}

template<class Identity>
void validateIdentities(const std::vector<Identity> &ids, std::size_t limit) {
	require(ids.size() <= limit, "too many disabled identities");
	for (std::size_t i = 0; i < ids.size(); ++i) {
		validateMap(ids[i].mapId);
		require(ids[i].recordIndex <= std::numeric_limits<std::uint32_t>::max(),
			"original record index exceeds save representation");
		require(i == 0 || ids[i - 1] < ids[i], "disabled identities must be strictly ordered");
	}
}

std::uint32_t checksum(const std::uint8_t *data, std::size_t size) {
	return static_cast<std::uint32_t>(crc32(crc32(0L, Z_NULL, 0), data, static_cast<uInt>(size)));
}

// Private, fixed-format byte operations; not a general serialization layer.
struct Writer {
	std::vector<std::uint8_t> bytes;
	void integer(std::uint64_t value, unsigned width) {
		require(bytes.size() <= XeenSaveFormat::kMaximumSize - width, "save is oversized");
		for (unsigned i = 0; i < width; ++i)
			bytes.push_back(static_cast<std::uint8_t>(value >> (8 * i)));
	}
	void u8(std::uint8_t value) { integer(value, 1); }
	void u16(std::uint16_t value) { integer(value, 2); }
	void u32(std::uint32_t value) { integer(value, 4); }
	void u64(std::uint64_t value) { integer(value, 8); }
	void i16(std::int16_t value) { u16(static_cast<std::uint16_t>(value)); }
	void i32(int value) { u32(static_cast<std::uint32_t>(value)); }
	void map(XeenMapIdentity id) { u8(static_cast<std::uint8_t>(id.side)); u16(id.number); }
	void fingerprint(XeenArchiveFingerprint value) { u64(value.size); u32(value.crc32); }
};

struct Reader {
	const std::vector<std::uint8_t> &bytes;
	std::size_t position = 0;
	std::size_t remaining() const { return bytes.size() - position; }
	std::uint64_t integer(unsigned width) {
		require(width <= remaining(), "truncated input");
		std::uint64_t value = 0;
		for (unsigned i = 0; i < width; ++i)
			value |= static_cast<std::uint64_t>(bytes[position++]) << (8 * i);
		return value;
	}
	std::uint8_t u8() { return static_cast<std::uint8_t>(integer(1)); }
	std::uint16_t u16() { return static_cast<std::uint16_t>(integer(2)); }
	std::uint32_t u32() { return static_cast<std::uint32_t>(integer(4)); }
	std::uint64_t u64() { return integer(8); }
	std::int16_t i16() {
		const auto value = u16();
		return static_cast<std::int16_t>(value <= 0x7fff ? value : static_cast<int>(value) - 0x10000);
	}
	int i32() {
		const auto value = u32();
		return static_cast<int>(value <= 0x7fffffffU ? static_cast<std::int64_t>(value) :
			static_cast<std::int64_t>(value) - 0x100000000LL);
	}
	bool boolean() {
		const auto value = u8();
		require(value <= 1, "invalid boolean byte");
		return value != 0;
	}
	XeenMapIdentity map() {
		const auto side = static_cast<XeenSide>(u8());
		return {side, u16()};
	}
	XeenArchiveFingerprint fingerprint() {
		const auto size = u64();
		return {size, u32()};
	}
};

void writeCharacter(Writer &out, const XeenCharacter &c) {
	out.u8(c.rosterId);
	out.u8(static_cast<std::uint8_t>(c.name.size()));
	for (unsigned char byte : c.name) out.u8(byte);
	out.u8(static_cast<std::uint8_t>(c.sex));
	out.u8(static_cast<std::uint8_t>(c.race));
	out.u8(static_cast<std::uint8_t>(c.characterClass));
	for (const auto a : {c.intellect, c.personality, c.endurance}) {
		out.i32(a.permanent); out.i32(a.temporary);
	}
	out.i32(c.permanentLevel); out.i32(c.temporaryLevel); out.i32(c.temporaryAge);
	out.u8(c.maxStatSkills.astrologer); out.u8(c.maxStatSkills.bodybuilder);
	out.u8(c.maxStatSkills.prayerMaster); out.u8(c.maxStatSkills.prestidigitation);
	out.u8(c.hasSpells);
	for (const auto *items : {&c.weapons, &c.armor, &c.accessories})
		for (const auto item : *items) {
			out.u8(item.material); out.u8(item.state); out.u8(item.frame);
		}
	out.i16(c.currentHp); out.i16(c.currentSp);
	for (const auto value : c.conditions) out.u8(value);
	out.u16(c.birthYear);
}

XeenCharacter readCharacter(Reader &in) {
	XeenCharacter c;
	c.rosterId = in.u8();
	const auto length = in.u8();
	require(length <= 16 && length <= in.remaining(), "invalid character name length");
	c.name.assign(reinterpret_cast<const char *>(in.bytes.data() + in.position), length);
	in.position += length;
	c.sex = static_cast<XeenSex>(in.u8());
	c.race = static_cast<XeenRace>(in.u8());
	c.characterClass = static_cast<XeenCharacterClass>(in.u8());
	for (auto *a : {&c.intellect, &c.personality, &c.endurance}) {
		a->permanent = in.i32(); a->temporary = in.i32();
	}
	c.permanentLevel = in.i32(); c.temporaryLevel = in.i32(); c.temporaryAge = in.i32();
	c.maxStatSkills.astrologer = in.boolean(); c.maxStatSkills.bodybuilder = in.boolean();
	c.maxStatSkills.prayerMaster = in.boolean(); c.maxStatSkills.prestidigitation = in.boolean();
	c.hasSpells = in.boolean();
	for (auto *items : {&c.weapons, &c.armor, &c.accessories})
		for (auto &item : *items) {
			item.material = in.u8(); item.state = in.u8(); item.frame = in.u8();
		}
	c.currentHp = in.i16(); c.currentSp = in.i16();
	for (auto &value : c.conditions) value = in.u8();
	c.birthYear = in.u16();
	return c;
}

template<class Identity>
void writeIdentities(Writer &out, const std::vector<Identity> &ids) {
	out.u32(static_cast<std::uint32_t>(ids.size()));
	for (const auto id : ids) {
		out.map(id.mapId); out.u32(static_cast<std::uint32_t>(id.recordIndex));
	}
}

template<class Identity>
std::vector<Identity> readIdentities(Reader &in, std::size_t limit) {
	const auto count = in.u32();
	require(count <= limit && count <= in.remaining() / 7, "invalid disabled-identity count");
	std::vector<Identity> ids;
	ids.reserve(count);
	for (std::uint32_t i = 0; i < count; ++i) {
		const auto mapId = in.map();
		const auto index = in.u32();
		require(index <= std::numeric_limits<std::size_t>::max(), "record index cannot be represented");
		ids.push_back({mapId, static_cast<std::size_t>(index)});
	}
	return ids;
}

} // namespace

void XeenSaveFormat::validate(const XeenSaveSnapshot &s) {
	validateMap(s.camera.mapId);
	require(s.camera.x >= 0 && s.camera.x <= 15 && s.camera.y >= 0 && s.camera.y <= 15 &&
		static_cast<unsigned>(s.camera.direction) <= 3, "invalid committed camera");
	require(s.activeRosterIds.size() <= XeenParty::kMaximumVisibleMembers, "too many active members");
	for (const auto id : s.activeRosterIds)
		require(id < XeenRoster::kCharacterCount, "invalid active roster identity");
	for (std::size_t i = 0; i < s.characters.size(); ++i) {
		const auto &c = s.characters[i];
		require(c.rosterId == i, "character identity differs from roster slot");
		require(c.name.size() <= 16 && c.name.find('\0') == std::string::npos, "invalid character name");
	}
	validateIdentities(s.disabledObjects, kMaximumObjects);
	validateIdentities(s.disabledEvents, kMaximumEvents);
}

std::vector<std::uint8_t> XeenSaveFormat::encode(const XeenSaveSnapshot &s) {
	validate(s);
	Writer out;
	out.bytes.resize(kHeaderSize);
	out.fingerprint(s.resources.clouds);
	out.u8(s.resources.darkside.has_value());
	out.fingerprint(s.resources.darkside.value_or(XeenArchiveFingerprint{}));
	out.map(s.camera.mapId);
	out.u8(static_cast<std::uint8_t>(s.camera.x)); out.u8(static_cast<std::uint8_t>(s.camera.y));
	out.u8(static_cast<std::uint8_t>(s.camera.direction));
	out.u8(static_cast<std::uint8_t>(s.activeRosterIds.size()));
	for (const auto id : s.activeRosterIds) out.u8(id);
	out.u8(static_cast<std::uint8_t>(s.characters.size()));
	for (const auto &c : s.characters) writeCharacter(out, c);
	for (const auto count : s.questItems) out.u32(count);
	for (const bool value : s.questFlags) out.u8(value);
	for (const bool value : s.gameFlags) out.u8(value);
	writeIdentities(out, s.disabledObjects);
	writeIdentities(out, s.disabledEvents);
	Writer header;
	for (const auto byte : kMagic) header.u8(byte);
	header.u16(kVersion); header.u8(0); header.u8(0);
	header.u32(static_cast<std::uint32_t>(out.bytes.size() - kHeaderSize));
	header.u32(checksum(out.bytes.data() + kHeaderSize, out.bytes.size() - kHeaderSize));
	std::copy(header.bytes.begin(), header.bytes.end(), out.bytes.begin());
	return std::move(out.bytes);
}

XeenSaveSnapshot XeenSaveFormat::decode(const std::vector<std::uint8_t> &bytes) {
	require(bytes.size() <= kMaximumSize, "save is oversized");
	Reader in{bytes};
	for (const auto byte : kMagic) require(in.u8() == byte, "unrecognized format");
	require(in.u16() == kVersion, "unsupported version");
	require(in.u8() == 0, "unsupported game side");
	require(in.u8() == 0, "nonzero reserved byte");
	const auto length = in.u32();
	const auto crc = in.u32();
	require(length == in.remaining(), "payload length does not match file length");
	require(crc == checksum(bytes.data() + kHeaderSize, length), "payload checksum mismatch");
	XeenSaveSnapshot s;
	s.resources.clouds = in.fingerprint();
	const bool hasDarkside = in.boolean();
	const auto darkside = in.fingerprint();
	if (hasDarkside) s.resources.darkside = darkside;
	else require(darkside.size == 0 && darkside.crc32 == 0, "absent archive has nonzero signature");
	s.camera.mapId = in.map();
	s.camera.x = in.u8(); s.camera.y = in.u8();
	s.camera.direction = static_cast<XeenDirection>(in.u8());
	const auto members = in.u8();
	require(members <= XeenParty::kMaximumVisibleMembers, "too many active members");
	for (unsigned i = 0; i < members; ++i) s.activeRosterIds.push_back(in.u8());
	require(in.u8() == s.characters.size(), "incorrect roster count");
	for (auto &c : s.characters) c = readCharacter(in);
	for (auto &count : s.questItems) count = in.u32();
	for (auto &value : s.questFlags) value = in.boolean();
	for (auto &value : s.gameFlags) value = in.boolean();
	s.disabledObjects = readIdentities<XeenObjectIdentity>(in, kMaximumObjects);
	s.disabledEvents = readIdentities<XeenEventIdentity>(in, kMaximumEvents);
	require(in.remaining() == 0, "trailing payload data");
	validate(s);
	return s;
}

XeenArchiveFingerprint XeenSaveFormat::fingerprint(std::istream &stream) {
	require(stream.good(), "archive stream is not readable");
	std::array<char, 65536> buffer{};
	XeenArchiveFingerprint result;
	uLong crc = crc32(0L, Z_NULL, 0);
	for (;;) {
		try { stream.read(buffer.data(), buffer.size()); }
		catch (const std::ios_base::failure &) {
			if (stream.bad() || !stream.eof()) throw;
		}
		require(!stream.bad() && (!stream.fail() || stream.eof()), "archive read failed");
		const auto count = static_cast<std::uint64_t>(stream.gcount());
		require(count <= std::numeric_limits<std::uint64_t>::max() - result.size,
			"archive size overflow");
		result.size += count;
		crc = crc32(crc, reinterpret_cast<const Bytef *>(buffer.data()), static_cast<uInt>(count));
		if (stream.eof()) break;
	}
	result.crc32 = static_cast<std::uint32_t>(crc);
	return result;
}

} // namespace mmodern
