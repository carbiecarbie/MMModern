#include "games/xeen/XeenCharacterRules.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace mmodern {
namespace {

// Original constants: devtools/create_mm/create_xeen/constants.cpp.
constexpr std::array<int, 10> kBaseHpByClass = {
	10, 8, 7, 5, 4, 8, 7, 12, 6, 9
};
constexpr std::array<int, 5> kRaceHpBonuses = {0, -2, 1, -1, 2};
constexpr std::array<std::array<int, 2>, 5> kRaceSpBonuses = {{
	{{ 0,  0}},
	{{ 2,  0}},
	{{-1, -1}},
	{{ 1,  1}},
	{{-2, -2}}
}};
constexpr std::array<int, 10> kAgeRanges = {
	1, 6, 11, 18, 36, 51, 76, 101, 201, 65535
};
constexpr std::array<std::array<int, 10>, 2> kAgeAdjustments = {{
	{{-250, -50, -20, -10, 0, -2, -5, -10, -20, -50}},
	{{-250, -50, -20, -10, 0,  2,  5,  10,  20,  50}}
}};
constexpr std::array<int, 24> kStatValues = {
	3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 25, 30,
	35, 40, 50, 75, 100, 125, 150, 175, 200, 225, 250, 65535
};
constexpr std::array<int, 24> kStatBonuses = {
	-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6,
	7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20
};
constexpr std::array<int, 10> kAttributeCategories = {
	9, 17, 25, 33, 39, 45, 50, 56, 61, 72
};
constexpr std::array<int, 72> kAttributeBonuses = {
	2, 3, 5, 8, 12, 17, 23, 30, 38, 47,
	2, 3, 5, 8, 12, 17, 23, 30,
	2, 3, 5, 8, 12, 17, 23, 30,
	2, 3, 5, 8, 12, 17, 23, 30,
	3, 5, 10, 15, 20, 30,
	5, 10, 15, 20, 25, 30,
	4, 6, 10, 20, 50,
	4, 8, 12, 16, 20, 25,
	2, 4, 6, 10, 16,
	4, 6, 8, 10, 12, 14, 16, 18, 20, 25
};

enum class DerivedAttribute {
	Intellect = 1,
	Personality = 2,
	Endurance = 3
};

constexpr int kHpBonusCategory = 7;
constexpr int kSpBonusCategory = 8;
constexpr std::uint8_t kCursedOrBrokenMask = 0xc0;

std::size_t enumIndex(XeenRace race) {
	return static_cast<std::size_t>(race);
}

std::size_t enumIndex(XeenCharacterClass characterClass) {
	return static_cast<std::size_t>(characterClass);
}

std::uint8_t condition(const XeenCharacter &character, XeenCondition value) {
	return character.conditions[static_cast<std::size_t>(value)];
}

template<bool Checked> int add(int a, int b) {
	if constexpr (Checked) {
		const auto result = static_cast<std::int64_t>(a) + b;
		if (result < std::numeric_limits<int>::min() || result > std::numeric_limits<int>::max())
			throw std::invalid_argument("character rule addition exceeds the integer domain");
		return static_cast<int>(result);
	} else {
		return a + b;
	}
}

template<bool Checked> int multiply(int a, int b) {
	if constexpr (Checked) {
		const auto result = static_cast<std::int64_t>(a) * b;
		if (result < std::numeric_limits<int>::min() || result > std::numeric_limits<int>::max())
			throw std::invalid_argument("character rule product exceeds the integer domain");
		return static_cast<int>(result);
	} else {
		return a * b;
	}
}

template<bool Checked>
int currentLevel(const XeenCharacter &character) {
	return std::max(add<Checked>(character.permanentLevel, character.temporaryLevel), 0);
}

template<bool Checked>
int effectiveAge(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	// The original subtracts unsigned years, caps the result, then adds temp age.
	const std::uint32_t baseAge = std::min(context.currentYear - character.birthYear, 254u);
	return add<Checked>(static_cast<int>(baseAge), character.temporaryAge);
}

template<bool Checked>
int ageAdjustment(const XeenCharacter &character,
		const XeenCharacterRulesContext &context, bool mental) {
	const int age = effectiveAge<Checked>(character, context);
	std::size_t index = 0;
	while (index + 1 < kAgeRanges.size() && kAgeRanges[index] <= age)
		++index;
	return kAgeAdjustments[mental ? 1 : 0][index];
}

// DOS WORLD/XEEN.DAT 0x1f9a8..0x1fbe4: unsigned condition counts,
// one attribute point per count. Shared by sheet stats and HP/SP rules.
int conditionModifier(const XeenCharacter &character, unsigned attribute) {
	if (condition(character, XeenCondition::Dead) ||
			condition(character, XeenCondition::Stoned) ||
			condition(character, XeenCondition::Eradicated))
		return 0;

	int result = int(condition(character, XeenCondition::InLove)) -
		condition(character, XeenCondition::HeartBroken) - condition(character, XeenCondition::Weak);
	const int insane = condition(character, XeenCondition::Insane);
	if (attribute == 0 || attribute == 4) result += insane;
	else if (attribute == 1 || attribute == 2 || attribute == 5) result -= insane;
	const int drunk = condition(character, XeenCondition::Drunk);
	result += (attribute == 2 || attribute == 6) ? drunk : -drunk;
	if (attribute == 6) result -= condition(character, XeenCondition::Cursed);
	if (attribute == 0 || attribute == 4 || attribute == 5)
		result -= condition(character, XeenCondition::Poisoned);
	if (attribute == 1 || attribute == 2 || attribute == 3)
		result -= condition(character, XeenCondition::Diseased);
	return result;
}

template<std::size_t N>
int itemBonusFrom(const std::array<XeenItem, N> &items, int category) {
	int result = 0;
	for (const XeenItem &item : items) {
		if (item.frame == 0 || (item.state & kCursedOrBrokenMask) != 0 ||
				item.material < 59 || item.material > 130 || category == 3)
			continue;

		const int materialIndex = item.material - 59;
		std::size_t attributeCategory = 0;
		while (kAttributeCategories[attributeCategory] < materialIndex)
			++attributeCategory;
		int effectiveCategory = static_cast<int>(attributeCategory);
		// Xeen has no equipment category for Endurance.
		if (effectiveCategory > 2)
			++effectiveCategory;
		if (effectiveCategory == category)
			result += kAttributeBonuses[static_cast<std::size_t>(materialIndex)];
	}
	return result;
}

int itemBonus(const XeenCharacter &character, int category) {
	return itemBonusFrom(character.weapons, category) +
		itemBonusFrom(character.armor, category) +
		itemBonusFrom(character.accessories, category);
}

int elementalBonus(const XeenCharacter &character, unsigned element) {
	constexpr unsigned categories[]{8,15,20,25,33,36};
	constexpr int bonuses[]{0,5,7,9,12,15,20,25,30,5,7,9,12,15,20,25,5,10,15,20,25,10,15,20,25,40,5,7,9,11,13,15,20,25,5,10,20};
	int result=0;
	for (const auto *items:{&character.armor,&character.accessories})
		for (const auto &item:*items) if (item.frame && !(item.state&kCursedOrBrokenMask) && item.material<37) {
			unsigned index=0;
			while (categories[index]<item.material) ++index;
			if (index==element) result+=bonuses[item.material];
		}
	return result;
}

int statBonus(int value) {
	std::size_t index = 0;
	while (index + 1 < kStatValues.size() && kStatValues[index] <= value)
		++index;
	return kStatBonuses[index];
}

const XeenAttributeValue &attributeValue(const XeenCharacter &character,
		DerivedAttribute attribute) {
	switch (attribute) {
	case DerivedAttribute::Intellect:
		return character.intellect;
	case DerivedAttribute::Personality:
		return character.personality;
	case DerivedAttribute::Endurance:
		return character.endurance;
	}
	return character.endurance;
}

template<bool Checked>
int effectiveAttribute(const XeenCharacter &character,
		const XeenCharacterRulesContext &context, DerivedAttribute attribute) {
	const XeenAttributeValue &value = attributeValue(character, attribute);
	const bool mental = attribute != DerivedAttribute::Endurance;
	const int equipment = attribute == DerivedAttribute::Endurance ? 0 :
		itemBonus(character, static_cast<int>(attribute));
	int result = add<Checked>(value.permanent, value.temporary);
	result = add<Checked>(result, ageAdjustment<Checked>(character, context, mental));
	result = add<Checked>(result, equipment);
	return std::max(add<Checked>(result, conditionModifier(character, static_cast<unsigned>(attribute))), 0);
}

template<bool Checked>
int spPass(const XeenCharacter &character, const XeenCharacterRulesContext &context,
		DerivedAttribute attribute, bool hasRelevantSkill) {
	const int effective = effectiveAttribute<Checked>(character, context, attribute);
	const std::size_t race = enumIndex(character.race);
	const std::size_t racialColumn = attribute == DerivedAttribute::Intellect ? 0 : 1;
	int base = statBonus(effective) + 3 + kRaceSpBonuses[race][racialColumn];
	if (hasRelevantSkill)
		base += 2;
	base = std::max(base, 1);
	int result = multiply<Checked>(base, currentLevel<Checked>(character));
	if (character.characterClass != XeenCharacterClass::Sorcerer &&
			character.characterClass != XeenCharacterClass::Cleric &&
			character.characterClass != XeenCharacterClass::Druid)
		result /= 2;
	return result;
}

template<bool Checked>
int maximumHp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	int hpPerLevel = kBaseHpByClass[enumIndex(character.characterClass)] +
		statBonus(effectiveAttribute<Checked>(character, context, DerivedAttribute::Endurance)) +
		kRaceHpBonuses[enumIndex(character.race)] +
		(character.skills[3] ? 1 : 0);
	hpPerLevel = std::max(hpPerLevel, 1);
	return std::max(add<Checked>(multiply<Checked>(hpPerLevel, currentLevel<Checked>(character)),
		itemBonus(character, kHpBonusCategory)), 0);
}

