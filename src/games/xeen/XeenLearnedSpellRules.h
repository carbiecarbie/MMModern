#ifndef MMODERN_XEEN_LEARNED_SPELL_RULES_H
#define MMODERN_XEEN_LEARNED_SPELL_RULES_H

#include "games/xeen/XeenParty.h"
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

enum class XeenSpellCategory : std::uint8_t { Clerical, Wizardry, Druidic };
enum class XeenLearnedSpell : std::uint8_t { Awaken = 1, FirstAid = 26 };

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

class XeenLearnedSpellRules {
public:
	static std::optional<XeenSpellCategory> categoryForClass(XeenCharacterClass) noexcept;
	static std::optional<std::uint8_t> spellForSlot(XeenSpellCategory, std::size_t) noexcept;
	static std::optional<XeenLearnedSpell> supported(std::uint8_t) noexcept;
	static bool known(const XeenCharacter &, std::size_t) noexcept;
	static bool eligible(const XeenPartyState &, std::size_t activeIndex, std::size_t slot) noexcept;
	static XeenSpellPreparation prepareFirstAid(const XeenPartyState &, std::size_t targetIndex,
		std::uint32_t currentYear);
	static XeenSpellPreparation prepareAwaken(const XeenPartyState &);
};

} // namespace mmodern
#endif
