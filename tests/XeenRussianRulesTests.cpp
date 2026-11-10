// M110 Part C: Russian rules and input. Without arguments, the rules on
// synthetic data: the Russian Shift rule, key sets, getString entry, the
// material parser, the Russian item order, literal glyphs in the flowing
// renderer (N1) and the entrance caption. With <cd-game-dir> <russian-dir>
// [pictures-dir], the key sets, combat rows, materials, item descriptions and
// caption strip read from the identified programs and archives; with a
// pictures directory it also writes the Russian combat rows and the caption
// frames as PPM, for inspection.
#include "XeenTestInstallation.h"
#include "app/XeenAudio.h"
#include "app/XeenCutsceneFlow.h"
#include "core/XeenKeyboard.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCutsceneData.h"
#include "formats/xeen/XeenDosText.h"
#include "formats/xeen/XeenFontFormat.h"
#include "formats/xeen/XeenMaterialNames.h"
#include "formats/xeen/XeenRussianTranslation.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenEntranceScene.h"
#include "games/xeen/XeenItemCatalog.h"
#include "games/xeen/XeenTextInput.h"
#include "games/xeen/XeenTextRenderer.h"
#include "platform/sdl/XeenControllerInput.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include "platform/sdl/XeenRussianKeys.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
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

Bytes read(const fs::path &path) {
	std::ifstream in(path, std::ios::binary);
	return Bytes(std::istreambuf_iterator<char>(in), {});
}

void picture(const fs::path &path, const IndexedFrame &frame) {
	std::ofstream out(path, std::ios::binary);
	out << "P6\n" << frame.width << ' ' << frame.height << "\n255\n";
	for (const auto pixel : frame.pixels)
		out.write(reinterpret_cast<const char *>(frame.palette.data() + 3 * pixel), 3);
}

// Every glyph 5 wide (4 reduced, 7/6 in the second bank), as the language
// layer tests use.
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

// RU 0x43ee5..0x43f06: Shift uppercases cp866 letters only.
void shiftRule() {
	for (unsigned c = 0; c < 256; ++c) {
		const auto shifted = xeenRussianShift(c);
		if (c >= 0xa0 && c <= 0xaf)
			check(shifted == c - 0x20, "Shift uppercases a-p");
		else if (c >= 0xe0 && c <= 0xef)
			check(shifted == c - 0x50, "Shift uppercases r-ya");
		else
			check(shifted == c, "Shift keeps digits, symbols, box drawing and yo");
	}
}

// Physical positions (USB usages, as SDL scancodes) to the Russian codes.
void positions() {
	XeenKeyboard k;
	k.letters[0x10] = 0xa9;k.letters[0x23] = 0xe0;k.letters[0x1a] = 0xe5;k.letters[0x33] = 0xa1;k.letters[0x27] = 0xa6;
	constexpr unsigned Q = 20, H = 11, leftBracket = 47, comma = 54, semicolon = 51, one = 30, minus = 45, slash = 56, grave = 53, A = 4;
	check(xeenRussianKeyCode(k, Q, false, false) == 0xa9 && xeenRussianKeyCode(k, H, false, false) == 0xe0, "Q types й, H types р (Ins's code)");
	check(xeenRussianKeyCode(k, Q, true, false) == 0x89 && xeenRussianKeyCode(k, H, true, true) == 0x90, "Shift capitals, before Ctrl");
	check(xeenRussianKeyCode(k, one, true, false) == '1' && xeenRussianKeyCode(k, one, false, true) == '!', "Shift+1 is 1, Ctrl+1 is !");
	check(xeenRussianKeyCode(k, leftBracket, false, true) == '{' && xeenRussianKeyCode(k, comma, false, true) == '<' &&
		xeenRussianKeyCode(k, semicolon, false, true) == ':', "Ctrl symbols on the Cyrillic keys");
	check(xeenRussianKeyCode(k, Q, false, true) == 0xa9 && xeenRussianKeyCode(k, minus, false, true) == '-' &&
		xeenRussianKeyCode(k, slash, false, true) == '?' && xeenRussianKeyCode(k, grave, false, false) == '`', "other Ctrl keys");
	check(xeenRussianKeyCode(k, A, false, false) == 'A', "an untranslated row keeps the CD code");
	check(!xeenRussianKeyCode(k, 58, false, false) && !xeenRussianKeyCode(k, 41, false, false) && !xeenRussianKeyCode(k, 80, false, false),
		"function keys, Escape and arrows keep their CD codes");
}

