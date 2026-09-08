#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyVisualState.h"
#include "games/xeen/XeenPartyLoader.h"
#include "XeenPartySnapshotTestSupport.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;
using Bytes = std::vector<std::uint8_t>;

namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

template<class Function> void rejects(Function function) {
	bool rejected = false;
	try { function(); } catch (const std::runtime_error &) { rejected = true; }
	check(rejected, "malformed character/party data was accepted");
}

Bytes rosterFixture() {
	return Bytes(XeenRoster::kCharacterCount * XeenCharacter::kSerializedSize, 0);
}

void setName(Bytes &bytes, std::size_t rosterId, const std::string &name) {
	const std::size_t offset = rosterId * XeenCharacter::kSerializedSize;
	const std::size_t length = std::min<std::size_t>(16, name.size());
	std::copy_n(name.begin(), length, bytes.begin() + offset);
}

void setItemBlock(Bytes &bytes, std::size_t blockOffset, std::uint8_t firstMaterial) {
	for (std::size_t i = 0; i < XeenCharacter::kEquipmentSlotsPerCategory; ++i) {
		const std::size_t item = blockOffset + i * 4;
		bytes[item] = static_cast<std::uint8_t>(firstMaterial + i);
		bytes[item + 1] = static_cast<std::uint8_t>(0xa0 + i);
		bytes[item + 2] = static_cast<std::uint8_t>(0x10 + i);
		bytes[item + 3] = static_cast<std::uint8_t>(1 + i);
	}
}

void checkItemBlock(const std::array<XeenItem,
		XeenCharacter::kEquipmentSlotsPerCategory> &items, std::uint8_t firstMaterial,
		const char *message) {
	check(items.size() == 9, message);
	for (std::size_t i = 0; i < items.size(); ++i) {
		check(items[i].material == static_cast<std::uint8_t>(firstMaterial + i) &&
			items[i].id == static_cast<std::uint8_t>(0xa0 + i) &&
			items[i].state == static_cast<std::uint8_t>(0x10 + i) &&
			items[i].frame == static_cast<std::uint8_t>(1 + i), message);
	}
}

Bytes partyFixture(std::uint8_t firstCount, std::uint8_t effectiveCount,
		std::initializer_list<std::uint8_t> ids) {
	Bytes bytes(10, 0xff);
	bytes.resize(XeenQuestItemFormat::kRequiredSize, 0);
	bytes[0] = firstCount;
	bytes[1] = effectiveCount;
	std::copy(ids.begin(), ids.end(), bytes.begin() + 2);
	return bytes;
}

bool hasDiagnostic(const XeenPartyState &state, const std::string &part) {
	return std::any_of(state.diagnostics.begin(), state.diagnostics.end(),
		[&](const std::string &message) { return message.find(part) != std::string::npos; });
}

