#ifndef MMODERN_GAMES_XEEN_SAVE_SNAPSHOT_H
#define MMODERN_GAMES_XEEN_SAVE_SNAPSHOT_H

#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenEncounterEntry.h"
#include "games/xeen/XeenCombatInputs.h"
#include "games/xeen/XeenGameplayContext.h"

#include <optional>
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
enum class XeenSaveItemState { Complete, LegacyV1MissingFields };

struct XeenSaveCombatSupplement {
	std::uint8_t owner = 0;
	XeenCombatInputs inputs;
};

struct XeenSaveCompletedEncounter {
	XeenEncounterEntry entry = XeenEncounterEntry::Diagnostic27;
	bool victory = true;
	bool accountingConsumed = true;
	XeenMonsterIdentity monster{{XeenSide::Clouds, 20}, 5};
	XeenGameplayContext context;
	std::array<XeenSaveCombatSupplement, 6> supplements{};
};

struct XeenSaveSnapshot {
	// Transient presence only; never stored on the wire or in live gameplay.
	XeenSaveItemState itemState = XeenSaveItemState::Complete;
	XeenSaveResourceSignature resources;
	XeenCamera camera;
	std::vector<std::uint8_t> activeRosterIds;
	std::array<XeenCharacter, XeenRoster::kCharacterCount> characters{};
	XeenCloudsQuestItems::Counts questItems{};
	XeenCloudsQuestFlags::Values questFlags{};
	XeenGameFlags::Storage gameFlags{};
	std::vector<XeenObjectIdentity> disabledObjects;
	std::vector<XeenEventIdentity> disabledEvents;
	std::optional<XeenSaveCompletedEncounter> completedEncounter;

	XeenSaveSnapshot() {
		for (std::size_t i = 0; i < characters.size(); ++i)
			characters[i].rosterId = static_cast<std::uint8_t>(i);
	}
};

} // namespace mmodern
#endif
