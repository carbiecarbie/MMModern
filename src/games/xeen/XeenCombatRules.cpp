// Adapted from ScummVM developers (upstream COPYRIGHT), GPL-3.0-or-later.
// Pin 6814ee9ba54582f5b5adcffab49efbbd8f589edd, character.cpp and constants.cpp.
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenCharacterRules.h"
#include <limits>
#include <stdexcept>
namespace mmodern {
XeenWeaponDice xeenOrdinaryWeaponDice(unsigned id) {
	constexpr unsigned count[]{0,3,2,3,2,2,4,1,2,4,2,3,2,2,1,1,1,1,4,4,3,2,4,2,2,2,5,3,3,3,3,5,4,2,6};
	constexpr unsigned sides[]{0,3,3,4,5,4,2,3,3,3,3,3,2,4,10,6,8,9,4,3,6,8,5,6,4,5,3,5,6,7,2,2,2,2,4};
	if (id > 34) throw std::invalid_argument("Unsupported ordinary physical weapon ID");
	return {count[id], sides[id]};
}
void xeenApplyPhysicalInjury(XeenCharacter &c, int damage, unsigned year) {
	const auto hp = std::int64_t(c.currentHp) - damage;
	if (damage < 0 || hp < std::numeric_limits<std::int16_t>::min())
		throw std::overflow_error("Physical damage exceeds HP storage");
	c.currentHp = static_cast<std::int16_t>(hp);
	bool dead = false;
	if (hp < 1) {
		dead = std::int64_t(XeenCharacterRules::maxHp(c,{year})) + hp < 1;
		c.conditions[dead ? 13 : 12] = 1;
	}
	if (dead || hp <= -10)
		for (auto &item : c.armor) if (item.id && item.frame) item.state |= 0x80;
}
}
namespace mmodern {
namespace {
using Rules = XeenCharacterRules;
void ruleRequire(bool valid, const char *message) { if (!valid) throw std::invalid_argument(message); }
int physicalChecked(std::int64_t v) {
	if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max())
		throw std::overflow_error("Physical arithmetic overflow");
	return static_cast<int>(v);
}
}
std::optional<unsigned> XeenConsequenceDraw::draw(unsigned lo, unsigned hi) {
	if (!remaining) return {};
	--remaining;
	const auto result = random.draw(lo,hi);
	if (check) check();
	return result;
}
XeenEnemyAttackCandidate::XeenEnemyAttackCandidate(const XeenConsequenceCharacters &c,
		const XeenConsequenceInputs &i, const XeenMonsterRecord &m, unsigned y, const std::array<bool,6> &b) :
		characters(c), inputs(i), monster(m), year(y), blocked(b), allParty(m.preferredClass()==16) {
	ruleRequire(m.strikes() && m.damageDie() && m.hitParameter() && m.raw[29]==0 &&
		(m.raw[30]==0 || m.raw[30]==5 || m.raw[30]==7 || m.raw[30]==9),"Unsupported physical attack profile");
	result.operation = XeenCombatOperation::EnemyAttack;
	if (allParty) { target=0; step=Step::Begin; }
}
bool XeenEnemyAttackCandidate::service(XeenConsequenceDraw &draw) {
	while (step!=Step::Done && draw.remaining) {
		switch (step) {
		case Step::Target: {
			if (monster.preferredClass()!=1) for (unsigned i=0;i<6;++i)
				if (xeenCombatTargetable(characters[i]) && unsigned(characters[i].characterClass)==monster.preferredClass()) { target=i; break; }
			if (target<0) { const auto n=draw.draw(0,5); if (!n) break; target=*n; }
			step=xeenCombatTargetable(characters[target]) ? Step::Begin : Step::Fallback; break;
		}
		case Step::Fallback: {
			std::array<unsigned,6> eligible{}; unsigned count=0;
			for (unsigned i=0;i<6;++i) if (xeenCombatTargetable(characters[i])) eligible[count++]=i;
			if (!count) { step=Step::Done; break; }
			const auto n=draw.draw(0,count-1); if (n) { target=eligible[*n]; step=Step::Begin; } break;
		}
		case Step::Begin:
            result.targetedMembers|=std::uint8_t(1u<<target);
			result.targetOwner=characters[target].rosterId;
			if (characters[target].conditions[8]) { characters[target].conditions[8]=0; afterInjury=Step::Next; step=Step::Dice; }
			else step=Step::Roll;
			break;
		case Step::Roll: {
			const auto n=draw.draw(1,20); if (!n) break;
			roll=*n; result.critical=result.critical || roll==20;
			if (roll==1) step=Step::Next;
			else if (roll==20) { characters[target].conditions[8]=0; afterInjury=Step::Parameter; step=Step::Dice; }
			else step=Step::Parameter;
			break;
		}
		case Step::Parameter: {
			const auto n=draw.draw(1,monster.hitParameter()); if (!n) break;
			const auto &c=characters[target];
			const auto threshold=std::int64_t(Rules::combatArmorClass(c,inputs[target],{year}))+
				(blocked[target] ? c.currentLevel()/2+15 : 10);
			if (std::int64_t(roll)+monster.hitParameter()/4+*n>=threshold) {
				characters[target].conditions[8]=0; afterInjury=Step::Next; step=Step::Dice;
			} else step=Step::Next;
			break;
		}
		case Step::Dice: {
            if(!dice) beforeDamageAc=Rules::combatArmorClass(characters[target],inputs[target],{year});
			const auto n=draw.draw(1,monster.damageDie()); if (!n) break;
			damage=physicalChecked(std::int64_t(damage)+*n);
			if (++dice==monster.strikes()) step=damage && monster.raw[30] ? Step::Special : Step::Injury;
			break;
		}
		case Step::Special: {
			auto &c=characters[target];
			const auto v=std::int64_t(Rules::physicalBonus(Rules::effectiveLuck(c,inputs[target])))+c.currentLevel();
			ruleRequire(v+20>0 && v+20<=std::numeric_limits<int>::max(),"Invalid physical save interval");
			const auto n=draw.draw(1,unsigned(v+20)); if (!n) break;
			if (std::int64_t(*n)>v) {
				const unsigned condition=monster.raw[30]==5 ? 3 : monster.raw[30]==7 ? 4 : 8;
				ruleRequire(c.conditions[condition]<255,"Physical special condition overflow"); ++c.conditions[condition];
			}
			step=Step::Injury; break;
		}
		case Step::Injury: {
			auto &c=characters[target];
			XeenCombatDamage injury; injury.owner=c.rosterId; injury.amount=damage;
			injury.beforeHp=c.currentHp; injury.beforeAc=beforeDamageAc;
			const auto armor=c.armor;
			xeenApplyPhysicalInjury(c,damage,year);
			injury.afterHp=c.currentHp; injury.afterAc=Rules::combatArmorClass(c,inputs[target],{year}); injury.conditions=c.conditions;
			result.injuries.at(result.injuryCount++)=injury;
			for (unsigned slot=0;slot<9;++slot) if (armor[slot].state!=c.armor[slot].state) {
				bool found=false;
				for (unsigned i=0;i<result.armorCount;++i) if (result.armor[i].owner==c.rosterId && result.armor[i].slot==slot) { result.armor[i].after=c.armor[slot];found=true; }
				if (!found) result.armor.at(result.armorCount++)={c.rosterId,static_cast<std::uint8_t>(slot),armor[slot],c.armor[slot]};
			}
			result.damage=physicalChecked(std::int64_t(result.damage)+damage);
			damage=0; dice=0; step=afterInjury; break;
		}
		case Step::Next:
			if (allParty && ++target<6) step=Step::Begin; else step=Step::Done;
			break;
		case Step::Done: break;
		}
	}
	if (step!=Step::Done) return false;
	result.attackOutcome=!result.injuryCount ? XeenCombatAttackOutcome::Miss : result.damage ?
		XeenCombatAttackOutcome::HitPositiveDamage : XeenCombatAttackOutcome::HitZeroDamage;
	return true;
}
XeenPhysicalPlayerCandidate::XeenPhysicalPlayerCandidate(const XeenCharacter &c,
		const XeenCombatInputs &i,const XeenMonsterRecord &m,unsigned type,unsigned year,bool missile) :
		character(c),monster(m),monsterType(type),shoot(missile) {
	constexpr unsigned divisors[]{1,2,2,3,4,2,2,1,3,2};
	ruleRequire(unsigned(c.characterClass)<10 && m.physicalResistance()<=100,"Invalid physical player operands");
	baseHit=physicalChecked(std::int64_t(Rules::physicalBonus(Rules::effectivePhysical(c,i,Rules::PhysicalAttribute::Accuracy,{year})))+5+c.currentLevel()/divisors[unsigned(c.characterClass)]);
	hitTotal=baseHit;
	might=shoot ? 0 : Rules::physicalBonus(Rules::effectivePhysical(c,i,Rules::PhysicalAttribute::Might,{year}));
	attacks=shoot ? 1 : xeenCombatAttackCount(c.characterClass,c.currentLevel());
}
bool XeenPhysicalPlayerCandidate::service(XeenConsequenceDraw &draw) {
	while (step!=Step::Done && draw.remaining) switch (step) {
	case Step::Weapon: {
		if (dice) { const auto n=draw.draw(1,sides); if (n) { weapon=physicalChecked(std::int64_t(weapon)+*n); --dice; } break; }
		if (slot==9) { weapon=physicalChecked(std::int64_t(weapon)*3);step=Step::Hit;break; }
		const auto &item=character.weapons[slot++];
		if (shoot ? item.frame!=4 : item.frame!=1 && item.frame!=13) break;
		ruleRequire(item.material==0 && (item.state&0x3f)==0 && item.id && item.id<=34 &&
			(!shoot || (item.id>=30 && item.id<=33)),"Unsupported equipped physical weapon");
		const auto value=xeenOrdinaryWeaponDice(item.id); dice=value.count;sides=value.sides;break;
	}
	case Step::Hit: {
		const auto n=draw.draw(1,20);if (!n) break;
		hitTotal=physicalChecked(std::int64_t(hitTotal)+*n);if (*n==20) break;
		if (hitTotal>=int(monster.armorClass()+10)) {
			hit=true;accumulated=physicalChecked(std::int64_t(accumulated)+(shoot ? weapon : std::max(physicalChecked(std::int64_t(weapon)+might),1)));
		}
		if (--attacks) { slot=0;weapon=0;hitTotal=baseHit;step=Step::Weapon;break; }
		damage=physicalChecked(std::int64_t(accumulated)*(100-monster.physicalResistance())/100);
		step=shoot && accumulated>0 ? Step::Save : Step::Done;break;
	}
	case Step::Save: {
		ruleRequire(monsterType<=std::numeric_limits<unsigned>::max()-50,"Monster save interval overflow");
		const auto n=draw.draw(1,50+monsterType);if (!n) break;
		if (*n<=monsterType) damage/=2;step=Step::Done;break;
	}
	case Step::Done: break;
	}
	return step==Step::Done;
}
XeenConditionTimeCandidate::XeenConditionTimeCandidate(const XeenGameplayContext &before,unsigned charge,
		const XeenConsequenceCharacters &c,const XeenConsequenceInputs &i) : characters(c),inputs(i) {
	ruleRequire(xeenRegionalContext(before) && (charge==1 || charge==10),"Unsupported consequence time charge");
	const auto prepared=xeenPrepareTime(before,charge);
	ruleRequire(!prepared.midnights && !prepared.yearRollovers && !prepared.dawns && !prepared.dusks && !prepared.dailyProcessing && prepared.processing480<=1,"Unsupported consequence time boundary");
	context=prepared.context;
	if (prepared.processing480) step=Step::Stats;
	for (const auto &input:inputs) ruleRequire(input.resistances.has_value(),"Missing condition-tick resistances");
}
bool XeenConditionTimeCandidate::service(XeenConsequenceDraw &draw) {
	while (step!=Step::Done && draw.remaining) {
		auto &c=characters[owner]; const auto &i=inputs[owner];
		switch (step) {
		case Step::Stats:
			if (!c.conditions[13]) {
				const auto kill=[&](int value) { if (value<1) c.conditions[13]=1; };
				kill(Rules::effectivePhysical(c,i,Rules::PhysicalAttribute::Might,{context.year}));
				kill(Rules::effectiveIntellect(c,{context.year}));kill(Rules::effectivePersonality(c,{context.year}));
				kill(Rules::effectiveEndurance(c,{context.year}));
				kill(Rules::effectivePhysical(c,i,Rules::PhysicalAttribute::Speed,{context.year}));
				kill(Rules::effectivePhysical(c,i,Rules::PhysicalAttribute::Accuracy,{context.year}));kill(Rules::effectiveLuck(c,i));
			}
			step=Step::Poison;break;
		case Step::Poison: {
			if (c.conditions[3]) { step=Step::Disease;break; }
			const auto n=draw.draw(1,10);if (n) step=*n==1 ? Step::Electrical : Step::Disease;break;
		}
		case Step::Electrical: {
			const auto &r=*i.resistances; const auto n=draw.draw(1,unsigned(r.electricalPermanent)+r.electricalTemporary+40);
			if (n) step=Step::Disease;break; // Both outcomes leave zero Poison unchanged.
		}
		case Step::Disease: {
			if (c.conditions[4]) { step=Step::Death;break; }
			const auto n=draw.draw(0,9);if (n) step=*n==1 ? Step::Cold : Step::Death;break;
		}
		case Step::Cold: {
			const auto &r=*i.resistances; const auto n=draw.draw(1,unsigned(r.coldPermanent)+r.coldTemporary+40);
			if (n) step=Step::Death;break;
		}
		case Step::Death:
			if (c.conditions[13]) { ruleRequire(c.conditions[13]<255,"Time Dead byte overflow"); ++c.conditions[13]; }
			step=++owner==6 ? Step::Done : Step::Stats;break;
		case Step::Done: break;
		}
	}
	return step==Step::Done;
}
}