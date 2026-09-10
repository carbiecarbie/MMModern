#ifndef MMODERN_EQUIPMENT_TEST_SUPPORT_H
#define MMODERN_EQUIPMENT_TEST_SUPPORT_H

#include "games/xeen/XeenEquipment.h"
#include <stdexcept>
#include <tuple>

namespace equipment_test {
using namespace mmodern;
inline void check(bool value, const char *message) {
	if (!value) throw std::runtime_error(message);
}
inline bool sameItem(const XeenItem &a, const XeenItem &b) {
	return a.material == b.material && a.id == b.id && a.state == b.state && a.frame == b.frame;
}
// Independent field-wise oracle: no object padding, serialization or production
// item equality. Shared only by domain tests and the original-data party smoke.
inline auto characterFields(const XeenCharacter &c) {
	return std::tie(c.rosterId, c.name, c.sex, c.race, c.characterClass,
		c.intellect.permanent, c.intellect.temporary, c.personality.permanent,
		c.personality.temporary, c.endurance.permanent, c.endurance.temporary,
		c.permanentLevel, c.temporaryLevel, c.temporaryAge,
		c.maxStatSkills.astrologer, c.maxStatSkills.bodybuilder,
		c.maxStatSkills.prayerMaster, c.maxStatSkills.prestidigitation, c.hasSpells,
		c.currentHp, c.currentSp, c.conditions, c.birthYear);
}
inline void sameParty(const XeenPartyState &a, const XeenPartyState &b) {
	check(a.party.activeRosterIds() == b.party.activeRosterIds() &&
		a.questItems.counts() == b.questItems.counts() && a.questFlags.values() == b.questFlags.values() &&
		a.firstSerializedCount == b.firstSerializedCount && a.effectiveSerializedCount == b.effectiveSerializedCount &&
		a.diagnostics == b.diagnostics, "equipment changed party state/metadata");
	for (std::size_t owner = 0; owner < 30; ++owner) {
		const auto &x = a.roster.at(owner), &y = b.roster.at(owner);
		check(characterFields(x) == characterFields(y), "equipment changed character fields");
		const XeenItemCategory *xs[]{&x.weapons, &x.armor, &x.accessories, &x.miscellaneous};
		const XeenItemCategory *ys[]{&y.weapons, &y.armor, &y.accessories, &y.miscellaneous};
		for (unsigned category = 0; category < 4; ++category)
			for (unsigned slot = 0; slot < 9; ++slot)
				check(sameItem((*xs[category])[slot], (*ys[category])[slot]), "equipment item bytes differ from independent expectation");
	}
}
inline XeenItemCategory &items(XeenCharacter &c, XeenInventoryCategory category) {
	switch (category) {
	case XeenInventoryCategory::Weapons: return c.weapons;
	case XeenInventoryCategory::Armor: return c.armor;
	case XeenInventoryCategory::Accessories: return c.accessories;
	case XeenInventoryCategory::Miscellaneous: return c.miscellaneous;
	}
	throw std::runtime_error("invalid test category");
}
} // namespace equipment_test
#endif