void keySets() {
	XeenKeyboard keyboard;
	keyboard.sets["TAVERN"].aliases = {{0xae, 's'}, {0xa2, 'd'}};
	keyboard.sets["QUICK_FIGHT"].clicks = {{'n', 0xe3}};
	check(keyboard.key("TAVERN", 0xae) == 's' && keyboard.key("TAVERN", 0xa2) == 'd', "aliases map program keys");
	check(keyboard.key("TAVERN", 0xe3) == 0xe3 && keyboard.key("TAVERN", 's') == 's', "other codes pass through");
	check(keyboard.key("NONE", 0xae) == 0xae, "unknown sets pass through");
	check(keyboard.click("QUICK_FIGHT", 'n') == 0xe3 && keyboard.click("QUICK_FIGHT", 27) == 27,
		"a dead button delivers its Russian value");
	// The CD combat target rows, unless the Russian program gives its own.
	const auto cdRow = [](int y) {
		const auto action = xeenMainScreenClick(240, y, MainScreen::Combat);
		return action && std::holds_alternative<SelectInventorySlotAction>(*action);
	};
	check(cdRow(27) && !cdRow(26) && cdRow(56) && !cdRow(57), "CD rows at y 27/37/47, 10 high");
	keyboard.targetTop = 10;keyboard.targetStride = 18;keyboard.targetHeight = 18;
	const auto row = [&](int y) -> std::optional<unsigned> {
		const auto action = xeenMainScreenClick(240, y, MainScreen::Combat, &keyboard);
		const auto *slot = action ? std::get_if<SelectInventorySlotAction>(&*action) : nullptr;
		return slot ? std::optional<unsigned>(unsigned(slot->slot)) : std::nullopt;
	};
	check(row(10) == 0u && row(27) == 0u && row(28) == 1u && row(45) == 1u && row(46) == 2u && row(63) == 2u && !row(9),
		"Russian rows at y 10/28/46, 18 high");
}

// The shared getString: 0x20-0x7f on the CD, 0x20-0xff in Russian, word
// capitalization on ASCII letters only.
void textEntry() {
	const XeenFontFormat english(testFont(false)), russian(testFont(true), XeenLanguage::Russian);
	XeenTextInput input;
	check(!input.type("\xa0", english) && input.value().empty(), "English entry refuses 0x80-0xff");
	check(input.type("\x88\xa3\xe0\xa0 \xf1\xc9\xd2\xff", russian) && input.value() == "\x88\xa3\xe0\xa0 \xf1\xc9\xd2\xff",
		"Russian entry keeps 0x20-0xff");
	check(!input.type("\x1f", russian), "controls stay refused");
	input.clear();
	check(input.type(" ", russian) == false && input.value().empty(), "a leading space is ignored");
	check(input.type("\xa0\xa1 \xe0x ab", russian, 20, true) && input.value() == "\xa0\xa1 \xe0x Ab",
		"word case never changes Cyrillic");
	input.clear();
	check(input.type(std::string(20, '\xa0'), russian) && input.type("\xa1", russian, 20) && input.value() == std::string(20, '\xa0'),
		"twenty bytes at most");
}

Bytes materialBytes(bool russian) {
	Bytes bytes;
	if (russian)
		bytes.push_back(' ');
	bytes.push_back(0);
	for (unsigned i = 1; i <= 130; ++i) {
		const std::string name = russian ? " \xa8\xa7 " + std::to_string(i) + " " : "metal" + std::to_string(i) + " ";
		bytes.insert(bytes.end(), name.begin(), name.end());
		bytes.push_back(0);
	}
	return bytes;
}

void materials() {
	const auto english = parseXeenMaterialNames(materialBytes(false));
	check(english.ready() && english.names[0].empty() && english.names[1] == "metal1", "English materials");
	check(!parseXeenMaterialNames(materialBytes(true)).ready(), "English refuses entry zero ' '");
	const auto russian = parseXeenMaterialNames(materialBytes(true), XeenLanguage::Russian);
	check(russian.ready() && russian.names[0].empty() && russian.names[7] == "\xa8\xa7 7", "Russian materials keep cp866");
	auto bad = materialBytes(true);
	bad.insert(bad.begin(), 'x');
	check(!parseXeenMaterialNames(bad, XeenLanguage::Russian).ready(), "entry zero is one space");
	auto english8 = materialBytes(false);
	english8[1] = 0xa0;
	check(parseXeenMaterialNames(english8).names[1].front() == '?', "English shows ASCII only");
}

