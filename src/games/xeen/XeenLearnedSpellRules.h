#ifndef MMODERN_XEEN_LEARNED_SPELL_RULES_H
#define MMODERN_XEEN_LEARNED_SPELL_RULES_H

#include "games/xeen/XeenParty.h"
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

enum class XeenSpellCategory : std::uint8_t { Clerical, Wizardry, Druidic };
enum class XeenLearnedSpell : std::uint8_t {
    FingerOfDeath=24, GolemStopper=29, HolyWord=32, MassDistortion=46,
    AcidSpray=0, ColdRay=5, DancingSword=11, DeadlySwarm=14, ElementalStorm=18,
    FantasticFreeze=22, FieryFlail=23, Fireball=25, Frostbite=28, Implosion=35,
    Incinerate=36, Inferno=37, LightningBolt=43, MegaVolts=47, MoonRay=48,
    PrismaticLight=54, StarBurst=63, SunRay=65,
    CreateFood=6, CureDisease=7, CureParalysis=8, CurePoison=9, NaturesCure=49,
    PowerCure=52, StoneToFlesh=64, SuppressDisease=67, SuppressPoison=68,
    DivineIntervention=16, RaiseDead=56, Resurrection=58,
    Awaken=1, BeastMaster=2, CureWounds=10, DragonSleep=17, EnergyBlast=20,
    FirstAid=26, FlyingFist=27, Hypnotize=33, InsectSpray=38, Jump=40,
    MagicArrow=45, Pain=50, PoisonVolley=51, Revitalize=59, Shrapmetal=60, Sleep=61, Sparks=62,
    ToxicCloud=72, TurnUndead=73, WizardEye=75
};

struct XeenSpellEffect {
	std::uint8_t owner = 0;
	std::int16_t hp = 0;
	std::array<std::uint8_t, XeenCharacter::kConditionCount> conditions{};
	std::optional<int> permanentEndurance, temporaryAge;
	bool healing = false;
	void apply(XeenCharacter &) const;
};

struct XeenSpellPreparation {
	bool failed = false;
	std::vector<XeenSpellEffect> effects;
	std::optional<std::uint16_t> food;
};

struct XeenConsequenceDraw;

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
enum class XeenSpellRange { Single, Group, All };
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
	static XeenSpellRange range(std::uint8_t) noexcept;
	static XeenSpellTarget target(std::uint8_t) noexcept;
	static XeenQuickSpellCheck quickSpellCheck(const XeenCharacter &,std::uint32_t gems) noexcept;
	static bool known(const XeenCharacter &, std::size_t) noexcept;
	static bool eligible(const XeenPartyState &, std::size_t activeIndex, std::size_t slot, bool combat=false) noexcept;
	static XeenSpellPreparation prepareFirstAid(const XeenPartyState &, std::size_t targetIndex,
		std::uint32_t currentYear, unsigned healing = 6);
	static XeenSpellPreparation prepareRevitalize(const XeenPartyState &, std::size_t targetIndex,
		std::uint32_t currentYear);
	static XeenSpellPreparation prepareAwaken(const XeenPartyState &);
	static std::optional<XeenSpellPreparation> preparePowerCure(const XeenPartyState &,
		std::size_t targetIndex,std::uint32_t currentYear,unsigned casterLevel,XeenConsequenceDraw &);
	static XeenSpellPreparation prepareEffect(XeenLearnedSpell,const XeenPartyState &,
		std::optional<std::size_t> targetIndex,std::uint32_t currentYear,
		std::optional<std::uint8_t> casterOwner = {});
	static void applyPartyEffect(XeenLearnedSpell,XeenGameplayContext &);
};

} // namespace mmodern
#endif
