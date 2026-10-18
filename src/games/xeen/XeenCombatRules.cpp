// Adapted from ScummVM developers (upstream COPYRIGHT), GPL-3.0-or-later.
// Pin 6814ee9ba54582f5b5adcffab49efbbd8f589edd, combat.cpp, character.cpp and constants.cpp.
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/detail/XeenItemTables.h"
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
void xeenApplySpecialAttack(XeenCharacter &c, unsigned special) {
	// DOS: inc al; jnz; mov 0xff: the byte saturates instead of wrapping to zero.
	const auto bump=[&](unsigned condition) { if (c.conditions[condition]!=255) ++c.conditions[condition]; };
	const auto zeroHp=[&] { if (c.currentHp>0) c.currentHp=0; };
	switch (special) {
	case 5: bump(3); break;   // Poison
	case 7: bump(4); break;   // Disease
	case 8: bump(5); break;   // Insane
	case 9: bump(8); break;   // Sleep, after the wake at entry
	case 10:                  // Curse Item (0x2d671..0x2d6ab): every slot, weapon ID 34 untouched
		for (unsigned slot=0;slot<9;++slot) {
			if (c.weapons[slot].id!=34) c.weapons[slot].state|=0x40;
			c.armor[slot].state|=0x40; c.accessories[slot].state|=0x40; c.miscellaneous[slot].state|=0x40;
		}
		break;
	case 12: c.currentSp=0; break; // Drain SP
	case 13: bump(0); break;  // Curse
	case 14: bump(11); break; // Paralyze
	case 15: bump(12); break; // Unconscious
	case 16: bump(10); break; // Confuse
	case 17:                  // Break Weapon (0x2d508..0x2d53e): equipped, nonempty, not ID 34
		for (auto &w:c.weapons) if (w.id!=34 && w.id && w.frame) { w.state|=0x80; w.frame=0; }
		break;
	case 18: bump(2); break;  // Weaken
	case 19:                  // Eradicate (0x2d5c6..0x2d60e, helper 0x2bd6a)
		bump(15);
		for (unsigned slot=0;slot<9;++slot) {
			// Every slot is written, empty ones included; weapon ID 34 is skipped entirely.
			if (c.weapons[slot].id!=34) { c.weapons[slot].state|=0x80; c.weapons[slot].frame=0; }
			c.armor[slot].state|=0x80; c.armor[slot].frame=0;
			c.accessories[slot].state|=0x80; c.accessories[slot].frame=0;
			c.miscellaneous[slot].state|=0x80; // frame (charges) untouched
		}
		zeroHp();
		break;
	case 20:                  // Aging (0x2d592..0x2d59b): plain byte increment, 255 wraps to 0
		c.temporaryAge=int(static_cast<std::uint8_t>(unsigned(c.temporaryAge)+1));
		break;
	case 21: bump(13); zeroHp(); break; // Death
	case 22: bump(14); zeroHp(); break; // Stone
	default: break;           // 0-4, 6 and 11 (In Love) reach no handler
	}
}
void xeenRecordItemChanges(XeenCombatResult &result, const XeenCharacter &before, const XeenCharacter &after) {
	for (unsigned category=0;category<4;++category) {
		const auto &a=*xeenInventoryItems(before,static_cast<XeenInventoryCategory>(category));
		const auto &b=*xeenInventoryItems(after,static_cast<XeenInventoryCategory>(category));
		for (unsigned slot=0;slot<9;++slot) {
			if (xeenSameItem(a[slot],b[slot])) continue;
			bool found=false;
			for (unsigned i=0;i<result.armorCount;++i) {
				auto &entry=result.armor[i];
				if (entry.owner==after.rosterId && entry.slot==slot && entry.category==category) { entry.after=b[slot]; found=true; break; }
			}
			if (!found) result.armor.at(result.armorCount++)={after.rosterId,static_cast<std::uint8_t>(slot),static_cast<std::uint8_t>(category),a[slot],b[slot]};
		}
	}
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
XeenTypedDamageCandidate::XeenTypedDamageCandidate(const XeenConsequenceCharacters &c,
		const XeenConsequenceInputs &i,int amount,XeenDamageType t,unsigned y,unsigned memberMask,
		const XeenDamageProtection &p) : characters(c),inputs(i),protection(p),type(t),year(y),mask(memberMask),damage(amount) {
	ruleRequire(unsigned(type)<=6 && mask<=0x3f,"Invalid typed damage operands");
	ruleRequire(p.powerShield>=0 && p.powerShield<=65535,"Invalid power shield");
	for (int value:p.resistances) ruleRequire(value>=0 && value<=65535,"Invalid party damage resistance");
	constexpr unsigned frames[]{0,6,1,2,3,4,5};portraitFrame=frames[unsigned(type)];
	for (unsigned n=0;n<6;++n) if (mask&(1u<<n)) {
		Rules::validateForUse(characters[n],{year});
		(void)Rules::sheetArmorClass(characters[n],&inputs[n],{year,protection.blessed});
		if (type!=XeenDamageType::Physical) {
			const auto v=Rules::damageSaveValue(characters[n],inputs[n],type,{year});
			ruleRequire(v>=0 && v<=std::numeric_limits<int>::max()-40,"Invalid typed saving interval");
		}
	}
}
bool XeenTypedDamageCandidate::operator==(const XeenTypedDamageCandidate &other) const {
    if(std::tie(deferInjury,injuryReady,injuryAcknowledged,injuryApplied,impactOwner,portraitFrame,step,type,year,mask,target,damage,beforeAc)!=
        std::tie(other.deferInjury,other.injuryReady,other.injuryAcknowledged,other.injuryApplied,other.impactOwner,other.portraitFrame,other.step,other.type,other.year,other.mask,other.target,other.damage,other.beforeAc) ||
        protection.resistances!=other.protection.resistances || protection.powerShield!=other.protection.powerShield || protection.blessed!=other.protection.blessed ||
        result.injuryCount!=other.result.injuryCount || result.armorCount!=other.result.armorCount || result.damage!=other.result.damage ||
        result.targetedMembers!=other.result.targetedMembers || result.targetOwner!=other.result.targetOwner || result.attackOutcome!=other.result.attackOutcome)return false;
    for(unsigned n=0;n<characters.size();++n)if(!xeen_state::sameCharacter(characters[n],other.characters[n]) || !xeen_state::sameInputs(inputs[n],other.inputs[n]))return false;
    for(unsigned n=0;n<result.injuryCount;++n) {
        const auto &a=result.injury(n),&b=other.result.injury(n);
        if(std::tie(a.owner,a.amount,a.beforeHp,a.afterHp,a.beforeAc,a.afterAc,a.beforeSp,a.afterSp,a.beforeAge,a.afterAge,a.conditions,a.attackOrdinal)!=
            std::tie(b.owner,b.amount,b.beforeHp,b.afterHp,b.beforeAc,b.afterAc,b.beforeSp,b.afterSp,b.beforeAge,b.afterAge,b.conditions,b.attackOrdinal))return false;
    }
    for(unsigned n=0;n<result.armorCount;++n) {const auto &a=result.armor[n],&b=other.result.armor[n];
        if(a.owner!=b.owner || a.slot!=b.slot || a.category!=b.category || std::tie(a.before.material,a.before.id,a.before.state,a.before.frame)!=std::tie(b.before.material,b.before.id,b.before.state,b.before.frame) || std::tie(a.after.material,a.after.id,a.after.state,a.after.frame)!=std::tie(b.after.material,b.after.id,b.after.state,b.after.frame))return false;}
    return true;
}
bool XeenTypedDamageCandidate::service(XeenConsequenceDraw &draw) {
	injuryApplied=false;
	while (step!=Step::Done) {
		switch (step) {
		case Step::Begin:
			while (target<6 && !(mask&(1u<<target))) ++target;
			if (target==6) {step=Step::Done;break;}
			beforeAc=Rules::sheetArmorClass(characters[target],&inputs[target],{year,protection.blessed});
			characters[target].conditions[8]=0;
			if (unsigned(type)>=2 && unsigned(type)<=5)
				damage=physicalChecked(std::int64_t(damage)-protection.resistances[unsigned(type)-2]);
			step=type==XeenDamageType::Physical ? Step::Injury : Step::Save;
			break;
		case Step::Save: {
			if (!draw.remaining) return false;
			const auto v=Rules::damageSaveValue(characters[target],inputs[target],type,{year});
			const auto n=draw.draw(1,unsigned(v+40));if (!n) break;
			// Reference evaluates the save before testing damage, including the
			// final zero/negative-damage iteration after a successful save.
			if (*n<=unsigned(v) && damage>0) damage/=2;
			else step=Step::Injury;
			break;
		}
		case Step::Injury: {
			auto &c=characters[target];
			if (deferInjury && !injuryAcknowledged) {impactOwner=c.rosterId;injuryReady=true;return false;}
			injuryReady=injuryAcknowledged=false;
			damage=std::max(physicalChecked(std::int64_t(damage)-protection.powerShield),0);
			const auto previous=c;
			XeenCombatDamage injury;injury.owner=c.rosterId;injury.amount=damage;
			injury.beforeHp=c.currentHp;injury.beforeAc=beforeAc;injury.beforeSp=injury.afterSp=c.currentSp;injury.beforeAge=injury.afterAge=c.temporaryAge;
			xeenApplyPhysicalInjury(c,damage,year);
			injury.afterHp=c.currentHp;injury.afterAc=Rules::sheetArmorClass(c,&inputs[target],{year,protection.blessed});injury.conditions=c.conditions;
			result.injuries.at(result.injuryCount++)=injury;result.targetedMembers|=std::uint8_t(1u<<target);result.targetOwner=c.rosterId;
			result.damage=physicalChecked(std::int64_t(result.damage)+damage);
			xeenRecordItemChanges(result,previous,c);
			step=Step::Next;
			if (deferInjury) {injuryApplied=true;return false;}
			break;
		}
		case Step::Next: ++target;step=Step::Begin;break;
		case Step::Done: break;
		}
	}
	result.attackOutcome=!mask ? XeenCombatAttackOutcome::NoParticipants : result.damage ?
		XeenCombatAttackOutcome::HitPositiveDamage : XeenCombatAttackOutcome::HitZeroDamage;
	return true;
}
unsigned xeenSpellProjectile(XeenLearnedSpell spell,unsigned type,unsigned itemPower) {
    switch (spell) {
    case XeenLearnedSpell::FingerOfDeath: return 14;
    case XeenLearnedSpell::GolemStopper: case XeenLearnedSpell::MassDistortion: return 6;
    case XeenLearnedSpell::HolyWord: return 13;
    case XeenLearnedSpell::AcidSpray: return 10;
    case XeenLearnedSpell::ColdRay: case XeenLearnedSpell::FantasticFreeze: return 8;
    case XeenLearnedSpell::DancingSword: case XeenLearnedSpell::PrismaticLight: return 14;
    case XeenLearnedSpell::DeadlySwarm: case XeenLearnedSpell::StarBurst: return 15;
    case XeenLearnedSpell::ElementalStorm: {
        constexpr unsigned projectiles[]{0,5,9,10};
        if(type<2 || type>5)throw std::invalid_argument("Invalid Elemental Storm type");
        return projectiles[type-2];
    }
    case XeenLearnedSpell::FieryFlail: return 2;
    case XeenLearnedSpell::Fireball: return 0;
    case XeenLearnedSpell::Frostbite: return itemPower==33?9:8;
    case XeenLearnedSpell::Implosion: return 6;
    case XeenLearnedSpell::Incinerate: return itemPower==64?2:1;
    case XeenLearnedSpell::Inferno: return 1;
    case XeenLearnedSpell::LightningBolt: return 3;
    case XeenLearnedSpell::MegaVolts: return 4;
    case XeenLearnedSpell::MoonRay: case XeenLearnedSpell::SunRay: return 13;
    case XeenLearnedSpell::MagicArrow: return 11; // POW_ARROW, not POW_MAGIC_ARROW
    case XeenLearnedSpell::EnergyBlast: case XeenLearnedSpell::TurnUndead: return 13;
    case XeenLearnedSpell::Sparks: return 5;
    case XeenLearnedSpell::PoisonVolley: return 11;
    case XeenLearnedSpell::InsectSpray: case XeenLearnedSpell::ToxicCloud: return 10;
    case XeenLearnedSpell::Sleep: case XeenLearnedSpell::BeastMaster:
    case XeenLearnedSpell::Hypnotize: case XeenLearnedSpell::DragonSleep: return 7;
    case XeenLearnedSpell::FlyingFist: case XeenLearnedSpell::Pain: return 14;
    case XeenLearnedSpell::Shrapmetal: return 15;
    default: throw std::invalid_argument("Unsupported spell projectile");
    }
}
XeenSpellDamagePreparation::XeenSpellDamagePreparation(XeenLearnedSpell spell,unsigned casterLevel) : level(casterLevel) {
    using S=XeenLearnedSpell;
    switch(spell) {
    case S::PoisonVolley: damage=10;type=5;break;
    case S::FingerOfDeath: type=8;break;
    case S::HolyWord: type=9;break;
    case S::MassDistortion: type=10;break;
    case S::GolemStopper: type=14;break;
    case S::AcidSpray: damage=15;type=5;break;
    case S::ColdRay: low=2;high=4;type=4;break;
    case S::DancingSword: low=6;high=14;break;
    case S::DeadlySwarm: damage=40;break;
    case S::ElementalStorm: damage=150;low=2;high=5;typeDraw=true;break;
    case S::FantasticFreeze: damage=40;type=4;break;
    case S::FieryFlail: damage=100;type=2;break;
    case S::Fireball: low=3;high=7;type=2;break;
    case S::Frostbite: damage=35;type=4;break;
    case S::Implosion: damage=1000;type=6;break;
    case S::Incinerate: case S::Inferno: damage=250;type=2;break;
    case S::LightningBolt: low=4;high=6;type=3;break;
    case S::MegaVolts: damage=150;type=3;break;
    case S::MoonRay: damage=30;type=6;break;
    case S::PrismaticLight: damage=80;low=0;high=6;typeDraw=true;break;
    case S::StarBurst: damage=500;type=2;break;
    case S::SunRay: damage=200;type=6;break;
    default: break; // Earlier spell dispatch remains in the per-target rule.
    }
    done=high==0;
}
bool XeenSpellDamagePreparation::service(XeenConsequenceDraw &draw) {
    while(!done && draw.remaining)if(const auto value=draw.draw(low,high)) {
        if(typeDraw)type=*value;
        else damage=physicalChecked(std::int64_t(*value)*level);
        done=true;
    }
    return done;
}
bool XeenMoonRayHealing::service(XeenConsequenceDraw &draw,const XeenPartyState &party,std::uint32_t year) {
    while(member<party.party.size() && draw.remaining)if(const auto amount=draw.draw(1,30)) {
        const auto effect=XeenLearnedSpellRules::prepareFirstAid(party,member++,year,*amount);
        result.effects.insert(result.effects.end(),effect.effects.begin(),effect.effects.end());
    }
    return member==party.party.size();
}
// Pinned combat.cpp rangedAttack/attack/attack2: physical reduction precedes
// the resource-ID save; Arrow draws the save but ignores its result.
XeenSpellDamageCandidate::XeenSpellDamageCandidate(std::int64_t permanent, std::int64_t temporary,
        unsigned r, unsigned index, XeenLearnedSpell spell, unsigned physical, XeenActorStatus initialStatus, unsigned targetSpecies,
        const XeenSpellDamagePreparation *prepared, int hp, bool isClouds) : status(initialStatus), resistance(r), resourceId(index), physicalResistance(physical), currentHp(hp), clouds(isClouds), species(targetSpecies), spell(spell) {
    ruleRequire(permanent>=std::numeric_limits<int>::min() && permanent<=std::numeric_limits<int>::max() &&
        temporary>=std::numeric_limits<int>::min() && temporary<=std::numeric_limits<int>::max(),"Invalid Arrow level operands");
    const auto level=std::max<std::int64_t>(permanent+temporary,0);
    ruleRequire(r<=100 && physical<=100 && level<=std::numeric_limits<int>::max()-100 &&
        index<=unsigned(std::numeric_limits<int>::max()-50),"Invalid Arrow interval");
    ruleRequire(xeenActorStatusSupported(status),"Unsupported spell target status");
    levelBound=unsigned(100+level);saveBound=50+index;
    ruleRequire(spell!=XeenLearnedSpell::PoisonVolley,"Poison Volley requires a bound shooter");
    switch (spell) {
    case XeenLearnedSpell::FingerOfDeath: case XeenLearnedSpell::HolyWord: case XeenLearnedSpell::MassDistortion:
        ruleRequire(currentHp>0,"Special spell requires live target HP");break;
    case XeenLearnedSpell::GolemStopper: acceptedDamage=100;halveOnSave=false;break;
    case XeenLearnedSpell::Sleep: control=XeenActorStatus::Sleep;break;
    case XeenLearnedSpell::BeastMaster: control=XeenActorStatus::BeastMaster;break;
    case XeenLearnedSpell::Hypnotize: control=XeenActorStatus::Hypnotize;break;
    case XeenLearnedSpell::DragonSleep: control=XeenActorStatus::DragonSleep;break;
    case XeenLearnedSpell::InsectSpray: case XeenLearnedSpell::TurnUndead: acceptedDamage=25;halveOnSave=false;break;
    case XeenLearnedSpell::Sparks: acceptedDamage=physicalChecked(2*level);break;
    case XeenLearnedSpell::ToxicCloud: acceptedDamage=10;break;
    case XeenLearnedSpell::MagicArrow: acceptedDamage=8;halveOnSave=false;break;
    case XeenLearnedSpell::EnergyBlast: acceptedDamage=physicalChecked(2*level);break;
    case XeenLearnedSpell::Shrapmetal: acceptedDamage=physicalChecked(2*level);physicalDamage=true;break;
    case XeenLearnedSpell::FlyingFist: acceptedDamage=6;physicalDamage=true;break;
    case XeenLearnedSpell::Pain: acceptedDamage=8;physicalDamage=true;break;
    default:
        ruleRequire(prepared && prepared->ready() && XeenLearnedSpellRules::target(std::uint8_t(spell))==XeenSpellTarget::Contact,"Missing ranged spell preparation");
        acceptedDamage=prepared->damage;physicalDamage=prepared->type==0;break;
    }
    // Zero damage skips waking and the save. Physical immunity reduces damage
    // only after entry, so it still consumes the saving throw.
    entersDamage=acceptedDamage!=0;
    step=r ? Step::Resistance : Step::Species;
}
XeenSpellDamageCandidate::XeenSpellDamageCandidate(const XeenCharacter &c,const XeenCombatInputs &i,
    const XeenMonsterRecord &m,unsigned id,unsigned year,XeenDifficulty difficulty,XeenActorStatus initial,bool isClouds,const XeenGameplayContext &partyContext)
    : XeenSpellDamageCandidate(0,0,0,id,XeenLearnedSpell::ToxicCloud,0,initial) {
    missile.emplace(c,i,m,id,year,true,difficulty,initial,true,isClouds,partyContext);
}
bool XeenSpellDamageCandidate::service(XeenConsequenceDraw &draw) {
    if(missile) {
        if(!missile->service(draw))return false;
        damage=missile->damage;status=missile->status;impact=missile->impact;miss=!missile->hit;
        return true;
    }
    // Complete the draw-free species dispatch even when the gate used the budget.
    while (step!=Step::Done && (draw.remaining || step==Step::Species || step==Step::Damage)) {
        if(step==Step::Species) {
            if(control!=XeenActorStatus::Physical) {
                const bool compatible=control==XeenActorStatus::DragonSleep ? species==6 : species==1 || species==3;
                if(!compatible) {step=Step::Done;continue;}
                step=Step::Save;continue;
            }
            if(spell==XeenLearnedSpell::InsectSpray && species!=2) {step=Step::Done;continue;}
            // Turn Undead calls attack2 with zero for other species: no wake/save.
            if(spell==XeenLearnedSpell::TurnUndead && species!=4) {acceptedDamage=0;entersDamage=false;}
            if(spell==XeenLearnedSpell::FingerOfDeath) {
                if(species!=1 && species!=3) {step=Step::Done;continue;}
                step=Step::FingerSave;continue;
            }
            if(spell==XeenLearnedSpell::GolemStopper && species!=5) {step=Step::Done;continue;}
            if(spell==XeenLearnedSpell::HolyWord) {
                // DOS enters attack2 with zero for the wrong species (sv-holy-word-miss).
                acceptedDamage=species==4 ? currentHp : 0;halveOnSave=false;
            }
            if(spell==XeenLearnedSpell::MassDistortion)acceptedDamage=std::max(currentHp/2,1);
            step=Step::Damage;continue;
        }
        if(step==Step::Damage) {
            entersDamage=acceptedDamage!=0;
            // Every spell projectile supplies a nonzero attack-range argument,
            // including Single. Refuse after Finger's first save, before waking.
            if(entersDamage && clouds && resourceId==89) {step=Step::Done;continue;}
            miss=!entersDamage && (spell==XeenLearnedSpell::HolyWord || spell==XeenLearnedSpell::TurnUndead);
            impact=entersDamage;
            step=entersDamage ? Step::Save : Step::Done;continue;
        }
        if(step==Step::Save && !damagePrepared) {
            status=xeenActorStatusAfterDamage(status,entersDamage);
            if(physicalDamage) acceptedDamage=physicalChecked(std::int64_t(acceptedDamage)*(100-physicalResistance)/100);
            damagePrepared=true;
        }
        const auto accepted=draw.draw(1,step==Step::Resistance ? levelBound : saveBound);
        if (!accepted) continue;
        if (step==Step::Resistance) {
            if (*accepted<resistance) {resisted=true;step=Step::Done;}
            else step=Step::Species;
        } else if(step==Step::FingerSave) {
            if(*accepted<=resourceId)step=Step::Done;
            else {acceptedDamage=std::min(currentHp,50);step=Step::Damage;}
        } else {
            if(control!=XeenActorStatus::Physical) {
                if(*accepted>resourceId)status=control;
                step=Step::Done;continue;
            }
            damage=acceptedDamage;
            if (*accepted<=resourceId) {
                if(spell==XeenLearnedSpell::FingerOfDeath || spell==XeenLearnedSpell::MassDistortion)damage=5;
                else if(halveOnSave)damage/=2;
            }
            step=Step::Done;
        }
    }
    return step==Step::Done;
}
XeenRunCandidate::XeenRunCandidate(int signedThreshold) : threshold(signedThreshold) {
	ruleRequire(threshold>=-128 && threshold<=127,"Invalid signed Run threshold");
}
bool XeenRunCandidate::service(XeenConsequenceDraw &draw) {
	while (!roll && draw.remaining) {
		const auto accepted=draw.draw(1,100);
		if (accepted) { roll=*accepted; success=int(roll)<threshold; }
	}
	return roll!=0;
}
XeenMonsterTargetCandidate::XeenMonsterTargetCandidate(const XeenConsequenceCharacters &characters,
        unsigned hatred,unsigned participantMask) {
    ruleRequire(participantMask<=0x3f,"Invalid combat participation mask");
    for(unsigned i=0;i<6;++i)if(participantMask&(1u<<i)) {
        participants[count++]=i;
        if(xeenCombatTargetable(characters[i])) {
            eligible[eligibleCount++]=i;
            const bool hated=(hatred<10 && hatred!=1 && unsigned(characters[i].characterClass)==hatred) ||
                (hatred==unsigned(XeenMonsterHatred::Dwarf) && characters[i].race==XeenRace::Dwarf);
            if(preferred==6 && hated)preferred=i;
        }
    }
    if(hatred==unsigned(XeenMonsterHatred::Party)) {mask=participantMask;step=Step::Done;}
    else if(!count)step=Step::Done;
}
bool XeenMonsterTargetCandidate::service(XeenConsequenceDraw &draw) {
    while(step!=Step::Done && draw.remaining) {
        if(step==Step::Target) {
            unsigned target=preferred;
            if(target==6) {const auto n=draw.draw(0,count-1);if(!n)continue;target=participants[*n];}
            if(std::find(eligible.begin(),eligible.begin()+eligibleCount,target)!=eligible.begin()+eligibleCount) {
                mask=1u<<target;step=Step::Done;
            } else step=Step::Fallback;
        } else {
            if(!eligibleCount) {step=Step::Done;continue;}
            const auto n=draw.draw(0,eligibleCount-1);if(n){mask=1u<<eligible[*n];step=Step::Done;}
        }
    }
    return step==Step::Done;
}
XeenEnemyAttackCandidate::XeenEnemyAttackCandidate(const XeenConsequenceCharacters &c,
        const XeenConsequenceInputs &i,const XeenMonsterRecord &m,unsigned y,unsigned mask,
        const std::array<bool,6> &blocked,XeenActorStatus status)
    : XeenEnemyAttackCandidate(c,i,m,y,mask,blocked,status,{}) {}
void XeenEnemyAttackCandidate::setPartyEffects(const XeenGameplayContext &context) {
    ruleRequire(step==Step::Target || step==Step::Done,"Monster party effects must be bound before service");
    partyContext=context;
}
XeenEnemyAttackCandidate::XeenEnemyAttackCandidate(const XeenConsequenceCharacters &c,
		const XeenConsequenceInputs &i, const XeenMonsterRecord &m, unsigned y, unsigned mask, const std::array<bool,6> &b, XeenActorStatus status,const XeenGameplayContext &partyContext) :
		characters(c), inputs(i), monster(m), year(y), blocked(b), allParty(m.hatred()==unsigned(XeenMonsterHatred::Party)),partyContext(partyContext) {
	ruleRequire(xeenActorStatusSupported(status),"Unsupported attacking monster status");
	disabled=status!=XeenActorStatus::Physical;
	m.validateAttackCapabilities();
	type=static_cast<XeenDamageType>(m.damageType());
	ruleRequire(mask<=0x3f,"Invalid combat participation mask");
	for (unsigned owner=0;owner<6;++owner) if (mask&(1u<<owner)) participants[participantCount++]=owner;
	result.operation = XeenCombatOperation::EnemyAttack;
	if (disabled || !participantCount || !monster.attacks()) step=Step::Done;
}
// After the saves: Power Shield and the zero clamp, then
// the acknowledged portrait, the secondary save (positive damage, nonzero
// special) or the HP subtraction.
XeenEnemyAttackCandidate::Step XeenEnemyAttackCandidate::afterSaves() {
	damage=std::max(physicalChecked(std::int64_t(damage)-partyContext.effects[XeenPartyEffect::PowerShield]),0);
	if (deferInjury) return Step::Portrait;
	return damage>0 && monster.specialAttack() ? Step::Special : Step::Injury;
}
bool XeenEnemyAttackCandidate::service(XeenConsequenceDraw &draw) {
	// charSavingThrow(DT_PHYSICAL): B(Luck) + L, success at R(1, v + 20) <= v.
	const auto physicalSave=[&](const XeenCharacter &c) {
		const auto v=std::int64_t(Rules::physicalBonus(Rules::effectiveLuck(c,inputs[target])))+c.currentLevel();
		ruleRequire(v+20>0 && v+20<=std::numeric_limits<int>::max(),"Invalid physical save interval");
		return v;
	};
	while (step!=Step::Done && draw.remaining) {
		switch (step) {
		case Step::Target: {
			if(!selection) {
                unsigned mask=0;for(unsigned p=0;p<participantCount;++p)mask|=1u<<participants[p];
                selection.emplace(characters,monster.hatred(),mask);
            }
            if(!selection->service(draw))break;
            if(!selection->mask){noTarget=true;step=Step::Done;break;}
            for(unsigned p=0;p<participantCount;++p)if(selection->mask&(1u<<participants[p])) {
                participantCursor=p;target=participants[p];break;
            }
            selection.reset();step=Step::Begin;break;
		}
		case Step::Begin:
            result.targetedMembers|=std::uint8_t(1u<<target);
			result.targetOwner=characters[target].rosterId;
			// doMonsterTurn: a nonphysical attack or a sleeping target enters the
			// damage application directly, which wakes the victim first.
			if (type!=XeenDamageType::Physical || characters[target].conditions[8]) {
				characters[target].conditions[8]=0; afterInjury=Step::Next; step=Step::Dice;
			} else step=Step::Roll;
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
			const auto threshold=std::int64_t(Rules::combatArmorClass(c,inputs[target],{year,partyContext.effects[XeenPartyEffect::Bless]}))+
				(blocked[target] ? c.currentLevel()/2+15 : 10);
			if (std::int64_t(roll)+monster.hitParameter()/4+*n>=threshold) {
				characters[target].conditions[8]=0; afterInjury=Step::Next; step=Step::Dice;
			} else step=Step::Next;
			break;
		}
		case Step::Dice: {
            if(!dice) { beforeDamageAc=Rules::combatArmorClass(characters[target],inputs[target],{year,partyContext.effects[XeenPartyEffect::Bless]}); before=characters[target]; }
			const auto n=draw.draw(1,monster.damageDie()); if (!n) break;
			damage=physicalChecked(std::int64_t(damage)+*n);
			if (++dice==monster.strikes()) step=type==XeenDamageType::Physical ? afterSaves() : Step::InitialSave;
			break;
		}
		case Step::InitialSave: {
			// DOS 0x2d2e4..0x2d2f7: the first save uses the Physical selector for
			// every nonphysical type; the pinned reference wrongly uses the element.
			const auto v=physicalSave(characters[target]);
			const auto n=draw.draw(1,unsigned(v+20)); if (!n) break;
			if (std::int64_t(*n)<=v) damage/=2;
			// DOS: the initial Physical save precedes flat party protection.
            if(unsigned(type)>=2 && unsigned(type)<=5)
                damage=physicalChecked(std::int64_t(damage)-partyContext.lightAndResistances[unsigned(type)]);
			step=Step::ElementSave;
			break;
		}
		case Step::ElementSave: {
			// DOS 0x2d35e..0x2d374: drawn once unconditionally on entry, even at zero
			// damage; halve on success and repeat while damage stays nonzero.
			const auto r=Rules::damageSaveValue(characters[target],inputs[target],type,{year});
			ruleRequire(r>=0 && r<=std::numeric_limits<int>::max()-40,"Invalid element save interval");
			const auto n=draw.draw(1,unsigned(r)+40); if (!n) break;
			if (int(*n)<=r) { damage/=2; if (damage!=0) break; }
			step=afterSaves();
			break;
		}
        case Step::Portrait:
            if(!injuryAcknowledged) {impactOwner=characters[target].rosterId;injuryReady=true;return false;}
            injuryReady=injuryAcknowledged=false;
            step=damage>0 && monster.specialAttack() ? Step::Special : Step::Injury;break;
		case Step::Special: {
			auto &c=characters[target];
			const auto v=physicalSave(c);
			const auto n=draw.draw(1,unsigned(v+20)); if (!n) break;
			if (std::int64_t(*n)>v) xeenApplySpecialAttack(c,monster.specialAttack());
			step=Step::Injury; break;
		}
		case Step::Injury: {
			auto &c=characters[target];
			XeenCombatDamage injury; injury.owner=c.rosterId; injury.amount=damage;
			injury.attackOrdinal=attackOrdinal;
			injury.beforeHp=before->currentHp; injury.beforeAc=beforeDamageAc;
			injury.beforeSp=before->currentSp; injury.beforeAge=before->temporaryAge;
			xeenApplyPhysicalInjury(c,damage,year);
			injury.afterHp=c.currentHp; injury.afterAc=Rules::combatArmorClass(c,inputs[target],{year,partyContext.effects[XeenPartyEffect::Bless]}); injury.conditions=c.conditions;
			injury.afterSp=c.currentSp; injury.afterAge=c.temporaryAge;
			result.injuries.at(result.injuryCount++)=injury;
			xeenRecordItemChanges(result,*before,c);
			result.damage=physicalChecked(std::int64_t(result.damage)+damage);
			damage=0; dice=0; before.reset(); step=afterInjury;
            if(deferInjury) {injuryApplied=true;return false;}
            break;
		}
		case Step::Next:
			if (allParty && ++participantCursor<participantCount) { target=participants[participantCursor]; step=Step::Begin; }
            else step=Step::Done;
			break;
		case Step::Done: break;
		}
	}
	if (step!=Step::Done) return false;
	result.attackOutcome=!participantCount ? XeenCombatAttackOutcome::NoParticipants : !result.injuryCount ? XeenCombatAttackOutcome::Miss : result.damage ?
		XeenCombatAttackOutcome::HitPositiveDamage : XeenCombatAttackOutcome::HitZeroDamage;
	return true;
}
bool XeenEnemyAttackCandidate::nextAttack() {
    ruleRequire(step==Step::Done,"Cannot advance an unfinished monster attack");
    if(disabled || noTarget || ++attackOrdinal>=monster.attacks())return false;
    result=XeenCombatResult{};result.operation=XeenCombatOperation::EnemyAttack;
    target=-1;participantCursor=0;step=Step::Target;return true;
}
unsigned xeenElementalCategory(unsigned material) {
	if (material>36) throw std::invalid_argument("Elemental category of a non-elemental material");
	unsigned index=0;
	while (item_tables::elementCategories[index]<int(material)) ++index;
	return index;
}
XeenPhysicalPlayerCandidate::XeenPhysicalPlayerCandidate(const XeenCharacter &c,
        const XeenCombatInputs &i,const XeenMonsterRecord &m,unsigned type,unsigned year,bool shoot,
        XeenDifficulty difficulty,XeenActorStatus status)
    : XeenPhysicalPlayerCandidate(c,i,m,type,year,shoot,difficulty,status,{}) {}
