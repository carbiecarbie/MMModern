#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenEquipment.h"
#include <stdexcept>

namespace mmodern {
namespace {
void require(bool value, const char *message) { if (!value) throw std::invalid_argument(message); }
bool byte(int value) { return value >= 0 && value <= 255; }
bool attribute(XeenAttributeValue value) { return byte(value.permanent) && byte(value.temporary); }
void contribution(bool supported, unsigned owner, const char *category, unsigned slot, const XeenItem &item) {
	if (!supported) throw std::invalid_argument("Unsupported Journey equipped " + std::string(category) +
		" contribution: owner " + std::to_string(owner) + ", slot " + std::to_string(slot) +
		", M/ID/S/F=" + std::to_string(item.material) + "/" + std::to_string(item.id) + "/" +
		std::to_string(item.state) + "/" + std::to_string(item.frame));
}
}
void xeenValidateJourneyParty(const XeenPartyState &party) {
	require(party.roster.combatMarked() && party.encounterContext.has_value(), "Journey requires complete owner state");
	const auto &context = *party.encounterContext;
	require(context.profile == XeenBehaviorProfile::WorldOfXeenClouds && context.difficulty == XeenDifficulty::Adventurer &&
		context.day == 1 && context.year == 610 && context.minutes >= 480 && context.minutes < 960 && context.ctr24 < 24 &&
		!context.rested && !context.newDay && context.effects == std::array<std::uint8_t,9>{} &&
		context.lightAndResistances == std::array<std::uint16_t,6>{}, "Unsupported Journey context");
	require(party.party.activeRosterIds() == std::vector<std::uint8_t>(kXeenCombatOwners.begin(), kXeenCombatOwners.end()) &&
		party.firstSerializedCount == 6 && party.effectiveSerializedCount == 6, "Unsupported Journey membership");
	for (unsigned id = 0; id < 30; ++id) {
		const auto &c = party.roster.at(id);
		const auto &input = party.roster.combatInputs(id);
		require(c.rosterId == id && input.has_value(), "Missing Journey owner supplement");
		require(attribute(input->might) && attribute(input->speed) && attribute(input->accuracy) && byte(input->temporaryAc),
			"Journey supplement outside byte range");
		require(c.name.size() <= 16 && c.name.find('\0') == std::string::npos, "Journey character outside storage bounds");
	}
	bool able = false;
	for (auto id : kXeenCombatOwners) {
		const auto &c = party.roster.at(id);
		require(attribute(c.intellect) && attribute(c.personality) && attribute(c.endurance) && byte(c.permanentLevel) &&
			byte(c.temporaryLevel) && byte(c.temporaryAge), "Journey active character outside byte range");
		require(static_cast<unsigned>(c.sex) <= 2 && static_cast<unsigned>(c.race) <= 4 &&
			static_cast<unsigned>(c.characterClass) <= 9 && c.permanentLevel > 0, "Unsupported Journey active rules");
		for (unsigned i = 0; i < c.conditions.size(); ++i)
			require(c.conditions[i] <= ((i == 12 || i == 13) ? 1 : 0), "Unsupported Journey condition");
		require((c.conditions[12] || c.conditions[13]) ? c.currentHp <= 0 : c.currentHp > 0,
			"Journey HP and condition signs disagree");
		XeenCharacterRules::validateForUse(c, {context.year});
		able = able || c.canAct();
	}
	require(able, "Journey has no acting character");
}
void xeenValidateJourneyMelee(const XeenPartyState &party) {
	xeenValidateJourneyParty(party);
	for (auto id : kXeenCombatOwners) {
		const auto &c = party.roster.at(id);
		for (unsigned slot = 0; slot < 9; ++slot) {
			const auto &item = c.weapons[slot];
			if (!item.frame) continue;
			const bool melee = item.frame == 1 || item.frame == 13;
			const bool weapon = item.id == 2 || item.id == 6 || item.id == 7 || item.id == 8 || item.id == 12 || item.id == 15;
			contribution(item.material == 0 && item.state == 0 && ((melee && weapon) || (item.frame == 4 && item.id == 30)),
				id,"weapon",slot,item);
		}
		for (unsigned slot = 0; slot < 9; ++slot) {
			const auto &item = c.armor[slot];
			if (item.frame) contribution(item.id <= 13 && (item.material == 0 || item.material == 38) && (item.state == 0 || item.state == 128),
				id,"armor",slot,item);
		}
		for (unsigned slot = 0; slot < 9; ++slot) {
			const auto &item = c.accessories[slot];
			if (item.frame) contribution(item.id <= 10 && (item.material == 0 || item.material == 38 || item.material == 42 || item.material == 86) &&
				(item.state == 0 || item.state == 128),id,"accessory",slot,item);
		}
		xeenValidateCompletedEquipment(c); // Shared M25 arrangement rules, including the exact legacy medal.
		const auto &input = *party.roster.combatInputs(id);
		using Rules = XeenCharacterRules;
		for (auto attr : {Rules::PhysicalAttribute::Might, Rules::PhysicalAttribute::Speed, Rules::PhysicalAttribute::Accuracy})
			(void)Rules::effectivePhysical(c, input, attr, {party.encounterContext->year});
		(void)Rules::combatArmorClass(c, input, {party.encounterContext->year});
		(void)xeenCombatAttackCount(c.characterClass, c.currentLevel());
	}
}
}
