#include "app/XeenEncounterFlow.h"
#include "app/XeenEventFlow.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenVertigoRoute.h"
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
 if (xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase() &&
     (!_smith->binding || !_smith->binding->matches(*day))) {
  _journeyPreimage->failed=true;
  throw std::logic_error("Complete Smith departure candidate changed");
 }
}
bool XeenEncounterFlow::beginSmith(const std::function<void()> &preflight) {
	const bool temple=xeenJourneyContent(_world.sessionState().journeyContract()).templeRecovery() &&
		_camera.mapId==XeenMapIdentity(28) && _camera.x==15 && _camera.y==28;
	if (!journeyEvent() || _smith || _smithPreparation || _busy || !current(ticket()) || !journeyCapacity() ||
		!xeenJourneyContent(_world.sessionState().journeyContract()).armorRepair() || _camera.mapId!=XeenMapIdentity(28) ||
		!((temple) || (_camera.x==8 && _camera.y==4)) || !_party.encounterContext) return false;
	SmithBusy busy(_busy);
	_journeyPreimage->check();
	// Reserve preparation/admission, two service frames, departure, Event
	// retirement and final presentation, including the inherited guard margin.
	const bool buy=xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase();
	if (!smithCapacity(buy?20:xeenJourneyContent(_world.sessionState().journeyContract()).serviceDays()?10:9,buy?5:3)) return false;
	if (!xeenPrepareSmithDeparture(*_party.encounterContext,_world.sessionState().journeyContract())) return false;
	xeenValidateJourneyParty(_party,_world.sessionState().journeyContract());
	checkSmithBoundary(XeenSmithBoundary::BeforeReservation);
	auto next=std::make_unique<SmithContinuation>();
	next->temple=temple;
    next->legacyDeparture=xeenPrepareSmithDeparture(*_party.encounterContext,_world.sessionState().journeyContract());
    if (xeenJourneyContent(_world.sessionState().journeyContract()).serviceDays()) {
        if (!_party.serviceEconomy || !_world.sessionState().journeyRandom())
            throw std::logic_error("Missing service-day owners");
        next->departure=std::make_unique<XeenServiceDayCandidate>(*_party.encounterContext,
            *_party.serviceEconomy,*_world.sessionState().journeyRandom(),_world.sessionState().journeyContract());
		next->reservation=1;
    }
	checkSmithBoundary(XeenSmithBoundary::AfterReservation);
 checkSmithBoundary(XeenSmithBoundary::BeforeAdmission);
	// Event remains exclusive throughout resource and first-frame preparation.
	try { XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);preflight();_journeyPreimage->check(); }
	catch (...) { _journeyPreimage->check();throw; }
    if (next->departure) {
        // Retain exclusive Event work with no service debt until all draws finish.
        _smithPreparation=std::move(next);
        return true;
    }
	next->lease=_boundary.hold(XeenCombatBoundary::Work::Service);
	_smith=std::move(next);
	_world._sessionState._journeyActivity=XeenJourneyActivity::Service;
	++_world._sessionState._journeyGeneration;++_generation;
	_journeyPreimage->adoptJourneyCoordination();
 checkSmithBoundary(XeenSmithBoundary::AfterAdmission);
	return true;
}
bool XeenEncounterFlow::serviceSmithPreparation() {
    if (!_smithPreparation || _smith || _busy || !journeyEvent() ||
        !current(ticket()) || !journeyCapacity()) return false;
    SmithBusy busy(_busy);
    _journeyPreimage->check();
    const bool buy=xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase();
    if (!smithCapacity(buy?17:9,buy?5:3)) throw std::overflow_error("Smith admission authority exhausted");
    auto &day=*_smithPreparation->departure;
    if (!day.service(64,[&] { _journeyPreimage->check(); },
        [&] { checkSmithBoundary(XeenSmithBoundary::StockComplete); })) return false;
	checkSmithBoundary(XeenSmithBoundary::BankPrepared);
    if (!day.complete() || !(day.beforeContext()==*_party.encounterContext) ||
        day.beforeEconomy()!=*_party.serviceEconomy ||
        day.beforeRandom()!=*_world.sessionState().journeyRandom())
        throw std::logic_error("Service-day reservation changed");
    xeenValidateCurrentServiceEconomy(day.economy(),_world.sessionState().journeyContract());
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
	xeenValidateJourneyParty(_party,_world.sessionState().journeyContract());
	if (xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase()) {
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
	if (xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase()) advanceSmith();
 checkSmithBoundary(XeenSmithBoundary::Quote);
}
void XeenEncounterFlow::confirmSmith() {
	if (!_smith || !_smith->quoted || _busy || _smith->frame || !journeyCapacity())
		throw std::logic_error("Smith quote authority unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();
	const bool buy=xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase();
	if (!smithCapacity(buy?19:10,buy?4:2)) throw std::overflow_error("Repair would consume its result and owed departure authority");
	if (buy) {
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
	if (buy) checkSmithReservation();
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
     !xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase() ||
     !smithCapacity(18,4) || !xeenSmithAuthorityRoom(_smith->operation,1))
  throw std::logic_error("Smith Buy selection authority unavailable");
 SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
 auto quote=std::make_unique<XeenEquipmentPurchaseCandidate>();
 quote->result=xeenQuoteEquipmentPurchase(_party,member,category,slot,_world.sessionState().journeyContract());
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
 const auto currentQuote=xeenQuoteEquipmentPurchase(_party,quoted.member,quoted.category,quoted.offerSlot,
     _world.sessionState().journeyContract(),quoted.side,quoted.shop);
 if (_smith->quoteOperation!=_smith->operation || _smith->quoteReservation!=_smith->reservation ||
     quoted.quotedOperation!=_smith->operation || quoted.quotedReservation!=_smith->reservation ||
     !samePurchaseQuote(quoted,currentQuote) || _smith->purchase->economyBefore!=*_party.serviceEconomy ||
     _smith->purchase->economyAfter!=*_party.serviceEconomy) {
  _journeyPreimage->failed=true;
  throw std::logic_error("Smith Buy quote preimage changed");
 }
 auto candidate=std::make_unique<XeenEquipmentPurchaseCandidate>(xeenPrepareEquipmentPurchase(_party,
     quoted.member,quoted.category,quoted.offerSlot,_world.sessionState().journeyContract(),quoted.side,quoted.shop));
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
	xeenValidateJourneyParty(_party,_world.sessionState().journeyContract());
	const auto owner=_party.party.activeRosterIds()[member];
	auto quote=xeenQuoteTempleHeal(_party.roster.at(owner),_party.monsterTreasure->gold,
		*_party.encounterContext);
	if(quote.outcome==XeenTempleHealOutcome::Quoted) {
		quote=xeenPrepareTempleHeal(_party,owner,*_party.encounterContext,14).result;
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
	if(!_smith->paid && !xeenPrepareTemplePaidDeparture(*_party.encounterContext,14)) {
		_smith->healResult.outcome=XeenTempleHealOutcome::SupportLimit;
		_smith->quoted=false;advanceSmith();return true;
	}
	auto delta=std::make_unique<XeenTempleHealCandidate>(
		xeenPrepareTempleHeal(_party,_smith->owner,*_party.encounterContext,14));
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
		if (xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase()) checkSmithReservation();
		const auto after=_smith->legacyDeparture;
		if (!after) throw std::logic_error("Owed smith departure is no longer canonical");
		const auto *day=_smith->departure.get();
		if (day && (!day->complete() || !(day->beforeContext()==*_party.encounterContext) ||
			day->beforeEconomy()!=*_party.serviceEconomy || day->beforeRandom()!=*_world.sessionState().journeyRandom())) {
			_journeyPreimage->failed=true;
			throw std::logic_error("Owed service-day preimage changed");
		}
		const XeenMutableOptional<XeenGameplayContext> endingContext(day?day->context():*after);
		const XeenMutableOptional<XeenServiceEconomy> endingEconomy=day?
			XeenMutableOptional<XeenServiceEconomy>(day->economy()):_party.serviceEconomy;
		const XeenMutableOptional<XeenJourneyRandomState> endingRandom=day?
			XeenMutableOptional<XeenJourneyRandomState>(day->continuation()):_world._sessionState._journeyRandom;
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);prepared->context=*after;
		if (day) {
			prepared->context=day->context();prepared->economy=day->economy();
			prepared->s._journeyRandom=day->continuation();
			xeenValidateCurrentServiceEconomy(day->economy(),_world.sessionState().journeyContract());
		}
		_journeyPreimage->check();
		checkSmithBoundary(XeenSmithBoundary::BeforeDeparture);
		if (xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase()) checkSmithReservation();
		static_assert(std::is_nothrow_copy_assignable_v<decltype(_party.encounterContext)> &&
			std::is_nothrow_copy_assignable_v<decltype(_party.serviceEconomy)> &&
			std::is_nothrow_copy_assignable_v<decltype(_world._sessionState._journeyRandom)>);
        _party.encounterContext=endingContext;
        if (day) {
            _party.serviceEconomy=endingEconomy;
            _world._sessionState._journeyRandom=endingRandom;
        }
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
		const bool temple=xeenJourneyContent(_world.sessionState().journeyContract()).templeRecovery() &&
			_camera.mapId==XeenMapIdentity(28) && _camera.x==15 && _camera.y==28;
		const unsigned inputFrames=xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase()?10:
			xeenJourneyContent(_world.sessionState().journeyContract()).serviceDays()?4:3;
		if (!xeenSmithAuthorityRoom(_inputGeneration,inputFrames))
			throw std::overflow_error("Smith input authority exhausted before admission");
		_encounter->_smithBoundary=[this](XeenSmithBoundary stage) { if(smithBoundary)smithBoundary(stage); };
  const bool admitted=_encounter->beginSmith([&] {
			if (temple ? !drawTempleArt : !drawSmithArt)
				throw std::runtime_error("Service artwork provider is unavailable");
			const auto events=_events.scriptForMap(28).file();
			const auto mainland=_events.scriptForMap(23).file();
			try { xeenValidateVertigoRoute(mainland,events,_world.sessionState().journeyContract()); }
            catch (const std::invalid_argument &) { _encounter->journeySavePreimage().failed=true;throw; }
			const auto text=_events.textForMap(28);
			_encounter->journeySavePreimage().admitVertigoText(text);
			SmithUi ui;ui.catalog=_catalog;ui.title=text.strings.at(temple?37:33);ui.art=_frame;
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
				xeenJourneyContent(_world.sessionState().journeyContract()).serviceDays() ?
				"Ironworks unavailable: departure exceeds the supported year." :
                "Ironworks unavailable: departure would require unsupported restocking.";
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
std::string XeenEventFlow::smithText() const {
	const auto &ui=*_smithUi;
	const auto owner=_party.party.activeRosterIds().at(ui.member);
	const auto &character=_party.roster.at(owner);
	const auto gold=_party.monsterTreasure->gold;
	if(ui.mode==SmithUi::Mode::Heal) {
		std::ostringstream text;
		text<<ui.title<<"\n"<<character.name<<"  Gold "<<gold<<"\n";
		if(ui.phase==SmithUi::Phase::Preparation)
			text<<"Preparing complete departure. Please wait.";
		else if(ui.phase==SmithUi::Phase::Upgrade)
			text<<"Preparing paid departure. Escape: cancel Heal and keep one-day exit.";
		else if(ui.phase==SmithUi::Phase::Departure)
			text<<"Departure settlement pending.\nEnter: retry departure";
		else if(ui.phase==SmithUi::Phase::Lobby) {
			const auto quote=xeenQuoteTempleHeal(character,gold,*_party.encounterContext);
			text<<"HP "<<character.currentHp<<" / "<<quote.maxHpBefore<<"  SP "<<character.currentSp<<"\n";
			text<<"Conditions: ";
			bool any=false;
			for(unsigned i=1;i<=15;++i)if(character.conditions[i]) {
				if(any)text<<", ";any=true;
				text<<xeenConditionName(static_cast<XeenCondition>(i))<<' '<<unsigned(character.conditions[i]);
			}
			if(!any)text<<"Good";
			text<<"\nQuote "<<quote.price<<" gold  Exit "<<(_encounter->_smith && _encounter->_smith->paid?2:1)<<" day(s)"
				<<"\nF1-F6: recipient  Enter: Heal quote\nEscape: depart";
		} else {
			auto r=_encounter->_smith->healResult;
			const bool refused=ui.phase==SmithUi::Phase::Result && !_encounter->_smith->published;
			if(refused) {
				// A refused Result reports no applied change, not the quote's projection.
				r.hpAfter=r.hpBefore;r.maxHpAfter=r.maxHpBefore;r.spAfter=r.spBefore;
				r.goldAfter=r.goldBefore;r.price=0;
			}
			text<<"HP "<<r.hpBefore<<" -> "<<r.hpAfter
				<<(refused?" (max ":" (healthy max ")<<r.maxHpAfter<<")"
				<<"\nSP "<<r.spBefore<<" -> "<<r.spAfter<<"  Gold "<<r.goldBefore;
			if(ui.phase==SmithUi::Phase::Quote)
				text<<"\nHeal price "<<r.price<<"  After "<<r.goldBefore-r.price
					<<"\nPaid exit: two days\nEnter: confirm  Escape: cancel";
			else {
				switch(r.outcome) {
				case XeenTempleHealOutcome::Healed:text<<"\nHealed. Paid "<<r.price<<" gold.";break;
				case XeenTempleHealOutcome::NoCharge:text<<"\nNo Heal charge; no change.";break;
				case XeenTempleHealOutcome::InsufficientGold:text<<"\nInsufficient carried gold.";break;
				case XeenTempleHealOutcome::SupportLimit:text<<"\nPaid departure outside supported date or capacity.";break;
				case XeenTempleHealOutcome::HpSupportLimit:text<<"\nHeal HP outside supported range; no change.";break;
				default:text<<"\nHeal unavailable.";break;
				}
				text<<"\nGold now "<<r.goldAfter<<"  Exit "<<(_encounter->_smith->paid?2:1)<<" day(s)"
					<<"\nEnter: return to Temple menu";
			}
		}
		if(!ui.feedback.empty())text<<"\n"<<ui.feedback;
		return text.str();
	}
	std::ostringstream text;
	text<<ui.title<<"\n"<<character.name<<"  Gold "<<gold;
	if (ui.mode==SmithUi::Mode::Buy && ui.phase==SmithUi::Phase::Browse) {
		static constexpr const char *categories[]={"Weapons","Armor","Accessories","Miscellaneous"};
		text<<"  Buy "<<categories[static_cast<unsigned>(ui.category)];
	}
	text<<"\n";
	if (ui.phase==SmithUi::Phase::Preparation) {
		text<<"Preparing one-day departure.\nPlease wait.";
	} else if (ui.phase==SmithUi::Phase::Lobby) {
		if (xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase())
			text<<"B: Buy  R/Enter: Armor repair\nF1-F6: recipient\nEscape: depart (costs one day)";
		else text<<"Armor repair only\nEnter: Repair armor   F1-F6: owner\nEscape: depart (costs one day)";
	} else if (ui.mode==SmithUi::Mode::Buy && ui.phase==SmithUi::Phase::Browse) {
		const auto &stock=_party.serviceEconomy->wares[0][0][static_cast<unsigned>(ui.category)];
		for (unsigned i=0;i<9;++i) {
			const auto &item=stock[i];
			text<<(ui.selected && i==ui.slot?">":" ")<<(i+1)<<" ";
			if (!item.id) text<<"Empty";
			else {
				text<<ui.catalog.describe(ui.category,item).displayName.substr(0,22);
				if (const auto price=xeenEquipmentPurchasePrice(ui.category,item)) text<<"  "<<*price<<" gold";
				else text<<"  unsupported";
			}
			text<<"\n";
		}
		if (ui.selected) {
			const auto &item=stock[ui.slot];
			text<<"Row "<<(ui.slot+1)<<" M/ID/S/F "<<unsigned(item.material)<<'/'<<unsigned(item.id)<<'/'<<unsigned(item.state)<<'/'<<unsigned(item.frame);
			if (item.id && !xeenSupportedEquipmentOffer(0,0,ui.category,item)) text<<" Purchase unsupported";
			text<<"\n";
		} else text<<"Choose a physical row.\n";
		text<<"Left/Right: category  F1-F6: recipient\n1-9: row  Enter: quote  Escape: lobby";
	} else if (ui.phase==SmithUi::Phase::Browse) {
		for (unsigned i=0;i<9;++i) {
			const auto d=ui.catalog.describe(XeenInventoryCategory::Armor,character.armor[i]);
			text<<(i==ui.slot?">":" ")<<(i+1)<<" "<<d.displayName.substr(0,24)
				<<(d.broken?" B":"")<<(d.cursed?" C":"")<<(d.equipped?" E":"")<<"\n";
		}
		text<<(ui.feedback.empty()?"B broken C cursed E equipped":ui.feedback)
			<<"\n1-9: slot  F1-F6: owner\nEnter: quote  Escape: lobby";
	} else if (ui.phase==SmithUi::Phase::Departure) {
		text<<"Departure settlement pending.\nEnter: retry departure";
	} else if (ui.mode==SmithUi::Mode::Buy) {
		const auto &r=_encounter->_smith->purchase->result;
		static constexpr const char *categories[]={"Weapons","Armor","Accessories","Miscellaneous"};
		text<<categories[static_cast<unsigned>(r.category)]<<" row "<<unsigned(r.offerSlot+1)<<": "
			<<ui.catalog.describe(r.category,r.offer).displayName.substr(0,35)<<"\n";
		text<<"M/ID/S/F "<<unsigned(r.offer.material)<<'/'<<unsigned(r.offer.id)<<'/'<<unsigned(r.offer.state)<<'/'<<unsigned(r.offer.frame)<<"\n";
		if (ui.phase==SmithUi::Phase::Quote) {
			text<<"Buy price: "<<r.price<<" gold\nCarried gold: "<<r.goldBefore<<"\n";
			if (r.shortfall) text<<"Shortfall: "<<r.shortfall<<" gold\n";
			else text<<"Gold after: "<<r.goldAfter<<"\n";
			text<<"Enter/Yes: buy  Escape/No: cancel";
		} else {
			switch (r.outcome) {
			case XeenEquipmentPurchaseOutcome::Purchased:
				text<<"Purchased unequipped. Paid "<<r.price<<" gold.\nRecipient slot "<<unsigned(r.recipientSlot+1)<<"; stock row removed.";break;
			case XeenEquipmentPurchaseOutcome::Empty:text<<"Empty stock row. No purchase.";break;
			case XeenEquipmentPurchaseOutcome::DestinationFull:text<<"Recipient category tail is occupied. No purchase.";break;
			case XeenEquipmentPurchaseOutcome::InsufficientGold:text<<"Not enough carried gold. No purchase.";break;
			default:text<<"Purchase unsupported. No purchase.";break;
			}
			text<<"\nGold: "<<r.goldBefore<<" -> "<<r.goldAfter<<"\nEnter/Escape: return to Buy";
		}
	} else {
		const auto &r=_encounter->_smith->result;
		text<<"Armor slot "<<(ui.slot+1)<<": "<<ui.catalog.describe(XeenInventoryCategory::Armor,r.before).displayName<<"\n";
		if (ui.phase==SmithUi::Phase::Quote) {
			text<<"Repair price: "<<r.price<<" gold\n";
			if (gold>=r.price) text<<"Gold after: "<<(gold-r.price)<<"\n";
			else text<<"Shortfall: "<<(r.price-gold)<<" gold\n";
			text<<"Enter/Yes: confirm  Escape/No: cancel";
		} else {
			switch (r.outcome) {
			case XeenArmorRepairOutcome::Repaired: text<<"Repaired. Paid "<<r.price<<" gold.";break;
			case XeenArmorRepairOutcome::Empty: text<<"Empty armor slot.";break;
			case XeenArmorRepairOutcome::Intact: text<<"Armor is not broken.";break;
			case XeenArmorRepairOutcome::InsufficientGold: text<<"Not enough carried gold. No repair.";break;
			default: text<<"Unsupported armor. No repair.";break;
			}
			text<<"\nGold: "<<r.goldBefore<<" -> "<<r.goldAfter<<"\nEnter: return to armor";
		}
	}
	if (!ui.feedback.empty() && ui.phase!=SmithUi::Phase::Browse) text<<"\n"<<ui.feedback;
 return text.str();
}
IndexedFrame XeenEventFlow::drawSmith(const IndexedFrame &world) const {
	const auto &ui=*_smithUi;
 XeenTextRenderOptions options;
	options.bounds={9,9,222,157};options.windowBounds={8,8,223,159};
	options.x=10;options.y=10;options.size=XeenFontSize::Reduced;
	options.paginate=true;options.drawWindow=true;
	if (ui.phase==SmithUi::Phase::Lobby && ui.mode!=SmithUi::Mode::Heal) {
		options.bounds={9,93,222,157};options.windowBounds={8,91,223,159};options.y=93;
	}
	if (ui.mode==SmithUi::Mode::Heal) {
		options.bounds={9,9,309,190};options.windowBounds={8,8,311,192};options.y=10;
	} else if (ui.mode==SmithUi::Mode::Buy && ui.phase!=SmithUi::Phase::Lobby) {
		options.bounds={9,9,309,157};options.windowBounds={8,8,311,159};
	}
	auto background=world;
 if (ui.phase==SmithUi::Phase::Lobby)
  for(int y=8;y<140;++y)
   std::copy_n(ui.art.pixels.data()+y*320+8,215,background.pixels.data()+y*320+8);
 auto rendered=XeenTextRenderer(_inventoryFont).render(background,smithText(),options);
	if (rendered.pages.size()!=1) throw std::runtime_error("Ironworks panel did not fit");
	return std::move(rendered.pages.front());
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
	if(_smithUi->mode==SmithUi::Mode::Heal)return handleTemple(action,input,inputFrame);
	if (_smithUi->phase==SmithUi::Phase::Preparation) return frameCopy();
	const bool successor=xeenJourneyContent(_world.sessionState().journeyContract()).equipmentPurchase();
	if (successor) {
		auto &ui=*_smithUi;
		const bool confirm=std::holds_alternative<AcknowledgeAction>(action) || std::holds_alternative<YesAction>(action);
		const bool cancel=std::holds_alternative<CancelInteractionAction>(action) || std::holds_alternative<NoAction>(action);
		const bool buy=std::holds_alternative<BlockAction>(action),repair=std::holds_alternative<RevisitCompletedAction>(action);
		const auto *member=std::get_if<SelectMemberAction>(&action);
		const auto *slot=std::get_if<SelectInventorySlotAction>(&action);
		const auto *navigation=std::get_if<NavigationAction>(&action);
		const bool chooseMember=member && member->partyIndex<_party.party.size() && member->partyIndex!=ui.member;
		const bool chooseSlot=slot && slot->slot<9 && (slot->slot!=ui.slot || (ui.mode==SmithUi::Mode::Buy && !ui.selected));
		const bool category=navigation && (*navigation==NavigationAction::TurnLeft || *navigation==NavigationAction::TurnRight);
		const bool allowed=(ui.phase==SmithUi::Phase::Lobby && (confirm || cancel || buy || repair || chooseMember)) ||
			(ui.phase==SmithUi::Phase::Browse && (cancel || chooseMember || chooseSlot ||
				(ui.mode==SmithUi::Mode::Buy && category) || (confirm && (ui.mode==SmithUi::Mode::Repair || ui.selected)))) ||
			((ui.phase==SmithUi::Phase::Quote || ui.phase==SmithUi::Phase::Result || ui.phase==SmithUi::Phase::Departure) && (confirm || cancel));
		if (!allowed) return frameCopy();
		if (_smithSettlement) {_handoffPending=true;return settleSmithEvent();}
		if (!_encounter->consumeSmithFrame(input,inputFrame)) return frameCopy();
		const bool confirming=ui.phase==SmithUi::Phase::Quote && confirm;
		const bool quoting=ui.phase==SmithUi::Phase::Browse && confirm;
		const unsigned additional=confirming?3:2;
		if (ui.phase!=SmithUi::Phase::Departure && !cancel &&
			(!_encounter->smithCapacity(16+additional,4) || !xeenSmithAuthorityRoom(_inputGeneration,10) ||
			 !xeenSmithAuthorityRoom(ui.revision,1) ||
			 (quoting && !xeenSmithAuthorityRoom(_encounter->_smith->operation,1)) ||
			 (confirming && ui.mode==SmithUi::Mode::Buy && !xeenSmithAuthorityRoom(_encounter->_smith->reservation,1)))) {
			ui.phase=SmithUi::Phase::Departure;
			ui.feedback="Further service actions unavailable. Departure remains reserved.";
			if (ui.revision!=UINT64_MAX) ++ui.revision;
			return renderEncounter();
		}
		// Cancellation/result acknowledgment also preserve the full suffix; only
		// a lobby departure may use the already reserved mandatory authority.
		if (ui.phase!=SmithUi::Phase::Departure && !(ui.phase==SmithUi::Phase::Lobby && cancel) &&
			(!_encounter->smithCapacity(16+additional,4) || !xeenSmithAuthorityRoom(_inputGeneration,10) || !xeenSmithAuthorityRoom(ui.revision,1))) {
			ui.phase=SmithUi::Phase::Departure;
			ui.feedback="Departure remains reserved.";
			if (ui.revision!=UINT64_MAX) ++ui.revision;
			return renderEncounter();
		}
		ui.feedback.clear();
		const auto originalPhase=ui.phase;
		const auto originalOperation=_encounter->_smith->operation;
		bool failed=false,revisionAdvanced=false;
		try {
			if (ui.phase==SmithUi::Phase::Lobby) {
				if (chooseMember) {ui.member=member->partyIndex;_encounter->advanceSmith();}
				else if (buy || repair || confirm) {
					ui.mode=buy?SmithUi::Mode::Buy:SmithUi::Mode::Repair;
					ui.category=XeenInventoryCategory::Weapons;ui.slot=0;ui.selected=false;
					ui.phase=SmithUi::Phase::Browse;_encounter->_smith->quoted=false;_encounter->advanceSmith();
				} else if (cancel) ui.phase=SmithUi::Phase::Departure;
			} else if (ui.phase==SmithUi::Phase::Browse) {
				if (chooseMember) {ui.member=member->partyIndex;_encounter->_smith->quoted=false;_encounter->advanceSmith();}
				else if (chooseSlot) {ui.slot=slot->slot;ui.selected=true;_encounter->_smith->quoted=false;_encounter->advanceSmith();}
				else if (category && ui.mode==SmithUi::Mode::Buy) {
					const unsigned next=(static_cast<unsigned>(ui.category)+(*navigation==NavigationAction::TurnLeft?3:1))%4;
					ui.category=static_cast<XeenInventoryCategory>(next);ui.selected=false;
					_encounter->_smith->quoted=false;_encounter->advanceSmith();
				} else if (cancel) {ui.phase=SmithUi::Phase::Lobby;_encounter->_smith->quoted=false;_encounter->advanceSmith();}
				else if (confirm) {
					if (ui.mode==SmithUi::Mode::Buy) _encounter->quoteSmithBuy(ui.member,ui.category,ui.slot);
					else _encounter->quoteSmith(ui.member,ui.slot);
					ui.phase=_encounter->_smith->quoted?SmithUi::Phase::Quote:SmithUi::Phase::Result;
				}
			} else if (ui.phase==SmithUi::Phase::Quote) {
				if (cancel) {ui.phase=SmithUi::Phase::Browse;_encounter->_smith->quoted=false;_encounter->advanceSmith();}
				else {
					if (ui.mode==SmithUi::Mode::Buy) _encounter->confirmSmithBuy();else _encounter->confirmSmith();
					ui.phase=SmithUi::Phase::Result;
				}
			} else if (ui.phase==SmithUi::Phase::Result) {
				if (ui.mode==SmithUi::Mode::Buy && _encounter->_smith->published) ui.selected=false;
				ui.phase=SmithUi::Phase::Browse;_encounter->advanceSmith();
			}
			if (ui.phase==SmithUi::Phase::Departure && (confirm || cancel)) {
				if (originalPhase!=ui.phase && ui.revision!=UINT64_MAX) {++ui.revision;revisionAdvanced=true;}
				_encounter->departSmith();_smithSettlement=true;return settleSmithEvent();
			}
		} catch (const std::exception &) {
			failed=true;_encounter->journeySavePreimage().check();
			if (!_encounter->_smith) throw;
			if (ui.phase==SmithUi::Phase::Browse && _encounter->_smith->operation!=originalOperation)
				ui.phase=_encounter->_smith->quoted?SmithUi::Phase::Quote:SmithUi::Phase::Result;
			if (ui.phase==SmithUi::Phase::Quote && _encounter->_smith->published) ui.phase=SmithUi::Phase::Result;
			ui.feedback="Preparation failed. Enter retries.";
		}
		if (!revisionAdvanced && (!failed || originalPhase!=ui.phase) && ui.revision!=UINT64_MAX) ++ui.revision;
		return renderEncounter();
	}
	if (_smithSettlement) {
		if (std::holds_alternative<AcknowledgeAction>(action) || std::holds_alternative<YesAction>(action) ||
			std::holds_alternative<CancelInteractionAction>(action) || std::holds_alternative<NoAction>(action))
			return settleSmithEvent();
		return renderEncounter();
	}
	const bool confirmation=std::holds_alternative<AcknowledgeAction>(action) || std::holds_alternative<YesAction>(action);
	const bool cancellation=std::holds_alternative<CancelInteractionAction>(action) || std::holds_alternative<NoAction>(action);
	// A wrong key cannot consume the last reserved departure frame and force
	// another presentation merely to repeat the same settlement instruction.
	if (_smithUi->phase==SmithUi::Phase::Departure && !confirmation && !cancellation) return frameCopy();
	if (!_encounter->consumeSmithFrame(input,responseFrame())) return frameCopy();
	auto &ui=*_smithUi;
 ui.feedback.clear();
	const unsigned remaining=ui.phase==SmithUi::Phase::Quote && confirmation?10:9;
	if (ui.phase!=SmithUi::Phase::Departure &&
		(!_encounter->smithCapacity(remaining,2) || !xeenSmithAuthorityRoom(_inputGeneration,3))) {
		ui.phase=SmithUi::Phase::Departure;
		ui.feedback="Further repairs unavailable. Departure remains reserved.";
		return renderEncounter();
	}
 try {
	const bool confirm=std::holds_alternative<AcknowledgeAction>(action) || std::holds_alternative<YesAction>(action);
	const bool cancel=std::holds_alternative<CancelInteractionAction>(action) || std::holds_alternative<NoAction>(action);
	if (ui.phase==SmithUi::Phase::Lobby || ui.phase==SmithUi::Phase::Browse) {
		if (const auto *member=std::get_if<SelectMemberAction>(&action)) {
			if (member->partyIndex<_party.party.size()) ui.member=member->partyIndex;
		}
	}
	if (ui.phase==SmithUi::Phase::Lobby) {
		if (confirm) ui.phase=SmithUi::Phase::Browse;
		else if (cancel) ui.phase=SmithUi::Phase::Departure;
	} else if (ui.phase==SmithUi::Phase::Browse) {
		if (const auto *slot=std::get_if<SelectInventorySlotAction>(&action)) {if(slot->slot<9)ui.slot=slot->slot;}
		if (cancel) ui.phase=SmithUi::Phase::Lobby;
		else if (confirm) {
			_encounter->quoteSmith(ui.member,ui.slot);
			ui.phase=_encounter->_smith->quoted?SmithUi::Phase::Quote:SmithUi::Phase::Result;
		}
	} else if (ui.phase==SmithUi::Phase::Quote) {
		if (cancel) {_encounter->_smith->quoted=false;_encounter->_smith->result.outcome=XeenArmorRepairOutcome::Cancelled;ui.phase=SmithUi::Phase::Browse;}
		else if (confirm) {_encounter->confirmSmith();ui.phase=SmithUi::Phase::Result;}
	} else if (ui.phase==SmithUi::Phase::Result) {
		if(confirm || cancel)ui.phase=SmithUi::Phase::Browse;
	}
	if (ui.phase==SmithUi::Phase::Departure && (confirm || cancel)) {
		_encounter->departSmith();
		_smithSettlement=true;
		return settleSmithEvent();
	}
 } catch (const std::exception &) {
  _encounter->journeySavePreimage().check();
  if (!_encounter->_smith) throw;
  if (ui.phase==SmithUi::Phase::Quote && !_encounter->_smith->quoted)
   ui.phase=SmithUi::Phase::Result;
  ui.feedback=ui.phase==SmithUi::Phase::Browse?"Quote failed; Enter retries.":"Preparation failed; retry this phase.";
 }
 return renderEncounter();
}
IndexedFrame XeenEventFlow::handleTemple(const PlayerAction &action,std::uint64_t input,
		const IndexedFrame::Presentation &inputFrame) {
	auto &ui=*_smithUi;
	if(ui.phase==SmithUi::Phase::Preparation)return frameCopy();
	const bool confirm=std::holds_alternative<AcknowledgeAction>(action) || std::holds_alternative<YesAction>(action);
	const bool cancel=std::holds_alternative<CancelInteractionAction>(action) || std::holds_alternative<NoAction>(action);
	const auto *member=std::get_if<SelectMemberAction>(&action);
	const bool chooseMember=member && ui.phase==SmithUi::Phase::Lobby &&
		member->partyIndex<_party.party.size() && member->partyIndex!=ui.member;
	const bool allowed=chooseMember ||
		(ui.phase==SmithUi::Phase::Lobby && (confirm || cancel)) ||
		(ui.phase==SmithUi::Phase::Quote && (confirm || cancel)) ||
		(ui.phase==SmithUi::Phase::Upgrade && cancel) ||
		(ui.phase==SmithUi::Phase::Result && confirm) ||
		(ui.phase==SmithUi::Phase::Departure && (confirm || cancel));
	if(!allowed)return frameCopy();
	if(_smithSettlement) {_handoffPending=true;return settleSmithEvent();}
	if(!_encounter->consumeSmithFrame(input,inputFrame))return frameCopy();
	const unsigned revisions=ui.phase==SmithUi::Phase::Quote && confirm?2:1;
	if(ui.phase!=SmithUi::Phase::Departure && !(cancel &&
		(ui.phase==SmithUi::Phase::Lobby || ui.phase==SmithUi::Phase::Upgrade)) &&
		(!_encounter->smithCapacity(19,4) || !xeenSmithAuthorityRoom(_inputGeneration,10) ||
		 !xeenSmithAuthorityRoom(ui.revision,revisions) ||
		 (ui.phase==SmithUi::Phase::Lobby && confirm && !xeenSmithAuthorityRoom(_encounter->_smith->operation,1)) ||
		 (ui.phase==SmithUi::Phase::Quote && confirm && !_encounter->_smith->paid &&
		  !xeenSmithAuthorityRoom(_encounter->_smith->reservation,1)))) {
		ui.phase=SmithUi::Phase::Departure;
		ui.feedback="Further Heal actions unavailable. Departure remains reserved.";
		if(ui.revision!=UINT64_MAX)++ui.revision;
		return renderEncounter();
	}
	ui.feedback.clear();
	const auto originalPhase=ui.phase;
	bool failed=false,revisionAdvanced=false;
	try {
		if(ui.phase==SmithUi::Phase::Lobby) {
			if(chooseMember) {ui.member=member->partyIndex;_encounter->advanceSmith();}
			else if(confirm) {
				_encounter->quoteTempleHeal(ui.member);
				ui.phase=_encounter->_smith->quoted?SmithUi::Phase::Quote:SmithUi::Phase::Result;
			} else if(cancel)ui.phase=SmithUi::Phase::Departure;
		} else if(ui.phase==SmithUi::Phase::Quote) {
			if(cancel) {_encounter->_smith->quoted=false;ui.phase=SmithUi::Phase::Lobby;_encounter->advanceSmith();}
			else ui.phase=_encounter->confirmTempleHeal()?SmithUi::Phase::Result:SmithUi::Phase::Upgrade;
		} else if(ui.phase==SmithUi::Phase::Upgrade) {
			_encounter->cancelTempleHeal();ui.phase=SmithUi::Phase::Lobby;
		} else if(ui.phase==SmithUi::Phase::Result) {
			ui.phase=SmithUi::Phase::Lobby;_encounter->advanceSmith();
		}
		if(ui.phase==SmithUi::Phase::Departure && (confirm || cancel)) {
			if(originalPhase!=ui.phase && ui.revision!=UINT64_MAX) {++ui.revision;revisionAdvanced=true;}
			_encounter->departSmith();_smithSettlement=true;return settleSmithEvent();
		}
	} catch(const std::exception &) {
		failed=true;
		_encounter->journeySavePreimage().check();
		if(!_encounter->_smith)throw;
		if(_encounter->_smith->healPending)ui.phase=SmithUi::Phase::Upgrade;
		else if(ui.phase==SmithUi::Phase::Quote && !_encounter->_smith->quoted)ui.phase=SmithUi::Phase::Result;
		ui.feedback="Temple preparation failed; retry or depart.";
	}
	if(!revisionAdvanced && (!failed || originalPhase!=ui.phase) && ui.revision!=UINT64_MAX)++ui.revision;
	return renderEncounter();
}
}
