#ifndef MMODERN_XEEN_MONSTER_TREASURE_H
#define MMODERN_XEEN_MONSTER_TREASURE_H
#include "games/xeen/XeenCharacter.h"
#include <array>
#include <cstdint>
#include <optional>
namespace mmodern {
// Durable produced consequences. The party owns this value; runtime receipts
// and partially executed delivery never belong here.
struct XeenMonsterTreasureItem {
	std::uint8_t source = 0;
	XeenItem item;
	friend bool operator==(const XeenMonsterTreasureItem &a, const XeenMonsterTreasureItem &b) noexcept {
		return a.source == b.source && a.item.material == b.item.material && a.item.id == b.item.id &&
			a.item.state == b.item.state && a.item.frame == b.item.frame;
	}
};
struct XeenMonsterTreasure {
	std::uint32_t gold = 0, gems = 0, pendingMask = 0, pendingGold = 0;
	std::array<XeenMonsterTreasureItem,10> weapons{}, armor{};
	bool pending() const noexcept { return pendingMask != 0; }
	friend bool operator==(const XeenMonsterTreasure &a, const XeenMonsterTreasure &b) noexcept {
		return a.gold == b.gold && a.gems == b.gems && a.pendingMask == b.pendingMask &&
			a.pendingGold == b.pendingGold && a.weapons == b.weapons && a.armor == b.armor;
	}
	friend bool operator!=(const XeenMonsterTreasure &a, const XeenMonsterTreasure &b) noexcept { return !(a == b); }
};
// Structural validation only; the caller separately binds pending sources to
// canonical world-owned defeated/accounted Orcs.
void xeenValidateMonsterTreasure(const XeenMonsterTreasure &);
struct XeenConsequenceDraw;
enum class XeenMonsterDropOutcome { None, Item, ReferenceMiscellaneousDropLoss, CategoryCapacityLoss };
struct XeenMonsterDropCandidate {
	XeenMonsterTreasure treasure;
	XeenMonsterDropOutcome outcome = XeenMonsterDropOutcome::None;
	XeenMonsterTreasureItem generated;
	bool armor = false;
	XeenMonsterDropCandidate(const XeenMonsterTreasure &, unsigned source);
	bool service(XeenConsequenceDraw &);
private:
	enum class Step { Drop, Category, Subcategory, Id, Enchantment, Special, Charges, Store, Done };
	Step step = Step::Drop;
	unsigned category = 0, subcategory = 0;
};
enum class XeenMonsterDeliveryLoss { None, GloballyFull, NoEligibleRecipient, CategoryTailsFull };
struct XeenMonsterDeliveryRecord {
	XeenMonsterTreasureItem production;
	bool armor = false;
	std::optional<std::uint8_t> recipient;
	XeenMonsterDeliveryLoss loss = XeenMonsterDeliveryLoss::None;
};
struct XeenMonsterDeliveryCandidate {
	std::array<XeenCharacter,6> characters;
	XeenMonsterTreasure treasure;
	std::array<XeenMonsterDeliveryRecord,12> records{};
	unsigned count = 0;
	bool globallyFull = false;
};
XeenMonsterDeliveryCandidate xeenPrepareMonsterDelivery(const XeenMonsterTreasure &,
	const std::array<XeenCharacter,6> &);
XeenMonsterTreasure xeenPrepareMonsterGoldCredit(const XeenMonsterTreasure &);
}
#endif
