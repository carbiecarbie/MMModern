#ifndef MMODERN_TESTS_PARTY_SNAPSHOT_SUPPORT_H
#define MMODERN_TESTS_PARTY_SNAPSHOT_SUPPORT_H
#include "games/xeen/XeenPartyLoader.h"
#include <sstream>
namespace remove_test {
using namespace mmodern;
// Compare all currently modeled character/member state, excluding quest counters.
inline std::string partySnapshot(const XeenPartyState &p) {
	std::ostringstream out;
	for(auto id:p.party.activeRosterIds())out<<+id<<',';
	out<<+p.firstSerializedCount<<','<<+p.effectiveSerializedCount;
	for(const auto &d:p.diagnostics)out<<d<<'\n';
	for(const auto &c:p.roster.characters()){
		out<<+c.rosterId<<c.name<<int(c.sex)<<int(c.race)<<int(c.characterClass);
		for(auto a:{c.intellect,c.personality,c.endurance})out<<a.permanent<<','<<a.temporary<<',';
		out<<c.permanentLevel<<','<<c.temporaryLevel<<','<<c.temporaryAge<<','<<c.birthYear<<','
			<<c.currentHp<<','<<c.currentSp<<','<<c.hasSpells<<c.maxStatSkills.astrologer
			<<c.maxStatSkills.bodybuilder<<c.maxStatSkills.prayerMaster<<c.maxStatSkills.prestidigitation;
		for(auto v:c.conditions)out<<+v<<',';
		for(const auto *items:{&c.weapons,&c.armor,&c.accessories})
			for(auto item:*items)out<<+item.material<<','<<+item.state<<','<<+item.frame<<',';
	}
	return out.str();
}

}
#endif
