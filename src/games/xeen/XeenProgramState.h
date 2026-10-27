#ifndef MMODERN_XEEN_PROGRAM_STATE_H
#define MMODERN_XEEN_PROGRAM_STATE_H
#include "games/xeen/XeenTemplePriceState.h"
#include "games/xeen/XeenMonsterTreasure.h"
#include "games/xeen/XeenWallClock.h"
#include <memory>
namespace mmodern {
class XeenAudio;
// Running-program state: one per process, created empty by main and borrowed by
// every gameplay session; never saved. Each member keeps its own revision, so a
// pending-treasure change never invalidates a prepared Temple visit. The games
// layer receives only the treasure staging, never this owner.
struct XeenProgramState {
    explicit XeenProgramState(XeenWallClock clock):wallClock(std::move(clock)),playTimeMark(wallClock.now()) {}
    XeenWallClock wallClock;
    std::uint32_t playTimeMark; // DOS DS:98b0: process lifetime, never restored.
    std::uint32_t playTime(std::uint32_t saved) const {return wallClock.now()-playTimeMark+saved;}
    void resetPlayTime() {playTimeMark=wallClock.now();}
    bool darksideIntroSeen=false;
    bool cloudsWon=false; // Title gate only; never a party completion source.
    bool eventCallSkipped=false; // DOS DS:9756: process lifetime, never saved.
    bool innSongStarted=false; // DOS DS:8d28: process lifetime, never saved.
    unsigned attackWeaponFx=0; // DS:96ee is retained even across ordinary spell impacts.
    unsigned steppingFx=0; // DAT DS:9775; process presentation state, never saved.
	XeenTemplePriceState templePrices;
	XeenTreasureStagingState treasure;
 std::shared_ptr<XeenAudio> audio;
};
}
#endif
