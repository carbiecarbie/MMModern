#ifndef MMODERN_XEEN_REGIONAL_RULES_H
#define MMODERN_XEEN_REGIONAL_RULES_H
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenJourneyProgression.h"
#include <bitset>

namespace mmodern {
// Disposable queries over checked immutable geometry; these own no live state.
XeenMonsterTerrain xeenRegionalActorTerrain(const XeenMap &, const XeenActor &, int x, int y);
XeenMonsterTerrain xeenIndoorActorTerrain(XeenWorld &, const XeenActor &, int x, int y);
std::bitset<256> xeenActorClosure(const XeenMap &, const XeenActor &);
// First excluded center row (edge or obstruction), or four for all rows.
// Player missiles admit middle 15; non-east enemy rays do not.
unsigned xeenPlayerRayRows(const XeenMap &, const XeenCamera &);
bool xeenOutdoorRangedRay(const XeenMap &, const XeenCamera &, const XeenActor &);
unsigned xeenPlayerRayRows(XeenWorld &, const XeenCamera &);
bool xeenIndoorRangedRay(XeenWorld &, const XeenCamera &, const XeenActor &);
std::optional<std::size_t> xeenRegionalEvent(const XeenEventFile &, const XeenCamera &);
bool xeenRegionalSign(const XeenEventFile &, const XeenCamera &);
enum class XeenRegionalInteraction { None, Sign, Myra, Phirna, Well, VertigoEntrance, VertigoDoor, VertigoExit, Ironworks, Training, Temple, TempleLabel, Event };
// A terminal service request must be the original first instruction at the
// physical camera. Unsupported actions never acquire a Service continuation.
std::optional<std::uint8_t> xeenRegionalService(const XeenEventFile &, const XeenCamera &);
XeenRegionalInteraction xeenRegionalInteraction(const XeenEventFile &, const XeenCamera &);
std::optional<std::int16_t> xeenWellHpAfter(std::int16_t before) noexcept;
void xeenValidateRegionalActors(const XeenMap &, const XeenObjectFile &, const std::vector<XeenActor> &,
	const std::set<XeenMonsterIdentity> &accounted);
void xeenValidateVertigoActors(XeenWorld &, const std::vector<XeenActor> &);
using XeenRegionalManifest = std::function<void(const XeenMap &, const XeenObjectFile &,
	const XeenEventFile &, const std::vector<XeenMonsterRecord> &)>;
void xeenValidateRegionalManifest(const XeenMap &, const XeenObjectFile &, const XeenEventFile &,
	const std::vector<XeenMonsterRecord> &, const std::vector<std::uint8_t> &dat,
	const std::vector<std::uint8_t> &mob, const std::vector<std::uint8_t> &evt);
struct XeenRegionalRangedShot {
	XeenMonsterIdentity source;
	int x=0, y=0;
	unsigned distance=0;
	XeenDirection direction=XeenDirection::North;
	XeenCombatResult attack;
};
struct XeenRegionalObservation {
 enum class Stage { Published, Travel, Portrait };
 Stage stage=Stage::Published;
 std::optional<XeenMonsterIdentity> impactSource;
 std::optional<std::uint8_t> impactOwner;
 // At most three owed opportunities, each with the reference's 36 sources.
 std::array<XeenRegionalRangedShot,108> shots{};
 unsigned count=0;
 XeenConsequenceCharacters after;
};
// Complete detached movement opportunity. No live owner or publication authority.
struct XeenRegionalOpportunityCandidate {
 bool staged=false,travelStarted=false,travelPublished=false,travelPresented=false,
     portraitPublished=false,impactPresented=false,impactApplied=false;
 std::optional<XeenMonsterIdentity> impactSource;
 std::optional<std::uint8_t> impactOwner;
	std::vector<XeenActor> actors;
	XeenConsequenceCharacters characters;
	std::array<XeenRegionalRangedShot,36> shots{};
	unsigned shotCount=0;
	XeenActorView view;
	XeenRegionalOpportunityCandidate(const XeenMap &, const std::vector<XeenActor> &,
		const XeenCamera &, const XeenConsequenceCharacters &, const XeenConsequenceInputs &,
		unsigned year, unsigned participantMask, const std::array<bool,6> &blocked = {});
	XeenRegionalOpportunityCandidate(XeenWorld &, const std::vector<XeenActor> &,
		const XeenCamera &, const XeenConsequenceCharacters &, const XeenConsequenceInputs &,
		unsigned year, unsigned participantMask, const std::array<bool,6> &blocked = {});
	bool service(XeenConsequenceDraw &);
 std::shared_ptr<const XeenRegionalObservation> presentation() const;
private:
	XeenCamera camera;
	XeenConsequenceInputs inputs;
	unsigned year, participantMask, cursor=0;
	std::array<bool,6> blocked;
	std::optional<XeenEnemyAttackCandidate> attack;
	XeenWorld *indoorWorld = nullptr;
};
// Retained preparation of one existing action/pulse publication. Flow holds the
// continuation while Approach retains sole authority to publish world deltas.
struct XeenRegionalActionCandidate {
	XeenCamera camera;
	XeenGameplayContext context;
	XeenConsequenceCharacters characters;
	XeenConsequenceInputs inputs;
	std::vector<XeenActor> actors;
	XeenCombatRandom random;
	XeenEncounterResult result;
	std::optional<XeenConditionTimeCandidate> time;
	std::optional<XeenRegionalOpportunityCandidate> opportunity;
	std::array<XeenRegionalRangedShot,108> shots{};
	unsigned shotCount=0, remaining=0, pending=0;
	bool classify=false, timeDone=false;
	std::uint64_t revision=0;
};

struct XeenShootCandidate {
	 enum class Stage { Travel, Impact, PostImpact };
	 Stage stage=Stage::Travel;
	 bool presented=false;
	 bool advanceDraw=false, suffixPrepared=false;
	 unsigned row=0,rows=4,charge=10;
	 std::uint64_t deadline=0;
	 std::optional<XeenActor> bound;
	 std::optional<XeenJourneyRandomState> impactRandom;
	 std::optional<XeenJourneyLethal> lethal;
	 std::string feedback;
	 std::optional<unsigned> retireLane;
	std::array<std::optional<XeenMonsterIdentity>,12> targets{};
	std::array<bool,6> eligible{},spent{};
	std::array<unsigned,6> projectileEnd{};
	unsigned target=0,shooter=0,blockedRow=4;
	XeenCombatRandom random;
	std::optional<XeenPhysicalPlayerCandidate> attack;
	std::optional<XeenMonsterDropCandidate> drop;
	std::optional<XeenConditionTimeCandidate> time;
	bool attackDone=false,volleyDone=false;
};

}
#endif
