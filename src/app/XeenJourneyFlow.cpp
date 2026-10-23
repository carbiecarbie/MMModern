#include "app/XeenEncounterFlow.h"
#include "app/XeenAudio.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenInventoryState.h"
#include "games/xeen/XeenJourneyProgression.h"
#include "games/xeen/XeenTraining.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenJumpRules.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenFmEffects.h"
#include <limits>
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace mmodern {
void XeenEncounterFlow::fmEffect(unsigned id) noexcept {
    try {if(_program.audio){_program.audio->observeOptions(_party.soundOptions);_program.audio->effect(id,_lastNanoseconds);}}
    catch(...) {} // An FM output failure cannot abort a gameplay action.
}
void XeenEncounterFlow::fmWeaponEffect() {
    if(_program.audio){_program.audio->observeOptions(_party.soundOptions);_program.audio->weaponEffect(_program.attackWeaponFx,_lastNanoseconds);}
}
void XeenEncounterFlow::digitalSample(const std::string &name,XeenSceneArchive archive,std::optional<std::uint64_t> now) {
    if(_program.audio){_program.audio->observeOptions(_party.soundOptions);_program.audio->startDigital(name,archive,now.value_or(_lastNanoseconds));}
}
void XeenEncounterFlow::digitalStop() {
    if(_program.audio){_program.audio->observeOptions(_party.soundOptions);_program.audio->stopDigital(_lastNanoseconds);}
}
void XeenEncounterFlow::digitalMiss() {
    if(_program.audio){_program.audio->observeOptions(_party.soundOptions);_program.audio->missEffect(_lastNanoseconds);}
}
void XeenEncounterFlow::eventVoice(unsigned index) {
    if(_program.audio){_program.audio->observeOptions(_party.soundOptions);_program.audio->eventVoice(index,_lastNanoseconds);}
}
namespace {
struct BusyJourney { bool &value; explicit BusyJourney(bool &v) : value(v) { value = true; } ~BusyJourney() { value = false; } };
bool sameEvents(const XeenEventFile &a,const XeenEventFile &b) {
	if(a.mapId!=b.mapId || a.resourceName!=b.resourceName || a.resourcePresent!=b.resourcePresent || a.records.size()!=b.records.size())return false;
	for(unsigned n=0;n<a.records.size();++n) {
		const auto &x=a.records[n],&y=b.records[n];
		if(x.fileOffset!=y.fileOffset || x.lengthField!=y.lengthField || x.x!=y.x || x.y!=y.y ||
			x.direction!=y.direction || x.line!=y.line || x.opcode!=y.opcode || x.parameters!=y.parameters)return false;
	}
	return true;
}
}
XeenProgramState &xeenBorrowProgram(XeenProgramState &program, XeenWorld &world) {
	world.borrowTreasureStaging(program.treasure);
	return program;
}
XeenEncounterFlow::XeenEncounterFlow(XeenProgramState &program, XeenWorld &w, XeenPartyState &p, XeenCamera &c, const XeenGameFlags &flags,
		const XeenEventPresenter::Clock &clock, const XeenJourneySetup &setup, const PresentationClock::Source *highResolution) :
	_journey(true), _journeyStatistics(setup.statistics), _learnedNamesProvider(setup.learnedNamesProvider), _program(xeenBorrowProgram(program,w)), _world(w), _party(p), _camera(c), _flags(flags),
	_clock(clock), _nanosecondClock(highResolution), _events(_journeyEvents), _boundary(w,p,c) {
	// Detach preparation arguments before compatibility/provider callbacks.
	const auto characters=setup.characters;
	const auto context=setup.context;
	const auto seed=setup.seed;
	xeenValidateTrainingSource(characters);
	const auto manifest=setup.regionalManifest;
	const auto purse=setup.purse;
	const auto recovery=setup.worldFlags;
	const auto regionalText=setup.regionalText;
	const auto learnedNames=setup.learnedNames;
	const auto bank=setup.bank;
	const auto preparedStart=setup.prepared;
	const auto mainlandProvider=setup.mainlandEventsProvider;
	const auto cityProvider=setup.cityEventsProvider;
	_travelEventsProvider=[provider=setup.eventsProvider,mainlandProvider,cityProvider,fixture=setup.eventFixture,original=setup.events](XeenMapIdentity id) {
        if(provider)return provider(id);
        if(fixture && id==fixture->mapId)return original;
        if(id==XeenMapIdentity(23) && mainlandProvider)return mainlandProvider();
        if(id==XeenMapIdentity(28) && cityProvider)return cityProvider();
        throw std::logic_error("Travel destination Events unavailable");
    };
    const auto start=setup.start;
	const auto eventFixture=setup.eventFixture;
	_journeyEvents = setup.events;
	_journeyCapture.reset(new XeenJourneyCapture(w,p,c,_state,_boundary,_busy,_journeyPreimage,&_needsRestNotice));
	_journeyCapture->initializationPending=!setup.prepared;
	if (w.hasEncounterState() || p.roster.combatMarked() || p.encounterContext || w._combatCheck || w._combatAuthorized)
		throw std::invalid_argument("Journey requires fresh uncoordinated owners");
	if (!recovery || p.worldFlags)
		throw std::invalid_argument("Journey recovery preparation mismatch");
	if (eventFixture) {
		if (!preparedStart || start || !eventFixture->mapId || eventFixture->mapId.side!=XeenSide::Clouds)
			throw std::invalid_argument("Invalid Event developer session configuration");
		w._developerEventMap=eventFixture->mapId;
	}
	if(preparedStart)p.worldFlags = recovery;
	try {
		// Retain lifetime controls before installing callbacks, including allocation failure paths.
		retainJourney();
		{
			if (!regionalText) throw std::invalid_argument("Regional text preflight is missing");
			_journeyPreimage->admitRegionalText(*regionalText);
		}
		{
			if (!learnedNames) throw std::invalid_argument("Learned spell names preflight is missing");
			_journeyPreimage->admitLearnedSpellNames(*learnedNames);
		}
		w._sessionState._encounterMarked = true;
		w._sessionState._journeyOwner = this;
		w._sessionState._journeyActivity = XeenJourneyActivity::Attachment;
		w._combatCheck = [this] { _journeyPreimage->check(); };
		retainJourney();
		std::shared_ptr<XeenRestoreGuard> publishedPreimage;
		const XeenActorApproach::StartPublicationPreparation prepareStartPublication=[&](const XeenPartyState &candidate,
			const std::vector<XeenActor> &actors,const std::optional<XeenJourneyRandomState> &random,
			const XeenMapTable &city,const std::vector<XeenMonsterRecord> &statistics) {
			_journeyPreimage->check();
			auto prepared=std::make_shared<XeenRestoreGuard>(w,p,c,flags);
			prepared->retainResources(*_journeyPreimage);
			prepared->prepareFreshJourneyPublication(candidate,actors,random,city,statistics);
			_journeyPreimage->check();
			publishedPreimage.swap(prepared);
		};
		{
			XeenRestoreGuard::Providers providers(*_journeyPreimage,w);
			const auto mainland=preparedStart && !eventFixture ? _events : (mainlandProvider ? mainlandProvider() : XeenEventFile{});
			_journeyPreimage->check();
			{
				if (!manifest) throw std::invalid_argument("Missing regional compatibility manifest");
				manifest(w.map(23),w.objectFile(23),mainland,_journeyStatistics);
				_journeyPreimage->check();
			}
			if(eventFixture) {
				_result=XeenActorApproach::initializeStart(w,p,c,_state,characters,context,_journeyStatistics,_events,mainland,
					seed,purse,bank,true,recovery,prepareStartPublication,eventFixture);
			} else if(preparedStart) {
				const auto prepare=[&](const auto &party,const auto &actors,const auto &random) {
					prepareStartPublication(party,actors,random,{},_journeyStatistics);
				};
				// The ordinary entry keeps its wrapped initialization; the fixture start is a separate overload.
				_result = start ? XeenActorApproach::initializeJourney(w,p,c,_state,characters,context,_journeyStatistics,_events,seed,purse,bank,prepare,*start)
					: XeenActorApproach::initializeJourney(w,p,c,_state,characters,context,_journeyStatistics,_events,seed,purse,bank,prepare);
			} else {
				const auto city=cityProvider ? cityProvider() : XeenEventFile{};
				_journeyPreimage->check();
				if(!city.resourcePresent || city.mapId!=XeenMapIdentity(28) ||
					!sameEvents(city,_events))throw std::invalid_argument("Original start city Event binding changed");
				_result=XeenActorApproach::initializeStart(w,p,c,_state,characters,context,
					_journeyStatistics,_events,mainland,seed,purse,bank,false,recovery,prepareStartPublication);
			}
		}
		w._combatCheck = {};
		_journeyPreimage.swap(publishedPreimage);
		auto &s = w._sessionState;
		s._journeyOwner = this; s._combatApproachState = &_state;
		s._journeyActivity = XeenJourneyActivity::Presentation;
		++s._journeyGeneration; ++_generation;
		_journeyPreimage->adoptJourneyCoordination();
		if(preparedStart) {
			XeenRestoreGuard::Providers providers(*_journeyPreimage,w);
			const auto cityEvents=setup.cityEventsProvider ? setup.cityEventsProvider() : XeenEventFile{};
			if(!cityEvents.resourcePresent || cityEvents.mapId!=XeenMapIdentity(28))
				throw std::invalid_argument("Missing city Event resource");
			_journeyPreimage->check();
		}
		_journeyCapture->generation=_boundary.generation();
		w._journeyCapture = _journeyCapture;
		recordJourneyStep();
	} catch (...) { closeJourney(); throw; }
}
XeenEncounterFlow::~XeenEncounterFlow() { if (_journey) closeJourney(); }
XeenEncounterFlow::XeenEncounterFlow(XeenProgramState &program, XeenWorld &w, XeenPartyState &p, XeenCamera &c, const XeenGameFlags &f,
		const XeenEventPresenter::Clock &clock, XeenJourneyRestoreTag, const PresentationClock::Source *highResolution) :
	_journey(true), _program(xeenBorrowProgram(program,w)), _world(w), _party(p), _camera(c), _flags(f), _clock(clock), _nanosecondClock(highResolution), _events(_journeyEvents), _boundary(w,p,c) {
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
	_journeyCapture->bind(w,p,c,_state,_boundary,_busy,_journeyPreimage,&_needsRestNotice);
	_journeyStatistics.swap(binding->statistics); std::swap(_journeyEvents,binding->events);
	_travelEventsProvider.swap(binding->eventsProvider);
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
void XeenEncounterFlow::recordJourneyStep() {
	_journeyPreimage->check();
	std::vector<XeenExploredTile> values;
	{XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
		values=_world.explorationAfterStep(_party,_camera);_journeyPreimage->check();}
	if(values==_world._sessionState.exploredTiles()) {
        _world.scenePresentation().markLightingStep();_journeyPreimage->adoptMutationBoundary();return;
    }
	auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
	prepared->retainResources(*_journeyPreimage);prepared->s.assignExploration(values);
	_journeyPreimage->check();
	XeenMutationWatch::write(&_world);_world._sessionState.publishExploration(values);
 _world.scenePresentation().markLightingStep();
	_journeyPreimage.swap(prepared);_journeyPreimage->adoptMutationBoundary();
}
void XeenEncounterFlow::publishJourneyExploration(std::vector<XeenExploredTile> &values,
		std::vector<XeenExploredTile> &retained) noexcept {
	_world._sessionState.publishExploration(values);
	_journeyPreimage->s.publishExploration(retained);_journeyPreimage->adoptMutationBoundary();
}
bool XeenEncounterFlow::configureSound(const Ticket &entry,XeenSoundOptions value) {
 if(!current(entry) || _busy)return false;
 if(_combat)return acceptCombatResult(_combat->configureSound(*entry.combat,value));
 if(!journeyMutable() || !journeyCapacity())return false;
 _journeyPreimage->check();
 auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
 prepared->retainResources(*_journeyPreimage);prepared->soundOptions=value;
 _journeyPreimage->check();_party.soundOptions=value;
 ++_generation;_journeyPreimage.swap(prepared);_journeyPreimage->adoptMutationBoundary();return true;
}
bool XeenEncounterFlow::toggleAutomap(const Ticket &entry) {
	if(!current(entry) || _busy)return false;
	if(_combat)return acceptCombatResult(_combat->toggleAutomap(*entry.combat));
	if(!journeyMutable() || !journeyCapacity())return false;
	_journeyPreimage->check();
	auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
	prepared->retainResources(*_journeyPreimage);
	prepared->context->effects[XeenPartyEffect::AutomapOn]=!prepared->context->effects[XeenPartyEffect::AutomapOn];
	_journeyPreimage->check();_party.encounterContext->effects[XeenPartyEffect::AutomapOn]=prepared->context->effects[XeenPartyEffect::AutomapOn];
	++_generation;_journeyPreimage.swap(prepared);_journeyPreimage->adoptMutationBoundary();return true;
}
// Mr Wizard: DOS 0x1f236..0x1f29b loads Vertigo, faces East, moves to its run
// position (0x1691f) and zeroes the party's gems, with no time, Event or
// monster turn. Its FX 51 stays silent until audio exists.
bool XeenEncounterFlow::mrWizard(const Ticket &entry) {
	if(!current(entry) || !journeyMutable() || !_boundary.quiet() || !journeyCapacity())return false;
	BusyJourney busy(_busy);
	try {
		_journeyPreimage->check();
		XeenCamera destination;
		{XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
			auto requested=_camera;requested.mapId=XeenMapIdentity(28);requested.direction=XeenDirection::East;
			const auto admitted=_world.prepareEventTransition(requested,true);_journeyPreimage->check();
			if(!admitted)return false;
			destination=admitted.destination;}
		// The original always reloads the map, Vertigo included.
		auto candidate=_world.transitionCandidate();
		XeenPartyState party;candidate->copyEventParty(_party,party);
		auto camera=_camera;auto flags=_flags;
		XeenActorView arrival;std::vector<XeenExploredTile> exploration;XeenEventFile events;
		{XeenRestoreGuard guard(*candidate,party,camera,flags);
			XeenRestoreGuard::EventProviders providers(*candidate,party,camera,flags,guard,
				[&]{_journeyPreimage->check();},[&]{_journeyPreimage->failed=true;});
			candidate->loadEventDestination(_camera.mapId,destination.mapId);
			events=_travelEventsProvider(destination.mapId);_journeyPreimage->check();
			if(events.mapId!=destination.mapId)throw std::logic_error("Mr Wizard Events identity changed");
			camera=destination;
			arrival=candidate->prepareTransitionArrival(camera);
			exploration=candidate->explorationAfterStep(party,camera);}
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		prepared->prepareSpellMapPublication(*candidate);prepared->cameraValue=camera;
		prepared->purse->gems=0;prepared->s.assignExploration(exploration);
		_world.checkTransitionTreasure(*candidate);_journeyPreimage->check();
        fmEffect(51);
		_world.publishTransition(*candidate);_camera=camera;_journeyEvents=std::move(events);
		_party.purse->gems=0;
		XeenMutationWatch::write(&_world);_world._sessionState.publishExploration(exploration);
		++_generation;_journeyPreimage.swap(prepared);_journeyPreimage->adoptMutationBoundary();
		publishArrival(arrival);
		return true;
	} catch (...) { closeJourney();throw; }
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
	const auto capture=_world._journeyCapture.lock();
	return _journey && !_terrainFall && !_rest && !_castingSettlement && !_regionalAutomatic && !_shootIntent && !projectilesPending() && !_busy && !_combat && !_failure && _boundary.quiet() && current(ticket()) &&
		capture && capture->current(_party,_camera);
}
bool XeenEncounterFlow::journeyMutable() const noexcept {
	return _journey && !_terrainFall && !_rest && !_castingSettlement && !_regionalAutomatic && !_shootIntent && !projectilesPending() && !_busy && !_combat && !_failure && current(ticket()) &&
		_state.pending() == 0 && _state.phase() == XeenEncounterPhase::Exploring &&
		_world.sessionState().journeyActivity() == XeenJourneyActivity::Quiet;
}
bool XeenEncounterFlow::journeyNavigationReady() const noexcept {
    // Deliberately narrower than relaxing journeyMutable: menus and saves still
    // require Quiet. A simple countdown carries no retained actor work.
    const auto activity=_world.sessionState().journeyActivity();
    return _journey && !_terrainFall && !_rest && !_casting && !_castingSettlement && !_regionalAutomatic && !_regionalWork &&
        !_shoot && !_shootIntent && !projectilesPending() && !_busy && !_combat && !_failure &&
        !_itemUse && !_smith && !_training && _boundary.quiet() && current(ticket()) &&
        _state.phase()==XeenEncounterPhase::Exploring &&
        (activity==XeenJourneyActivity::Quiet || (activity==XeenJourneyActivity::Approach && _state.pending()));
}
bool XeenEncounterFlow::itemUseReady() const noexcept { return _itemUse && _itemUse->ready; }
bool XeenEncounterFlow::itemUseNeedsTarget() const noexcept {
	return _itemUse && !_itemUse->effectDone && !_itemUse->targetPending && (_itemUse->information || !_itemUse->spell || XeenLearnedSpellRules::target(static_cast<std::uint8_t>(*_itemUse->spell))==XeenSpellTarget::Member ||
        XeenLearnedSpellRules::target(static_cast<std::uint8_t>(*_itemUse->spell))==XeenSpellTarget::Item ||
        XeenLearnedSpellRules::target(static_cast<std::uint8_t>(*_itemUse->spell))==XeenSpellTarget::Element);
}
bool XeenEncounterFlow::itemUseNeedsElement() const noexcept {
    return _itemUse && _itemUse->spell==XeenLearnedSpell::ProtectionFromElements && !_itemUse->effectDone;
}
std::optional<std::uint64_t> XeenEncounterFlow::beginItemUse(const Ticket &entry,const ItemUseSelection &selection,
		std::uint64_t inventoryLease,std::uint64_t certificateLease,unsigned distance) {
	if (!journeyMutable() || !current(entry) || !_journeyPreimage || _itemUse ||
		!_boundary.holds(XeenCombatBoundary::Work::Inventory,inventoryLease) ||
		!_boundary.holds(XeenCombatBoundary::Work::Certificate,certificateLease) ||
		selection.category!=XeenInventoryCategory::Miscellaneous || selection.slot>=9 ||
		!xeenItemUseSupported(selection.record) ||
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
		xeenValidateJourneyParty(_party);
		auto next=std::make_unique<ItemUseContinuation>();
        next->spell=xeenItemUseSpell(selection.record.id);
        XeenSpellTravel travel;
        if(next->spell==XeenLearnedSpell::DetectMonster || next->spell==XeenLearnedSpell::IdentifyMonster) {
            const auto &actors=_world.sessionState().regionalActors(_camera.mapId);
            const auto view=XeenActorApproach::classify(actors,_camera);
            std::array<std::optional<XeenMonsterIdentity>,3> slots;
            for(unsigned n=0;n<3;++n)slots[n]=view.slots[n];
            next->information=XeenLearnedSpellRules::information(*next->spell,_camera,actors,slots);
            if(next->information && next->information->failed) {next->failed=true;next->information.reset();}
        }
		{XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
			if(next->spell==XeenLearnedSpell::Jump) {
				const auto jump=xeenJumpPreflight(_world,_camera);_journeyPreimage->check();
				if(jump.admission==XeenJumpAdmission::Unsupported)return {};
				if(jump.admission==XeenJumpAdmission::Ready) {
					next->jumpDestination=jump.destination;travel.destination=jump.destination;
					travel.reload=jump.destination.mapId!=_camera.mapId;
				}
			}
            if(next->spell && (*next->spell==XeenLearnedSpell::LloydsBeacon || *next->spell==XeenLearnedSpell::TownPortal || *next->spell==XeenLearnedSpell::Etherealize || *next->spell==XeenLearnedSpell::TimeDistortion || *next->spell==XeenLearnedSpell::SuperShelter || *next->spell==XeenLearnedSpell::Teleport)) {
                travel=XeenLearnedSpellRules::travel(_world,_camera,*next->spell,distance,&source);
                _journeyPreimage->check();if(travel.unsupported)return {};
                next->jumpDestination=travel.destination;next->failed=travel.failed;next->shelterPending=travel.shelter;
            }
			if(next->spell && XeenLearnedSpellRules::target(static_cast<std::uint8_t>(*next->spell))==XeenSpellTarget::Contact) {
				next->projectile=prepareSpellProjectile(*next->spell,selection.sourceOwner,0);
				next->projectile->itemPower=selection.record.id;
                if(next->projectile->spellPreparation->ready())
                    for(unsigned lane=0;lane<(*next->spell==XeenLearnedSpell::PoisonVolley?6u:1u);++lane)
                        next->appearances.push_back({false,0,lane,3,{},xeenSpellProjectile(*next->spell,next->projectile->spellPreparation->type,selection.record.id)});
			}
			_journeyPreimage->check();}
		auto mapWork=prepareSpellMap(travel);
        next->opportunity=std::make_unique<XeenRegionalActionCandidate>();
		auto &c=*next->opportunity;
		const auto &s=_world.sessionState();
		c.owners=_party.party.activeOrder();c.camera=next->jumpDestination.value_or(_camera);c.context=*_party.encounterContext;c.actors=(mapWork.world?mapWork.world->sessionState():s).regionalActors(c.camera.mapId);
		c.random=XeenCombatRandom(*s.journeyRandom());c.revision=_state.revision();c.pending=0;
		c.remaining=1;c.classify=true;c.result.outcome=XeenEncounterOutcome::Pulsed;
		for(unsigned i=0;i<6;++i) { c.characters[i]=_party.roster.at(c.owners[i]);c.inputs[i]=*_party.roster.combatInputs(c.owners[i]); }
		{XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
			c.exploration=_world.explorationAfterStep(_party,_camera);_journeyPreimage->check();}
		next->generation=++_itemUseGeneration;next->epoch=selection.epoch+1;
		next->sourceOwner=selection.sourceOwner;next->slot=selection.slot;
		next->spentCharge=selection.record.state&0x3f;next->exhausted=next->spentCharge==1;
		next->lease=_boundary.hold(XeenCombatBoundary::Work::ItemUse);
		// The charge and continuation are published together before any target frame or callback.
		auto items=source.miscellaneous;
		--items[selection.slot].state;
		XeenAntidoteUse::settle(items,selection.slot,next->exhausted);
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
        prepared->retainResources(*_journeyPreimage);
        prepared->characters[selection.sourceOwner].miscellaneous=items;
        travel.applyBeacon(prepared->characters[selection.sourceOwner]);
        if(mapWork.world) {prepared->prepareSpellMapPublication(*mapWork.world);prepared->cameraValue=*next->jumpDestination;}
        if(mapWork.world)_world.checkTransitionTreasure(*mapWork.world);
        _journeyPreimage->check();
        if(next->spell && xeenTravelFmBeforeRelocation(*next->spell))
            if(const auto cue=xeenTravelFmEffect(*next->spell,travel.failed,bool(travel.destination),distance))fmEffect(*cue);
        if(mapWork.world) {_world.publishTransition(*mapWork.world);_camera=*next->jumpDestination;_journeyEvents=std::move(mapWork.events);}
        travel.applyBeacon(_party.roster.at(selection.sourceOwner));
        _party.roster.at(selection.sourceOwner).miscellaneous=items;
		_itemUse=std::move(next);_itemUseResult.reset();
        if(_itemUse->information)fmEffectAfterFrame(20);
		_world._sessionState._journeyActivity=XeenJourneyActivity::ItemUse;
		++_world._sessionState._journeyGeneration;++_generation;
        prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
        _journeyCapture->generation=_boundary.generation();
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
		_itemUse->effectDone ||
		inventoryEpoch!=_itemUse->epoch ||
		_world.sessionState().journeyActivity()!=XeenJourneyActivity::ItemUse || !_boundary.holds(XeenCombatBoundary::Work::ItemUse,_itemUse->lease)) return false;
	const auto &ids=_party.party.activeRosterIds();
	if (targetIndex && *targetIndex>=(itemUseNeedsElement()?4:ids.size())) return false;
	try {
		_journeyPreimage->check();
        if(_itemUse->itemSpell) {
            // Abandoning a later Items dialog never returns to the early refund.
            _itemUse->itemSpell->selected=true;return serviceSpellItem();
        }
		if(_itemUse->spell) {
			const auto spell=*_itemUse->spell;
            if(targetIndex && XeenLearnedSpellRules::target(static_cast<std::uint8_t>(spell))==XeenSpellTarget::Item) {
                _itemUse->itemSpell.emplace();_itemUse->itemSpell->member=*targetIndex;
                _itemUse->selectorFrame.reset();_itemUse->selectorTicket.reset();
                ++_world._sessionState._journeyGeneration;++_generation;retainJourney();return true;
            }
			XeenSpellPreparation effect;
            if(spell==XeenLearnedSpell::PowerCure && targetIndex) {
                _itemUse->targetPending=true;_itemUse->targetIndex=targetIndex;
                _itemUse->selectorFrame.reset();_itemUse->selectorTicket.reset();
                XeenConsequenceDraw draw{_itemUse->opportunity->random,64,[&]{_journeyPreimage->check();},[this](unsigned id){fmEffect(id);},[this](unsigned id){fmSelectWeapon(id);}};
                const auto healing=XeenLearnedSpellRules::preparePowerCure(_party,*targetIndex,_party.encounterContext->year,
                    _party.roster.at(_itemUse->sourceOwner).currentLevel(),draw,&_itemUse->fmRequested);
                if(!healing)return true;
                effect=*healing;
                _world._sessionState._journeyRandom=_itemUse->opportunity->random.continuation();
            } else effect=XeenLearnedSpellRules::prepareEffect(spell,_party,itemUseNeedsElement()?std::nullopt:targetIndex,_party.encounterContext->year,_itemUse->sourceOwner,
                spell==XeenLearnedSpell::Light && _world.sceneIsBlack(_party,_camera));
			bool automaticArrival=false;
			if(_itemUse->jumpDestination) {
				XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
				const auto &destination=*_itemUse->jumpDestination;
				const auto sample=_world.sampleCell(destination.mapId,destination.x,destination.y);
				automaticArrival=sample && (sample->cell->rawAttributes & kXeenAutomaticEventFlag);
				_journeyPreimage->check();
			}
            const bool member=XeenLearnedSpellRules::target(std::uint8_t(spell))==XeenSpellTarget::Member;
            if(member)if(const auto cue=xeenSpellResultFmEffect(spell,effect.failed,!targetIndex))fmEffect(*cue);
			for(const auto &value:effect.effects) {
				auto &live=_party.roster.at(value.owner);
				if(value.healing)
					_world.scenePresentation().spellEffect(value.owner,_lastTime,true);
				value.apply(live);
			}
			XeenLearnedSpellRules::applyPartyEffect(spell,*_party.encounterContext,_party.roster.at(_itemUse->sourceOwner).currentLevel(),
                itemUseNeedsElement() && targetIndex?std::optional<unsigned>{unsigned(*targetIndex)}:std::nullopt);
			if(effect.food)_party.food=*effect.food;
            if(!member)if(const auto cue=xeenSpellResultFmEffect(spell,effect.failed)) {
                if(spell==XeenLearnedSpell::Awaken)fmEffectAfterFrame(*cue);else fmEffect(*cue);
            }
            const auto id=static_cast<std::uint8_t>(spell);
            // M61 First Aid and the M75 DOS item prompts refund without a debit.
            // Other pre-M75 item cancellation paths retain their existing policy.
            if(!targetIndex) if(const auto refund=xeenItemCancellationRefund(id,_party.roster.at(_itemUse->sourceOwner).currentLevel())) {
                auto &user=_party.roster.at(_itemUse->sourceOwner);
                user.currentSp=static_cast<std::int16_t>(user.currentSp+refund->sp);
                _party.purse->gems+=refund->gems;
			}
			if(_itemUse->jumpDestination)_camera=*_itemUse->jumpDestination;
            if(!xeenTravelFmBeforeRelocation(spell))
                if(const auto cue=xeenTravelFmEffect(spell,_itemUse->failed,bool(_itemUse->jumpDestination)))fmEffect(*cue);
            // The used-item sheet close marks stepped (DOS 0x3a493),
            // independently of the learned-cast terrain-arrival suffix.
            _world.scenePresentation().markLightingStep();
			_itemUse->failed=_itemUse->failed || effect.failed || (spell==XeenLearnedSpell::Jump && !_itemUse->jumpDestination);
			_itemUse->automaticArrival=automaticArrival;
			_projectiles.swap(_itemUse->appearances);_projectileDeadline=0;
			if(_itemUse->projectile) {
                auto &projectile=*_itemUse->projectile;
                XeenConsequenceDraw draw{projectile.random,0,{},[this](unsigned id){fmEffect(id);}};
                projectile.spellPreparation->requestFmEffect(draw,projectile.itemPower);
            }
			_shoot=std::move(_itemUse->projectile);
		} else {
		const auto target=targetIndex ? std::optional<std::uint8_t>{ids[*targetIndex]} : std::nullopt;
		std::optional<XeenCharacter> targetCandidate;
		if (target) targetCandidate=_party.roster.at(*target);
		auto result=XeenAntidoteUse::effect(_itemUse->sourceOwner,target,targetCandidate ? &*targetCandidate : nullptr,
			_itemUse->spentCharge,_itemUse->exhausted);
		if (target) {
            const auto &c=_party.roster.at(*target);
            if(!c.conditions[13] && !c.conditions[14] && !c.conditions[15] && c.currentHp<=XeenCharacterRules::maxHp(c,{_party.encounterContext->year}))
                _world.scenePresentation().spellEffect(*target,_lastTime,true);
			_party.roster.at(*target).conditions=targetCandidate->conditions;
			for(auto &c:_itemUse->opportunity->characters) if(c.rosterId==*target) c.conditions=targetCandidate->conditions;
		}
		_itemUseResult=result;
		}
		_itemUse->information.reset();
		_itemUse->effectDone=true;_itemUse->ready=!_shoot;
		_itemUse->selectorTicket.reset();_itemUse->selectorFrame.reset();
		++_world._sessionState._journeyGeneration;++_generation;
		retainJourney();return true;
	} catch (...) { closeJourney(); throw; }
}
bool XeenEncounterFlow::serviceItemUse() {
    if(_rest || (_itemUse && _itemUse->shelterPending))return false;
    if(_itemUse && _itemUse->itemSpell && _itemUse->itemSpell->selected && !_itemUse->effectDone)return serviceSpellItem();
    if(_itemUse && !_itemUse->effectDone && _itemUse->targetPending && !_busy)
        return settleItemUse(ticket(),_itemUse->generation,_itemUse->epoch,_itemUse->targetIndex);
	if (!itemUseReady() || _busy || _combat || !_journeyPreimage || !journeyCapacity() ||
		!_boundary.only(XeenCombatBoundary::Work::ItemUse,_itemUse->lease)) return false;
	const auto boundaryGeneration=_boundary.generation();
	BusyJourney busy(_busy);
	try {
		_journeyPreimage->check();
		auto &s=_world._sessionState;
		if(!_regionalWork) {
			// Effects precede the one normal response, including projectile RNG/deaths.
			auto &c=*_itemUse->opportunity;
			c.camera=_camera;c.context=*_party.encounterContext;c.actors=s.regionalActors(_camera.mapId);
			c.random=XeenCombatRandom(*s.journeyRandom());c.revision=_state.revision();
			for(unsigned i=0;i<6;++i) {c.characters[i]=_party.roster.at(c.owners[i]);c.inputs[i]=*_party.roster.combatInputs(c.owners[i]);}
			{XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
				c.exploration=_world.explorationAfterStep(_party,_camera);_journeyPreimage->check();}
		}
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
			result=XeenActorApproach::regionalTransition(_world,_party,_camera,_state,_journeyEvents,{},_regionalWork,_travelEventsProvider,[this](unsigned id){fmEffect(id);});
		}
		if (!_journeyPreimage->ownersAlive() || _boundary.generation()!=boundaryGeneration ||
			result.outcome==XeenEncounterOutcome::Stale || result.outcome==XeenEncounterOutcome::Refused ||
			result.outcome==XeenEncounterOutcome::Stopped) { closeJourney();return false; }
		_world._combatCheck={};_world._combatAuthorized={};
		if(result.outcome==XeenEncounterOutcome::Pending) {
            // A staged response may already publish movement, injury and RNG.
            // Retain that preimage and acquire its travel/portrait before resuming.
            _result=result;++_generation;_journeyFramePrepared=false;
            if(result.consequences)observeRanged(result.consequences);
			s._journeyActivity=XeenJourneyActivity::ItemUse;++s._journeyGeneration;
			retainJourney();return true;
		}
		_result=result;++_generation;_journeyFramePrepared=false;
		if(result.needsRest)_needsRestNotice=true;
		if(result.consequences) observeRanged(result.consequences);
		s._journeyActivity=_state.phase()==XeenEncounterPhase::Engaged ? XeenJourneyActivity::Attachment : XeenJourneyActivity::Presentation;
		++s._journeyGeneration;
		_journeyPreimage->adoptJourneyCoordination();
		_boundary.release(XeenCombatBoundary::Work::ItemUse,_itemUse->lease);
		if(_itemUse->automaticArrival && _state.phase()!=XeenEncounterPhase::SupportStopped) {
			_regionalAutomatic=true;_regionalAutomaticAddress=XeenCombatLocation{_camera.mapId,_camera.x,_camera.y,_camera.direction};
		}
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
	if (s._journeyActivity == XeenJourneyActivity::Casting || s._journeyActivity == XeenJourneyActivity::Service) return;
	if (s._journeyActivity != XeenJourneyActivity::Quiet && s._journeyActivity != XeenJourneyActivity::Approach && s._journeyActivity != XeenJourneyActivity::SupportStopped)
		throw std::logic_error("Journey frame cannot replace active work");
	s._journeyActivity = XeenJourneyActivity::Presentation;
	++s._journeyGeneration; ++_generation;
	_journeyFramePrepared = false;
	_journeyPreimage->adoptJourneyCoordination();
}
void XeenEncounterFlow::beginJourneyEvent(bool barrier) {

	if(_regionalAutomatic && (!_regionalAutomaticAddress || _regionalAutomaticAddress->mapId!=_camera.mapId ||
		_regionalAutomaticAddress->x!=_camera.x || _regionalAutomaticAddress->y!=_camera.y || _regionalAutomaticAddress->direction!=_camera.direction))
		throw std::logic_error("Automatic Journey event address changed");
	const bool ready=_regionalAutomatic ? !_busy && !_combat && !_failure && _boundary.quiet() && current(ticket()) &&
		_state.pending()==0 && _state.phase()==XeenEncounterPhase::Exploring : journeyQuiet();
	if (!ready || !journeyCapacity() || (barrier && _regionalAutomatic))
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
	_world._sessionState._journeyActivity = _state.phase()==XeenEncounterPhase::Engaged ?
		XeenJourneyActivity::Attachment : XeenJourneyActivity::Presentation;
	++_world._sessionState._journeyGeneration; ++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	_boundary.release(XeenCombatBoundary::Work::Event,_eventLease);
	_trainingEventSettlement=false;
	_smithEventSettlement=false;
    if(_manualStepPendingReset){_state._pending=0;_manualStepPendingReset=false;}
	_eventLease=0; _journeyFramePrepared = _journeyFrameRetry = false;
}
void XeenEncounterFlow::publishArrival(const XeenActorView &view) noexcept {
	bool living=false;for(auto owner:_party.party.activeRosterIds())living=living || xeenCombatTargetable(_party.roster.at(owner));
	if(!living || _terrainDead) {
		_state._phase=XeenEncounterPhase::SupportStopped;_state._reason=XeenEncounterStop::Defeat;
		_world._sessionState._encounterTerminal=true;_journeyPreimage->adoptJourneyCoordination();return;
	}
	_state._phase=view.engaged()?XeenEncounterPhase::Engaged:XeenEncounterPhase::Exploring;
	_state._reason=XeenEncounterStop::None;
	_world._sessionState._encounterTerminal=view.engaged();
	_result.view=view;
	_journeyPreimage->adoptJourneyCoordination();
}
void XeenEncounterFlow::settleManualStep(bool needsRest) noexcept {
 _manualStepPendingReset=true;_needsRestNotice=needsRest;
}
void XeenEncounterFlow::beginTerrainTransition() {
	if(!_terrainFall || _busy || _combat || _failure || !_boundary.quiet() || !current(ticket()) || !journeyCapacity())
		throw std::logic_error("Terrain transition publication is unavailable");
	_state._pending=0;
	_eventLease=_boundary.hold(XeenCombatBoundary::Work::Event);
	_regionalAutomatic=false;_regionalAutomaticAddress.reset();
	_world._sessionState._journeyActivity=XeenJourneyActivity::Event;
	++_world._sessionState._journeyGeneration;++_generation;
	_journeyPreimage->adoptJourneyCoordination();_journeyFramePrepared=_journeyFrameRetry=false;
}
void XeenEncounterFlow::journeyRead(const std::function<void()> &operation) {
	if (!journeyMutable() || !_boundary.quiet()) throw std::logic_error("Journey interaction unavailable");
	// Objective scripts require the exclusive Event continuation, never this read seam.
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
		out << " RNG=" << random->state << " draws=" << random->count;
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
	// Diagnostics list the Journey entry map's table, as before M69.
	for (const auto &a : _world.sessionState().monsters(xeenJourneyContent().entry.mapId))
		out << "Actor " << a.id.recordIndex << " map=" << a.id.mapId << ' ' << a.x << ' ' << a.y << " HP=" << a.hp
			<< " active=" << a.activated << " lifecycle=" << unsigned(a.lifecycle) << " status=" << unsigned(a.status)
			<< " accounted=" << _world.sessionState().accountedMonsters().count(a.id) << '\n';
 if(_party.purse)out<<"Purse gold="<<_party.purse->gold<<" gems="<<_party.purse->gems<<'\n';
 {
  const auto &v=_world.treasureStaging().value();
  out<<"Pending gold="<<v.gold<<" gems="<<v.gems<<" ready="<<bool(v.ready)<<'\n';
  const char *names[]={"weapon","armor","accessory","misc"};
  for(unsigned category=0;category<4;++category)for(const auto &item:v.items(static_cast<XeenRewardCategory>(category)))
   if(item.id || item.material)out<<"Pending "<<names[category]<<" M/ID/S/F="<<unsigned(item.material)<<'/'<<unsigned(item.id)<<'/'<<unsigned(item.state)<<'/'<<unsigned(item.frame)<<'\n';
 }
 if(_combat)out<<"Participants="<<unsigned(_combat->participants())<<" exit="<<unsigned(_combat->exitCause())<<'\n';
 const auto describe=[&](const XeenCombatResult &r) {
  if(r.operation==XeenCombatOperation::PlayerRun)out<<"Run owner="<<unsigned(*r.actingOwner)<<" roll="<<r.runRoll<<" success="<<r.runSuccess<<" mask="<<unsigned(r.participantsBefore)<<"->"<<unsigned(r.participantsAfter)<<'\n';
  if(r.operation==XeenCombatOperation::FinishDisengagement)out<<"Disengaged cause="<<unsigned(r.exitCause)<<" casualties="<<unsigned(r.casualties)<<" forfeitedGold="<<r.forfeitedGold<<" forfeitedGems="<<r.forfeitedGems<<'\n';
  if(r.monsterDrop)out<<"Drop outcome="<<unsigned(*r.monsterDrop)<<" category="<<unsigned(r.generatedCategory)<<" M/ID/S/F="<<unsigned(r.generatedItem.material)<<'/'<<unsigned(r.generatedItem.id)<<'/'<<unsigned(r.generatedItem.state)<<'/'<<unsigned(r.generatedItem.frame)<<'\n';
  for(unsigned i=0;i<r.injuryCount;++i){const auto &v=r.injuries[i];out<<"Injury owner="<<unsigned(v.owner)<<" amount="<<v.amount<<" HP="<<v.beforeHp<<"->"<<v.afterHp<<" AC="<<v.beforeAc<<"->"<<v.afterAc<<" conditions=";for(auto c:v.conditions)out<<unsigned(c)<<',';out<<'\n';}
  for(unsigned i=0;i<r.armorCount;++i){const auto &v=r.armor[i];out<<"Item change owner="<<unsigned(v.owner)<<" category="<<unsigned(v.category)<<" slot="<<unsigned(v.slot)<<" state="<<unsigned(v.before.state)<<"->"<<unsigned(v.after.state)<<" frame="<<unsigned(v.before.frame)<<"->"<<unsigned(v.after.frame)<<'\n';}
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
		(_world.sessionState().journeyActivity() != XeenJourneyActivity::Presentation && !journeyEvent() && !_shoot && !monsterReward() && !_itemUse && !_casting && !_smith && !_training)) return false;
	if (!journeyCapacity()) return false;
	_journeyCapture->initializationPending=false;
    projectilesPresented();
    if(_rest) {
        _rest->presented=true;
        if(_rest->phase==RestContinuation::Phase::Dream && _rest->dreamFade()) {
            std::uint64_t acquiredAt=0;
            guardCallback(entry,[&]{acquiredAt=_nanosecondClock?(*_nanosecondClock)():PresentationClock::fromMilliseconds(_clock());});
            _rest->dreamDeadline=PresentationClock::retraceAfter(acquiredAt);
        } else if(_rest->phase!=RestContinuation::Phase::Dream) {
            std::uint64_t acquiredAt=0;guardCallback(entry,[&]{acquiredAt=_clock();});
            _rest->deadline=PresentationClock::afterMilliseconds(acquiredAt,2);
        }
    }
	auto &s = _world._sessionState;
	if(_shoot && !_shoot->presented) {_shoot->presented=true;_shoot->deadline=counterDeadline(_lastTime,2);}
	if (!journeyEvent() && !_rest && !_shoot && !monsterReward() && !_itemUse && !_casting && !_smith && !_training) s._journeyActivity = _state.phase()==XeenEncounterPhase::SupportStopped ? XeenJourneyActivity::SupportStopped : (_state.pending() || _regionalWork || projectilesPending() || _shootIntent || _regionalAutomatic) ? XeenJourneyActivity::Approach : XeenJourneyActivity::Quiet;
	if (_castingSettlement && (s._journeyActivity==XeenJourneyActivity::Quiet ||
		s._journeyActivity==XeenJourneyActivity::SupportStopped || journeyEvent() || monsterReward() ||
        (_regionalAutomatic && !projectilesPending())))
		_castingSettlement=false;
	const bool smithSemantic=(_smith || _smithPreparation || (journeyEvent() && _smithEventSettlement));
	if(!_training && !_trainingPreparation && !(journeyEvent() && _trainingEventSettlement) && !smithSemantic){++s._journeyGeneration; ++_generation;}
	_journeyFramePrepared = _journeyFrameRetry = false;
	_journeyPreimage->adoptJourneyCoordination();
	_journeyCapture->generation = _boundary.generation();
	return true;
}
bool XeenEncounterFlow::prepareJourneyFrame(const Ticket &entry, const std::function<void()> &compose, bool cosmetic) {
	if (!_journey || _busy || _combat || !current(entry) || !compose ||
		(!cosmetic && _world.sessionState().journeyActivity() != XeenJourneyActivity::Presentation && !journeyEvent() && !_shoot && !monsterReward() && !_itemUse && !_casting && !_smith && !_training)) return false;
	BusyJourney busy(_busy);
	if (!cosmetic) _journeyFramePrepared = false;
	if (_itemUse && !cosmetic) { _itemUse->selectorTicket.reset(); _itemUse->selectorFrame.reset(); }
	try {
		std::uint16_t light;
		{XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
			light=_world.lightAfterScenePreparation(_party,_camera);_journeyPreimage->check();}
		if(light!=_party.encounterContext->lightAndResistances[0]) {
			auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
			prepared->retainResources(*_journeyPreimage);prepared->context->lightAndResistances[0]=light;
			_journeyPreimage->check();_party.encounterContext->lightAndResistances[0]=light;
			_journeyPreimage.swap(prepared);_journeyPreimage->adoptMutationBoundary();
		}
		_world.scenePresentation().lightingPrepared=true;
		_journeyPreimage->adoptMutationBoundary();
		XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
		compose(); _journeyPreimage->check();
		_world.scenePresentation().completeLightingDraw();
		_journeyPreimage->adoptMutationBoundary();
		if (!current(entry)) throw std::logic_error("Stale Journey frame preparation");
		if (!cosmetic) _journeyFramePrepared = true;
		else _journeyFrameRetry = false;
		return true;
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
	if (_rest || (action && (_castingSettlement || _regionalAutomatic || _regionalWork))) return refused;
	if (!_journey || _busy || _combat || !current(entry) || !_boundary.quiet() ||
		_state.phase() != XeenEncounterPhase::Exploring) return refused;
	auto &s = _world._sessionState;
	if (s._journeyActivity != XeenJourneyActivity::Quiet && s._journeyActivity != XeenJourneyActivity::Approach &&
		!(s._journeyActivity == XeenJourneyActivity::Presentation && !action && !_journeyFramePrepared && !_journeyFrameRetry)) return refused;
	if (!journeyCapacity()) return refused;
	try {
        // A retained, guarded opportunity may have published the last able
        // member's injury. Its suffix must still finish before normal defeat;
        // quiet admission applies only when starting new work.
        if(!_regionalWork) {
		    xeenValidateJourneyParty(_party);
		    const auto active=s.monsters(_camera.mapId);
		    if (std::any_of(active.begin(),active.end(),[&](const XeenActor &a) { return a.lifecycle == XeenActorLifecycle::Present; })) xeenValidateJourneyMelee(_party);
        }
	} catch (const std::invalid_argument &e) { _journeyRefusal = "This action is not supported yet for this party."; refused.reason = XeenEncounterStop::Domain; return refused; }
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
			result = XeenActorApproach::regionalTransition(_world,_party,_camera,_state,_journeyEvents,action,_regionalWork,_travelEventsProvider,
                [this](unsigned id){fmEffect(id);},[this]{fmStepEffect();});
		}
		if (!_journeyPreimage->ownersAlive()) { closeJourney(); return refused; }
		// A replaced boundary belongs to newer work. Leave its coordination alone.
		if (_boundary.generation() != boundaryGeneration) return refused;
		_world._combatCheck = {}; _world._combatAuthorized = {};
		if (result.outcome == XeenEncounterOutcome::Stale) { closeJourney(); return refused; }
		if (_state.phase() == XeenEncounterPhase::SupportStopped) {
			_regionalAutomatic=false;_regionalAutomaticAddress.reset();

			_result=result;++_generation;_journeyFramePrepared=false;s._journeyActivity=XeenJourneyActivity::Presentation;
			retainJourney();return result;
		}
		if (result.outcome == XeenEncounterOutcome::Refused) {
			if(result.arrivalRefusal)_journeyRefusal=result.arrivalRefusal;
			if (!journeyEvent() && !_shoot && !monsterReward()) s._journeyActivity = _state.phase()==XeenEncounterPhase::SupportStopped ? XeenJourneyActivity::SupportStopped : (_state.pending() || _regionalWork || projectilesPending() || _shootIntent || _regionalAutomatic) ? XeenJourneyActivity::Approach : XeenJourneyActivity::Quiet;
			_journeyPreimage->adoptJourneyCoordination(); _journeyPreimage->check(); return result;
		}
		_result = result; ++_generation; _journeyFramePrepared = false;
		if(result.terrainFall)_terrainFall=*result.terrainFall;
		if(result.terrainPortrait<7)for(auto owner:_party.party.activeRosterIds())
			_world.scenePresentation().portraitDamage(owner,result.terrainPortrait,_lastTime);
		if(action) { _itemUseResult.reset(); retireCastingFeedback(); }
		if(result.needsRest)_needsRestNotice=true;
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
bool XeenEncounterFlow::journeyExchange(const Ticket &entry,std::size_t from,std::size_t to,std::uint64_t lease) {
    if(!journeyMutable() || !current(entry) || !journeyCapacity() ||
        !_boundary.only(XeenCombatBoundary::Work::Inventory,lease) || from>=6 || to>=6 || from==to)return false;
    BusyJourney busy(_busy);
    try {
        xeenValidateJourneyParty(_party);std::vector<std::uint8_t> ids=_party.party.activeRosterIds();std::swap(ids[from],ids[to]);
        auto next=XeenParty::fromRosterIds(ids);_journeyPreimage->check();_party.party=std::move(next);
        ++_generation;retainJourney();return true;
    }catch(...) {closeJourney();throw;}
}
bool XeenEncounterFlow::rememberSpell(const Ticket &entry,std::size_t active,unsigned slot) {
    if(!current(entry) || _busy || _combat || !_casting || _casting->committed || active>=_party.party.size() || slot>=39 || !journeyCapacity())return false;
    const auto owner=_casting->owners[active];if(!XeenLearnedSpellRules::known(_party.roster.at(owner),slot))return false;
    BusyJourney busy(_busy);_journeyPreimage->check();
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);prepared->characters[owner].currentSpell=slot;
    _journeyPreimage->check();
    _party.roster.at(owner).currentSpell=slot;++_generation;
    prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
    if(_journeyCapture)_journeyCapture->generation=_boundary.generation();
    return true;
}
bool XeenEncounterFlow::configureQuickFight(const Ticket &entry,std::size_t active) {
    if(!current(entry) || _busy || !_combat)return false;
    const auto result=_combat->configureQuickFight(*entry.combat,active);return acceptCombatResult(result);
}
bool XeenEncounterFlow::journeyDiscard(const Ticket &entry,std::size_t active,XeenInventoryCategory category,
        std::size_t slot,const XeenItem &record) {
    if(_combat) {
        if(!current(entry) || active>=6 || !(_combat->participants()&(1u<<active)))return false;
        return _combat->changeInventory(*entry.combat,[&](XeenPartyState &candidate) {
            auto *items=xeenInventoryItems(candidate.roster.at(candidate.party.activeRosterIds()[active]),category);
            if(!items || slot>=9 || !xeenSameItem((*items)[slot],record))return false;
            const auto after=xeenDiscardItems(*items,slot,category);if(!after)return false;
            *items=*after;return true;
        });
    }
    if(!journeyMutable() || !_boundary.preparationReady() || !current(entry) || !journeyCapacity() || active>=_party.party.size())return false;
    BusyJourney busy(_busy);_journeyPreimage->check();
    const auto owner=_party.party.activeRosterIds()[active];
    auto *items=xeenInventoryItems(_party.roster.at(owner),category);
    if(!items || slot>=9 || !xeenSameItem((*items)[slot],record))return false;
    const auto after=xeenDiscardItems(*items,slot,category);if(!after)return false;
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);
    *xeenInventoryItems(prepared->characters[owner],category)=*after;
    xeenValidateCompletedEquipment(prepared->characters[owner]);
    XeenCharacterRules::validateForUse(prepared->characters[owner],{_party.encounterContext->year});
    _journeyPreimage->check();
    *items=*after;++_generation;
    _world._sessionState._journeyActivity=XeenJourneyActivity::Presentation;
    prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
    return true;
}
XeenEquipmentResult XeenEncounterFlow::journeyEquipment(const Ticket &entry, std::size_t active,
		XeenInventoryCategory category, std::size_t slot, XeenEquipmentOperation operation) {
	if(_combat) {
        XeenEquipmentResult result;
        if(current(entry) && active<6 && (_combat->participants()&(1u<<active)))
            _combat->changeInventory(*entry.combat,[&](XeenPartyState &candidate) {
                result=xeenSetEquipment(candidate,active,category,slot,operation);
                return result.status==XeenEquipmentStatus::Success;
            });
        return result;
    }
	if (!journeyMutable() || !_boundary.preparationReady() || !current(entry) || !journeyCapacity()) return {};
	BusyJourney busy(_busy);
	try {
		xeenValidateJourneyParty(_party);
		const auto result = xeenSetEquipment(_party,active,category,slot,operation);
		if (result.status == XeenEquipmentStatus::Success) _world._sessionState._journeyActivity = XeenJourneyActivity::Presentation;
		++_generation; retainJourney(); return result;
	} catch (...) { closeJourney(); throw; }
}
XeenTransferResult XeenEncounterFlow::journeyTransfer(const Ticket &entry, std::size_t from, std::size_t to,
		XeenInventoryCategory category, std::size_t slot) {
	if(_combat) {
        XeenTransferResult result;
        if(current(entry) && from<6 && to<6 && (_combat->participants()&(1u<<from)) && (_combat->participants()&(1u<<to)))
            _combat->changeInventory(*entry.combat,[&](XeenPartyState &candidate) {
                result=xeenTransferItem(candidate,from,to,category,slot);
                return result.status==XeenTransferStatus::Success;
            });
        return result;
    }
	if (!journeyMutable() || !_boundary.preparationReady() || !current(entry) || !journeyCapacity()) return {};
	BusyJourney busy(_busy);
	try {
		xeenValidateJourneyParty(_party);
		const auto result = xeenTransferItem(_party,from,to,category,slot);
		if (result.status == XeenTransferStatus::Success) _world._sessionState._journeyActivity = XeenJourneyActivity::Presentation;
		++_generation; retainJourney(); return result;
	} catch (...) { closeJourney(); throw; }
}
bool XeenEncounterFlow::attachJourney(const Ticket &entry, const std::function<void()> &prepareSprites) {

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
		xeenValidateJourneyMelee(_party);
		const bool carryFinish = _disengagementNoticeRevision && *_disengagementNoticeRevision == _state.revision();
		{
			XeenRestoreGuard::Providers providers(*_journeyPreimage,_world,checkBoundary);
			XeenActorApproach::validateEnvironment(_world,_world.sessionState().regionalActors(_camera.mapId),_events);
			checkBoundary();
			_journeyPreimage->check();
			if (prepareSprites) prepareSprites();
			checkBoundary();
			_journeyPreimage->check();
			// Sprite preparation may discard geometry/MOB caches. Rebuild under the
			// same entry authority before combat construction can publish a borrow.
			XeenActorApproach::validateEnvironment(_world,_world.sessionState().regionalActors(_camera.mapId),_events);
		}
		checkBoundary();
		_journeyPreimage->check();
		_combat.reset(new XeenCombat(_world,_party,_camera,_boundary,_flags,_state,_journeyStatistics,_events));
        _fmAttackGeneration.reset(); // Combat generations are local to this new instance.
        _combat->fmEffects([this](unsigned id){fmEffect(id);},[this]{fmWeaponEffect();},[this](unsigned weapon){fmSelectWeapon(weapon);},[this](unsigned id){fmEffectAfterFrame(id);},[this]{digitalMiss();});
        _combat->inheritResources(_combat->ticket(),*_journeyPreimage);
		++_generation;
		if (!acceptCombatResult(_combat->beginCombat(_combat->ticket()))) return false;
		// Only this immediate destination incarnation may present the retained finish.
		if (carryFinish) _disengagementNoticeCombat = _combat->ticket();
		observeCombat(); _deadline.reset(); _frame = 0; _appearanceStep = 0;
		std::uint64_t now;
		if (!prepareTime(ticket(),now)) throw std::runtime_error("Journey attachment scheduling failed");
		_lastTime = now; _cosmeticDeadline = counterDeadline(now,2); scheduleCombat(now);
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
		std::vector<XeenExploredTile> exploration;
		{XeenRestoreGuard::Providers providers(*prepared,_world);
			exploration=_world.explorationAfterStep(_party,_camera);prepared->check();}
		prepared->s.assignExploration(exploration);
		const bool disengaged=_combat->phase()==XeenCombatPhase::Disengaged;
		if(disengaged && _regionalAutomatic && !_combat->result().originAutomaticSuperseded) throw std::logic_error("Finish did not supersede the origin event");
		if(disengaged) _combat->retireDisengagedJourney(*entry.combat,_state);
		else _combat->retireJourney(*entry.combat,_state);
        if(disengaged)_world.scenePresentation().markLightingStep();
		if(disengaged) { _journeyCapture->inputPassPending=false; _regionalAutomatic=false;_regionalAutomaticAddress.reset();_combatObservation=_combat->result(); }
		_retiredCombatResult = _combat->result(); _combat.reset(); ++_generation;
		_disengagementNoticeRevision = disengaged ? std::optional<std::uint64_t>{_state.revision()} : std::nullopt;
		_disengagementNoticeCombat.reset();
		// The origin observations have completed presentation before retirement.
		if(disengaged) { _journeyRefusal.clear();_rangedObservation.reset(); }
		_deadline.reset(); _frame = 0; _appearanceStep = 0; _appearanceIdentity.reset();
		_scheduleAfterFrame = _appearanceAfterFrame = false;
		_world._sessionState.publishExploration(exploration);
		prepared->adoptJourneyCoordination(); prepared->adoptJourneyBorrowRelease(); _journeyPreimage.swap(prepared);
		return true;
	} catch (const std::invalid_argument &) { return false; }
}
}
