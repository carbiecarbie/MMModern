#ifndef MMODERN_COVERAGE_CONTEXT_H
#define MMODERN_COVERAGE_CONTEXT_H
#include "app/XeenEncounterFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenSaveState.h"
namespace mmodern::coverage {
struct Support;
// The same original New Game or developer Event-fixture initialization as gameplay,
// retained only in memory. The fixture admits its Clouds map through normal owners.
// Each probe receives a fresh detached candidate including actors and Journey RNG.
class OriginalContext {
public:
    explicit OriginalContext(XeenTemplePriceState &, const GameInstallation &, std::optional<XeenCamera> fixture = {});
    XeenTemplePriceState &templePrices;
    const GameInstallation installation;
    XeenAssetSource assets;
    XeenAssetSource darkAssets;
    XeenMapLoader maps;
    XeenEventLoader events;
    XeenEventTextLoader texts;
    XeenWorld world;
    XeenPartyState party;
    XeenCamera camera{28,18,4,XeenDirection::West};
    XeenGameFlags flags;
    XeenEventPresenter::Clock clock=[]{return 0;};
    std::unique_ptr<XeenEncounterFlow> flow;
    XeenEventTextFile text(XeenMapIdentity);
    Support replayRoot(XeenCamera);
private:
    std::map<XeenMapIdentity,XeenEventTextFile> textCache;
    XeenSaveState::Resources resources;
    std::optional<XeenSaveSnapshot> initial;
};
}
#endif
