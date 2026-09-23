#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenJourneyProgression.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace mmodern {
namespace {
struct BusyCast {
	bool &flag;
	explicit BusyCast(bool &value) : flag(value) { flag=true; }
	~BusyCast() { flag=false; }
};
bool unsupportedCharge(const XeenGameplayContext &context) {
	const auto t=xeenPrepareTime(context,10);
	return t.dusks || t.dawns || t.midnights || t.yearRollovers || t.dailyProcessing;
}
XeenConsequenceCharacters activeCharacters(const XeenPartyState &party) {
	XeenConsequenceCharacters characters;
	for (unsigned i=0;i<6;++i) characters[i]=party.roster.at(kXeenCombatOwners[i]);
	return characters;
}
XeenConsequenceInputs activeInputs(const XeenPartyState &party) {
	XeenConsequenceInputs inputs;
	for (unsigned i=0;i<6;++i) inputs[i]=*party.roster.combatInputs(kXeenCombatOwners[i]);
	return inputs;
}
}

bool XeenEncounterFlow::beginCasting(const Ticket &entry) {
	if (!current(entry) || !journeyMutable() || _world.sessionState().journeyContract()!=7 ||
			_casting || !_boundary.quiet() || _castingGeneration==std::numeric_limits<std::uint64_t>::max() ||
			!journeyCapacity()) return false;
	BusyCast busy(_busy);
	try {
		_journeyPreimage->check();
		xeenValidateJourneyParty(_party,7);
		auto next=std::make_unique<CastingContinuation>();
		next->generation=++_castingGeneration;
		next->lease=_boundary.hold(XeenCombatBoundary::Work::Casting);
		_casting=std::move(next);
		_castingResult.clear();
		_world._sessionState._journeyActivity=XeenJourneyActivity::Casting;
		++_world._sessionState._journeyGeneration;++_generation;
		_journeyPreimage->adoptJourneyCoordination();
		return true;
	} catch (...) { closeJourney();throw; }
}

void XeenEncounterFlow::authorizeCastingFrame(const Ticket &entry, std::uint64_t input,
		const IndexedFrame::Presentation &frame) {
	if (!_casting || !frame || !input || !current(entry) ||
		_world.sessionState().journeyActivity()!=XeenJourneyActivity::Casting ||
		!_boundary.holds(XeenCombatBoundary::Work::Casting,_casting->lease))
		throw std::logic_error("Casting frame authority unavailable");
	_casting->displayedInput=input;
	_casting->displayedFrame=frame;
}

bool XeenEncounterFlow::castingFrameCurrent(std::uint64_t input,
		const IndexedFrame::Presentation &frame) const noexcept {
	return _casting && !_failure && !_busy && frame && _casting->displayedFrame==frame &&
		_casting->displayedInput==input && current(ticket()) &&
		_world.sessionState().journeyActivity()==XeenJourneyActivity::Casting &&
		_boundary.holds(XeenCombatBoundary::Work::Casting,_casting->lease);
}

bool XeenEncounterFlow::cancelCasting(const Ticket &entry, std::uint64_t input,
		const IndexedFrame::Presentation &frame) {
	if (!current(entry) || !castingFrameCurrent(input,frame) || _casting->committed || !journeyCapacity()) return false;
	BusyCast busy(_busy);
	const auto lease=_casting->lease;
	_casting.reset();
	_world._sessionState._journeyActivity=XeenJourneyActivity::Presentation;
	++_world._sessionState._journeyGeneration;++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	_boundary.release(XeenCombatBoundary::Work::Casting,lease);
	_journeyCapture->generation=_boundary.generation();
	return true;
}

bool XeenEncounterFlow::confirmCasting(const Ticket &entry, std::size_t casterIndex,
		std::size_t slot, std::uint64_t input, const IndexedFrame::Presentation &frame) {
	if (!current(entry) || !castingFrameCurrent(input,frame) || _casting->committed ||
		casterIndex>=_party.party.size() || slot>=39 || !journeyCapacity()) return false;
	BusyCast busy(_busy);
	try {
		_journeyPreimage->check();
		xeenValidateJourneyParty(_party,7);
		if (!XeenLearnedSpellRules::eligible(_party,casterIndex,slot) ||
			unsupportedCharge(*_party.encounterContext)) return false;
		const auto owner=_party.party.activeRosterIds()[casterIndex];
		const auto &caster=_party.roster.at(owner);
		const auto category=XeenLearnedSpellRules::categoryForClass(caster.characterClass);
		const auto id=category ? XeenLearnedSpellRules::spellForSlot(*category,slot) : std::nullopt;
		const auto spell=id ? XeenLearnedSpellRules::supported(*id) : std::nullopt;
		if (!spell) return false;
		const auto after=static_cast<std::int16_t>(caster.currentSp-1);
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		prepared->characters[owner].currentSp=after;
		_casting->casterOwner=owner;_casting->slot=static_cast<std::uint8_t>(slot);
		_casting->spell=*spell;_casting->originalSp=caster.currentSp;
		_casting->random=XeenCombatRandom(*_world.sessionState().journeyRandom());
		_journeyPreimage->check();
		_party.roster.at(owner).currentSp=after;
		_casting->committed=true;
		_casting->displayedFrame.reset();_casting->displayedInput=0;
		++_world._sessionState._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
		return true;
	} catch (...) { closeJourney();throw; }
}

