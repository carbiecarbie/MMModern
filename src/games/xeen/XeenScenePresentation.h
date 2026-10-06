#ifndef MMODERN_XEEN_SCENE_PRESENTATION_H
#define MMODERN_XEEN_SCENE_PRESENTATION_H

#include "games/xeen/XeenActor.h"
#include "core/NavigationAction.h"
#include "formats/xeen/XeenMonsterAppearance.h"
#include <map>
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
	void advance(const XeenMonsterRecord &data) {
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
 struct PortraitEffect {unsigned spellFrame=4, damageFrame=0, damageTicks=0;std::uint64_t spellDeadline=0,damageDeadline=0;};
 struct Splat {unsigned frame=0, duration=0;int damage=0;bool alternatePosition=false;std::uint64_t deadline=0;};
 std::array<PortraitEffect,30> portraits{};
 std::array<Splat,3> splats{};
 bool feedbackActive() const noexcept {
  for(const auto &p:portraits)if(p.spellFrame<4 || p.damageTicks)return true;
  for(const auto &s:splats)if(s.duration)return true;
  return false;
 }
 void spellEffect(unsigned owner,std::uint64_t now=0,bool reset=false) noexcept {
  // Character::addHitPoints clears _charFX for each invocation, including
  // zero-HP Cure Poison; giveTake's duplicate recipients remain suppressed.
  if(owner<portraits.size() && (reset || portraits[owner].spellFrame==4)) {portraits[owner].spellFrame=0;portraits[owner].spellDeadline=now+100;}
 }
 void portraitDamage(unsigned owner,unsigned frame,std::uint64_t now=0) noexcept {
  if(owner<portraits.size()) {portraits[owner].damageFrame=frame;portraits[owner].damageTicks=1;portraits[owner].damageDeadline=now+100;}
 }
 void hitSplat(unsigned row,int damage,unsigned frame,bool alternatePosition,std::uint64_t now=0) noexcept {
  if(row<3 && damage>0)splats[row]={frame,3,damage,alternatePosition,now+100};
 }
 bool advanceFeedback(std::uint64_t now) noexcept {
  bool changed=false;
  for(auto &p:portraits) {
   if(p.spellFrame<4 && now>=p.spellDeadline) {++p.spellFrame;p.spellDeadline=now+100;changed=true;}
   if(p.damageTicks && now>=p.damageDeadline) {--p.damageTicks;changed=true;}
  }
  for(auto &s:splats)if(s.duration && now>=s.deadline) {--s.duration;s.deadline=now+100;changed=true;}
  return changed;
 }
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
		if (turn) sky=!sky;
		if (!combat && (turn || moved)) {ground=!ground;defaultGround=!defaultGround;}
	}
	// perform's Wait flips cancel the flips in stepTime.
	void wait() {}
	void include(const std::vector<XeenActor> &actors) {
		for (const auto &a:actors) if (a.statistics && a.statistics->supportsRendering()) {
			if (_actors.count(a.id)) continue;
			XeenMonsterAnimation animation;
			// Original normal-frame initialization and the approved effect-phase
			// initialization both consume only disposable cosmetic randomness.
			animation.frame=std::uniform_int_distribution<unsigned>(0,7)(_random);
			animation.effect1=animation.effect2=a.statistics->animationEffect();
			// Approved M48 deviation: random initial phases use only this cosmetic RNG.
			if (animation.effect2) animation.effect3=std::uniform_int_distribution<unsigned>(0,7)(_random);
			_actors.emplace(a.id, Entry{*a.statistics,animation});
		}
	}
	const XeenMonsterAnimation *animation(XeenMonsterIdentity id) const {
		const auto i=_actors.find(id);return i==_actors.end()?nullptr:&i->second.animation;
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
	void advance(XeenMapIdentity map) {
		overallFrame=(overallFrame+1)%5;floatPhase=(floatPhase+1)%8;++wallPhase;
		if (!(_uiPhase=(_uiPhase+1)%4)) water=!water;
		for (auto &[id,entry]:_actors) if (id.mapId==map) entry.animation.advance(entry.data);
	}
	static int floatX(unsigned phase) {static constexpr int values[]{-2,-1,0,1,2,1,0,-1};return values[phase%8];}
	static int floatY(unsigned phase) {static constexpr int values[]{-2,0,2,0,-1,0,2,0};return values[phase%8];}
private:
	struct Entry {XeenMonsterRecord data;XeenMonsterAnimation animation;};
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
