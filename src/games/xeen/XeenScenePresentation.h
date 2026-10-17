#ifndef MMODERN_XEEN_SCENE_PRESENTATION_H
#define MMODERN_XEEN_SCENE_PRESENTATION_H

#include "games/xeen/XeenActor.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "core/NavigationAction.h"
#include "core/PresentationClock.h"
#include "formats/xeen/XeenMonsterAppearance.h"
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <tuple>

namespace mmodern {
// Adapted from ScummVM's interface_scene.cpp and constants.cpp at
// 6814ee9ba54582f5b5adcffab49efbbd8f589edd, GPL-3.0-or-later.
// Disposable presentation state; never included in the session/save or guards.
struct XeenMonsterAnimation {
	unsigned frame=0, postAttackDelay=0, effect1=0, effect2=0, effect3=0;
	bool reverse=false;
	void attack(unsigned delay) { frame=8; postAttackDelay=delay; }
	void hit(unsigned delay) { frame=11; postAttackDelay=delay; }
	void advance(const XeenMonsterRecord &data, XeenActorStatus status=XeenActorStatus::Physical) {
		if (status==XeenActorStatus::Physical) {
		if (frame<8) {
			if (!data.loopAnimation()) frame=(frame+1)%8;
			else if (!reverse) {frame=(frame+1)%8; if (!frame) {reverse=true;frame=6;}}
			else if (frame && !--frame) reverse=false;
		} else if (frame==11) {
			if (postAttackDelay && !--postAttackDelay) frame=0;
		} else if (++frame==11) {
			if (postAttackDelay) --postAttackDelay;
			frame=postAttackDelay?10:0;
		}
		}
		if (effect2) {
			if (effect1) {
				if (effect1&0x80) {if (effect3) --effect3; if (!effect3) effect1^=0x80;}
				else {effect3=(effect3+1)%3; if (!effect3) {effect1^=0x80;effect3=2;}}
			} else if (!(effect3=(effect3+1)%8)) effect1=effect2=data.animationEffect();
		}
	}
	unsigned flags() const {
		static constexpr unsigned table[15][8]{
			{0x104,0x105,0x106,0x107,0x108,0x109,0x10a,0x10b},
			{0x10c,0x10d,0x10e,0x10f}, {0x110,0x111,0x112,0x113}, {0x114,0x115,0x116,0x117},
			{0x200,0x201,0x202,0x203}, {0x300,0x301,0x302,0x303,0x400,0x401,0x402,0x403},
			{0x500,0x501,0x502,0x503}, {0x600,0x601,0x602,0x603},
			{0x604,0x605,0x606,0x607,0x608,0x609,0x60a,0x60b}, {0x60c,0x60d,0x60e,0x60f},
			{0x100,0x100,0x100,0x100,0x100,0x100,0x100,0x100},
			{0x101,0x101,0x101,0x101,0x101,0x101,0x101,0x101},
			{0x102,0x102,0x102,0x102,0x102,0x102,0x102,0x102},
			{0x103,0x103,0x103,0x103,0x103,0x103,0x103,0x103},
			{0x108,0x108,0x108,0x108,0x108,0x108,0x108,0x108}};
		if (effect2>15 || effect3>7) throw std::invalid_argument("Invalid monster effect phase");
		return effect2?table[effect2-1][effect3]:0;
	}
};

class XeenScenePresentation {
public:
 std::map<XeenObjectIdentity,bool> objectFlips;
 // giveExt's chest frame (0x235f6, 0x23703): open while the lock is picked,
 // until the map is loaded again. Not saved, as in DOS.
 std::map<XeenObjectIdentity,unsigned> objectFrames;
 struct PortraitEffect {unsigned spellFrame=4, damageFrame=0, damageTicks=0;std::uint64_t spellDeadline=0,damageDeadline=0;};
 // One physical (frame 0) or spell splat per row, plus the original's separate
 // elemental weapon splat beside it (_elemFrame/_elemScale, M67).
 struct Splat {unsigned frame=0, duration=0;int damage=0;bool alternatePosition=false;std::uint64_t deadline=0;
  unsigned elementalFrame=0;int elementalDamage=0;};
 std::array<PortraitEffect,30> portraits{};
 std::array<Splat,3> splats{};
 bool feedbackActive() const noexcept {
  for(const auto &p:portraits)if(p.spellFrame<4 || p.damageTicks)return true;
  for(const auto &s:splats)if(s.duration)return true;
  return false;
 }
 void spellEffect(unsigned owner,std::uint64_t=0,bool reset=false) noexcept {
  // Character::addHitPoints clears _charFX for each invocation, including
  // zero-HP Cure Poison; giveTake's duplicate recipients remain suppressed.
  if(owner<portraits.size() && (reset || portraits[owner].spellFrame==4)) {portraits[owner].spellFrame=0;portraits[owner].spellDeadline=0;}
 }
 void portraitDamage(unsigned owner,unsigned frame,std::uint64_t=0) noexcept {
  if(owner<portraits.size()) {portraits[owner].damageFrame=frame;portraits[owner].damageTicks=1;portraits[owner].damageDeadline=0;}
 }
 void hitSplat(unsigned row,int damage,unsigned frame,bool alternatePosition,std::uint64_t=0,
   unsigned elementalFrame=0,int elementalDamage=0) noexcept {
  if(row<3 && damage>0)splats[row]={frame,3,damage,alternatePosition,0,elementalFrame,elementalDamage};
 }
 // Flow admits one retained frame at a time and owns its clock/acquisition.
 // Repaint/polling never advances a recipient or consumes a hold.
 bool advanceFeedback(std::uint64_t) noexcept {return false;}
 void spellFrame(unsigned owner,unsigned frame) {
     if(owner>=portraits.size() || frame>4)throw std::invalid_argument("Invalid retained portrait sparkle");
     portraits[owner].spellFrame=frame;portraits[owner].spellDeadline=0;
 }
 void clearDamage() noexcept {for(auto &portrait:portraits){portrait.damageTicks=0;portrait.damageDeadline=0;}}

