#ifndef MMODERN_XEEN_REGIONAL_RULES_H
#define MMODERN_XEEN_REGIONAL_RULES_H
#include "games/xeen/XeenActorApproach.h"
#include <bitset>

namespace mmodern {
// Disposable queries over checked immutable geometry; these own no live state.
XeenMonsterTerrain xeenRegionalActorTerrain(const XeenMap &, const XeenActor &, int x, int y);
std::bitset<256> xeenActorClosure(const XeenMap &, const XeenActor &);
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
}
#endif
