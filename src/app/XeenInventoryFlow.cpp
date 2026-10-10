#include "app/XeenEventFlow.h"
#include <algorithm>
#include <iostream>
#include <limits>

namespace mmodern {
namespace {
struct Scope {
	bool &busy;
	explicit Scope(bool &value) : busy(value) { busy = true; }
	~Scope() { busy = false; }
};
}
void XeenEventFlow::advanceInventoryEpoch() noexcept {
	if (_certificateLease && journey() && !_encounter->combat()) {
		try { _encounter->releaseJourneyWork(XeenCombatBoundary::Work::Certificate,_certificateLease); }
		catch (...) { closeGameplay(); }
		_certificateLease = 0;
	}
	if (_certificateLease && _encounter && _encounter->combat()) {
		try { _encounter->boundary().release(XeenCombatBoundary::Work::Certificate,_certificateLease); }
		catch (...) { _encounter->combat()->invalidate(); _fatal=true; }
		_certificateLease = 0;
	}
	_equipmentSelection.reset();
	if (_inventoryEpoch != std::numeric_limits<std::uint64_t>::max()) ++_inventoryEpoch;
	else _inventory = {}; // Exhausted generations can never arm again.
}
void XeenEventFlow::armEquipmentSelection() {
	const auto &ids = _party.party.activeRosterIds();
	if (_inventory.mode != XeenInventoryMode::Browse || !_inventory.slot ||
			ids.size() > XeenParty::kMaximumVisibleMembers || _inventory.source >= ids.size() ||
			_inventory.sourceOwner != ids[_inventory.source] || ids[_inventory.source] >= XeenRoster::kCharacterCount ||
			*_inventory.slot >= 9) return;
	const auto owner = ids[_inventory.source];
	const auto &character = _party.roster.at(owner);
	if (character.rosterId != owner) return;
	const auto *items = xeenInventoryItems(character, _inventory.category);
	if (!items || !xeenSameItem((*items)[*_inventory.slot], _inventory.record)) return;
	EquipmentSelection certificate;
	certificate.epoch = _inventoryEpoch;
	certificate.membershipSize = ids.size();
	std::copy(ids.begin(), ids.end(), certificate.membership.begin());
	certificate.sourceActiveIndex = _inventory.source;
	certificate.resolvedOwner = owner;
	certificate.category = _inventory.category;
	certificate.physicalSlot = *_inventory.slot;
	certificate.selectedRecord = _inventory.record;
	_equipmentSelection = certificate;
	syncCombatInventory();
}
bool XeenEventFlow::validEquipmentSelection(const EquipmentSelection &certificate) const {
	const auto &ids = _party.party.activeRosterIds();
	if (certificate.epoch != _inventoryEpoch || ids.size() != certificate.membershipSize ||
			ids.size() > XeenParty::kMaximumVisibleMembers ||
			!std::equal(ids.begin(), ids.end(), certificate.membership.begin()) ||
			certificate.sourceActiveIndex >= ids.size() ||
			ids[certificate.sourceActiveIndex] != certificate.resolvedOwner ||
			_inventory.source != certificate.sourceActiveIndex ||
			_inventory.sourceOwner != certificate.resolvedOwner ||
			certificate.resolvedOwner >= XeenRoster::kCharacterCount ||
			_inventory.category != certificate.category || _inventory.slot != certificate.physicalSlot ||
			certificate.physicalSlot >= 9 || !xeenSameItem(_inventory.record, certificate.selectedRecord)) return false;
	const auto &character = _party.roster.at(certificate.resolvedOwner);
	if (character.rosterId != certificate.resolvedOwner) return false;
	const auto *items = xeenInventoryItems(character, certificate.category);
	return items && xeenSameItem((*items)[certificate.physicalSlot], certificate.selectedRecord);
}
bool XeenEventFlow::validInventorySource(bool record) const {
	const auto &ids = _party.party.activeRosterIds();
	if (_inventory.source >= ids.size() || _inventory.sourceOwner != ids[_inventory.source]) return false;
	const auto &c = _party.roster.at(ids[_inventory.source]);
	if (c.rosterId != ids[_inventory.source]) return false;
	const auto *items = xeenInventoryItems(c,_inventory.category);
	if (!items) return false;
	return !record || (_inventory.slot && *_inventory.slot < 9 && xeenSameItem((*items)[*_inventory.slot],_inventory.record));
}
void XeenEventFlow::invalidateInventorySelection() {
	if (_encounter && _encounter->itemUseActive() && !_encounter->itemUseReady()) {
		if (!_encounter->abandonItemUse(_encounter->ticket(),_itemUseGeneration.value_or(0),_inventoryEpoch)) {
			closeGameplay();return;
		}
	}
	_itemUseGeneration.reset();
	advanceInventoryEpoch();
	_equipmentResult.reset();
	if (!inventoryOpen()) return;
	_inventory.mode = XeenInventoryMode::Browse;
	if (!validInventorySource(false)) {
		_inventory.source = 0;
		_inventory.sourceOwner.reset();
		if (_party.party.size()) _inventory.sourceOwner = _party.party.activeRosterIds()[0];
		_inventory.slot.reset(); _inventory.record = {};
	} else if (!validInventorySource(true)) { _inventory.slot.reset(); _inventory.record = {}; }
}
void XeenEventFlow::invalidateInventory() {
	requireCurrentOwners();
	if (_encounter) {
		if (journey()) { closeGameplay(); return; }
		if (_encounter->combat()) _encounter->combat()->invalidate();
		return;
	}
	// Notification remains effective inside an observer, but never draws/reenters.
	invalidateInventorySelection();
}
void XeenEventFlow::closeInventory() noexcept {
	// A spent charge remains owned by EncounterFlow; shutdown cannot publish Quiet.
	if (_encounter && _encounter->itemUseActive() && !_encounter->itemUseReady() && _itemUseGeneration) {
		try { if (!_encounter->abandonItemUse(_encounter->ticket(),*_itemUseGeneration,_inventoryEpoch)) closeGameplay(); }
		catch (...) { closeGameplay(); }
	}
	_itemUseGeneration.reset();
	advanceInventoryEpoch();
	if (_inventoryLease && journey() && !_encounter->combat()) {
		try { _encounter->releaseJourneyWork(XeenCombatBoundary::Work::Inventory,_inventoryLease); }
		catch (...) { closeGameplay(); }
		_inventoryLease = 0;
	}
	if (_inventoryLease && _encounter && _encounter->combat()) {
		try { _encounter->boundary().release(XeenCombatBoundary::Work::Inventory,_inventoryLease); }
		catch (...) { _encounter->combat()->invalidate(); _fatal=true; }
		_inventoryLease = 0;
	}
	_inventory = {};
	_sheet.reset();_itemsVisible=false;_combatItems=false;_statPopup.reset();_dialogError.reset();_itemOption.reset();
	_inventoryFeedback = "";
	_equipmentResult.reset();
}
void XeenEventFlow::recoverInventory() {
	if (journey()) {
		if (!_encounter->current(_encounter->ticket())) { _fatal = true; throw std::runtime_error("Journey inventory integrity failure"); }
		closeInventory();
		return; // The retained outer dispatcher performs the guarded rebuild.
	}
	if (_encounter && (_encounter->combat())) {
		const auto entry = _encounter->ticket();
		_encounter->fail(entry);
		closeInventory();
		renderEncounter();
		return;
	}
	closeInventory();
	_presenter.clear();
	try { refreshScene(true,OrdinaryCause::None); }
	catch (...) { _fatal = true; throw; }
}
void XeenEventFlow::drawInventory() {
	if (journey()) return; // One composition under the Journey presentation guard.
	if (_encounter && (_encounter->combat())) { syncCombatInventory(); renderEncounter(); return; }
	try { _frame = drawCharacterDialog(_inventoryUnderlay);if(_summary)_frame=drawSummary(_frame); }
	catch (...) { recoverInventory(); }
}
IndexedFrame XeenEventFlow::refuseInventorySave() {
	requireCurrentOwners();
	if (_encounter) return _frame;
	if (_dispatching || _fatal || !inventoryOpen()) return _frame;
	Scope scope(_dispatching);
	_equipmentResult.reset();
	_inventoryFeedback = "Close inventory before F9; then press F9 again";
	drawInventory();
	return _frame;
}
void XeenEventFlow::handleEquipment(XeenEquipmentOperation operation) {
	const auto certificate = _equipmentSelection;
	const bool currentCertificate = certificate && validEquipmentSelection(*certificate);
	advanceInventoryEpoch(); // Every E is consumed before preparation or callbacks.
	_equipmentResult.reset();
	const auto &ids = _party.party.activeRosterIds();
	if (ids.empty()) { _inventoryFeedback = "No active characters"; drawInventory(); return; }
	if (_inventory.category == XeenInventoryCategory::Miscellaneous) {
		_inventoryFeedback = "Misc equipment is unsupported"; drawInventory(); return;
	}
	// An empty record is ordinary only if that was what the explicit selection
	// captured. An occupied certificate whose live/UI record changed to empty is stale.
	if (!_inventory.slot || (certificate ? !certificate->selectedRecord.id : !_inventory.record.id)) {
		_inventoryFeedback = "Select an occupied item"; drawInventory(); return;
	}
	if (!currentCertificate) {
		_inventory.slot.reset(); _inventory.record = {};
		_inventoryFeedback = "Selection changed; select again"; drawInventory(); return;
	}

	const auto result = journey() ? _encounter->journeyEquipment(_encounter->ticket(),
		certificate->sourceActiveIndex,certificate->category,certificate->physicalSlot,operation) :
		xeenSetEquipment(_party, certificate->sourceActiveIndex, certificate->category, certificate->physicalSlot, operation);
	_equipmentResult = result;
	_inventory.slot.reset(); _inventory.record = {};
	_inventoryFeedback = "";
	const auto report = result; // Stable even if the callback invalidates Flow state.
	const auto authority = _encounter ? std::optional<XeenEncounterFlow::Ticket>{_encounter->ticket()} : std::nullopt;
	try { if (reportEquipment) reportEquipment(report); }
	catch (...) { if (authority && !journey() && !_encounter->fail(*authority)) _fatal=true; throw; }
	if (authority && !_encounter->current(*authority)) { _fatal=true; throw std::runtime_error("Stale equipment reporting"); }
	if (result.status == XeenEquipmentStatus::Success)
		refreshScene(true, OrdinaryCause::None);
	else {
		using S=XeenEquipmentStatus;
		const auto &c=_party.roster.at(certificate->resolvedOwner);
		const auto description=_catalog.describe(certificate->category,certificate->selectedRecord).displayName;
		if(result.status==S::Cursed) dialogError(std::string(xeenDialogText(dosText(),XeenDialogText::CursedItem)));
		else if(result.status==S::NotProficient) dialogError(xeenDialogFormat(xeenDialogText(dosText(),XeenDialogText::NotProficient),{std::string(dosText().table("CLASS_NAMES").at(static_cast<unsigned>(c.characterClass))),description}));
		else if(result.status==S::RingLimit || result.status==S::MedalLimit)
			dialogError(xeenDialogFormat(xeenDialogText(dosText(),XeenDialogText::EquippedAll),{std::string(xeenDialogText(dosText(),result.status==S::RingLimit?XeenDialogText::Ring:XeenDialogText::Medal))}));
		else if(result.status==S::Conflict && result.conflict) {
			const auto &item=(*xeenInventoryItems(c,result.conflict->category))[result.conflict->physicalSlot];
			dialogError(xeenDialogFormat(xeenDialogText(dosText(),XeenDialogText::RemoveToEquip),{_catalog.describe(result.conflict->category,item).displayName,description}));
		} else if(result.status!=S::NoChange) dialogError("Equipment: not supported yet");
		drawInventory();
	}
}
void XeenEventFlow::transferInventory(std::size_t destination) {
 // M47 Part A approved combat limit; never bypass the combat preimage.
 if(_encounter && _encounter->combat()) {dialogError("Transfer in combat: not supported yet");return;}
 const auto certificate=_equipmentSelection;
 const bool valid=certificate && validEquipmentSelection(*certificate);
 advanceInventoryEpoch();
 _transferResult={XeenTransferStatus::StaleSelection};
 if(valid) _transferResult=journey()?_encounter->journeyTransfer(_encounter->ticket(),certificate->sourceActiveIndex,
  destination,certificate->category,certificate->physicalSlot):xeenTransferItem(_party,certificate->sourceActiveIndex,
  destination,certificate->category,certificate->physicalSlot);
 invalidateInventorySelection();_inventory.slot.reset();_inventory.record={};
 const auto authority=_encounter?std::optional<XeenEncounterFlow::Ticket>{_encounter->ticket()}:std::nullopt;
 try {if(reportInventory) reportInventory(_transferResult);} catch(...) {if(authority && !journey() && !_encounter->fail(*authority)) _fatal=true;throw;}
 if(authority && !_encounter->current(*authority)) {_fatal=true;throw std::runtime_error("Stale transfer reporting");}
 if(_transferResult.status==XeenTransferStatus::Cursed) dialogError(std::string(xeenDialogText(dosText(),XeenDialogText::CursedItem)));
 else if(_transferResult.status==XeenTransferStatus::DestinationFull)
  dialogError(xeenBackpackFull(dosText(),certificate->category,_party.party.member(_party.roster,destination).name));
 if(_transferResult.status==XeenTransferStatus::Success) refreshScene(true,OrdinaryCause::None);
}
IndexedFrame XeenEventFlow::handleInventory(const PlayerAction &action) {
 using Mode=XeenInventoryMode;
 try {
  if(_inventoryEpoch>=std::numeric_limits<std::uint64_t>::max()-2) {closeInventory();_frame=_inventoryUnderlay;return _frame;}
  if(!inventoryOpen()) {
   advanceInventoryEpoch();_inventory.mode=Mode::Browse;_itemsVisible=true;
   _inventory.source=0;if(_party.party.size()) _inventory.sourceOwner=_party.party.activeRosterIds()[0];
   syncCombatInventory();_transferResult={};_equipmentResult.reset();_inventoryFeedback="";
   _presenter.clear();refreshScene(true,OrdinaryCause::None);return _frame;
  }
  unsigned key=0;
  if(const auto *dialog=std::get_if<DialogKeyAction>(&action)) key=dialog->key;
  else if(const auto *member=std::get_if<SelectMemberAction>(&action)) key=InputKey::F1+member->partyIndex;
  else if(const auto *slot=std::get_if<SelectInventorySlotAction>(&action)) key='1'+slot->slot;
  else if(std::holds_alternative<CancelInteractionAction>(action)) key=InputKey::Escape;
  else if(std::holds_alternative<EquipmentInventoryAction>(action)) key='e';
  else if(std::holds_alternative<UseItemAction>(action)) key='u';
  if(_inventory.mode==Mode::UseTarget) {
   if(key>=InputKey::F1 && key<InputKey::F1+6 && key-InputKey::F1>=_party.party.size()) return _frame;
   if((key>=InputKey::F1 && key<InputKey::F1+6) || key==InputKey::Escape) {
    if(!_itemUseGeneration || !_encounter->finishItemUse(_encounter->ticket(),*_itemUseGeneration,_inventoryEpoch,
     key==InputKey::Escape?std::nullopt:std::optional<std::size_t>{key-InputKey::F1},_inputGeneration,responseFrame())) {closeGameplay();return _frame;}
    closeInventory();_frame=_inventoryUnderlay;return _frame;
   }
   return _frame;
  }
  if(key==InputKey::Escape) {
   if(_itemOption) _itemOption.reset();
   else if(_sheet) {_itemsVisible=false;invalidateInventorySelection();_inventory.slot.reset();_inventory.record={};}
   else {closeInventory();_frame=_inventoryUnderlay;return _frame;}
  } else if(key>='1' && key<='9' && validInventorySource(false)) {
   const std::size_t slot=key-'1';
   const auto &item=(*xeenInventoryItems(_party.roster.at(*_inventory.sourceOwner),_inventory.category))[slot];
   const bool occupied=_inventory.category==XeenInventoryCategory::Miscellaneous?item.material!=0:item.id!=0;
   if(occupied) {
    const auto option=_itemOption;_itemOption.reset();
    const bool selected=_inventory.slot==slot;
    advanceInventoryEpoch();
    if(selected && !option) {_inventory.slot.reset();_inventory.record={};}
    else {_inventory.slot=slot;_inventory.record=item;armEquipmentSelection();if(option) performItemOption(*option);}
   }
  } else if(!_itemOption && key>=InputKey::F1 && key<InputKey::F1+6 && !_combatItems) {
   if(const auto member=dialogMember(key-InputKey::F1)) {
    if(_inventory.slot) transferInventory(*member);
    else {advanceInventoryEpoch();_inventory.source=*member;_inventory.sourceOwner=_party.party.activeRosterIds()[*member];_inventory.record={};}
   }
  } else if(!_itemOption && (key=='w' || key=='a' || key=='c' || key=='m')) {
   advanceInventoryEpoch();_inventory.slot.reset();_inventory.record={};
   _inventory.category=key=='w'?XeenInventoryCategory::Weapons:key=='a'?XeenInventoryCategory::Armor:key=='c'?XeenInventoryCategory::Accessories:XeenInventoryCategory::Miscellaneous;
  } else if(!_itemOption && key=='q') dialogError("Quest: not supported yet");
  else if(!_itemOption && (key=='e' || key=='r' || key=='u' || key=='d')) {
   const bool misc=_inventory.category==XeenInventoryCategory::Miscellaneous;
   if((key=='e' && misc) || (key=='u' && !misc)) return _frame;
   const unsigned option=key=='e'?0:key=='r'?1:key=='u'?2:3;
   if(_encounter && _encounter->combat() && option<3) {
    dialogError(option==2?std::string(xeenDialogText(dosText(),XeenDialogText::UseInCombat)):"Equipment in combat: not supported yet");
    drawInventory();return _frame;
   }
   const auto &items=*xeenInventoryItems(_party.roster.at(*_inventory.sourceOwner),_inventory.category);
   const bool empty=misc?items[0].material==0:items[0].id==0;
   if(empty) {if(!misc) {if(_sheet) _itemsVisible=false;else closeInventory();}}
   else if(_inventory.slot) performItemOption(option);
   else _itemOption=option;
  }
  if(inventoryOpen()) drawInventory();return _frame;
 } catch(...) {if(_fatal) throw;recoverInventory();return _frame;}
}
void XeenEventFlow::syncCombatInventory() {
	if (journey() && !_encounter->combat()) {
		if (inventoryOpen() && !_inventoryLease) _inventoryLease = _encounter->holdJourneyWork(XeenCombatBoundary::Work::Inventory);
		if (_equipmentSelection && !_certificateLease)
			_certificateLease = _encounter->holdJourneyWork(XeenCombatBoundary::Work::Certificate);
		return;
	}
	if (!_encounter || !_encounter->combat()) return;
	auto &boundary = _encounter->boundary();
	if (inventoryOpen() && !_inventoryLease) _inventoryLease = boundary.hold(XeenCombatBoundary::Work::Inventory);
	if (_equipmentSelection && !_certificateLease)
		_certificateLease = boundary.hold(XeenCombatBoundary::Work::Certificate);
}
}
