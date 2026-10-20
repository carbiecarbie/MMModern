#ifndef MMODERN_XEEN_ENTRANCE_SCENE_H
#define MMODERN_XEEN_ENTRANCE_SCENE_H
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenNavigation.h"
#include "core/PresentationClock.h"
#include <functional>
#include <vector>

namespace mmodern {
struct XeenEntranceSite {
    unsigned action, map;
    enum class Position { Any, EqualX, OtherX } position;
    int branchX;
    int quest; // -1: no item; indices beyond the Clouds counters are never held.
    XeenCamera destination;
};
const std::vector<XeenEntranceSite> &xeenEntranceSites();
const XeenEntranceSite *xeenEntranceSite(unsigned action,const XeenCamera &);
bool xeenEntranceKey(const XeenEntranceSite &,const XeenCloudsQuestItems &);
XeenCamera xeenEntranceDestination(const XeenEntranceSite &,const XeenCamera &,const XeenCloudsQuestItems &);

// Value-only presentation cursor. EventFlow owns acquisition, saved screens,
// providers and gameplay RNG. Nothing here is persisted or owns game state.
class XeenEntranceAnimation {
public:
    enum class Phase { Zoom, Slide, Rise, Speech, Pause, EntryTail, Growl, Retreat, Finish };
    struct Sprite { const char *resource; unsigned frame; int x=0,y=0,scale=0; };
    struct Frame {
        Phase phase;
        unsigned iteration;
        bool restore=false,saveZoom=false;
        std::vector<Sprite> sprites;
    };
    using Duration=std::function<std::uint64_t(const char *)>;
    using Draw=std::function<unsigned(unsigned,unsigned)>;
    XeenEntranceAnimation(unsigned action,bool keyed,const Duration &);
    // Once per acquired iteration; a late wake advances only one iteration.
    std::optional<Frame> advance(std::uint64_t now,const Draw &);
    void acquired(std::uint64_t now);
    std::uint64_t deadline() const noexcept {return _deadline;}
    bool awaitingFrame() const noexcept {return _awaiting;}
    bool finished() const noexcept {return _phase>=_phases.size();}
    unsigned action() const noexcept {return _action;}
private:
    struct Step {Phase kind; unsigned count,ticks; const char *sample=nullptr; std::uint64_t duration=0;};
    unsigned _action,_iteration=0;
    bool _awaiting=false,_started=false;
    std::size_t _phase=0;
    std::uint64_t _deadline=0,_sampleEnd=0,_minimumEnd=0,_entryEnd=0,_entryDuration=0;
    std::vector<Step> _phases;
    Frame compose(const Draw &) const;
};
}
#endif
