// Adapted from ScummVM interface.cpp/scripts.cpp at 6814ee9b.
// ScummVM developers (upstream COPYRIGHT), GPL-3.0-or-later.
#ifndef MMODERN_XEEN_BARRIER_RULES_H
#define MMODERN_XEEN_BARRIER_RULES_H
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenWorld.h"
namespace mmodern {
struct XeenBarrierCandidate {
	XeenConsequenceCharacters characters;
	XeenConsequenceInputs inputs;
	XeenCombatRandom random;
	XeenCamera camera;
	XeenDamageProtection protection;
	std::optional<XeenTypedDamageCandidate> injury;
	unsigned year=610,wall=0,targetWall=0,target=0,portraitMask=0,portraitFrame=0,animation=0;
	int threshold=0,trapDamage=0;
	bool bash=false,selection=false,handled=false,opened=false,moved=false,done=false,deferInjury=false;
	bool damagePauseReady=false,damagePauseAcknowledged=false;
	enum class Step { Select, Trap, Type, Damage, DamagePause, Unlock, BashRoll, Done } step=Step::Done;
	XeenBarrierCandidate(XeenWorld &world,const XeenCamera &c,const XeenConsequenceCharacters &chars,
		const XeenConsequenceInputs &in,const XeenJourneyRandomState &rng,const XeenGameplayContext &context,bool b) :
		characters(chars),inputs(in),random(rng),camera(c),year(context.year),bash(b) {
		if(unsigned(c.direction)>3)throw std::invalid_argument("Invalid barrier facing");
		const auto sample=world.sampleCell(c.mapId,c.x,c.y);
		if(!sample)throw std::invalid_argument("Barrier camera cell is absent");
		// mazeData() always retains the primary map; getCell resolves only geometry.
		const auto &map=world.map(c.mapId).geometry;
		// perform(B) charges outdoors before Interface::bash returns immediately.
		if(map.isOutdoors()) {handled=bash;done=true;return;}
		wall=wallAt(*sample->cell,c.direction);
		const bool unlocked=(sample->cell->rawAttributes&0x80)!=0;
		const auto &difficulty=map.difficulties;
		if(bash) {
			handled=true;
			if(wall<unsigned(difficulty[0])) {
				constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
				const auto next=world.sampleCell(c.mapId,c.x+dx[unsigned(c.direction)],c.y+dy[unsigned(c.direction)]);
				if(!next || c.x+dx[unsigned(c.direction)]<0 || c.x+dx[unsigned(c.direction)]>=32 ||
					c.y+dy[unsigned(c.direction)]<0 || c.y+dy[unsigned(c.direction)]>=32)
					throw std::invalid_argument("Bash movement leaves logical map");
				// perform(B): a passable forward wall moves without checkMoveDirection's surface test.
				camera.x+=dx[unsigned(c.direction)];camera.y+=dy[unsigned(c.direction)];moved=true;done=true;return;
			}
			unsigned count=0;
			for(unsigned n=0;n<6 && count<2;++n) {
				const auto condition=characters[n].worstCondition();
				if(condition==XeenCondition::Asleep || (condition>=XeenCondition::Paralyzed && condition<=XeenCondition::Eradicated))continue;
				if(!count)target=n;
				portraitMask|=1u<<n;++count;xeenApplyPhysicalInjury(characters[n],2,year);
			}
			if(!count)throw std::invalid_argument("No member can Bash");
			constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
			if(!world.sampleCell(c.mapId,c.x+dx[unsigned(c.direction)],c.y+dy[unsigned(c.direction)])) {done=true;return;}
			if(wall==7 || wall==14 || wall==15) {animation=wall;done=true;return;}
			threshold=difficulty[wall==9?5:6];targetWall=3;step=Step::BashRoll;
		} else {
			targetWall=wall==1?13:wall==6?9:wall==9?6:wall==13?1:0;
			if(!targetWall || (targetWall==13 && !unlocked) ||
				(c.mapId.side==XeenSide::Darkside && targetWall==9 && map.wallKind==2)) {done=true;return;}
			handled=true;threshold=difficulty[2];trapDamage=map.trapDamage;
			selection=targetWall!=9 && !unlocked;step=selection?Step::Select:Step::Done;
			if(!selection) {opened=true;done=true;}
		}
		for(unsigned n=0;n<4;++n)protection.resistances[n]=context.lightAndResistances[n+2];
		protection.powerShield=context.effects[6];
	}
	void choose(unsigned member) {
		if(step!=Step::Select || member>=6 || !characters[member].canAct())throw std::logic_error("Stale or ineligible unlock selection");
		target=member;selection=false;step=Step::Trap;
	}
	bool service(XeenConsequenceDraw &draw) {
		if(done)return true;
		while(draw.remaining && step!=Step::Select && step!=Step::Done) {
			switch(step) {
			case Step::Trap: {const auto n=draw.draw(1,4);if(n)step=*n==1?Step::Type:Step::Unlock;break;}
			case Step::Type: {const auto n=draw.draw(0,6);if(n) {injury.emplace(characters,inputs,trapDamage,XeenDamageType(*n),year,1u<<target,protection);injury->deferInjury=deferInjury;step=Step::Damage;}break;}
			case Step::Damage:
				if(!injury->service(draw))return false;
				characters=injury->characters;portraitMask=1u<<target;portraitFrame=injury->portraitFrame;step=Step::DamagePause;break;
			case Step::DamagePause:
				// giveCharDamage's ipause(5) returns before the unlock roll.
				if(deferInjury && !damagePauseAcknowledged) {damagePauseReady=true;return false;}
				damagePauseReady=false;step=Step::Unlock;break;
			case Step::Unlock: {
				const auto n=draw.draw(1,20);if(!n)break;
				opened=XeenCharacterRules::thievery(characters[target])+int(*n)>=threshold;
				if(opened) {
					const auto level=characters[target].currentLevel();
					const auto xp=std::int64_t(inputs[target].experience)+std::int64_t(threshold)*level;
					if(xp<0 || xp>std::numeric_limits<std::uint32_t>::max())throw std::overflow_error("Unlock XP overflow");
					inputs[target].experience=std::uint32_t(xp);
				}
				step=Step::Done;break;
			}
			case Step::BashRoll: {
				const auto n=draw.draw(1,30);if(!n)break;
				int might=int(*n);
				for(unsigned member=0;member<6;++member)if(portraitMask&(1u<<member))
					might+=XeenCharacterRules::sheetStat(characters[member],&inputs[member],0,{year});
				opened=might>=threshold;step=Step::Done;break;
			}
			default:break;
			}
		}
		done=step==Step::Done;return done;
	}
};
}
#endif