void XeenPhysicalPlayerCandidate::setPartyEffects(const XeenGameplayContext &context) {
    ruleRequire(step==Step::Weapon && slot==0 && dice==0,"Player party effects must be bound before service");
    heroism=context.effects[XeenPartyEffect::Heroism];holyBonus=context.effects[XeenPartyEffect::HolyBonus];
}
XeenPhysicalPlayerCandidate::XeenPhysicalPlayerCandidate(const XeenCharacter &c,
    const XeenCombatInputs &i,const XeenMonsterRecord &m,unsigned type,unsigned year,bool shoot,
    XeenDifficulty difficulty,XeenActorStatus status,const XeenGameplayContext &partyContext)
    : XeenPhysicalPlayerCandidate(c,i,m,type,year,shoot,difficulty,status,false,true,partyContext) {}
XeenPhysicalPlayerCandidate::XeenPhysicalPlayerCandidate(const XeenCharacter &c,
		const XeenCombatInputs &i,const XeenMonsterRecord &m,unsigned type,unsigned year,bool missile,XeenDifficulty difficulty, XeenActorStatus initialStatus, bool poison, bool isClouds,const XeenGameplayContext &partyContext) :
		status(initialStatus),character(c),monster(m),monsterType(type),shoot(missile),adventurer(difficulty==XeenDifficulty::Adventurer),poisonVolley(poison),clouds(isClouds),heroism(partyContext.effects[XeenPartyEffect::Heroism]),holyBonus(partyContext.effects[XeenPartyEffect::HolyBonus]) {
	ruleRequire(xeenActorStatusSupported(status),"Unsupported physical target status");
	constexpr unsigned divisors[]{1,2,2,3,4,2,2,1,3,2};
	ruleRequire(unsigned(c.characterClass)<10 && m.physicalResistance()<=100 && unsigned(difficulty)<=1,"Invalid physical player operands");
	baseHit=physicalChecked(std::int64_t(Rules::physicalBonus(Rules::effectivePhysical(c,i,Rules::PhysicalAttribute::Accuracy,{year})))+(adventurer?5:0)+c.currentLevel()/divisors[unsigned(c.characterClass)]);
	hitTotal=baseHit;
	might=shoot ? 0 : Rules::physicalBonus(Rules::effectivePhysical(c,i,Rules::PhysicalAttribute::Might,{year}));
	attacks=shoot ? 1 : xeenCombatAttackCount(c.characterClass,c.currentLevel());
	// Combat::attack: after the attempts, every equipped weapon (frame != 0,
	// bad and missile weapons included) whose effectiveness counter names the
	// monster's species triples the melee total once. Shoot never applies it.
	// Counter 1 Dragon(6), 2 Undead(4), 3 Golem(5), 4 Insect(2), 5 Monsters(0), 6 Animal(1).
	constexpr unsigned species[]{255,6,4,5,2,0,1};
	if (!shoot) for (const auto &item:c.weapons) if (item.frame) {
		const unsigned counter=item.state&0x3f;
		if (counter>=1 && counter<=6 && species[counter]==m.monsterType()) effectiveness=physicalChecked(std::int64_t(effectiveness)*3);
	}
}
bool XeenPhysicalPlayerCandidate::service(XeenConsequenceDraw &draw) {
	while (step!=Step::Done && draw.remaining) switch (step) {
	case Step::Weapon: {
		if (dice) { const auto n=draw.draw(1,sides); if (n) { weapon=physicalChecked(std::int64_t(weapon)+*n); --dice; } break; }
		if (slot==9) {
			// getWeaponDamage: clamp below 1 to 0, then Adventurer triples and adds 5 to hit.
			if (weapon<1) weapon=0;
			weapon=physicalChecked(std::int64_t(weapon)*(adventurer?3:1));
			hitTotal=physicalChecked(std::int64_t(baseHit)+hitBonus);step=Step::Hit;break;
		}
		const auto &item=character.weapons[slot++];
		if (shoot ? item.frame!=4 : item.frame!=1 && item.frame!=13) break;
		ruleRequire(item.id && item.id<=34 && (!shoot || (item.id>=30 && item.id<=33)),"Unsupported equipped physical weapon");
		if (!(item.state&0xc0)) {
			// A good weapon's material: elemental selector below 37, metal hit and
			// damage modifiers 37-58 (assigned, not accumulated), nothing above.
			if (item.material<37) elementalMaterial=item.material;
			else if (item.material<59) { hitBonus=item_tables::metalHit[item.material-37];weapon=item_tables::metalDamage[item.material-37]; }
		}
		hitBonus=physicalChecked(std::int64_t(hitBonus)+heroism);
        weapon=physicalChecked(std::int64_t(weapon)+holyBonus);
        attackWeaponId=item.id;
		const auto value=xeenOrdinaryWeaponDice(item.id); dice=value.count;sides=value.sides;break;
	}
	case Step::Hit: {
		const auto n=draw.draw(1,20);if (!n) break;
		hitTotal=physicalChecked(std::int64_t(hitTotal)+*n);if (*n==20) break;
		if (status!=XeenActorStatus::Physical) hitTotal=physicalChecked(std::int64_t(hitTotal)+20);
		// DOS compares the final player hit total as a 16-bit unsigned word
		// (sv-hit-total-unsigned), shared by melee and missile attacks.
		if (static_cast<std::uint16_t>(hitTotal)>=unsigned(monster.armorClass()+10)) {
			hit=true;accumulated=physicalChecked(std::int64_t(accumulated)+(shoot ? weapon : std::max(physicalChecked(std::int64_t(weapon)+might),1)));
		}
		if (--attacks) { slot=0;weapon=0;hitBonus=0;elementalMaterial=0;hitTotal=baseHit;step=Step::Weapon;break; }
		// attack2: effectiveness, then physical resistance (bypassed from the Xeen
		// Slayer Sword, ID 34, upward), then the elemental material once, only for
		// a nonzero accumulated input; the Shoot save halves the combined amount.
        if(poisonVolley) {
            accumulated=hit?10:0;
            if(hit && clouds && monsterType==89) {step=Step::Done;break;}
        }
        impact=accumulated!=0;
		accumulated=physicalChecked(std::int64_t(accumulated)*effectiveness);
		status=xeenActorStatusAfterDamage(status,accumulated!=0);
		physical=poisonVolley || attackWeaponId>=34 ? accumulated : physicalChecked(std::int64_t(accumulated)*(100-monster.physicalResistance())/100);
		elemental=0;elementalFrame=0;
		if (accumulated>0 && elementalMaterial) {
			const auto category=xeenElementalCategory(elementalMaterial);
			const auto resistance=monster.elementalResistance(category);
			ruleRequire(resistance<=100,"Monster elemental resistance outside 0..100");
			elemental=resistance==100 ? 0 : physicalChecked(std::int64_t(100-resistance)*item_tables::elemental[elementalMaterial]/100);
			if (elemental>0) elementalFrame=category+1;
		}
		damage=physicalChecked(std::int64_t(physical)+elemental);
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
// Party::resetTemps, shared with Rest and Training.
void xeenResetCharacterTemps(XeenCharacter &c,XeenCombatInputs &i) {
 c.temporaryLevel=0;c.intellect.temporary=c.personality.temporary=c.endurance.temporary=0;
 i.might.temporary=i.speed.temporary=i.accuracy.temporary=i.temporaryAc=0;
 if(i.luck)i.luck->temporary=0;
 if(i.poisonResistance)i.poisonResistance->temporary=0;
 if(i.resistances) {
  auto &r=*i.resistances;r.fireTemporary=r.coldTemporary=r.electricalTemporary=0;
  r.energyTemporary=r.magicTemporary=0;
 }
}
void xeenResetPartyTemps(XeenGameplayContext &c) {
 // automapOn and torchCount survive; temporary age and conditions also survive.
 for(auto effect:kXeenPartyEffects)if(effect!=XeenPartyEffect::AutomapOn)c.effects[effect]=0;
 c.lightAndResistances[0]=0;for(unsigned n=2;n<6;++n)c.lightAndResistances[n]=0;
}
XeenConditionTimeCandidate::XeenConditionTimeCandidate(const XeenGameplayContext &before,std::uint64_t charge,
 const XeenConsequenceCharacters &c,const XeenConsequenceInputs &i,const XeenServiceEconomy *e,
 XeenTimeMode m,XeenTimeCall operation):context(before),characters(c),inputs(i),mode(m),call(operation) {
 ruleRequire(xeenRegionalContext(before),"Noncanonical consequence time context");
 ruleRequire(static_cast<unsigned>(mode)<=3 && static_cast<unsigned>(call)<=1,"Invalid time call mode");
 const auto prepared=xeenPrepareTime(before,charge);ending=prepared.context;
 if(e){xeenValidateCurrentServiceEconomy(*e);economy=*e;}
 if(xeenServiceDayRegenerates(before.day,ending.day,charge)) {
  ruleRequire(e,"Missing daily economy");stock=std::make_shared<XeenMerchantStockCandidate>();
 }
 step=call==XeenTimeCall::Change && prepared.processing480 ? Step::Stats : Step::Economy;
 for(const auto &input:inputs)ruleRequire(input.resistances.has_value(),"Missing condition-tick resistances");
}
bool XeenConditionTimeCandidate::service(XeenConsequenceDraw &draw) {
 if(draw.check)draw.check();
 while(step!=Step::Done && draw.remaining) {
  auto &c=characters[owner];auto &i=inputs[owner];
  switch(step) {
  case Step::Stats:
   if(!c.conditions[13] && !c.conditions[14] && !c.conditions[15]) {
    for(unsigned stat=0;stat<7;++stat)
     if(Rules::sheetStat(c,&i,stat,{context.year})<1)c.conditions[13]=1;
   }
   if(c.conditions[1]) {c.conditions[1]=std::uint8_t(unsigned(c.conditions[1])+1);
    if(c.conditions[1]>10){c.conditions[1]=0;c.conditions[9]=1;}}
   step=Step::Poison;break;
  case Step::Poison: {
   // DOS WORLD/XEEN.DAT 0x25425/0x25473: per member, Poison then
   // Disease, only when present. Failed checks double the byte with wrap.
   if(!c.conditions[3]){step=Step::Disease;break;}
   const auto n=draw.draw(1,10);
   if(n) {if(*n!=1)c.conditions[3]=std::uint8_t(unsigned(c.conditions[3])*2);
    step=*n==1?Step::Electrical:Step::Disease;}break;
  }
  case Step::Electrical: {
   const int v=Rules::damageSaveValue(c,i,XeenDamageType::Electrical,{context.year});
   ruleRequire(v>=0 && v<=std::numeric_limits<int>::max()-40,"Invalid Poison condition save interval");
   // DOS 0x2d164/0x2d200: resistance + temporary + equipment;
   // draw 1..(value+40), pass at <= value.
   const auto n=draw.draw(1,unsigned(v)+40);
   if(n){c.conditions[3]=*n<=unsigned(v)?0:std::uint8_t(unsigned(c.conditions[3])*2);step=Step::Disease;}break;
  }
  case Step::Disease: {
   if(!c.conditions[4]){step=Step::Remaining;break;}
   const auto n=draw.draw(1,10);
   if(n){if(*n!=1)c.conditions[4]=std::uint8_t(unsigned(c.conditions[4])*2);
    step=*n==1?Step::Cold:Step::Remaining;}break;
  }
  case Step::Cold: {
   const int v=Rules::damageSaveValue(c,i,XeenDamageType::Cold,{context.year});
   ruleRequire(v>=0 && v<=std::numeric_limits<int>::max()-40,"Invalid Disease condition save interval");
   const auto n=draw.draw(1,unsigned(v)+40);
   if(n){c.conditions[4]=*n<=unsigned(v)?0:std::uint8_t(unsigned(c.conditions[4])*2);step=Step::Remaining;}break;
  }
  case Step::Remaining:
   if(c.conditions[5])c.conditions[5]=std::uint8_t(unsigned(c.conditions[5])+1);
   for(unsigned n=13;n<16;++n)if(c.conditions[n] && c.conditions[n]!=255)++c.conditions[n];
   if(c.conditions[6]) {c.conditions[6]=std::uint8_t(unsigned(c.conditions[6])+1);
    if(c.conditions[6]>10){c.conditions[6]=0;c.conditions[1]=1;}}
   // DOS 0x255ab: unconditional replacement, including Weak/Drunk 255.
   c.conditions[2]=c.conditions[7];c.conditions[7]=0;
   if(c.conditions[9])c.conditions[9]=(unsigned(c.conditions[9])+1)%4;
   if(++owner==6){owner=0;step=Step::Economy;}else step=Step::Stats;break;
  case Step::Economy:
   if(stock) {
    // Copies are independent detached continuations, including partial stock.
    if(!stock.unique())stock=std::make_shared<XeenMerchantStockCandidate>(*stock);
    if(!stock->service(draw))return false;
    xeenValidateMerchantWares(stock->wares());economy->wares=stock->wares();
    economy->bank=xeenPrepareBankInterest(economy->bank);stock.reset();
   }
   context=ending;step=Step::Dawn;break;
  case Step::Dawn:
   if(context.newDay && context.minutes>=300) {
    if(mode!=XeenTimeMode::Script && mode!=XeenTimeMode::Interactive7) {
     resetTemps=true;xeenResetPartyTemps(context);
     for(unsigned n=0;n<6;++n)xeenResetCharacterTemps(characters[n],inputs[n]);
     if(context.rested || mode==XeenTimeMode::Sleeping)context.rested=false;
     else {
      needsRest=true;
      // DOS 0x26452: unsigned increment stops at 255.
      for(auto &v:characters)if(v.conditions[2]!=255)v.conditions[2]=std::uint8_t(unsigned(v.conditions[2])+1);
     }
    }
    context.newDay=false;
   }
   step=call==XeenTimeCall::Change?Step::Confused:Step::Done;break;
  case Step::Confused: {
   if(!c.conditions[10]){step=Step::Paralyzed;break;}
   const auto n=draw.draw(0,2);if(n)step=*n==1?Step::Physical:Step::Paralyzed;break;
  }
  case Step::Physical: {
   const int v=Rules::damageSaveValue(c,i,XeenDamageType::Physical,{context.year});
   ruleRequire(v>-20 && v<=std::numeric_limits<int>::max()-20,"Invalid Confused physical save interval");
   const auto n=draw.draw(1,unsigned(v+20));
   if(n){if(int(*n)<=v)c.conditions[10]=0;else --c.conditions[10];step=Step::Paralyzed;}break;
  }
  case Step::Paralyzed: {
   if(c.conditions[11]) {const auto n=draw.draw(0,4);if(!n)break;if(*n==1)--c.conditions[11];}
   if(++owner==6){owner=0;step=Step::Done;}else step=Step::Confused;break;
  }
  case Step::Done:break;
  }
 }
 return step==Step::Done;
}
}
