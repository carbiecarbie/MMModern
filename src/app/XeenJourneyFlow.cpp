#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenInventoryView.h"
#include "games/xeen/XeenJourneyProgression.h"
#include "games/xeen/XeenCombatRules.h"
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace mmodern {
namespace {
struct BusyJourney { bool &value; explicit BusyJourney(bool &v) : value(v) { value = true; } ~BusyJourney() { value = false; } };
}
XeenEncounterFlow::XeenEncounterFlow(XeenWorld &w, XeenPartyState &p, XeenCamera &c, const XeenGameFlags &flags,
		const XeenEventPresenter::Clock &clock, const XeenJourneySetup &setup) :
	_journey(true), _journeyStatistics(setup.statistics), _learnedNamesProvider(setup.learnedNamesProvider), _world(w), _party(p), _camera(c), _flags(flags),
	_clock(clock), _events(_journeyEvents), _boundary(w,p,c) {
	// Detach preparation arguments before compatibility/provider callbacks.
	const auto characters=setup.characters;
	const auto context=setup.context;
	const auto seed=setup.seed;
	const auto contract=setup.contract;
	const auto manifest=setup.regionalManifest;
	const auto purse=setup.purse;
	const auto recovery=setup.regionalRecovery;
	const auto regionalText=setup.regionalText;
	const auto learnedNames=setup.learnedNames;
	_journeyEvents = setup.events;
	_journeyCapture.reset(new XeenJourneyCapture(w,p,c,_state,_boundary,_busy,_journeyPreimage));
	if (w.hasEncounterState() || p.roster.combatMarked() || p.encounterContext || w._combatCheck || w._combatAuthorized)
		throw std::invalid_argument("Journey requires fresh uncoordinated owners");
	if (bool(recovery) != xeenJourneyContent(contract).connectedRecovery() || p.regionalRecovery)
		throw std::invalid_argument("Journey recovery preparation mismatch");
	p.regionalRecovery = recovery;
	try {
		// Retain lifetime controls before installing callbacks, including allocation failure paths.
		retainJourney();
		if (contract==6 || contract==7) {
			if (!regionalText) throw std::invalid_argument("Regional text preflight is missing");
			_journeyPreimage->admitRegionalText(*regionalText);
		}
		if (contract==7) {
			if (!learnedNames) throw std::invalid_argument("Learned spell names preflight is missing");
			_journeyPreimage->admitLearnedSpellNames(*learnedNames);
		}
		w._sessionState._encounterMarked = true;
		w._sessionState._journeyOwner = this;
		w._sessionState._journeyActivity = XeenJourneyActivity::Attachment;
		w._combatCheck = [this] { _journeyPreimage->check(); };
		retainJourney();
		{
			XeenRestoreGuard::Providers providers(*_journeyPreimage,w);
			if (contract>=3) {
				if (!manifest) throw std::invalid_argument("Missing regional compatibility manifest");
				manifest(w.map(23),w.objectFile(23),_events,_journeyStatistics);
				_journeyPreimage->check();
			}
			_result = xeenJourneyContent(contract).consequences() ? XeenActorApproach::initializeJourney(w,p,c,_state,characters,context,
				_journeyStatistics,_events,seed,contract,purse) : XeenActorApproach::initializeJourney(w,p,c,_state,characters,context,_journeyStatistics,_events,seed,contract);
		}
		w._combatCheck = {};
		_journeyCapture->admittedActors = w.sessionState().actors();
		auto &s = w._sessionState;
		s._journeyOwner = this; s._combatApproachState = &_state;
		s._journeyActivity = XeenJourneyActivity::Presentation;
		++s._journeyGeneration; ++_generation;
		retainJourney();
		w._journeyCapture = _journeyCapture;
	} catch (...) { closeJourney(); throw; }
}
XeenEncounterFlow::~XeenEncounterFlow() { if (_journey) closeJourney(); }
XeenEncounterFlow::XeenEncounterFlow(XeenWorld &w, XeenPartyState &p, XeenCamera &c, const XeenGameFlags &f,
		const XeenEventPresenter::Clock &clock, XeenJourneyRestoreTag) :
	_journey(true), _world(w), _party(p), _camera(c), _flags(f), _clock(clock), _events(_journeyEvents), _boundary(w,p,c) {
	const auto binding = w._journeyRestoration;
	if (!binding || !binding->guard || !binding->guard->current() ||
		&binding->guard->w != &w || &binding->guard->p != &p || &binding->guard->c != &c || &binding->guard->f != &f ||
		!w.sessionState().journey() ||
		w._sessionState._journeyActivity != XeenJourneyActivity::Unbound || w._sessionState._journeyOwner)
		throw std::logic_error("Journey restoration binding is unavailable or stale");
	// SaveState prepared all storage before publication. This handoff only binds
	// fresh final-owner coordination; it performs no allocation or resource work.
	binding->guard->check();
	_journeyCapture.swap(binding->capture);
	_journeyCapture->bind(w,p,c,_state,_boundary,_busy,_journeyPreimage);
	_journeyStatistics.swap(binding->statistics); std::swap(_journeyEvents,binding->events);
	_learnedNamesProvider.swap(binding->learnedNamesProvider);
	_journeyPreimage.swap(binding->guard);
	_state._world = &w; _state._party = &p; _state._camera = &c; _state._revision = 1;
	w._sessionState._journeyOwner = this; w._sessionState._combatApproachState = &_state;
	w._sessionState._journeyActivity = XeenJourneyActivity::Presentation;
	++w._sessionState._journeyGeneration; ++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	w._journeyCapture = _journeyCapture; w._journeyRestoration.reset();
}
void XeenEncounterFlow::retainJourney() {
	auto next = std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
	if (_journeyPreimage) next->retainResources(*_journeyPreimage);
	_journeyPreimage.swap(next);
	if (_journeyCapture) _journeyCapture->generation = _boundary.generation();
}
void XeenEncounterFlow::adoptJourneyFlowBorrow() {
	// EventFlow checked this guard immediately before acquiring its one borrow.
	// Admit exactly that revision increment, retaining every value and identity.
	_journeyPreimage->adoptJourneyBorrowRelease();
	_journeyPreimage->check();
}
bool XeenEncounterFlow::journeyCapacity() noexcept {
	if (_generation < std::numeric_limits<std::uint64_t>::max()-2 &&
		_world._sessionState._journeyGeneration < std::numeric_limits<std::uint64_t>::max()-2) return true;
	closeJourney(); return false;
}
void XeenEncounterFlow::closeJourney() noexcept {
	_failure = true;
	if (_journeyCapture) _journeyCapture->closed = true;
	if (!_journeyPreimage || !_journeyPreimage->worldAlive()) return;
	auto &s = _world._sessionState;
	if (s._journeyOwner && s._journeyOwner != this) return;
	s._journeyActivity = XeenJourneyActivity::Failed;
	_world._combatCheck = {}; _world._combatAuthorized = {};
	// Keep the failed binding: destruction is never a new quiet boundary.
}
bool XeenEncounterFlow::journeyQuiet() const noexcept {
	return _journey && !_castingSettlement && !_regionalAutomatic && !_shootIntent && !projectilesPending() && !_busy && !_combat && !_failure && _boundary.quiet() && current(ticket()) &&
		_world.journeyCaptureEligible(_party,_camera);
}
bool XeenEncounterFlow::journeyMutable() const noexcept {
	return _journey && !_castingSettlement && !_regionalAutomatic && !_shootIntent && !projectilesPending() && !_busy && !_combat && !_failure && current(ticket()) &&
		_state.pending() == 0 && _state.phase() == XeenEncounterPhase::Exploring &&
		_world.sessionState().journeyActivity() == XeenJourneyActivity::Quiet;
}
bool XeenEncounterFlow::itemUseReady() const noexcept { return _itemUse && _itemUse->ready; }
std::optional<std::uint64_t> XeenEncounterFlow::beginItemUse(const Ticket &entry,const ItemUseSelection &selection,
		std::uint64_t inventoryLease,std::uint64_t certificateLease) {
	if (!journeyMutable() || !current(entry) || !_journeyPreimage || _itemUse ||
		(_world.sessionState().journeyContract()!=6 && _world.sessionState().journeyContract()!=7) ||
		!_boundary.holds(XeenCombatBoundary::Work::Inventory,inventoryLease) ||
		!_boundary.holds(XeenCombatBoundary::Work::Certificate,certificateLease) ||
		selection.category!=XeenInventoryCategory::Miscellaneous || selection.slot>=9 ||
		!XeenAntidoteUse::eligible(selection.record) ||
		_itemUseGeneration==std::numeric_limits<std::uint64_t>::max()) return {};
	const auto &ids=_party.party.activeRosterIds();
	if (selection.membershipSize!=ids.size() || ids.size()>XeenParty::kMaximumVisibleMembers ||
		!std::equal(ids.begin(),ids.end(),selection.membership.begin()) || selection.sourceIndex>=ids.size() ||
		ids[selection.sourceIndex]!=selection.sourceOwner || selection.sourceOwner>=XeenRoster::kCharacterCount) return {};
	const auto &source=_party.roster.at(selection.sourceOwner);
	if (source.rosterId!=selection.sourceOwner || !source.canAct() ||
		!xeenSameItem(source.miscellaneous[selection.slot],selection.record)) return {};
	_journeyPreimage->check();
	try {
		xeenValidateJourneyParty(_party,_world.sessionState().journeyContract());
		auto next=std::make_unique<ItemUseContinuation>();
		next->opportunity=std::make_unique<XeenRegionalActionCandidate>();
		auto &c=*next->opportunity;
		const auto &s=_world.sessionState();
		c.camera=_camera;c.context=*_party.encounterContext;c.actors=s.actors();
		c.random=XeenCombatRandom(*s.journeyRandom());c.revision=_state.revision();c.pending=0;
		c.remaining=1;c.classify=true;c.result.outcome=XeenEncounterOutcome::Pulsed;
		for(unsigned i=0;i<6;++i) { c.characters[i]=_party.roster.at(kXeenCombatOwners[i]);c.inputs[i]=*_party.roster.combatInputs(kXeenCombatOwners[i]); }
		next->generation=++_itemUseGeneration;next->epoch=selection.epoch+1;
		next->sourceOwner=selection.sourceOwner;next->slot=selection.slot;
		next->spentCharge=selection.record.state&0x3f;next->exhausted=next->spentCharge==1;
		next->lease=_boundary.hold(XeenCombatBoundary::Work::ItemUse);
		// The charge and continuation are published together before any target frame or callback.
		_party.roster.at(selection.sourceOwner).miscellaneous[selection.slot]=XeenAntidoteUse::debit(selection.record);
		_itemUse=std::move(next);_itemUseResult.reset();
		_world._sessionState._journeyActivity=XeenJourneyActivity::ItemUse;
		++_world._sessionState._journeyGeneration;++_generation;
		retainJourney();
		return _itemUseGeneration;
	} catch (...) { closeJourney(); throw; }
}
bool XeenEncounterFlow::authorizeItemUseTarget(const Ticket &entry,std::uint64_t generation,
		std::uint64_t inventoryEpoch,std::uint64_t displayedInput,
		const IndexedFrame::Presentation &selectorFrame) {
	if (_busy || !selectorFrame || !_itemUse || _itemUse->ready || !current(entry) ||
		generation!=_itemUse->generation || inventoryEpoch!=_itemUse->epoch ||
		_world.sessionState().journeyActivity()!=XeenJourneyActivity::ItemUse ||
		!_boundary.holds(XeenCombatBoundary::Work::ItemUse,_itemUse->lease)) return false;
	_itemUse->selectorTicket=entry;
	_itemUse->selectorInput=displayedInput;
	_itemUse->selectorFrame=selectorFrame;
	return true;
}
bool XeenEncounterFlow::finishItemUse(const Ticket &entry,std::uint64_t generation,std::uint64_t inventoryEpoch,
		std::optional<std::size_t> targetIndex,std::uint64_t displayedInput,
		const IndexedFrame::Presentation &selectorFrame) {
	if (_busy || !_itemUse || !_itemUse->selectorTicket || !selectorFrame ||
		!_itemUse->selectorFrame || selectorFrame!=_itemUse->selectorFrame ||
		displayedInput!=_itemUse->selectorInput || !current(*_itemUse->selectorTicket)) return false;
	return settleItemUse(entry,generation,inventoryEpoch,targetIndex);
}
bool XeenEncounterFlow::abandonItemUse(const Ticket &entry,std::uint64_t generation,
		std::uint64_t inventoryEpoch) {
	return settleItemUse(entry,generation,inventoryEpoch,{});
}
bool XeenEncounterFlow::settleItemUse(const Ticket &entry,std::uint64_t generation,std::uint64_t inventoryEpoch,
		std::optional<std::size_t> targetIndex) {
	if (_busy || !_itemUse || _itemUse->ready || !current(entry) || generation!=_itemUse->generation ||
		inventoryEpoch!=_itemUse->epoch ||
		_world.sessionState().journeyActivity()!=XeenJourneyActivity::ItemUse || !_boundary.holds(XeenCombatBoundary::Work::ItemUse,_itemUse->lease)) return false;
	const auto &ids=_party.party.activeRosterIds();
	if (targetIndex && *targetIndex>=ids.size()) return false;
	try {
		_journeyPreimage->check();
		const auto target=targetIndex ? std::optional<std::uint8_t>{ids[*targetIndex]} : std::nullopt;
		std::optional<XeenCharacter> targetCandidate;
		if (target) targetCandidate=_party.roster.at(*target);
		auto result=XeenAntidoteUse::effect(_itemUse->sourceOwner,target,targetCandidate ? &*targetCandidate : nullptr,
			_itemUse->spentCharge,_itemUse->exhausted);
		auto sourceItems=_party.roster.at(_itemUse->sourceOwner).miscellaneous;
		XeenAntidoteUse::settle(sourceItems,_itemUse->slot,_itemUse->exhausted);
		if (target) {
			_party.roster.at(*target).conditions=targetCandidate->conditions;
			for(auto &c:_itemUse->opportunity->characters) if(c.rosterId==*target) c.conditions=targetCandidate->conditions;
		}
		_party.roster.at(_itemUse->sourceOwner).miscellaneous=sourceItems;
		_itemUseResult=result;_itemUse->ready=true;
		_itemUse->selectorTicket.reset();_itemUse->selectorFrame.reset();
		++_world._sessionState._journeyGeneration;++_generation;
		retainJourney();return true;
	} catch (...) { closeJourney(); throw; }
}
bool XeenEncounterFlow::serviceItemUse() {
	if (!itemUseReady() || _busy || _combat || !_journeyPreimage || !journeyCapacity()) return false;
	const auto boundaryGeneration=_boundary.generation();
	BusyJourney busy(_busy);
	try {
		_journeyPreimage->check();
		auto &s=_world._sessionState;
		s._journeyActivity=XeenJourneyActivity::Approach;++s._journeyGeneration;
		_world._combatCheck=[this,boundaryGeneration] {
			if(_boundary.generation()!=boundaryGeneration) throw std::logic_error("Stale item opportunity boundary");
			_journeyPreimage->check();
		};
		_world._combatAuthorized=[this,boundaryGeneration,generation=_generation] {
			return !_failure && _generation==generation && _boundary.generation()==boundaryGeneration && _journeyPreimage->ownersAlive();
		};
		_journeyPreimage->adoptJourneyCoordination();
		if (!_regionalWork) _regionalWork.swap(_itemUse->opportunity);
		XeenEncounterResult result;
		{
			XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
			result=XeenActorApproach::regionalTransition(_world,_party,_camera,_state,_events,{},_regionalWork);
		}
		if (!_journeyPreimage->ownersAlive() || _boundary.generation()!=boundaryGeneration ||
			result.outcome==XeenEncounterOutcome::Stale || result.outcome==XeenEncounterOutcome::Refused ||
			result.outcome==XeenEncounterOutcome::Stopped) { closeJourney();return false; }
		_world._combatCheck={};_world._combatAuthorized={};
		if(result.outcome==XeenEncounterOutcome::Pending) {
			s._journeyActivity=XeenJourneyActivity::ItemUse;++s._journeyGeneration;
			_journeyPreimage->adoptJourneyCoordination();return true;
		}
		_result=result;++_generation;_journeyFramePrepared=false;
		if(result.consequences) observeRanged(result.consequences);
		s._journeyActivity=_state.phase()==XeenEncounterPhase::Engaged ? XeenJourneyActivity::Attachment : XeenJourneyActivity::Presentation;
		++s._journeyGeneration;
		_journeyPreimage->adoptJourneyCoordination();
		_boundary.release(XeenCombatBoundary::Work::ItemUse,_itemUse->lease);
		_itemUse.reset();
		retainJourney();return true;
	} catch (...) { closeJourney();throw; }
}
std::uint64_t XeenEncounterFlow::holdJourneyWork(XeenCombatBoundary::Work work) {
	if (!journeyMutable()) throw std::logic_error("Journey modal boundary unavailable");
	return _boundary.hold(work);
}
void XeenEncounterFlow::releaseJourneyWork(XeenCombatBoundary::Work work, std::uint64_t lease) {
	if (!_journey || _busy || _combat || !current(ticket())) throw std::logic_error("Stale Journey modal release");
	_boundary.release(work,lease);
	_journeyCapture->generation = _boundary.generation();
}
void XeenEncounterFlow::holdJourneyFrame() {
	if (!_journey || _busy || _combat || !current(ticket()) || !journeyCapacity()) throw std::logic_error("Journey frame boundary unavailable");
	auto &s = _world._sessionState;
	if (s._journeyActivity == XeenJourneyActivity::Presentation || s._journeyActivity == XeenJourneyActivity::Event || s._journeyActivity == XeenJourneyActivity::Shoot || s._journeyActivity == XeenJourneyActivity::Reward || s._journeyActivity == XeenJourneyActivity::ItemUse) return;
	if (s._journeyActivity == XeenJourneyActivity::Casting) return;
	if (s._journeyActivity != XeenJourneyActivity::Quiet && s._journeyActivity != XeenJourneyActivity::Approach && s._journeyActivity != XeenJourneyActivity::SupportStopped)
		throw std::logic_error("Journey frame cannot replace active work");
	s._journeyActivity = XeenJourneyActivity::Presentation;
	++s._journeyGeneration; ++_generation;
	_journeyFramePrepared = false;
	_journeyPreimage->adoptJourneyCoordination();
}
void XeenEncounterFlow::beginJourneyEvent() {
	const bool regional=_world.sessionState().journeyContract()>=3;
	if(_regionalAutomatic && (!_regionalAutomaticAddress || _regionalAutomaticAddress->mapId!=_camera.mapId ||
		_regionalAutomaticAddress->x!=_camera.x || _regionalAutomaticAddress->y!=_camera.y || _regionalAutomaticAddress->direction!=_camera.direction))
		throw std::logic_error("Automatic Journey event address changed");
	const bool ready=_regionalAutomatic ? !_busy && !_combat && !_failure && _boundary.quiet() && current(ticket()) &&
		_state.pending()==0 && _state.phase()==XeenEncounterPhase::Exploring : journeyQuiet();
	const auto interaction = regional ? xeenRegionalInteraction(_events,_camera,_world.sessionState().journeyContract()) : XeenRegionalInteraction::None;
	if (!ready || !journeyCapacity() || (regional ? interaction==XeenRegionalInteraction::None || (_regionalAutomatic && interaction!=XeenRegionalInteraction::Sign) :
		_world.sessionState().journeyContract()!=2 || _camera.mapId!=XeenMapIdentity(20) || _camera.x!=5 || _camera.y!=14))
		throw std::logic_error("Journey objective admission unavailable");
	if (!_regionalAutomatic) retireCastingFeedback();
	_eventLease = _boundary.hold(XeenCombatBoundary::Work::Event);
	_regionalAutomatic=false;_regionalAutomaticAddress.reset();
	_world._sessionState._journeyActivity = XeenJourneyActivity::Event;
	++_world._sessionState._journeyGeneration; ++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	_journeyFramePrepared = _journeyFrameRetry = false;
}
void XeenEncounterFlow::endJourneyEvent() {
	if (!journeyEvent() || _busy || !current(ticket()) || !journeyCapacity())
		throw std::logic_error("Journey event retirement unavailable");
	// Transfer to Presentation before releasing Event: no quiet capture gap.
	_world._sessionState._journeyActivity = XeenJourneyActivity::Presentation;
	++_world._sessionState._journeyGeneration; ++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	_boundary.release(XeenCombatBoundary::Work::Event,_eventLease);
	_eventLease=0; _journeyFramePrepared = _journeyFrameRetry = false;
}
void XeenEncounterFlow::journeyRead(const std::function<void()> &operation) {
	if (!journeyMutable() || !_boundary.quiet()) throw std::logic_error("Journey interaction unavailable");
	// Objective scripts require the exclusive Event continuation, never this read seam.
	const auto &content = xeenJourneyContent(_world.sessionState().journeyContract());
	if (content.manualObjective && _camera.mapId == XeenMapIdentity(20) && _camera.x == 5 && _camera.y == 14)
		throw std::logic_error("Journey objective requires Event authority");
	holdJourneyFrame();
	BusyJourney busy(_busy);
	try {
		XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
		operation(); _journeyPreimage->check();
	} catch (...) { if (!_journeyPreimage->current()) closeJourney(); throw; }
}
std::string XeenEncounterFlow::journeyInspection() const {
	std::ostringstream out;
	out << "Journey " << xeenInventoryInspection(_party);
	out << "Camera " << _camera.mapId << ' ' << _camera.x << ' ' << _camera.y << ' ' << unsigned(_camera.direction)
		<< " pending=" << _state.pending() << " combat=" << bool(_combat);
	if (const auto &random=_world.sessionState().journeyRandom())
		out << " contract=" << _world.sessionState().journeyContract() << " RNG=" << random->state << " draws=" << random->count;
	else out << " seed=" << _world.sessionState().skeletonSeed();
	out << '\n';
	const auto &c = *_party.encounterContext;
	out << "Context profile=" << unsigned(c.profile) << " difficulty=" << unsigned(c.difficulty)
		<< " minutes=" << c.minutes << " ctr24=" << c.ctr24 << " day=" << c.day << " year=" << c.year
		<< " rested=" << c.rested << " newDay=" << c.newDay << " effects=";
	for (auto v:c.effects) out << unsigned(v) << ',';
	out << " light/resistances="; for (auto v:c.lightAndResistances) out << v << ','; out << '\n';
	for (unsigned owner=0;owner<30;++owner) {
		const auto &v = *_party.roster.combatInputs(owner);
		out << "Supplement " << owner << " Might=" << v.might.permanent << '/' << v.might.temporary
			<< " Speed=" << v.speed.permanent << '/' << v.speed.temporary << " Accuracy=" << v.accuracy.permanent << '/' << v.accuracy.temporary
			<< " temporaryAC=" << v.temporaryAc << " XP=" << v.experience;
		if(v.luck) out << " Luck=" << v.luck->permanent << '/' << v.luck->temporary;
  if(v.resistances) {const auto &r=*v.resistances;out<<" Cold="<<unsigned(r.coldPermanent)<<'/'<<unsigned(r.coldTemporary)<<" Electrical="<<unsigned(r.electricalPermanent)<<'/'<<unsigned(r.electricalTemporary);}
  const auto &ch=_party.roster.at(owner);out<<" HP="<<ch.currentHp<<" SP="<<ch.currentSp<<" conditions=";for(auto v:ch.conditions)out<<unsigned(v)<<',';
		out << '\n';
	}
	for (const auto &a : _world.sessionState().actors())
		out << "Actor " << a.id.recordIndex << " map=" << a.id.mapId << ' ' << a.x << ' ' << a.y << " HP=" << a.hp
			<< " active=" << a.activated << " lifecycle=" << unsigned(a.lifecycle) << " status=" << unsigned(a.status)
			<< " accounted=" << _world.sessionState().accountedMonsters().count(a.id) << '\n';
 if(_party.monsterTreasure) {
  const auto &v=*_party.monsterTreasure;out<<"Purse gold="<<v.gold<<" gems="<<v.gems<<" pendingMask="<<v.pendingMask<<" pendingGold="<<v.pendingGold<<'\n';
  for(unsigned category=0;category<2;++category)for(const auto &r:category?v.armor:v.weapons)out<<"Pending "<<(category?"armor":"weapon")<<" source="<<unsigned(r.source)<<" M/ID/S/F="<<unsigned(r.item.material)<<'/'<<unsigned(r.item.id)<<'/'<<unsigned(r.item.state)<<'/'<<unsigned(r.item.frame)<<'\n';
 }
 if(_combat)out<<"Participants="<<unsigned(_combat->participants())<<" exit="<<unsigned(_combat->exitCause())<<'\n';
 const auto describe=[&](const XeenCombatResult &r) {
  if(r.operation==XeenCombatOperation::PlayerRun)out<<"Run owner="<<unsigned(*r.actingOwner)<<" roll="<<r.runRoll<<" success="<<r.runSuccess<<" mask="<<unsigned(r.participantsBefore)<<"->"<<unsigned(r.participantsAfter)<<'\n';
  if(r.operation==XeenCombatOperation::FinishDisengagement)out<<"Disengaged cause="<<unsigned(r.exitCause)<<" casualties="<<unsigned(r.casualties)<<" forfeitedGold="<<r.forfeitedGold<<'\n';
  if(r.monsterDrop)out<<"Drop source="<<unsigned(r.generatedItem.source)<<" outcome="<<unsigned(*r.monsterDrop)<<" armor="<<r.generatedArmor<<" M/ID/S/F="<<unsigned(r.generatedItem.item.material)<<'/'<<unsigned(r.generatedItem.item.id)<<'/'<<unsigned(r.generatedItem.item.state)<<'/'<<unsigned(r.generatedItem.item.frame)<<'\n';
  for(unsigned i=0;i<r.injuryCount;++i){const auto &v=r.injuries[i];out<<"Injury owner="<<unsigned(v.owner)<<" amount="<<v.amount<<" HP="<<v.beforeHp<<"->"<<v.afterHp<<" AC="<<v.beforeAc<<"->"<<v.afterAc<<" conditions=";for(auto c:v.conditions)out<<unsigned(c)<<',';out<<'\n';}
  for(unsigned i=0;i<r.armorCount;++i){const auto &v=r.armor[i];out<<"Broken armor owner="<<unsigned(v.owner)<<" slot="<<unsigned(v.slot)<<" state="<<unsigned(v.before.state)<<"->"<<unsigned(v.after.state)<<'\n';}
 };
 describe(_combatObservation);
 if(_rangedObservation)for(unsigned i=0;i<_rangedObservation->count;++i){const auto &v=_rangedObservation->shots[i];out<<"Ranged actor="<<v.source.recordIndex<<" at="<<v.x<<','<<v.y<<" direction="<<unsigned(v.direction)<<" distance="<<v.distance<<'\n';describe(v.attack);}
	out << "Game flags="; for (auto v:_flags.values()) out << (v?'1':'0');
	out << "\nQuest flags="; for (auto v:_party.questFlags.values()) out << (v?'1':'0');
	out << "\nQuest counts="; for (auto v:_party.questItems.counts()) out << v << ',';
	out << "\nDisabled objects="; for (auto id:_world.sessionState().disabledObjects()) out << id.mapId << ':' << id.recordIndex << ',';
	out << "\nDisabled events="; for (auto id:_world.sessionState().disabledEvents()) out << id.mapId << ':' << id.recordIndex << ',';
	out << '\n';
	return out.str();
}
XeenEncounterFlow::Ticket XeenEncounterFlow::beginJourneySave() {
	if (!journeyQuiet() || !journeyCapacity()) throw std::logic_error("Journey save boundary unavailable");
	_world._sessionState._journeyActivity = XeenJourneyActivity::Saving;
	++_world._sessionState._journeyGeneration; ++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	return ticket();
}
bool XeenEncounterFlow::journeySaveCurrent(const Ticket &t) const noexcept {
	return _journey && !_busy && !_combat && !_failure && current(t) && _boundary.quiet() &&
		_world.sessionState().journeyActivity() == XeenJourneyActivity::Saving;
}
bool XeenEncounterFlow::endJourneySave(const Ticket &t) noexcept {
	if (!journeySaveCurrent(t)) return false;
	_world._sessionState._journeyActivity = XeenJourneyActivity::Quiet;
	++_world._sessionState._journeyGeneration; ++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	_journeyCapture->generation = _boundary.generation();
	return true;
}
bool XeenEncounterFlow::presentJourney(const Ticket &entry) {
	if (!_journey || _busy || _combat || !_journeyFramePrepared || !current(entry) ||
		(_world.sessionState().journeyActivity() != XeenJourneyActivity::Presentation && !journeyEvent() && !_shoot && !monsterReward() && !_itemUse && !_casting)) return false;
	if (!journeyCapacity()) return false;
	auto &s = _world._sessionState;
	if (!journeyEvent() && !_shoot && !monsterReward() && !_itemUse && !_casting) s._journeyActivity = _state.phase()==XeenEncounterPhase::SupportStopped ? XeenJourneyActivity::SupportStopped : (_state.pending() || _regionalWork || projectilesPending() || _shootIntent || _regionalAutomatic) ? XeenJourneyActivity::Approach : XeenJourneyActivity::Quiet;
	if (_castingSettlement && (s._journeyActivity==XeenJourneyActivity::Quiet ||
		s._journeyActivity==XeenJourneyActivity::SupportStopped || journeyEvent() || monsterReward()))
		_castingSettlement=false;
	++s._journeyGeneration; ++_generation;
	_journeyFramePrepared = _journeyFrameRetry = false;
	_journeyPreimage->adoptJourneyCoordination();
	_journeyCapture->generation = _boundary.generation();
	return true;
}
bool XeenEncounterFlow::prepareJourneyFrame(const Ticket &entry, const std::function<void()> &compose) {
	if (!_journey || _busy || _combat || !current(entry) || !compose ||
		(_world.sessionState().journeyActivity() != XeenJourneyActivity::Presentation && !journeyEvent() && !_shoot && !monsterReward() && !_itemUse && !_casting)) return false;
	BusyJourney busy(_busy);
	_journeyFramePrepared = false;
	if (_itemUse) { _itemUse->selectorTicket.reset(); _itemUse->selectorFrame.reset(); }
	try {
		XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
		compose(); _journeyPreimage->check();
		if (!current(entry)) throw std::logic_error("Stale Journey frame preparation");
		_journeyFramePrepared = true; return true;
	} catch (...) {
		if (!_journeyPreimage->current() || _journeyFrameRetry) closeJourney();
		else _journeyFrameRetry = true;
		return false;
	}
}
XeenEncounterResult XeenEncounterFlow::journeyAction(const Ticket &entry, XeenEncounterAction action) {
	return advanceJourney(entry,action);
}
XeenEncounterResult XeenEncounterFlow::journeyPulse(const Ticket &entry) { return advanceJourney(entry,{}); }
XeenEncounterResult XeenEncounterFlow::advanceJourney(const Ticket &entry, std::optional<XeenEncounterAction> action) {
	XeenEncounterResult refused;
	if (action && (_castingSettlement || _regionalAutomatic || _regionalWork)) return refused;
	if (!_journey || _busy || _combat || !current(entry) || !_boundary.quiet() ||
		_state.phase() != XeenEncounterPhase::Exploring) return refused;
	auto &s = _world._sessionState;
	if (s._journeyActivity != XeenJourneyActivity::Quiet && s._journeyActivity != XeenJourneyActivity::Approach &&
		!(s._journeyActivity == XeenJourneyActivity::Presentation && !action && !_journeyFramePrepared && !_journeyFrameRetry)) return refused;
	if (!journeyCapacity()) return refused;
	try {
		xeenValidateJourneyParty(_party,_world.sessionState().journeyContract());
		if (s.journeyContract()!=3 && std::any_of(s._actors.begin(),s._actors.end(),[&](const XeenActor &a) { return xeenJourneyContent(s.journeyContract()).influences(a.id.recordIndex) && a.lifecycle == XeenActorLifecycle::Present; })) xeenValidateJourneyMelee(_party,_world.sessionState().journeyContract());
	} catch (const std::invalid_argument &e) { _journeyRefusal = e.what(); refused.reason = XeenEncounterStop::Domain; return refused; }
	_journeyRefusal.clear();
	const auto boundaryGeneration = _boundary.generation();
	BusyJourney busy(_busy);
	try {
		s._journeyActivity = XeenJourneyActivity::Approach; ++s._journeyGeneration;
		_world._combatCheck = [this, boundaryGeneration] {
			if (_boundary.generation() != boundaryGeneration) throw std::logic_error("stale Journey approach boundary");
			_journeyPreimage->check();
		};
		_world._combatAuthorized = [this, generation = _generation, boundaryGeneration] {
			return !_failure && _generation == generation && _boundary.generation() == boundaryGeneration && _journeyPreimage->ownersAlive();
		};
		_journeyPreimage->adoptJourneyCoordination();
		XeenEncounterResult result;
		{
			XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
			result = xeenJourneyContent(s.journeyContract()).consequences() ? XeenActorApproach::regionalTransition(_world,_party,_camera,_state,_events,action,_regionalWork) : action ? XeenActorApproach::action(_world,_party,_camera,_state,*action,_events) :
				XeenActorApproach::pulse(_world,_party,_camera,_state,_events);
		}
		if (!_journeyPreimage->ownersAlive()) { closeJourney(); return refused; }
		// A replaced boundary belongs to newer work. Leave its coordination alone.
		if (_boundary.generation() != boundaryGeneration) return refused;
		_world._combatCheck = {}; _world._combatAuthorized = {};
		if (result.outcome == XeenEncounterOutcome::Stale) { closeJourney(); return refused; }
		if (_state.phase() == XeenEncounterPhase::SupportStopped) {
			_regionalAutomatic=false;_regionalAutomaticAddress.reset();
			if (s.journeyContract()<3) { closeJourney(); return result; }
			_result=result;++_generation;_journeyFramePrepared=false;s._journeyActivity=XeenJourneyActivity::Presentation;
			retainJourney();return result;
		}
		if (result.outcome == XeenEncounterOutcome::Refused) {
			if (!journeyEvent() && !_shoot && !monsterReward()) s._journeyActivity = _state.phase()==XeenEncounterPhase::SupportStopped ? XeenJourneyActivity::SupportStopped : (_state.pending() || _regionalWork || projectilesPending() || _shootIntent || _regionalAutomatic) ? XeenJourneyActivity::Approach : XeenJourneyActivity::Quiet;
			_journeyPreimage->adoptJourneyCoordination(); _journeyPreimage->check(); return result;
		}
		_result = result; ++_generation; _journeyFramePrepared = false;
		if(action) { _itemUseResult.reset(); retireCastingFeedback(); }
		if(result.consequences) observeRanged(result.consequences);
		if(result.automaticEvent) _regionalAutomaticAddress=XeenCombatLocation{_camera.mapId,_camera.x,_camera.y,_camera.direction};
		_regionalAutomatic = _regionalAutomatic || result.automaticEvent;
		s._journeyActivity = _state.phase() == XeenEncounterPhase::Engaged ? XeenJourneyActivity::Attachment :
			(_state.pending() || _regionalWork) ? XeenJourneyActivity::Approach : XeenJourneyActivity::Presentation;
		retainJourney(); return result;
	} catch (...) {
		if (_boundary.generation() != boundaryGeneration) return refused;
		closeJourney(); throw;
	}
}
XeenEquipmentResult XeenEncounterFlow::journeyEquipment(const Ticket &entry, std::size_t active,
		XeenInventoryCategory category, std::size_t slot, XeenEquipmentOperation operation) {
	if (!journeyMutable() || !_boundary.preparationReady() || !current(entry) || !journeyCapacity()) return {};
	BusyJourney busy(_busy);
	try {
		xeenValidateJourneyParty(_party,_world.sessionState().journeyContract());
		const auto result = xeenSetEquipment(_party,active,category,slot,operation);
		if (result.status == XeenEquipmentStatus::Success) _world._sessionState._journeyActivity = XeenJourneyActivity::Presentation;
		++_generation; retainJourney(); return result;
	} catch (...) { closeJourney(); throw; }
}
XeenTransferResult XeenEncounterFlow::journeyTransfer(const Ticket &entry, std::size_t from, std::size_t to,
		XeenInventoryCategory category, std::size_t slot) {
	if (!journeyMutable() || !_boundary.preparationReady() || !current(entry) || !journeyCapacity()) return {};
	BusyJourney busy(_busy);
	try {
		xeenValidateJourneyParty(_party,_world.sessionState().journeyContract());
		const auto result = xeenTransferItem(_party,from,to,category,slot);
		if (result.status == XeenTransferStatus::Success) _world._sessionState._journeyActivity = XeenJourneyActivity::Presentation;
		++_generation; retainJourney(); return result;
	} catch (...) { closeJourney(); throw; }
}
bool XeenEncounterFlow::attachJourney(const Ticket &entry, const std::function<void()> &prepareSprites) {
	if (_world.sessionState().journeyContract()==3) return false;
	if (!_journey || _busy || _combat || !current(entry) || !_boundary.quiet() ||
		_world.sessionState().journeyActivity() != XeenJourneyActivity::Attachment) return false;
	if (!journeyCapacity()) return false;
	_shootIntent=false; // Contact consumes a pending exploration Shoot intent.
	const auto boundaryGeneration = _boundary.generation();
	const auto checkBoundary = [this, boundaryGeneration] {
		if (_boundary.generation() != boundaryGeneration) throw std::logic_error("stale Journey attachment boundary");
	};
	BusyJourney busy(_busy);
	try {
		xeenValidateJourneyMelee(_party,_world.sessionState().journeyContract());
		const bool carryFinish = _disengagementNoticeRevision && *_disengagementNoticeRevision == _state.revision();
		{
			XeenRestoreGuard::Providers providers(*_journeyPreimage,_world,checkBoundary);
			XeenActorApproach::validateEnvironment(_world,_world.sessionState().actors(),_events,_world.sessionState().journeyContract());
			checkBoundary();
			_journeyPreimage->check();
			if (prepareSprites) prepareSprites();
			checkBoundary();
			_journeyPreimage->check();
			// Sprite preparation may discard geometry/MOB caches. Rebuild under the
			// same entry authority before combat construction can publish a borrow.
			XeenActorApproach::validateEnvironment(_world,_world.sessionState().actors(),_events,_world.sessionState().journeyContract());
		}
		checkBoundary();
		_journeyPreimage->check();
		_combat.reset(new XeenCombat(_world,_party,_camera,_boundary,_flags,_state,_journeyStatistics,_events));
		++_generation;
		if (!acceptCombatResult(_combat->beginCombat(_combat->ticket()))) return false;
		// Only this immediate destination incarnation may present the retained finish.
		if (carryFinish) _disengagementNoticeCombat = _combat->ticket();
		observeCombat(); _deadline.reset(); _frame = 0; _appearanceStep = 0;
		std::uint64_t now;
		if (!prepareTime(ticket(),now)) throw std::runtime_error("Journey attachment scheduling failed");
		_lastTime = now; _cosmeticDeadline = now + 100; scheduleCombat(now);
		return true;
	} catch (...) {
		// As in approach, obsolete work cannot fail or release a newer boundary.
		if (_boundary.generation() != boundaryGeneration) return false;
		closeJourney(); throw;
	}
}
bool XeenEncounterFlow::retireJourney(const Ticket &entry) {
	if (!_journey || _busy || !_combat || !current(entry) || !entry.combat) return false;
	if (!journeyCapacity()) return false;
	BusyJourney busy(_busy);
	try {
		// Allocate the returned preimage before consuming either completion proof.
		// Gameplay values are already published; retirement changes only coordination.
		auto prepared = std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		_combat->retainResources(*prepared);
		const bool disengaged=_combat->phase()==XeenCombatPhase::Disengaged;
		if(disengaged && _regionalAutomatic && !_combat->result().originAutomaticSuperseded) throw std::logic_error("Finish did not supersede the origin event");
		if(disengaged) _combat->retireDisengagedJourney(*entry.combat,_state);
		else _combat->retireJourney(*entry.combat,_state);
		if(disengaged) { _regionalAutomatic=false;_regionalAutomaticAddress.reset();_combatObservation=_combat->result(); }
		_retiredCombatResult = _combat->result(); _combat.reset(); ++_generation;
		_disengagementNoticeRevision = disengaged ? std::optional<std::uint64_t>{_state.revision()} : std::nullopt;
		_disengagementNoticeCombat.reset();
		// The origin observations have completed presentation before retirement.
		if(disengaged) { _journeyRefusal.clear();_rangedObservation.reset(); }
		_deadline.reset(); _frame = 0; _appearanceStep = 0; _appearanceIdentity.reset();
		_scheduleAfterFrame = _appearanceAfterFrame = false;
		prepared->adoptJourneyCoordination(); prepared->adoptJourneyBorrowRelease(); _journeyPreimage.swap(prepared);
		return true;
	} catch (const std::invalid_argument &) { return false; }
}
}
