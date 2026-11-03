// M110 Part B with the CD and the Russian World of Xeen translation: every
// English program field is mapped to the identified Russian program or listed
// as CD-only, with the argument signature checked; the Russian program's
// plurals and gendered texts are settled; cutscene text is Russian while its
// timing, coordinates, cues and actors stay the CD's; item names come from
// the Russian program. Arguments: <cd-game-dir> <russian-dir> [pictures-dir].
// With a pictures directory it also writes the last frame of every intro,
// ending and score segment, as PPM, for inspection.
#include "XeenTestInstallation.h"
#include "app/XeenAudio.h"
#include "app/XeenCutsceneFlow.h"
#include "app/XeenTitleFlow.h"
#include "core/PlayerAction.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCutsceneData.h"
#include "formats/xeen/XeenDosText.h"
#include "formats/xeen/XeenFontFormat.h"
#include "formats/xeen/XeenRussianTranslation.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenItemCatalog.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenWallClock.h"

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

bool cyrillic(const std::string &text) {
	return std::any_of(text.begin(), text.end(), [](char c) { return static_cast<unsigned char>(c) >= 0x80; });
}

std::string withoutDigits(std::string text) {
	text.erase(std::remove_if(text.begin(), text.end(), [](char c) { return c >= '0' && c <= '9'; }), text.end());
	return text;
}

