#include "games/xeen/XeenCompletedDomain.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenStateEquality.h"
#include <algorithm>
#include <stdexcept>

namespace mmodern {
namespace {
void require(bool value, const char *message) {
	if (!value) throw std::invalid_argument(message);
}
}

void xeenValidateCompletedParty(const XeenPartyState &party, const XeenPartyState &initial,
		const std::array<XeenCombatInputs, 6> &inputs, const XeenMonsterRecord &monster) {
	require(party.party.activeRosterIds() == initial.party.activeRosterIds() &&
		party.firstSerializedCount == 6 && party.effectiveSerializedCount == 6 && party.roster.combatMarked(),
		"completed party membership/metadata mismatch");
	using Item = std::array<unsigned, 4>;
	std::vector<Item> expectedItems, savedItems;
	unsigned eligible = 0, able = 0;
	for (auto owner : kXeenCombatOwners) {
		const auto &c = party.roster.at(owner);
		eligible += xeenCombatXpEligible(c.worstCondition());
		able += c.canAct();
	}
	require(able != 0, "completed victory has no able owner");
	for (unsigned owner = 0; owner < XeenRoster::kCharacterCount; ++owner) {
		const auto pos = std::find(kXeenCombatOwners.begin(), kXeenCombatOwners.end(), owner);
		const auto &c = party.roster.at(owner), &base = initial.roster.at(owner);
		if (pos == kXeenCombatOwners.end()) {
			require(xeen_state::sameCharacter(c, base) && !party.roster.combatInputs(owner),
				"completed inactive owner differs from initial CHR");
			continue;
		}
		auto immutable = c;
		immutable.currentHp = base.currentHp; immutable.conditions = base.conditions;
		immutable.weapons = base.weapons; immutable.armor = base.armor;
		immutable.accessories = base.accessories; immutable.miscellaneous = base.miscellaneous;
		require(xeen_state::sameCharacter(immutable, base), "completed immutable character differs from CHR");
		XeenCharacterRules::validateForUse(c, {610});
		require(c.currentHp >= -23 && c.currentHp <= base.currentHp, "completed HP outside admitted injury range");
		for (unsigned i = 0; i < c.conditions.size(); ++i)
			require((i == 12 || i == 13) ? c.conditions[i] <= 1 : c.conditions[i] == 0,
				"unsupported completed condition");
		const int threshold = XeenCharacterRules::maxHp(c, {610}) + c.currentHp;
		require(c.conditions[13] ? threshold <= 0 : c.conditions[12] ? c.currentHp <= 0 && threshold > 0 : c.currentHp > 0,
			"completed HP/conditions disagree");
		xeenValidateCompletedEquipment(c);
		for (unsigned category = 0; category < 4; ++category) {
			const auto cat = static_cast<XeenInventoryCategory>(category);
			for (const auto &item : *xeenInventoryItems(base, cat))
				if (item.id) expectedItems.push_back({category, item.material, item.id, item.state});
			for (const auto &item : *xeenInventoryItems(c, cat)) {
				if (!item.id) {
					require(item.material == 0 && item.state == 0 && item.frame == 0, "completed empty item metadata changed");
					continue;
				}
				if (item.state & 0x80)
					require(category == 1 && item.frame != 0, "completed breakage is not equipped armor");
				savedItems.push_back({category, item.material, item.id, unsigned(item.state & (category == 1 ? 0x7f : 0xff))});
			}
		}
		const auto &saved = party.roster.combatInputs(owner);
		require(bool(saved), "completed supplemental owner missing");
		auto expected = inputs[pos - kXeenCombatOwners.begin()];
		if (xeenCombatXpEligible(c.worstCondition()))
			expected.experience = xeenCombatExperience(monster.experience(), eligible, c.permanentLevel, expected.experience);
		require(xeen_state::sameInputs(*saved, expected), "completed CHR inputs or earned XP mismatch");
	}
	std::sort(expectedItems.begin(), expectedItems.end()); std::sort(savedItems.begin(), savedItems.end());
	require(expectedItems == savedItems, "completed item multiset is not conserved");
}

void xeenApplyCompletedOverlay(std::vector<XeenActor> &actors, XeenMonsterIdentity monster) {
	require(actors.size() == 27 && monster == XeenMonsterIdentity{{XeenSide::Clouds, 20}, 5} &&
		actors[5].id == monster && actors[5].original.resourceId == 8 && actors[5].statistics,
		"completed actor resource identity mismatch");
	actors[5].statistics->validateCombat();
	auto &target = actors[5];
	target.hp = 0; target.x = target.y = -128; target.activated = false;
	target.status = XeenActorStatus::Physical; target.lifecycle = XeenActorLifecycle::Defeated;
}

XeenCompletedEncounterAuthority xeenCompletedPreimage(const XeenWorld &world,
		const XeenPartyState &party, const XeenCamera &camera) {
	XeenCompletedEncounterAuthority a;
	a.party = &party; a.roster = &party.roster; a.camera = &camera;
	a.characters = party.roster.characters();
	for (unsigned i = 0; i < a.combatInputs.size(); ++i) a.combatInputs[i] = party.roster.combatInputs(i);
	a.activeRosterIds = party.party.activeRosterIds(); a.questItems = party.questItems.counts();
	a.questFlags = party.questFlags.values(); a.context = party.encounterContext;
	a.firstSerializedCount = party.firstSerializedCount; a.effectiveSerializedCount = party.effectiveSerializedCount;
	a.diagnostics = party.diagnostics; a.cameraValue = camera;
	a.actors = world.sessionState().actors(); a.objects = world.sessionState().disabledObjects();
	a.events = world.sessionState().disabledEvents(); a.monster = world.sessionState().completedMonster();
	return a;
}
}
