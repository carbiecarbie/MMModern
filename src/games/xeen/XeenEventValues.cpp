#include "games/xeen/XeenEventValues.h"

#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenSkills.h"

#include <functional>
#include <limits>
#include <stdexcept>

namespace mmodern {

XeenMutableOptional<XeenCombatInputs> &XeenEventValueAccess::inputs(XeenPartyState &party, std::uint8_t owner) {
	return party.roster._combatInputs.at(owner);
}

namespace {

constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();

std::uint8_t ownerOf(const XeenPartyState &party, std::size_t member) {
	if (member >= party.party.size()) throw std::invalid_argument("Event value member is outside the party");
	return party.party.activeRosterIds()[member];
}

XeenCharacter &character(const XeenEventValueOwners &o, std::size_t member) {
	return o.party.roster.at(ownerOf(o.party, member));
}

// Modes that touch only party-wide state; they run even without a selected member.
bool partyMode(unsigned mode, std::uint32_t value) noexcept {
	switch (mode) {
	case 0: case 17: case 20: case 25: case 34: case 35: case 44: case 65: case 69: case 70: case 71: case 72:
	case 73: case 74: case 76: case 79: case 83: case 84: case 85: case 93: case 94: case 99: case 100:
	case 103:
		return true;
	default:
		return false;
	}
}

// The selected member's character, or a scratch record when a party-wide
// mode runs without one (the original indexes its party array regardless).
XeenCharacter &characterOrScratch(const XeenEventValueOwners &o, std::size_t member) {
	static thread_local XeenCharacter scratch;
	if (member < o.party.party.size()) return character(o, member);
	scratch = XeenCharacter{};
	return scratch;
}

XeenCombatInputs *inputs(const XeenEventValueOwners &o, std::size_t member) {
	auto &value = XeenEventValueAccess::inputs(o.party, ownerOf(o.party, member));
	return value ? &*value : nullptr;
}

// A byte field of the original CHR record, stored as 0..255 by MMModern.
struct ByteField {
	std::function<std::uint8_t()> get;
	std::function<void(std::uint8_t)> set;
};

template<class T> ByteField field(T &value) {
	return {[&value] { return static_cast<std::uint8_t>(static_cast<int>(value)); },
		[&value](std::uint8_t v) { value = v; }};
}

// Per-character byte operands shared by ifProc, setValue and giveTake:
// 10/77 AC bonus (+0x22), 11 level bonus (+0x24), 12 age bonus (+0x26),
// 37..43 temporary and 45..51 permanent attributes (+0x14..+0x21),
// 52..63 permanent then temporary resistances (+0x137..+0x142), 64 level (+0x23).
std::optional<ByteField> characterByte(const XeenEventValueOwners &o, unsigned mode, std::size_t member,
		std::string &reason) {
	auto &c = character(o, member);
	auto *in = inputs(o, member);
	const auto needInputs = [&]() -> bool {
		if (!in) reason = "Event character inputs are absent";
		return in != nullptr;
	};
	switch (mode) {
	case 10: case 77: if (!needInputs()) return std::nullopt; return field(in->temporaryAc);
	case 11: return field(c.temporaryLevel);
	case 12: return field(c.temporaryAge);
	case 64: return field(c.permanentLevel);
	case 38: return field(c.intellect.temporary);
	case 39: return field(c.personality.temporary);
	case 40: return field(c.endurance.temporary);
	case 46: return field(c.intellect.permanent);
	case 47: return field(c.personality.permanent);
	case 48: return field(c.endurance.permanent);
	default: break;
	}
	if (!needInputs()) return std::nullopt;
	switch (mode) {
	case 37: return field(in->might.temporary);
	case 41: return field(in->speed.temporary);
	case 42: return field(in->accuracy.temporary);
	case 45: return field(in->might.permanent);
	case 49: return field(in->speed.permanent);
	case 50: return field(in->accuracy.permanent);
	case 43: case 51:
		if (!in->luck) { reason = "Event Luck input is absent"; return std::nullopt; }
		return mode == 43 ? field(in->luck->temporary) : field(in->luck->permanent);
	case 55: case 61:
		if (!in->poisonResistance) { reason = "Event poison resistance is absent"; return std::nullopt; }
		return mode == 55 ? field(in->poisonResistance->permanent) : field(in->poisonResistance->temporary);
	default: break;
	}
	if (mode >= 52 && mode <= 63) {
		if (!in->resistances) { reason = "Event resistances are absent"; return std::nullopt; }
		auto &r = *in->resistances;
		switch (mode) {
		case 52: return field(r.firePermanent);
		case 53: return field(r.electricalPermanent);
		case 54: return field(r.coldPermanent);
		case 56: return field(r.energyPermanent);
		case 57: return field(r.magicPermanent);
		case 58: return field(r.fireTemporary);
		case 59: return field(r.electricalTemporary);
		case 60: return field(r.coldTemporary);
		case 62: return field(r.energyTemporary);
		case 63: return field(r.magicTemporary);
		default: break;
		}
	}
	return std::nullopt;
}

bool characterByteMode(unsigned mode) noexcept {
	return mode == 10 || mode == 11 || mode == 12 || mode == 77 || (mode >= 37 && mode <= 64 && mode != 44);
}

// The party PTY word block 0xeff8..0xf003: light, unused, fire, electricity, cold, poison.
std::optional<std::size_t> partyWord(unsigned mode) noexcept {
	switch (mode) {
	case 70: return 0;
	case 71: return 2;
	case 72: return 3;
	case 73: return 4;
	case 17: return 5;
	default: return std::nullopt;
	}
}

XeenGameplayContext *context(const XeenEventValueOwners &o, std::string &reason) {
	if (!o.party.encounterContext) { reason = "Event gameplay context is absent"; return nullptr; }
	return &*o.party.encounterContext;
}

XeenMonsterTreasure *purse(const XeenEventValueOwners &o, std::string &reason) {
	if (!o.party.monsterTreasure) { reason = "Event party purse is absent"; return nullptr; }
	return &*o.party.monsterTreasure;
}

XeenEventWrite unsupported(std::string reason, XeenEventRefusal refusal = XeenEventRefusal::Mode) {
	XeenEventWrite result;
	result.status = XeenEventWriteStatus::Unsupported;
	result.reason = std::move(reason);
	result.refusal = refusal;
	return result;
}
constexpr auto kIndex = XeenEventRefusal::Index;
constexpr auto kContext = XeenEventRefusal::Context;

XeenEventWrite applied(bool sparkle = false) {
	XeenEventWrite result;
	result.sparkle = sparkle;
	return result;
}

// Award nibbles (0x3ba44..0x3bb05): byte k of CHR +0x39 holds award k in its
// low nibble and award k+64 in its high one; 73 and 81 use bytes 62 and 63.
// MMModern stores the nibbles expanded; byte 9's low half is the whole byte.
struct AwardSlot { std::size_t index; bool high; std::size_t byte; };

AwardSlot awardSlot(std::uint32_t id) noexcept {
	const std::size_t byte = id == 73 ? 62 : id == 81 ? 63 : id % 64;
	const bool high = id >= 64;
	return {high ? byte + 64 : byte, high, byte};
}

bool hasAward(const XeenCharacter &c, std::uint32_t id) noexcept {
	const auto slot = awardSlot(id);
	const unsigned value = c.awards[slot.index];
	return slot.high ? value != 0 : (slot.byte == 9 ? (value & 0x0f) != 0 : value != 0);
}

void orNibble(XeenCharacter &c, std::size_t byte, bool high, unsigned bits) {
	if (!high && byte == 9) { c.awards[9] = static_cast<std::uint8_t>(c.awards[9] | bits); return; }
	if (high && byte == 9) { c.awards[9] = static_cast<std::uint8_t>(c.awards[9] | (bits << 4)); return; }
	auto &value = c.awards[high ? byte + 64 : byte];
	value = static_cast<std::uint8_t>((value | bits) & 0x0f);
}

// setAward: a grant ORs 1 into the award's nibble. A removal ORs 0xf into the
// other nibble of the same byte instead of clearing (original defect).
void setAward(XeenCharacter &c, std::uint32_t id, bool give) {
	const auto slot = awardSlot(id);
	if (give) orNibble(c, slot.byte, slot.high, 1);
	else orNibble(c, slot.byte, !slot.high, 0x0f);
}

// The class's spell book (0x21729/0x22182/0x2280b). Knights, Robbers, Ninjas
// and Barbarians reuse whichever book the program last selected (global
// 0x9604); that value is not owned here.
std::optional<std::size_t> spellSlot(const XeenCharacter &c, std::uint32_t spell, std::string &reason, bool &found) {
	found = false;
	const auto category = XeenLearnedSpellRules::categoryForClass(c.characterClass);
	if (!category) { reason = "Event spell for a class without spells uses a stale book"; return std::nullopt; }
	if (!c.learnedSpells) { reason = "Event spell book is absent"; return std::nullopt; }
	if (spell > 0xff) return std::size_t(0);
	const auto slot = XeenLearnedSpellRules::slotForSpell(*category, static_cast<std::uint8_t>(spell));
	found = slot.has_value();
	return slot ? *slot : 0;
}

// Condition bytes: 16 clears all, 6 (In Love) sets 1, others increment; a
// fatal condition (13..15) zeroes positive HP (0x22782, 0x239ed).
XeenEventWrite applyCondition(XeenCharacter &c, std::uint32_t value) {
	if (value > 16) return unsupported("Event condition index is outside the conditions", kIndex);
	if (value == 16) for (std::size_t n = 0; n < c.conditions.size(); ++n) c.conditions[n] = 0;
	else if (value == 6) c.conditions[6] = 1;
	else c.conditions[value] = static_cast<std::uint8_t>(c.conditions[value] + 1);
	if (value >= 13 && value <= 15 && c.currentHp > 0) c.currentHp = 0;
	return applied();
}

bool canonicalEffects(const XeenGameplayContext &c) noexcept {
	return c.effects[XeenPartyEffect::AutomapOn] <= 1; // The Day spells write level bytes to the other flags/counts.
}

std::uint32_t signedWord(int value) noexcept {
	return static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(value)));
}

} // namespace

