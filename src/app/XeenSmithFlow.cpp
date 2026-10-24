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
#include "games/xeen/XeenFoodBank.h"

namespace mmodern {
std::string XeenEventFlow::serviceNotEnoughGold() {
    _encounter->fmEffect(21);return xeenNotEnoughGold(dosText());
}
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
const XeenTempleSurcharges &XeenEncounterFlow::templePrices(
        const XeenTempleSurcharges *settledRow,std::uint64_t settledRevision) const {
    const auto *visit=_smith ? _smith.get() : _smithPreparation.get();
    if(!visit) {
        if(settledRow && journeyEvent() && _smithEventSettlement) {
            _program.templePrices.checkVisit(*settledRow,settledRevision);
            return *settledRow;
        }
        throw std::logic_error("Temple prices require a prepared visit or pending settlement");
    }
    if(!visit->temple)throw std::logic_error("Temple prices require a Temple visit");
    if(_smith)_program.templePrices.checkVisit(visit->templePrices,visit->templePriceRevision);
    return visit->templePrices;
}
void XeenEncounterFlow::checkSmithBoundary(XeenSmithBoundary boundary) {
 const auto retained=_journeyPreimage;
 XeenRestoreGuard::LightingObservation lighting(*retained);
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
 if (!day || !day->reservationReady() || !_party.encounterContext || !_party.serviceEconomy ||
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
XeenSimpleServiceOutcome XeenEncounterFlow::simpleService(std::size_t member,char action) {
    if(!_smith || _smith->departed || _smith->frame || _busy || !smithCapacity(20,4) ||
        !xeenSmithAuthorityRoom(_smith->reservation,1) || member>=_party.party.size() ||
        !((_smith->serviceAction==3 && (action=='d' || action=='t')) ||
          (_smith->temple && (action=='D' || action=='u'))))
        throw std::logic_error("Simple service authority unavailable");
    SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
    auto &visit=*_smith;visit.published=false;
    const auto owner=_party.party.activeRosterIds()[member];const auto &character=_party.roster.at(owner);
    auto afterCharacter=character;auto afterContext=*_party.encounterContext;
    auto afterGold=std::uint32_t(_party.purse->gold);
    auto afterRandom=*_world.sessionState().journeyRandom();
    auto counter=visit.donationCounter;bool blessed=visit.templeBlessed,bought=visit.boughtDrink;
    std::optional<std::uint8_t> tip;
    bool burp=false;
    std::unique_ptr<XeenServiceDayCandidate> departure;
    if(action=='d' || action=='t') {
        if(!character.canAct())return XeenSimpleServiceOutcome::NoAction;
        if(action=='t') {
            if(!bought)return XeenSimpleServiceOutcome::HaveDrink;
            // DOS consumes eligibility even for a drunk patron or empty purse.
            visit.boughtDrink=false;bought=false;
            if(character.conditions[7])return XeenSimpleServiceOutcome::Drunk;
        }
        if(!afterGold)return XeenSimpleServiceOutcome::InsufficientGold;
        --afterGold;
        if(action=='d') {
            XeenCombatRandom random(afterRandom);std::optional<std::uint32_t> roll;
            // A rejected xorshift32 conversion below 96 has a successor above
            // that threshold, so this inclusive request needs at most two raws.
            for(unsigned attempt=0;attempt<2 && !roll;++attempt)roll=random.draw(1,100);
            if(!roll)throw std::logic_error("Tavern Drink conversion exceeded its bound");
            afterCharacter.conditions[7]=xeenTavernDrinkCondition(character.conditions[7],*roll);
            burp=*roll<26;
            afterRandom=random.continuation();bought=true;
            departure=std::make_unique<XeenServiceDayCandidate>(visit.departure->rebindRandom(afterRandom));
        } else tip=xeenTavernNextTip(_camera.mapId,_world.tavernTipIndex(_camera.mapId));
    } else if(action=='D') {
        if(blessed)return XeenSimpleServiceOutcome::NoAction;
        const auto price=static_cast<std::uint32_t>(templePrices().donation);
        if(!price)return XeenSimpleServiceOutcome::NoAction;
        if(afterGold<price)return XeenSimpleServiceOutcome::InsufficientGold;
        afterGold-=price;blessed=xeenTempleDonationBlessing(afterContext,counter);
        departure=std::make_unique<XeenServiceDayCandidate>(visit.departure->rebindContext(afterContext));
    } else {
        const auto price=xeenTempleUncurseCost(character,templePrices());
        if(!price)return XeenSimpleServiceOutcome::NoAction;
        if(afterGold<price)return XeenSimpleServiceOutcome::InsufficientGold;
        afterGold-=price;xeenTempleClearCurse(afterCharacter);
        try {departure=std::make_unique<XeenServiceDayCandidate>(visit.departure->addTemplePaidDay());}
        catch(const std::overflow_error &) {return XeenSimpleServiceOutcome::SupportLimit;}
    }
    auto *tipLive=tip?_world._sessionState._mapStates.find(_camera.mapId):nullptr;
    if(tip && !tipLive)throw std::logic_error("Tavern visit has no loaded map state");
    XeenMutableOptional<XeenMutable<std::uint8_t>> tipAfter;
    if(tip)tipAfter.emplace(*tip);
    const SmithDepartureBinding binding(departure?*departure:*visit.departure);
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);prepared->characters[owner]=afterCharacter;
    prepared->purse->gold=afterGold;prepared->context=afterContext;prepared->s._journeyRandom=afterRandom;
    if(tip)prepared->s._mapStates.state(_camera.mapId).tavernTips.emplace(*tip);
    checkSmithBoundary(XeenSmithBoundary::BeforePurchase);checkSmithReservation();_journeyPreimage->check();
    // All allocations and successor validation precede the publication.
    auto &live=_party.roster.at(owner);
    if(action=='d')live.conditions[7]=afterCharacter.conditions[7];
    if(action=='u')xeenTempleClearCurse(live);
    _party.purse->gold=afterGold;
    _party.encounterContext=afterContext;_world._sessionState._journeyRandom=afterRandom;
    static_assert(noexcept(tipLive->tavernTips.swap(tipAfter)));
    if(tip)tipLive->tavernTips.swap(tipAfter);
    if(departure){visit.departure.swap(departure);*visit.binding=binding;++visit.reservation;}
    visit.boughtDrink=bought;visit.donationCounter=counter;visit.templeBlessed=blessed;
    if(action=='u')visit.paid=true;
    visit.published=true;++_world._sessionState._journeyGeneration;++_generation;
    prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
    visit.burpPending=visit.burpPending || burp;
    checkSmithBoundary(XeenSmithBoundary::AfterPurchase);return XeenSimpleServiceOutcome::Done;
}
void XeenEncounterFlow::buyFood() {
 if(!_smith || _smith->serviceAction!=3 || _smith->departed || _smith->frame || _busy || !smithCapacity(19,4))
  throw std::logic_error("Food purchase authority unavailable");
 SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
 const auto result=xeenBuyFood(_party.party.size(),_party.food,_party.purse->gold,true,_camera.mapId);
 if(result.outcome!=XeenFoodOutcome::Purchased)return;
 auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
 prepared->retainResources(*_journeyPreimage);prepared->food=result.foodAfter;prepared->purse->gold=result.goldAfter;
 checkSmithBoundary(XeenSmithBoundary::BeforePurchase);checkSmithReservation();
 _party.food=result.foodAfter;_party.purse->gold=result.goldAfter;
 _smith->published=true;++_world._sessionState._journeyGeneration;++_generation;
 prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
 checkSmithBoundary(XeenSmithBoundary::AfterPurchase);
}
bool XeenEncounterFlow::buySpell(std::size_t member,std::size_t slot) {
 if(!_smith || _smith->serviceAction!=2 || _smith->departed || _smith->frame || _busy || !smithCapacity(19,4))
  throw std::logic_error("Spell purchase authority unavailable");
 SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
 if(member>=_party.party.size() || !xeenGuildMember(_party,member,_camera.mapId))return false;
 const auto owner=_party.party.activeRosterIds()[member];const auto &c=_party.roster.at(owner);
 const auto rows=XeenLearnedSpellRules::rows(c,true,false,_camera.mapId);
 if(std::find(rows.begin(),rows.end(),slot)==rows.end())return false;
 const auto id=*XeenLearnedSpellRules::spellForSlot(*XeenLearnedSpellRules::categoryForClass(c.characterClass),slot);
 const auto price=XeenLearnedSpellRules::purchasePrice(id,c.characterClass);
 if(_party.purse->gold<price)return false;
 const auto after=_party.purse->gold-price;
 auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
 prepared->retainResources(*_journeyPreimage);prepared->purse->gold=after;prepared->characters[owner].learnedSpells->at(slot)=1;
 checkSmithBoundary(XeenSmithBoundary::BeforePurchase);checkSmithReservation();
 _party.purse->gold=after;_party.roster.at(owner).learnedSpells->at(slot)=1;
 _smith->published=true;++_world._sessionState._journeyGeneration;++_generation;
 prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
 checkSmithBoundary(XeenSmithBoundary::AfterPurchase);return true;
}
void XeenEncounterFlow::transferBank(bool withdrawal,bool gems,std::uint64_t amount) {
 if(!_smith || _smith->serviceAction!=0 || _smith->departed || _smith->frame || _busy || !smithCapacity(19,4) ||
    !xeenSmithAuthorityRoom(_smith->reservation,1))throw std::logic_error("Bank transfer authority unavailable");
 SmithBusy busy(_busy);_journeyPreimage->check();checkSmithReservation();
 auto &carried=gems?_party.purse->gems:_party.purse->gold;
 auto &banked=gems?_party.serviceEconomy->bank.gems:_party.serviceEconomy->bank.gold;
 const auto result=xeenTransferBank(withdrawal?banked:carried,withdrawal?carried:banked,amount);
 if(result.outcome!=XeenBankOutcome::Transferred)return;
 auto economy=*_party.serviceEconomy;
 (gems?economy.bank.gems:economy.bank.gold)=withdrawal?result.sourceAfter:result.destinationAfter;
 auto departure=std::make_unique<XeenServiceDayCandidate>(_smith->departure->rebindBank(economy));
 const SmithDepartureBinding binding(*departure);
 auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
 prepared->retainResources(*_journeyPreimage);prepared->economy=economy;
 (gems?prepared->purse->gems:prepared->purse->gold)=withdrawal?result.destinationAfter:result.sourceAfter;
 checkSmithBoundary(XeenSmithBoundary::BeforePurchase);checkSmithReservation();
 carried=withdrawal?result.destinationAfter:result.sourceAfter;
 banked=withdrawal?result.sourceAfter:result.destinationAfter;
 _smith->departure.swap(departure);*_smith->binding=binding;++_smith->reservation;
 _smith->published=true;++_world._sessionState._journeyGeneration;++_generation;
 prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
 checkSmithBoundary(XeenSmithBoundary::AfterPurchase);
}
void XeenEncounterFlow::publishServiceAnimationRandom(const XeenJourneyRandomState &before,const XeenJourneyRandomState &after) {
    if(_busy || !current(ticket()) || !_world.sessionState().journeyRandom() ||
        *_world.sessionState().journeyRandom()!=before || !smithCapacity(18,4))
        throw std::logic_error("Service animation RNG authority changed");
    SmithBusy busy(_busy);_journeyPreimage->check();
    std::unique_ptr<XeenServiceDayCandidate> replacement;
    if(_smith) {checkSmithReservation();replacement=std::make_unique<XeenServiceDayCandidate>(_smith->departure->rebindRandom(after));}
    else if(_training)replacement=std::make_unique<XeenServiceDayCandidate>(_training->departure->rebindRandom(after));
    else throw std::logic_error("Service animation owner unavailable");
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);prepared->s._journeyRandom=after;
    _journeyPreimage->check();
    _world._sessionState._journeyRandom=after;
    if(_smith) {_smith->departure.swap(replacement);*_smith->binding=SmithDepartureBinding(*_smith->departure);}
    else _training->departure.swap(replacement);
    ++_world._sessionState._journeyGeneration;++_generation;
    prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
}

bool XeenEncounterFlow::beginSmith(const std::function<void()> &preflight,std::uint8_t service) {
	const bool temple=service==4;
	if (!journeyEvent() || _smith || _smithPreparation || _busy || !current(ticket()) || !journeyCapacity() ||
		!xeenTownServiceKnown(_camera.mapId,service) ||
		!(temple || service==1 || service==0 || service==2 || service==3) || !_party.encounterContext) return false;
	SmithBusy busy(_busy);
	_journeyPreimage->check();
	// Reserve preparation/admission, two service frames, departure, Event
	// retirement and final presentation, including the inherited guard margin.

	if (!smithCapacity(20,5)) return false;
	if (!xeenPrepareSmithDeparture(*_party.encounterContext)) return false;
	xeenValidateJourneyParty(_party);
	checkSmithBoundary(XeenSmithBoundary::BeforeReservation);
	auto next=std::make_unique<SmithContinuation>();
	next->temple=temple;next->serviceAction=service;
    if(temple) {
        next->templePrices=_program.templePrices.prepare(_camera.mapId);
        next->templePriceRevision=_program.templePrices.revision();
    }
    {
        if (!_party.serviceEconomy || !_world.sessionState().journeyRandom())
            throw std::logic_error("Missing service-day owners");
        next->departure=std::make_unique<XeenServiceDayCandidate>(*_party.encounterContext,
            *_party.serviceEconomy,*_world.sessionState().journeyRandom(),XeenScriptServiceCharge::OneDay,service==0,true);
		next->reservation=1;
    }
	checkSmithBoundary(XeenSmithBoundary::AfterReservation);
 checkSmithBoundary(XeenSmithBoundary::BeforeAdmission);
	// Event remains exclusive throughout resource and first-frame preparation.
	_smithPreparation=std::move(next);
    try { XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);preflight();_journeyPreimage->check(); }
	catch (...) { _smithPreparation.reset();_journeyPreimage->check();throw; }
    return true;
}
bool XeenEncounterFlow::serviceSmithPreparation(std::uint64_t &uiTempleRevision) {
    if (!_smithPreparation || _smith || _busy || !journeyEvent() ||
        !current(ticket()) || !journeyCapacity()) return false;
    SmithBusy busy(_busy);
    _journeyPreimage->check();

    if (!smithCapacity(17,5)) throw std::overflow_error("Smith admission authority exhausted");
    auto &day=*_smithPreparation->departure;

	checkSmithBoundary(XeenSmithBoundary::BankPrepared);
    if (!day.reservationReady() || !(day.beforeContext()==*_party.encounterContext) ||
        day.beforeEconomy()!=*_party.serviceEconomy ||
        day.beforeRandom()!=*_world.sessionState().journeyRandom())
        throw std::logic_error("Service-day reservation changed");
    xeenValidateCurrentServiceEconomy(day.economy());
    _smithPreparation->binding.emplace(day);
    _journeyPreimage->check();
    if(_smithPreparation->temple)_program.templePrices.checkPublication(_smithPreparation->templePriceRevision);
    _smithPreparation->lease=_boundary.hold(XeenCombatBoundary::Work::Service);
    // No fallible checks or callbacks between the lease and admission publication.
    if(_smithPreparation->temple) {
        _program.templePrices.publish(_smithPreparation->templePrices);
        _smithPreparation->templePriceRevision=_program.templePrices.revision();
        uiTempleRevision=_smithPreparation->templePriceRevision;
    }
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
void XeenEncounterFlow::quoteSmith(std::size_t member,std::size_t slot,XeenInventoryCategory category,XeenItemService action) {
	if (!_smith || _busy || _smith->frame || member>=_party.party.size() || slot>=9 ||
        static_cast<unsigned>(category)>=4 || action==XeenItemService::Buy || static_cast<unsigned>(action)>3)
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
	_smith->category=category;_smith->itemAction=action;
	const auto &character=_party.roster.at(_smith->owner);
	_smith->result=xeenQuoteItemService(category,
		(*xeenInventoryItems(character,category))[slot],_party.purse->gold,action,character.skills[8]);
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
	const auto &live=(*xeenInventoryItems(_party.roster.at(_smith->owner),_smith->category))[_smith->slot];
	if (!xeenSameItem(live,before.before) || _party.purse->gold!=before.goldBefore)
		throw std::logic_error("Smith quote preimage changed");
	const auto result=xeenPrepareItemService(_smith->category,live,_party.purse->gold,
		_smith->itemAction,_party.roster.at(_smith->owner).skills[8]);
	auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
	prepared->retainResources(*_journeyPreimage);
	auto &items=*xeenInventoryItems(prepared->characters[_smith->owner],_smith->category);
	items[_smith->slot]=result.after;
	if(result.outcome==XeenArmorRepairOutcome::Sold)xeenCompactItems(items);
    xeenValidateCompletedEquipment(prepared->characters[_smith->owner]);
	prepared->purse->gold=result.goldAfter;
	_journeyPreimage->check();
	checkSmithBoundary(XeenSmithBoundary::BeforeRepair);
	checkSmithReservation();
 // One callback-free nonthrowing commit: purse, physical item and fixed result.
	_party.purse->gold=result.goldAfter;
	*xeenInventoryItems(_party.roster.at(_smith->owner),_smith->category)=items;
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
 quote->result=xeenQuoteEquipmentPurchase(_party,member,category,slot,0,xeenIronworksShop(_camera.mapId));
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
     result.category,result.offerSlot,result.offer,result.side,result.shop));
 checkSmithBoundary(XeenSmithBoundary::AfterRebind);
 const SmithDepartureBinding binding(*departure);
 auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
 prepared->retainResources(*_journeyPreimage);
 *xeenInventoryItems(prepared->characters[result.owner],result.category)=result.recipientAfter;
 prepared->purse->gold=result.goldAfter;prepared->economy=candidate->economyAfter;
 auto *recipient=xeenInventoryItems(_party.roster.at(result.owner),result.category);
 auto *stock=&_party.serviceEconomy->wares[result.side][result.shop][static_cast<unsigned>(result.category)];
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
 _party.purse->gold=result.goldAfter;
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
	auto quote=xeenQuoteTempleHeal(_party.roster.at(owner),_party.purse->gold,
		*_party.encounterContext,templePrices());
	if(quote.outcome==XeenTempleHealOutcome::Quoted) {
		quote=xeenPrepareTempleHeal(_party,owner,*_party.encounterContext,templePrices()).result;
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
		_party.purse->gold,*_party.encounterContext,templePrices());
	if(_smith->quoteOperation!=_smith->operation || _smith->quoteReservation!=_smith->reservation ||
		quoted.owner!=current.owner || quoted.price!=current.price ||
		quoted.goldBefore!=current.goldBefore || quoted.hpBefore!=current.hpBefore ||
		quoted.spBefore!=current.spBefore || quoted.maxHpBefore!=current.maxHpBefore ||
		current.outcome!=XeenTempleHealOutcome::Quoted) {
		_journeyPreimage->failed=true;
		throw std::logic_error("Temple Heal quote preimage changed");
	}
	if(!XeenServiceDayCandidate::randomRoom(_smith->departure->beforeRandom())) {
		_smith->healResult.outcome=XeenTempleHealOutcome::SupportLimit;
		_smith->quoted=false;advanceSmith();return true;
	}
	auto delta=std::make_unique<XeenTempleHealCandidate>(
		xeenPrepareTempleHeal(_party,_smith->owner,*_party.encounterContext,templePrices()));
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
    checkSmithBoundary(XeenSmithBoundary::BeforeHealUpgrade);
    upgrade=std::make_unique<XeenServiceDayCandidate>(_smith->departure->addTemplePaidDay());
    checkSmithBoundary(XeenSmithBoundary::AfterHealUpgrade);
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
		checkSmithBoundary(XeenSmithBoundary::BankPrepared);
		if(!(visit.templeUpgrade->beforeContext()==visit.departure->beforeContext()) ||
			visit.templeUpgrade->beforeEconomy()!=visit.departure->beforeEconomy() ||
			visit.templeUpgrade->beforeRandom()!=visit.departure->beforeRandom()) {
			_journeyPreimage->failed=true;
			throw std::logic_error("Temple replacement departure changed");
		}
		visit.templeUpgrade->validateReservation();
	}
	if(!smithCapacity(19,4))throw std::overflow_error("Temple publication would consume departure authority");
	const auto &delta=*visit.healPending;
	const SmithDepartureBinding binding(visit.templeUpgrade?*visit.templeUpgrade:*visit.departure);
	auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
	prepared->retainResources(*_journeyPreimage);
	prepared->characters[visit.owner]=delta.character;
	prepared->inputs[visit.owner]=delta.inputs;
	prepared->purse->gold=delta.result.goldAfter;
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
	_party.purse->gold=delta.result.goldAfter;
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
bool XeenEncounterFlow::departSmith() {
	if (!_smith || _busy || _smith->frame || !journeyCapacity())
		throw std::logic_error("Smith departure authority unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();
	if (!smithCapacity(_smith->departed?5:6,2))
		throw std::overflow_error("Reserved smith departure authority exhausted");
	if (!_smith->departed) {
		checkSmithReservation();
        if(!_smith->departure->service(64,[&]{_journeyPreimage->check();},
            [&]{checkSmithBoundary(XeenSmithBoundary::StockComplete);}))return false;
        *_smith->binding=SmithDepartureBinding(*_smith->departure);
		if(_smith->serviceAction==0) {
            auto settled=std::make_unique<XeenServiceDayCandidate>(_smith->departure->settleBank());
            const SmithDepartureBinding binding(*settled);
            _smith->departure.swap(settled);*_smith->binding=binding;
        }
        const auto &day=*_smith->departure;
        const XeenMutableOptional<XeenGameplayContext> endingContext(day.context());
		const XeenMutableOptional<XeenServiceEconomy> endingEconomy(day.economy());
		const XeenMutableOptional<XeenJourneyRandomState> endingRandom(day.continuation());
		std::vector<XeenExploredTile> exploration;
		{XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
			exploration=_world.explorationAfterStep(_party,_camera);_journeyPreimage->check();}
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		prepared->s.assignExploration(exploration);
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
        _world._sessionState.publishExploration(exploration);
        _smith->departed=true;
		++_world._sessionState._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
		checkSmithBoundary(XeenSmithBoundary::DeparturePublished);
	}
 checkSmithBoundary(XeenSmithBoundary::AfterDeparture);
	// Date is already paid. Classification failure cannot repeat it or undo repairs.
	XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
	const auto view=XeenIndoorScene().classifyActors(_world,_camera,_world.sessionState().regionalActors(_camera.mapId));
	_journeyPreimage->check();
	checkSmithBoundary(XeenSmithBoundary::Return);
 publishArrival(view);
	const auto lease=_smith->lease;
	_world._sessionState._journeyActivity=XeenJourneyActivity::Event;
	++_world._sessionState._journeyGeneration;++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	_smith.reset();_boundary.release(XeenCombatBoundary::Work::Service,lease);
	_smithEventSettlement=true;return true;
}

void XeenEventFlow::prepareSmith() {
	try {
		const auto kind=_pending->state.pendingPresentation->request.kind;
        const bool temple=kind==XeenPresentationKind::TempleService;
        const bool food=kind==XeenPresentationKind::TavernService,bank=kind==XeenPresentationKind::BankService,guild=kind==XeenPresentationKind::GuildService;
        const std::uint8_t service=guild?2:bank?0:food?3:temple?4:1;
		const unsigned inputFrames=10;
		if (!xeenSmithAuthorityRoom(_inputGeneration,inputFrames))
			throw std::overflow_error("Smith input authority exhausted before admission");
		_encounter->_smithBoundary=[this](XeenSmithBoundary stage) { if(smithBoundary)smithBoundary(stage); };
  const bool admitted=_encounter->beginSmith([&] {
			if (guild ? !drawGuildArt : food ? !drawTavernArt || !tavernFoodOffer : bank ? !drawBankArt : temple ? !drawTempleArt : !drawSmithArt)
				throw std::runtime_error("Service artwork provider is unavailable");
			const auto events=_events.scriptForMap(_camera.mapId).file();
			const auto text=_events.textForMap(_camera.mapId);
			_encounter->journeySavePreimage().admitEventText(text);
			SmithUi ui;ui.catalog=_catalog;ui.art=_frame;
			ui.animation.kind=guild?XeenServiceArt::Guild:food?XeenServiceArt::Tavern:bank?XeenServiceArt::Bank:temple?XeenServiceArt::Temple:XeenServiceArt::Smith;
			if(temple) {
                ui.mode=SmithUi::Mode::Heal;
                ui.templePrices=_encounter->templePrices();
                ui.templePriceRevision=_encounter->_smithPreparation->templePriceRevision;
            }
            if(food){ui.mode=SmithUi::Mode::Food;ui.foodOffer=tavernFoodOffer(xeenTavernFoodOffer(_camera.mapId)->text);}
            if(bank)ui.mode=SmithUi::Mode::Bank;
            if(guild)ui.mode=SmithUi::Mode::Guild;
			try { if(guild)drawGuildArt(ui.art);else if(food)drawTavernArt(ui.art);else if(bank)drawBankArt(ui.art);else if(temple)drawTempleArt(ui.art);else drawSmithArt(ui.art); }
            catch (const std::invalid_argument &) { _encounter->journeySavePreimage().failed=true;throw; }
			_encounter->journeySavePreimage().check();
			if (!ui.art.isValid() || ui.art.width!=320 || ui.art.height!=200)
				throw std::runtime_error("Ironworks artwork frame is invalid");
			// Detach retained storage from every mutable buffer exposed to the provider.
			_smithUi=ui;
			(void)drawSmith(_frame); // All fallible initial display preparation precedes admission.
		},service);
		if (!admitted) {
			_smithUi.reset();_pending.reset();_journeyEventLayers=false;
			_encounter->_journeyRefusal=temple ? "Temple unavailable: departure exceeds the supported year." :
				"Ironworks unavailable: departure exceeds the supported year.";
		} else {
			claimServiceContinuation(&*_smithUi);
            _world.scenePresentation().resetServiceBorder();
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
   _smithUi->feedback="Unable to finish. Press Escape to retry.";
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
        if(!ui.feedback.empty()) {input={};input.anyKey=input.anyClick=input.keyMouseWait=true;}
    } else if(_smithUi) {
        const auto &ui=*_smithUi;
        if((ui.goodbye && !_smithSettlement) || ui.phase==SmithUi::Phase::Preparation) return std::make_shared<const DialogInput>();
        if(ui.phase==SmithUi::Phase::Departure) input.keys={InputKey::Escape,InputKey::Enter};
        else if(ui.phase==SmithUi::Phase::Detail) {input.anyKey=input.anyClick=input.keyMouseWait=true;}
        else if(ui.phase==SmithUi::Phase::Upgrade) input.keys={InputKey::Escape};
        else if(ui.phase==SmithUi::Phase::Amount) {
            input.keys={InputKey::Escape,InputKey::Enter,8};input.textEntry=true;
        }
        else if(ui.phase==SmithUi::Phase::Currency) input=xeenBankInput();
        else if(ui.phase==SmithUi::Phase::Confirm) input=ui.mode==SmithUi::Mode::Guild?xeenSpellConfirmInput(ui.spellInfo):ui.mode==SmithUi::Mode::Food?xeenFoodConfirmInput():xeenConfirmInput();
        else if(ui.phase==SmithUi::Phase::Browse) input=ui.mode==SmithUi::Mode::Guild?xeenSpellListInput(false):xeenBuyInput(ui.itemAction());
        else input=xeenLocationInput(ui.mode==SmithUi::Mode::Guild?XeenLocationDialog::Guild:ui.mode==SmithUi::Mode::Food?XeenLocationDialog::Tavern:ui.mode==SmithUi::Mode::Bank?XeenLocationDialog::Bank:ui.mode==SmithUi::Mode::Heal?XeenLocationDialog::Temple:XeenLocationDialog::Smith);
        if((!ui.feedback.empty() || !ui.tavernStatus.empty() || !ui.tavernPopup.empty()) && ui.phase!=SmithUi::Phase::Departure) {input={};input.anyKey=input.anyClick=input.keyMouseWait=true;}
    } else return {};
    return std::make_shared<const DialogInput>(std::move(input));
}
std::string XeenEventFlow::smithText() const {
    const auto &ui=*_smithUi;
    if(!ui.feedback.empty()) return ui.feedback;
    if(ui.phase==SmithUi::Phase::Detail)return ui.itemDetails;
    if(ui.mode==SmithUi::Mode::Guild) {
        if(ui.phase==SmithUi::Phase::Confirm) {
            const auto &c=_party.party.member(_party.roster,ui.member);
            const auto id=*XeenLearnedSpellRules::spellForSlot(*XeenLearnedSpellRules::categoryForClass(c.characterClass),ui.slot);
            const auto &name=_encounter->journeySavePreimage().learnedNames->names[id];
            return ui.spellInfo?xeenDialogFormat(dosText().scalar("SPELL_INFO"),{name,ui.descriptions.at(id)}):
                xeenDialogFormat(dosText().scalar("SPELL_PURCHASE"),{ui.descriptions.at(id),name,std::to_string(XeenLearnedSpellRules::purchasePrice(id,c.characterClass))});
        }
        return xeenLocationText(dosText(),XeenLocationDialog::Guild,_party,ui.member,_camera.mapId);
    }
    if(ui.mode==SmithUi::Mode::Food) return ui.phase==SmithUi::Phase::Confirm?ui.foodOffer:xeenLocationText(dosText(),XeenLocationDialog::Tavern,_party,ui.member,_camera.mapId);
    if(ui.mode==SmithUi::Mode::Bank) return ui.phase==SmithUi::Phase::Currency || ui.phase==SmithUi::Phase::Amount?
        xeenBankText(dosText(),_party,ui.withdrawal,ui.bankRefreshed):xeenLocationText(dosText(),XeenLocationDialog::Bank,_party,ui.member,_camera.mapId);
    if(ui.phase==SmithUi::Phase::Confirm) {
        const auto &visit=*_encounter->_smith;
        const auto &item=ui.mode==SmithUi::Mode::Buy?visit.purchase->result.offer:visit.result.before;
        return xeenServiceConfirm(dosText(),ui.itemAction(),
            ui.catalog.describe(ui.category,item).coloredName(15),
            ui.mode==SmithUi::Mode::Buy?visit.purchase->result.price:visit.result.price);
    }
    if(ui.mode==SmithUi::Mode::Heal) {
        const auto *visit=_encounter->_smith ? _encounter->_smith.get() : _encounter->_smithPreparation.get();
        return xeenTempleText(dosText(),_party,ui.member,_encounter->templePrices(
            _smithSettlement && ui.phase==SmithUi::Phase::Departure?&ui.templePrices:nullptr,ui.templePriceRevision),visit && visit->templeBlessed);
    }
    return xeenLocationText(dosText(),XeenLocationDialog::Smith,_party,ui.member,_camera.mapId);
}
IndexedFrame XeenEventFlow::drawSmith(const IndexedFrame &world) const {
    const auto &ui=*_smithUi;
    auto frame=drawXeenLocation(dosText(),world,ui.art,_inventoryFont,
        ui.mode==SmithUi::Mode::Guild?XeenLocationDialog::Guild:ui.mode==SmithUi::Mode::Food?XeenLocationDialog::Tavern:ui.mode==SmithUi::Mode::Bank?XeenLocationDialog::Bank:ui.mode==SmithUi::Mode::Heal?XeenLocationDialog::Temple:XeenLocationDialog::Smith,_party,ui.member,_camera.mapId,drawDialogSprite,_encounter->_smith && _encounter->_smith->templeBlessed,ui.tavernStatus,ui.mode==SmithUi::Mode::Heal?&_encounter->templePrices(
            _smithSettlement && ui.phase==SmithUi::Phase::Departure?&ui.templePrices:nullptr,ui.templePriceRevision):nullptr);
    if(ui.mode==SmithUi::Mode::Guild && (ui.phase==SmithUi::Phase::Browse || ui.phase==SmithUi::Phase::Confirm)) {
        frame=drawXeenSpellList(dosText(),frame,_inventoryFont,_party.party.member(_party.roster,ui.member),
            *_encounter->journeySavePreimage().learnedNames,ui.spells,_party.purse->gold,true,ui.spellInfo,drawDialogSprite,_party.encounterContext->year,_camera.mapId);
        if(ui.phase==SmithUi::Phase::Confirm)frame=drawXeenSpellConfirm(frame,_inventoryFont,smithText(),ui.spellInfo,drawDialogSprite);
    }
    else if(ui.mode==SmithUi::Mode::Food && ui.phase==SmithUi::Phase::Confirm) {
        frame=world;
        for(int y=8;y<140;++y)std::copy_n(ui.art.pixels.data()+y*320+8,216,frame.pixels.data()+y*320+8);
        frame=drawXeenFoodOffer(frame,_inventoryFont,ui.foodOffer,drawDialogSprite);
    }
    else if(ui.mode==SmithUi::Mode::Bank && (ui.phase==SmithUi::Phase::Currency || ui.phase==SmithUi::Phase::Amount)) {
        frame=drawXeenBankCurrency(dosText(),frame,_inventoryFont,_party,ui.withdrawal,drawDialogSprite,ui.bankRefreshed);
        if(ui.phase==SmithUi::Phase::Amount)frame=drawXeenBankAmount(dosText(),frame,_inventoryFont,ui.amount,drawDialogSprite,char(ui.amountCursor.cursor()));
    }
    else if(ui.phase==SmithUi::Phase::Browse || ui.phase==SmithUi::Phase::Confirm || ui.phase==SmithUi::Phase::Detail) {
        XeenInventorySelection selection;selection.source=ui.member;selection.category=ui.category;
        if(ui.selected) selection.slot=ui.slot;
        frame=drawXeenBuy(dosText(),frame,_inventoryFont,ui.catalog,_party,selection,ui.itemAction(),drawDialogSprite,_camera.mapId);
        if(ui.phase==SmithUi::Phase::Confirm) frame=drawXeenConfirm(frame,_inventoryFont,smithText(),false,drawDialogSprite);
        if(ui.phase==SmithUi::Phase::Detail)frame=drawXeenIdentifiedDetails(frame,_inventoryFont,ui.itemDetails);
    }
    if(!ui.tavernPopup.empty())frame=drawXeenTavernPopup(frame,_inventoryFont,ui.tavernPopup);
    if(!ui.feedback.empty()) frame=drawXeenErrorScroll(frame,_inventoryFont,ui.feedback);
    return frame;
}
IndexedFrame XeenEventFlow::settleSmithEvent() {
	requireServiceSettlementEntry(_smithUi?static_cast<const void *>(&*_smithUi):nullptr);
	if(!_smithSettlement)throw std::logic_error("Service has not reached its settlement boundary");
	return journeyEventWork([&] {
		_encounter->checkSmithBoundary(XeenSmithBoundary::BeforeEventSettlement);
		if (!_smithTerminalResult) {
			// Retain the admitted terminal suffix until execution succeeds. A failed
			// provider must not consume the pending state or admit another visit.
			auto result=resumeOwnedServiceEvent(&*_smithUi);
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
    if(_smithUi->mode==SmithUi::Mode::Food || _smithUi->mode==SmithUi::Mode::Bank || _smithUi->mode==SmithUi::Mode::Guild) return handleFoodBank(action,input,inputFrame);
    auto &ui=*_smithUi;const auto key=serviceKey(action);
    if(ui.phase==SmithUi::Phase::Preparation) return frameCopy();
    const bool member=key>=InputKey::F1 && key<InputKey::F1+_party.party.size();
    const bool allowed=ui.phase==SmithUi::Phase::Detail?true:ui.phase==SmithUi::Phase::Departure?(key==InputKey::Escape || key==InputKey::Enter):
        ui.phase==SmithUi::Phase::Confirm?xeenConfirmAnswer(key).has_value():
        ui.phase==SmithUi::Phase::Lobby?(key=='b' || key=='r' || key==InputKey::Escape || member):
        (key==InputKey::Escape || member || (key>='1' && key<='9') || key=='w' || key=='a' || key=='c' || key=='m' || key=='b' || key=='s' || key=='i' || key=='f');
    if((ui.feedback.empty() || ui.phase==SmithUi::Phase::Departure) && !allowed) return frameCopy();
    if(_smithSettlement) {_handoffPending=true;return settleSmithEvent();}
    if(!_encounter->consumeSmithFrame(input,inputFrame)) return frameCopy();
    serviceInputPolled(ui.animation);
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
        if(ui.phase==SmithUi::Phase::Detail) {ui.phase=SmithUi::Phase::Browse;ui.selected=false;_encounter->advanceSmith();}
        else if(ui.phase!=SmithUi::Phase::Departure && !ui.feedback.empty()) {ui.feedback.clear();_encounter->advanceSmith();}
        else if(ui.phase==SmithUi::Phase::Lobby) {
            if(key>=InputKey::F1 && key<InputKey::F1+_party.party.size()) {
                ui.member=key-InputKey::F1;_encounter->advanceSmith();
            } else if(key=='b' || key=='r') {
                // R retains the maintainer-authorized Armor Repair lobby path.
                ui.mode=key=='r'?SmithUi::Mode::Repair:SmithUi::Mode::Buy;
                ui.category=key=='r'?XeenInventoryCategory::Armor:XeenInventoryCategory::Weapons;ui.selected=false;
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
            } else if(key=='b' || key=='f' || key=='s' || key=='i') {
                ui.mode=key=='b'?SmithUi::Mode::Buy:key=='s'?SmithUi::Mode::Sell:key=='i'?SmithUi::Mode::Identify:SmithUi::Mode::Repair;ui.selected=false;
                _encounter->_smith->quoted=false;_encounter->advanceSmith();
            } else if(cancel) {ui.phase=SmithUi::Phase::Lobby;ui.selected=false;_encounter->_smith->quoted=false;_encounter->advanceSmith();}
            else if(row) {
                ui.slot=key-'1';ui.selected=true;
                const auto &c=_party.party.member(_party.roster,ui.member);
                const auto &item=ui.mode==SmithUi::Mode::Buy?_party.serviceEconomy->wares[0][xeenIronworksShop(_camera.mapId)][static_cast<unsigned>(ui.category)][ui.slot]:
                    (*xeenInventoryItems(c,ui.category))[ui.slot];
                if(!item.id) {ui.selected=false;_encounter->advanceSmith();}
                else if(ui.mode==SmithUi::Mode::Buy) {
                    _encounter->quoteSmithBuy(ui.member,ui.category,ui.slot);
                    const auto outcome=_encounter->_smith->purchase->result.outcome;
                    if(_encounter->_smith->quoted) ui.phase=SmithUi::Phase::Confirm;
                    else if(outcome==XeenEquipmentPurchaseOutcome::DestinationFull) ui.feedback=xeenBackpackFull(dosText(),ui.category,c.name);
                    else ui.feedback="Buy: not supported yet";
                } else {
                    if(ui.mode==SmithUi::Mode::Identify) {
                        const auto details=xeenIdentifiedDetails(dosText(),c,ui.category,item,*_encounter->journeySavePreimage().learnedNames);
                        ui.itemDetails=xeenDialogFormat(dosText().scalar("IDENTIFY_ITEM_MSG"),{ui.catalog.describe(ui.category,item).coloredName(15),details});
                    }
                    _encounter->quoteSmith(ui.member,ui.slot,ui.category,ui.itemAction());
                    const auto outcome=_encounter->_smith->result.outcome;
                    if(_encounter->_smith->quoted) ui.phase=SmithUi::Phase::Confirm;
                    else if(outcome==XeenArmorRepairOutcome::Intact)ui.feedback=std::string(xeenDialogText(dosText(),XeenDialogText::ItemNotBroken));
                    else if(outcome==XeenArmorRepairOutcome::Refused)ui.feedback=xeenDialogFormat(dosText().scalar("NO_NEED_OF_THIS"),{ui.catalog.describe(ui.category,item).coloredName(15)});
                    else ui.feedback="Item service: not supported yet";
                }
            }
        } else if(ui.phase==SmithUi::Phase::Confirm) {
            if(confirming) {
                if(ui.mode==SmithUi::Mode::Buy) _encounter->confirmSmithBuy();else _encounter->confirmSmith();
                const bool shortfall=ui.mode==SmithUi::Mode::Buy?
                    _encounter->_smith->purchase->result.outcome==XeenEquipmentPurchaseOutcome::InsufficientGold:
                    _encounter->_smith->result.outcome==XeenArmorRepairOutcome::InsufficientGold;
                if(shortfall) ui.feedback=serviceNotEnoughGold();
            } else {_encounter->_smith->quoted=false;_encounter->advanceSmith();}
            ui.phase=confirming && ui.mode==SmithUi::Mode::Identify && _encounter->_smith->result.outcome==XeenArmorRepairOutcome::Identified?SmithUi::Phase::Detail:SmithUi::Phase::Browse;ui.selected=false;
        }
        if(ui.phase==SmithUi::Phase::Departure && (cancel || key==InputKey::Enter)) {
            if(originalPhase!=ui.phase && ui.revision!=UINT64_MAX) {++ui.revision;revisionAdvanced=true;}
            if(!_encounter->departSmith())return renderEncounter();_smithSettlement=true;return settleSmithEvent();
        }
    } catch(const std::exception &) {
        failed=true;_encounter->journeySavePreimage().check();if(!_encounter->_smith) throw;
        if(_encounter->_smith->quoted) ui.phase=SmithUi::Phase::Confirm;
        if(_encounter->_smith->published && ui.phase!=SmithUi::Phase::Departure) {ui.phase=ui.mode==SmithUi::Mode::Identify && _encounter->_smith->result.outcome==XeenArmorRepairOutcome::Identified?SmithUi::Phase::Detail:SmithUi::Phase::Browse;ui.selected=false;}
        // A retained Confirm can be retried with a fresh Y. A published action
        // is never repeated, including a failure in a post-publication hook.
        if(ui.phase!=SmithUi::Phase::Confirm && ui.phase!=SmithUi::Phase::Detail) ui.feedback="Service preparation failed. Escape: depart.";
    }
    if(!revisionAdvanced && (!failed || originalPhase!=ui.phase) && ui.revision!=UINT64_MAX) ++ui.revision;
    if(reportText && !ui.feedback.empty()) reportText(ui.feedback);
    return renderEncounter();
}
IndexedFrame XeenEventFlow::handleFoodBank(const PlayerAction &action,std::uint64_t input,const IndexedFrame::Presentation &inputFrame) {
    auto &ui=*_smithUi;const auto key=serviceKey(action);
    const auto *typed=std::get_if<TextInputAction>(&action);
    const bool food=ui.mode==SmithUi::Mode::Food,guild=ui.mode==SmithUi::Mode::Guild;
    if(ui.goodbye && !_smithSettlement)return frameCopy();
    if(ui.phase==SmithUi::Phase::Preparation)return frameCopy();
    const auto controls=serviceDialogInput();
    if((ui.feedback.empty() || ui.phase==SmithUi::Phase::Departure) && !(ui.phase==SmithUi::Phase::Amount && (typed || (key>='0' && key<='9'))) && !controls->key(key))return frameCopy();
    if(_smithSettlement){ui.feedback.clear();if(ui.signIn)return finishSignIn();_handoffPending=true;return settleSmithEvent();}
    if(!_encounter->consumeSmithFrame(input,inputFrame))return frameCopy();
    serviceInputPolled(ui.animation);
    if(ui.phase!=SmithUi::Phase::Departure && !(ui.phase==SmithUi::Phase::Lobby && key==InputKey::Escape) &&
        (!_encounter->smithCapacity(19,4) || !xeenSmithAuthorityRoom(_inputGeneration,10) ||
        !xeenSmithAuthorityRoom(ui.revision,1) || !xeenSmithAuthorityRoom(_encounter->_smith->reservation,1))) {
        ui.phase=SmithUi::Phase::Departure;ui.feedback="Further service actions unavailable. Escape: depart.";
        if(ui.revision!=UINT64_MAX)++ui.revision;return renderEncounter();
    }
    const auto refusal=[&](bool empty) {
        _encounter->fmEffect(21);
        return xeenDialogFormat(dosText().scalar(empty?"NO_X_IN_THE_Y":"NOT_ENOUGH_X_IN_THE_Y"),
            {dosText().table("CONSUMABLE_NAMES")[ui.gems?1:0],dosText().table("WHERE_NAMES")[ui.withdrawal?1:0]});
    };
    try {
        if(food && (!ui.tavernStatus.empty() || !ui.tavernPopup.empty())) {
            ui.tavernStatus.clear();ui.tavernPopup.clear();
        }
        else if(!ui.feedback.empty() && ui.phase!=SmithUi::Phase::Departure) {
            ui.feedback.clear();
            if(food && ui.phase==SmithUi::Phase::Confirm)ui.phase=SmithUi::Phase::Lobby;
        }
        else if(ui.phase==SmithUi::Phase::Lobby) {
            if(key==InputKey::Escape)ui.phase=SmithUi::Phase::Departure;
            else if((food || guild) && key>=InputKey::F1 && key<InputKey::F1+_party.party.size()) {
                ui.member=key-InputKey::F1;
                if(food)_encounter->_smith->boughtDrink=false;
                if(guild && !xeenGuildMember(_party,ui.member,_camera.mapId))startServiceSample(ui.animation,"guild11.voc");
            }
            else if(guild) {
                if(xeenGuildMember(_party,ui.member,_camera.mapId)) {
                    if(key=='b' && !_party.party.member(_party.roster,ui.member).canAct())ui.feedback=xeenDialogFormat(dosText().scalar("IN_NO_CONDITION"),{_party.party.member(_party.roster,ui.member).name});
                    if(key=='s' || (key=='b' && _party.party.member(_party.roster,ui.member).canAct())) {
                        if(ui.descriptions.empty()) {
                            if(!loadSpellDescriptions)throw std::runtime_error("Missing spell descriptions provider");
                            ui.descriptions=xeenSpellDescriptions(loadSpellDescriptions());
                        }
                        ui.spellInfo=key=='s';ui.spells={};ui.phase=SmithUi::Phase::Browse;
                    }
                }
            }
            else if(food) {
                if(key=='s') {ui.signIn=true;ui.phase=SmithUi::Phase::Departure;}
                else if(key=='f')ui.phase=SmithUi::Phase::Confirm;
                else if(key=='d' || key=='t') {
                    std::string popup;
                    const auto &c=_party.party.member(_party.roster,ui.member);
                    if(key=='t' && c.canAct() && _encounter->_smith->boughtDrink && !c.conditions[7] && _party.purse->gold) {
                        if(!tavernString)throw std::runtime_error("Missing Tavern strings provider");
                        popup=xeenDialogFormat(dosText().scalar("TAVERN_RUMOR_FORMAT"),{tavernString(xeenTavernTipText(_camera.mapId,_world.tavernTipIndex(_camera.mapId)))});
                    }
                    const auto result=_encounter->simpleService(ui.member,static_cast<char>(key));
                    if(result==XeenSimpleServiceOutcome::NoAction && !c.canAct())ui.feedback=xeenDialogFormat(dosText().scalar("IN_NO_CONDITION"),{c.name});
                    else if(result==XeenSimpleServiceOutcome::InsufficientGold)ui.feedback=serviceNotEnoughGold();
                    else if(result==XeenSimpleServiceOutcome::HaveDrink)ui.tavernStatus=dosText().scalar("TAVERN_HAVE_A_DRINK");
                    else if(result==XeenSimpleServiceOutcome::Drunk)ui.tavernStatus=dosText().scalar("TAVERN_YOURE_DRUNK");
                    else if(result==XeenSimpleServiceOutcome::Done) {
                        if(key=='d'){ui.tavernStatus=dosText().scalar("TAVERN_GOOD_STUFF");startServiceSample(ui.animation,"gulp.voc");}
                        else {ui.tavernPopup=std::move(popup);startServiceSample(ui.animation,"thankyou.voc");}
                    }
                } else if(key=='r') {
                    if(!tavernString)throw std::runtime_error("Missing Tavern strings provider");
                    ui.tavernPopup=xeenDialogFormat(dosText().scalar("TAVERN_RUMOR_FORMAT"),{tavernString(xeenTavernRumorText(_camera.mapId,_party.encounterContext->day))});
                } else if(const auto refusal=xeenLocationUnsupportedAction(XeenLocationDialog::Tavern,key))ui.feedback=*refusal;
            } else if(key=='d' || key=='w') {
                ui.withdrawal=key=='w';ui.bankRefreshed=false;ui.phase=SmithUi::Phase::Currency;ui.animation.sample.stop();
            }
        } else if(guild && ui.phase==SmithUi::Phase::Browse) {
            if(key==InputKey::Escape)ui.phase=SmithUi::Phase::Lobby;
            else if(key>=InputKey::F1 && key<InputKey::F1+_party.party.size()) {
                ui.member=key-InputKey::F1;
                if(!xeenGuildMember(_party,ui.member,_camera.mapId))ui.phase=SmithUi::Phase::Lobby;
            } else {
                const auto &c=_party.party.member(_party.roster,ui.member);
                const auto rows=XeenLearnedSpellRules::rows(c,true,ui.spellInfo,_camera.mapId);ui.spells.scroll(key,rows.size());
                if(const auto row=ui.spells.row(key,rows.size())) {
                    ui.slot=rows[*row];ui.phase=SmithUi::Phase::Confirm;_encounter->_smith->published=false;
                }
            }
        } else if(guild && ui.phase==SmithUi::Phase::Confirm) {
            if(ui.spellInfo)ui.phase=SmithUi::Phase::Browse;
            else if(const auto answer=xeenConfirmAnswer(key)) {
                if(*answer) {
                    if(!_encounter->buySpell(ui.member,ui.slot))ui.feedback=serviceNotEnoughGold();
                    else startServiceSample(ui.animation,"guild12.voc");
                }
                ui.phase=SmithUi::Phase::Browse;
            }
        } else if(ui.phase==SmithUi::Phase::Confirm) {
            const auto answer=xeenConfirmAnswer(key);
            if(answer) {
                ui.phase=SmithUi::Phase::Lobby;
                if(*answer) {
                    const auto quote=xeenBuyFood(_party.party.size(),_party.food,_party.purse->gold,true,_camera.mapId);
                    if(quote.outcome==XeenFoodOutcome::Full) {ui.feedback=dosText().scalar("FOOD_PACKS_FULL");ui.phase=SmithUi::Phase::Confirm;}
                    else if(quote.outcome==XeenFoodOutcome::InsufficientGold) {ui.feedback=serviceNotEnoughGold();ui.phase=SmithUi::Phase::Confirm;}
                    else {_encounter->buyFood();startServiceSample(ui.animation,"thankyou.voc");}
                }
            }
        } else if(ui.phase==SmithUi::Phase::Currency) {
            if(key==InputKey::Escape)ui.phase=SmithUi::Phase::Lobby;
            else if(key=='o' || key=='e') {
                ui.gems=key=='e';
                const auto &bank=_party.serviceEconomy->bank;const auto &purse=*_party.purse;
                const std::uint32_t source=ui.withdrawal?(ui.gems?bank.gems:bank.gold):(ui.gems?purse.gems:purse.gold);
                if(!source)ui.feedback=refusal(true);
                else {ui.amount.clear();ui.amountCursor.begin();ui.phase=SmithUi::Phase::Amount;}
            }
        } else if(ui.phase==SmithUi::Phase::Amount) {
            if(key==InputKey::Escape) {ui.amount.clear();ui.bankRefreshed=true;ui.phase=SmithUi::Phase::Currency;startServiceSample(ui.animation,"coina.voc");}
            else if(key==InputKey::Enter) {
                const auto amount=xeenBankAmount(ui.amount);ui.bankRefreshed=true;ui.phase=SmithUi::Phase::Currency;ui.amount.clear();
                const auto &bank=_party.serviceEconomy->bank;const auto &purse=*_party.purse;
                const std::uint32_t source=ui.withdrawal?(ui.gems?bank.gems:bank.gold):(ui.gems?purse.gems:purse.gold);
                const std::uint32_t destination=ui.withdrawal?(ui.gems?purse.gems:purse.gold):(ui.gems?bank.gems:bank.gold);
                const auto result=xeenTransferBank(source,destination,amount);
                if(result.outcome==XeenBankOutcome::Excess || result.outcome==XeenBankOutcome::EmptySource)
                    ui.feedback=refusal(result.outcome==XeenBankOutcome::EmptySource);
                else if(result.outcome==XeenBankOutcome::Transferred)_encounter->transferBank(ui.withdrawal,ui.gems,amount);
                startServiceSample(ui.animation,"coina.voc");
            } else if(key==8) {if(!ui.amount.empty())ui.amount.pop_back();ui.amountCursor.keyRedraw();}
            else {
                const std::string packet=typed?typed->text:(key>='0' && key<='9'?std::string(1,char(key)):std::string{});
                if(std::all_of(packet.begin(),packet.end(),[](unsigned char c){return c>='0' && c<='9';}))
                    ui.amount+=packet.substr(0,10-ui.amount.size());
                ui.amountCursor.keyRedraw();
            }
        }
        if(ui.phase==SmithUi::Phase::Departure && (key==InputKey::Escape || key==InputKey::Enter)) {
            if(food && !ui.goodbye) {
                _encounter->digitalStop();
                startServiceSample(ui.animation,"goodbye.voc");ui.goodbye=true;
                if(serviceSampleBusy(ui.animation))return frameCopy();
            }
            if(ui.revision!=UINT64_MAX)++ui.revision;
            if(!_encounter->departSmith())return renderEncounter();_smithSettlement=true;return settleSmithEvent();
        }
    } catch(const std::exception &) {
        _encounter->journeySavePreimage().check();if(!_encounter->_smith)throw;
        if(guild && ui.phase==SmithUi::Phase::Confirm && _encounter->_smith->published)ui.phase=SmithUi::Phase::Browse;
        ui.feedback=ui.phase==SmithUi::Phase::Departure?"Service departure failed. Escape: retry.":"Service action could not finish. Continue.";
    }
    if(ui.revision!=UINT64_MAX)++ui.revision;
    if(reportText)reportText(smithText());
    return renderEncounter();
}
IndexedFrame XeenEventFlow::handleTemple(const PlayerAction &action,std::uint64_t input,const IndexedFrame::Presentation &inputFrame) {
    auto &ui=*_smithUi;const auto key=serviceKey(action);
    if(ui.phase==SmithUi::Phase::Preparation) return frameCopy();
    const bool member=key>=InputKey::F1 && key<InputKey::F1+_party.party.size();
    const bool allowed=ui.phase==SmithUi::Phase::Departure?(key==InputKey::Escape || key==InputKey::Enter):
        ui.phase==SmithUi::Phase::Upgrade?key==InputKey::Escape:(member || key==InputKey::Escape || key=='h' || key=='d' || key=='u');
    if((ui.feedback.empty() || ui.phase==SmithUi::Phase::Departure) && !allowed) return frameCopy();
    if(_smithSettlement) {_handoffPending=true;return settleSmithEvent();}
    if(!_encounter->consumeSmithFrame(input,inputFrame)) return frameCopy();
    serviceInputPolled(ui.animation);
    const bool cancel=key==InputKey::Escape;
    const unsigned revisions=key=='h'?2:1;
    if(ui.phase!=SmithUi::Phase::Departure && !(cancel && (ui.phase==SmithUi::Phase::Lobby || ui.phase==SmithUi::Phase::Upgrade)) &&
        (!_encounter->smithCapacity(20,4) || !xeenSmithAuthorityRoom(_inputGeneration,10) || !xeenSmithAuthorityRoom(ui.revision,revisions) ||
        (key=='h' && !xeenSmithAuthorityRoom(_encounter->_smith->operation,1)) ||
        ((key=='h' || key=='u') && !xeenSmithAuthorityRoom(_encounter->_smith->reservation,1)))) {
        ui.phase=SmithUi::Phase::Departure;ui.feedback="Further Temple actions unavailable. Escape: depart.";
        if(ui.revision!=UINT64_MAX) ++ui.revision;return renderEncounter();
    }
    const auto originalPhase=ui.phase;
    bool failed=false,revisionAdvanced=false;
    try {
        if(ui.phase!=SmithUi::Phase::Departure && !ui.feedback.empty()) {ui.feedback.clear();_encounter->advanceSmith();}
        else if(ui.phase==SmithUi::Phase::Lobby) {
            if(key>=InputKey::F1 && key<InputKey::F1+_party.party.size()) {
                ui.member=key-InputKey::F1;_encounter->_smith->donationCounter=0;_encounter->_smith->quoted=false;_encounter->advanceSmith();
            } else if(key=='d' || key=='u') {
                const auto result=_encounter->simpleService(ui.member,key=='d'?'D':'u');
                if(result==XeenSimpleServiceOutcome::InsufficientGold)ui.feedback=serviceNotEnoughGold();
                else if(result==XeenSimpleServiceOutcome::SupportLimit)ui.feedback="Uncurse: not supported yet at this date or capacity";
                else if(result==XeenSimpleServiceOutcome::Done)startServiceSample(ui.animation,key=='d' && !_encounter->_smith->templeBlessed?"coina.voc":"ahh.voc");
            } else if(key=='h') {
                if(!_encounter->_smith->quoted) _encounter->quoteTempleHeal(ui.member);
                if(_encounter->_smith->quoted) ui.phase=_encounter->confirmTempleHeal()?SmithUi::Phase::Lobby:SmithUi::Phase::Upgrade;
                const auto outcome=_encounter->_smith->healResult.outcome;
                if(outcome==XeenTempleHealOutcome::InsufficientGold) ui.feedback=serviceNotEnoughGold();
                else if(outcome==XeenTempleHealOutcome::SupportLimit || outcome==XeenTempleHealOutcome::HpSupportLimit)
                    ui.feedback="Heal: not supported yet at this date or capacity";
            } else if(cancel) ui.phase=SmithUi::Phase::Departure;
        } else if(ui.phase==SmithUi::Phase::Upgrade && cancel) {_encounter->cancelTempleHeal();ui.phase=SmithUi::Phase::Lobby;}
        if(ui.phase==SmithUi::Phase::Departure && (cancel || key==InputKey::Enter)) {
            if(originalPhase!=ui.phase && ui.revision!=UINT64_MAX) {++ui.revision;revisionAdvanced=true;}
            if(!_encounter->departSmith())return renderEncounter();_smithSettlement=true;return settleSmithEvent();
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