// Weapons, armor and accessories: name, broken, cursed, then material and
// suffix; miscellaneous items keep the CD order; "of" is a space.
void items() {
	auto catalog = loadXeenItemCatalogWithMaterials(parseXeenMaterialNames(materialBytes(true), XeenLanguage::Russian));
	auto names = std::make_shared<XeenItemNames>();
	names->weapons.assign(41, "\x8c\xa5\xe7");
	names->armor.assign(14, "\x81\xe0\xae\xad\xef");
	names->accessories.assign(11, "\x8a\xae\xab\xec\xe6\xae");
	names->misc.assign(22, "\x8f\xa0\xab\xaa\xa0");
	names->special.assign(74, "\xe1\xa8\xab\xeb");
	names->bonus.assign(7, "\xa3\xae\xe0\xa3\xae\xad\xa0");
	names->broken = "\f32 \xe1\xab\xae\xac\xa0\xad\xae";
	names->cursed = "\f09 \xaf\xe0\xae\xaa\xab\xef\xe2\xae";
	names->of = " ";
	names->russian = true;
	catalog.setNames(names);
	const auto describe = [&](XeenInventoryCategory category, unsigned id, unsigned material, unsigned state) {
		XeenItem item;item.id = std::uint8_t(id);item.material = std::uint8_t(material);item.state = std::uint8_t(state);
		return catalog.describe(category, item);
	};
	using C = XeenInventoryCategory;
	check(describe(C::Weapons, 1, 7, 0).displayName == "\x8c\xa5\xe7 \xa8\xa7 7", "name, then material");
	check(describe(C::Weapons, 1, 7, 2).displayName == "\x8c\xa5\xe7 \xa8\xa7 7 \xa3\xae\xe0\xa3\xae\xad\xa0",
		"material, then suffix");
	const auto broken = describe(C::Armor, 1, 7, 128 | 64);
	check(broken.displayName == "\x81\xe0\xae\xad\xef \xe1\xab\xae\xac\xa0\xad\xae \xaf\xe0\xae\xaa\xab\xef\xe2\xae",
		"name, broken, cursed; no material");
	check(broken.coloredName(15) == "\f15\x81\xe0\xae\xad\xef\f32 \xe1\xab\xae\xac\xa0\xad\xae\f09 \xaf\xe0\xae\xaa\xab\xef\xe2\xae\f15",
		"broken in 32 and cursed in 09 after the name");
	check(describe(C::Accessories, 1, 0, 64).coloredName(9) == "\f09\x8a\xae\xab\xec\xe6\xae\f09 \xaf\xe0\xae\xaa\xab\xef\xe2\xae\f09",
		"cursed accessory");
	check(describe(C::Miscellaneous, 1, 1, 0).displayName == "\x8f\xa0\xab\xaa\xa0 \xe1\xa8\xab\xeb", "of is a space");
	// The program's raw concatenation: " broken" runs into the name (kept).
	const auto misc = describe(C::Miscellaneous, 1, 1, 128 | 64);
	check(misc.displayName == " \xe1\xab\xae\xac\xa0\xad\xae \xaf\xe0\xae\xaa\xab\xef\xe2\xae\x8f\xa0\xab\xaa\xa0" &&
		misc.coloredName(15) == "\f15\f32 \xe1\xab\xae\xac\xa0\xad\xae\f09 \xaf\xe0\xae\xaa\xab\xef\xe2\xae\f15\x8f\xa0\xab\xaa\xa0",
		"miscellaneous keeps the CD order and the program's spacing");
	check(describe(C::Miscellaneous, 1, 1, 128).displayName == " \xe1\xab\xae\xac\xa0\xad\xae\x8f\xa0\xab\xaa\xa0",
		"broken runs into the name");
	check(describe(C::Weapons, 1, 0, 0).displayName == "\x8c\xa5\xe7", "Cyrillic is never capitalized");
	names->weapons[2] = "sword";
	check(describe(C::Weapons, 2, 0, 0).displayName == "Sword", "an ASCII first letter is");
}