template<bool Checked>
int maximumSp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	if (!character.hasSpells)
		return 0;

	int result = 0;
	if (character.characterClass == XeenCharacterClass::Sorcerer ||
			character.characterClass == XeenCharacterClass::Archer) {
		result = spPass<Checked>(character, context, DerivedAttribute::Intellect,
			character.skills[13]);
	} else if (character.characterClass == XeenCharacterClass::Druid ||
			character.characterClass == XeenCharacterClass::Ranger) {
		const int personality = spPass<Checked>(character, context, DerivedAttribute::Personality,
			character.skills[2]);
		const int intellect = spPass<Checked>(character, context, DerivedAttribute::Intellect,
			character.skills[2]);
		result = add<Checked>(personality, intellect) / 2;
	} else {
		// This also preserves the original fallback for anomalous spell-capable records.
		result = spPass<Checked>(character, context, DerivedAttribute::Personality,
			character.skills[12]);
	}

	return std::max(add<Checked>(result, itemBonus(character, kSpBonusCategory)), 0);
}

} // namespace

// Character::getStat/itemScan at the documented ScummVM pin;
// GPL-3.0-or-later, ScummVM developers (COPYRIGHT).
// Condition modifiers use the DOS routine documented above.
int XeenCharacterRules::sheetStat(const XeenCharacter &c,const XeenCombatInputs *input,
		unsigned attribute,const XeenCharacterRulesContext &context,bool baseOnly) {
	if (attribute>=7) throw std::invalid_argument("Invalid sheet attribute");
	std::array<int,2> value{};

	const XeenAttributeValue *v=attribute==1?&c.intellect:attribute==2?&c.personality:attribute==3?&c.endurance:nullptr;
	if (input) {
		if(attribute==0) v=&input->might;
		else if(attribute==4) v=&input->speed;
		else if(attribute==5) v=&input->accuracy;
		else if(attribute==6 && input->luck) v=&*input->luck;
	}
	if(v) value={v->permanent,v->temporary};
	int result=add<true>(value[0],itemBonus(c,attribute));
	if(attribute<6) result=add<true>(result,ageAdjustment<true>(c,context,attribute==1 || attribute==2));
	if(!baseOnly) {
		result=add<true>(result,value[1]);
		result=add<true>(result,conditionModifier(c,attribute));
	}
	return std::max(result,0);
}
int XeenCharacterRules::sheetAge(const XeenCharacter &c,const XeenCharacterRulesContext &context,bool baseOnly) {
	return std::min(context.currentYear-c.birthYear,254u)+(baseOnly?0:int(c.temporaryAge));
}
// Character::itemScan(9): ARMOR_STRENGTHS plus METAL_LAC for good equipped
// armor, and the equipment AC attribute bonus of every category (M67).
int XeenCharacterRules::equipmentArmorClass(const XeenCharacter &c) {
	constexpr int strengths[]{0,2,4,5,6,7,8,10,4,2,1,1,1,1};
	constexpr int metal[]{-3,0,-2,-1,1,2,4,6,8,0,1,1,2,2,3,4,5,10,12,14,16,20};
	int result=itemBonus(c,9);
	for(const auto &item:c.armor) if(item.frame && !(item.state&0xc0)) {
		if(item.id>=14) throw std::invalid_argument("Invalid sheet armor id");
		result=add<true>(result,strengths[item.id]);
		if(item.material>=37 && item.material<=58) result=add<true>(result,metal[item.material-37]);
	}
	return result;
}
int XeenCharacterRules::sheetArmorClass(const XeenCharacter &c,const XeenCombatInputs *input,
		const XeenCharacterRulesContext &context,bool baseOnly) {
	int result=add<true>(statBonus(sheetStat(c,input,4,context)),equipmentArmorClass(c));
	if(!baseOnly) {
        result=add<true>(result,input?int(input->temporaryAc):0);
        result=add<true>(result,context.blessed);
    }
	return std::max(result,0);
}
int XeenCharacterRules::sheetResistance(const XeenCharacter &c,const XeenCombatInputs *input,unsigned resistance) {
	if(resistance>=6) throw std::invalid_argument("Invalid sheet resistance");
	std::array<int,2> value{};

	if(input && input->resistances) {
		if(resistance==1) value={input->resistances->coldPermanent,input->resistances->coldTemporary};
		if(resistance==2) value={input->resistances->electricalPermanent,input->resistances->electricalTemporary};
		if(resistance==0) value={input->resistances->firePermanent,input->resistances->fireTemporary};
		if(resistance==4) value={input->resistances->energyPermanent,input->resistances->energyTemporary};
		if(resistance==5) value={input->resistances->magicPermanent,input->resistances->magicTemporary};
	}
	if(input && input->poisonResistance && resistance==3) value={input->poisonResistance->permanent,input->poisonResistance->temporary};
	constexpr unsigned elements[]{0,2,1,3,4,5};
	return add<true>(add<true>(value[0],value[1]),elementalBonus(c,elements[resistance]));
}
int XeenCharacterRules::statColor(int amount,int threshold) {
	return amount<1?6:amount>threshold?2:amount==threshold?15:amount>=threshold/4?9:32;
}
unsigned XeenCharacterRules::skillCount(const XeenCharacter &c) {
	return std::count_if(c.skills.begin(),c.skills.end(),[](auto n){return n!=0;});
}
unsigned XeenCharacterRules::awardCount(const XeenCharacter &c) {
	unsigned count=0;
	for(unsigned i=0;i<88;++i) if(c.awards[XeenCharacter::awardIndex(i)]) ++count;
	return count;
}
std::uint32_t XeenCharacterRules::currentExperience(const XeenCharacter &c,const XeenCombatInputs *input) {
	constexpr std::uint32_t bases[]{1500,2000,2000,1500,2000,1000,1500,1500,1500,2000};
	const auto xp=input?std::uint32_t(input->experience):0u;
	const int level=c.permanentLevel-1;
	if(level<=0) return xp;
	return xp+(level>=12?std::uint32_t(level-12)*1024000u:0u)+(bases[static_cast<unsigned>(c.characterClass)]<<(level>=12?10:level-1));
}
std::uint32_t XeenCharacterRules::experienceToNextLevel(const XeenCharacter &c,const XeenCombatInputs *input) {
	constexpr std::uint32_t bases[]{1500,2000,2000,1500,2000,1000,1500,1500,1500,2000};
	const int level=c.permanentLevel;
	if(level<1) return 0;
	const auto next=(level>=12?std::uint32_t(level-12)*1024000u:0u)+(bases[static_cast<unsigned>(c.characterClass)]<<(level>=12?10:level-1));
	const auto current=currentExperience(c,input);
	return current>=next?0u:next-current;
}