// Decision 6: the census. Decision 7: the signatures. Decision 9: English
// values of the program text that moved out of code.
void census(const Bytes &cd, const Bytes &program, std::ostream &log) {
	const XeenDosText english(cd), russian(cd, program);
	check(english.language() == XeenLanguage::English && russian.language() == XeenLanguage::Russian, "layout languages");
	const std::set<std::string_view> absent(XeenDosText::russianAbsent().begin(), XeenDosText::russianAbsent().end());
	check(absent == std::set<std::string_view>{"NPC_TITLE_ONLY", "CREDITS_RESOURCE", "MENU_SPRITE", "TITLE_ANIMATIONS"},
		"confirmed-absent fields");
	std::set<std::string_view> russianNames;
	for (const auto &field : XeenDosText::russianLayout())
		russianNames.insert(field.name);
	unsigned mapped = 0, translated = 0;
	for (const auto &field : XeenDosText::layout()) {
		const std::string name = field.name;
		const bool isAbsent = absent.count(field.name) != 0;
		check(isAbsent != (russianNames.count(field.name) != 0), name + " is mapped or confirmed absent, never both or neither");
		const auto &values = russian.table(name);
		check(values.size() == english.table(name).size(), name + " keeps its entry count");
		if (isAbsent) {
			check(values == english.table(name), name + " keeps the CD value");
			log << "absent " << name << '\n';
			continue;
		}
		++mapped;
		// The one signature the Russian program changed: its panel has no
		// Text and Speech toggles.
		const std::string expected = name == "PANEL_BUTTON_TEXT" ? "s,s" : field.arguments;
		check(russian.arguments(name) == expected, name + " argument signature");
		for (const auto &value : values)
			check(XeenDosText::formatSignature(value) == expected, name + " value signature");
		if (std::any_of(values.begin(), values.end(), cyrillic))
			++translated;
		log << "mapped " << name << " " << russian.arguments(name) << '\n';
	}
	check(mapped == 275 && mapped + absent.size() == XeenDosText::layout().size(), "275 mapped (271 and the 4 decision-9 strings), 4 absent, none unlocated");
	check(translated == 221, "221 mapped fields hold Cyrillic text");
	// Decision 3: a resource the program names is the CD's resource in both
	// languages. The DOS save-file patterns are displayed text, not resources.
	const auto resource = [](const std::string &value) {
		const auto dot = value.rfind('.');
		return !value.empty() && dot != std::string::npos && dot + 4 >= value.size() &&
			std::all_of(value.begin(), value.end(), [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '%'; });
	};
	unsigned resources = 0;
	for (const auto &field : XeenDosText::layout()) {
		const std::string name = field.name;
		if (absent.count(field.name) || name == "DOS_SLOT_PATTERN" || name == "DOS_FILE_PATTERN")
			continue;
		const auto &values = english.table(name);
		if (std::all_of(values.begin(), values.end(), resource))
			++resources, check(russian.table(name) == values, name + " names the CD resource");
	}
	check(resources == 11, "the 11 resource-name fields compared");
	check(russian.scalar("MILLIONS_FORMAT") == "%lu \xac\xa8\xab" && russian.arguments("MILLIONS_FORMAT") == "lu", "Russian millions");
	check(cyrillic(std::string(russian.scalar("CREDITS_TEXT"))) && !russian.table("CREDITS_RESOURCE").empty(),
		"Russian one-page credits beside the CD-only credits.bin name");
	// The panel window (23) is the Russian program's; windows the CD shares stay equal.
	check(english.window(23).height == 140 && russian.window(23).height == 120 && russian.window(36).y == 74 &&
		russian.window(21).x == english.window(21).x && russian.window(21).height == english.window(21).height, "Russian windows");
	// The Russian program: no inflection; the same panel without Text/Speech.
	check(russian.characters("FOOD_DAY_SUFFIX") == "  " && russian.characters("QUEST_ITEM_SUFFIX") == "  " &&
		russian.characters("SLOT_DIFFICULTY") == "AW" && russian.characters("CLOCK_MERIDIEM") == "ap", "Russian characters");
	check(russian.scalar("DAY_PLURAL").empty() && russian.scalar("DAY_SINGULAR").empty() && russian.scalar("THOUSANDS_FORMAT") == "%uk",
		"Russian program strings");
	const auto &panel = russian.buttons("PANEL");
	check(panel.size() == 7, "Russian panel buttons");
	for (unsigned i = 0; i < panel.size(); ++i)
		check(panel[i].slot == (i < 6 ? i : 8u) && panel[i].key >= 0x80, "Russian panel slots and keys");
	for (const char *table : {"TITLE", "OTHER", "OTHER_ENDING", "DIFFICULTY"})
		for (const auto &button : russian.buttons(table))
			check(button.key >= 0x80, std::string(table) + " Russian keys are cp866");
	for (std::size_t i = 0; i < 4; ++i)
		check(russian.buttons("TITLE")[i].x == english.buttons("TITLE")[i].x && russian.buttons("TITLE")[i].y == english.buttons("TITLE")[i].y,
			"Russian title keeps the CD positions");
	// Shared CD strings the translators split keep each table's own word.
	check(russian.table("ITEM_ACTIONS")[1] != russian.scalar("INN_REMOVE"), "Inn and item Remove differ in Russian");
	check(russian.scalar("GOOD") == russian.table("CONDITION_NAMES")[16], "Good is condition 16");
	check(russian.table("WEAPON_NAMES").size() == 35 && russian.table("SPECIAL_NAMES").size() == 74, "Russian item name tables");
}