// N1: a literal byte is never a break point in the flowing renderer.
void literalWrap() {
	const XeenFontFormat english(testFont(false));
	XeenTextRenderOptions options;
	options.bounds = {0, 0, 22, 200};
	options.originalControls = false;
	options.colorIndex = 15;
	const auto render = [&](const std::string &text) {
		return XeenTextRenderer(english).render(IndexedFrame(320, 200, Bytes(64000, 0)), text, options).pages.at(0);
	};
	const auto glyphAt = [](const IndexedFrame &frame, int left, int top) {
		for (int y = top; y < top + 8; ++y)
			for (int x = left; x < left + 5; ++x)
				if (frame.pixels[std::size_t(y) * 320 + x])
					return true;
		return false;
	};
	// A real space breaks "AA AAA" into AA / AAA; the literal 0xa0 (a space
	// glyph in English) does not, so the first line holds four glyphs.
	const auto spaced = render("AA AAA"), literal = render("AA" + xeenLiteralText("\xa0") + "AAA");
	check(!glyphAt(spaced, 15, 0) && glyphAt(spaced, 10, 10), "a space is a break point");
	check(glyphAt(literal, 15, 0) && !glyphAt(literal, 10, 10) && glyphAt(literal, 5, 10), "a literal is not");
}

// RU 0xd819/0xd826: the caption shows from cursor 1 with Efx too, and the CD's
// text-mode phase rules stay.
void entranceCaption() {
	using A = XeenEntranceAnimation;using P = A::Phase;
	for (unsigned action = 8; action <= 11; ++action) {
		const auto run = [&](bool russian, std::vector<unsigned> &captions) {
			std::vector<P> phases;std::uint64_t now = 0;
			A a(action, false, [](const char *) { return 400000000ULL; }, true, 6, false, false, XeenSide::Clouds, false, russian);
			for (unsigned n = 0; n < 4000 && !a.finished(); ++n) {
				auto frame = a.advance(now, [](unsigned lo, unsigned) { return lo; });
				if (frame) {
					if (phases.empty() || phases.back() != frame->phase)phases.push_back(frame->phase);
					if (frame->caption)captions.push_back(*frame->caption);
					a.acquired(now);
				}
				now = std::max(now, a.deadline()) + 1000000;
			}
			check(a.finished(), "entrance scene finishes");
			return phases;
		};
		std::vector<unsigned> english, russian;
		const auto cd = run(false, english), ru = run(true, russian);
		check(english.empty() && cd == ru, "Efx on: same phases, no CD caption");
		check(!russian.empty() && russian.front() >= 1 && russian.back() == 0 &&
			std::count(russian.begin(), russian.end(), 0u) == 1, "Russian caption from 1 until it wraps");
	}
}