void testCharacterOffsetsAndBounds() {
	Bytes bytes = rosterFixture();
	const std::size_t base = 18 * XeenCharacter::kSerializedSize;
	const std::string sixteenCharacters = "ABCDEFGHIJKLMNOP";
	setName(bytes, 18, sixteenCharacters);
	bytes[base + 16] = 1;
	bytes[base + 17] = 4;
	bytes[base + 18] = 1;
	bytes[base + 19] = 9;
	bytes[base + 22] = 0xff;
	bytes[base + 23] = 0x80;
	bytes[base + 24] = 24;
	bytes[base + 25] = 25;
	bytes[base + 26] = 26;
	bytes[base + 27] = 27;
	bytes[base + 35] = 1;
	bytes[base + 36] = 0xff; // syncAsByte semantics: this remains unsigned.
	bytes[base + 38] = 38;
	bytes[base + 41] = 0;    // Zero means absent.
	bytes[base + 42] = 1;
	bytes[base + 51] = 2;    // Every nonzero value means present.
	bytes[base + 52] = 0xff;
	bytes[base + 163] = 0x80;
	setItemBlock(bytes, base + 166, 60);
	setItemBlock(bytes, base + 202, 80);
	setItemBlock(bytes, base + 238, 100);
	setItemBlock(bytes, base + 274, 140);
	bytes[base + 323 + 3] = 2;
	bytes[base + 323 + 15] = 1;
	bytes[base + 342] = 0xfe;
	bytes[base + 343] = 0xff; // -2
	bytes[base + 344] = 0x00;
	bytes[base + 345] = 0x80; // -32768
	bytes[base + 346] = 0x50;
	bytes[base + 347] = 0x02; // 592, little-endian.
	bytes[base + 348] = 0x78; // Preserved even though experience is not exposed.

	const XeenRoster roster = XeenCharacterFormat::parseRoster(bytes);
	const XeenCharacter &character = roster.at(18);
	check(character.rosterId == 18 && character.name == sixteenCharacters,
		"roster ID or bounded 16-byte name");
	check(character.sex == XeenSex::Female && character.race == XeenRace::HalfOrc &&
		character.characterClass == XeenCharacterClass::Ranger,
		"identity offsets");
	check(character.intellect.permanent == 255 && character.intellect.temporary == 128 &&
		character.personality.permanent == 24 && character.personality.temporary == 25 &&
		character.endurance.permanent == 26 && character.endurance.temporary == 27,
		"attribute offsets or unsigned byte loading");
	check(character.permanentLevel == 1 && character.temporaryLevel == 255 &&
		character.currentLevel() == 256, "syncAsByte level semantics");
	check(character.temporaryAge == 38, "temporary age offset");
	check(!character.maxStatSkills.astrologer && character.maxStatSkills.bodybuilder &&
		character.maxStatSkills.prayerMaster && character.maxStatSkills.prestidigitation,
		"skill offsets or zero/nonzero semantics");
	check(character.hasSpells && !roster.at(0).hasSpells,
		"hasSpells offset or boolean semantics");
	checkItemBlock(character.weapons, 60, "nine weapon modifier sources");
	checkItemBlock(character.armor, 80, "nine armor modifier sources");
	checkItemBlock(character.accessories, 100, "nine accessory modifier sources");
	checkItemBlock(character.miscellaneous, 140, "nine miscellaneous records");
	check(character.currentHp == -2 && character.currentSp == -32768,
		"signed little-endian HP/SP");
	check(character.birthYear == 592, "little-endian birth year");
	check(character.conditions[3] == 2 && character.conditions[15] == 1 &&
		character.worstCondition() == XeenCondition::Eradicated, "condition offsets/priority");
	check(character.portraitResourceName() == std::optional<std::string>("char19.fac"),
		"generic portrait mapping");
	check(roster.at(0).portraitResourceName() == std::optional<std::string>("char01.fac") &&
		roster.at(23).portraitResourceName() == std::optional<std::string>("char24.fac") &&
		!roster.at(24).portraitResourceName(), "portrait boundary");

	rejects([&] { XeenCharacterFormat::parseRoster(Bytes(bytes.begin(), bytes.end() - 1)); });
	bytes.push_back(0);
	rejects([&] { XeenCharacterFormat::parseRoster(bytes); });
}

void testPartyHeaderAndReferences() {
	Bytes roster = rosterFixture();
	setName(roster, 0, "Alpha");
	setName(roster, 18, "Beta");
	const XeenPartyLoader loader;
	const auto state = loader.loadFromResources(roster,
		partyFixture(5, 3, {0, 0xff, 18}));
	check(state.firstSerializedCount == 5 && state.effectiveSerializedCount == 3 &&
		hasDiagnostic(state, "quantidades divergentes"), "second party count must prevail with diagnostic");
	check(state.party.activeRosterIds() == std::vector<std::uint8_t>({0, 18}),
		"active order / absent 0xff entry");
	check(&state.party.member(state.roster, 0) == &state.roster.at(0) &&
		&state.party.member(state.roster, 1) == &state.roster.at(18),
		"active characters must reference roster storage");

	auto duplicate = loader.loadFromResources(roster, partyFixture(2, 2, {0, 0}));
	check(duplicate.party.size() == 2 && hasDiagnostic(duplicate, "duplicado"),
		"original duplicate semantics must be preserved and diagnosed");

	Bytes invalid = partyFixture(1, 1, {0});
	invalid[9] = 30; // Validate every serialized slot before any possible access.
	rejects([&] { loader.loadFromResources(roster, invalid); });
	rejects([&] { loader.loadFromResources(roster, partyFixture(7, 7, {0, 1, 2, 3, 4, 5, 6})); });
	rejects([&] { XeenCharacterFormat::parsePartyHeader(Bytes(9, 0)); });
}

