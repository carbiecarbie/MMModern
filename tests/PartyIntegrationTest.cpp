#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenPartyLoader.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace mmodern;

namespace {

struct ExpectedCharacter {
	std::uint8_t rosterId;
	const char *name;
	XeenCharacterClass characterClass;
	std::int16_t currentHp;
	int maxHp;
	std::int16_t currentSp;
	int maxSp;
	const char *portrait;
};

constexpr std::array<ExpectedCharacter, 6> kExpected = {{
	{0,  "Arturius", XeenCharacterClass::Paladin,  12, 12, 2, 2, "char01.fac"},
	{18, "Tyro",     XeenCharacterClass::Knight,   16, 16, 0, 0, "char19.fac"},
	{14, "Badger",   XeenCharacterClass::Ranger,   12, 12, 2, 2, "char15.fac"},
	{11, "Zippo",    XeenCharacterClass::Robber,   10, 10, 0, 0, "char12.fac"},
	{1,  "Rebecca",  XeenCharacterClass::Cleric,    7,  7, 7, 7, "char02.fac"},
	{6,  "Seymour",  XeenCharacterClass::Sorcerer,  5,  5, 9, 9, "char07.fac"}
}};

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

} // namespace

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: mmodern_party_smoke <game directory>\n";
		return 1;
	}
	try {
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(), "Clouds installation unavailable");
		XeenAssetSource assets(*installation);
		const auto rosterBytes = assets.readInitialResource("maze.chr");
		const auto partyBytes = assets.readInitialResource("maze.pty");
		check(rosterBytes.size() == 10620, "unexpected real maze.chr size");
		check(partyBytes.size() == 812, "unexpected real maze.pty size");
		const XeenPartyState state = XeenPartyLoader().loadFromResources(rosterBytes, partyBytes);
		check(state.party.size() == kExpected.size(), "unexpected real active party size");
		const XeenCharacterRulesContext rulesContext{kCloudsInitialYear};
		check(rulesContext.currentYear == 610, "unexpected Clouds initial year");
		const auto placements = CloudsUiComposer::buildPortraitPlacements(state);
		const auto hpPlacements = CloudsUiComposer::buildHpPlacements(state, rulesContext);
		check(placements.size() == kExpected.size(), "unexpected real portrait count");
		check(hpPlacements.size() == kExpected.size(), "unexpected real HP indicator count");
		constexpr std::array<int, 6> kExpectedHpX = {13, 50, 86, 122, 158, 194};
		for (std::size_t i = 0; i < kExpected.size(); ++i) {
			const XeenCharacter &character = state.party.member(state.roster, i);
			const ExpectedCharacter &expected = kExpected[i];
			check(character.rosterId == expected.rosterId && character.name == expected.name &&
				character.characterClass == expected.characterClass && character.currentLevel() == 1 &&
				character.currentHp == expected.currentHp &&
				XeenCharacterRules::maxHp(character, rulesContext) == expected.maxHp &&
				character.currentSp == expected.currentSp &&
				XeenCharacterRules::maxSp(character, rulesContext) == expected.maxSp &&
				character.worstCondition() == XeenCondition::Good,
				"real initial character HP/SP differs from expected Clouds roster");
			check(placements[i].resourceName == expected.portrait && placements[i].frame == 0,
				"real initial portrait order/frame differs from expected Clouds party");
			check(hpPlacements[i].partySlot == i &&
				hpPlacements[i].rosterId == expected.rosterId &&
				hpPlacements[i].frame == 0 && hpPlacements[i].x == kExpectedHpX[i] &&
				hpPlacements[i].y == 182,
				"real initial HP indicator order/frame/position differs from expected Clouds party");
		}
		std::cout << "Real Clouds party: current/max HP/SP and portrait order OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
