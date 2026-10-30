#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_VALUES_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_VALUES_H

#include "games/xeen/XeenEventDecoder.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenParty.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace mmodern {

// Script value operands shared by If (8-10), TakeOrGive (12, 28, 29, 38),
// GiveMulti (20) and SetVar (27): ifProc (WORLD/XEEN.DAT 0x2162e..0x21bd8),
// setValue (0x23945..0x23c93) and giveTake (0x2200c..0x23315). Values are
// zero-extended unless a rule below says otherwise. Shared by both sides; the
// Clouds banks are selected by the current side.

inline bool xeenEventUses32(unsigned mode) noexcept { return mode==16 || mode==34 || mode==100; }
inline bool xeenEventUses16(unsigned mode) noexcept { return mode==25 || mode==35 || mode==101 || mode==106; }

// Run-local script globals, reset when an Event run starts (0x26789, 0x26aaa..0x26ab3).
struct XeenEventScriptValues {
	std::uint8_t damageType = 0;   // 0x97f5, SetVar 83 and giveTake 8/82
	int itemType = 0;              // 0x8c74, ItemType (45) and give 66
	std::uint8_t combatTarget = 1; // 0x98d7, SetChar 1..6; excluded when the selection is 8
	std::uint8_t lockCounter = 0;  // 0x9755, giveExt's lock state (reset at 0x26a96)
	friend bool operator==(const XeenEventScriptValues &a,const XeenEventScriptValues &b) noexcept {
		return a.damageType==b.damageType && a.itemType==b.itemType && a.combatTarget==b.combatTarget &&
			a.lockCounter==b.lockCounter;
	}
};

// The roster supplements an Event value writes (permanent and temporary
// attributes, resistances, AC bonus and experience).
struct XeenEventValueAccess {
	static XeenMutableOptional<XeenCombatInputs> &inputs(XeenPartyState &, std::uint8_t owner);
};

struct XeenEventValueOwners {
	XeenPartyState &party;
	XeenGameFlags &flags;
	XeenCamera &camera;
	XeenEventScriptValues &script;
    std::uint16_t *prefixLight=nullptr;
};

// A failed skill, award, condition or spell test, and an unhandled action,
// leave ifProc's local uninitialised (0x216c0, 0x216d3, 0x21703, 0x21729,
// 0x21b84). Its value is then the residue of the previous frame at that depth.
struct XeenEventProbe {
	std::uint32_t value = 0;
	bool residue = false;
};

// The value a residue holds. Before the first test of an instruction the
// frame holds the interpreter's saved SI:DI from the per-instruction message
// copy (0x26c26..0x26c4d, 0xacd6..0xacda): DI is the record's offset in the
// Event file and SI is unknown. A later test in the same loop sees the
// previous test's value; any other call in between makes it unknown.
struct XeenEventResidue {
	enum class Kind { RecordOffset, Value, Unknown };
	Kind kind = Kind::RecordOffset;
	std::uint32_t value = 0;
};

// Why a value operation is unsupported: the mode, an index outside its bank,
// the side or other context, or a missing selected member.
enum class XeenEventRefusal { Mode, Index, Context, Member };

// nullopt: the action reads state MMModern does not own yet (reason set).
std::optional<XeenEventProbe> xeenEventProbe(const XeenEventValueOwners &, unsigned action,
	std::uint32_t value, std::size_t member, std::string &reason, XeenEventRefusal *refusal = nullptr);
// nullopt: the outcome depends on the unknown part of a residue.
std::optional<bool> xeenEventCompare(const XeenEventProbe &, const XeenEventResidue &,
	std::uint32_t value, XeenEventComparison);
// The residue left behind by a completed test.
XeenEventResidue xeenEventAfterProbe(const XeenEventProbe &, const XeenEventResidue &);

enum class XeenEventWriteStatus { Applied, Failed, NotEnough, Unsupported };
struct XeenEventWrite {
	XeenEventWriteStatus status = XeenEventWriteStatus::Applied;
	bool sparkle = false;          // spellFX for the member after the write (0x23278)
	std::uint8_t consumable = 0;   // NotEnough: 0 gold, 1 gems, 2 food (0x1e941)
	std::string reason;
	XeenEventRefusal refusal = XeenEventRefusal::Mode;
};

// setValue: plain stores, byte/word/dword as the original field.
XeenEventWrite xeenEventSetValue(const XeenEventValueOwners &, unsigned mode, std::uint32_t value, std::size_t member);
// giveTake's take half and give half for every mode that needs no draw,
// presentation, treasure, damage or time owner. The interpreter owns those.
XeenEventWrite xeenEventTake(const XeenEventValueOwners &, unsigned mode, std::uint32_t value, std::size_t member);
XeenEventWrite xeenEventGive(const XeenEventValueOwners &, unsigned mode, std::uint32_t value, std::size_t member);

// Random amounts of give 100 and GiveMulti 100/101/106 pass only the operand's
// low word to the random helper (0x232bc, 0x23770..0x237de). The helper
// (0x45b9a) returns min + r % (max + 1 - min) for r in 0..32767, dividing by a
// signed word (0x45ba3..0x45bac). With min 1 and word w the outcomes are 1..w
// below 0x8000 and 1..(0x10000 - w) from 0x8000; w = 0 divides by zero, which
// MMModern refuses (nullopt).
inline std::optional<std::uint32_t> xeenEventWordDrawBound(std::uint32_t operand) noexcept {
	const std::uint16_t word = static_cast<std::uint16_t>(operand);
	if (!word) return std::nullopt;
	return word < 0x8000 ? word : 0x10000u - word;
}
// The drawn word is sign-extended (cwd) before a 32-bit gold or gems addition
// (0x232c9, 0x23791, 0x237e6): an amount of 32768 subtracts 32768.
inline std::uint32_t xeenEventSignedWord(std::uint32_t amount) noexcept {
	return static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(amount)));
}

// Character and party fields an Event may change, for candidate publication
// and live-path preimages. Identity, name, birth and spell selection are not.
inline void xeenEventCopyOwnedCharacter(XeenCharacter &d, const XeenCharacter &s) {
	d.sex = s.sex; d.race = s.race; d.characterClass = s.characterClass;
	d.intellect.permanent = s.intellect.permanent; d.intellect.temporary = s.intellect.temporary;
	d.personality.permanent = s.personality.permanent; d.personality.temporary = s.personality.temporary;
	d.endurance.permanent = s.endurance.permanent; d.endurance.temporary = s.endurance.temporary;
	d.permanentLevel = s.permanentLevel; d.temporaryLevel = s.temporaryLevel; d.temporaryAge = s.temporaryAge;
	d.skills = s.skills; d.learnedSpells = s.learnedSpells;
	d.weapons = s.weapons; d.armor = s.armor; d.accessories = s.accessories; d.miscellaneous = s.miscellaneous;
	d.currentHp = s.currentHp; d.currentSp = s.currentSp; d.conditions = s.conditions; d.awards = s.awards;
	d.characterFlags = s.characterFlags;
}

} // namespace mmodern

#endif
