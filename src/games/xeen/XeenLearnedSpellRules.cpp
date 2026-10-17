#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenCharacterRules.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

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
	if (bytes.empty() || bytes.size()>XeenLearnedSpellNames{}.names.size()*64u)
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
    switch (id) {
    case 1: case 10: case 20: case 26: case 27: case 40: case 45:
    case 2: case 17: case 33: case 38: case 61: case 62: case 72: case 73:
    case 50: case 59: case 60: case 75: return static_cast<XeenLearnedSpell>(id);
    default: return std::nullopt;
    }
}

std::optional<XeenLearnedSpell> XeenLearnedSpellRules::supportedIn(std::uint8_t id, bool combat) noexcept {
    if (combat && (id==40 || id==75)) return std::nullopt;
    return supported(id);
}
unsigned XeenLearnedSpellRules::cost(XeenLearnedSpell spell) noexcept {
    return cost(static_cast<std::uint8_t>(spell),1).sp;
}

// Pinned Spells::calcSpellPoints/calcSpellCost and constants.cpp cost tables.
// ScummVM developers (upstream COPYRIGHT), GPL-3.0-or-later, 6814ee9b.
namespace {
constexpr int kSp[]{8,1,5,-2,5,-2,20,10,12,8,3,-3,75,40,12,6,200,10,100,30,-1,30,15,25,10,-2,1,2,7,20,-2,-2,100,15,5,100,35,75,5,20,4,5,1,-2,6,2,75,40,60,6,4,25,-2,-2,60,-1,50,15,125,2,-1,3,-1,200,35,150,15,5,4,10,8,30,4,5,7,5,0};
constexpr unsigned kGems[]{0,0,2,1,2,4,5,0,0,0,0,10,10,10,0,0,20,4,10,20,1,10,5,5,4,2,0,0,0,10,3,1,20,4,0,20,10,10,1,10,0,0,0,2,2,0,10,10,10,0,0,10,3,2,10,1,10,10,20,0,0,1,1,20,5,20,5,0,0,0,0,5,1,2,0,2,0};
}
XeenSpellCost XeenLearnedSpellRules::cost(std::uint8_t id,unsigned level) noexcept {
    if(id>=77)return {};
    return {kSp[id]>=0?unsigned(kSp[id]):level*unsigned(-kSp[id]),kGems[id]};
}
std::optional<std::size_t> XeenLearnedSpellRules::slotForSpell(XeenSpellCategory category,std::uint8_t id) noexcept {
    for(std::size_t slot=0;slot<39;++slot)if(spellForSlot(category,slot)==id)return slot;
    return {};
}
std::uint32_t XeenLearnedSpellRules::purchasePrice(std::uint8_t id,XeenCharacterClass c) noexcept {
    if(id>=76)return 0;
    const unsigned base=kSp[id]<0?500*unsigned(-kSp[id]):100*unsigned(kSp[id]);
    return base*((c==XeenCharacterClass::Paladin || c==XeenCharacterClass::Archer || c==XeenCharacterClass::Ranger)?2:1);
}
std::vector<std::size_t> XeenLearnedSpellRules::rows(const XeenCharacter &c,bool guild,bool info) {
    std::vector<std::size_t> result;const auto category=categoryForClass(c.characterClass);
    if(!category || !c.learnedSpells)return result;
    if(guild) {
        constexpr std::uint8_t catalogue[]{1,10,20,26,27,38,40,42,45,50,55,59,60,61,62,68,72,75};
        for(auto id:catalogue)if(const auto slot=slotForSpell(*category,id);slot && (info || !known(c,*slot)))result.push_back(*slot);
    } else for(std::size_t slot=0;slot<39;++slot)if(known(c,slot))result.push_back(slot);
    return result;
}
bool XeenLearnedSpellRules::group(std::uint8_t id) noexcept {
    switch(id) {
    case 2: case 33: case 38: case 50: case 60: case 61: case 62: case 72: case 73: return true;
    default: return false;
    }
}
XeenSpellTarget XeenLearnedSpellRules::target(std::uint8_t id) noexcept {
    switch (id) {
    case 1: case 75: return XeenSpellTarget::Party;
    case 10: case 26: case 59: return XeenSpellTarget::Member;
    case 2: case 17: case 33: case 38: case 61: case 62: case 72: case 73:
    case 20: case 27: case 45: case 50: case 60: return XeenSpellTarget::Contact;
    case 40: return XeenSpellTarget::Relocation;
    default: return XeenSpellTarget::Unsupported;
    }
}
XeenQuickSpellCheck XeenLearnedSpellRules::check(const XeenCharacter &c,std::size_t slot,std::uint32_t gems,bool combat) noexcept {
    using Q=XeenQuickSpellCheck;if(slot==255)return Q::NoSpell;
    const auto category=categoryForClass(c.characterClass);
    const auto id=category?spellForSlot(*category,slot):std::nullopt;
    // Explicit incomplete-engine policy: no paid failure/turn for any missing effect.
    if(!id || !supported(*id) || !c.hasSpells || !known(c,slot))return Q::Unsupported;
    const auto required=cost(*id,c.currentLevel());
    if(c.currentSp<int(required.sp))return Q::InsufficientSp;
    if(gems<required.gems)return Q::InsufficientGems;
    if(combat)for(auto forbidden:{19,21,40,44,66,69,71,75})if(*id==forbidden)return Q::CombatForbidden;
    return Q::Supported;
}
XeenQuickSpellCheck XeenLearnedSpellRules::quickSpellCheck(const XeenCharacter &c,std::uint32_t gems) noexcept {
    return check(c,c.currentSpell,gems,true);
}

