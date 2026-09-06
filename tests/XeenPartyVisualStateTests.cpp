#include "games/xeen/XeenPartyVisualState.h"

#include <iostream>
#include <stdexcept>

using namespace mmodern;

namespace {

void checkFrame(int currentHp, int maxHp, std::size_t expectedFrame) {
	const std::size_t actualFrame = XeenPartyVisualState::selectHpFrame(currentHp, maxHp);
	if (actualFrame != expectedFrame)
		throw std::runtime_error("unexpected classic HP indicator frame");
}

void testExactHpFrameBoundaries() {
	checkFrame(100, 100, 0);
	checkFrame(99, 100, 1);
	checkFrame(25, 100, 1);
	checkFrame(24, 100, 2);
	checkFrame(101, 100, 3);
	checkFrame(0, 100, 4);
	checkFrame(-1, 100, 4);
	checkFrame(0, 0, 4);
	checkFrame(1, 0, 3);
	checkFrame(3, 12, 1);
	checkFrame(2, 12, 2);
}

void testCharacterRulesIntegration() {
	XeenCharacter character;
	character.characterClass = XeenCharacterClass::Knight;
	character.race = XeenRace::Human;
	character.permanentLevel = 1;
	character.endurance.permanent = 13;
	const XeenCharacterRulesContext context{610};
	const int calculatedMaxHp = XeenCharacterRules::maxHp(character, context);

	character.currentHp = static_cast<std::int16_t>(calculatedMaxHp);
	if (XeenPartyVisualState::hpFrame(character, context) != 0)
		throw std::runtime_error("hpFrame did not use XeenCharacterRules maximum HP");

	character.currentHp = static_cast<std::int16_t>(calculatedMaxHp + 1);
	if (XeenPartyVisualState::hpFrame(character, context) != 3)
		throw std::runtime_error("hpFrame did not compare against XeenCharacterRules maximum HP");
}

} // namespace

int main() {
	try {
		testExactHpFrameBoundaries();
		testCharacterRulesIntegration();
		std::cout << "Classic HP indicator visual rules OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
