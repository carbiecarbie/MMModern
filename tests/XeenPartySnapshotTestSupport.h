#ifndef MMODERN_TESTS_PARTY_SNAPSHOT_SUPPORT_H
#define MMODERN_TESTS_PARTY_SNAPSHOT_SUPPORT_H
#include "games/xeen/XeenPartyLoader.h"
#include <sstream>
#include <stdexcept>
namespace remove_test {
using namespace mmodern;
// Typed comparison for durable values. Loading diagnostics/count metadata are
// intentionally not part of this comparison; historical snapshots below stay intact.
inline void checkSameCharacter(const XeenCharacter &a, const XeenCharacter &b) {
	if (a.rosterId != b.rosterId || a.name != b.name || a.sex != b.sex ||
			a.race != b.race || a.characterClass != b.characterClass ||
			a.permanentLevel != b.permanentLevel || a.temporaryLevel != b.temporaryLevel ||
			a.temporaryAge != b.temporaryAge || a.birthYear != b.birthYear ||
			a.currentHp != b.currentHp || a.currentSp != b.currentSp || a.hasSpells != b.hasSpells ||
			a.conditions != b.conditions ||
			a.maxStatSkills.astrologer != b.maxStatSkills.astrologer ||
			a.maxStatSkills.bodybuilder != b.maxStatSkills.bodybuilder ||
			a.maxStatSkills.prayerMaster != b.maxStatSkills.prayerMaster ||
			a.maxStatSkills.prestidigitation != b.maxStatSkills.prestidigitation)
		throw std::runtime_error("modeled character values differ");
	const XeenAttributeValue av[]{a.intellect, a.personality, a.endurance};
	const XeenAttributeValue bv[]{b.intellect, b.personality, b.endurance};
	for (std::size_t i = 0; i < 3; ++i)
		if (av[i].permanent != bv[i].permanent || av[i].temporary != bv[i].temporary)
			throw std::runtime_error("modeled character attributes differ");
	using Modifiers = std::array<XeenItemModifierSource, XeenCharacter::kEquipmentSlotsPerCategory>;
	const Modifiers *ai[]{&a.weapons, &a.armor, &a.accessories};
	const Modifiers *bi[]{&b.weapons, &b.armor, &b.accessories};
	for (std::size_t category = 0; category < 3; ++category)
		for (std::size_t i = 0; i < ai[category]->size(); ++i) {
			const auto x = (*ai[category])[i], y = (*bi[category])[i];
			if (x.material != y.material || x.state != y.state || x.frame != y.frame)
				throw std::runtime_error("modeled character modifiers differ");
		}
}

// Character/member snapshot. Compare BOTH quest collections explicitly below.
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

inline void checkPartyQuestState(const XeenPartyState &party,
		const XeenCloudsQuestItems::Counts &items, const XeenCloudsQuestFlags::Values &flags) {
	if (party.questItems.counts() != items || party.questFlags.values() != flags)
		throw std::runtime_error("unexpected quest counter or quest flag mutation");
}

}
#endif