// The Russian program's plural and gendered forms, settled from its code:
// every count from the boundaries draws the same words.
void grammar(XeenAssetSource &en, XeenAssetSource &ru) {
	const unsigned counts[]{1, 2, 3, 4, 5, 11, 12, 14, 20, 21};
	for (auto *assets : {&en, &ru}) {
		const bool russian = assets->language() == XeenLanguage::Russian;
		const auto &text = assets->uiText();
		auto party = XeenPartyLoader().loadInitialParty(*assets);
		const auto owner = party.party.activeRosterIds().at(0);
		auto &character = party.roster.at(owner);
		character.conditions.fill(0);
		constexpr unsigned gains[]{5, 6, 6, 7, 8, 6, 5, 4, 7, 6};
		std::set<std::string> food, attacks, quests;
		for (const auto n : counts) {
			party.food = static_cast<std::uint16_t>(n * party.party.size() * 3);
			food.insert(withoutDigits(xeenSheetPopup(text, party, 0, 18).text));
			character.permanentLevel = static_cast<int>((n - 1) * gains[static_cast<unsigned>(character.characterClass)]);
			character.temporaryLevel = 0;
			attacks.insert(withoutDigits(xeenSheetPopup(text, party, 0, 8).text));
			quests.insert(withoutDigits(xeenDialogFormat(text.scalar("QUEST_ITEM_COUNT"),
				{std::to_string(n), "x", std::string(1, text.characters("QUEST_ITEM_SUFFIX")[n > 1 ? 1 : 0])})));
		}
		// English inflects one against many; the Russian program never does.
		const std::size_t forms = russian ? 1 : 2;
		check(food.size() == forms && attacks.size() == forms && quests.size() == forms,
			std::string(russian ? "Russian" : "English") + " plural forms");
	}
	// Gendered verbs are written for both sexes at once ("взломал(а)"): no
	// field takes the sex, so both draw the same text.
	const auto &text = ru.uiText();
	check(text.arguments("PICKS_THE_LOCK") == "s" && text.arguments("UNABLE_TO_PICK_LOCK") == "s" &&
		text.arguments("X_FOUND_Y") == "s,s", "no Russian field takes the sex");
	const std::string picks(text.scalar("PICKS_THE_LOCK")), unable(text.scalar("UNABLE_TO_PICK_LOCK"));
	check(picks.find("(\xa0)") != std::string::npos && unable.find("(\xab\xa0)") != std::string::npos,
		"lock texts carry both endings");
}

void cutscenes(const GameInstallation &english, const GameInstallation &russian) {
	const auto en = XeenCutsceneData::installed(english), ru = XeenCutsceneData::installed(russian);
	// Text fields: Russian, with the CD's signatures.
	std::vector<std::pair<const std::string *, const std::string *>> texts{
		{&en.labels, &ru.labels}, {&en.captionFormat, &ru.captionFormat}, {&en.worldEnding.opening, &ru.worldEnding.opening}};
	for (unsigned i = 0; i < 2; ++i) {
		texts.push_back({&en.ending.scorePages[i], &ru.ending.scorePages[i]});
		texts.push_back({&en.darksideEnding.scorePages[i], &ru.darksideEnding.scorePages[i]});
		texts.push_back({&en.darksideEnding.scoreFormats[i], &ru.darksideEnding.scoreFormats[i]});
	}
	for (unsigned i = 0; i < 4; ++i)
		texts.push_back({&en.worldEnding.formats[i], &ru.worldEnding.formats[i]});
	for (unsigned i = 0; i < 8; ++i)
		texts.push_back({&en.worldEnding.captions[i], &ru.worldEnding.captions[i]});
	unsigned changed = 0;
	for (const auto &[e, r] : texts) {
		check(XeenDosText::formatSignature(*e) == XeenDosText::formatSignature(*r), "cutscene text signature");
		if (*e != *r)
			++changed, check(cyrillic(*r), "changed cutscene text is Russian");
	}
	// Unchanged: the caption format and the four World ending formats, which hold no words.
	check(changed == texts.size() - 5, "every cutscene text with words is translated");
	// Everything else stays the CD's.
	check(en.resources == ru.resources && en.voices == ru.voices && en.mouths == ru.mouths && en.waits == ru.waits &&
		en.rows == ru.rows && en.towerLeft == ru.towerLeft && en.towerRight == ru.towerRight && en.towerY == ru.towerY &&
		en.handPattern == ru.handPattern && en.marblePattern == ru.marblePattern, "Clouds intro data");
	const auto &a = en.ending, &b = ru.ending;
	check(a.resources == b.resources && a.prec == b.prec && a.vortex == b.vortex && a.castleTop == b.castleTop &&
		a.castleBottom == b.castleBottom && a.powers == b.powers && a.darkLord == b.darkLord && a.kingVoices == b.kingVoices &&
		a.castleFrames == b.castleFrames && a.kingLeft == b.kingLeft && a.kingRight == b.kingRight && a.kingY == b.kingY &&
		a.mirrorX == b.mirrorX && a.mirrorY == b.mirrorY && a.flyingX == b.flyingX && a.flyingY == b.flyingY &&
		a.initialFlying == b.initialFlying && a.stormFm == b.stormFm && a.vortexFm == b.vortexFm && a.randomWords == b.randomWords,
		"Clouds ending data");
	for (std::size_t i = 0; i < a.parade.size(); ++i)
		check(a.parade[i].selector == b.parade[i].selector && a.parade[i].metadata == b.parade[i].metadata &&
			a.parade[i].idle == b.parade[i].idle && a.parade[i].attack == b.parade[i].attack && a.parade[i].voice == b.parade[i].voice,
			"ending parade actor");
	const auto &d = en.darkside, &e = ru.darkside;
	check(d.resources == e.resources && d.song == e.song && d.alternateSong == e.alternateSong && d.title == e.title &&
		d.pyramid == e.pyramid && d.room == e.room && d.roomFrames == e.roomFrames && d.ball == e.ball, "Darkside intro data");
	for (std::size_t i = 0; i < d.birdCues.size(); ++i)
		check(d.birdCues[i].key == e.birdCues[i].key && d.birdCues[i].target == e.birdCues[i].target, "Darkside cues");
	check(en.darksideEnding.resources == ru.darksideEnding.resources && en.worldEnding.strings == ru.worldEnding.strings,
		"ending resources");
	bool refused = false;
	try {
		XeenCutsceneData copy = en;
		copy.applyRussianText(Bytes(142257, 0));
	} catch (const std::exception &) {
		refused = true;
	}
	check(refused, "a different Russian XEEN.EXE is refused");
}