int XeenCharacterRules::physicalBonus(int value) { return statBonus(value); }
int XeenCharacterRules::effectiveLuck(const XeenCharacter &c, const XeenCombatInputs &input) {
	if (!input.luck) throw std::invalid_argument("Missing physical saving throw Luck");
	return sheetStat(c,&input,6,{0});
}
int XeenCharacterRules::equipmentBonus(const XeenCharacter &c,int category) {
	if(category<0 || category>14)throw std::invalid_argument("Unsupported equipment attribute query");
	return itemBonus(c,category);
}
int XeenCharacterRules::poisonSaveValue(const XeenCharacter &c,const XeenCombatInputs &input) {
	if(!input.poisonResistance)throw std::invalid_argument("Missing poison resistance input");
	return std::max(add<true>(add<true>(input.poisonResistance->permanent,
		input.poisonResistance->temporary),equipmentBonus(c,14)),0);
}
// Character::charSavingThrow/getThievery at the pinned ScummVM revision.
int XeenCharacterRules::damageSaveValue(const XeenCharacter &c,const XeenCombatInputs &input,
		XeenDamageType type,const XeenCharacterRulesContext &context) {
	if (type==XeenDamageType::Physical) {
		if (!input.luck) throw std::invalid_argument("Missing physical saving throw Luck");
		return add<true>(statBonus(sheetStat(c,&input,6,context)),currentLevel<true>(c));
	}
	const auto index=static_cast<unsigned>(type);
	if (index>6) throw std::invalid_argument("Unsupported character damage type");
	// Roster supplement resistance pairs remain authoritative for elements
	// without a live supplement. Missing input is never an implicit zero.
	const unsigned resistance=index==1 ? 5 : index==2 ? 0 : index==3 ? 2 : index==4 ? 1 : index==5 ? 3 : 4;
	const bool live=resistance!=3 ? input.resistances.has_value() :
		resistance==3 && input.poisonResistance.has_value();
	if (!live) throw std::invalid_argument("Missing character damage resistance");
	return sheetResistance(c,&input,resistance);
}
int XeenCharacterRules::thievery(const XeenCharacter &c) {
	if (enumIndex(c.race)>=5 || enumIndex(c.characterClass)>=10)
		throw std::invalid_argument("Invalid Thievery race or class");
	int result=multiply<true>(currentLevel<true>(c),2);
	if (c.characterClass==XeenCharacterClass::Ninja) result=add<true>(result,15);
	else if (c.characterClass==XeenCharacterClass::Robber) result=add<true>(result,30);
	constexpr int racial[]{0,10,5,10,-10};
	result=add<true>(add<true>(result,racial[enumIndex(c.race)]),itemBonus(c,10));
	return c.skills[0] ? std::max(result,0) : 0;
}
int XeenCharacterRules::effectivePhysical(const XeenCharacter &c, const XeenCombatInputs &input,
		PhysicalAttribute attribute, const XeenCharacterRulesContext &context) {
 return sheetStat(c,&input,static_cast<unsigned>(attribute),context);
}
int XeenCharacterRules::combatArmorClass(const XeenCharacter &c, const XeenCombatInputs &input,
		const XeenCharacterRulesContext &context) {
	// ScummVM developers, GPL-3.0-or-later, pin 6814ee9ba54582f5b5adcffab49efbbd8f589edd:
	// Character::getArmorClass = statBonus(Speed) + itemScan(9) + temporary AC
	// + party Blessed unless base-only. The monster hit threshold, the sheet
	// and the injury observations share this one equipment scan.
	return sheetArmorClass(c,&input,context);
}