bool XeenLearnedSpellRules::known(const XeenCharacter &character, std::size_t slot) noexcept {
	return slot < 39 && character.learnedSpells && (*character.learnedSpells)[slot] != 0;
}

bool XeenLearnedSpellRules::eligible(const XeenPartyState &party, std::size_t activeIndex,
		std::size_t slot, bool combat) noexcept {
	if (activeIndex >= party.party.size()) return false;
	const auto owner = party.party.activeRosterIds()[activeIndex];
	if (owner >= XeenRoster::kCharacterCount) return false;
	const auto &character = party.roster.at(owner);
	const auto category = categoryForClass(character.characterClass);
	const auto spell = category ? spellForSlot(*category, slot) : std::nullopt;
	const auto effect=spell ? supportedIn(*spell,combat) : std::nullopt;
	return character.hasSpells && character.canAct() && effect &&
		character.currentSp >= int(cost(*spell,character.currentLevel()).sp) && known(character, slot);
}

XeenSpellPreparation XeenLearnedSpellRules::prepareEffect(XeenLearnedSpell spell,const XeenPartyState &party,
        std::optional<std::size_t> target,std::uint32_t year) {
    if(spell==XeenLearnedSpell::Awaken)return prepareAwaken(party);
    if(!target)return {};
    switch(spell) {
    case XeenLearnedSpell::FirstAid:return prepareFirstAid(party,*target,year);
    case XeenLearnedSpell::CureWounds:return prepareFirstAid(party,*target,year,15);
    case XeenLearnedSpell::Revitalize:return prepareRevitalize(party,*target,year);
    default:throw std::invalid_argument("Unsupported member spell effect");
    }
}
void XeenLearnedSpellRules::applyPartyEffect(XeenLearnedSpell spell,XeenGameplayContext &context) {
    if(spell==XeenLearnedSpell::WizardEye) {context.effects[2]=1;context.effects[1]=0;}
}

XeenSpellPreparation XeenLearnedSpellRules::prepareFirstAid(const XeenPartyState &party,
		std::size_t targetIndex, std::uint32_t year, unsigned healing) {
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
	const auto after = hp <= maximum ? std::min(static_cast<std::int64_t>(hp) + healing,
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

XeenSpellPreparation XeenLearnedSpellRules::prepareRevitalize(const XeenPartyState &party,
        std::size_t targetIndex, std::uint32_t year) {
    const auto owner = ownerFor(party,targetIndex);
    const auto &target = party.roster.at(owner);
    XeenSpellEffect effect;
    effect.owner=static_cast<std::uint8_t>(owner);
    effect.hp=target.currentHp;
    effect.conditions=target.conditions;
    // addHitPoints(0) skips terminal characters, including its Unconscious repair.
    // Revitalize still clears Weak on them; it is not a failed healing cast.
    if (!target.conditions[13] && !target.conditions[14] && !target.conditions[15]) {
        XeenCharacterRules::validateForUse(target,{year});
        if (effect.hp>0) effect.conditions[12]=0;
    }
    effect.conditions[2]=0;
    XeenSpellPreparation result;
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
