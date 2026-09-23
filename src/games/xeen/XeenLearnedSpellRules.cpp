#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenCharacterRules.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <zlib.h>

namespace mmodern {
namespace {
// Adapted from ScummVM 6814ee9ba54582f5b5adcffab49efbbd8f589edd,
// devtools/create_mm/create_xeen/constants.cpp, SPELLS_ALLOWED. ScummVM
// contributors are credited in upstream COPYRIGHT; GPL-3.0-or-later.
constexpr std::array<std::array<std::uint8_t, 39>, 3> kAllowed{{
	{{0,1,2,3,5,6,7,8,9,10,12,14,16,23,26,27,28,30,31,32,33,42,46,48,49,50,52,55,56,58,59,62,64,65,67,68,71,73,74}},
	{{1,4,11,13,15,17,18,19,20,21,22,24,25,29,34,35,36,37,38,39,40,41,42,43,44,45,47,51,53,54,57,60,61,63,66,69,70,72,75}},
	{{0,1,2,3,4,5,7,9,10,20,25,26,27,28,30,31,34,38,40,41,42,43,44,45,49,50,52,53,55,59,60,61,62,67,68,72,73,74,75}}
}};

std::size_t ownerFor(const XeenPartyState &party, std::size_t activeIndex) {
	if (activeIndex >= party.party.size()) throw std::invalid_argument("Invalid spell target index");
	const auto owner = party.party.activeRosterIds()[activeIndex];
	if (owner >= XeenRoster::kCharacterCount) throw std::invalid_argument("Invalid spell target owner");
	return owner;
}
}

XeenLearnedSpellNames XeenLearnedSpellNames::parse(std::vector<std::uint8_t> bytes) {
	if (bytes.size() != 937 || crc32(0, bytes.data(), static_cast<uInt>(bytes.size())) != 0x63568f11UL)
		throw std::invalid_argument("Incompatible DARK.CC/spells.xen resource");
	XeenLearnedSpellNames result;
	std::size_t offset=0;
	for (auto &name:result.names) {
		const auto begin=offset;
		while (offset<bytes.size() && bytes[offset]) ++offset;
		if (offset==bytes.size() || offset==begin || offset-begin>63)
			throw std::invalid_argument("Malformed DARK.CC/spells.xen name");
		name.assign(reinterpret_cast<const char *>(bytes.data()+begin),offset-begin);
		++offset;
	}
	if (offset!=bytes.size()) throw std::invalid_argument("Trailing DARK.CC/spells.xen data");
	result.raw=std::move(bytes);
	return result;
}

std::optional<XeenSpellCategory> XeenLearnedSpellRules::categoryForClass(XeenCharacterClass value) noexcept {
	switch (value) {
	case XeenCharacterClass::Paladin: case XeenCharacterClass::Cleric: return XeenSpellCategory::Clerical;
	case XeenCharacterClass::Archer: case XeenCharacterClass::Sorcerer: return XeenSpellCategory::Wizardry;
	case XeenCharacterClass::Druid: case XeenCharacterClass::Ranger: return XeenSpellCategory::Druidic;
	default: return std::nullopt;
	}
}

std::optional<std::uint8_t> XeenLearnedSpellRules::spellForSlot(XeenSpellCategory category,
		std::size_t slot) noexcept {
	const auto index = static_cast<std::size_t>(category);
	if (index >= kAllowed.size() || slot >= kAllowed[index].size()) return std::nullopt;
	return kAllowed[index][slot];
}

std::optional<XeenLearnedSpell> XeenLearnedSpellRules::supported(std::uint8_t id) noexcept {
	if (id == 1) return XeenLearnedSpell::Awaken;
	if (id == 26) return XeenLearnedSpell::FirstAid;
	return std::nullopt;
}

bool XeenLearnedSpellRules::known(const XeenCharacter &character, std::size_t slot) noexcept {
	return slot < 39 && character.learnedSpells && (*character.learnedSpells)[slot] != 0;
}

bool XeenLearnedSpellRules::eligible(const XeenPartyState &party, std::size_t activeIndex,
		std::size_t slot) noexcept {
	if (activeIndex >= party.party.size()) return false;
	const auto owner = party.party.activeRosterIds()[activeIndex];
	if (owner >= XeenRoster::kCharacterCount) return false;
	const auto &character = party.roster.at(owner);
	const auto category = categoryForClass(character.characterClass);
	const auto spell = category ? spellForSlot(*category, slot) : std::nullopt;
	return character.hasSpells && character.canAct() && character.currentSp >= 1 &&
		known(character, slot) && spell && supported(*spell).has_value();
}

XeenSpellPreparation XeenLearnedSpellRules::prepareFirstAid(const XeenPartyState &party,
		std::size_t targetIndex, std::uint32_t year) {
	const auto owner = ownerFor(party, targetIndex);
	const auto &target = party.roster.at(owner);
	XeenSpellPreparation result;
	if (target.conditions[13] || target.conditions[14] || target.conditions[15]) {
		result.failed = true;
		return result;
	}
	XeenCharacterRules::validateForUse(target, {year});
	const int maximum = XeenCharacterRules::maxHp(target, {year});
	const auto hp = target.currentHp;
	const auto after = hp <= maximum ? std::min(static_cast<std::int64_t>(hp) + 6,
		static_cast<std::int64_t>(maximum)) : static_cast<std::int64_t>(hp);
	if (after < std::numeric_limits<std::int16_t>::min() ||
			after > std::numeric_limits<std::int16_t>::max())
		throw std::invalid_argument("First Aid HP exceeds storage domain");
	XeenSpellEffect effect;
	effect.owner = static_cast<std::uint8_t>(owner);
	effect.hp = static_cast<std::int16_t>(after);
	effect.conditions = target.conditions;
	if (after > 0) effect.conditions[12] = 0;
	result.effects.push_back(effect);
	return result;
}

XeenSpellPreparation XeenLearnedSpellRules::prepareAwaken(const XeenPartyState &party) {
	XeenSpellPreparation result;
	std::array<bool, XeenRoster::kCharacterCount> seen{};
	result.effects.reserve(party.party.size());
	for (std::size_t index = 0; index < party.party.size(); ++index) {
		const auto owner = ownerFor(party, index);
		if (seen[owner]) continue;
		seen[owner] = true;
		const auto &target = party.roster.at(owner);
		XeenSpellEffect effect;
		effect.owner = static_cast<std::uint8_t>(owner);
		effect.hp = target.currentHp;
		effect.conditions = target.conditions;
		effect.conditions[8] = 0;
		if (effect.hp > 0) effect.conditions[12] = 0;
		result.effects.push_back(effect);
	}
	return result;
}

} // namespace mmodern