std::optional<XeenEventProbe> xeenEventProbe(const XeenEventValueOwners &o, unsigned action,
		std::uint32_t value, std::size_t member, std::string &reason, XeenEventRefusal *refusal) {
	const auto refuse = [&](const char *why, XeenEventRefusal kind) {
		reason = why; if (refusal) *refusal = kind; return std::optional<XeenEventProbe>{};
	};
	if (refusal) *refusal = XeenEventRefusal::Mode;
	const auto known = [](std::uint32_t v) { return std::optional<XeenEventProbe>(XeenEventProbe{v, false}); };
	const auto residue = [] { return std::optional<XeenEventProbe>(XeenEventProbe{0, true}); };
	const auto side = o.camera.mapId.side;
	if (member >= o.party.party.size() && !partyMode(action, value)) {
		return refuse("Event condition requires an active party member", XeenEventRefusal::Member);
	}
	// ifProc 10 and 12 compute the armor class and age; the setters use the bonus bytes.
	if (characterByteMode(action) && action != 10 && action != 12) {
		const auto f = characterByte(o, action, member, reason);
		if (!f) return std::nullopt;
		return known(f->get());
	}
	switch (action) {
	case 3: return known(static_cast<std::uint8_t>(character(o, member).sex));
	case 4: return known(static_cast<std::uint8_t>(character(o, member).race));
	case 5: return known(static_cast<std::uint8_t>(character(o, member).characterClass));
	case 8: return known(signedWord(character(o, member).currentHp));
	case 9: return known(signedWord(character(o, member).currentSp));
	case 13:
		if (value >= 18) return refuse("Event skill index is outside the skills", kIndex);
		return character(o, member).skills[value] ? known(value) : residue();
	case 15:
		if (value > 0xff) return refuse("Event award is outside the award table", kIndex);
		return hasAward(character(o, member), value) ? known(value) : residue();
	case 16: {
		auto *in = inputs(o, member);
		if (!in) { reason = "Event character inputs are absent"; return std::nullopt; }
		return known(in->experience);
	}
	case 18: {
		if (value > 16) return refuse("Event condition index is outside the conditions", kIndex);
		const auto &c = character(o, member);
		if (value == 16) return known(value);
		return c.conditions[value] ? known(value) : residue();
	}
	case 19: {
		bool found = false;
		const auto slot = spellSlot(character(o, member), value, reason, found);
		if (!slot) return std::nullopt;
		return found && (*character(o, member).learnedSpells)[*slot] ? known(value) : residue();
	}
	case 20:
		if (side != XeenSide::Clouds || value >= XeenGameFlags::kCount)
			return refuse("Event game flag is outside the Clouds bank", side != XeenSide::Clouds ? kContext : kIndex);
		return known(o.flags.isSet(static_cast<int>(value)) ? value : kNone);
	case 21: {
		const auto &c = character(o, member);
		if (value >= 82) {
			const auto index = XeenCloudsQuestItems::indexForItemId(value);
			if (side != XeenSide::Clouds || !index)
				return refuse("Event quest item is outside the Clouds items", side != XeenSide::Clouds ? kContext : XeenEventRefusal::Mode);
			return known(o.party.questItems.at(*index) & 0xff ? value : kNone);
		}
		// Each slot tests one category; an empty slot matches an ID of zero.
		for (std::size_t slot = 0; slot < XeenCharacter::kEquipmentSlotsPerCategory; ++slot) {
			if (value < 35 ? c.weapons[slot].id == value : value < 49 ? c.armor[slot].id == value - 35 :
				value < 60 ? c.accessories[slot].id == value - 49 : c.miscellaneous[slot].material == value - 60)
				return known(value);
		}
		return known(kNone);
	}
	case 25: case 76: case 85: case 93: {
		const auto *c = context(o, reason);
		if (!c) return std::nullopt;
		if (action == 25) return known(c->minutes);
		if (action == 85) return known(c->year);
		if (action == 76) return known(signedWord(c->day));
		// 0x21ad1: the remainder of day/10, not the quotient.
		return known(signedWord(c->day % 10));
	}
	case 34: case 35: {
		const auto *p = purse(o, reason);
		if (!p) return std::nullopt;
		return known(action == 34 ? p->gold : p->gems);
	}
	case 65: return known(o.party.food);
	case 69: case 79: case 94: case 70: case 71: case 72: case 73: case 17: {
		const auto *c = context(o, reason);
		if (!c) return std::nullopt;
		if (action == 69) return known(c->effects[XeenPartyEffect::Levitate] ? 1 : 0); // 0x21a36: a flag, not the count
		if (action == 79) return known(c->effects[XeenPartyEffect::WizardEye]);
		if (action == 94) return known(c->effects[XeenPartyEffect::WalkOnWater]);
		return known(c->lightAndResistances[*partyWord(action)]);
	}
	case 78: case 81: {
		const auto *c = context(o, reason);
		if (!c) return std::nullopt;
		const auto &ch = character(o, member);
		// 0x21a6c/0x21a8e: maximum >= current, compared as unsigned words.
		const int maximum = action == 78 ? XeenCharacterRules::maxHp(ch, {c->year}) : XeenCharacterRules::maxSp(ch, {c->year});
		const int current = action == 78 ? int(ch.currentHp) : int(ch.currentSp);
		return known(std::uint16_t(maximum) >= std::uint16_t(current) ? 1 : 0);
	}
	case 84: return known(static_cast<std::uint8_t>(static_cast<XeenDirection>(o.camera.direction)));
	case 86: case 87: case 88: case 89: case 90: case 91: case 92: {
		const auto *c = context(o, reason);
		if (!c) return std::nullopt;
		const auto owner = ownerOf(o.party, member);
		const auto &in = o.party.roster.combatInputs(owner);
		return known(static_cast<std::uint16_t>(XeenCharacterRules::sheetStat(character(o, member),
			in ? &*in : nullptr, action - 86, {c->year})));
	}
	case 99:
		if (value >= 18) return refuse("Event skill index is outside the skills", kIndex);
		return known(xeenPartyHasSkill(o.party, value) ? value : kNone);
	case 102: return known(signedWord(XeenCharacterRules::thievery(character(o, member))));
	case 103:
		if (!o.party.worldFlags || value >= XeenWorldFlags::kCount)
			return refuse("Event world flag is outside the world bank", kIndex);
		return known(o.party.worldFlags->values[value] ? value : kNone);
	case 104:
		if (side != XeenSide::Clouds || !XeenCloudsQuestFlags::validIndex(value))
			return refuse("Event quest flag is outside the Clouds bank", side != XeenSide::Clouds ? kContext : kIndex);
		return known(o.party.questFlags.isSet(value) ? value : kNone);
	case 105:
		if (side != XeenSide::Clouds) return refuse("Megacredits are a Clouds counter", kContext);
		return known(o.party.questItems.at(26) & 0xff);
	case 107:
		reason = "Darkside character flags are not supported yet";
		return std::nullopt;
	case 44:
		reason = "Action 44 is a presentation";
		return std::nullopt;
	case 10: {
		const auto *c = context(o, reason);
		if (!c) return std::nullopt;
		const auto owner = ownerOf(o.party, member);
		const auto &in = o.party.roster.combatInputs(owner);
		return known(static_cast<std::uint16_t>(XeenCharacterRules::sheetArmorClass(character(o, member),
			in ? &*in : nullptr, {c->year,c->effects[XeenPartyEffect::Bless]})));
	}
	case 12: {
		const auto *c = context(o, reason);
		if (!c) return std::nullopt;
		return known(static_cast<std::uint16_t>(XeenCharacterRules::sheetAge(character(o, member), {c->year})));
	}
	default:
		return residue();
	}
}

