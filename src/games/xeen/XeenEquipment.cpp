#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenPartyLoader.h"
#include <array>
#include <stdexcept>

namespace mmodern {
namespace {
using Category = XeenInventoryCategory;
using Status = XeenEquipmentStatus;

// Adapted from ScummVM, copyright its developers (see upstream COPYRIGHT),
// GPL-3.0-or-later, without warranty. Corresponding source:
// https://github.com/scummvm/scummvm/tree/6814ee9ba54582f5b5adcffab49efbbd8f589edd
// devtools/create_mm/create_xeen/constants.cpp:
// LangConstants::ITEM_RESTRICTIONS and RESTRICTION_OFFSETS {0,35,49,60}.
// Local [id-1] maps to upstream [id] for weapons, [id+35] for armor.
// Only these 42 numeric masks are imported. Legality predicates follow
// engines/mm/xeen/item.cpp: InventoryItems::passRestrictions/removeItem,
// WeaponItems/ArmorItems/AccessoryItems::equipItem. See docs/dependencies.md.
constexpr std::array<std::uint8_t, 34> kWeaponRestrictions = {
	86,86,86,86,86,86,0,6,239,239,239,2,4,4,4,4,6,
	70,70,70,70,94,70,0,4,239,86,86,86,70,70,70,70,0
};
constexpr std::array<std::uint8_t, 8> kArmorRestrictions = {
	0,68,100,116,125,255,255,85
};

std::optional<XeenEquipmentPosition> firstConflict(const XeenCharacter &character,
		Category category, std::uint8_t frame, std::uint8_t alternateFrame) {
	const auto &items = *xeenInventoryItems(character, category);
	for (std::size_t i = 0; i < items.size(); ++i)
		if (items[i].frame == frame || items[i].frame == alternateFrame)
			return XeenEquipmentPosition{category, i};
	return std::nullopt;
}

XeenEquipmentValues modeledValues(const XeenCharacter &character) {
	const XeenCharacterRulesContext context{kCloudsInitialYear};
	return {XeenCharacterRules::effectiveIntellect(character, context),
		XeenCharacterRules::effectivePersonality(character, context),
		XeenCharacterRules::effectiveEndurance(character, context),
		XeenCharacterRules::maxHp(character, context),
		XeenCharacterRules::maxSp(character, context)};
}
} // namespace

XeenEquipmentResult xeenSetEquipment(XeenPartyState &party, std::size_t activeIndex,
		Category category, std::size_t physicalSlot, XeenEquipmentOperation operation) {
	XeenEquipmentResult result;
	if (operation == XeenEquipmentOperation::Equip || operation == XeenEquipmentOperation::Remove)
		result.operation = operation;
	const auto refuse = [&result](Status status) { result.status = status; return result; };
	const auto &ids = party.party.activeRosterIds();
	if (ids.empty()) return refuse(Status::EmptyParty);
	if (activeIndex >= ids.size()) return refuse(Status::InvalidParticipant);
	if (ids[activeIndex] >= XeenRoster::kCharacterCount) return refuse(Status::InvalidOwner);
	auto &character = party.roster.at(ids[activeIndex]);
	if (character.rosterId != ids[activeIndex]) return refuse(Status::InvalidOwner);
	result.owner = ids[activeIndex];
	if (!result.operation) return refuse(Status::InvalidOperation);
	if (category != Category::Weapons && category != Category::Armor && category != Category::Accessories)
		return refuse(Status::InvalidCategory);
	auto &items = *xeenInventoryItems(character, category);
	if (physicalSlot >= items.size()) return refuse(Status::InvalidSlot);
	result.selection = XeenEquipmentPosition{category, physicalSlot};
	auto &selected = items[physicalSlot];
	result.beforeItem = selected;
	if (!selected.id) return refuse(Status::EmptySource);

	std::uint8_t candidateFrame = 0;
	if (operation == XeenEquipmentOperation::Remove) {
		if (selected.state & 0x40) return refuse(Status::Cursed);
	} else {
		const auto id = selected.id;
		if ((category == Category::Weapons && id > 34) ||
				(category == Category::Armor && id > 13) ||
				(category == Category::Accessories && id > 10))
			return refuse(Status::UnsupportedItem);
		if (category == Category::Weapons || (category == Category::Armor && id <= 8)) {
			const auto c = static_cast<unsigned>(character.characterClass);
			if (c > 9) return refuse(Status::UnsafeRules);
			const auto mask = category == Category::Weapons ? kWeaponRestrictions[id - 1] : kArmorRestrictions[id - 1];
			if (c >= 2 && (mask & (1u << (c - 2)))) return refuse(Status::NotProficient);
		}
		if (category == Category::Weapons)
			candidateFrame = id <= 17 ? 1 : (id >= 30 && id <= 33 ? 4 : 13);
		else if (category == Category::Armor)
			candidateFrame = id <= 7 ? 3 : id == 8 ? 2 : id == 9 ? 5 : id == 10 ? 9 : id <= 12 ? 10 : 6;
		else
			candidateFrame = id == 1 ? 8 : id == 2 ? 12 : id <= 7 ? 7 : 11;

		if (category == Category::Accessories && (candidateFrame == 8 || candidateFrame == 7)) {
			std::size_t count = 0;
			for (const auto &item : items) if (item.frame == candidateFrame) ++count;
			if (count >= 2) {
				result.matchingFrameCount = count;
				return refuse(candidateFrame == 8 ? Status::RingLimit : Status::MedalLimit);
			}
		} else {
			const bool melee = category == Category::Weapons && candidateFrame != 4;
			result.conflict = firstConflict(character, category, melee ? 1 : candidateFrame,
				melee ? 13 : candidateFrame);
			if (result.conflict) return refuse(Status::Conflict);
		}
		if (category == Category::Weapons && candidateFrame == 13)
			result.conflict = firstConflict(character, Category::Armor, 2, 2);
		else if (category == Category::Armor && candidateFrame == 2)
			result.conflict = firstConflict(character, Category::Weapons, 13, 13);
		if (result.conflict) return refuse(Status::Conflict);
	}

	auto candidate = character;
	(*xeenInventoryItems(candidate, category))[physicalSlot].frame = candidateFrame;
	try {
		XeenCharacterRules::validateForUse(character, {kCloudsInitialYear});
		XeenCharacterRules::validateForUse(candidate, {kCloudsInitialYear});
	} catch (const std::invalid_argument &) { return refuse(Status::UnsafeRules); }
	result.modeled = XeenEquipmentChange{modeledValues(character), modeledValues(candidate)};
	result.afterItem = (*xeenInventoryItems(candidate, category))[physicalSlot];
	result.status = candidateFrame == selected.frame ? Status::NoChange : Status::Success;
	if (result.status == Status::NoChange) return result;
	// Sole durable publication. All fallible preparation and result construction
	// are complete; the fixed result's copy/move operations cannot throw.
	selected.frame = candidateFrame;
	return result;
}
} // namespace mmodern