// The key sets and combat rows of the identified Russian program.
void programKeys(const Bytes &cd, const Bytes &program) {
	check(!XeenDosText(cd).keyboard(), "English has no Russian keyboard");
	const XeenDosText text(cd, program);
	const auto &k = *text.keyboard();
	check(k.letters[0x10] == 0xa9 && k.letters[0x23] == 0xe0 && k.letters[0x1a] == 0xe5 && k.letters[0x34] == 0xee &&
		!k.letters[0x29] && !k.letters[0x35] && !k.letters[0x02], "JCUKEN letter rows; H gives 0xe0");
	check(k.key("MAIN", 0xa0) == 's' && k.key("MAIN", 0xab) == 'b' && k.key("MAIN", 0xe5) == 0xb0 && k.key("MAIN", 0xea) == 0xb1,
		"main screen keys and strafe");
	check(k.key("COMBAT", 0xeb) == 'f' && k.key("COMBAT", 0xa0) == 'a' && k.key("COMBAT", 0xad) == 'o', "combat keys");
	const auto reaches = [&](const char *set, unsigned internal) {
		const auto &aliases = k.sets.at(set).aliases;
		return std::any_of(aliases.begin(), aliases.end(), [&](const auto &a) { return a.second == internal; });
	};
	check(!reaches("MAIN", 'w') && !reaches("INN", 'e') && !reaches("ITEMS", 'g') && !reaches("LLOYD", 't'),
		"Latin W, E, G and T stay untypable");
	check(k.key("INN", 0xeb) == 'x' && k.key("ITEMS", 0xa1) == 'a' && k.key("ITEMS", 0xaa) == 'b' && k.key("SHEET", 0xa2) == 'i',
		"Inn, items and sheet keys");
	check(k.key("YESNO", 0xa4) == 'y' && k.key("YESNO", 0xad) == 'n' && k.key("TAVERN", 0xae) == 's' && k.key("BANK", 0xe1) == 'w' &&
		k.key("BANK_AMOUNT", 0xa7) == 'o' && k.key("BANK_AMOUNT", 0xe1) == 'e' && k.key("CHOOSER", 0xa2) == 's', "location keys");
	check(k.key("PANEL", 0xa8) == 0xae && k.key("PANEL", 0xaa) == 0xa5 && k.key("PANEL", 0xa6) == 'g' && k.key("PANEL", 0xa1) == 'b' &&
		k.key("PANEL", 0xb1) == 'r' && !reaches("PANEL", 'o') && k.sets.at("PANEL").aliases.size() == 5, "panel aliases and clock stages");
	check(k.key("QUICK_FIGHT", 0xae) == 'n' && k.key("QUICK_FIGHT", 0xe1) == 'n', "Quick Fight Next keys");
	// Approved deviation: clicking Next works in Russian. The click reaches
	// the dialog's Next exactly as SDL resolves it.
	const auto quickFight = xeenQuickFightInput();
	const auto clicked = quickFight.click(250, 115);
	const auto *next = clicked ? std::get_if<DialogKeyAction>(&*clicked) : nullptr;
	check(quickFight.keySet && next && k.click(quickFight.keySet, next->key) == 'n' && quickFight.key('n'),
		"a Russian click on Quick Fight's Next dispatches Next");
	check(k.click("CREATE_EXCHANGE", 'm') == 'M' && k.key("CREATE_EXCHANGE", 0xab) == 'm' && k.key("CREATE", 0xa1) == 'r',
		"Create's attribute exchange: keys only");
	check(k.key("LLOYD", 0xa2) == 'r' && k.key("LLOYD", 0xe3) == 's', "Lloyd's Beacon keys");
	// M115: a controller sends the Russian program's key for the same button.
	{
		using Control = ControllerControl;
		const auto sends = [&](Control control, MainScreen screen, std::shared_ptr<const DialogInput> dialog = {}) {
			InputContext context;context.mainScreen = screen;context.dialog = std::move(dialog);
			return xeenControllerKeyEvent(control, xeenControllerContext(context, text.keyboard()));
		};
		const auto at = [&](Control control, MainScreen screen, SDL_Scancode scancode) {
			const auto event = sends(control, screen);
			return event && event->scancode == scancode && event->mod == KMOD_NONE;
		};
		check(at(Control::LeftShoulder, MainScreen::Exploration, SDL_SCANCODE_LEFTBRACKET) &&
			at(Control::RightShoulder, MainScreen::Exploration, SDL_SCANCODE_RIGHTBRACKET), "controller strafes on the х and ъ keys");
		check(at(Control::Y, MainScreen::Exploration, SDL_SCANCODE_F) && at(Control::LeftTrigger, MainScreen::Exploration, SDL_SCANCODE_K) &&
			at(Control::A, MainScreen::Combat, SDL_SCANCODE_F) && at(Control::Y, MainScreen::Combat, SDL_SCANCODE_S),
			"controller Shoot, Bash, Attack and Quick Fight are the Russian letters");
		for (const auto control : {Control::X, Control::View})
			check(sends(control, MainScreen::Exploration) && sends(control, MainScreen::Combat), "controller Cast and Rest/Run are typable");
		// The Russian combat key set has no Map key, so neither has the controller.
		check(sends(Control::RightStick, MainScreen::Exploration) && !sends(Control::RightStick, MainScreen::Combat),
			"controller Map: exploration only in Russian");
		check(!sends(Control::LeftStick, MainScreen::Exploration) && !sends(Control::LeftStick, MainScreen::Combat),
			"the controller's W sends nothing in Russian");
		auto yesNo = std::make_shared<DialogInput>();
		yesNo->keys = {'y', 'n', InputKey::Escape};yesNo->keySet = "YESNO";
		const auto yes = sends(Control::A, MainScreen::None, yesNo);
		check(yes && yes->scancode == SDL_SCANCODE_L, "controller Yes is the Russian program's д key");
	}
	const auto rows = text.combatTargets();
	check(rows.top == 10 && rows.stride == 18 && rows.height == 18, "Russian combat rows");
	check(XeenDosText(cd).combatTargets().top == 27 && XeenDosText(cd).combatTargets().stride == 10, "CD combat rows");
}

