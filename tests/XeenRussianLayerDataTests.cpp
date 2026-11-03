// M110 Part A with the CD and the Russian World of Xeen translation: every
// substituted entry keeps the CD's non-text content, reads through the one
// substitution point, English reads stay the CD's, and raw archive opens are
// refused while Russian is active. Arguments: <cd-game-dir> <russian-dir>.
#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include "XeenTestInstallation.h"
#include "compat/scummvm/ScummVmRuntime.h"
#include "formats/xeen/XeenCutsceneData.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include "platform/XeenSaveFile.h"
#include "tools/XeenTestSave.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenDosText.h"
#include "formats/xeen/XeenEventTextFormat.h"
#include "formats/xeen/XeenFontFormat.h"
#include "formats/xeen/XeenRussianTranslation.h"
#include "games/xeen/XeenTextRenderer.h"

#include "common/archive.h"
#include "common/path.h"
#include "common/stream.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iterator>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;
namespace fs = std::filesystem;

namespace {

using Bytes = std::vector<std::uint8_t>;

void check(bool condition, const std::string &message) {
	if (!condition)
		throw std::runtime_error("FAILED: " + message);
}

// The CD archive's entries, decoded independently of the bridge.
std::map<std::uint16_t, Bytes> cdEntries(const ReadOnlyDataFile &file) {
	auto input = file.open();
	Bytes bytes(static_cast<std::size_t>(input->size()));
	check(input->read(bytes.data(), bytes.size()) == bytes.size(), "CD archive read");
	const std::size_t count = bytes[0] | (std::size_t(bytes[1]) << 8);
	Bytes raw(bytes.begin() + 2, bytes.begin() + 2 + count * 8);
	unsigned seed = 0xac;
	for (auto &value : raw) {
		value = static_cast<std::uint8_t>(((value << 2) | (value >> 6)) + seed);
		seed = (seed + 0x67) & 0xff;
	}
	std::map<std::uint16_t, Bytes> result;
	for (std::size_t i = 0; i < count; ++i) {
		const auto *e = raw.data() + i * 8;
		const std::size_t offset = e[2] | (std::size_t(e[3]) << 8) | (std::size_t(e[4]) << 16), size = e[5] | (e[6] << 8);
		Bytes payload(bytes.begin() + offset, bytes.begin() + offset + size);
		for (auto &b : payload)
			b ^= 0x35;
		result.emplace(static_cast<std::uint16_t>(e[0] | (e[1] << 8)), std::move(payload));
	}
	return result;
}

std::size_t nuls(const Bytes &b) { return static_cast<std::size_t>(std::count(b.begin(), b.end(), 0)); }

// Offsets, within one record, where the merged entry may differ from the CD.
bool differsOnlyIn(const Bytes &merged, const Bytes &cd, std::size_t first, std::size_t record, std::size_t text) {
	if (merged.size() != cd.size())
		return false;
	for (std::size_t i = 0; i < cd.size(); ++i)
		if (merged[i] != cd[i] && (i < first || (i - first) % record >= text))
			return false;
	return true;
}

std::string hexName(std::uint16_t id) {
	char name[5];
	std::snprintf(name, sizeof(name), "%04x", unsigned(id));
	return name;
}

std::size_t rosterOffset(const Bytes &block) {
	const std::size_t count = block[0] | (std::size_t(block[1]) << 8);
	unsigned seed = 0xac;
	Bytes raw(block.begin() + 2, block.begin() + 2 + count * 8);
	for (auto &value : raw) {
		value = static_cast<std::uint8_t>(((value << 2) | (value >> 6)) + seed);
		seed = (seed + 0x67) & 0xff;
	}
	for (std::size_t i = 0; i < count; ++i)
		if ((raw[i * 8] | (raw[i * 8 + 1] << 8)) == 0xd621)
			return raw[i * 8 + 2] | (std::size_t(raw[i * 8 + 3]) << 8) | (std::size_t(raw[i * 8 + 4]) << 16);
	throw std::runtime_error("maze.chr missing from the New Game block");
}

// Decision 3: per substituted entry, its non-text content equals the CD's.
void perEntry(const GameInstallation &english, const GameInstallation &russian, std::ostream &log) {
	const auto &translation = *russian.translation;
	XeenAssetSource en(english, 320, 200), ru(russian, 320, 200);
	std::map<XeenLayerFormat, unsigned> formats;
	unsigned kept = 0, kept_cd_fields = 0;
	for (const auto role : {XeenArchiveRole::Clouds, XeenArchiveRole::Darkside, XeenArchiveRole::Intro}) {
		const auto cd = cdEntries(archiveDataFile(english, role));
		const auto selection = role == XeenArchiveRole::Clouds ? XeenSceneArchive::Clouds :
			role == XeenArchiveRole::Darkside ? XeenSceneArchive::DarksideOnly : XeenSceneArchive::IntroOnly;
		for (const auto &entry : xeenRussianEntries(role)) {
			const std::string label = hexName(entry.id) + " " + entry.name;
			const auto &original = cd.at(entry.id);
			const auto &translated = translation.translatedEntry(role, entry.id);
			const auto substituted = translation.substitute(role, entry.id, original);
			const Bytes merged = substituted.value_or(original);
			++formats[entry.format];
			// The single substitution point: the Russian bridge reads the merged
			// entry and the English bridge the CD's, for every listed id.
			check(ru.readArchiveResource(hexName(entry.id), selection) == merged, "Russian read: " + label);
			check(en.readArchiveResource(hexName(entry.id), selection) == original, "English read: " + label);
			switch (entry.format) {
			case XeenLayerFormat::EventText:
			case XeenLayerFormat::MapName:
			case XeenLayerFormat::StringTable:
				check(merged == translated && nuls(merged) == nuls(original), "text keeps the string count: " + label);
				break;
			case XeenLayerFormat::Picture:
				check(merged == translated && merged[0] >= original[0] && merged[1] == 0 && original[1] == 0,
					"picture keeps the CD frames: " + label);
				break;
			case XeenLayerFormat::RawPicture:
				check(merged == translated && merged.size() == original.size(), "raw picture size: " + label);
				break;
			case XeenLayerFormat::Font:
				check(merged == translated && std::equal(original.begin(), original.begin() + 0x800, merged.begin()) &&
					std::equal(original.begin() + 0x800, original.begin() + 0x1000, merged.begin() + 0x1000), "font bank: " + label);
				break;
			case XeenLayerFormat::NewGameContainer: {
				const auto at = rosterOffset(original);
				check(at == rosterOffset(merged) && differsOnlyIn(merged, original, at, 354, 16) &&
					std::equal(merged.begin() + at, merged.begin() + at + 16, translated.begin() + rosterOffset(translated)),
					"New Game container keeps the CD outside roster names: " + label);
				break;
			}
			case XeenLayerFormat::MonsterTable:
				check(differsOnlyIn(merged, original, 0, 60, 16) && std::equal(merged.begin(), merged.begin() + 16, translated.begin()),
					"monster table keeps CD statistics: " + label);
				break;
			case XeenLayerFormat::MirrorList:
				check(differsOnlyIn(merged, original, 0, 32, 28) && translated.size() < original.size() &&
					std::equal(merged.begin() + translated.size(), merged.end(), original.begin() + translated.size()),
					"mirror list keeps CD destinations and records: " + label);
				break;
			case XeenLayerFormat::KeptCd:
				check(!substituted, "kept entry stays CD: " + label);
				++kept;
				break;
			}
			// Structured merges whose result keeps CD bytes the translation changed.
			kept_cd_fields += entry.format != XeenLayerFormat::KeptCd && merged != translated;
			log << label << ' ' << xeenLayerFormatName(entry.format) << " cd=" << original.size()
				<< " translated=" << translated.size() << " merged=" << merged.size() << '\n';
		}
	}
	check(kept == 5 && formats[XeenLayerFormat::NewGameContainer] == 2 && formats[XeenLayerFormat::MirrorList] == 2 &&
		formats[XeenLayerFormat::MonsterTable] == 2 && formats[XeenLayerFormat::Font] == 3 && kept_cd_fields == 4,
		"format census");
}

// New Game blocks and metadata reads use the substituted bytes.
void newGameAndMetadata(const GameInstallation &english, const GameInstallation &russian) {
	XeenAssetSource en(english, 320, 200), ru(russian, 320, 200);
	for (const auto side : {XeenSide::Clouds, XeenSide::Darkside}) {
		const auto a = en.readInitialResource("maze.chr", side), b = ru.readInitialResource("maze.chr", side);
		check(differsOnlyIn(b, a, 0, 354, 16), "New Game roster keeps CD statistics");
		unsigned named = 0;
		for (std::size_t record = 0; record < b.size(); record += 354)
			named += b[record] >= 0x80;
		check(named == 6, "six translated roster names");
	}
	for (const auto side : {XeenSide::Clouds, XeenSide::Darkside}) {
		const auto a = *en.readMonsterStatistics(side), b = *ru.readMonsterStatistics(side);
		check(a != b && differsOnlyIn(b, a, 0, 60, 16), "monster statistics read through the layer");
	}
}

void searchManRefusal(const GameInstallation &english, const GameInstallation &russian) {
	const Common::Path path("xeen.cc", Common::Path::kNoSeparator);
	{
		ScummVmRuntime runtime(russian);
		bool refused = false;
		try {
			std::unique_ptr<Common::SeekableReadStream> stream(SearchMan.createReadStreamForMember(path));
		} catch (const std::runtime_error &) {
			refused = true;
		}
		check(refused, "raw xeen.cc open while Russian is active");
	}
	ScummVmRuntime runtime(english);
	std::unique_ptr<Common::SeekableReadStream> stream(SearchMan.createReadStreamForMember(path));
	check(stream && stream->size() > 0, "raw xeen.cc open in English");
}

// 8-bit text: the Russian strings carry no command in 0x80-0xff.
void russianText(const GameInstallation &russian) {
	XeenAssetSource ru(russian, 320, 200);
	const XeenFontFormat font(ru.readArchiveResource("fnt", XeenSceneArchive::Clouds), ru.language());
	const XeenTextRenderer renderer(font);
	const auto &translation = *russian.translation;
	unsigned strings = 0;
	for (const auto role : {XeenArchiveRole::Clouds, XeenArchiveRole::Darkside})
		for (const auto &entry : xeenRussianEntries(role)) {
			if (entry.format != XeenLayerFormat::EventText && entry.format != XeenLayerFormat::MapName)
				continue;
			for (const auto &text : XeenEventTextFormat::parse(translation.translatedEntry(role, entry.id))) {
				// With English masking these strings yield thousands of bogus
				// controls; with the Russian font every one parses cleanly.
				std::vector<std::string> diagnostics;
				renderer.textWidth(text, XeenFontSize::Normal, &diagnostics);
				check(diagnostics.empty(), std::string(entry.name) + ": " + (diagnostics.empty() ? "" : diagnostics.front()));
				++strings;
			}
		}
	check(strings > 4000, "Russian Event text strings measured");
	// Every substituted string table through its production reader.
	const auto language = ru.language();
	for (const auto archive : {XeenSceneArchive::Clouds, XeenSceneArchive::DarksideOnly}) {
		check(xeenSpellDescriptions(ru.readArchiveResource("spldesc.bin", archive), language).size() >= 76, "spldesc.bin");
		const auto tavern = ru.readArchiveResource("tavern.bin", archive);
		const auto count = static_cast<std::size_t>(std::count(tavern.begin(), tavern.end(), 0));
		check(count == (archive == XeenSceneArchive::Clouds ? 78u : 63u), "tavern.bin strings");
		for (std::size_t i = 0; i < count; ++i)
			ru.tavernString(i, archive);
	}
	// The journal reads both resources from DARK.CC only (Application, loadJournalText).
	check(xeenJournalStrings(ru.readArchiveResource("award.bin", XeenSceneArchive::DarksideOnly), true, language).size() >= 88 &&
		xeenJournalStrings(ru.readArchiveResource("qnotes.bin", XeenSceneArchive::DarksideOnly), false, language).size() >= 184,
		"journal text");
	for (const auto archive : {XeenSceneArchive::Clouds, XeenSceneArchive::DarksideOnly, XeenSceneArchive::IntroOnly}) {
		const auto bytes = ru.readArchiveResource("special.bin", archive);
		check(!XeenCutsceneData::captions(bytes, language).records.empty() &&
			!XeenCutsceneData::firstCaption(bytes, language).empty(), "special.bin captions");
		bool refused = false;
		try {
			XeenCutsceneData::captions(bytes);
		} catch (const std::exception &) {
			refused = true;
		}
		check(refused, "English masking would refuse the Russian captions");
	}
	check(XeenLearnedSpellNames::parse(*ru.readLearnedSpellNamesFromDarkArchive()).names.size() == 77, "spells.xen");
	// The Vertigo texts draw with the two-bank font and differ from English.
	XeenTextRenderOptions options;
	options.bounds = {9, 9, 222, 140};
	options.originalControls = true;
	const auto vertigo = XeenEventTextFormat::parse(ru.readArchiveResource("aaze0028.txt", XeenSceneArchive::Clouds));
	const auto page = renderer.render(IndexedFrame(320, 200, Bytes(64000, 0)), vertigo.at(0), options);
	check(page.diagnostics.empty() || page.diagnostics.front().find("clipped") != std::string::npos, "Vertigo Russian text renders");
}

fs::path linkTranslation(const fs::path &source, const fs::path &target, const std::string &omit, const std::string &fake) {
	fs::remove_all(target);
	fs::create_directories(target);
	for (const char *name : {"xeen.cc", "dark.cc", "intro.cc", "xeen.dat", "xeen.exe"}) {
		if (name == omit)
			continue;
		if (name == fake) {
			// Same size as the identified file, other content: the checksum refuses it.
			std::ofstream(target / name, std::ios::binary) << std::string(fs::file_size(source / name), 'x');
			continue;
		}
		std::error_code linked;
		fs::create_hard_link(source / name, target / name, linked);
		if (linked)
			fs::copy_file(source / name, target / name);
	}
	return target;
}

// Decision 10: English displays draw Russian roster names literally, with no
// extra lines and no command; Russian displays draw them as Cyrillic.
void displays(const GameInstallation &english, const GameInstallation &russian) {
	XeenAssetSource en(english, 320, 200), ru(russian, 320, 200);
	const XeenFontFormat englishFont(en.readArchiveResource("fnt")), russianFont(ru.readArchiveResource("fnt"), ru.language());
	const auto &text = en.uiText();
	const auto englishParty = XeenPartyLoader().loadInitialParty(en), russianParty = XeenPartyLoader().loadInitialParty(ru);
	const IndexedFrame base(320, 200, Bytes(64000, 0));
	const XeenDialogSpriteDraw draw = [&](IndexedFrame &frame, const char *name, unsigned index, int x, int y) {
		en.drawDialogSprite(frame, name, index, x, y);
	};
	const auto lines = [&](const XeenFontFormat &font, const std::string &value) {
		XeenTextRenderOptions options;
		options.bounds = {0, 0, 320, 200};
		options.originalControls = true;
		return XeenTextRenderer(font).render(base, value, options).writeY;
	};
	std::vector<std::size_t> members;
	for (std::size_t i = 0; i < russianParty.party.size(); ++i)
		members.push_back(i);
	check(static_cast<unsigned char>(std::string(russianParty.party.member(russianParty.roster, 0).name)[0]) >= 0x80,
		"Russian New Game roster");
	check(lines(englishFont, "\1" + xeenQuickReferenceText(text, russianParty, members)) ==
		lines(englishFont, "\1" + xeenQuickReferenceText(text, englishParty, members)), "English Quick Reference keeps its lines");
	drawXeenQuickReference(text, base, englishFont, russianParty, members);
	drawXeenQuickReference(text, base, russianFont, russianParty, members);
	std::vector<std::string> awards(88, "award");
	for (std::size_t i = 0; i < members.size(); ++i) {
		drawXeenSheet(text, base, englishFont, russianParty, i, 0, false, draw);
		drawXeenSheet(text, base, russianFont, russianParty, i, 0, false, draw);
		XeenJournalUi ui;
		ui.page = XeenJournalPage::Awards;
		const auto &member = russianParty.party.member(russianParty.roster, i);
		check(lines(englishFont, xeenJournalText(text, ui, awards, &member)) ==
			lines(englishFont, xeenJournalText(text, ui, awards, &englishParty.party.member(englishParty.roster, i))),
			"English Awards header keeps its lines");
	}
}

// Saves: a Russian New Game save differs from the English one only in the
// roster names, round-trips exactly and is found by either language.
void saves(const GameInstallation &english, const GameInstallation &russian) {
	const auto root = fs::temp_directory_path() / ("mmodern-russian-layer-saves-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	fs::remove_all(root);
	fs::create_directories(root);
	developer::generateTestSave(english, "m102-playtest", root / "english.mmsave");
	developer::generateTestSave(russian, "m102-playtest", root / "russian.mmsave");
	const auto read = [](const fs::path &path) {
		std::ifstream input(path, std::ios::binary);
		return Bytes(std::istreambuf_iterator<char>(input), {});
	};
	const auto englishBytes = read(root / "english.mmsave"), russianBytes = read(root / "russian.mmsave");
	auto englishSave = XeenSaveFormat::decode(englishBytes), russianSave = XeenSaveFormat::decode(russianBytes);
	check(XeenSaveFormat::encode(russianSave) == russianBytes, "Russian-name save round trip");
	unsigned names = 0;
	for (std::size_t i = 0; i < russianSave.characters.size(); ++i)
		if (russianSave.characters[i].name != englishSave.characters[i].name) {
			russianSave.characters[i].name = englishSave.characters[i].name;
			++names;
		}
	check(names == 6 && XeenSaveFormat::encode(russianSave) == XeenSaveFormat::encode(englishSave),
		"Russian and English saves differ only in roster names");
	const auto signature = XeenSaveFile::fingerprint(english);
	check(XeenSaveFile::dataId(signature) == XeenSaveFile::dataId(XeenSaveFile::fingerprint(russian)), "one save folder");
	// The saves carry the raw CD signature, which both languages compute.
	for (const char *name : {"english.mmsave", "russian.mmsave"})
		check(XeenSaveFile::read(root / name).resources == signature &&
			XeenSaveFile::read(root / name).resources == XeenSaveFile::fingerprint(russian),
			std::string("save readable in either language: ") + name);
	fs::remove_all(root);
}

// Decision 2: an incomplete or unknown translation is refused naming the file.
void refusals(const fs::path &source) {
	const auto root = fs::absolute("russian-layer-refusals");
	for (const auto &[omit, fake] : {std::pair<std::string, std::string>{"dark.cc", ""}, {"", "xeen.exe"}}) {
		const auto folder = linkTranslation(source, root, omit, fake);
		std::string message;
		try {
			XeenRussianTranslation::open(folder);
		} catch (const XeenTranslationError &e) {
			message = e.what();
		}
		check(message.find(omit.empty() ? fake : omit) != std::string::npos &&
			(omit.empty() ? message.find("checksum") != std::string::npos : true), "refusal names the file: " + message);
	}
	fs::remove_all(root);
}

} // namespace

int main(int argc, char **argv) {
	try {
		if (argc != 3)
			throw std::runtime_error("Usage: mmodern_russian_layer_data_tests <cd-game-dir> <russian-dir>");
		const auto english = xeenTestInstallationDetector().detect(fs::u8path(argv[1]));
		check(english.has_value() && !english->translation, "CD installation");
		auto russian = *english;
		russian.translation = XeenRussianTranslation::open(fs::u8path(argv[2]));
		check(xeenLanguage(russian) == XeenLanguage::Russian && xeenLanguage(*english) == XeenLanguage::English, "language");
		std::ofstream log("russian-layer-entries.log");
		perEntry(*english, russian, log);
		newGameAndMetadata(*english, russian);
		searchManRefusal(*english, russian);
		russianText(russian);
		displays(*english, russian);
		saves(*english, russian);
		refusals(fs::u8path(argv[2]));
		std::cout << "Russian layer: per-entry structure, substitution point, English reads, readers, New Game, metadata, SearchMan, displays, saves and refusals passed\n";
		return 0;
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return 1;
	}
}