std::optional<bool> xeenEventCompare(const XeenEventProbe &probe, const XeenEventResidue &residue,
		std::uint32_t value, XeenEventComparison comparison) {
	const auto compare = [&](std::uint32_t actual) {
		switch (comparison) {
		case XeenEventComparison::GreaterOrEqual: return actual >= value;
		case XeenEventComparison::Equal: return actual == value;
		case XeenEventComparison::LessOrEqual: return actual <= value;
		}
		return false;
	};
	if (!probe.residue) return compare(probe.value);
	if (residue.kind == XeenEventResidue::Kind::Value) return compare(residue.value);
	if (residue.kind == XeenEventResidue::Kind::Unknown) return std::nullopt;
	// SI:DI with DI the record offset and SI unknown.
	const std::uint32_t low = residue.value & 0xffff;
	switch (comparison) {
	case XeenEventComparison::Equal:
		if ((value & 0xffff) != low) return false;
		return std::nullopt;
	case XeenEventComparison::GreaterOrEqual:
		if (value <= low) return true;
		return std::nullopt;
	case XeenEventComparison::LessOrEqual:
		if (value == std::numeric_limits<std::uint32_t>::max()) return true;
		return std::nullopt;
	}
	return std::nullopt;
}

XeenEventResidue xeenEventAfterProbe(const XeenEventProbe &probe, const XeenEventResidue &before) {
	if (probe.residue) return before;
	return {XeenEventResidue::Kind::Value, probe.value};
}

