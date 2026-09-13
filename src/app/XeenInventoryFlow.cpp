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
	if (_certificateLease && journey()) {
		try { _encounter->releaseJourneyWork(XeenCombatBoundary::Work::Certificate,_certificateLease); }
		catch (...) { closeGameplay(); }
		_certificateLease = 0;
	}
	if (_certificateLease && _encounter && _encounter->combat()) {
		try { _encounter->boundary().release(XeenCombatBoundary::Work::Certificate,_certificateLease); }
		catch (...) { _encounter->combat()->invalidate(); _fatal=true; }
		_certificateLease = 0;
	}
	_inventoryConfirmation.reset();
	_equipmentSelection.reset();
	if (_inventoryEpoch != std::numeric_limits<std::uint64_t>::max()) ++_inventoryEpoch;
	else _inventory = {}; // Exhausted generations can never arm again.
}
void XeenEventFlow::armEquipmentSelection() {
	if (completed()) return;
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
std::optional<std::uint64_t> XeenEventFlow::inventoryConfirmation() const {
	return _inventoryConfirmation ? std::optional<std::uint64_t>{_inventoryConfirmation->epoch} : std::nullopt;
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
	advanceInventoryEpoch();
	_equipmentResult.reset();
	if (!inventoryOpen()) return;
	_inventory.mode = XeenInventoryMode::Browse;
	_inventory.destination.reset(); _inventory.destinationOwner.reset();
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
		if (completed()) { _encounter->closeCompleted(); _fatal = true; }
		return;
	}
	// Notification remains effective inside an observer, but never draws/reenters.
	invalidateInventorySelection();
}
void XeenEventFlow::closeInventory() noexcept {
	advanceInventoryEpoch();
	if (_inventoryLease && journey()) {
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
	_inventoryFeedback = "";
	_equipmentResult.reset();
}
void XeenEventFlow::recoverInventory() {
	if (journey()) {
		if (!_encounter->current(_encounter->ticket())) { _fatal = true; throw std::runtime_error("Journey inventory integrity failure"); }
		closeInventory();
		return; // The retained outer dispatcher performs the guarded rebuild.
	}
	if (_encounter && (_encounter->combat() || completed())) {
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
	if (completed()) return; // The completed dispatcher performs one guarded render.
	if (_encounter && (_encounter->combat() || completed())) { syncCombatInventory(); renderEncounter(); return; }
	try { _frame = drawXeenInventory(_inventoryUnderlay,_inventoryFont,_catalog,_party,_inventory,_inventoryFeedback,
		_equipmentResult ? &*_equipmentResult : nullptr); }
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
void XeenEventFlow::handleEquipment() {
	if (completed()) return;
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
	const auto operation = certificate->selectedRecord.frame == 0 ?
		XeenEquipmentOperation::Equip : XeenEquipmentOperation::Remove;
	const auto result = journey() ? _encounter->journeyEquipment(_encounter->ticket(),
		certificate->sourceActiveIndex,certificate->category,certificate->physicalSlot,operation) :
		_encounter && _encounter->combat() ? _encounter->combat()->equipment(_encounter->combat()->ticket(),
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
	else drawInventory();
}
void XeenEventFlow::confirmInventory() {
	if (completed()) return;
	if (!_inventoryConfirmation || _inventory.mode != XeenInventoryMode::Confirm) return;
	const auto token = *_inventoryConfirmation;
	const bool current = token.epoch == _inventoryEpoch;
	advanceInventoryEpoch(); // Consume before any helper or fallible preparation.
	const auto &ids = _party.party.activeRosterIds();
	const auto &s = token.selection;
	bool participants = current && ids.size() == token.size &&
		std::equal(ids.begin(),ids.end(),token.membership.begin()) &&
		_inventory.source == s.source && _inventory.sourceOwner == s.sourceOwner &&
		s.source < ids.size() && s.sourceOwner == ids[s.source] &&
		s.destination && *s.destination < ids.size() && s.destinationOwner == ids[*s.destination] &&
		_inventory.destination == s.destination && _inventory.destinationOwner == s.destinationOwner;
	_transferResult = {XeenTransferStatus::StaleSelection};
	if (participants) {
		const bool ownersValid = _party.roster.at(*s.sourceOwner).rosterId == *s.sourceOwner &&
			_party.roster.at(*s.destinationOwner).rosterId == *s.destinationOwner;
		if (!ownersValid) _transferResult.status = XeenTransferStatus::InvalidOwner;
		else if (s.sourceOwner == s.destinationOwner) _transferResult.status = XeenTransferStatus::SameOwner;
		else if (_inventory.category == s.category && _inventory.slot == s.slot && s.slot &&
			validInventorySource(true) && xeenSameItem(_inventory.record,s.record))
			_transferResult = journey() ? _encounter->journeyTransfer(_encounter->ticket(),s.source,*s.destination,s.category,*s.slot) :
				_encounter && _encounter->combat() ? _encounter->combat()->transfer(_encounter->combat()->ticket(),
				s.source,*s.destination,s.category,*s.slot) : xeenTransferItem(_party,s.source,*s.destination,s.category,*s.slot);
	}
	const bool success = _transferResult.status == XeenTransferStatus::Success;
	invalidateInventorySelection();
	if (success) { _inventory.slot.reset(); _inventory.record = {}; }
	_inventoryFeedback = xeenTransferMessage(_transferResult.status);
	// Result and disarming precede every callback, formatting operation and draw.
	const auto authority = _encounter ? std::optional<XeenEncounterFlow::Ticket>{_encounter->ticket()} : std::nullopt;
	try { if (reportInventory) reportInventory(_transferResult); }
	catch (...) { if (authority && !journey() && !_encounter->fail(*authority)) _fatal=true; throw; }
	if (authority && !_encounter->current(*authority)) { _fatal=true; throw std::runtime_error("Stale transfer reporting"); }
	if (success) refreshScene(true,OrdinaryCause::None);
}
IndexedFrame XeenEventFlow::handleInventory(const PlayerAction &action) {
	using Mode = XeenInventoryMode;
	try {
	if (completed() && (std::holds_alternative<TransferInventoryAction>(action) ||
		std::holds_alternative<EquipmentInventoryAction>(action) || std::holds_alternative<AcknowledgeAction>(action) ||
		std::holds_alternative<RevisitCompletedAction>(action))) return _frame;
	if (_inventoryEpoch >= std::numeric_limits<std::uint64_t>::max()-2) {
		closeInventory(); _frame=_inventoryUnderlay; return _frame;
	}
	if (inventoryOpen() && !std::holds_alternative<EquipmentInventoryAction>(action))
		_equipmentResult.reset();
	if (!inventoryOpen()) {
		if (_inventoryEpoch == std::numeric_limits<std::uint64_t>::max()) return _frame;
		advanceInventoryEpoch();
		_inventory.mode = Mode::Browse;
		syncCombatInventory();
		if (_party.party.size()) _inventory.sourceOwner = _party.party.activeRosterIds()[0];
		_transferResult = {};
		_equipmentResult.reset();
		_inventoryFeedback = "";
		_presenter.clear();
		if (!completed()) refreshScene(true,OrdinaryCause::None);
		if (!inventoryOpen()) return _frame;
		if (!journey()) std::cout << (completed() ? XeenEncounterFlow::completedInspection(_world, _party, _camera) : xeenInventoryInspection(_party));
		return _frame;
	}
	if (std::holds_alternative<CancelInteractionAction>(action) ||
		(combatPreparation() && _inventory.mode != Mode::Browse && std::holds_alternative<InspectInventoryAction>(action)) ||
		(_inventory.mode == Mode::Confirm && std::holds_alternative<NoAction>(action))) {
		if (_inventory.mode == Mode::Browse) { closeInventory(); _frame = _inventoryUnderlay; return _frame; }
		invalidateInventorySelection(); _inventoryFeedback = "Transfer cancelled";
	} else if (_inventory.mode == Mode::Browse && std::holds_alternative<InspectInventoryAction>(action)) {
		closeInventory(); _frame = _inventoryUnderlay; return _frame;
	} else if (const auto *member = std::get_if<SelectMemberAction>(&action)) {
		advanceInventoryEpoch();
		if (member->partyIndex >= _party.party.size()) {
			_inventoryFeedback = "No active member at that F-key";
			if (_inventory.mode != Mode::Browse) {
				_inventory.mode = Mode::ChooseDestination;
				_inventory.destination.reset(); _inventory.destinationOwner.reset();
			}
		} else if (_inventory.mode == Mode::Browse) {
			_inventory.source = member->partyIndex;
			_inventory.sourceOwner = _party.party.activeRosterIds()[member->partyIndex];
			_inventory.slot.reset(); _inventory.record = {}; _inventoryFeedback = "";
		} else {
			_inventory.destination = member->partyIndex;
			_inventory.destinationOwner = _party.party.activeRosterIds()[member->partyIndex];
			_inventory.mode = Mode::Confirm;
			InventoryConfirmation token{_inventoryEpoch,_inventory};
			const auto &ids = _party.party.activeRosterIds();
			token.size = ids.size(); std::copy(ids.begin(),ids.end(),token.membership.begin());
			_inventoryConfirmation = token;
			syncCombatInventory();
			_inventoryFeedback = "Enter to confirm one transfer";
		}
	} else if (_inventory.mode == Mode::Confirm && std::holds_alternative<AcknowledgeAction>(action)) {
		confirmInventory();
	} else if (_inventory.mode != Mode::Browse) {
		_inventoryFeedback = combatPreparation() ? "I to cancel; Esc exits session" : "Escape to cancel";
	} else if (std::holds_alternative<EquipmentInventoryAction>(action)) {
		handleEquipment();
		return _frame;
	} else if (const auto *nav = std::get_if<NavigationAction>(&action)) {
		advanceInventoryEpoch(); _inventoryFeedback = "";
		if (*nav == NavigationAction::TurnLeft || *nav == NavigationAction::TurnRight) {
			_inventory.category = static_cast<XeenInventoryCategory>((static_cast<unsigned>(_inventory.category)+
				(*nav == NavigationAction::TurnLeft ? 3 : 1))%4);
			_inventory.slot.reset(); _inventory.record = {};
		} else if (validInventorySource(false)) {
			const bool up = *nav == NavigationAction::MoveForward;
			_inventory.slot = _inventory.slot ? (*_inventory.slot + (up ? 8 : 1))%9 : up ? 8 : 0;
			_inventory.record = (*xeenInventoryItems(_party.roster.at(*_inventory.sourceOwner),_inventory.category))[*_inventory.slot];
			armEquipmentSelection();
		}
	} else if (const auto *slot = std::get_if<SelectInventorySlotAction>(&action)) {
		advanceInventoryEpoch();
		if (slot->slot < 9 && validInventorySource(false)) {
			_inventory.slot = slot->slot;
			_inventory.record = (*xeenInventoryItems(_party.roster.at(*_inventory.sourceOwner),_inventory.category))[slot->slot];
			_inventoryFeedback = "";
			armEquipmentSelection();
		}
	} else if (std::holds_alternative<TransferInventoryAction>(action)) {
		advanceInventoryEpoch();
		if (!_party.party.size()) _inventoryFeedback = "No active characters";
		else if (!validInventorySource(true) || !_inventory.record.id) _inventoryFeedback = "Select an occupied item";
		else { _inventory.mode = Mode::ChooseDestination; _inventoryFeedback = "Choose recipient F1-F6"; }
	}
	if (inventoryOpen()) drawInventory();
	return _frame;
	} catch (...) {
		if (completed()) throw; // Recovery belongs to the retained completed dispatcher.
		if (_fatal) throw; // A failed clean-base recovery is already terminal.
		recoverInventory(); return _frame;
	}
}
void XeenEventFlow::syncCombatInventory() {
	if (journey()) {
		if (inventoryOpen() && !_inventoryLease) _inventoryLease = _encounter->holdJourneyWork(XeenCombatBoundary::Work::Inventory);
		if ((_equipmentSelection || _inventoryConfirmation) && !_certificateLease)
			_certificateLease = _encounter->holdJourneyWork(XeenCombatBoundary::Work::Certificate);
		return;
	}
	if (!_encounter || !_encounter->combat()) return;
	auto &boundary = _encounter->boundary();
	if (inventoryOpen() && !_inventoryLease) _inventoryLease = boundary.hold(XeenCombatBoundary::Work::Inventory);
	if ((_equipmentSelection || _inventoryConfirmation) && !_certificateLease)
		_certificateLease = boundary.hold(XeenCombatBoundary::Work::Certificate);
}
}