void archiveItems(XeenAssetSource &en, XeenAssetSource &ru) {
	const auto bytes = ru.readItemMaterialNamesFromDarkArchive(), cdBytes = en.readItemMaterialNamesFromDarkArchive();
	check(bytes && cdBytes && bytes->front() == ' ', "the translated mae.xen");
	// Its only non-text content is its structure: 131 terminated names.
	check(std::count(bytes->begin(), bytes->end(), 0) == std::count(cdBytes->begin(), cdBytes->end(), 0) &&
		bytes->back() == 0 && cdBytes->back() == 0, "mae.xen keeps the CD's entries");
	const auto parsed = parseXeenMaterialNames(*bytes, XeenLanguage::Russian);
	check(parsed.ready() && std::all_of(parsed.names.begin() + 1, parsed.names.end(),
		[](const std::string &name) { return std::any_of(name.begin(), name.end(), [](char c) { return static_cast<unsigned char>(c) >= 0x80; }); }),
		"every Russian material name is Cyrillic");
	const auto catalog = loadXeenItemCatalog(ru).catalog, english = loadXeenItemCatalog(en).catalog;
	XeenItem item;item.id = 1;item.material = 1;
	const auto trim = [](std::string value) {
		value.erase(0, value.find_first_not_of(' '));value.erase(value.find_last_not_of(' ') + 1);return value;
	};
	const auto name = trim(ru.uiText().table("WEAPON_NAMES").at(1)), material = parsed.names[1];
	check(catalog.describe(XeenInventoryCategory::Weapons, item).displayName == name + " " + material, "Russian weapon order");
	check(english.describe(XeenInventoryCategory::Weapons, item).displayName.rfind("Burning ", 0) == 0, "English order");
	item.state = 128;
	const auto broken = catalog.describe(XeenInventoryCategory::Weapons, item);
	check(broken.displayName.rfind(name + " ", 0) == 0 && broken.brokenEnd == broken.displayName.size(), "broken follows the name");
}

// Combat rows with a two-line monster name and the selected row, in both
// languages.
void combatRows(XeenAssetSource &en, XeenAssetSource &ru, const fs::path &out) {
	for (auto *assets : {&en, &ru}) {
		const bool russian = assets->language() == XeenLanguage::Russian;
		const XeenFontFormat font(assets->readArchiveResource("fnt"), assets->language());
		std::array<XeenCombatPanelRow, 3> rows{};
		rows[0] = {russian ? std::string("\x91\xe2\xae\xe7\xad\xeb\xa9\n\xae\xe5\xae\xe2\xad\xa8\xaa") : "Sewer\nStalker", 10, 10};
		rows[1] = {russian ? std::string("\x8a\xe0\xeb\xe1\xa0") : "Rat", 3, 10};
		rows[2] = {russian ? std::string("\x8c\xae\xe0\xa4\xa0") : "Snout", 1, 10};
		assets->loadPalette("mm4.pal");assets->loadRawFramebuffer("back.raw");
		const auto frame = drawXeenCombatPanel(assets->uiText(), assets->snapshot(), font, rows, 1u,
			[&](IndexedFrame &f, const char *resource, unsigned index, int x, int y) { assets->drawDialogSprite(f, resource, index, x, y); });
		if (!out.empty())
			picture(out / (russian ? "combat-rows-ru.ppm" : "combat-rows-en.ppm"), frame);
	}
}

// One strip record of a scene: its advances until the next record starts,
// and the segments where it started and ended.
struct StripRecord {unsigned record;std::size_t length;unsigned advances = 0;std::string start, end;};
struct StripRun {std::vector<StripRecord> records;std::vector<std::string> segments;};

// Runs a scene with a fake clock and a key whenever it waits for one.
StripRun runStrip(const GameInstallation &installation, XeenAssetSource &assets, const XeenFontFormat &font, XeenAudio &audio,
		std::uint64_t &now, XeenCutscene scene, bool digital, XeenSceneArchive archive, const XeenCutsceneData::Captions &captions,
		unsigned bound) {
	(void)installation;
	auto services = XeenCutsceneFlow::original(assets, font, audio, [&] { return now; }, digital, archive);
	XeenCutsceneFlow flow(std::move(scene), std::move(services));
	StripRun run;
	const auto &observed = *flow.cutscene().observations;
	std::size_t records = 0, advances = 0;
	unsigned held = 0;
	for (unsigned guard = 0; guard < 400000 && !flow.done(); ++guard) {
		flow.presented(flow.frame().presentation());
		flow.completeInput(flow.frame().presentation());
		const auto name = flow.segmentName();
		if (run.segments.empty() || run.segments.back() != name) {run.segments.push_back(name);held = 0;}
		while (records < observed.captionRecords.size()) {
			if (!run.records.empty())run.records.back().end = name;
			const auto record = observed.captionRecords[records++];
			run.records.push_back({record, captions.record(record, bound).size(), 0, name, {}});
		}
		for (; advances < observed.captionAdvances.size(); ++advances)
			if (!run.records.empty())++run.records.back().advances;
		if (++held == 2000 && flow.inputContext(flow.frame().presentation()).readyForAction) {
			flow.handle(DialogKeyAction{' '}, flow.frame().presentation());
			held = 0;
		}
		const auto next = flow.nextPresentationDeadline();
		now = next ? std::max(now + 1, *next) : now + 20000000;
		audio.advance(now);
		flow.update();
	}
	check(flow.done(), "scene finishes");
	if (!run.records.empty())run.records.back().end = "end";
	return run;
}

