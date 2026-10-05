#include "app/XeenEncounterFlow.h"
#include "app/XeenEventFlow.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenEventPublication.h"
#include <stdexcept>
#include <sstream>
#include <limits>
#include "games/xeen/XeenEquipment.h"

namespace mmodern {
namespace {
struct SmithBusy { bool &flag; explicit SmithBusy(bool &value):flag(value){flag=true;} ~SmithBusy(){flag=false;} };
bool samePurchaseQuote(const XeenEquipmentPurchaseResult &a,const XeenEquipmentPurchaseResult &b) {
 return a.outcome==b.outcome && a.category==b.category && a.side==b.side && a.shop==b.shop &&
  a.member==b.member && a.owner==b.owner && a.offerSlot==b.offerSlot &&
  a.activeRosterIds==b.activeRosterIds && xeenSameItem(a.offer,b.offer) &&
  xeen_state::sameItemCategory(a.recipientBefore,b.recipientBefore) &&
  xeen_state::sameItemCategory(a.stockBefore,b.stockBefore) && a.price==b.price &&
  a.goldBefore==b.goldBefore && a.goldAfter==b.goldAfter && a.shortfall==b.shortfall;
}
}
void XeenEncounterFlow::checkSmithBoundary(XeenSmithBoundary boundary) {
 _journeyPreimage->check();
 try { if (_smithBoundary) _smithBoundary(boundary); }
 catch (...) { _journeyPreimage->check();throw; }
 _journeyPreimage->check();
}
bool XeenEncounterFlow::smithCapacity(unsigned journeySteps,unsigned boundarySteps) const noexcept {
 return xeenSmithAuthorityRoom(_generation,journeySteps) &&
  xeenSmithAuthorityRoom(_world.sessionState()._journeyGeneration,journeySteps) &&
  xeenSmithAuthorityRoom(_boundary.generation(),boundarySteps);
}
void XeenEncounterFlow::advanceSmith() noexcept {
 ++_world._sessionState._journeyGeneration; ++_generation;
 _journeyPreimage->adoptJourneyCoordination();
}
void XeenEncounterFlow::checkSmithReservation() {
 const auto *day=_smith ? _smith->departure.get() : nullptr;
 if (!day || !day->complete() || !_party.encounterContext || !_party.serviceEconomy ||
     !_world.sessionState().journeyRandom() || !(day->beforeContext()==*_party.encounterContext) ||
     day->beforeEconomy()!=*_party.serviceEconomy || day->beforeRandom()!=*_world.sessionState().journeyRandom()) {
  _journeyPreimage->failed=true;
  throw std::logic_error("Complete Smith departure reservation changed");
 }
 if ((!_smith->binding || !_smith->binding->matches(*day))) {
  _journeyPreimage->failed=true;
  throw std::logic_error("Complete Smith departure candidate changed");
 }
}
bool XeenEncounterFlow::beginSmith(const std::function<void()> &preflight) {
	const bool temple=_camera.mapId==XeenMapIdentity(28) && _camera.x==15 && _camera.y==28;
	if (!journeyEvent() || _smith || _smithPreparation || _busy || !current(ticket()) || !journeyCapacity() ||
		_camera.mapId!=XeenMapIdentity(28) ||
		!((temple) || (_camera.x==8 && _camera.y==4)) || !_party.encounterContext) return false;
	SmithBusy busy(_busy);
	_journeyPreimage->check();
	// Reserve preparation/admission, two service frames, departure, Event
	// retirement and final presentation, including the inherited guard margin.

	if (!smithCapacity(20,5)) return false;
	if (!xeenPrepareSmithDeparture(*_party.encounterContext)) return false;
	xeenValidateJourneyParty(_party);
	checkSmithBoundary(XeenSmithBoundary::BeforeReservation);
	auto next=std::make_unique<SmithContinuation>();
	next->temple=temple;
    {
        if (!_party.serviceEconomy || !_world.sessionState().journeyRandom())
            throw std::logic_error("Missing service-day owners");
        next->departure=std::make_unique<XeenServiceDayCandidate>(*_party.encounterContext,
            *_party.serviceEconomy,*_world.sessionState().journeyRandom());
		next->reservation=1;
    }
	checkSmithBoundary(XeenSmithBoundary::AfterReservation);
 checkSmithBoundary(XeenSmithBoundary::BeforeAdmission);
	// Event remains exclusive throughout resource and first-frame preparation.
	try { XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);preflight();_journeyPreimage->check(); }
	catch (...) { _journeyPreimage->check();throw; }
    _smithPreparation=std::move(next);
    return true;
}
bool XeenEncounterFlow::serviceSmithPreparation() {
    if (!_smithPreparation || _smith || _busy || !journeyEvent() ||
        !current(ticket()) || !journeyCapacity()) return false;
    SmithBusy busy(_busy);
    _journeyPreimage->check();

    if (!smithCapacity(17,5)) throw std::overflow_error("Smith admission authority exhausted");
    auto &day=*_smithPreparation->departure;
    if (!day.service(64,[&] { _journeyPreimage->check(); },
        [&] { checkSmithBoundary(XeenSmithBoundary::StockComplete); })) return false;
	checkSmithBoundary(XeenSmithBoundary::BankPrepared);
    if (!day.complete() || !(day.beforeContext()==*_party.encounterContext) ||
        day.beforeEconomy()!=*_party.serviceEconomy ||
        day.beforeRandom()!=*_world.sessionState().journeyRandom())
        throw std::logic_error("Service-day reservation changed");
    xeenValidateCurrentServiceEconomy(day.economy());
    _smithPreparation->binding.emplace(day);
    _journeyPreimage->check();
    _smithPreparation->lease=_boundary.hold(XeenCombatBoundary::Work::Service);
    _smith=std::move(_smithPreparation);
    _world._sessionState._journeyActivity=XeenJourneyActivity::Service;
    ++_world._sessionState._journeyGeneration;++_generation;
    _journeyPreimage->adoptJourneyCoordination();
    checkSmithBoundary(XeenSmithBoundary::AfterAdmission);
    return true;
}
void XeenEncounterFlow::authorizeSmithFrame(std::uint64_t input,const IndexedFrame::Presentation &frame) {
	if (!_smith || !frame || !input || _busy || _failure || !current(ticket()) ||
		_world.sessionState().journeyActivity()!=XeenJourneyActivity::Service ||
		!_boundary.holds(XeenCombatBoundary::Work::Service,_smith->lease))
		throw std::logic_error("Smith frame authority unavailable");
	_smith->input=input;_smith->frame=frame;
}
bool XeenEncounterFlow::consumeSmithFrame(std::uint64_t input,const IndexedFrame::Presentation &frame) {
	if (!_smith || !frame || _busy || _failure || !current(ticket()) ||
		_smith->input!=input || _smith->frame!=frame ||
		!_boundary.holds(XeenCombatBoundary::Work::Service,_smith->lease)) return false;
	// Consume before any validation/provider call; a recursive response cannot reuse it.
	_smith->frame.reset();_smith->input=0;
	_journeyPreimage->check();return true;
}
void XeenEncounterFlow::quoteSmith(std::size_t member,std::size_t slot) {
	if (!_smith || _busy || _smith->frame || member>=_party.party.size() || slot>=9)
		throw std::logic_error("Smith selection authority unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();
	xeenValidateJourneyParty(_party);
	{
		if (!smithCapacity(18,4)) throw std::overflow_error("Smith quote would consume mandatory departure authority");
		checkSmithReservation();
	}
	if (_smith->operation==std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Smith operation generation exhausted");
    ++_smith->operation;
    _smith->owner=_party.party.activeRosterIds()[member];_smith->slot=static_cast<std::uint8_t>(slot);
	_smith->buy=false;_smith->published=false;_smith->purchase.reset();
	_smith->result=xeenQuoteArmorRepair(XeenInventoryCategory::Armor,
		_party.roster.at(_smith->owner).armor[slot],_party.monsterTreasure->gold);
	_smith->quoted=_smith->result.outcome==XeenArmorRepairOutcome::Quoted;
	_smith->quoteOperation=_smith->operation;_smith->quoteReservation=_smith->reservation;
	advanceSmith();
 checkSmithBoundary(XeenSmithBoundary::Quote);
}
void XeenEncounterFlow::confirmSmith() {
	if (!_smith || !_smith->quoted || _busy || _smith->frame || !journeyCapacity())
		throw std::logic_error("Smith quote authority unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();

	if (!smithCapacity(19,4)) throw std::overflow_error("Repair would consume its result and owed departure authority");
	{
		checkSmithReservation();
		if (_smith->buy || _smith->published || _smith->quoteOperation!=_smith->operation || _smith->quoteReservation!=_smith->reservation)
			throw std::logic_error("Smith repair operation changed");
	}
	const auto before=_smith->result;
	const auto &live=_party.roster.at(_smith->owner).armor[_smith->slot];
	if (!xeenSameItem(live,before.before) || _party.monsterTreasure->gold!=before.goldBefore)
		throw std::logic_error("Smith quote preimage changed");
	const auto result=xeenPrepareArmorRepair(live,_party.monsterTreasure->gold);
	auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
	prepared->retainResources(*_journeyPreimage);
	prepared->characters[_smith->owner].armor[_smith->slot]=result.after;
    xeenValidateCompletedEquipment(prepared->characters[_smith->owner]);
	prepared->treasure->gold=result.goldAfter;
	_journeyPreimage->check();
	checkSmithBoundary(XeenSmithBoundary::BeforeRepair);
	checkSmithReservation();
 // One callback-free nonthrowing commit: purse, physical item and fixed result.
	_party.monsterTreasure->gold=result.goldAfter;
	_party.roster.at(_smith->owner).armor[_smith->slot]=result.after;
	_smith->result=result;_smith->quoted=false;_smith->published=true;
	++_world._sessionState._journeyGeneration;++_generation;
	prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
 checkSmithBoundary(XeenSmithBoundary::AfterRepair);
}
void XeenEncounterFlow::quoteSmithBuy(std::size_t member,XeenInventoryCategory category,std::size_t slot) {
 if (!_smith || _busy || _smith->frame || _smith->departed ||
     !smithCapacity(18,4) || !xeenSmithAuthorityRoom(_smith->operation,1))
  throw std::logic_error("Smith Buy selection authority unavailable");
 SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
 auto quote=std::make_unique<XeenEquipmentPurchaseCandidate>();
 quote->result=xeenQuoteEquipmentPurchase(_party,member,category,slot);
 quote->economyBefore=quote->economyAfter=*_party.serviceEconomy;
 quote->result.quotedOperation=_smith->operation+1;
 quote->result.quotedReservation=_smith->reservation;
 quote->result.publishedReservation=_smith->reservation;
 _journeyPreimage->check();
 // The retained quote is installed before fallible observation. A retry may
 // present it again, but cannot issue another operation for the same response.
 ++_smith->operation;_smith->quoteOperation=_smith->operation;
 _smith->quoteReservation=_smith->reservation;
 _smith->owner=quote->result.owner;_smith->slot=quote->result.offerSlot;
 _smith->buy=true;_smith->published=false;
 _smith->quoted=quote->result.outcome==XeenEquipmentPurchaseOutcome::Quoted;
 _smith->purchase.swap(quote);advanceSmith();
 checkSmithBoundary(XeenSmithBoundary::Quote);
}
void XeenEncounterFlow::confirmSmithBuy() {
 if (!_smith || !_smith->buy || !_smith->quoted || _smith->published || !_smith->purchase ||
     _busy || _smith->frame || _smith->departed || !smithCapacity(19,4) ||
     !xeenSmithAuthorityRoom(_smith->reservation,1))
  throw std::logic_error("Smith Buy confirmation authority unavailable");
 SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
 const auto &quoted=_smith->purchase->result;
 const auto currentQuote=xeenQuoteEquipmentPurchase(_party,quoted.member,quoted.category,quoted.offerSlot,quoted.side,quoted.shop);
 if (_smith->quoteOperation!=_smith->operation || _smith->quoteReservation!=_smith->reservation ||
     quoted.quotedOperation!=_smith->operation || quoted.quotedReservation!=_smith->reservation ||
     !samePurchaseQuote(quoted,currentQuote) || _smith->purchase->economyBefore!=*_party.serviceEconomy ||
     _smith->purchase->economyAfter!=*_party.serviceEconomy) {
  _journeyPreimage->failed=true;
  throw std::logic_error("Smith Buy quote preimage changed");
 }
 auto candidate=std::make_unique<XeenEquipmentPurchaseCandidate>(xeenPrepareEquipmentPurchase(_party,
     quoted.member,quoted.category,quoted.offerSlot,quoted.side,quoted.shop));
 candidate->result.quotedOperation=quoted.quotedOperation;
 candidate->result.quotedReservation=quoted.quotedReservation;
 candidate->result.publishedReservation=_smith->reservation+
     (candidate->result.outcome==XeenEquipmentPurchaseOutcome::Purchased?1:0);
 const auto &result=candidate->result;
 if (result.outcome==XeenEquipmentPurchaseOutcome::InsufficientGold) {
  _journeyPreimage->check();checkSmithReservation();
  _smith->purchase.swap(candidate);_smith->quoted=false;_smith->published=true;advanceSmith();return;
 }
 if (result.outcome!=XeenEquipmentPurchaseOutcome::Purchased) {
  _journeyPreimage->failed=true;throw std::logic_error("Smith Buy candidate no longer matches its admitted quote");
 }
 checkSmithBoundary(XeenSmithBoundary::BeforeRebind);
 auto departure=std::make_unique<XeenServiceDayCandidate>(_smith->departure->rebindPurchase(candidate->economyAfter,
     result.category,result.offerSlot,result.offer));
 checkSmithBoundary(XeenSmithBoundary::AfterRebind);
 const SmithDepartureBinding binding(*departure);
 auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
 prepared->retainResources(*_journeyPreimage);
 *xeenInventoryItems(prepared->characters[result.owner],result.category)=result.recipientAfter;
 prepared->treasure->gold=result.goldAfter;prepared->economy=candidate->economyAfter;
 auto *recipient=xeenInventoryItems(_party.roster.at(result.owner),result.category);
 auto *stock=&_party.serviceEconomy->wares[0][0][static_cast<unsigned>(result.category)];
 // All allocation, validation, physical-index resolution and observation
 // preparation are complete. Preserve the old obligation until this succeeds.
 checkSmithBoundary(XeenSmithBoundary::BeforePurchase);
 checkSmithReservation();
 if (!smithCapacity(19,4) || !xeenSmithAuthorityRoom(_smith->reservation,1))
  throw std::overflow_error("Smith Buy lost its mandatory settlement headroom");
 static_assert(std::is_nothrow_copy_assignable_v<XeenItemCategory> &&
     std::is_nothrow_copy_assignable_v<SmithDepartureBinding>);
 // One callback-free, nonthrowing publication: exactly the two touched arrays,
 // purse, completed replacement obligation, fixed result and spent operation.
 *recipient=result.recipientAfter;*stock=result.stockAfter;
 _party.monsterTreasure->gold=result.goldAfter;
 _smith->departure.swap(departure);*_smith->binding=binding;++_smith->reservation;
 _smith->purchase.swap(candidate);_smith->quoted=false;_smith->published=true;
 ++_world._sessionState._journeyGeneration;++_generation;
 prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
 checkSmithBoundary(XeenSmithBoundary::AfterPurchase);
}
void XeenEncounterFlow::quoteTempleHeal(std::size_t member) {
	if(!_smith || !_smith->temple || _smith->departed || _busy || _smith->frame ||
		member>=_party.party.size() || !smithCapacity(18,4) ||
		!xeenSmithAuthorityRoom(_smith->operation,1))
		throw std::logic_error("Temple selection authority unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
	xeenValidateJourneyParty(_party);
	const auto owner=_party.party.activeRosterIds()[member];
	auto quote=xeenQuoteTempleHeal(_party.roster.at(owner),_party.monsterTreasure->gold,
		*_party.encounterContext);
	if(quote.outcome==XeenTempleHealOutcome::Quoted) {
		quote=xeenPrepareTempleHeal(_party,owner,*_party.encounterContext).result;
		quote.outcome=XeenTempleHealOutcome::Quoted;quote.goldAfter=quote.goldBefore;
	}
	_smith->owner=owner;_smith->healResult=quote;
	_smith->quoted=quote.outcome==XeenTempleHealOutcome::Quoted;
	_smith->published=false;++_smith->operation;
	_smith->quoteOperation=_smith->operation;_smith->quoteReservation=_smith->reservation;
	advanceSmith();checkSmithBoundary(XeenSmithBoundary::Quote);
}
bool XeenEncounterFlow::confirmTempleHeal() {
	if(!_smith || !_smith->temple || !_smith->quoted || _smith->published || _smith->healPending ||
		_busy || _smith->frame || _smith->departed || !smithCapacity(19,4))
		throw std::logic_error("Temple Heal confirmation authority unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
	const auto &quoted=_smith->healResult;
	const auto current=xeenQuoteTempleHeal(_party.roster.at(_smith->owner),
		_party.monsterTreasure->gold,*_party.encounterContext);
	if(_smith->quoteOperation!=_smith->operation || _smith->quoteReservation!=_smith->reservation ||
		quoted.owner!=current.owner || quoted.price!=current.price ||
		quoted.goldBefore!=current.goldBefore || quoted.hpBefore!=current.hpBefore ||
		quoted.spBefore!=current.spBefore || quoted.maxHpBefore!=current.maxHpBefore ||
		current.outcome!=XeenTempleHealOutcome::Quoted) {
		_journeyPreimage->failed=true;
		throw std::logic_error("Temple Heal quote preimage changed");
	}
	if(!_smith->paid && !xeenPrepareTemplePaidDeparture(*_party.encounterContext)) {
		_smith->healResult.outcome=XeenTempleHealOutcome::SupportLimit;
		_smith->quoted=false;advanceSmith();return true;
	}
	auto delta=std::make_unique<XeenTempleHealCandidate>(
		xeenPrepareTempleHeal(_party,_smith->owner,*_party.encounterContext));
	if(delta->result.outcome!=XeenTempleHealOutcome::Healed ||
		delta->result.price!=quoted.price) {
		_journeyPreimage->failed=true;
		throw std::logic_error("Temple Heal candidate differs from its quote");
	}
	// Assigned maximum HP zero with cleared conditions is unrepresentable in a
	// Journey party; refuse before any debit instead of failing after publication.
	if(delta->result.hpAfter<=0) {
		_smith->healResult.outcome=XeenTempleHealOutcome::HpSupportLimit;
		_smith->quoted=false;advanceSmith();return true;
	}
	std::unique_ptr<XeenServiceDayCandidate> upgrade;
	if(!_smith->paid) {
		checkSmithBoundary(XeenSmithBoundary::BeforeHealUpgrade);
		upgrade=std::make_unique<XeenServiceDayCandidate>(_smith->departure->upgradeTemplePaid());
		checkSmithBoundary(XeenSmithBoundary::AfterHealUpgrade);
	}
	_journeyPreimage->check();checkSmithReservation();
	_smith->healPending.swap(delta);_smith->templeUpgrade.swap(upgrade);
	_smith->quoted=false;advanceSmith();
	return false; // Private preparation and publication run in idle slices.
}
bool XeenEncounterFlow::serviceTempleHeal() {
	if(!_smith || !_smith->temple || !_smith->healPending || _busy || !current(ticket()))return false;
	SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
	auto &visit=*_smith;
	if(visit.templeUpgrade) {
		bool callbackOverflow=false;
		try {
			if(!visit.templeUpgrade->service(64,[&]{
				try {_journeyPreimage->check();}
				catch(const std::overflow_error &) {callbackOverflow=true;throw;}
			},[&]{
				try {checkSmithBoundary(XeenSmithBoundary::StockComplete);}
				catch(const std::overflow_error &) {callbackOverflow=true;throw;}
			}))return false;
		} catch(const std::overflow_error &) {
			if(callbackOverflow)throw;
			_journeyPreimage->check();visit.healPending.reset();visit.templeUpgrade.reset();
			visit.healResult.outcome=XeenTempleHealOutcome::SupportLimit;advanceSmith();return true;
		}
		checkSmithBoundary(XeenSmithBoundary::BankPrepared);
		if(!(visit.templeUpgrade->beforeContext()==visit.departure->beforeContext()) ||
			visit.templeUpgrade->beforeEconomy()!=visit.departure->beforeEconomy() ||
			visit.templeUpgrade->beforeRandom()!=visit.departure->beforeRandom()) {
			_journeyPreimage->failed=true;
			throw std::logic_error("Temple replacement departure changed");
		}
		visit.templeUpgrade->validateComplete();
	}
	if(!smithCapacity(19,4))throw std::overflow_error("Temple publication would consume departure authority");
	const auto &delta=*visit.healPending;
	const SmithDepartureBinding binding(visit.templeUpgrade?*visit.templeUpgrade:*visit.departure);
	auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
	prepared->retainResources(*_journeyPreimage);
	prepared->characters[visit.owner]=delta.character;
	prepared->inputs[visit.owner]=delta.inputs;
	prepared->treasure->gold=delta.result.goldAfter;
	static_assert(std::is_nothrow_copy_assignable_v<XeenTempleHealResult> &&
		std::is_nothrow_copy_assignable_v<SmithDepartureBinding>);
	_journeyPreimage->check();checkSmithBoundary(XeenSmithBoundary::BeforeHeal);checkSmithReservation();
	_journeyPreimage->check();
	// The publication writes only the selected modeled fields and carried purse.
	auto &c=_party.roster.at(visit.owner);auto &i=*_party.roster._combatInputs[visit.owner];
	c.intellect.temporary=0;c.personality.temporary=0;c.endurance.temporary=0;c.temporaryLevel=0;
	i.might.temporary=0;i.speed.temporary=0;i.accuracy.temporary=0;i.temporaryAc=0;
	i.luck->temporary=0;i.resistances->coldTemporary=0;i.resistances->electricalTemporary=0;
	i.poisonResistance->temporary=0;
	c.currentHp=delta.result.hpAfter;
	for(unsigned condition=1;condition<=15;++condition)c.conditions[condition]=0;
	_party.monsterTreasure->gold=delta.result.goldAfter;
	if(visit.templeUpgrade) {
		visit.departure.swap(visit.templeUpgrade);
		*visit.binding=binding;++visit.reservation;visit.paid=true;
	}
	visit.healResult=delta.result;visit.published=true;
	visit.healPending.reset();visit.templeUpgrade.reset();
	++_world._sessionState._journeyGeneration;++_generation;
	prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
	checkSmithBoundary(XeenSmithBoundary::AfterHeal);
	return true;
}
void XeenEncounterFlow::cancelTempleHeal() {
	if(!_smith || !_smith->temple || !_smith->healPending || _smith->published ||
		_busy || _smith->frame || !current(ticket()))
		throw std::logic_error("Unpublished Temple Heal cancellation unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
	_smith->healPending.reset();_smith->templeUpgrade.reset();
	_smith->quoted=false;advanceSmith();
}
void XeenEncounterFlow::departSmith() {
	if (!_smith || _busy || _smith->frame || !journeyCapacity())
		throw std::logic_error("Smith departure authority unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();
	if (!smithCapacity(_smith->departed?5:6,2))
		throw std::overflow_error("Reserved smith departure authority exhausted");
	if (!_smith->departed) {
		checkSmithReservation();
		const auto &day=*_smith->departure;
		const XeenMutableOptional<XeenGameplayContext> endingContext(day.context());
		const XeenMutableOptional<XeenServiceEconomy> endingEconomy(day.economy());
		const XeenMutableOptional<XeenJourneyRandomState> endingRandom(day.continuation());
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		prepared->context=day.context();prepared->economy=day.economy();
		prepared->s._journeyRandom=day.continuation();
		xeenValidateCurrentServiceEconomy(day.economy());
		_journeyPreimage->check();
		checkSmithBoundary(XeenSmithBoundary::BeforeDeparture);
		checkSmithReservation();
		static_assert(std::is_nothrow_copy_assignable_v<decltype(_party.encounterContext)> &&
			std::is_nothrow_copy_assignable_v<decltype(_party.serviceEconomy)> &&
			std::is_nothrow_copy_assignable_v<decltype(_world._sessionState._journeyRandom)>);
        _party.encounterContext=endingContext;
        _party.serviceEconomy=endingEconomy;
        _world._sessionState._journeyRandom=endingRandom;
        _smith->departed=true;
		++_world._sessionState._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
		checkSmithBoundary(XeenSmithBoundary::DeparturePublished);
	}
 checkSmithBoundary(XeenSmithBoundary::AfterDeparture);
	// Date is already paid. Classification failure cannot repeat it or undo repairs.
	XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
	const auto view=XeenIndoorScene().classifyActors(_world,_camera,_world.sessionState().regionalActors(28));
	_journeyPreimage->check();
	checkSmithBoundary(XeenSmithBoundary::Return);
 publishArrival(view);
	const auto lease=_smith->lease;
	_world._sessionState._journeyActivity=XeenJourneyActivity::Event;
	++_world._sessionState._journeyGeneration;++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	_smith.reset();_boundary.release(XeenCombatBoundary::Work::Service,lease);
	_smithEventSettlement=true;
}

void XeenEventFlow::prepareSmith() {
	try {
		const bool temple=_camera.mapId==XeenMapIdentity(28) && _camera.x==15 && _camera.y==28;
		const unsigned inputFrames=10;
		if (!xeenSmithAuthorityRoom(_inputGeneration,inputFrames))
			throw std::overflow_error("Smith input authority exhausted before admission");
		_encounter->_smithBoundary=[this](XeenSmithBoundary stage) { if(smithBoundary)smithBoundary(stage); };
  const bool admitted=_encounter->beginSmith([&] {
			if (temple ? !drawTempleArt : !drawSmithArt)
				throw std::runtime_error("Service artwork provider is unavailable");
			const auto events=_events.scriptForMap(28).file();
			const auto mainland=_events.scriptForMap(23).file();
			const auto text=_events.textForMap(28);
			_encounter->journeySavePreimage().admitVertigoText(text);
			SmithUi ui;ui.catalog=_catalog;ui.art=_frame;
			if(temple)ui.mode=SmithUi::Mode::Heal;
			try { if(temple)drawTempleArt(ui.art);else drawSmithArt(ui.art); }
            catch (const std::invalid_argument &) { _encounter->journeySavePreimage().failed=true;throw; }
			_encounter->journeySavePreimage().check();
			if (!ui.art.isValid() || ui.art.width!=320 || ui.art.height!=200)
				throw std::runtime_error("Ironworks artwork frame is invalid");
			// Detach retained storage from every mutable buffer exposed to the provider.
			_smithUi=ui;
			(void)drawSmith(_frame); // All fallible initial display preparation precedes admission.
		});
		if (!admitted) {
			_smithUi.reset();_pending.reset();_journeyEventLayers=false;
			_encounter->_journeyRefusal=temple ? "Temple unavailable: departure exceeds the supported year." :
				"Ironworks unavailable: departure exceeds the supported year.";
		} else {
            if (_encounter->_smithPreparation) _smithUi->phase=SmithUi::Phase::Preparation;
            _presenter.clear();_journeyEventLayers=false;
        }
 } catch (const std::exception &) {
  _encounter->journeySavePreimage().check();
		if (!_encounter->_smith) {
		 _encounter->_smithPreparation.reset();
   _smithUi.reset();_pending.reset();_journeyEventLayers=false;
		 _encounter->_journeyRefusal="Service preparation failed. Try entry again.";
  } else {
   _presenter.clear();_journeyEventLayers=false;
   _smithUi->feedback="Retry; one-day departure still owed.";
  }
 }
}
namespace {
unsigned serviceKey(const PlayerAction &action) {
    if(const auto *key=std::get_if<DialogKeyAction>(&action)) return key->key;
    if(const auto *member=std::get_if<SelectMemberAction>(&action)) return InputKey::F1+member->partyIndex;
    if(const auto *slot=std::get_if<SelectInventorySlotAction>(&action)) return '1'+slot->slot;
    if(std::holds_alternative<CancelInteractionAction>(action)) return InputKey::Escape;
    if(std::holds_alternative<YesAction>(action)) return 'y';
    if(std::holds_alternative<NoAction>(action)) return 'n';
    if(std::holds_alternative<AcknowledgeAction>(action)) return InputKey::Enter;
    return 0;
}
}
std::shared_ptr<const DialogInput> XeenEventFlow::serviceDialogInput() const {
    DialogInput input;
    if(_trainingUi) {
        const auto &ui=*_trainingUi;
        if(ui.phase==TrainingUi::Phase::Preparation || ui.phase==TrainingUi::Phase::Candidate) return std::make_shared<const DialogInput>();
        if(ui.phase==TrainingUi::Phase::Departure) input.keys={InputKey::Escape,InputKey::Enter};
        else input=xeenLocationInput(XeenLocationDialog::Training);
        if(!ui.feedback.empty()) {input={};input.anyKey=input.anyClick=true;}
    } else if(_smithUi) {
        const auto &ui=*_smithUi;
        if(ui.phase==SmithUi::Phase::Preparation) return std::make_shared<const DialogInput>();
        if(ui.phase==SmithUi::Phase::Departure) input.keys={InputKey::Escape,InputKey::Enter};
        else if(ui.phase==SmithUi::Phase::Upgrade) input.keys={InputKey::Escape};
        else if(ui.phase==SmithUi::Phase::Confirm) input=xeenConfirmInput();
        else if(ui.phase==SmithUi::Phase::Browse) input=xeenBuyInput(ui.mode==SmithUi::Mode::Repair);
        else input=xeenLocationInput(ui.mode==SmithUi::Mode::Heal?XeenLocationDialog::Temple:XeenLocationDialog::Smith);
        if(!ui.feedback.empty() && ui.phase!=SmithUi::Phase::Departure) {input={};input.anyKey=input.anyClick=true;}
    } else return {};
    return std::make_shared<const DialogInput>(std::move(input));
}
std::string XeenEventFlow::smithText() const {
    const auto &ui=*_smithUi;
    if(!ui.feedback.empty()) return ui.feedback;
    if(ui.phase==SmithUi::Phase::Confirm) {
        const auto &visit=*_encounter->_smith;
        const auto &item=ui.mode==SmithUi::Mode::Buy?visit.purchase->result.offer:visit.result.before;
        return xeenServiceConfirm(ui.mode==SmithUi::Mode::Repair,
            ui.catalog.describe(ui.category,item).displayName,
            ui.mode==SmithUi::Mode::Buy?visit.purchase->result.price:visit.result.price);
    }
    return xeenLocationText(ui.mode==SmithUi::Mode::Heal?XeenLocationDialog::Temple:XeenLocationDialog::Smith,_party,ui.member);
}
IndexedFrame XeenEventFlow::drawSmith(const IndexedFrame &world) const {
    const auto &ui=*_smithUi;
    auto frame=drawXeenLocation(world,ui.art,_inventoryFont,
        ui.mode==SmithUi::Mode::Heal?XeenLocationDialog::Temple:XeenLocationDialog::Smith,_party,ui.member,drawDialogSprite);
    if(ui.phase==SmithUi::Phase::Browse || ui.phase==SmithUi::Phase::Confirm) {
        XeenInventorySelection selection;selection.source=ui.member;selection.category=ui.category;
        if(ui.selected) selection.slot=ui.slot;
        frame=drawXeenBuy(frame,_inventoryFont,ui.catalog,_party,selection,ui.mode==SmithUi::Mode::Repair,drawDialogSprite);
        if(ui.phase==SmithUi::Phase::Confirm) frame=drawXeenConfirm(frame,_inventoryFont,smithText(),false,drawDialogSprite);
    }
    if(!ui.feedback.empty()) frame=drawXeenErrorScroll(frame,_inventoryFont,ui.feedback);
    return frame;
}
IndexedFrame XeenEventFlow::settleSmithEvent() {
	return journeyEventWork([&] {
		_encounter->checkSmithBoundary(XeenSmithBoundary::BeforeEventSettlement);
		if (!_smithTerminalResult) {
			// Retain the admitted terminal suffix until execution succeeds. A failed
			// provider must not consume the pending state or admit another visit.
			auto state=_pending->state;
			auto result=_events.resumeManualEvent(std::move(state),XeenPresentationResponse::Acknowledged,
				_world,_party,_camera,_flags,_eventPublication);
			if (!std::holds_alternative<XeenManualEventCompleted>(result)) {
				_fatal=true;throw std::runtime_error("Ironworks mandatory Event settlement failed");
			}
			_smithTerminalResult=std::move(result);
		}
		if (!_smithReported) {
			if (reportManual) reportManual(*_smithTerminalResult);
			_eventPublication->check();_smithReported=true;
		}
		_encounter->checkSmithBoundary(XeenSmithBoundary::AfterEventSettlement);
		_pending.reset();_smithUi.reset();_smithTerminalResult.reset();_smithRenderedRevision.reset();
		_smithSettlement=_smithReported=false;
	});
}
IndexedFrame XeenEventFlow::handleSmith(const PlayerAction &action,std::uint64_t input,const IndexedFrame::Presentation &inputFrame) {
    if(_smithUi->mode==SmithUi::Mode::Heal) return handleTemple(action,input,inputFrame);
    auto &ui=*_smithUi;const auto key=serviceKey(action);
    if(ui.phase==SmithUi::Phase::Preparation) return frameCopy();
    const bool member=key>=InputKey::F1 && key<InputKey::F1+_party.party.size();
    const bool allowed=ui.phase==SmithUi::Phase::Departure?(key==InputKey::Escape || key==InputKey::Enter):
        ui.phase==SmithUi::Phase::Confirm?xeenConfirmAnswer(key).has_value():
        ui.phase==SmithUi::Phase::Lobby?(key=='b' || key==InputKey::Escape || member):
        (key==InputKey::Escape || member || (key>='1' && key<='9') || key=='w' || key=='a' || key=='c' || key=='m' || key=='b' || key=='s' || key=='i' || key=='f');
    if((ui.feedback.empty() || ui.phase==SmithUi::Phase::Departure) && (!allowed || (member && key-InputKey::F1==ui.member))) return frameCopy();
    if(_smithSettlement) {_handoffPending=true;return settleSmithEvent();}
    if(!_encounter->consumeSmithFrame(input,inputFrame)) return frameCopy();
    const bool cancel=key==InputKey::Escape;
    const bool confirming=ui.phase==SmithUi::Phase::Confirm && key=='y';
    const bool row=ui.phase==SmithUi::Phase::Browse && key>='1' && key<='9';
    const unsigned additional=confirming?3:2;
    if(ui.phase!=SmithUi::Phase::Departure && !(ui.phase==SmithUi::Phase::Lobby && cancel) &&
        (!_encounter->smithCapacity(16+additional,4) || !xeenSmithAuthorityRoom(_inputGeneration,10) ||
        !xeenSmithAuthorityRoom(ui.revision,1) || (row && !xeenSmithAuthorityRoom(_encounter->_smith->operation,1)) ||
        (confirming && ui.mode==SmithUi::Mode::Buy && !xeenSmithAuthorityRoom(_encounter->_smith->reservation,1)))) {
        ui.phase=SmithUi::Phase::Departure;ui.feedback="Further service actions unavailable. Escape: depart.";
        if(ui.revision!=UINT64_MAX) ++ui.revision;return renderEncounter();
    }
    const auto originalPhase=ui.phase;
    bool failed=false,revisionAdvanced=false;
    try {
        if(ui.phase!=SmithUi::Phase::Departure && !ui.feedback.empty()) {ui.feedback.clear();_encounter->advanceSmith();}
        else if(ui.phase==SmithUi::Phase::Lobby) {
            if(key>=InputKey::F1 && key<InputKey::F1+_party.party.size()) {
                ui.member=key-InputKey::F1;_encounter->advanceSmith();
            } else if(key=='b') {
                ui.mode=SmithUi::Mode::Buy;ui.category=XeenInventoryCategory::Weapons;ui.selected=false;
                ui.phase=SmithUi::Phase::Browse;_encounter->_smith->quoted=false;_encounter->advanceSmith();
            } else if(cancel) ui.phase=SmithUi::Phase::Departure;
            else return renderEncounter(false,true);
        } else if(ui.phase==SmithUi::Phase::Browse) {
            if(key>=InputKey::F1 && key<InputKey::F1+_party.party.size()) {
                ui.member=key-InputKey::F1;ui.selected=false;_encounter->_smith->quoted=false;_encounter->advanceSmith();
            } else if(key=='w' || key=='a' || key=='c' || key=='m') {
                ui.category=key=='w'?XeenInventoryCategory::Weapons:key=='a'?XeenInventoryCategory::Armor:
                    key=='c'?XeenInventoryCategory::Accessories:XeenInventoryCategory::Miscellaneous;
                ui.selected=false;_encounter->_smith->quoted=false;_encounter->advanceSmith();
            } else if(key=='b' || key=='f') {
                ui.mode=key=='b'?SmithUi::Mode::Buy:SmithUi::Mode::Repair;ui.selected=false;
                _encounter->_smith->quoted=false;_encounter->advanceSmith();
            } else if(key=='s' || key=='i') ui.feedback=key=='s'?"Sell: not supported yet":"Identify: not supported yet";
            else if(cancel) {ui.phase=SmithUi::Phase::Lobby;ui.selected=false;_encounter->_smith->quoted=false;_encounter->advanceSmith();}
            else if(row) {
                ui.slot=key-'1';ui.selected=true;
                const auto &c=_party.party.member(_party.roster,ui.member);
                const auto &item=ui.mode==SmithUi::Mode::Buy?_party.serviceEconomy->wares[0][0][static_cast<unsigned>(ui.category)][ui.slot]:
                    (*xeenInventoryItems(c,ui.category))[ui.slot];
                if(!item.id) {ui.selected=false;_encounter->advanceSmith();}
                else if(ui.mode==SmithUi::Mode::Buy) {
                    _encounter->quoteSmithBuy(ui.member,ui.category,ui.slot);
                    const auto outcome=_encounter->_smith->purchase->result.outcome;
                    if(_encounter->_smith->quoted) ui.phase=SmithUi::Phase::Confirm;
                    else if(outcome==XeenEquipmentPurchaseOutcome::DestinationFull) ui.feedback=xeenBackpackFull(ui.category,c.name);
                    else ui.feedback="Buy: not supported yet";
                } else if(!(item.state&0x80)) ui.feedback=std::string(xeenDialogText(XeenDialogText::ItemNotBroken));
                else if(ui.category!=XeenInventoryCategory::Armor) ui.feedback="Fix: not supported yet";
                else {
                    _encounter->quoteSmith(ui.member,ui.slot);
                    if(_encounter->_smith->quoted) ui.phase=SmithUi::Phase::Confirm;
                    else ui.feedback="Fix: not supported yet";
                }
            }
        } else if(ui.phase==SmithUi::Phase::Confirm) {
            if(confirming) {
                if(ui.mode==SmithUi::Mode::Buy) _encounter->confirmSmithBuy();else _encounter->confirmSmith();
                const bool shortfall=ui.mode==SmithUi::Mode::Buy?
                    _encounter->_smith->purchase->result.outcome==XeenEquipmentPurchaseOutcome::InsufficientGold:
                    _encounter->_smith->result.outcome==XeenArmorRepairOutcome::InsufficientGold;
                if(shortfall) ui.feedback=xeenNotEnoughGold();
            } else {_encounter->_smith->quoted=false;_encounter->advanceSmith();}
            ui.phase=SmithUi::Phase::Browse;ui.selected=false;
        }
        if(ui.phase==SmithUi::Phase::Departure && (cancel || key==InputKey::Enter)) {
            if(originalPhase!=ui.phase && ui.revision!=UINT64_MAX) {++ui.revision;revisionAdvanced=true;}
            _encounter->departSmith();_smithSettlement=true;return settleSmithEvent();
        }
    } catch(const std::exception &) {
        failed=true;_encounter->journeySavePreimage().check();if(!_encounter->_smith) throw;
        if(_encounter->_smith->quoted) ui.phase=SmithUi::Phase::Confirm;
        if(_encounter->_smith->published && ui.phase!=SmithUi::Phase::Departure) {ui.phase=SmithUi::Phase::Browse;ui.selected=false;}
        // A retained Confirm can be retried with a fresh Y. A published action
        // is never repeated, including a failure in a post-publication hook.
        if(ui.phase!=SmithUi::Phase::Confirm) ui.feedback="Service preparation failed. Escape: depart.";
    }
    if(!revisionAdvanced && (!failed || originalPhase!=ui.phase) && ui.revision!=UINT64_MAX) ++ui.revision;
    if(reportText && !ui.feedback.empty()) reportText(ui.feedback);
    return renderEncounter();
}
IndexedFrame XeenEventFlow::handleTemple(const PlayerAction &action,std::uint64_t input,const IndexedFrame::Presentation &inputFrame) {
    auto &ui=*_smithUi;const auto key=serviceKey(action);
    if(ui.phase==SmithUi::Phase::Preparation) return frameCopy();
    const bool member=key>=InputKey::F1 && key<InputKey::F1+_party.party.size();
    const bool allowed=ui.phase==SmithUi::Phase::Departure?(key==InputKey::Escape || key==InputKey::Enter):
        ui.phase==SmithUi::Phase::Upgrade?key==InputKey::Escape:(member || key==InputKey::Escape || key=='h' || key=='d' || key=='u');
    if((ui.feedback.empty() || ui.phase==SmithUi::Phase::Departure) && (!allowed || (member && key-InputKey::F1==ui.member))) return frameCopy();
    if(_smithSettlement) {_handoffPending=true;return settleSmithEvent();}
    if(!_encounter->consumeSmithFrame(input,inputFrame)) return frameCopy();
    const bool cancel=key==InputKey::Escape;
    const unsigned revisions=key=='h'?2:1;
    if(ui.phase!=SmithUi::Phase::Departure && !(cancel && (ui.phase==SmithUi::Phase::Lobby || ui.phase==SmithUi::Phase::Upgrade)) &&
        (!_encounter->smithCapacity(20,4) || !xeenSmithAuthorityRoom(_inputGeneration,10) || !xeenSmithAuthorityRoom(ui.revision,revisions) ||
        (key=='h' && (!xeenSmithAuthorityRoom(_encounter->_smith->operation,1) ||
        (!_encounter->_smith->paid && !xeenSmithAuthorityRoom(_encounter->_smith->reservation,1)))))) {
        ui.phase=SmithUi::Phase::Departure;ui.feedback="Further Heal actions unavailable. Escape: depart.";
        if(ui.revision!=UINT64_MAX) ++ui.revision;return renderEncounter();
    }
    const auto originalPhase=ui.phase;
    bool failed=false,revisionAdvanced=false;
    try {
        if(ui.phase!=SmithUi::Phase::Departure && !ui.feedback.empty()) {ui.feedback.clear();_encounter->advanceSmith();}
        else if(ui.phase==SmithUi::Phase::Lobby) {
            if(key>=InputKey::F1 && key<InputKey::F1+_party.party.size()) {
                ui.member=key-InputKey::F1;_encounter->_smith->quoted=false;_encounter->advanceSmith();
            } else if(key=='d') ui.feedback="Donation: not supported yet";
            else if(key=='u') {
                if(xeenTempleUncurseCost(_party.party.member(_party.roster,ui.member))) ui.feedback="Uncurse: not supported yet";
            } else if(key=='h') {
                if(!_encounter->_smith->quoted) _encounter->quoteTempleHeal(ui.member);
                if(_encounter->_smith->quoted) ui.phase=_encounter->confirmTempleHeal()?SmithUi::Phase::Lobby:SmithUi::Phase::Upgrade;
                const auto outcome=_encounter->_smith->healResult.outcome;
                if(outcome==XeenTempleHealOutcome::InsufficientGold) ui.feedback=xeenNotEnoughGold();
                else if(outcome==XeenTempleHealOutcome::SupportLimit || outcome==XeenTempleHealOutcome::HpSupportLimit)
                    ui.feedback="Heal: not supported yet at this date or capacity";
            } else if(cancel) ui.phase=SmithUi::Phase::Departure;
        } else if(ui.phase==SmithUi::Phase::Upgrade && cancel) {_encounter->cancelTempleHeal();ui.phase=SmithUi::Phase::Lobby;}
        if(ui.phase==SmithUi::Phase::Departure && (cancel || key==InputKey::Enter)) {
            if(originalPhase!=ui.phase && ui.revision!=UINT64_MAX) {++ui.revision;revisionAdvanced=true;}
            _encounter->departSmith();_smithSettlement=true;return settleSmithEvent();
        }
    } catch(const std::exception &) {
        failed=true;_encounter->journeySavePreimage().check();if(!_encounter->_smith) throw;
        if(_encounter->_smith->healPending) ui.phase=SmithUi::Phase::Upgrade;
        else if(_encounter->_smith->published && ui.phase!=SmithUi::Phase::Departure) ui.phase=SmithUi::Phase::Lobby;
        else if(ui.phase==SmithUi::Phase::Departure) ui.feedback="Temple departure failed. Escape: retry.";
    }
    if(!revisionAdvanced && (!failed || originalPhase!=ui.phase) && ui.revision!=UINT64_MAX) ++ui.revision;
    if(reportText && !ui.feedback.empty()) reportText(ui.feedback);
    return renderEncounter();
}
}