void items(XeenAssetSource &en, XeenAssetSource &ru) {
	const auto english = loadXeenItemCatalog(en).catalog, russian = loadXeenItemCatalog(ru).catalog;
	const auto name = [](const XeenItemCatalog &catalog, XeenInventoryCategory category, std::uint8_t id, std::uint8_t material) {
		XeenItem item{};
		item.id = id;
		item.material = material;
		return category == XeenInventoryCategory::Miscellaneous ? catalog.foundName(category, item) : catalog.describe(category, item).displayName;
	};
	check(name(english, XeenInventoryCategory::Weapons, 1, 0) == "Long sword", "English catalog unchanged");
	check(name(russian, XeenInventoryCategory::Weapons, 1, 0) == "\x8c\xa5\xe7", "Russian weapon name");
	check(name(russian, XeenInventoryCategory::Armor, 1, 0) == ru.uiText().table("ARMOR_NAMES")[1], "Russian armor name");
	check(name(russian, XeenInventoryCategory::Accessories, 3, 0) == ru.uiText().table("ACCESSORY_NAMES")[3], "Russian accessory name");
	check(name(russian, XeenInventoryCategory::Miscellaneous, 0, 1) == ru.uiText().table("MISC_NAMES")[1], "Russian misc name");
	// Ids 35-40 are in neither program: the catalog's names.
	check(name(russian, XeenInventoryCategory::Weapons, 35, 0) == name(english, XeenInventoryCategory::Weapons, 35, 0), "weapon 35 fallback");
}

