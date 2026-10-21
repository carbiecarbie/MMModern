#include "games/xeen/XeenEntranceScene.h"
#include <algorithm>
#include <stdexcept>

namespace mmodern {
const std::vector<XeenEntranceSite> &xeenEntranceSites() {
    using P=XeenEntranceSite::Position;using D=XeenDirection;using S=XeenSide;
    static const std::vector<XeenEntranceSite> sites={
        {8,12,P::Any,0,3,{55,3,8,D::East}}, {8,15,P::Any,0,2,{59,11,8,D::West}},
        {8,24,P::Any,0,1,{51,7,12,D::South}},
        {8,7,P::Any,0,46,{{S::Darkside,113},7,4,D::North}},
        {8,13,P::Any,0,45,{{S::Darkside,117},7,4,D::North}},
        {9,8,P::Any,0,6,{81,1,17,D::East}}, {9,12,P::Any,0,5,{80,29,16,D::West}},
        {9,20,P::Any,0,7,{79,5,16,D::East}},
        {9,19,P::Any,0,50,{{S::Darkside,121},18,0,D::North}},
        {10,14,P::Any,0,-1,{37,1,4,D::East}},
        {10,18,P::EqualX,9,-1,{35,1,12,D::East}}, {10,18,P::OtherX,9,-1,{36,7,1,D::North}},
        {10,23,P::EqualX,5,-1,{33,7,1,D::North}}, {10,23,P::OtherX,5,-1,{34,7,30,D::South}},
        {11,5,P::Any,0,4,{82,7,5,D::North}},
        {11,2,P::Any,0,51,{{S::Darkside,125},7,6,D::North}}
    };
    return sites;
}
const XeenEntranceSite *xeenEntranceSite(unsigned action,const XeenCamera &camera) {
    if(camera.mapId.side!=XeenSide::Clouds)return nullptr;
    for(const auto &site:xeenEntranceSites())if(site.action==action && site.map==camera.mapId.number &&
        (site.position==XeenEntranceSite::Position::Any ||
         (camera.x==site.branchX)==(site.position==XeenEntranceSite::Position::EqualX)))return &site;
    return nullptr;
}
bool xeenEntranceKey(const XeenEntranceSite &site,const XeenCloudsQuestItems &items) {
    // Maintainer 2026-10-20: Darkside counters are outside Clouds, never held.
    return site.quest<0 || (unsigned(site.quest)<XeenCloudsQuestItems::kCount && items.at(site.quest)!=0);
}
XeenCamera xeenEntranceDestination(const XeenEntranceSite &site,const XeenCamera &camera,const XeenCloudsQuestItems &items) {
    return xeenEntranceKey(site,items)?site.destination:camera;
}
namespace {
// Clouds coordinate rows from the pinned locations.cpp; DOS table calibration
// and the two retreat corrections: docs/rules/evidence.md, M80 section 3.
constexpr int reaperX[]{0,-10,-20,-30,-40,-49,-49,-49,-49,-49,-49,-49,-49,-49};
constexpr int reaperY[]{0,12,25,37,45,50,56,61,67,72,78,83,89,94};
constexpr int golemX[]{0,-5,0,6,10,13,17,20,23,26,29,31};
constexpr int golemX2[]{160,145,140,136,130,123,117,110,103,96,89,81};
constexpr int golemY[]{0,0,0,0,0,5,10,15,20,25,30,35};
constexpr int dwarfX[]{0,-5,-7,-8,-11,-9,-3,1,6,10,15,18,23};
constexpr int dwarfX2[]{160,145,133,122,109,101,97,91,86,80,75,68,63};
constexpr int dwarfY[]{0,0,4,9,13,15,20,24,30,37,45,51,58};
constexpr int riseX[]{0,-2,-4,-6,-8,-10,-12,-14,-16,-18,-20,-20,-20,-20,-20,-20};
constexpr int riseY[]{0,12,25,37,50,62,75,87,100,112,125,137,150,162,175,187};
constexpr int sphinxX[]{0,-5,-10,-15,-20,-17,-12,-7,0};
constexpr int sphinxX2[]{160,145,130,115,100,93,88,83,80};
constexpr int sphinxY[]{0,0,0,6,11,16,20,23,28};
struct Art {
    const char *background,*actor,*mouth;
    const int *x,*x2,*y;
    unsigned zoom,speechTicks,randomFirst,randomLast;
};
const Art &art(unsigned action) {
    static const Art entries[]{
        {"tower.vga","freap.vga","freap.vga",reaperX,nullptr,reaperY,14,2,0,4},
        {"golmback.vga","golem.vga","golem.vga",golemX,golemX2,golemY,12,1,0,0},
        {"dwarf1.vga","dwarf3.vga","dwarf2.vga",dwarfX,dwarfX2,dwarfY,13,2,0,9},
        {"sphinx.vga","sphinx.vga","sphinx.vga",sphinxX,sphinxX2,sphinxY,9,0,2,10}
    };
    if(action<8 || action>11)throw std::invalid_argument("Unknown entrance scene");
    return entries[action-8];
}
}
XeenEntranceAnimation::XeenEntranceAnimation(unsigned action,bool keyed,const Duration &duration,bool efx,unsigned captionLength):_action(action),_efx(efx),_captionLength(captionLength) {
    const auto &a=art(action);
    if(!_efx && !_captionLength)throw std::invalid_argument("Missing entrance caption record");
    const auto add=[&](Phase phase,unsigned count,unsigned ticks,const char *sample=nullptr) {
        const auto length=sample && _efx?duration(sample):0;
        if(sample && _efx && !length)throw std::invalid_argument("Invalid entrance sample duration");
        _phases.push_back({phase,count,ticks,sample,length});
    };
    add(Phase::Zoom,a.zoom,action<10?1:0);
    if(action==8 || action==9)add(Phase::Slide,13,1);
    if(action==9){_entryDuration=_efx?duration("ogre.voc"):0;if(_efx && !_entryDuration)throw std::invalid_argument("Invalid entrance growl duration");add(Phase::EntryTail,1,0);}
    if(action==10)add(Phase::Rise,16,0);
    static const char *samples[4][2][3]={
        {{"reaper14.voc","reaper15.voc",nullptr},{"reaper12.voc","reaper13.voc",nullptr}},
        {{"golem13.voc","golem14.voc",nullptr},{"golem15.voc","golem16.voc",nullptr}},
        {{"dwarf10.voc","dwarf11.voc","dwarf12.voc"},{"dwarf10.voc","dwarf11.voc","dwarf12.voc"}},
        {{"sphinx13.voc","sphinx14.voc",nullptr},{"sphinx10.voc","sphinx11.voc","sphinx12.voc"}}
    };
    for(const char *sample:samples[action-8][keyed?1:0])if(sample) {
        if(_phases.back().kind==Phase::Speech) {
            if(action==8)add(Phase::Pause,1,7);
            if(action==9){add(Phase::Pause,1,1);add(Phase::Growl,1,0,"ogre.voc");}
        }
        if(!_firstSpeech)_firstSpeech=_phases.size();
        add(Phase::Speech,0,a.speechTicks,sample);
        if(!_efx && action>=10)break;
    }
    if(action==8)add(Phase::Pause,1,1);
    if(!keyed && action!=10)add(Phase::Retreat,a.zoom,0);
    add(Phase::Finish,1,0);
}
std::optional<XeenEntranceAnimation::Frame> XeenEntranceAnimation::advance(std::uint64_t now,const Draw &draw) {
    if(finished() || _awaiting || (_started && now<_deadline))return {};
    if(!_clockMarked){_captionMark=PresentationClock::tick(now);_clockMarked=true;}
    if(_started) {
        const auto &step=_phases[_phase];
        const bool speech=step.kind==Phase::Speech;
        const bool captionBusy=!_efx && _phase==_firstSpeech && !_captionComplete;
        if(speech && !captionBusy && now>=_sampleEnd && now<_minimumEnd){_deadline=_minimumEnd;return {};}
        if(speech?(!captionBusy && now>=std::max(_sampleEnd,_minimumEnd)):++_iteration>=step.count) {
            ++_phase;_iteration=0;_started=false;
            if(finished())return {};
        } else if(speech)++_iteration;
    }
    // Skip an already elapsed growl tail without inventing an extra wait.
    if(_phases[_phase].kind==Phase::EntryTail && now>=_entryEnd){++_phase;_iteration=0;_started=false;}
    auto frame=compose(draw);
    if(!_efx && _phase==_firstSpeech) {
        const auto tick=PresentationClock::tick(now);
        if(tick-_captionMark>1) {
            _cursor=(_cursor+1)%_captionLength;_captionMark=tick;
            _captionComplete=_cursor==0;
        }
        frame.caption=_cursor;
    }
    _awaiting=true;
    return frame;
}
void XeenEntranceAnimation::acquired(std::uint64_t now) {
    if(!_awaiting)return;
    const auto &step=_phases[_phase];
    if(!_started && step.sample) {
        _sampleEnd=PresentationClock::checkedAdd(now,step.duration);
        // Dwarf's WAIT(3) checks the phase's game-counter mark, not a new
        // three-tick pause after the sample. The two-tick loop uses timer 5.
        _minimumEnd=_action==10?PresentationClock::after(now,3):0;
    }
    if(step.kind==Phase::Slide && _action==9 && (!_iteration || now>=_entryEnd))
        _entryEnd=PresentationClock::checkedAdd(now,_entryDuration);
    _deadline=step.kind==Phase::Finish?now:step.kind==Phase::Growl?_sampleEnd:step.kind==Phase::EntryTail?_entryEnd:
        step.ticks?PresentationClock::after(now,step.ticks):
        PresentationClock::retraceAfter(PresentationClock::retraceAfter(now));
    _started=true;_awaiting=false;
}
XeenEntranceAnimation::Frame XeenEntranceAnimation::compose(const Draw &draw) const {
    const auto &a=art(_action);const auto phase=_phases[_phase].kind;
    Frame result{phase,_iteration};
    const auto put=[&](const char *name,unsigned frame,int x=0,int y=0,int scale=0){result.sprites.push_back({name,frame,x,y,scale});};
    if(phase==Phase::Finish){
        if(_action==10){put(a.actor,0);put(a.mouth,1);}else result.restore=true;
        return result;
    }
    if(phase==Phase::Zoom || phase==Phase::Retreat) {
        const auto i=phase==Phase::Zoom?a.zoom-1-_iteration:_iteration;
        result.restore=_action!=8 || phase==Phase::Retreat;
        put(a.background,0,a.x[i],a.y[i],i);
        if(a.x2)put(a.background,1,a.x2[i],a.y[i],i);
        return result;
    }
    if(phase==Phase::Rise) {
        const auto i=15-_iteration;result.restore=true;result.saveZoom=_iteration==0;
        put(a.actor,0,riseX[i],riseY[i],i);return result;
    }
    if(_action==10) {
        put(a.actor,0);
    } else {
        put(a.background,0);if(a.x2)put(a.background,1,160);
    }
    if(phase==Phase::Slide) {
        const int x=_action==8?200-int(_iteration)*16:-200+int(_iteration)*16;
        put(a.actor,0,x);
        if(_action==9) {
            put(a.actor,1,x+160);
            const int y=int(draw(0,9))-5,dx=int(draw(0,9))-5;
            put(a.actor,2,x+dx,y);
        }
    } else if(_action==9) {
        put(a.actor,0);put(a.actor,1,160);
        int x=0,y=0;
        if(phase==Phase::Speech){y=int(draw(0,9))-3;x=int(draw(0,5))-3;}
        put(a.actor,2,x,y);
    } else {
        if(_action==8)put(a.actor,0);
        if(phase==Phase::Speech)put(a.mouth,draw(a.randomFirst,a.randomLast));
    }
    return result;
}
}