XeenEventWrite xeenEventSetValue(const XeenEventValueOwners &o, unsigned mode, std::uint32_t value, std::size_t member) {
	std::string reason;
	const std::uint8_t byte = static_cast<std::uint8_t>(value);
	const std::uint16_t word = static_cast<std::uint16_t>(value);
	if (mode >= 95 || mode < 3) return applied();
	if (member >= o.party.party.size() && !partyMode(mode, value)) return unsupported("Event value requires an active party member", XeenEventRefusal::Member);
	if (characterByteMode(mode)) {
		const auto f = characterByte(o, mode, member, reason);
		if (!f) return unsupported(reason);
		f->set(byte);
		return applied();
	}
	auto &c = characterOrScratch(o, member);
	switch (mode) {
	case 3:
		if (byte > 2) return unsupported("Event sex value is outside the original sexes");
		c.sex = static_cast<XeenSex>(byte); return applied();
	case 4:
		if (byte > 4) return unsupported("Event race value is outside the original races");
		c.race = static_cast<XeenRace>(byte); return applied();
	case 5:
		if (byte > 9) return unsupported("Event class value is outside the original classes");
		c.characterClass = static_cast<XeenCharacterClass>(byte); return applied();
	case 8: c.currentHp = static_cast<std::int16_t>(word); return applied();
	case 9: c.currentSp = static_cast<std::int16_t>(word); return applied();
	case 16: {
		auto *in = inputs(o, member);
		if (!in) return unsupported("Event character inputs are absent");
		in->experience = value; return applied();
	}
	case 18: return applyCondition(c, value);
	case 34: case 35: {
		auto *p = purse(o, reason);
		if (!p) return unsupported(reason);
		(mode == 34 ? p->gold : p->gems) = value; return applied();
	}
	case 65: o.party.food = word; return applied();
	case 83: o.script.damageType = byte; return applied();
	case 84:
		if (byte > 3) return unsupported("Event facing is outside the four directions");
		o.camera.direction = static_cast<XeenDirection>(byte); return applied();
	default: break;
	}
	auto *ctx = context(o, reason);
	switch (mode) {
	case 17: case 25: case 69: case 70: case 71: case 72: case 73: case 74: case 76: case 79: case 85: case 94: {
		if (!ctx) return unsupported(reason);
		auto next = *ctx;
		if (mode == 25) { if (word >= 1440) return unsupported("Event minutes are outside the day"); next.minutes = word; }
		else if (mode == 76) { if (word >= 100) return unsupported("Event day is outside the year"); next.day = word; }
		else if (mode == 85) next.year = word;
		else if (mode == 69) next.effects[XeenPartyEffect::Levitate] = byte;
		else if (mode == 79) next.effects[XeenPartyEffect::WizardEye] = 1;
		else if (mode == 94) next.effects[XeenPartyEffect::WalkOnWater] = byte;
		else if (mode == 74) {
			// 0x23c38: every listed field receives the low byte.
			next.effects[XeenPartyEffect::WalkOnWater] = byte; next.effects[XeenPartyEffect::WizardEye] = byte; next.effects[XeenPartyEffect::Levitate] = byte;
			for (std::size_t n : {0u, 2u, 3u, 4u, 5u}) next.lightAndResistances[n] = byte;
		} else next.lightAndResistances[*partyWord(mode)] = word;
		if (!canonicalEffects(next)) return unsupported("Event party effect value is outside its flag range");
		*ctx = next;
		return applied();
	}
	default: break;
	}
	return applied();
}

