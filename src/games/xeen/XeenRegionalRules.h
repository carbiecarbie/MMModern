#ifndef MMODERN_XEEN_REGIONAL_RULES_H
#define MMODERN_XEEN_REGIONAL_RULES_H
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenCombatRules.h"
#include <bitset>

namespace mmodern {
// Disposable queries over checked immutable geometry; these own no live state.
XeenMonsterTerrain xeenRegionalActorTerrain(const XeenMap &, const XeenActor &, int x, int y);
std::bitset<256> xeenActorClosure(const XeenMap &, const XeenActor &);
// First excluded center row (edge or obstruction), or four for all rows.
// Player missiles admit middle 15; non-east enemy rays do not.
unsigned xeenPlayerRayRows(const XeenMap &, const XeenCamera &);
bool xeenOutdoorRangedRay(const XeenMap &, const XeenCamera &, const XeenActor &);
std::optional<std::size_t> xeenRegionalEvent(const XeenEventFile &, const XeenCamera &);
bool xeenRegionalSign(const XeenEventFile &, const XeenCamera &);
void xeenValidateRegionalActors(const XeenMap &, const XeenObjectFile &, const std::vector<XeenActor> &,
	const std::set<XeenMonsterIdentity> &accounted);
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
 std::array<XeenRegionalRangedShot,24> shots{};
 unsigned count=0;
 XeenConsequenceCharacters after;
};
// Complete detached movement opportunity. No live owner or publication authority.
struct XeenRegionalOpportunityCandidate {
	std::vector<XeenActor> actors;
	XeenConsequenceCharacters characters;
	std::array<XeenRegionalRangedShot,19> shots{};
	unsigned shotCount=0;
	XeenActorView view;
	XeenRegionalOpportunityCandidate(const XeenMap &, const std::vector<XeenActor> &,
		const XeenCamera &, const XeenConsequenceCharacters &, const XeenConsequenceInputs &,
		unsigned year, unsigned participantMask, const std::array<bool,6> &blocked = {});
	bool service(XeenConsequenceDraw &);
private:
	XeenCamera camera;
	XeenConsequenceInputs inputs;
	unsigned year, participantMask, cursor=0;
	std::array<bool,6> blocked;
	std::optional<XeenEnemyAttackCandidate> attack;
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
	std::array<XeenRegionalRangedShot,24> shots{};
	unsigned shotCount=0, remaining=0, pending=0;
	bool classify=false, timeDone=false;
	std::uint64_t revision=0;
};

struct XeenShootCandidate {
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