// RU 0x112cb and the 29 strip waits: with speech in Russian, each waiting
// record scrolls once to its wrap before its scene moves on; cleared clips
// stay off and the Release caption starts at 1 (RU 0xca98, CD 0xcdb6 at 0).
void sceneCaptions(const GameInstallation &english, const GameInstallation &russian, std::ostream &log) {
	for (const auto *installation : {&english, &russian}) for (const unsigned device : {0u, 3u}) {
		const bool ru = installation == &russian;
		XeenAssetSource assets(*installation, 320, 200);
		const XeenFontFormat font(assets.readArchiveResource("fnt", XeenSceneArchive::DarksideOnly), assets.language());
		auto data = XeenCutsceneData::installed(*installation);
		data.digitalDevice = device;
		std::uint64_t now = 0;
		auto audio = XeenAudio::installed(*installation, assets, now, false, fs::path("mmodern-nonexistent-rom-fixture"));
		audio->observeOptions({false, false, true, false});
		const auto captions = [&](const std::string &resource, XeenSceneArchive archive) {
			return XeenCutsceneData::captions(assets.readArchiveResource(resource, archive), assets.language());
		};
		const auto darkside = captions(data.darkside.resources[XeenCutsceneData::Darkside::Caption], XeenSceneArchive::DarksideProgram);
		const auto clouds = captions(data.resources[XeenCutsceneData::Caption], XeenSceneArchive::CloudsEnding);
		const std::string label = std::string(ru ? "Russian" : "English") + " device " + std::to_string(device);
		const auto report = [&](const std::string &scene, const StripRun &run) {
			for (const auto &r : run.records)
				log << label << ' ' << scene << " record " << r.record << " length " << r.length << " advances " << r.advances
					<< " from '" << r.start << "' to '" << r.end << "'\n";
		};
		const auto intro = runStrip(*installation, assets, font, *audio, now, XeenCutscene::darkside(data, darkside,
			XeenWallClock::fixed(17000000), XeenCutscene::DarksidePath::Full), device >= 2, XeenSceneArchive::DarksideProgram, darkside, 1000);
		report("Darkside intro", intro);
		const auto cloudsEnding = runStrip(*installation, assets, font, *audio, now, XeenCutscene::cloudsEnding(data, clouds,
			XeenWallClock::fixed(17000000), 1234567), device >= 2, XeenSceneArchive::CloudsEnding, clouds, 300);
		report("Clouds ending", cloudsEnding);
		const auto ending = runStrip(*installation, assets, font, *audio, now, XeenCutscene::darksideEnding(data, darkside,
			XeenWallClock::fixed(17000000), 1234567), device >= 2, XeenSceneArchive::DarksideProgram, darkside, 1000);
		report("Darkside ending", ending);
		const auto order = [](const StripRun &run) {
			std::vector<unsigned> records;
			for (const auto &r : run.records)records.push_back(r.record);
			return records;
		};
		check(order(intro) == std::vector<unsigned>{0, 1, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 6}, label + ": Darkside intro records");
		check(order(ending) == std::vector<unsigned>{21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31}, label + ": Darkside ending records");
		const bool strip = ru || device < 2;
		check(order(cloudsEnding) == (strip ? std::vector<unsigned>{8, 10, 10} : std::vector<unsigned>{}), label + ": Clouds ending records");
		// A record seeded at s scrolls length-s advances to its wrap; the CD
		// starts Release at 0 and its 35 frames end before the wrap; Corak's
		// "cannot fail" (record 28) has no wait site.
		const auto seed = [&](unsigned record) { return record == 0 ? 8u : record == 1 ? 25u : record == 6 && !ru ? 0u : 1u; };
		for (const auto *run : {&intro, &cloudsEnding, &ending})for (const auto &r : run->records) {
			const std::string what = label + " record " + std::to_string(r.record);
			if (!strip)check(!r.advances, what + ": no strip with speech");
			else if (r.record == 28 || (r.record == 6 && !ru))check(r.advances < r.length - seed(r.record), what + ": cut by its frames");
			else check(r.advances == r.length - seed(r.record), what + ": one scroll to the wrap, never restarted");
		}
	}
}

