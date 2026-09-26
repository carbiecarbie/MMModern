#include "app/XeenEncounterFlow.h"
#include "app/XeenEventFlow.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenVertigoRoute.h"
#include <stdexcept>
#include <sstream>
#include <limits>
#include "games/xeen/XeenEquipment.h"

namespace mmodern {
namespace {
struct SmithBusy { bool &flag; explicit SmithBusy(bool &value):flag(value){flag=true;} ~SmithBusy(){flag=false;} };
}
void XeenEncounterFlow::checkSmithBoundary(XeenSmithBoundary boundary) {
 _journeyPreimage->check();
 try { if (_smithBoundary) _smithBoundary(boundary); }
 catch (...) { _journeyPreimage->check();throw; }
 _journeyPreimage->check();
}
bool XeenEncounterFlow::beginSmith(const std::function<void()> &preflight) {
	if (!journeyEvent() || _smith || _busy || !current(ticket()) || !journeyCapacity() ||
		_world.sessionState().journeyContract()!=9 || _camera.mapId!=XeenMapIdentity(28) ||
		_camera.x!=8 || _camera.y!=4 || !_party.encounterContext) return false;
	SmithBusy busy(_busy);
	_journeyPreimage->check();
	if (!xeenPrepareSmithDeparture(*_party.encounterContext,9)) return false;
	xeenValidateJourneyParty(_party,9);
	auto next=std::make_unique<SmithContinuation>();
 checkSmithBoundary(XeenSmithBoundary::BeforeAdmission);
	// Event remains exclusive throughout resource and first-frame preparation.
	try { XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);preflight();_journeyPreimage->check(); }
	catch (...) { _journeyPreimage->check();throw; }
	next->lease=_boundary.hold(XeenCombatBoundary::Work::Service);
	_smith=std::move(next);
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
	xeenValidateJourneyParty(_party,9);
	if (_smith->operation==std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Smith operation generation exhausted");
    ++_smith->operation;
    _smith->owner=_party.party.activeRosterIds()[member];_smith->slot=static_cast<std::uint8_t>(slot);
	_smith->result=xeenQuoteArmorRepair(XeenInventoryCategory::Armor,
		_party.roster.at(_smith->owner).armor[slot],_party.monsterTreasure->gold);
	_smith->quoted=_smith->result.outcome==XeenArmorRepairOutcome::Quoted;
 checkSmithBoundary(XeenSmithBoundary::Quote);
}
void XeenEncounterFlow::confirmSmith() {
	if (!_smith || !_smith->quoted || _busy || _smith->frame || !journeyCapacity())
		throw std::logic_error("Smith quote authority unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();
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
 // One callback-free nonthrowing commit: purse, physical item and fixed result.
	_party.monsterTreasure->gold=result.goldAfter;
	_party.roster.at(_smith->owner).armor[_smith->slot]=result.after;
	_smith->result=result;_smith->quoted=false;
	++_world._sessionState._journeyGeneration;++_generation;
	prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
 checkSmithBoundary(XeenSmithBoundary::AfterRepair);
}
void XeenEncounterFlow::departSmith() {
	if (!_smith || _busy || _smith->frame || !journeyCapacity())
		throw std::logic_error("Smith departure authority unavailable");
	SmithBusy busy(_busy);_journeyPreimage->check();
	if (!_smith->departed) {
		const auto after=xeenPrepareSmithDeparture(*_party.encounterContext,9);
		if (!after) throw std::logic_error("Owed smith departure is no longer canonical");
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);prepared->context=*after;
		_journeyPreimage->check();
		checkSmithBoundary(XeenSmithBoundary::BeforeDeparture);
  _party.encounterContext=*after;_smith->departed=true;
		++_world._sessionState._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
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
}

void XeenEventFlow::prepareSmith() {
	try {
		_encounter->_smithBoundary=[this](XeenSmithBoundary stage) { if(smithBoundary)smithBoundary(stage); };
  const bool admitted=_encounter->beginSmith([&] {
			if (!drawSmithArt) throw std::runtime_error("Ironworks artwork provider is unavailable");
			const auto events=_events.scriptForMap(28).file();
			const auto mainland=_events.scriptForMap(23).file();
			try { xeenValidateVertigoRoute(mainland,events,9); }
            catch (const std::invalid_argument &) { _encounter->journeySavePreimage().failed=true;throw; }
			const auto text=_events.textForMap(28);
			_encounter->journeySavePreimage().admitVertigoText(text);
			SmithUi ui;ui.catalog=_catalog;ui.title=text.strings.at(33);ui.art=_frame;
			try { drawSmithArt(ui.art); }
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
			_encounter->_journeyRefusal="Ironworks unavailable: departure would require unsupported restocking.";
		} else { _presenter.clear();_journeyEventLayers=false; }
 } catch (const std::exception &) {
  _encounter->journeySavePreimage().check();
  if (!_encounter->_smith) {
   _smithUi.reset();_pending.reset();_journeyEventLayers=false;
   _encounter->_journeyRefusal="Ironworks preparation failed. Try entry again.";
  } else {
   _presenter.clear();_journeyEventLayers=false;
   _smithUi->feedback="Retry; one-day departure still owed.";
  }
 }
}
IndexedFrame XeenEventFlow::drawSmith(const IndexedFrame &world) const {
	const auto &ui=*_smithUi;
	const auto owner=_party.party.activeRosterIds().at(ui.member);
	const auto &character=_party.roster.at(owner);
	const auto gold=_party.monsterTreasure->gold;
	std::ostringstream text;
	text<<ui.title<<"\n"<<character.name<<"  Gold "<<gold<<"\n";
	if (ui.phase==SmithUi::Phase::Lobby) {
		text<<"Armor repair only\nEnter: Repair armor   F1-F6: owner\nEscape: depart (costs one day)";
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
 XeenTextRenderOptions options;
	options.bounds={9,9,222,157};options.windowBounds={8,8,223,159};
	options.x=10;options.y=10;options.size=XeenFontSize::Reduced;
	options.paginate=true;options.drawWindow=true;
	if (ui.phase==SmithUi::Phase::Lobby) {
		options.bounds={9,93,222,157};options.windowBounds={8,91,223,159};options.y=93;
	}
	auto background=world;
 if (ui.phase==SmithUi::Phase::Lobby)
  for(int y=8;y<140;++y)
   std::copy_n(ui.art.pixels.data()+y*320+8,215,background.pixels.data()+y*320+8);
 auto rendered=XeenTextRenderer(_inventoryFont).render(background,text.str(),options);
	if (rendered.pages.size()!=1) throw std::runtime_error("Ironworks panel did not fit");
	return std::move(rendered.pages.front());
}
IndexedFrame XeenEventFlow::handleSmith(const PlayerAction &action,std::uint64_t input) {
	if (!_encounter->consumeSmithFrame(input,_frame.presentation())) return frameCopy();
	auto &ui=*_smithUi;
 ui.feedback.clear();
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
		_smithUi.reset();
		// Resume only the admitted cmdExit continuation, under the retained Event lease.
		return journeyEventWork([&] {
			auto pending=std::move(*_pending);_pending.reset();
			drive(_events.resumeManualEvent(std::move(pending.state),XeenPresentationResponse::Acknowledged,
				_world,_party,_camera,_flags,_eventPublication),false);
		});
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
}