void XeenCharacterRules::validateForUse(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	if (enumIndex(character.race) >= kRaceHpBonuses.size() ||
			enumIndex(character.characterClass) >= kBaseHpByClass.size())
		throw std::invalid_argument("active character has an unsupported race or class");
	static_cast<void>(currentLevel<true>(character));
	for (const auto attribute : {DerivedAttribute::Intellect, DerivedAttribute::Personality,
			DerivedAttribute::Endurance})
		static_cast<void>(effectiveAttribute<true>(character, context, attribute));
	static_cast<void>(maximumHp<true>(character, context));
	static_cast<void>(maximumSp<true>(character, context));
}

int XeenCharacterRules::effectiveEndurance(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return effectiveAttribute<false>(character, context, DerivedAttribute::Endurance);
}

int XeenCharacterRules::effectiveIntellect(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return effectiveAttribute<false>(character, context, DerivedAttribute::Intellect);
}

int XeenCharacterRules::effectivePersonality(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return effectiveAttribute<false>(character, context, DerivedAttribute::Personality);
}

int XeenCharacterRules::maxHp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return maximumHp<false>(character, context);
}

int XeenCharacterRules::maxSp(const XeenCharacter &character,
		const XeenCharacterRulesContext &context) {
	return maximumSp<false>(character, context);
}

} // namespace mmodern