XeenEventWrite xeenEventTake(const XeenEventValueOwners &o, unsigned mode, std::uint32_t value, std::size_t member) {
	std::string reason;
	const std::uint8_t byte = static_cast<std::uint8_t>(value);
	const std::uint16_t word = static_cast<std::uint16_t>(value);
	if (mode == 0) return applied();
	if (member >= o.party.party.size() && !partyMode(mode, value)) return unsupported("Event value requires an active party member", XeenEventRefusal::Member);
	if (characterByteMode(mode)) {
		const auto f = characterByte(o, mode, member, reason);
		if (!f) return unsupported(reason);
		f->set(static_cast<std::uint8_t>(f->get() - byte));
		return applied();
	}
	auto &c = characterOrScratch(o, member);
	switch (mode) {
	case 9:
		// 0x220db: only a spellcaster loses SP, never below zero.
		if (c.hasSpells) {
			c.currentSp = static_cast<std::int16_t>(static_cast<std::uint16_t>(c.currentSp) - word);
			if (c.currentSp < 1) c.currentSp = 0;
		}
		return applied();
	case 13:
		if (value >= 18) return unsupported("Event skill index is outside the skills", kIndex);
		c.skills[value] = 0; return applied();
	case 15:
		if (value > 0xff) return unsupported("Event award is outside the award table", kIndex);
		setAward(c, value, false); return applied();
	case 16: {
		auto *in = inputs(o, member);
		if (!in) return unsupported("Event character inputs are absent");
		in->experience = in->experience - value; return applied();
	}
	case 18:
		if (value >= 16) return unsupported("Event condition index is outside the conditions", kIndex);
		c.conditions[value] = 0; return applied();
	case 19: {
		bool found = false;
		const auto slot = spellSlot(c, value, reason, found);
		if (!slot) return unsupported(reason);
		if (found) (*c.learnedSpells)[*slot] = 0;
		return applied();
	}
	case 20:
		if (o.camera.mapId.side != XeenSide::Clouds || value >= XeenGameFlags::kCount)
			return unsupported("Event game flag is outside the Clouds bank", o.camera.mapId.side != XeenSide::Clouds ? kContext : kIndex);
		o.flags.clear(static_cast<int>(value)); return applied();
	case 21: {
		if (value >= 82) {
			const auto index = XeenCloudsQuestItems::indexForItemId(value);
			if (o.camera.mapId.side != XeenSide::Clouds || !index)
				return unsupported("Event quest item is outside the Clouds items", o.camera.mapId.side != XeenSide::Clouds ? kContext : XeenEventRefusal::Mode);
			o.party.questItems.eventTake(*index);
			return applied();
		}
		// 0x221fa: the first matching slot of the ID's category is removed, then sorted.
		auto &items = value < 35 ? c.weapons : value < 49 ? c.armor : value < 60 ? c.accessories : c.miscellaneous;
		for (std::size_t slot = 0; slot < items.size(); ++slot) {
			const bool match = value < 35 ? items[slot].id == value : value < 49 ? items[slot].id == value - 35 :
				value < 60 ? items[slot].id == value - 49 : items[slot].material == value - 60;
			if (!match) continue;
			items[slot].material = 0; items[slot].id = 0; items[slot].state = 0; items[slot].frame = 0;
			xeenCompactItems(items);
			return applied();
		}
		XeenEventWrite failed; failed.status = XeenEventWriteStatus::Failed; return failed;
	}
	case 34: case 35: case 65: {
		// Party::subtract (0x1e941): too little leaves the value and reports it.
		XeenEventWrite short_;short_.status = XeenEventWriteStatus::NotEnough;
		short_.consumable = mode == 34 ? 0 : mode == 35 ? 1 : 2;
		if (mode == 65) {
			if (o.party.food < value) return short_;
			o.party.food = static_cast<std::uint16_t>(o.party.food - word); return applied();
		}
		auto *p = purse(o, reason);
		if (!p) return unsupported(reason);
		auto &amount = mode == 34 ? p->gold : p->gems;
		if (amount < value) return short_;
		amount = amount - value; return applied();
	}
	case 103:
		if (!o.party.worldFlags || value >= XeenWorldFlags::kCount) return unsupported("Event world flag is outside the world bank", kIndex);
		o.party.worldFlags->values[value] = false; return applied();
	case 104:
		if (o.camera.mapId.side != XeenSide::Clouds || !XeenCloudsQuestFlags::validIndex(value))
			return unsupported("Event quest flag is outside the Clouds bank", o.camera.mapId.side != XeenSide::Clouds ? kContext : kIndex);
		o.party.questFlags.clear(value); return applied();
	case 107: return unsupported("Darkside character flags are not supported yet");
	case 8: case 25: case 76: return unsupported("Event take mode needs another owner");
	default: break;
	}
	auto *ctx = context(o, reason);
	switch (mode) {
	case 17: case 69: case 70: case 71: case 72: case 73: case 74: case 79: case 85: case 94: {
		if (!ctx) return unsupported(reason);
		auto next = *ctx;
		const auto subtract = [&](std::size_t n) {
			next.lightAndResistances[n] = static_cast<std::uint16_t>(next.lightAndResistances[n] - word);
		};
		if (mode == 69) next.effects[XeenPartyEffect::Levitate] = static_cast<std::uint8_t>(next.effects[XeenPartyEffect::Levitate] - byte);
		else if (mode == 79) next.effects[XeenPartyEffect::WizardEye] = 0;
		else if (mode == 94) next.effects[XeenPartyEffect::WalkOnWater] = 0;
		else if (mode == 85) next.year = static_cast<std::uint16_t>(next.year - word);
		else if (mode == 74) {
			next.effects[XeenPartyEffect::Levitate] = static_cast<std::uint8_t>(next.effects[XeenPartyEffect::Levitate] - byte);
			for (std::size_t n : {0u, 2u, 3u, 4u, 5u}) subtract(n);
			next.effects[XeenPartyEffect::WalkOnWater] = 0;
		} else subtract(*partyWord(mode));
		*ctx = next;
		return applied();
	}
	default: break;
	}
	return applied();
}

