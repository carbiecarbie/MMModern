#ifndef MMODERN_XEEN_MONSTER_TREASURE_H
#define MMODERN_XEEN_MONSTER_TREASURE_H
#include "games/xeen/XeenCharacter.h"
#include <array>
#include <cstdint>
#include <optional>
#include "games/xeen/XeenMutation.h"
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenItemGeneration.h"
namespace mmodern {
enum class XeenRewardCategory : std::uint8_t { Weapons, Armor, Accessories, Miscellaneous };
// The party's carried gold and gems (DOS DS:f00a/f00e), saved with the party.
// Both are 32-bit and wrap modulo 2^32 like the original add/carry pairs.
struct XeenPurse {
	XeenMutable<std::uint32_t> gold = 0, gems = 0;
	friend bool operator==(const XeenPurse &a, const XeenPurse &b) noexcept { return a.gold == b.gold && a.gems == b.gems; }
	friend bool operator!=(const XeenPurse &a, const XeenPurse &b) noexcept { return !(a == b); }
};
// Pending treasure (DOS DS:9970..9a19): four ten-slot category arrays filled at
// the first empty slot, pending gold/gems and the readiness flag. Running-program
// state, never saved; the arrays carry no contributor identity.
struct XeenTreasureStaging {
	static constexpr std::size_t kSlots = 10;
	using Items = std::array<XeenItem, kSlots>;
	XeenMutable<std::uint32_t> gold = 0, gems = 0;
	XeenMutable<bool> ready = false;
	Items weapons{}, armor{}, accessories{}, miscellaneous{};
	Items &items(XeenRewardCategory) noexcept;
	const Items &items(XeenRewardCategory) const noexcept;
	// DOS tests the equipment ID byte, but the Misc container byte (0x2c4f5..0x2c53c).
	static bool emptySlot(const XeenItem &, XeenRewardCategory) noexcept;
	bool storedItems() const noexcept;
	bool full(XeenRewardCategory) const noexcept;
	// The first empty slot of the category receives the item and readiness is set;
	// a full category refuses it (monster 0x2c402..0x2c540, Event 0x287f9..0x28925).
	bool insert(XeenRewardCategory, const XeenItem &) noexcept;
	// DOS delivery and Event stash trigger (0x2412e..0x24149): currency or readiness.
	bool pending() const noexcept { return ready || gold || gems; }
	bool dormant() const noexcept { return storedItems() && !pending(); }
	// Run, Load and New Game clear currency and readiness and keep the arrays.
	void clearPending() noexcept { gold = 0; gems = 0; ready = false; }
	friend bool operator==(const XeenTreasureStaging &a, const XeenTreasureStaging &b) noexcept {
		const auto same = [](const Items &x, const Items &y) {
			for (std::size_t i = 0; i < x.size(); ++i)
				if (x[i].material != y[i].material || x[i].id != y[i].id || x[i].state != y[i].state || x[i].frame != y[i].frame) return false;
			return true;
		};
		return a.gold == b.gold && a.gems == b.gems && a.ready == b.ready && same(a.weapons, b.weapons) &&
			same(a.armor, b.armor) && same(a.accessories, b.accessories) && same(a.miscellaneous, b.miscellaneous);
	}
	friend bool operator!=(const XeenTreasureStaging &a, const XeenTreasureStaging &b) noexcept { return !(a == b); }
};
// The staging value with its own revision. The running program owns one
// (XeenProgramState); prepared work stages on a copy taken at a revision and
// publishes it only while that revision is still current.
class XeenTreasureStagingState {
public:
	XeenTreasureStagingState() = default;
	XeenTreasureStagingState(const XeenTreasureStagingState &) = delete;
	XeenTreasureStagingState &operator=(const XeenTreasureStagingState &) = delete;
	const XeenTreasureStaging &value() const noexcept { return _value; }
	std::uint64_t revision() const noexcept { return _revision; }
	void check(std::uint64_t base) const;
	// Callers check the base revision first; the store itself cannot fail.
	void publish(const XeenTreasureStaging &value) noexcept { _value = value; ++_revision; }
	// A successful game entry: currency and readiness clear, the arrays stay.
	void enterGame() noexcept { _value.clearPending(); ++_revision; }
	// Restores the exact value and revision captured before an entry that failed.
	struct Saved { XeenTreasureStaging value; std::uint64_t revision = 0; };
	Saved save() const { return {_value, _revision}; }
	void restore(const Saved &saved) noexcept { _value = saved.value; _revision = saved.revision; }
private:
	XeenTreasureStaging _value;
	std::uint64_t _revision = 0;
};
struct XeenConsequenceDraw;
enum class XeenMonsterDropOutcome { None, Item, CategoryCapacityLoss };
struct XeenMonsterDropCandidate {
	XeenTreasureStaging treasure;
	XeenMonsterDropOutcome outcome = XeenMonsterDropOutcome::None;
	XeenItem generated;
	XeenRewardCategory category{};
	// The staging revision this copy was taken at; publication checks it.
	std::uint64_t base = 0;
	explicit XeenMonsterDropCandidate(const XeenTreasureStaging &, std::uint64_t base = 0);
	explicit XeenMonsterDropCandidate(const XeenTreasureStagingState &state) :
		XeenMonsterDropCandidate(state.value(), state.revision()) {}
	bool service(XeenConsequenceDraw &);
private:
	enum class Step { Drop, Generate, Store, Done };
	Step step = Step::Drop;
	XeenItemCandidate item{1};
};
// Final acknowledgment of a delivery: credits the staged gold/gems to the purse
// modulo 2^32 (DOS 0x2486a..0x24884); the staging is then emptied (0x2489f..0x248d7).
XeenPurse xeenCreditTreasure(const XeenPurse &, const XeenTreasureStaging &) noexcept;
}
#endif
