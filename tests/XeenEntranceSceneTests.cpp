#include "XeenTestInstallation.h"
#include "app/XeenAudio.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenEntranceScene.h"
#include "games/xeen/XeenStateEquality.h"
#include "formats/xeen/XeenVoc.h"
#include <iostream>
#include <map>

using namespace mmodern;
using training_test::check;
namespace mmodern {
struct XeenEntranceTestAccess {
    static unsigned wound(XeenEventFlow &flow) {
        auto &e=*flow._encounter;
        auto &actors=e._world._sessionState.mutableMonsters(e._camera.mapId);
        for(unsigned i=0;i<actors.size();++i) {
            auto &a=actors[i];
            if(a.lifecycle!=XeenActorLifecycle::Present || !a.statistics ||
                std::abs(int(a.x)-int(e._camera.x))+std::abs(int(a.y)-int(e._camera.y))<6)continue;
            a.hp=1;a.status=XeenActorStatus::Sleep;a.activated=true;
            e.retainJourney();flow._encounterFrame=e.ticket();return i;
        }
        throw std::runtime_error("No distant original monster at the entrance");
    }
    static std::uint64_t serviceSample(XeenEventFlow &flow) {
        XeenEventFlow::ServiceArtState state;flow.startServiceSample(state,"hello.voc");return state.sample.deadline();
    }
    static bool active(const XeenEventFlow &flow){return flow._entranceUi.has_value();}
    static bool noNotice(const XeenEventFlow &flow){return flow._presenter.pageCount()==0;}
};
}
namespace {
void sites() {
    using D=XeenDirection;using P=XeenEntranceSite::Position;
    const auto &rows=xeenEntranceSites();check(rows.size()==16,"Entrance row count");
    unsigned dark=0,dwarf=0;
    for(const auto &site:rows) {
        const int x=site.position==P::EqualX?site.branchX:site.position==P::OtherX?site.branchX+1:10;
        XeenCamera camera{site.map,x,9,D::West};
        check(xeenEntranceSite(site.action,camera)==&site,"Shared row lookup/position branch");
        check(xeenTownServiceKnown(camera.mapId,site.action),"Entrance row not admitted");
        XeenEventRecord record;record.opcode=0x11;record.parameters={std::uint8_t(site.action)};record.lengthField=6;record.x=x;record.y=9;
        check(std::holds_alternative<XeenDecodedEventInstruction>(XeenEventDecoder::decode(record,{camera.mapId})),"Table row not decoded");
        const auto empty=xeenEntranceDestination(site,camera,{});
        check(xeen_state::sameCamera(empty,site.quest<0?site.destination:camera),"Keyless destination");
        XeenCloudsQuestItems::Counts counts{};
        if(site.quest>=0 && site.quest<int(counts.size()))counts[site.quest]=256;
        else counts.fill(255);
        const XeenCloudsQuestItems items(counts);
        const auto destination=xeenEntranceDestination(site,camera,items);
        if(site.quest>=int(counts.size())) {
            ++dark;check(!xeenEntranceKey(site,items) && xeen_state::sameCamera(destination,camera),"Darkside key must be never held");
            check(site.destination.mapId.side==XeenSide::Darkside,"Darkside row lost its destination archive");
        } else check(xeenEntranceKey(site,items) && xeen_state::sameCamera(destination,site.destination),"Nonzero whole quest counter check");
        dwarf+=site.action==10;
    }
    check(dark==4 && dwarf==5,"Darkside/Dwarf branch coverage");
    check(!xeenEntranceSite(8,{28,10,9,D::North}) && !xeenTownServiceKnown(28,8),"Unknown map admitted");
    for(unsigned action:{6u,12u,13u})check(!xeenTownServiceKnown(24,action),"Out-of-scope scene admitted");
    check(!xeenEntranceSite(8,{{XeenSide::Darkside,24},10,9,D::North}),"Darkside scene assets admitted");
    // Destination constants are independent of iteration over the implementation table.
    const XeenCamera expected[]{ {55,3,8,D::East},{59,11,8,D::West},{51,7,12,D::South},
        {{XeenSide::Darkside,113},7,4,D::North},{{XeenSide::Darkside,117},7,4,D::North},
        {81,1,17,D::East},{80,29,16,D::West},{79,5,16,D::East},{{XeenSide::Darkside,121},18,0,D::North},
        {37,1,4,D::East},{35,1,12,D::East},{36,7,1,D::North},{33,7,1,D::North},{34,7,30,D::South},
        {82,7,5,D::North},{{XeenSide::Darkside,125},7,6,D::North}};
    for(unsigned i=0;i<rows.size();++i)check(xeen_state::sameCamera(rows[i].destination,expected[i]),"Destination table constant");
}
void animation(unsigned action,bool keyed,training_test::Inputs *resources=nullptr) {
    using A=XeenEntranceAnimation;using P=A::Phase;
    std::vector<std::string> samples;
    A value(action,keyed,[&](const char *name)->std::uint64_t {
        samples.push_back(name);
        std::uint64_t length=1000000000ULL;
        if(resources) {
            const auto bytes=resources->assets.readArchiveResource(name,XeenSceneArchive::Clouds);
            length=xeenVocDurationNanoseconds(bytes);
        }
        if(resources)std::cout<<"sample="<<name<<" duration_ns="<<length<<'\n';
        return length;
    });
    const std::vector<std::string> speech=action==8?(keyed?std::vector<std::string>{"reaper12.voc","reaper13.voc"}:std::vector<std::string>{"reaper14.voc","reaper15.voc"}):
        action==9?(keyed?std::vector<std::string>{"ogre.voc","golem15.voc","ogre.voc","golem16.voc"}:std::vector<std::string>{"ogre.voc","golem13.voc","ogre.voc","golem14.voc"}):
        action==10?std::vector<std::string>{"dwarf10.voc","dwarf11.voc","dwarf12.voc"}:
        keyed?std::vector<std::string>{"sphinx10.voc","sphinx11.voc","sphinx12.voc"}:std::vector<std::string>{"sphinx13.voc","sphinx14.voc"};
    check(samples==speech,"Sample phase sequence");
    XeenCombatRandom actual(7),oracle(7);
    std::map<P,unsigned> counts;std::vector<P> phases;std::vector<std::pair<P,unsigned>> mixers;
    std::uint64_t now=0;unsigned frames=0;
    while(!value.finished() && frames<1000) {
        const auto command=value.advance(now,[&](unsigned lo,unsigned hi){return *actual.draw(lo,hi);});
        if(!command){now=value.deadline();continue;}
        ++frames;const auto &f=*command;
        if(f.mixer){mixers.push_back({f.phase,*f.mixer});check(!f.iteration,"Mixer command repeated within a phase");}
        ++counts[f.phase];if(phases.empty() || phases.back()!=f.phase)phases.push_back(f.phase);
        if(f.phase==P::Zoom || f.phase==P::Retreat) {
            const unsigned size=action==8?14:action==9?12:action==10?13:9;
            check(f.sprites.front().frame==0 && unsigned(f.sprites.front().scale)==(f.phase==P::Zoom?size-1-f.iteration:f.iteration),"Zoom/retreat frame order");
            check(f.sprites.size()==(action==8?1:2),"Zoom split frames");
            if(f.sprites.size()==2)check(f.sprites[1].frame==1,"Second background frame");
        }
        if(action==9 && (f.phase==P::Slide || f.phase==P::Speech)) {
            const int y=int(*oracle.draw(0,9))-(f.phase==P::Slide?5:3);
            const int x=int(*oracle.draw(0,f.phase==P::Slide?9:5))-(f.phase==P::Slide?5:3)+(f.phase==P::Slide?-200+16*int(f.iteration):0);
            check(f.sprites.back().frame==2 && f.sprites.back().x==x && f.sprites.back().y==y,"Golem Y-then-X jitter formula");
        } else if(f.phase==P::Speech) {
            const auto mouth=*oracle.draw(action==11?2:0,action==8?4:action==10?9:10);
            check(f.sprites.back().frame==mouth,"Speech frame sequence/RNG interleaving");
        }
        if(f.phase==P::Slide && action==8)check(f.sprites.back().x==200-16*int(f.iteration),"Reaper slide");
        if(f.phase==P::Rise)check(f.sprites.front().scale==15-int(f.iteration) && (f.saveZoom==(f.iteration==0)),"Dwarf rise sequence/background capture");
        if(f.phase==P::Finish && action==10)check(f.sprites.back().frame==1,"Dwarf final mouth frame");
        const auto before=actual.continuation();
        check(!value.advance(now,[&](auto,auto){throw std::runtime_error("Duplicate acquisition drew RNG");return 0u;}),"Unacquired frame advanced");
        value.acquired(now);const auto deadline=value.deadline();
        if(deadline>now) {
            const auto expected=(f.phase==P::Zoom && action<10) || f.phase==P::Slide || (f.phase==P::Speech && action==9)?PresentationClock::after(now,1):
                f.phase==P::Speech && (action==8 || action==10)?PresentationClock::after(now,2):
                f.phase==P::Pause?PresentationClock::after(now,action==8 && counts[P::Pause]==1?7:1):
                f.phase==P::EntryTail || f.phase==P::Growl?deadline:PresentationClock::retraceAfter(PresentationClock::retraceAfter(now));
            check(deadline==expected,"BIOS/retrace wait value");
            check(!value.advance(deadline-1,[&](auto,auto){return 0u;}),"Early deadline advanced");
        }
        check(before==actual.continuation(),"Polling consumed random draws");now=deadline;
    }
    check(value.finished() && actual.continuation()==oracle.continuation(),"Scene did not finish/exact gameplay RNG state");
    const unsigned draws=action==8?20:action==9?102:action==10?30:keyed?105:70;
    if(!resources)check(actual.position()==draws,"Fixed-duration direct draw count");
    check(counts[P::Retreat]==(!keyed && action!=10?(action==8?14:action==9?12:9):0),"DOS refusal retreat condition/count");
    const std::vector<P> expected=action==8?std::vector<P>{P::Zoom,P::Slide,P::Speech,P::Pause,P::Speech,P::Pause}:
        action==9?std::vector<P>{P::Zoom,P::Slide,P::EntryTail,P::Speech,P::Pause,P::Growl,P::Speech}:
        action==10?std::vector<P>{P::Zoom,P::Rise,P::Speech}:std::vector<P>{P::Zoom,P::Speech};
    auto complete=expected;
    if(!counts[P::EntryTail])complete.erase(std::remove(complete.begin(),complete.end(),P::EntryTail),complete.end());
    if(!keyed && action!=10)complete.push_back(P::Retreat);complete.push_back(P::Finish);
    check(phases==complete,"Scene phase order");
    // DAT 0x29838/0x29984 Sphinx, 0x29e92/0x2a060/0x2a453 Golem, 0x2ab98/0x2afd2 Reaper, 0x2b4fd/0x2b702 Dwarf.
    std::vector<std::pair<P,unsigned>> expectedMixers{{P::Speech,48}};
    if(action==9)expectedMixers.insert(expectedMixers.begin(),{P::Slide,48});
    expectedMixers.push_back({(action==9 || action==11) && !keyed?P::Retreat:P::Finish,95});
    check(mixers==expectedMixers,"Entrance mixer sites");
    std::cout<<(resources?"VOC ":"fixed ")<<"action="<<action<<" keyed="<<keyed<<" frames="<<frames<<" speech="<<counts[P::Speech]<<" draws="<<actual.position()<<" elapsed_ns="<<now<<'\n';
}
XeenSaveSnapshot entranceSave(training_test::Inputs &in,const XeenCamera &camera,bool keyed,unsigned action) {
    auto s=in.base();s.camera=camera;
    const auto *site=xeenEntranceSite(action,camera);check(bool(site),"Fixture site");
    if(keyed && site->quest>=0 && site->quest<int(s.questItems.size()))s.questItems[site->quest]=1;
    const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,camera.mapId),in.statistics);
    auto &table=s.map(camera.mapId).monsters;
    table.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
    for(const auto &a:actors)table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
    return s;
}
void captions() {
    using A=XeenEntranceAnimation;using P=A::Phase;
    for(unsigned action=8;action<=11;++action)for(bool keyed:{false,true}) {
        const unsigned length=action==10?107:action==8?(keyed?76:78):action==9?(keyed?72:73):(keyed?73:50);
        unsigned sampleCalls=0,advances=0,prior=0,speechPhases=0;
        std::uint64_t now=0,lastMark=0;bool inSpeech=false;
        A a(action,keyed,[&](const char *){++sampleCalls;return 1000000000ULL;},false,length);
        for(unsigned count=0;count<2000 && !a.finished();++count) {
            auto frame=a.advance(now,[](unsigned lo,unsigned){return lo;});
            if(frame) {
                if(frame->phase==P::Speech && !inSpeech)++speechPhases;
                inSpeech=frame->phase==P::Speech;
                if(frame->caption && *frame->caption!=prior) {
                    const auto tick=PresentationClock::tick(now);
                    if(advances)check(tick-lastMark>=2,"Caption advanced before two BIOS ticks");
                    check(*frame->caption==(prior+1)%length,"Caption skipped a character or failed record wrap");
                    lastMark=tick;prior=*frame->caption;++advances;
                    // Late wake must still advance just one character.
                    if(advances==3)now+=1000000000ULL;
                }
                a.acquired(now);
            }
            now=std::max(now,a.deadline());
        }
        check(a.finished() && !sampleCalls && advances==length && prior==0,"Efx-off caption cycle/sample gate");
        check(speechPhases==(action>=10?1u:2u),"Dwarf/Sphinx later phases not skipped or Reaper/Golem second loop lost");
    }
}
void original(training_test::Inputs &in,const GameInstallation &installation,XeenCamera camera,unsigned action,bool keyed,bool wounded=false,bool efx=true,bool device=false) {
    auto save=entranceSave(in,camera,keyed,action);save.soundOptions.efx=efx;training_test::Fixture f(in,save);
    std::vector<std::string> voices;
    f.programState.audio=XeenAudio::installed(installation,in.assets,0,device);
    f.programState.audio->digitalSink([&](const auto &name,auto){voices.push_back(name);});
    unsigned captions=0;
    f.flow->loadEntranceCaption=[&](unsigned kind,bool hasKey) {
        const auto bytes=in.assets.readArchiveResource("special.bin",XeenSceneArchive::Clouds);
        const unsigned record=kind==10?1:kind==8?(hasKey?2:3):kind==9?(hasKey?4:5):(hasKey?6:7);
        std::size_t start=0;
        for(unsigned i=0;i<record;++i){while(bytes.at(start))++start;++start;}
        auto end=start;while(bytes.at(end))++end;
        const unsigned expected=kind==10?107:kind==8?(hasKey?76:78):kind==9?(hasKey?72:73):(hasKey?73:50);
        check(end-start==expected,"Original entrance record length");
        return std::string(bytes.begin()+start,bytes.begin()+end);
    };
    f.flow->drawEntranceCaption=[&](IndexedFrame &frame,const std::string &text) {
        ++captions;in.assets.drawDialogSprite(frame,"box.vga",0,36,189,XeenSceneArchive::Clouds);
        XeenTextRenderOptions options;options.originalControls=true;options.bounds={0,0,320,200};
        frame=XeenTextRenderer(in.font).render(frame,std::string("\f35\3c\v190\t000")+text,options).pages.front();
    };
    f.flow->drawEntranceSprite=[&](auto &frame,const char *name,unsigned id,int x,int y,int scale){in.assets.drawDialogSprite(frame,name,id,x,y,XeenSceneArchive::Clouds,scale);};
    std::vector<std::pair<std::string,std::uint64_t>> samples;
    f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){
        const auto duration=xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));
        samples.emplace_back(name,duration);return duration;
    };
    if(!efx)check(XeenEntranceTestAccess::serviceSample(*f.flow)==f.flow->presentationNow() && samples.empty(),
                  "Efx-off service greeting retained a VOC-busy interval");
    unsigned failures=0;
    f.flow->reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r)){++failures;std::cerr<<e->message<<'\n';}};
    std::optional<unsigned> hurt;if(wounded)hurt=XeenEntranceTestAccess::wound(*f.flow);
    const auto time=*f.p.encounterContext;const auto rng=*f.w.sessionState().journeyRandom();
    f.act(InteractionAction{});
    check(XeenEntranceTestAccess::active(*f.flow) && !f.flow->canSave(),"Original Event did not enter scene");
    if(efx)check(XeenEntranceTestAccess::noNotice(*f.flow),"Audible entrance published an unsupported-audio notice");
    const auto context=f.flow->inputContext(f.flow->frame().presentation());
    check(!context.readyForAction && context.hideCursor,"Entrance input/cursor policy");
    for(unsigned i=0;i<4000 && !f.flow->canSave();++i) {
        if(const auto deadline=f.flow->nextPresentationDeadline())f.now=std::max(f.now,PresentationClock::ceilMilliseconds(*deadline));
        f.flow->beginCycle(++f.cycle);if(const auto frame=f.flow->updatePresentation())f.present(*frame);
    }
    check(!failures && !XeenEntranceTestAccess::active(*f.flow) && f.flow->canSave(),"Original entrance never returned to gameplay");
    check(xeen_state::sameCamera(f.c,xeenEntranceDestination(*xeenEntranceSite(action,camera),camera,XeenCloudsQuestItems(save.questItems))),"Original entrance destination/position/facing");
    check(f.p.encounterContext->day==time.day && f.p.encounterContext->minutes==time.minutes && f.p.encounterContext->year==time.year,"Entrance charged game/service time");
    check(f.w.sessionState().journeyRandom()->count>rng.count,"Scene did not consume gameplay RNG");
    if(!efx)check(samples.empty() && captions>0,"Efx-off entrance resolved samples or omitted caption");
    if(!efx)check(voices.empty(),"Muted entrance published a sample");
    else {
        std::vector<std::string> expected;
        for(const auto &sample:samples) {
            if(sample.first=="ogre.voc")continue;
            expected.push_back(sample.first);
        }
        auto spoken=voices;
        if(action==9) {
            spoken.erase(std::remove(spoken.begin(),spoken.end(),"ogre.voc"),spoken.end());
            check(voices.front()=="ogre.voc" && std::count(voices.begin(),voices.end(),"ogre.voc")>=2,"Golem initial/repeated/between-speech growls missing");
        }
        check(spoken==expected,"Entrance starts differ from acquired speech/growl phases");
    }
    if(efx && action==11 && !keyed) {
        check(samples==std::vector<std::pair<std::string,std::uint64_t>>{{"sphinx13.voc",(39245ULL-10+1)*68000},{"sphinx14.voc",(18753ULL-10+1)*68000}},
            "Sphinx refusal must retain the damaged-descriptor sample's driver-count duration");
        check(f.now>=3943,"Sphinx refusal skipped a speech phase");
    }
    if(hurt) {
        const auto &actor=f.w.sessionState().regionalActors(camera.mapId).at(*hurt);
        check(actor.hp==actor.statistics->baseHp() && actor.status==XeenActorStatus::Physical && !actor.activated,"Keyless reload did not rebuild distant monster");
    }
    const auto snapshot=f.snapshot();training_test::Fixture restored(in,snapshot);
    check(XeenSaveFormat::encode(snapshot)==XeenSaveFormat::encode(restored.snapshot()),"Entrance current-save roundtrip changed bytes");
}
}
int main(int argc,char **argv) {try {
    sites();captions();for(unsigned action=8;action<=11;++action)for(bool keyed:{false,true})if(action!=10 || keyed)animation(action,keyed);
    if(argc>1) {
        const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing original installation");
        training_test::Inputs in(*installation);
        for(unsigned action=8;action<=11;++action)for(bool keyed:{false,true})if(action!=10 || keyed)animation(action,keyed,&in);
        original(in,*installation,{24,10,9,XeenDirection::North},8,true);
        original(in,*installation,{24,10,9,XeenDirection::North},8,false,true);
        original(in,*installation,{23,5,13,XeenDirection::North},10,true);
        original(in,*installation,{23,5,13,XeenDirection::North},10,true,false,false);
        original(in,*installation,{5,12,8,XeenDirection::South},11,false);
        original(in,*installation,{5,12,8,XeenDirection::South},11,false,false,true,true);
        for(const auto map:{8u,5u}) {
            const unsigned action=map==8?9:11;const auto events=in.events.load(map);
            bool tested=false;
            for(const auto &r:events.records)if(r.opcode==0x11 && r.line==0 && r.parameters==std::vector<std::uint8_t>{std::uint8_t(action)}) {
                original(in,*installation,{map,r.x,r.y,r.direction<4?XeenDirection(r.direction):XeenDirection::North},action,true);tested=true;break;
            }
            check(tested,"No original Golem/Sphinx entrance record");
        }
    }
    std::cout<<"Entrance scene tests passed\n";return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