void testItemStorage() {
	check(XeenCharacter::kSerializedSize == 354 && rosterFixture().size() == 10620,
		"original CHR size changed with the save schema");
	Bytes bytes = rosterFixture();
	const unsigned offsets[]{166, 202, 238, 274};
	for (unsigned i = 0; i < 30; ++i)
		for (unsigned category = 0; category < 4; ++category)
			for (unsigned slot = 0; slot < 9; ++slot)
				for (unsigned field = 0; field < 4; ++field)
					bytes[i * 354 + offsets[category] + slot * 4 + field] =
						static_cast<std::uint8_t>(i * 37 + category * 19 + slot * 11 + field * 67);
	// Empty metadata, unknown IDs and extreme values are all preserved on load.
	bytes[29 * 354 + 274] = 255; bytes[29 * 354 + 275] = 0;
	bytes[29 * 354 + 276] = 255; bytes[29 * 354 + 277] = 255;
	auto party = XeenPartyLoader().loadFromResources(bytes, partyFixture(3, 3, {18, 0, 18}));
	for (unsigned i = 0; i < 30; ++i) {
		const auto &c = party.roster.at(i);
		const XeenItemCategory *categories[]{&c.weapons, &c.armor, &c.accessories, &c.miscellaneous};
		for (unsigned category = 0; category < 4; ++category)
			for (unsigned slot = 0; slot < 9; ++slot) {
				const auto &item = categories[category]->at(slot);
				const auto offset = i * 354 + offsets[category] + slot * 4;
				check(item.material == bytes[offset] && item.id == bytes[offset + 1] &&
					item.state == bytes[offset + 2] && item.frame == bytes[offset + 3],
					"all-roster item bytes differ from original offsets");
			}
	}
	for (unsigned value = 0; value < 256; ++value) {
		std::fill_n(bytes.begin() + 29 * 354 + 274, 4, value);
		const auto c = XeenCharacterFormat::parseRoster(bytes).at(29);
		check(c.miscellaneous[0].material == value && c.miscellaneous[0].id == value &&
			c.miscellaneous[0].state == value && c.miscellaneous[0].frame == value, "item byte domain narrowed");
	}
	for (unsigned category = 0; category < 4; ++category) {
		auto &owner = party.roster.at(18);
		XeenItemCategory *categories[]{&owner.weapons, &owner.armor, &owner.accessories, &owner.miscellaneous};
		auto &items = *categories[category];
		items.fill({255, 0, 128, 99});
		check(xeenItemHasTailCapacity(items), "empty tail metadata is capacity");
		items[1] = {200, 250, 255, 37}; items[5] = {1, 3, 128, 255}; items[8] = {99, 1, 0, 2};
		const auto before = party;
		check(!xeenItemHasTailCapacity(items), "earlier holes must not override a full tail");
		check(remove_test::partySnapshot(party) == remove_test::partySnapshot(before), "capacity query mutated storage");
		xeenCompactItems(items);
		auto expected = before;
		auto &expectedOwner = expected.roster.at(18);
		XeenItemCategory *expectedCategories[]{&expectedOwner.weapons, &expectedOwner.armor, &expectedOwner.accessories, &expectedOwner.miscellaneous};
		*expectedCategories[category] = {{{200, 250, 255, 37}, {1, 3, 128, 255}, {99, 1, 0, 2}}};
		check(remove_test::partySnapshot(party) == remove_test::partySnapshot(expected), "compaction changed order, fields or bystanders");
		check(xeenItemHasTailCapacity(items), "compacted tail has no capacity");
		check(&party.party.member(party.roster, 0) == &owner &&
			&party.party.member(party.roster, 2) == &owner, "aliases copied item ownership");
		xeenCompactItems(items);
		check(remove_test::partySnapshot(party) == remove_test::partySnapshot(expected), "compaction is not idempotent");
	}
}

void testPortraitLayoutAndConditions() {
	Bytes roster = rosterFixture();
	setName(roster, 0, "Alpha");
	setName(roster, 18, "Beta");
	XeenPartyState state = XeenPartyLoader().loadFromResources(roster,
		partyFixture(2, 2, {0, 18}));
	auto placements = CloudsUiComposer::buildPortraitPlacements(state);
	check(placements.size() == 2 && placements[0].resourceName == "char01.fac" &&
		placements[1].resourceName == "char19.fac" && placements[0].x == 10 &&
		placements[1].x == 45 && placements[0].y == 150,
		"ordered layout / remaining four slots left empty");

	state.roster.at(0).conditions[3] = 1;
	placements = CloudsUiComposer::buildPortraitPlacements(state);
	check(placements[0].resourceName == "char01.fac" && placements[0].frame == 1,
		"individual condition frame");
	state.roster.at(0).conditions.fill(0);
	state.roster.at(0).conditions[13] = 1;
	placements = CloudsUiComposer::buildPortraitPlacements(state);
	check(placements[0].resourceName == "dse.fac" && placements[0].frame == 0,
		"dead face selection");
	state.roster.at(0).conditions.fill(0);
	state.roster.at(0).conditions[14] = 1;
	placements = CloudsUiComposer::buildPortraitPlacements(state);
	check(placements[0].resourceName == "dse.fac" && placements[0].frame == 1,
		"stoned face selection");
	state.roster.at(0).conditions.fill(0);
	state.roster.at(0).conditions[15] = 1;
	placements = CloudsUiComposer::buildPortraitPlacements(state);
	check(placements[0].resourceName == "dse.fac" && placements[0].frame == 2,
		"eradicated face selection");

	setName(roster, 24, "Unsupported");
	state = XeenPartyLoader().loadFromResources(roster, partyFixture(1, 1, {24}));
	check(hasDiagnostic(state, "nao possui retrato"), "unsupported portrait diagnostic");
	rejects([&] { CloudsUiComposer::buildPortraitPlacements(state); });
}

