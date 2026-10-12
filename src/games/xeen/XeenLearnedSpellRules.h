#ifndef MMODERN_XEEN_LEARNED_SPELL_RULES_H
#define MMODERN_XEEN_LEARNED_SPELL_RULES_H

#include "games/xeen/XeenParty.h"
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

enum class XeenSpellCategory : std::uint8_t { Clerical, Wizardry, Druidic };
enum class XeenLearnedSpell : std::uint8_t { Awaken = 1, CureWounds = 10, EnergyBlast = 20, FirstAid = 26, FlyingFist = 27, Jump = 40, MagicArrow = 45, Pain = 50, Revitalize = 59, Shrapmetal = 60, WizardEye = 75 };

struct XeenSpellEffect {
	std::uint8_t owner = 0;
	std::int16_t hp = 0;
	std::array<std::uint8_t, XeenCharacter::kConditionCount> conditions{};
};

struct XeenSpellPreparation {
	bool failed = false;
	std::vector<XeenSpellEffect> effects;
};

struct XeenLearnedSpellNames {
	std::vector<std::uint8_t> raw;
	std::array<std::string, 77> names{};
	static XeenLearnedSpellNames parse(std::vector<std::uint8_t>);
	bool operator==(const XeenLearnedSpellNames &other) const noexcept {
		return raw==other.raw && names==other.names;
	}
};

enum class XeenQuickSpellCheck { Supported, NoSpell, InsufficientSp, InsufficientGems, CombatForbidden, Unsupported };
enum class XeenSpellTarget { Party, Member, Contact, Relocation, Unsupported };
struct XeenSpellCost { unsigned sp=0, gems=0; };
class XeenLearnedSpellRules {
public:
	static std::optional<XeenSpellCategory> categoryForClass(XeenCharacterClass) noexcept;
	static std::optional<std::uint8_t> spellForSlot(XeenSpellCategory, std::size_t) noexcept;
	static std::optional<XeenLearnedSpell> supported(std::uint8_t) noexcept;
	static std::optional<XeenLearnedSpell> supportedIn(std::uint8_t, bool combat) noexcept;
	static unsigned cost(XeenLearnedSpell) noexcept;
	static XeenSpellCost cost(std::uint8_t id, unsigned level) noexcept;
	static std::optional<std::size_t> slotForSpell(XeenSpellCategory,std::uint8_t) noexcept;
	static std::uint32_t purchasePrice(std::uint8_t,XeenCharacterClass) noexcept;
	static std::vector<std::size_t> rows(const XeenCharacter &,bool guild=false,bool info=false);
	static XeenQuickSpellCheck check(const XeenCharacter &,std::size_t slot,std::uint32_t gems,bool combat) noexcept;
	static XeenSpellTarget target(std::uint8_t) noexcept;
	static XeenQuickSpellCheck quickSpellCheck(const XeenCharacter &,std::uint32_t gems) noexcept;
	static bool known(const XeenCharacter &, std::size_t) noexcept;
	static bool eligible(const XeenPartyState &, std::size_t activeIndex, std::size_t slot, bool combat=false) noexcept;
	static XeenSpellPreparation prepareFirstAid(const XeenPartyState &, std::size_t targetIndex,
		std::uint32_t currentYear, unsigned healing = 6);
	static XeenSpellPreparation prepareRevitalize(const XeenPartyState &, std::size_t targetIndex,
		std::uint32_t currentYear);
	static XeenSpellPreparation prepareAwaken(const XeenPartyState &);
	static XeenSpellPreparation prepareEffect(XeenLearnedSpell,const XeenPartyState &,
		std::optional<std::size_t> targetIndex,std::uint32_t currentYear);
	static void applyPartyEffect(XeenLearnedSpell,XeenGameplayContext &);
};

} // namespace mmodern
#endif
