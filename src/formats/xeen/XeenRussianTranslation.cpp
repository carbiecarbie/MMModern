#include "formats/xeen/XeenRussianTranslation.h"

#include <zlib.h>

#include <algorithm>
#include <array>
#include <cstring>

namespace mmodern {
namespace {

#include "formats/xeen/XeenRussianEntries.inc"

// The five files of the translation, by size and CRC-32 (M110 decision 2).
struct KnownFile {
	const char *name;
	std::uint64_t size;
	std::uint32_t crc;
};
constexpr KnownFile kFiles[] = {
	{"xeen.cc", 13417817, 0x03ac657a},
	{"dark.cc", 11260653, 0x77e237e3},
	{"intro.cc", 8985197, 0xec52d29f},
	{"xeen.dat", 380928, 0x53496c03},
	{"xeen.exe", 142257, 0x52021807},
};

// Character and monster names: 16 bytes at the start of each record
// (pinned character.cpp Character::synchronize, map.cpp MonsterStruct).
constexpr std::size_t kNameBytes = 16;
constexpr std::size_t kCharacterBytes = 354, kMonsterBytes = 60;
// Mirror destinations: a 28-byte word, then map, x, y and direction.
constexpr std::size_t kMirrorBytes = 32, kMirrorWordBytes = 28;
constexpr std::uint16_t kRosterId = 0xd621; // maze.chr inside the New Game container

[[noreturn]] void refuse(XeenLayerFormat format, const std::string &why) {
	throw std::runtime_error(std::string("Russian ") + xeenLayerFormatName(format) +
		" entry does not keep the CD structure: " + why);
}

std::size_t nulCount(const std::vector<std::uint8_t> &bytes) {
	return static_cast<std::size_t>(std::count(bytes.begin(), bytes.end(), 0));
}

struct CcEntry {
	std::uint16_t id;
	std::size_t offset, size;
};

// The CC index as BaseCCArchive::loadIndex decodes it.
std::vector<CcEntry> ccIndex(const std::uint8_t *data, std::size_t size) {
	if (size < 2)
		throw std::runtime_error("Truncated CC index count");
	const std::size_t count = data[0] | (std::size_t(data[1]) << 8);
	if (!count || 2 + count * 8 > size)
		throw std::runtime_error("Invalid CC index extent");
	std::vector<std::uint8_t> raw(data + 2, data + 2 + count * 8);
	unsigned seed = 0xac;
	for (auto &value : raw) {
		value = static_cast<std::uint8_t>(((value << 2) | (value >> 6)) + seed);
		seed = (seed + 0x67) & 0xff;
	}
	std::vector<CcEntry> result;
	for (std::size_t i = 0; i < count; ++i) {
		const auto *e = raw.data() + i * 8;
		result.push_back({static_cast<std::uint16_t>(e[0] | (e[1] << 8)),
			std::size_t(e[2]) | (std::size_t(e[3]) << 8) | (std::size_t(e[4]) << 16),
			std::size_t(e[5]) | (std::size_t(e[6]) << 8)});
	}
	return result;
}

const CcEntry *findEntry(const std::vector<CcEntry> &index, std::uint16_t id) {
	const CcEntry *found = nullptr;
	for (const auto &entry : index)
		if (entry.id == id) {
			if (found)
				throw std::runtime_error("Ambiguous CC member ID");
			found = &entry;
		}
	return found;
}

void requireFrames(const std::vector<std::uint8_t> &bytes, const char *which, std::size_t &frames) {
	if (bytes.size() < 2)
		refuse(XeenLayerFormat::Picture, std::string(which) + " sprite has no frame count");
	frames = bytes[0] | (std::size_t(bytes[1]) << 8);
	if (!frames || 2 + frames * 4 > bytes.size())
		refuse(XeenLayerFormat::Picture, std::string(which) + " sprite index is truncated");
	for (std::size_t i = 0; i < frames * 2; ++i) {
		const std::size_t offset = bytes[2 + i * 2] | (std::size_t(bytes[3 + i * 2]) << 8);
		if (offset >= bytes.size())
			refuse(XeenLayerFormat::Picture, std::string(which) + " sprite cell lies outside the resource");
	}
}

std::vector<std::uint8_t> mergeNewGame(const std::vector<std::uint8_t> &cd,
		const std::vector<std::uint8_t> &translated) {
	// The block holds the container index and, inside it, the whole roster.
	const auto cdIndex = ccIndex(cd.data(), cd.size()), ruIndex = ccIndex(translated.data(), translated.size());
	const auto *cdRoster = findEntry(cdIndex, kRosterId), *ruRoster = findEntry(ruIndex, kRosterId);
	if (!cdRoster || !ruRoster)
		refuse(XeenLayerFormat::NewGameContainer, "maze.chr is missing");
	if (cdRoster->size != ruRoster->size || cdRoster->size % kCharacterBytes ||
			cdRoster->offset + cdRoster->size > cd.size() || ruRoster->offset + ruRoster->size > translated.size())
		refuse(XeenLayerFormat::NewGameContainer, "maze.chr extent differs or leaves the block");
	auto result = cd;
	for (std::size_t record = 0; record < cdRoster->size; record += kCharacterBytes)
		std::copy_n(translated.begin() + ruRoster->offset + record, kNameBytes,
			result.begin() + cdRoster->offset + record);
	return result;
}

} // namespace

const char *xeenLayerFormatName(XeenLayerFormat format) {
	switch (format) {
	case XeenLayerFormat::EventText: return "Event text";
	case XeenLayerFormat::MapName: return "map name";
	case XeenLayerFormat::StringTable: return "string table";
	case XeenLayerFormat::Picture: return "picture";
	case XeenLayerFormat::RawPicture: return "raw picture";
	case XeenLayerFormat::Font: return "font";
	case XeenLayerFormat::NewGameContainer: return "New Game container";
	case XeenLayerFormat::MonsterTable: return "monster table";
	case XeenLayerFormat::MirrorList: return "mirror list";
	case XeenLayerFormat::KeptCd: return "kept CD";
	}
	return "unknown";
}

const std::vector<XeenLayerEntry> &xeenRussianEntries(XeenArchiveRole role) {
	static const std::vector<XeenLayerEntry> clouds(std::begin(kCloudsEntries), std::end(kCloudsEntries)),
		darkside(std::begin(kDarksideEntries), std::end(kDarksideEntries)),
		intro(std::begin(kIntroEntries), std::end(kIntroEntries));
	return role == XeenArchiveRole::Clouds ? clouds : role == XeenArchiveRole::Darkside ? darkside : intro;
}

std::vector<std::uint8_t> xeenLayerMerge(XeenLayerFormat format,
		const std::vector<std::uint8_t> &cd, const std::vector<std::uint8_t> &translated) {
	switch (format) {
	case XeenLayerFormat::EventText:
	case XeenLayerFormat::MapName:
	case XeenLayerFormat::StringTable:
		// Callers address the strings by index.
		if (nulCount(cd) != nulCount(translated))
			refuse(format, "string count differs");
		return translated;
	case XeenLayerFormat::Picture: {
		std::size_t cdFrames = 0, ruFrames = 0;
		requireFrames(cd, "CD", cdFrames);
		requireFrames(translated, "translated", ruFrames);
		// The CD program addresses only the CD's frames.
		if (ruFrames < cdFrames)
			refuse(format, "fewer frames than the CD");
		return translated;
	}
	case XeenLayerFormat::RawPicture:
		if (cd.size() != 64000 || translated.size() != 64000)
			refuse(format, "not a 320x200 frame");
		return translated;
	case XeenLayerFormat::Font: {
		// Pinned font.cpp:48-57: English glyphs, second-bank glyphs, both reduced,
		// then the four width tables. The English half must be the CD font.
		if (cd.size() != 0x1100 || translated.size() != 0x2200)
			refuse(format, "not a one-bank CD font and a two-bank translated font");
		const std::array<std::array<std::size_t, 3>, 4> banks{{
			{0x0000, 0x0000, 0x0800}, {0x0800, 0x1000, 0x0800}, {0x1000, 0x2000, 0x0080}, {0x1080, 0x2100, 0x0080}}};
		for (const auto &bank : banks)
			if (!std::equal(cd.begin() + bank[0], cd.begin() + bank[0] + bank[2], translated.begin() + bank[1]))
				refuse(format, "English bank differs from the CD font");
		return translated;
	}
	case XeenLayerFormat::NewGameContainer:
		return mergeNewGame(cd, translated);
	case XeenLayerFormat::MonsterTable: {
		if (cd.size() != translated.size() || cd.size() % kMonsterBytes)
			refuse(format, "record count differs");
		auto result = cd;
		for (std::size_t record = 0; record < cd.size(); record += kMonsterBytes)
			std::copy_n(translated.begin() + record, kNameBytes, result.begin() + record);
		return result;
	}
	case XeenLayerFormat::MirrorList: {
		// The CD appends records (Bloopers); the translation pairs by index.
		if (cd.size() % kMirrorBytes || translated.size() % kMirrorBytes || translated.size() > cd.size())
			refuse(format, "record count differs");
		auto result = cd;
		for (std::size_t record = 0; record < translated.size(); record += kMirrorBytes) {
			const auto word = translated.begin() + record;
			if (std::find(word, word + kMirrorWordBytes, 0) == word + kMirrorWordBytes)
				refuse(format, "unterminated mirror word");
			std::copy_n(word, kMirrorWordBytes, result.begin() + record);
		}
		return result;
	}
	case XeenLayerFormat::KeptCd:
		return cd;
	}
	refuse(format, "unknown format");
}

std::shared_ptr<const XeenRussianTranslation> XeenRussianTranslation::open(const std::filesystem::path &folder) {
	std::error_code error;
	if (folder.empty() || !std::filesystem::is_directory(folder, error))
		throw XeenTranslationError("Russian translation folder not found: " + folder.u8string());
	const auto lower = [](std::string s) {
		for (auto &c : s)
			if (c >= 'A' && c <= 'Z')
				c = static_cast<char>(c + 32);
		return s;
	};
	std::shared_ptr<XeenRussianTranslation> result(new XeenRussianTranslation());
	result->_folder = folder;
	for (const auto &known : kFiles) {
		std::filesystem::path found;
		for (const auto &entry : std::filesystem::directory_iterator(folder))
			if (lower(entry.path().filename().u8string()) == known.name) {
				if (!found.empty())
					throw XeenTranslationError("Ambiguous Russian translation file: " + entry.path().u8string());
				found = entry.path();
			}
		const auto shown = (found.empty() ? folder / known.name : found).u8string();
		if (found.empty() || !std::filesystem::is_regular_file(found, error))
			throw XeenTranslationError("Russian translation file missing: " + shown);
		const auto input = ReadOnlyDataFile::plain(found).open();
		if (input->size() != known.size)
			throw XeenTranslationError("Unknown Russian translation file (size " + std::to_string(input->size()) +
				"): " + shown);
		std::vector<std::uint8_t> bytes(static_cast<std::size_t>(known.size));
		if (input->read(bytes.data(), bytes.size()) != bytes.size())
			throw XeenTranslationError("Cannot read Russian translation file: " + shown);
		if (static_cast<std::uint32_t>(crc32(crc32(0, nullptr, 0), bytes.data(), static_cast<uInt>(bytes.size()))) != known.crc)
			throw XeenTranslationError("Unknown Russian translation file (checksum): " + shown);
		const std::string name = known.name;
		if (name == "xeen.dat" || name == "xeen.exe") {
			(name == "xeen.exe" ? result->_exe : result->_dat) = found;
			continue;
		}
		const auto role = name == "xeen.cc" ? XeenArchiveRole::Clouds :
			name == "dark.cc" ? XeenArchiveRole::Darkside : XeenArchiveRole::Intro;
		const auto index = ccIndex(bytes.data(), bytes.size());
		auto &entries = result->_entries[role];
		for (const auto &listed : xeenRussianEntries(role)) {
			const auto *entry = findEntry(index, listed.id);
			if (!entry || entry->offset + entry->size > bytes.size())
				throw XeenTranslationError("Russian translation entry missing from " + shown);
			std::vector<std::uint8_t> payload(bytes.begin() + entry->offset, bytes.begin() + entry->offset + entry->size);
			for (auto &b : payload)
				b ^= 0x35;
			entries.emplace(listed.id, std::move(payload));
		}
	}
	return result;
}

const std::vector<std::uint8_t> &XeenRussianTranslation::translatedEntry(XeenArchiveRole role, std::uint16_t id) const {
	const auto archive = _entries.find(role);
	if (archive != _entries.end()) {
		const auto found = archive->second.find(id);
		if (found != archive->second.end())
			return found->second;
	}
	throw std::out_of_range("Entry is not substituted by the Russian translation");
}

std::optional<std::vector<std::uint8_t>> XeenRussianTranslation::substitute(XeenArchiveRole role,
		std::uint16_t id, const std::vector<std::uint8_t> &cd) const {
	for (const auto &listed : xeenRussianEntries(role))
		if (listed.id == id) {
			if (listed.format == XeenLayerFormat::KeptCd)
				return std::nullopt;
			return xeenLayerMerge(listed.format, cd, translatedEntry(role, id));
		}
	return std::nullopt;
}

} // namespace mmodern