	explicit XeenScenePresentation(std::uint32_t seed=std::random_device{}()) : _random(seed) {}
	bool ground=false, defaultGround=false, sky=false, water=false;
	unsigned overallFrame=0, floatPhase=0;
	std::uint64_t wallPhase=0;
	void enterMap(XeenMapIdentity map) {
		const auto key=std::make_pair(XeenSide(map.side),std::uint16_t(map.number));
		if (_map && *_map==key) return;
		// MonsterObjectData reconstructs normal/effect phases and starts wall
		// frames at zero on map entry. Cache reconstruction is not map entry.
		for(auto i=_actors.begin();i!=_actors.end();) {
			if(i->first.mapId==map) i=_actors.erase(i);else ++i;
		}
		_map=key;_wallOrigin=wallPhase;_lastAttack.reset();
	}
	std::size_t wallFrame(std::size_t count) const {
		if(!count) throw std::invalid_argument("Empty wall-item animation");
		return (wallPhase-_wallOrigin)%count;
	}
	void navigation(NavigationAction action, bool moved, bool combat=false) {
		const bool turn=action==NavigationAction::TurnLeft || action==NavigationAction::TurnRight;
		const bool strafe=action==NavigationAction::StrafeLeft || action==NavigationAction::StrafeRight;
		if (turn || (!combat && moved && strafe)) sky=!sky;
		if (!combat && (turn || moved)) {ground=!ground;defaultGround=!defaultGround;}
	}
	// perform's Wait flips cancel the flips in stepTime.
	void wait() {}
	void include(const std::vector<XeenActor> &actors) {
		for (const auto &a:actors) if (a.statistics && a.statistics->supportsRendering()) {
			if (auto i=_actors.find(a.id);i!=_actors.end()) {i->second.status=a.status;continue;}
			XeenMonsterAnimation animation;
			// Original normal-frame initialization and the approved effect-phase
			// initialization both consume only disposable cosmetic randomness.
			animation.frame=std::uniform_int_distribution<unsigned>(0,7)(_random);
			animation.effect1=animation.effect2=a.statistics->animationEffect();
			// Approved M48 deviation: random initial phases use only this cosmetic RNG.
			if (animation.effect2) animation.effect3=std::uniform_int_distribution<unsigned>(0,7)(_random);
			_actors.emplace(a.id, Entry{*a.statistics,animation,a.status});
		}
	}
	const XeenMonsterAnimation *animation(XeenMonsterIdentity id) const {
		const auto i=_actors.find(id);return i==_actors.end()?nullptr:&i->second.animation;
	}
    // attack2's RT_GROUP path assigns the transient monster tint even when
    // physical resistance reduces the accepted spell to zero damage. Call once
    // after an accepted magic gate and nonzero base damage, never on redraw.
    void spellImpact(const XeenActor &actor,XeenLearnedSpell spell,unsigned type=0) {
        unsigned effect;
        switch(spell) {
        case XeenLearnedSpell::FingerOfDeath: case XeenLearnedSpell::MassDistortion:
        case XeenLearnedSpell::EnergyBlast: effect=5;break;
        // Combat::DAMAGE_TYPE_EFFECTS; spell-handler FX14/17/18 are deferred audio.
        case XeenLearnedSpell::HolyWord: case XeenLearnedSpell::TurnUndead: effect=14;break;
        case XeenLearnedSpell::PoisonVolley: case XeenLearnedSpell::InsectSpray: case XeenLearnedSpell::ToxicCloud: effect=2;break;
        case XeenLearnedSpell::Sparks: effect=11;break;
        case XeenLearnedSpell::GolemStopper: case XeenLearnedSpell::MagicArrow: case XeenLearnedSpell::Shrapmetal:
        case XeenLearnedSpell::FlyingFist: case XeenLearnedSpell::Pain: effect=3;break;
        case XeenLearnedSpell::AcidSpray: case XeenLearnedSpell::ColdRay:
        case XeenLearnedSpell::DancingSword: case XeenLearnedSpell::DeadlySwarm:
        case XeenLearnedSpell::ElementalStorm: case XeenLearnedSpell::FantasticFreeze:
        case XeenLearnedSpell::FieryFlail: case XeenLearnedSpell::Fireball:
        case XeenLearnedSpell::Frostbite: case XeenLearnedSpell::Implosion:
        case XeenLearnedSpell::Incinerate: case XeenLearnedSpell::Inferno:
        case XeenLearnedSpell::LightningBolt: case XeenLearnedSpell::MegaVolts:
        case XeenLearnedSpell::MoonRay: case XeenLearnedSpell::PrismaticLight:
        case XeenLearnedSpell::StarBurst: case XeenLearnedSpell::SunRay: {
            constexpr unsigned effects[]{3,10,4,11,1,2,5};
            if(type>=std::size(effects))throw std::invalid_argument("Invalid spell damage tint");
            effect=effects[type];break;
        }
        default: throw std::invalid_argument("Unsupported spell impact presentation");
        }
        include({actor});
        const auto entry=_actors.find(actor.id);
        if(entry==_actors.end())return;
        entry->second.animation.effect1=0;
        entry->second.animation.effect2=effect;
        // attack2 leaves effect3 intact; advance restores the resource's own
        // animationEffect at the next original eight-phase wrap.
    }
	void spawn(const XeenActor &actor) {
		const bool present=_actors.count(actor.id)!=0;
		include({actor});
		auto &animation=_actors.at(actor.id).animation;
		if(present)animation.frame=std::uniform_int_distribution<unsigned>(0,7)(_random);
		animation.postAttackDelay=0;animation.reverse=false;
	}
	void copySpawn(const XeenActor &actor,const XeenScenePresentation &candidate) {
		const auto found=candidate._actors.find(actor.id);
		if(found==candidate._actors.end())throw std::logic_error("Spawn animation is absent");
		_actors.insert_or_assign(actor.id,found->second);
	}
	void copySpawnRandom(const XeenScenePresentation &candidate) {
		// Publish the consumed cosmetic cursor without replacing live animation phases.
		_random=candidate._random;
	}
	void appearance(const XeenMonsterAppearance &value) {
		if (!value.identity || value.kind!=XeenMonsterSpriteKind::Attack) {_lastAttack.reset();return;}
		const auto key=std::make_tuple(value.identity->mapId.side,
			std::uint16_t(value.identity->mapId.number),std::size_t(value.identity->recordIndex),unsigned(value.frame));
		if (_lastAttack && *_lastAttack==key) return;
		_lastAttack=key;
		const auto i=_actors.find(*value.identity);
		if (i!=_actors.end()) {i->second.animation.frame=8+value.frame;i->second.animation.postAttackDelay=value.frame==3?5:1;}
	}
	// draw3d updates UI/water before composition (DAT 0x10bda..0x10bf4).
	void beginDraw() noexcept {
		if (!(_uiPhase=(_uiPhase+1)%4)) water=!water;
	}
	// animate3d runs after the chosen scene has been composed (DAT 0x107b6).
	void advanceAfterDraw(XeenMapIdentity map) {
		overallFrame=(overallFrame+1)%5;floatPhase=(floatPhase+1)%8;++wallPhase;
		for (auto &[id,entry]:_actors) if (id.mapId==map) entry.animation.advance(entry.data,entry.status);
        for(auto &s:splats)if(s.duration)--s.duration;
	}
	void advance(XeenMapIdentity map) {composeFromRetained(false);beginDraw();advanceAfterDraw(map);}
	// Reconstruction reads the values that produced the retained frame, while
	// the live values already describe the next draw. Neither view owns gameplay.
	void composeFromRetained(bool reconstruct) noexcept {_reconstruct=reconstruct && bool(_drawn);}
	const XeenScenePresentation &drawingState() const noexcept {return _reconstruct?*_drawn:*this;}
	void retainDrawingState() {
		if(_reconstruct)return;
		auto drawn=std::make_shared<XeenScenePresentation>(*this);
		drawn->_drawn.reset();drawn->_reconstruct=false; // No snapshot chain.
		_drawn=std::move(drawn);
	}
	static int floatX(unsigned phase) {static constexpr int values[]{-2,-1,0,1,2,1,0,-1};return values[phase%8];}
	static int floatY(unsigned phase) {static constexpr int values[]{-2,0,2,0,-1,0,2,0};return values[phase%8];}
private:
	std::shared_ptr<const XeenScenePresentation> _drawn;
	bool _reconstruct=false;
	struct Entry {XeenMonsterRecord data;XeenMonsterAnimation animation;XeenActorStatus status;};
	std::mt19937 _random;
	unsigned _uiPhase=0;
	std::uint64_t _wallOrigin=0;
	std::optional<std::pair<XeenSide,std::uint16_t>> _map;
	std::map<XeenMonsterIdentity,Entry> _actors;
	// Plain values: no tracked gameplay fields in this transient key.
	std::optional<std::tuple<XeenSide,std::uint16_t,std::size_t,unsigned>> _lastAttack;
};
}
#endif
