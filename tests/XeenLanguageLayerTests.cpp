// M110 Part A: the Russian language layer without original data. Merge rules,
// the fixed entry lists, the two-bank font, 8-bit text, literal names, the
// language setting and the refusal of folders that are not the translation.
#include "formats/xeen/XeenDosText.h"
#include "formats/xeen/XeenFontFormat.h"
#include "formats/xeen/XeenRussianTranslation.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenTextRenderer.h"
#include "platform/XeenWonStore.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;
namespace fs = std::filesystem;

namespace {

void check(bool condition, const std::string &message) {
	if (!condition)
		throw std::runtime_error("FAILED: " + message);
}

template<typename F>
std::string failure(F &&f) {
	try {
		f();
	} catch (const std::exception &e) {
		return e.what();
	}
	return {};
}

using Bytes = std::vector<std::uint8_t>;

Bytes strings(std::initializer_list<const char *> values) {
	Bytes result;
	for (const char *value : values) {
		result.insert(result.end(), value, value + std::char_traits<char>::length(value));
		result.push_back(0);
	}
	return result;
}

// A CC index as BaseCCArchive::loadIndex reads it (inverse of its decoding).
Bytes ccContainer(const std::vector<std::pair<std::uint16_t, Bytes>> &members) {
	std::vector<std::uint8_t> plain;
	std::size_t offset = 2 + members.size() * 8;
	for (const auto &[id, data] : members) {
		const std::uint8_t entry[8] = {std::uint8_t(id), std::uint8_t(id >> 8), std::uint8_t(offset),
			std::uint8_t(offset >> 8), std::uint8_t(offset >> 16), std::uint8_t(data.size()),
			std::uint8_t(data.size() >> 8), 0};
		plain.insert(plain.end(), entry, entry + 8);
		offset += data.size();
	}
	Bytes result{std::uint8_t(members.size()), std::uint8_t(members.size() >> 8)};
	unsigned seed = 0xac;
	for (const auto value : plain) {
		const std::uint8_t shifted = std::uint8_t(value - seed);
		result.push_back(std::uint8_t((shifted >> 2) | (shifted << 6)));
		seed = (seed + 0x67) & 0xff;
	}
	for (const auto &member : members)
		result.insert(result.end(), member.second.begin(), member.second.end());
	return result;
}

Bytes sprite(unsigned frames, std::size_t size) {
	Bytes result(size, 0x11);
	result[0] = std::uint8_t(frames);
	result[1] = 0;
	for (unsigned i = 0; i < frames * 2; ++i) {
		result[2 + i * 2] = std::uint8_t(2 + frames * 4);
		result[3 + i * 2] = 0;
	}
	return result;
}

void mergeRules() {
	using F = XeenLayerFormat;
	for (const auto format : {F::EventText, F::MapName, F::StringTable}) {
		const auto cd = strings({"One", "Two"}), ru = strings({"\x8e\xa4\xa8\xad", "\x84\xa2\xa0"});
		check(xeenLayerMerge(format, cd, ru) == ru, "text takes the translated strings");
		check(!failure([&] { xeenLayerMerge(format, cd, strings({"One"})); }).empty(),
			"text with another string count stays refused");
	}
	check(xeenLayerMerge(F::Picture, sprite(4, 64), sprite(5, 80)) == sprite(5, 80), "picture keeps extra frames");
	check(!failure([&] { xeenLayerMerge(F::Picture, sprite(4, 64), sprite(3, 64)); }).empty(),
		"picture with fewer frames than the CD is refused");
	check(!failure([&] { xeenLayerMerge(F::RawPicture, Bytes(64000), Bytes(63999)); }).empty(), "raw size");

	Bytes font(0x1100), twoBanks(0x2200);
	for (std::size_t i = 0; i < font.size(); ++i)
		font[i] = std::uint8_t(i * 7);
	std::copy_n(font.begin(), 0x800, twoBanks.begin());
	std::copy_n(font.begin() + 0x800, 0x800, twoBanks.begin() + 0x1000);
	std::copy_n(font.begin() + 0x1000, 0x80, twoBanks.begin() + 0x2000);
	std::copy_n(font.begin() + 0x1080, 0x80, twoBanks.begin() + 0x2100);
	check(xeenLayerMerge(F::Font, font, twoBanks) == twoBanks, "two-bank font over the CD font");
	auto changed = twoBanks;
	changed[0x2101] ^= 1;
	check(!failure([&] { xeenLayerMerge(F::Font, font, changed); }).empty(), "font whose English widths differ");

	// Monster records: the 16-byte name only.
	Bytes cdMonsters(120), ruMonsters(120);
	for (std::size_t i = 0; i < 120; ++i) {
		cdMonsters[i] = std::uint8_t(i);
		ruMonsters[i] = std::uint8_t(200 - i);
	}
	const auto monsters = xeenLayerMerge(F::MonsterTable, cdMonsters, ruMonsters);
	for (std::size_t i = 0; i < 120; ++i)
		check(monsters[i] == (i % 60 < 16 ? ruMonsters[i] : cdMonsters[i]), "monster merge keeps CD statistics");
	check(!failure([&] { xeenLayerMerge(F::MonsterTable, cdMonsters, Bytes(60)); }).empty(), "monster count");

	// Mirrors: words by index, CD destinations, the CD's extra record kept.
	Bytes cdMirrors(96, 0), ruMirrors(64, 0);
	const auto word = [](Bytes &table, unsigned record, const std::string &text, std::uint8_t map) {
		std::copy(text.begin(), text.end(), table.begin() + record * 32);
		table[record * 32 + 28] = map;
	};
	word(cdMirrors, 0, "vertigo", 28);
	word(cdMirrors, 1, "dragon tower", 5);
	word(cdMirrors, 2, "bloopers", 0);
	word(ruMirrors, 0, "\x82\xa5\xe0", 28);
	word(ruMirrors, 1, "\x84\xe0\xa0", 13);
	const auto mirrors = xeenLayerMerge(F::MirrorList, cdMirrors, ruMirrors);
	check(std::equal(mirrors.begin(), mirrors.begin() + 28, ruMirrors.begin()) && mirrors[28] == 28 &&
		std::equal(mirrors.begin() + 32, mirrors.begin() + 60, ruMirrors.begin() + 32) && mirrors[60] == 5 &&
		std::equal(mirrors.begin() + 64, mirrors.end(), cdMirrors.begin() + 64), "mirror merge");
	auto unterminated = ruMirrors;
	std::fill_n(unterminated.begin(), 28, 'a');
	check(!failure([&] { xeenLayerMerge(F::MirrorList, cdMirrors, unterminated); }).empty(), "unterminated word");

	// New Game container: CD block, roster names from the translation.
	Bytes cdRoster(354 * 2), ruRoster(354 * 2);
	for (std::size_t i = 0; i < cdRoster.size(); ++i) {
		cdRoster[i] = std::uint8_t(i);
		ruRoster[i] = std::uint8_t(i + 99);
	}
	const auto cdBlock = ccContainer({{0x1234, Bytes(10, 1)}, {0xd621, cdRoster}});
	const auto ruBlock = ccContainer({{0x1234, Bytes(10, 2)}, {0xd621, ruRoster}});
	const auto merged = xeenLayerMerge(F::NewGameContainer, cdBlock, ruBlock);
	check(merged.size() == cdBlock.size(), "New Game block keeps the CD size");
	const std::size_t roster = 2 + 2 * 8 + 10;
	for (std::size_t i = 0; i < merged.size(); ++i) {
		const bool name = i >= roster && (i - roster) % 354 < 16;
		check(merged[i] == (name ? ruBlock[i] : cdBlock[i]), "New Game merge takes only roster names");
	}
	check(!failure([&] { xeenLayerMerge(F::NewGameContainer, cdBlock, ccContainer({{0x1234, Bytes(4)}})); }).empty(),
		"New Game block without maze.chr");
	check(xeenLayerMerge(F::KeptCd, cdBlock, ruBlock) == cdBlock, "kept CD entry");
}

void entryLists() {
	const std::map<XeenArchiveRole, std::pair<std::size_t, std::size_t>> expected{
		{XeenArchiveRole::Clouds, {105, 1}}, {XeenArchiveRole::Darkside, {379, 3}}, {XeenArchiveRole::Intro, {7, 0}}};
	for (const auto &[role, counts] : expected) {
		const auto &entries = xeenRussianEntries(role);
		std::set<std::uint16_t> ids;
		std::size_t kept = 0, fonts = 0;
		for (const auto &entry : entries) {
			check(ids.insert(entry.id).second, "duplicate substituted id");
			check(std::string(xeenLayerFormatName(entry.format)) != "unknown", "every entry has a format");
			kept += entry.format == XeenLayerFormat::KeptCd;
			fonts += entry.format == XeenLayerFormat::Font;
			check(!std::string(entry.name).empty() || entry.format == XeenLayerFormat::KeptCd,
				"an entry no reader names stays on the CD");
		}
		check(entries.size() == counts.first && kept == counts.second && fonts == 1, "entry list counts");
	}
}

Bytes testFont(bool twoBanks) {
	Bytes bytes(twoBanks ? 0x2200 : 0x1100);
	const std::size_t widths = twoBanks ? 0x2000 : 0x1000;
	for (unsigned c = 0; c < 128; ++c) {
		for (unsigned row = 0; row < 8; ++row) {
			bytes[c * 16 + row * 2] = std::uint8_t(c | 1);
			if (twoBanks)
				bytes[0x800 + c * 16 + row * 2] = 0x55;
		}
		bytes[widths + c] = 5;
		bytes[widths + 0x100 - (twoBanks ? 0 : 0x80) + c] = 4;
		if (twoBanks) {
			bytes[0x2080 + c] = 7;
			bytes[0x2180 + c] = 6;
		}
	}
	return bytes;
}

XeenTextRenderResult draw(const XeenFontFormat &font, const std::string &text) {
	XeenTextRenderOptions options;
	options.bounds = {0, 0, 320, 200};
	options.originalControls = true;
	return XeenTextRenderer(font).render(IndexedFrame(320, 200, Bytes(64000, 0)), text, options);
}

void fontAndText() {
	const XeenFontFormat english(testFont(false)), russian(testFont(true), XeenLanguage::Russian);
	check(!failure([] { XeenFontFormat(testFont(false), XeenLanguage::Russian); }).empty(),
		"Russian text needs the two-bank font");
	// English: every byte is masked to 7 bits, as before M110.
	for (unsigned c = 0; c < 256; ++c) {
		check(english.advance(std::uint8_t(c), XeenFontSize::Normal) == english.advance(std::uint8_t(c & 0x7f), XeenFontSize::Normal) &&
			english.glyph(std::uint8_t(c), XeenFontSize::Reduced).pixels == english.glyph(std::uint8_t(c & 0x7f), XeenFontSize::Reduced).pixels,
			"English masks to 7 bits");
	}
	// Russian: 0x80-0xff from the second bank; only 0x20 is a space.
	check(russian.advance(0xa0, XeenFontSize::Normal) == 7 && russian.advance(0xa0, XeenFontSize::Reduced) == 6 &&
		russian.advance(' ', XeenFontSize::Normal) == 4 && russian.advance('A', XeenFontSize::Normal) == 5,
		"second-bank widths");
	check(russian.glyph(0x8a, XeenFontSize::Normal).pixels == russian.glyph(0xff, XeenFontSize::Normal).pixels &&
		russian.glyph(0x8a, XeenFontSize::Normal).pixels != russian.glyph(0x0a, XeenFontSize::Normal).pixels &&
		russian.glyph('A', XeenFontSize::Normal).pixels == english.glyph('A', XeenFontSize::Normal).pixels,
		"second-bank glyphs");
	// 0x8a is a newline in English and the letter Ka in Russian.
	const auto en = draw(english, "A\x8a" "B"), ru = draw(russian, "A\x8a" "B");
	check(en.writeY == 10 && en.writeX == 5, "English 0x8a still breaks the line");
	check(ru.writeY == 0 && ru.writeX == 17, "Russian 0x8a is a character");
	check(XeenTextRenderer(english).textWidth("\xa0\xa0", XeenFontSize::Normal) == 8 &&
		XeenTextRenderer(russian).textWidth("\xa0\xa0", XeenFontSize::Normal) == 14,
		"0xa0 is a space only in English");
	// Controls: English 0x83 is an alignment, Russian 0x83 the letter Ge.
	check(!failure([] { XeenDosText::validateControls("\x83x"); }).empty() &&
		failure([] { XeenDosText::validateControls("\x83" "c"); }).empty() &&
		failure([] { XeenDosText::validateControls("\x83x\x8c", XeenLanguage::Russian); }).empty() &&
		!failure([] { XeenDosText::validateControls("\x0c" "x", XeenLanguage::Russian); }).empty(),
		"validateControls follows the language");
	std::vector<std::string> diagnostics;
	XeenTextRenderer(russian).textWidth("\x83\x8c\x80", XeenFontSize::Normal, &diagnostics);
	check(diagnostics.empty(), "Russian flowing text has no controls in 0x80-0xff");
}

void literalNames() {
	check(xeenLiteralText("Crag Hack") == "Crag Hack" && xeenLiteralText("Mo~") == "Mo~", "ASCII names are unchanged");
	const auto ka = xeenLiteralText("\x8a" "B");
	check(ka == std::string("\x1f\x1f\x8a" "B") && xeenVisibleLength(ka) == 2 && xeenVisibleLength("x\x1f" "a") == 3,
		"marked bytes count once");
	check(xeenDialogFormat("[%4s]", {ka}) == std::string("[  \x1f\x1f\x8a" "B]"), "padding counts shown characters");
	const XeenFontFormat english(testFont(false)), russian(testFont(true), XeenLanguage::Russian);
	// English: a name byte draws the glyph of its low 7 bits, never a command.
	const auto literal = draw(english, "A" + ka);
	check(literal.writeY == 0 && literal.writeX == 15, "English literal 0x8a is not a newline");
	check(draw(english, xeenLiteralText("\xc1")).pages[0].pixels == draw(english, "A").pages[0].pixels,
		"English 0xc1 draws A");
	check(draw(english, "X" + xeenLiteralText("\x83" "c")).writeX == 15, "English literal 0x83 is not an alignment");
	// Russian: a name byte is its own Cyrillic glyph.
	check(draw(russian, "A" + ka).pages[0].pixels == draw(russian, "A\x8a" "B").pages[0].pixels,
		"Russian literal draws the Cyrillic glyph");
	std::vector<std::string> diagnostics;
	check(XeenTextRenderer(english).textWidth(ka, XeenFontSize::Normal, &diagnostics) == 10 && diagnostics.empty(),
		"flowing renderer draws literal bytes");
	XeenTextRenderer(english).textWidth("\x1f" "garbage", XeenFontSize::Normal, &diagnostics);
	check(diagnostics.size() == 1, "a single 0x1f stays an unsupported control");
	// Wrapping never breaks at a literal byte whose low 7 bits are a space.
	XeenTextRenderOptions narrow;
	narrow.bounds = {0, 0, 22, 200};
	narrow.originalControls = true;
	const auto wrapped = XeenTextRenderer(english).render(IndexedFrame(320, 200, Bytes(64000, 0)),
		"AA" + xeenLiteralText("\xa0") + "AAA", narrow);
	check(wrapped.writeY == 10 && wrapped.writeX == 10, "a literal space is not a break opportunity");
}

std::string fileBytes(const fs::path &path) {
	std::ifstream input(path, std::ios::binary);
	return std::string(std::istreambuf_iterator<char>(input), {});
}

void settingsRoundTrip() {
	const auto path = fs::absolute("language-layer-settings.ini");
	const auto folder = fs::absolute("translation folder");
	fs::remove(path);
	const auto write = [&](const std::string &text) {
		std::ofstream(path, std::ios::binary) << text;
	};
	write("won=1\ndarkside_intro_seen=0\ndarkside_won=1\nworld_won=0\n");
	auto s = xeenReadSettings(path);
	check(s.won && s.darksideWon && s.language == XeenLanguage::English && s.translation.empty(), "four-key file");
	check(xeenWriteLanguage(path, XeenLanguage::Russian, folder), "language publication");
	check(fileBytes(path) == "won=1\ndarkside_intro_seen=0\ndarkside_won=1\nworld_won=0\nlanguage=russian\nrussian_translation=" +
		folder.u8string() + "\n", "complete six-key file");
	s = xeenReadSettings(path);
	check(s.won && s.darksideWon && s.language == XeenLanguage::Russian && s.translation == folder, "language read back");
	check(xeenWriteWon(path, XeenWonKind::World) && xeenReadSettings(path).language == XeenLanguage::Russian &&
		xeenReadSettings(path).translation == folder, "winning keeps the language");
	check(xeenWriteLanguage(path, XeenLanguage::English, folder) && xeenReadSettings(path).language == XeenLanguage::English &&
		xeenReadSettings(path).translation == folder, "English keeps the folder");
	check(!xeenWriteLanguage(path, XeenLanguage::Russian, {}) && !xeenWriteLanguage(path, XeenLanguage::English, "relative") &&
		xeenReadSettings(path).translation == folder, "invalid choices are not written");
	const std::string head = "won=0\ndarkside_intro_seen=0\ndarkside_won=0\nworld_won=0\n";
	for (const std::string &bad : {head + "language=french\nrussian_translation=\n",
			head + "language=russian\nrussian_translation=\n", head + "language=english\nrussian_translation=relative\n",
			head + "language=english\nlanguage=english\nrussian_translation=\n", head + "language=english\n",
			head + "russian_translation=\n"}) {
		write(bad);
		s = xeenReadSettings(path);
		check(!s.won && s.language == XeenLanguage::English && s.translation.empty(), "strict parser refuses: " + bad);
	}
	write(head + "language=english\nrussian_translation=\n");
	check(xeenReadSettings(path).language == XeenLanguage::English, "complete English file");
	fs::remove(path);
}

void refusals() {
	const auto root = fs::absolute("language-layer-translation");
	fs::remove_all(root);
	auto message = failure([&] { XeenRussianTranslation::open(root); });
	check(message.find("folder not found") != std::string::npos, "missing folder: " + message);
	fs::create_directories(root);
	message = failure([&] { XeenRussianTranslation::open(root); });
	check(message.find("missing") != std::string::npos && message.find("xeen.cc") != std::string::npos,
		"partial translation names the file: " + message);
	std::ofstream(root / "XEEN.CC", std::ios::binary) << "not the translation";
	message = failure([&] { XeenRussianTranslation::open(root); });
	check(message.find("Unknown Russian translation file") != std::string::npos && message.find("XEEN.CC") != std::string::npos,
		"unknown file names the file: " + message);
	fs::remove_all(root);
}

} // namespace

int main() {
	try {
		mergeRules();
		entryLists();
		fontAndText();
		literalNames();
		settingsRoundTrip();
		refusals();
		std::cout << "Language layer merges, entry lists, two-bank font, 8-bit text, literal names, settings and refusals passed\n";
		return 0;
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return 1;
	}
}