// B1: the Russian Other Options rows carry cp866 keys; a click on each won
// ending row starts that ending.
void endingRows(XeenAssetSource &assets, const XeenFontFormat &font) {
	const auto original = XeenTitleFlow::original(assets, font, [] { return std::array<XeenSaveFile::Slot, 10>{}; },
		[](unsigned i) { return fs::path(std::to_string(i)); });
	const XeenSessionEntry::Kind kinds[]{XeenSessionEntry::Kind::DarksideEnding, XeenSessionEntry::Kind::CloudsEnding,
		XeenSessionEntry::Kind::WorldEnding};
	for (unsigned row = 0; row < 3; ++row) {
		auto services = original;
		services.darksideWon = services.cloudsWon = services.worldWon = true;
		XeenTitleFlow flow(std::move(services));
		std::uint64_t now = 1;
		const auto present = [&] {
			flow.presented(flow.frame().presentation());
			flow.completeInput(flow.frame().presentation());
		};
		const auto ready = [&] {
			present();
			flow.animate(now);
			for (unsigned i = 0; i < 65 && !flow.inputContext(flow.frame().presentation()).readyForAction; ++i) {
				now += 20;
				flow.animate(now);
				present();
			}
			check(flow.inputContext(flow.frame().presentation()).readyForAction, "Russian title input");
		};
		const auto click = [&](int x, int y) {
			const auto action = flow.inputContext(flow.frame().presentation()).dialog->click(x, y);
			check(bool(action), "Russian title hit");
			flow.handle(*action, flow.frame().presentation());
			if (flow.current())
				present();
		};
		ready();
		if (flow.screen() == XeenTitleFlow::Screen::Background)
			click(160, 100);
		const auto &other = assets.uiText().buttons("TITLE").at(3);
		click(int(other.x) + 2, int(other.y) + 2);
		check(flow.screen() == XeenTitleFlow::Screen::Other, "Russian Other Options");
		const auto dialog = flow.inputContext(flow.frame().presentation()).dialog;
		check(dialog->hits.size() == 5 && dialog->hits.at(2 + row).key >= 0x80, "Russian ending rows");
		click(dialog->hits.at(2 + row).left + 2, dialog->hits.at(2 + row).top + 2);
		check(flow.entry() && flow.entry()->kind == kinds[row], "Russian ending row starts its ending");
	}
}

// The last frame of each segment of a cutscene, driven with a fake clock and
// a key whenever the scene waits for one.
void pictures(const GameInstallation &installation, XeenAssetSource &assets, const XeenFontFormat &font, const fs::path &out) {
	fs::create_directories(out);
	const auto data = XeenCutsceneData::installed(installation);
	std::uint64_t now = 0;
	auto audio = XeenAudio::installed(installation, assets, now, false, fs::path("mmodern-nonexistent-rom-fixture"));
	audio->observeOptions({false, false, true, false});
	const auto run = [&](const std::string &label, XeenCutscene scene, XeenSceneArchive archive) {
		auto services = XeenCutsceneFlow::original(assets, font, *audio, [&] { return now; }, false, archive);
		XeenCutsceneFlow flow(std::move(scene), std::move(services));
		unsigned written = 0;
		std::string segment = flow.segmentName();
		IndexedFrame last = flow.frame();
		const auto write = [&] {
			if (segment.rfind("parade", 0) == 0 || written >= 300)
				return;
			std::string file = label + "-" + std::to_string(written++) + "-" + segment;
			std::replace(file.begin(), file.end(), ' ', '_');
			std::ofstream picture(out / (file + ".ppm"), std::ios::binary);
			picture << "P6\n320 200\n255\n";
			for (const auto pixel : last.pixels)
				picture.write(reinterpret_cast<const char *>(last.palette.data() + 3 * pixel), 3);
		};
		unsigned held = 0;
		for (unsigned guard = 0; guard < 200000 && !flow.done(); ++guard) {
			flow.presented(flow.frame().presentation());
			flow.completeInput(flow.frame().presentation());
			// The last frame of each segment, once its text is drawn.
			if (flow.segmentName() != segment) {
				write();
				segment = flow.segmentName();
				held = 0;
			}
			last = flow.frame();
			// A page that keeps animating while it waits gets a key.
			if (++held == 2000 && flow.inputContext(flow.frame().presentation()).readyForAction) {
				write();
				segment.clear();
				flow.handle(DialogKeyAction{' '}, flow.frame().presentation());
				flow.update();
				continue;
			}
			if (const auto next = flow.nextPresentationDeadline()) {
				now = std::max(now, *next);
				audio->advance(now);
				flow.update();
			} else {
				write();
				segment.clear();
				flow.handle(DialogKeyAction{' '}, flow.frame().presentation());
				now += 20000000;
				audio->advance(now);
				flow.update();
			}
		}
	};
	const auto captions = [&](const std::string &resource, XeenSceneArchive archive) {
		return XeenCutsceneData::captions(assets.readArchiveResource(resource, archive), assets.language());
	};
	run("clouds-intro", XeenCutscene::clouds(data, XeenCutsceneData::firstCaption(
		assets.readArchiveResource(data.resources[XeenCutsceneData::Caption], XeenSceneArchive::CloudsProgram), assets.language())),
		XeenSceneArchive::CloudsProgram);
	run("darkside-intro", XeenCutscene::darkside(data, captions(data.darkside.resources[XeenCutsceneData::Darkside::Caption],
		XeenSceneArchive::DarksideProgram), XeenWallClock::fixed(17000000), XeenCutscene::DarksidePath::Full), XeenSceneArchive::DarksideProgram);
	run("clouds-ending", XeenCutscene::cloudsEnding(data, captions(data.resources[XeenCutsceneData::Caption], XeenSceneArchive::CloudsEnding),
		XeenWallClock::fixed(17000000), 1234567), XeenSceneArchive::CloudsEnding);
	run("darkside-ending", XeenCutscene::darksideEnding(data, captions(data.darkside.resources[XeenCutsceneData::Darkside::Caption],
		XeenSceneArchive::DarksideProgram), XeenWallClock::fixed(17000000), 1234567), XeenSceneArchive::DarksideProgram);
	run("world-ending", XeenCutscene::worldEnding(data, 1234567), XeenSceneArchive::DarksideProgram);
	// The title menu and the seven-button control panel.
	const auto title = XeenTitleFlow::original(assets, font, [] { return std::array<XeenSaveFile::Slot, 10>{}; },
		[](unsigned i) { return fs::path(std::to_string(i)); });
	check(title.credits.size() == 1, "Russian credits are one page");
	{
		std::ofstream picture(out / "credits.ppm", std::ios::binary);
		picture << "P6\n320 200\n255\n";
		for (const auto pixel : title.credits.front().pixels)
			picture.write(reinterpret_cast<const char *>(title.credits.front().palette.data() + 3 * pixel), 3);
	}
	for (const bool panel : {false, true}) {
		auto services = title;
		services.panel = panel;
		services.currentName = "Test";
		XeenTitleFlow flow(std::move(services));
		const auto &frame = flow.frame();
		std::ofstream picture(out / (panel ? "panel.ppm" : "title.ppm"), std::ios::binary);
		picture << "P6\n320 200\n255\n";
		for (const auto pixel : frame.pixels)
			picture.write(reinterpret_cast<const char *>(frame.palette.data() + 3 * pixel), 3);
	}
}

} // namespace

