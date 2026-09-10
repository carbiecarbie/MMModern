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
	_inventoryConfirmation.reset();
	if (_inventoryEpoch != std::numeric_limits<std::uint64_t>::max()) ++_inventoryEpoch;
	else _inventory = {}; // Exhausted generations can never arm again.
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
	// Notification remains effective inside an observer, but never draws/reenters.
	invalidateInventorySelection();
}
void XeenEventFlow::closeInventory() noexcept {
	advanceInventoryEpoch();
	_inventory = {};
	_inventoryFeedback = "";
}
void XeenEventFlow::recoverInventory() {
	closeInventory();
	_presenter.clear();
	try { refreshScene(true,OrdinaryCause::None); }
	catch (...) { _fatal = true; throw; }
}
void XeenEventFlow::drawInventory() {
	try { _frame = drawXeenInventory(_inventoryUnderlay,_inventoryFont,_catalog,_party,_inventory,_inventoryFeedback); }
	catch (...) { recoverInventory(); }
}
IndexedFrame XeenEventFlow::refuseInventorySave() {
	if (_dispatching || _fatal || !inventoryOpen()) return _frame;
	Scope scope(_dispatching);
	_inventoryFeedback = "Close inventory before F9; then press F9 again";
	drawInventory();
	return _frame;
}
void XeenEventFlow::confirmInventory() {
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
			_transferResult = xeenTransferItem(_party,s.source,*s.destination,s.category,*s.slot);
	}
	const bool success = _transferResult.status == XeenTransferStatus::Success;
	invalidateInventorySelection();
	if (success) { _inventory.slot.reset(); _inventory.record = {}; }
	_inventoryFeedback = xeenTransferMessage(_transferResult.status);
	// Result and disarming precede every callback, formatting operation and draw.
	if (reportInventory) reportInventory(_transferResult);
	if (success) refreshScene(true,OrdinaryCause::None);
}
IndexedFrame XeenEventFlow::handleInventory(const PlayerAction &action) {
	using Mode = XeenInventoryMode;
	try {
	if (_inventoryEpoch >= std::numeric_limits<std::uint64_t>::max()-2) {
		closeInventory(); _frame=_inventoryUnderlay; return _frame;
	}
	if (!inventoryOpen()) {
		if (_inventoryEpoch == std::numeric_limits<std::uint64_t>::max()) return _frame;
		advanceInventoryEpoch();
		_inventory.mode = Mode::Browse;
		if (_party.party.size()) _inventory.sourceOwner = _party.party.activeRosterIds()[0];
		_transferResult = {};
		_inventoryFeedback = "";
		_presenter.clear();
		refreshScene(true,OrdinaryCause::None);
		if (!inventoryOpen()) return _frame;
		std::cout << xeenInventoryInspection(_party);
		return _frame;
	}
	if (std::holds_alternative<CancelInteractionAction>(action) ||
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
			_inventoryFeedback = "Enter to confirm one transfer";
		}
	} else if (_inventory.mode == Mode::Confirm && std::holds_alternative<AcknowledgeAction>(action)) {
		confirmInventory();
	} else if (_inventory.mode != Mode::Browse) {
		_inventoryFeedback = "Escape to cancel";
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
		}
	} else if (const auto *slot = std::get_if<SelectInventorySlotAction>(&action)) {
		advanceInventoryEpoch();
		if (slot->slot < 9 && validInventorySource(false)) {
			_inventory.slot = slot->slot;
			_inventory.record = (*xeenInventoryItems(_party.roster.at(*_inventory.sourceOwner),_inventory.category))[slot->slot];
			_inventoryFeedback = "";
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
		if (_fatal) throw; // A failed clean-base recovery is already terminal.
		recoverInventory(); return _frame;
	}
}
}
