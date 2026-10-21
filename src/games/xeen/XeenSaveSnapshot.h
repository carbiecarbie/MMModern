#ifndef MMODERN_GAMES_XEEN_SAVE_SNAPSHOT_H
#define MMODERN_GAMES_XEEN_SAVE_SNAPSHOT_H

#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenEncounterEntry.h"
#include "games/xeen/XeenCombatInputs.h"
#include "games/xeen/XeenGameplayContext.h"
#include "games/xeen/XeenActor.h"
#include "games/xeen/XeenJourneyContent.h"
#include "games/xeen/XeenBarrierState.h"
#include "games/xeen/XeenObjectState.h"
#include "games/xeen/XeenExploration.h"
#include "games/xeen/XeenHeadState.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace mmodern {

// Content compatibility, not authentication. No path or resource payload.
struct XeenArchiveFingerprint {
	std::uint64_t size = 0;
	std::uint32_t crc32 = 0;
	friend bool operator==(XeenArchiveFingerprint a, XeenArchiveFingerprint b) {
		return a.size == b.size && a.crc32 == b.crc32;
	}
};

struct XeenSaveResourceSignature {
	XeenArchiveFingerprint clouds;
	std::optional<XeenArchiveFingerprint> darkside;
	friend bool operator==(const XeenSaveResourceSignature &a, const XeenSaveResourceSignature &b) {
		return a.clouds == b.clouds && a.darkside == b.darkside;
	}
};

// Temporary transfer values only. Gameplay keeps its existing live owners.
// Loading metadata, caches and all interpreter/presentation state are absent.

struct XeenSaveCombatSupplement {
	std::uint8_t owner = 0;
	XeenCombatInputs inputs;
};

// v13 per-map table (M69 decision 5). Every value is relative to its map's
// original resources: record indices are MOB/EVT order, cells are y*16+x of
// the entry's own physical DAT tile. Resources stay immutable.

// One slot of a map's monster table: every MOB record, then script slots.
// HP and wake state are rebuilt on every map and game load (decision 8).
struct XeenSaveMonster {
	int x = 0, y = 0;
	XeenActorLifecycle lifecycle = XeenActorLifecycle::Present;
	XeenActorStatus status = XeenActorStatus::Physical;
	bool accounted = false;
	// -1 binds a MOB record or an unresolved script slot; a materialized
	// script slot stores its MON type.
	std::int16_t spawnedType = -1;
	friend bool operator==(const XeenSaveMonster &a, const XeenSaveMonster &b) {
		return a.x == b.x && a.y == b.y && a.lifecycle == b.lifecycle && a.status == b.status &&
			a.accounted == b.accounted && a.spawnedType == b.spawnedType;
	}
};

struct XeenSaveMonsterTable {
	std::uint16_t originalCount = 0;
	std::vector<XeenSaveMonster> slots;
	friend bool operator==(const XeenSaveMonsterTable &a, const XeenSaveMonsterTable &b) {
		return a.originalCount == b.originalCount && a.slots == b.slots;
	}
};

// MOB object position and effective flip; (-128,-128) is a removed object,
// as the original's Remove writes it, and is canonical with no flip.
struct XeenSaveObjectOverride {
	std::uint16_t record = 0;
	int x = 0, y = 0;
	bool flipped = false;
	friend bool operator==(const XeenSaveObjectOverride &a, const XeenSaveObjectOverride &b) {
		return a.record == b.record && a.x == b.x && a.y == b.y && a.flipped == b.flipped;
	}
};

// MOB wall-item position, written by the MoveWallObj Event.
struct XeenSaveWallItemOverride {
	std::uint16_t record = 0;
	int x = 0, y = 0;
	friend bool operator==(const XeenSaveWallItemOverride &a, const XeenSaveWallItemOverride &b) {
		return a.record == b.record && a.x == b.x && a.y == b.y;
	}
};

// Effective DAT word and attribute byte of one cell, with the original
// preimage they replace. An entry always differs from its preimage. Producers:
// doors (wall faces, unlocked bit), AlterMap and outdoor AlterCellFlag (wall
// word), indoor AlterCellFlag (surface bits 0-2). Outdoor writes to a
// neighbouring tile last only for the visit and are never saved.
struct XeenSaveCellOverride {
	std::uint8_t cell = 0;
	std::uint16_t word = 0;
	std::uint8_t attributes = 0;
	std::uint16_t originalWord = 0;
	std::uint8_t originalAttributes = 0;
	friend bool operator==(const XeenSaveCellOverride &a, const XeenSaveCellOverride &b) {
		return a.cell == b.cell && a.word == b.word && a.attributes == b.attributes &&
			a.originalWord == b.originalWord && a.originalAttributes == b.originalAttributes;
	}
};