bool XeenEncounterFlow::respondCastingTarget(const Ticket &entry,
		std::optional<std::size_t> targetIndex, std::uint64_t input,
		const IndexedFrame::Presentation &frame) {
	if (!current(entry) || !castingFrameCurrent(input,frame) || !_casting->committed ||
		_casting->effectDone || !journeyCapacity() ||
		_casting->spell!=XeenLearnedSpell::FirstAid ||
		(targetIndex && *targetIndex>=_party.party.size())) return false;
	BusyCast busy(_busy);
	try {
		_journeyPreimage->check();
		const auto preparedEffect=targetIndex ? XeenLearnedSpellRules::prepareFirstAid(
			_party,*targetIndex,_party.encounterContext->year) : XeenSpellPreparation{};
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		std::string result;
		if (!targetIndex) {
			if (_party.roster.at(_casting->casterOwner).currentSp!=_casting->originalSp-1)
				throw std::logic_error("Casting refund preimage changed");
			prepared->characters[_casting->casterOwner].currentSp=_casting->originalSp;
			result="First Aid target cancelled; SP refunded";
		} else if (preparedEffect.failed) result="First Aid: Spell failed; SP spent";
		else {
			const auto &effect=preparedEffect.effects.front();
			prepared->characters[effect.owner].currentHp=effect.hp;
			prepared->characters[effect.owner].conditions=effect.conditions;
			result="First Aid: "+_party.roster.at(_casting->casterOwner).name+" -> "+
				_party.roster.at(effect.owner).name+" HP "+
				std::to_string(_party.roster.at(effect.owner).currentHp)+"->"+std::to_string(effect.hp)+
				"; SP spent";
		}
		_journeyPreimage->check();
		if (!targetIndex) _party.roster.at(_casting->casterOwner).currentSp=_casting->originalSp;
		else for (const auto &effect:preparedEffect.effects) {
			auto &live=_party.roster.at(effect.owner);
			live.currentHp=effect.hp;live.conditions=effect.conditions;
		}
		_castingResult.swap(result);
		_casting->effectDone=true;
		_casting->displayedFrame.reset();_casting->displayedInput=0;
		++_world._sessionState._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
		return true;
	} catch (...) { closeJourney();throw; }
}

bool XeenEncounterFlow::publishAwaken(const Ticket &entry) {
	if (!current(entry) || !_casting || !_casting->committed || _casting->effectDone ||
		_casting->spell!=XeenLearnedSpell::Awaken || !journeyCapacity()) return false;
	BusyCast busy(_busy);
	try {
		_journeyPreimage->check();
		const auto effect=XeenLearnedSpellRules::prepareAwaken(_party);
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		std::ostringstream message;
		message<<"Awaken: "<<_party.roster.at(_casting->casterOwner).name<<"; SP spent; ";
		unsigned cleared=0;
		for (const auto &value:effect.effects) {
			const auto &before=_party.roster.at(value.owner);
			cleared+=(before.conditions[8]!=0 && value.conditions[8]==0);
			prepared->characters[value.owner].conditions=value.conditions;
		}
		message<<cleared<<" Sleep cleared";
		auto result=message.str();
		_journeyPreimage->check();
		for (const auto &value:effect.effects) _party.roster.at(value.owner).conditions=value.conditions;
		_castingResult.swap(result);
		_casting->effectDone=true;
		_casting->displayedFrame.reset();_casting->displayedInput=0;
		++_world._sessionState._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
		return true;
	} catch (...) { closeJourney();throw; }
}

bool XeenEncounterFlow::serviceCasting() {
	if (!_casting || !_casting->committed || !_casting->effectDone || _busy || _failure ||
		!current(ticket()) || !journeyCapacity()) return false;
	BusyCast busy(_busy);
	try {
		_journeyPreimage->check();
		auto &work=*_casting;
		if (!work.time) work.time.emplace(*_party.encounterContext,10,activeCharacters(_party),activeInputs(_party));
		XeenConsequenceDraw draw{work.random,64,[&] { _journeyPreimage->check(); }};
		if (!work.time->service(draw)) return true;
		if (_world._sessionState._encounterRevision==std::numeric_limits<std::uint64_t>::max())
			throw std::overflow_error("Casting charge generation exhausted");
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		bool living=false;
		for (const auto &value:work.time->characters) {
			auto &owner=prepared->characters[value.rosterId];
			owner.currentHp=value.currentHp;owner.conditions=value.conditions;
			living=living || xeenCombatTargetable(owner);
		}
		prepared->context=work.time->context;
		const auto continuation=work.random.continuation();
		prepared->s._journeyRandom=continuation;
		_journeyPreimage->check();
		for (const auto &value:work.time->characters) {
			auto &owner=_party.roster.at(value.rosterId);
			owner.currentHp=value.currentHp;owner.conditions=value.conditions;
		}
		_party.encounterContext=work.time->context;
		auto &session=_world._sessionState;
		session._journeyRandom=continuation;
		_state._pending=living?3:0;
		if (!living) { _state._phase=XeenEncounterPhase::SupportStopped;
			_state._reason=XeenEncounterStop::Defeat;session._encounterTerminal=true; }
		++session._encounterRevision;_state._revision=session._encounterRevision;
		session._journeyActivity=living?XeenJourneyActivity::Approach:XeenJourneyActivity::SupportStopped;
		++session._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
		_castingSettlement=true;
		const auto lease=work.lease;_casting.reset();
		_boundary.release(XeenCombatBoundary::Work::Casting,lease);
		_journeyCapture->generation=_boundary.generation();
		return true;
	} catch (...) { closeJourney();throw; }
}

} // namespace mmodern