void prepareHpCharacter(XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	character.characterClass = XeenCharacterClass::Knight;
	character.race = XeenRace::Human;
	character.permanentLevel = 2;
	character.temporaryLevel = 0;
	character.endurance.permanent = 20;
	character.endurance.temporary = 0;
	character.birthYear = static_cast<std::uint16_t>(context.currentYear - 18);
}

void testHpLayout() {
	Bytes roster = rosterFixture();
	const XeenCharacterRulesContext context{610};
	const std::vector<std::uint8_t> partyOrder = {5, 1, 9, 2, 7, 3};
	XeenPartyState six = XeenPartyLoader().loadFromResources(roster,
		partyFixture(6, 6, {5, 1, 9, 2, 7, 3}));

	for (const std::uint8_t rosterId : partyOrder)
		prepareHpCharacter(six.roster.at(rosterId), context);

	const int maxHp = XeenCharacterRules::maxHp(six.roster.at(partyOrder[0]), context);
	check(maxHp >= 8, "synthetic HP layout fixture maximum");
	six.roster.at(partyOrder[0]).currentHp = static_cast<std::int16_t>(maxHp);
	six.roster.at(partyOrder[1]).currentHp = static_cast<std::int16_t>(maxHp - 1);
	six.roster.at(partyOrder[2]).currentHp = static_cast<std::int16_t>(maxHp / 4 - 1);
	six.roster.at(partyOrder[3]).currentHp = static_cast<std::int16_t>(maxHp + 1);
	six.roster.at(partyOrder[4]).currentHp = 0;
	six.roster.at(partyOrder[5]).currentHp = static_cast<std::int16_t>(maxHp);

	const auto placements = CloudsUiComposer::buildHpPlacements(six, context);
	const int expectedX[] = {13, 50, 86, 122, 158, 194};
	const std::size_t expectedFrames[] = {0, 1, 2, 3, 4, 0};
	check(placements.size() == 6, "six active members must produce six HP placements");
	for (std::size_t i = 0; i < placements.size(); ++i) {
		check(placements[i].partySlot == i &&
			placements[i].rosterId == partyOrder[i] &&
			placements[i].x == expectedX[i] && placements[i].y == 182 &&
			placements[i].frame == expectedFrames[i] &&
			placements[i].frame == XeenPartyVisualState::hpFrame(
				six.party.member(six.roster, i), context),
			"HP placement order, coordinate or visual frame");
	}

	XeenPartyState one = XeenPartyLoader().loadFromResources(roster,
		partyFixture(1, 1, {18}));
	prepareHpCharacter(one.roster.at(18), context);
	one.roster.at(18).currentHp = static_cast<std::int16_t>(
		XeenCharacterRules::maxHp(one.roster.at(18), context));
	const auto onePlacement = CloudsUiComposer::buildHpPlacements(one, context);
	check(onePlacement.size() == 1 && onePlacement[0].partySlot == 0 &&
		onePlacement[0].rosterId == 18 && onePlacement[0].x == 13 &&
		onePlacement[0].y == 182,
		"one member must produce only the first HP placement");

	XeenPartyState four = XeenPartyLoader().loadFromResources(roster,
		partyFixture(4, 4, {4, 8, 12, 16}));
	for (const std::uint8_t rosterId : four.party.activeRosterIds())
		prepareHpCharacter(four.roster.at(rosterId), context);
	const auto fourPlacements = CloudsUiComposer::buildHpPlacements(four, context);
	check(fourPlacements.size() == 4 && fourPlacements.back().partySlot == 3 &&
		fourPlacements.back().rosterId == 16 && fourPlacements.back().x == 122 &&
		fourPlacements.back().y == 182,
		"four members must produce four ordered HP placements and no empty slots");
}

} // namespace

int main() {
	try {
		testCharacterOffsetsAndBounds();
		testPartyHeaderAndReferences();
		testItemStorage();
		testPortraitLayoutAndConditions();
		testHpLayout();
		std::cout << "CHR/PTY: offsets, validation, roster references and portrait layout OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