// Effective EVT record opcode. 0 is a deleted record (Remove, MakeNothingHere,
// AlterEvent to 0); a nonzero value is an AlterEvent replacement.
struct XeenSaveEventOverride {
	std::uint16_t record = 0;
	std::uint8_t opcode = 0;
	friend bool operator==(const XeenSaveEventOverride &a, const XeenSaveEventOverride &b) {
		return a.record == b.record && a.opcode == b.opcode;
	}
};

struct XeenSaveMapState {
	XeenMapIdentity id;
	// Absence (never staged) is distinct from an empty table.
	std::optional<XeenSaveMonsterTable> monsters;
	std::vector<XeenSaveObjectOverride> objects;
	std::vector<XeenSaveWallItemOverride> wallItems;
	std::vector<XeenSaveCellOverride> cells;
	std::vector<XeenSaveEventOverride> events;
	// This tile's explored bits (y*16+x); presence is kept even when zero.
	std::optional<std::array<std::uint8_t, 32>> explored;
	std::optional<std::uint8_t> tavernTips;
	bool empty() const noexcept {
		return !monsters && objects.empty() && wallItems.empty() && cells.empty() && events.empty() && !explored && !tavernTips;
	}
	friend bool operator==(const XeenSaveMapState &a, const XeenSaveMapState &b) {
		return a.id == b.id && a.monsters == b.monsters && a.objects == b.objects && a.wallItems == b.wallItems &&
			a.cells == b.cells && a.events == b.events && a.explored == b.explored && a.tavernTips == b.tavernTips;
	}
};

struct XeenSaveJourney {
	XeenEncounterEntry entry = XeenEncounterEntry::Journey;
	std::uint16_t schema = 1, content = 1;
	std::optional<XeenGameplayContext> context;
	std::array<XeenSaveCombatSupplement, 30> supplements{};
	std::uint32_t skeletonSeed = 0;
	std::optional<XeenJourneyRandomState> random;
	// v17: the purse only; pending treasure is running-program state, never saved.
	std::optional<XeenPurse> purse;
	std::optional<XeenWorldFlags> worldFlags;
	std::optional<XeenServiceEconomy> serviceEconomy;
};

struct XeenSaveSnapshot {
	XeenSaveResourceSignature resources;
	// v7: absent only for explicit developer loose saves. Never a filesystem name.
	std::optional<std::string> name;
	std::uint16_t food=0;
	XeenSoundOptions soundOptions;
	XeenCamera camera;
	std::vector<std::uint8_t> activeRosterIds;
	std::array<XeenCharacter, XeenRoster::kCharacterCount> characters{};
	XeenCloudsQuestItems::Counts questItems{};
	XeenCloudsQuestFlags::Values questFlags{};
	XeenGameFlags::Storage gameFlags{};
	std::optional<XeenHeadCache> headCache;
	std::optional<XeenSaveJourney> journey;
	// v13: strictly ordered by identity, no empty entry.
	std::vector<XeenSaveMapState> maps;

	const XeenSaveMapState *find(XeenMapIdentity id) const {
		for (const auto &entry : maps) if (entry.id == id) return &entry;
		return nullptr;
	}
	const XeenSaveMapState &map(XeenMapIdentity id) const {
		if (const auto *entry = find(id)) return *entry;
		throw std::out_of_range("Saved map state is absent");
	}
	// Finds or inserts the entry, keeping the identity order.
	XeenSaveMapState &map(XeenMapIdentity id) {
		auto at = maps.begin();
		while (at != maps.end() && at->id < id) ++at;
		if (at == maps.end() || at->id != id) at = maps.insert(at, XeenSaveMapState{id, {}, {}, {}, {}, {}, {}, {}});
		return *at;
	}

	XeenSaveSnapshot() {
		for (std::size_t i = 0; i < characters.size(); ++i)
			characters[i].rosterId = static_cast<std::uint8_t>(i);
	}
};

} // namespace mmodern
#endif