// RU 0x112cb/0x8548: the Clouds intro strip runs with speech, and without
// speech all 14 narration slots run.
void cloudsCaptions(const GameInstallation &english, const GameInstallation &russian, const fs::path &out) {
	for (const auto *installation : {&english, &russian}) for (const unsigned device : {0u, 3u}) {
		const bool ru = installation == &russian;
		XeenAssetSource assets(*installation, 320, 200);
		const XeenFontFormat font(assets.readArchiveResource("fnt", XeenSceneArchive::DarksideOnly), assets.language());
		auto data = XeenCutsceneData::installed(*installation);
		data.digitalDevice = device;
		std::uint64_t now = 0;
		auto audio = XeenAudio::installed(*installation, assets, now, false, fs::path("mmodern-nonexistent-rom-fixture"));
		audio->observeOptions({false, false, true, false});
		const auto story = XeenCutsceneData::firstCaption(assets.readArchiveResource(data.resources[XeenCutsceneData::Caption],
			XeenSceneArchive::CloudsProgram), assets.language());
		auto services = XeenCutsceneFlow::original(assets, font, *audio, [&] { return now; }, device >= 2, XeenSceneArchive::CloudsProgram);
		XeenCutsceneFlow flow(XeenCutscene::clouds(data, story), std::move(services));
		std::set<std::string> narration;
		unsigned stripFrames = 0;
		std::size_t advances = 0;
		for (unsigned guard = 0; guard < 400000 && !flow.done(); ++guard) {
			flow.presented(flow.frame().presentation());
			flow.completeInput(flow.frame().presentation());
			const auto name = flow.segmentName();
			if (name.rfind("narration ", 0) == 0) {
				narration.insert(name);
				const auto observed = flow.cutscene().observations->captionAdvances.size();
				if (observed != advances) {
					advances = observed;
					if (!out.empty() && ru && stripFrames++ % 60 == 0)
						picture(out / ("clouds-intro-" + std::to_string(device) + "-" + std::to_string(stripFrames) + ".ppm"), flow.frame());
				}
			}
			const auto next = flow.nextPresentationDeadline();
			now = next ? std::max(now + 1, *next) : now + 20000000;
			audio->advance(now);
			flow.update();
		}
		check(flow.done(), "Clouds intro finishes");
		const auto total = flow.cutscene().observations->captionAdvances.size();
		const std::string label = std::string(ru ? "Russian" : "English") + " device " + std::to_string(device) + ": ";
		if (!ru) {
			check(narration.size() == (device >= 2 ? 14u : 1u), label + "CD narration slots");
			check((total > 0) == (device < 2), label + "CD strip only without speech");
		} else {
			check(narration.size() == 14, label + "all 14 narration slots");
			check(total + 1 >= story.size() && total > 0, label + "the strip runs through to its wrap");
		}
	}
}

} // namespace

int main(int argc, char **argv) {
	try {
		if (argc == 1) {
			shiftRule();
			positions();
			keySets();
			textEntry();
			materials();
			items();
			literalWrap();
			entranceCaption();
			std::cout << "Russian rules: Shift, key sets, entry, materials, item order, literal wrap and entrance caption passed\n";
			return 0;
		}
		if (argc != 3 && argc != 4)
			throw std::runtime_error("Usage: mmodern_russian_rules_tests [<cd-game-dir> <russian-dir> [pictures-dir]]");
		const auto english = xeenTestInstallationDetector().detect(fs::u8path(argv[1]));
		check(english.has_value() && english->uiModule && !english->translation, "CD installation");
		auto russian = *english;
		russian.translation = XeenRussianTranslation::open(fs::u8path(argv[2]));
		const fs::path out = argc == 4 ? fs::u8path(argv[3]) : fs::path{};
		if (!out.empty())
			fs::create_directories(out);
		programKeys(read(english->uiModule->physicalPath), read(russian.translation->program(false)));
		XeenAssetSource en(*english, 320, 200), ru(russian, 320, 200);
		archiveItems(en, ru);
		combatRows(en, ru, out);
		cloudsCaptions(*english, russian, out);
		std::ofstream log(fs::temp_directory_path() / "mmodern-russian-scene-captions.log");
		sceneCaptions(*english, russian, log);
		std::cout << "Russian rules with the programs: key sets, combat rows, materials, items and the caption strip passed\n";
		return 0;
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return 1;
	}
}