int main(int argc, char **argv) {
	try {
		if (argc != 3 && argc != 4)
			throw std::runtime_error("Usage: mmodern_russian_program_text_tests <cd-game-dir> <russian-dir> [pictures-dir]");
		const auto english = xeenTestInstallationDetector().detect(fs::u8path(argv[1]));
		check(english.has_value() && english->uiModule && !english->translation, "CD installation");
		auto russian = *english;
		russian.translation = XeenRussianTranslation::open(fs::u8path(argv[2]));
		std::ofstream log(fs::temp_directory_path() / "mmodern-russian-program-text.log");
		census(read(english->uiModule->physicalPath), read(russian.translation->program(false)), log);
		XeenAssetSource en(*english, 320, 200), ru(russian, 320, 200);
		grammar(en, ru);
		cutscenes(*english, russian);
		items(en, ru);
		{
			const XeenFontFormat font(ru.readArchiveResource("fnt", XeenSceneArchive::DarksideOnly), ru.language());
			endingRows(ru, font);
		}
		if (argc == 4) {
			const XeenFontFormat font(ru.readArchiveResource("fnt", XeenSceneArchive::DarksideOnly), ru.language());
			pictures(russian, ru, font, fs::u8path(argv[3]));
		}
		std::cout << "Russian program text: census, signatures, plurals and genders, cutscene text and item names passed\n";
		return 0;
	} catch (const std::exception &e) {
		std::cerr << e.what() << '\n';
		return 1;
	}
}