XeenEventWrite xeenEventGive(const XeenEventValueOwners &o, unsigned mode, std::uint32_t value, std::size_t member) {
	std::string reason;
	const std::uint8_t byte = static_cast<std::uint8_t>(value);
	const std::uint16_t word = static_cast<std::uint16_t>(value);
	if (mode == 0) return applied();
	if (member >= o.party.party.size() && !partyMode(mode, value)) return unsupported("Event value requires an active party member", XeenEventRefusal::Member);
	if (characterByteMode(mode)) {
		const auto f = characterByte(o, mode, member, reason);
		if (!f) return unsupported(reason);
		if (mode == 10 || mode == 77 || mode == 11 || mode == 12) {
			f->set(static_cast<std::uint8_t>(f->get() + byte));
			return applied(mode != 12);
		}
		// Attribute, resistance and level grants saturate at 255 (0x229af...).
		const std::uint64_t sum = std::uint64_t(f->get()) + value;
		f->set(sum > 0xff ? 0xff : static_cast<std::uint8_t>(sum));
		return applied(true);
	}
	auto &c = characterOrScratch(o, member);
	switch (mode) {
	case 3:
		if (byte > 2) return unsupported("Event sex value is outside the original sexes");
		c.sex = static_cast<XeenSex>(byte); return applied(true);
	case 4:
		if (byte > 4) return unsupported("Event race value is outside the original races");
		c.race = static_cast<XeenRace>(byte); return applied(true);
	case 5:
		if (byte > 9) return unsupported("Event class value is outside the original classes");
		c.characterClass = static_cast<XeenCharacterClass>(byte); return applied(true);
	case 8:
		c.currentHp = static_cast<std::int16_t>(static_cast<std::uint16_t>(c.currentHp) + word);
		return applied();
	case 9:
		c.currentSp = static_cast<std::int16_t>(static_cast<std::uint16_t>(c.currentSp) + word);
		return applied(true);
	case 13:
		if (value >= 18) return unsupported("Event skill index is outside the skills", kIndex);
		xeenGrantSkill(c, value);
		return applied(true);
	case 15:
		if (value > 0xff) return unsupported("Event award is outside the award table", kIndex);
		setAward(c, value, true);
		return applied(value != 8); // Convicted Thief has no sparkle (0x22754).
	case 16: {
		auto *in = inputs(o, member);
		if (!in) return unsupported("Event character inputs are absent");
		in->experience = in->experience + value;
		return applied(true);
	}
	case 18: return applyCondition(c, value);
	case 19: {
		bool found = false;
		const auto slot = spellSlot(c, value, reason, found);
		if (!slot) return unsupported(reason);
		if (!found) return applied();
		(*c.learnedSpells)[*slot] = 1;
		return applied(true);
	}
	case 20:
		if (o.camera.mapId.side != XeenSide::Clouds || value >= XeenGameFlags::kCount)
			return unsupported("Event game flag is outside the Clouds bank", o.camera.mapId.side != XeenSide::Clouds ? kContext : kIndex);
		o.flags.set(static_cast<int>(value)); return applied();
	case 34: case 35: {
		// 0x2298d: straight into the party purse, then the sparkle.
		auto *p = purse(o, reason);
		if (!p) return unsupported(reason);
		auto &amount = mode == 34 ? p->gold : p->gems;
		amount = amount + value;
		return applied(true);
	}
	case 65: o.party.food = static_cast<std::uint16_t>(o.party.food + word); return applied(true);
	case 78: case 81: {
		auto *ctx = context(o, reason);
		if (!ctx) return unsupported(reason);
		if (mode == 78) c.currentHp = static_cast<std::int16_t>(XeenCharacterRules::maxHp(c, {ctx->year}));
		else c.currentSp = static_cast<std::int16_t>(XeenCharacterRules::maxSp(c, {ctx->year}));
		return applied(true);
	}
	case 103:
		if (!o.party.worldFlags || value >= XeenWorldFlags::kCount) return unsupported("Event world flag is outside the world bank", kIndex);
		o.party.worldFlags->values[value] = true; return applied();
	case 104:
		if (o.camera.mapId.side != XeenSide::Clouds || !XeenCloudsQuestFlags::validIndex(value))
			return unsupported("Event quest flag is outside the Clouds bank", o.camera.mapId.side != XeenSide::Clouds ? kContext : kIndex);
		o.party.questFlags.set(value); return applied();
	case 107: return unsupported("Darkside character flags are not supported yet");
	case 21: case 25: case 66: case 76: case 82: case 85: case 100:
		return unsupported("Event give mode needs another owner");
	default: break;
	}
	auto *ctx = context(o, reason);
	switch (mode) {
	case 17: case 69: case 70: case 71: case 72: case 73: case 74: case 79: case 94: {
		if (!ctx) return unsupported(reason);
		auto next = *ctx;
		const auto add = [&](std::size_t n) {
			next.lightAndResistances[n] = static_cast<std::uint16_t>(next.lightAndResistances[n] + word);
		};
		if (mode == 69) next.effects[XeenPartyEffect::Levitate] = static_cast<std::uint8_t>(next.effects[XeenPartyEffect::Levitate] + byte);
		else if (mode == 79) next.effects[XeenPartyEffect::WizardEye] = 1;
		else if (mode == 94) next.effects[XeenPartyEffect::WalkOnWater] = 1;
		else if (mode == 74) {
			// 0x231df..0x232b5: Walk on Water becomes active (the reference clears it).
			next.effects[XeenPartyEffect::Levitate] = static_cast<std::uint8_t>(next.effects[XeenPartyEffect::Levitate] + byte);
			for (std::size_t n : {0u, 2u, 3u, 4u, 5u}) add(n);
			next.effects[XeenPartyEffect::WalkOnWater] = 1;
		} else add(*partyWord(mode));
		*ctx = next;
		return applied();
	}
	default: break;
	}
	return applied();
}

} // namespace mmodern
