#include "XeenTestInstallation.h"
#include "app/XeenAudio.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenEntranceScene.h"
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/CloudsMapComposer.h"
#include "formats/xeen/XeenVoc.h"
#include "SyntheticXeenArchive.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>

using namespace mmodern;
using training_test::check;
namespace mmodern {
struct XeenAudioTestAccess {
    static std::vector<std::uint8_t> resource(const std::filesystem::path &folder,const std::string &name,
        XeenSceneArchive archive,const XeenAudio::Loader &fallback) {
        return XeenAudio::readProgramResource(folder,name,archive,fallback);
    }
};
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
    const auto &rows=xeenEntranceSites();check(rows.size()==30,"Entrance row count");
    unsigned dark=0,dwarf=0;
    for(const auto &site:rows) {
        if(site.side!=XeenSide::Clouds)continue;
        const int x=site.position==P::EqualX?site.branchX:site.position==P::OtherX?site.branchX+1:10;
        XeenCamera camera{site.map,x,9,D::West};
        check(xeenEntranceSite(site.action,camera)==&site,"Shared row lookup/position branch");
        check(xeenTownServiceKnown(camera.mapId,site.action),"Entrance row not admitted");
        XeenEventRecord record;record.opcode=0x11;record.parameters={std::uint8_t(site.action)};record.lengthField=6;record.x=x;record.y=9;
        check(std::holds_alternative<XeenDecodedEventInstruction>(XeenEventDecoder::decode(record,{camera.mapId})),"Table row not decoded");
        const auto empty=xeenEntranceDestination(site,camera,{});
        check(xeen_state::sameCamera(empty,site.quest<0?site.destination:camera),"Keyless destination");
        XeenQuestItems::Counts counts{};
        if(site.quest>=0 && site.quest<int(counts.size()))counts[site.quest]=256;
        else counts.fill(255);
        const XeenQuestItems items(counts);
        const auto destination=xeenEntranceDestination(site,camera,items);
        if(site.quest>=35) {
            ++dark;check(!xeenEntranceKey(site,items) && xeen_state::sameCamera(destination,camera),"Darkside key must be never held");
            check(site.destination.mapId.side==XeenSide::Darkside,"Darkside row lost its destination archive");
        } else check(xeenEntranceKey(site,items) && xeen_state::sameCamera(destination,site.destination),"Nonzero whole quest counter check");
        dwarf+=site.action==10;
    }
    check(dark==4 && dwarf==5,"Darkside/Dwarf branch coverage");
    check(!xeenEntranceSite(8,{28,10,9,D::North}) && !xeenTownServiceKnown(28,8),"Unknown map admitted");
    for(unsigned action:{6u,13u})check(!xeenTownServiceKnown(24,action),"Out-of-scope scene admitted");
    check(!xeenEntranceSite(8,{{XeenSide::Darkside,24},10,9,D::North}),"Darkside scene assets admitted");
    // Destination constants are independent of iteration over the implementation table.
    const XeenCamera expected[]{ {55,3,8,D::East},{59,11,8,D::West},{51,7,12,D::South},
        {{XeenSide::Darkside,113},7,4,D::North},{{XeenSide::Darkside,117},7,4,D::North},
        {81,1,17,D::East},{80,29,16,D::West},{79,5,16,D::East},{{XeenSide::Darkside,121},18,0,D::North},
        {37,1,4,D::East},{35,1,12,D::East},{36,7,1,D::North},{33,7,1,D::North},{34,7,30,D::South},
        {82,7,5,D::North},{{XeenSide::Darkside,125},7,6,D::North}};
    for(unsigned i=0;i<std::size(expected);++i)check(xeen_state::sameCamera(rows[i].destination,expected[i]),"Destination table constant");
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

void darksideSites() {
    using D=XeenDirection;using S=XeenSide;
    struct Row {unsigned action,map,key,destination;int x,y;D facing;};
    const Row expected[]{
        {8,3,40,57,11,8,D::West},{8,13,43,69,7,4,D::North},{8,16,41,61,7,12,D::South},
        {8,23,42,65,3,8,D::East},{8,29,44,53,11,8,D::West},
        {9,12,47,73,0,7,D::East},{9,22,48,78,8,14,D::South},
        {9,14,49,83,11,1,D::North},{9,19,50,121,18,0,D::North},
        {13,4,35,29,15,31,D::South},{13,6,38,35,15,8,D::West},{13,19,36,31,31,16,D::West},
        {13,22,37,33,0,3,D::East},{13,98,39,37,7,0,D::North}};
    for(const auto &r:expected) {
        const XeenCamera at{{S::Darkside,std::uint16_t(r.map)},7,6,D::East};
        const auto *site=xeenEntranceSite(r.action,at);
        check(site && site->quest==int(r.key) && site->side==S::Darkside,"Darkside entrance site/key");
        check(!xeenEntranceKey(*site,{}) && xeen_state::sameCamera(xeenEntranceDestination(*site,at,{}),at),"Darkside refusal location");
        XeenQuestItems::Counts keys{};keys[r.key]=256;const XeenQuestItems held(keys);
        check(xeenEntranceKey(*site,held) && xeen_state::sameCamera(xeenEntranceDestination(*site,at,held),
            {{S::Darkside,std::uint16_t(r.destination)},r.x,r.y,r.facing}),"Darkside whole key counter/arrival");
        XeenEventRecord e;e.opcode=17;e.lengthField=6;e.parameters={std::uint8_t(r.action)};e.x=7;e.y=6;
        check(xeenTownServiceKnown(at.mapId,r.action) && std::holds_alternative<XeenDecodedEventInstruction>(XeenEventDecoder::decode(e,{at.mapId})),"Darkside scene decoder admission, action 13 included");
    }
    for(unsigned minute=0;minute<1440;++minute)check(xeenNight(minute)==(minute<300 || minute>=1260),"Shared night rule changed the sky");
    for(bool keyed:{false,true}) {
        check(xeenEntranceCaptionRecord(8,keyed,S::Darkside)==(keyed?2u:3u),"Tower caption record");
        check(xeenEntranceCaptionRecord(9,keyed,S::Darkside)==(keyed?5u:4u),"Dungeon caption record");
        check(xeenEntranceCaptionRecord(13,keyed,S::Darkside)==(keyed?7u:8u),"Town caption record");
    }
}
void darksideAnimation(training_test::Inputs *resources=nullptr) {
    using A=XeenEntranceAnimation;using P=A::Phase;
    for(unsigned action:{8u,9u,13u})for(bool keyed:{false,true})for(bool efx:{false,true})for(bool night:{false,true}) {
        const unsigned length=action==8?(keyed?71:42):action==9?(keyed?50:84):(keyed?57:52);
        std::vector<std::string> loaded,spoken;
        A value(action,keyed,[&](const char *name){loaded.emplace_back(name);return resources?
            xeenVocDurationNanoseconds(resources->assets.readArchiveResource(name,XeenSceneArchive::DarksideOnly)):1000000000ULL;
        },efx,efx?0:length,false,false,XeenSide::Darkside,night);
        std::vector<P> phases;std::vector<std::pair<P,unsigned>> mixers;std::map<P,unsigned> counts;
        unsigned captionFrames=0,draws=0,speechPhases=0,cursor=0,cursorAdvances=0;std::uint64_t now=0;
        while(!value.finished() && counts[P::Speech]<3000) {
            const auto f=value.advance(now,[&](unsigned lo,unsigned hi){
                check(lo==0 && hi==(action==8?4u:action==9?6u:8u),"Darkside speech random range");++draws;return hi;
            });
            if(!f){now=value.deadline();continue;}
            ++counts[f->phase];if(phases.empty() || phases.back()!=f->phase){phases.push_back(f->phase);speechPhases+=f->phase==P::Speech;}
            if(f->mixer)mixers.push_back({f->phase,*f->mixer});
            if(f->caption){
                ++captionFrames;check(speechPhases==1 && !efx,"Caption outside first speech/Efx gate");
                if(*f->caption!=cursor){check(*f->caption==(cursor+1)%length,"Darkside caption cursor skipped a character");cursor=*f->caption;++cursorAdvances;}
            }
            if(f->phase==P::Speech) {
                check(f->sprites.back().frame==(action==8?9u:action==9?6u:8u),"Darkside paired mouth frame");
                check(f->sprites.back().x==(action==13?0:160),"Darkside paired mouth position");
            }
            if(f->phase==P::Zoom || f->phase==P::Retreat) {
                const unsigned count=action==8?14:action==9?9:11;
                check(f->sprites.front().scale==int(f->phase==P::Zoom?count-1-f->iteration:f->iteration),"Darkside zoom/retreat order");
                check(f->sprites.size()==(action==9?2u:3u),"Darkside zoom halves/night layer");
                if(action!=9)check(f->sprites.back().frame==2+night,"Approach/retreat night frame");
            }
            if(f->phase==P::Slide) {
                const auto &left=f->sprites[f->sprites.size()-2];const auto &right=f->sprites.back();
                check(left.x==-200+16*int(f->iteration) && left.frame==0 && right.x==left.x+160 && right.frame==(action==8?5u:1u),"Darkside slide halves");
                if(action==8)check(f->sprites[2].frame==2+night,"Tower slide keeps night");
            }
            if(f->phase==P::Pause && action==9)check(f->sprites.back().frame==0 && std::string(f->sprites.back().resource)=="dung3.zom","Dungeon resting dung3 frame");
            if(f->phase==P::Rise)check(f->sprites.front().scale==15-int(f->iteration) && f->sprites.front().x==10*(15-int(f->iteration)),"Town rise geometry");
            if(f->phase==P::Finish && action==13)check(f->sprites.size()==1 && std::string(f->sprites[0].resource)=="town2.zom","Town final body without separate mouth");
            const auto *sample=value.acquired(now);if(sample)spoken.emplace_back(sample);
            const auto deadline=value.deadline();
            if(f->phase==P::Zoom || f->phase==P::Slide || f->phase==P::Rise || f->phase==P::Retreat) {
                const auto expected=(f->phase==P::Zoom && action!=13) || f->phase==P::Slide?
                    PresentationClock::after(now,1):PresentationClock::retraceAfter(PresentationClock::retraceAfter(now));
                check(deadline==expected,"Darkside explicit wait/unpaced retrace cadence");
            }
            if(f->phase==P::Pause)check(deadline==PresentationClock::after(now,action==9?10:counts[P::Pause]==1?7:1),"Darkside unskippable pause");
            if(f->phase==P::Speech)check(deadline==PresentationClock::after(now,action==9?1:2),"Darkside speech wait");
            if(resources)for(const auto &sprite:f->sprites) {
                auto frame=training_test::frame();resources->assets.drawDialogSprite(frame,sprite.resource,sprite.frame,sprite.x,sprite.y,XeenSceneArchive::DarksideOnly,sprite.scale);
            }
            now=deadline;
        }
        check(value.finished() && draws==counts[P::Speech],"Darkside scene completion/exact draw sites");
        if(!efx)check(cursorAdvances==length && !cursor,"Darkside caption must wrap exactly once");
        std::vector<P> expected{P::Zoom};if(action==13)expected.push_back(P::Rise);else expected.push_back(P::Slide);
        expected.push_back(P::Speech);
        if(action!=13 || efx){expected.push_back(P::Pause);expected.push_back(P::Speech);}
        if(action==8)expected.push_back(P::Pause);
        if(!keyed && action!=13)expected.push_back(P::Retreat);expected.push_back(P::Finish);
        check(phases==expected,"Darkside scene step list");
        check(counts[P::Retreat]==(!keyed && action!=13?(action==8?14u:9u):0u),"Darkside refusal-only retreat");
        check(mixers==std::vector<std::pair<P,unsigned>>{{P::Speech,48},{action==9 && !keyed?P::Retreat:P::Finish,95}},"Darkside raw mixer order (no Dungeon pre-slide request)");
        const std::vector<std::string> samples=action==8?(keyed?std::vector<std::string>{"howdid1.voc","goin1.voc"}:std::vector<std::string>{"needkey1.voc"}):
            action==9?std::vector<std::string>{"what2.voc",keyed?"go2.voc":"key2.voc"}:std::vector<std::string>{"pass2.voc",keyed?"ok2.voc":"back2.voc"};
        check(efx?(loaded==samples && spoken==samples):(loaded.empty() && spoken.empty() && captionFrames>0),"Darkside voice order/silent Tower first loop/caption gate");
    }
}

XeenSaveSnapshot entranceSave(training_test::Inputs &in,const XeenCamera &camera,bool keyed,unsigned action) {
    auto s=in.base();s.camera=camera;
    if(camera.mapId.side==XeenSide::Darkside)s.darksideSong=7*xeenMapMusicRow(in.maps.loadGeometryMap(in.assets,camera.mapId).geometry);
    const auto *site=xeenEntranceSite(action,camera);check(bool(site),"Fixture site");
    if(keyed && site->quest>=0 && site->quest<int(s.questItems.size()))s.questItems[site->quest]=1;
    const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,camera.mapId),camera.mapId.side==XeenSide::Clouds?in.statistics:XeenMonsterFormat::parse(*in.assets.readMonsterStatistics(camera.mapId.side)));
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
    // The raw digital id, not audible output, admits captions. Null and
    // deferred drivers still run the caption cycle without a sample wait.
    for(unsigned id:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u})for(bool efx:{false,true}) {
        XeenAudioConfig cfg;cfg.digital=id;const bool show=cfg.captions(efx),silent=!cfg.dmaDigital();
        unsigned reads=0,speech=0,captionFrames=0;std::uint64_t now=0;
        A a(11,false,[&](const char *){++reads;return 300000000ULL;},efx,show?4:0,show && efx,silent);
        for(unsigned n=0;n<2000 && !a.finished();++n) {
            auto frame=a.advance(now,[](unsigned lo,unsigned){return lo;});
            if(frame){if(frame->phase==P::Speech){++speech;if(frame->caption)++captionFrames;}a.acquired(now);}
            now=std::max(now,a.deadline());
        }
        check(a.finished() && speech>0 && (captionFrames>0)==show,"Entrance caption gate by digital id/Efx");
        check((reads>0)==(efx && !silent),"Entrance null driver loaded or waited for PCM");
    }
}
void original(training_test::Inputs &in,const GameInstallation &installation,XeenCamera camera,unsigned action,bool keyed,bool wounded=false,bool efx=true,bool device=false,bool drain=false) {
    auto save=entranceSave(in,camera,keyed,action);save.soundOptions.efx=efx;
    const bool dark=camera.mapId.side==XeenSide::Darkside;const auto archive=xeenPictureArchive(camera.mapId);
    save.characters[save.activeRosterIds[0]].skills[4]=1;
    for(auto &m:save.maps)m.explored.reset();
    if(drain)save.journey->context->lightAndResistances[0]=5;
    const auto loader=[&](auto id){auto m=in.maps.loadGeometryMap(in.assets,id);if(drain)for(auto &c:m.geometry.cells)c.rawAttributes|=0x20;return m;};
    training_test::Fixture f(in,save,false,nullptr,loader);
    std::vector<std::string> voices;
    f.programState.audio=XeenAudio::installed(installation,in.assets,0,device,std::filesystem::path("mmodern-nonexistent-rom-fixture"));
    f.programState.audio->digitalSink([&](const auto &name,auto){voices.push_back(name);});
    unsigned captions=0;
    f.flow->loadEntranceCaption=[&](unsigned kind,bool hasKey) {
        const auto bytes=in.assets.readArchiveResource("special.bin",archive);
        const unsigned record=xeenEntranceCaptionRecord(kind,hasKey,camera.mapId.side);
        std::size_t start=0;
        for(unsigned i=0;i<record;++i){while(bytes.at(start))++start;++start;}
        auto end=start;while(bytes.at(end))++end;
        const unsigned expected=dark?(kind==8?(hasKey?71:42):kind==9?(hasKey?50:84):(hasKey?57:52)):
            kind==10?107:kind==8?(hasKey?76:78):kind==9?(hasKey?72:73):(hasKey?73:50);
        check(end-start==expected,"Original entrance record length");
        return std::string(bytes.begin()+start,bytes.begin()+end);
    };
    f.flow->drawEntranceCaption=[&](IndexedFrame &frame,const std::string &text) {
        ++captions;in.assets.drawDialogSprite(frame,"box.vga",0,36,189,archive);
        XeenTextRenderOptions options;options.originalControls=true;options.bounds={0,0,320,200};
        frame=XeenTextRenderer(in.font).render(frame,std::string("\f35\3c\v190\t000")+text,options).pages.front();
    };
    f.flow->drawEntranceSprite=[&](auto &frame,const char *name,unsigned id,int x,int y,int scale){in.assets.drawDialogSprite(frame,name,id,x,y,archive,scale);};
    std::vector<std::pair<std::string,std::uint64_t>> samples;
    f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive requested){
        check(requested==archive,"Entrance sample archive differs from its site");
        const auto duration=xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,requested));
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
    if(dark && action==9) {
        const auto before=*f.w.sessionState().journeyRandom();f.act(DialogKeyAction{InputKey::Escape});
        check(XeenEntranceTestAccess::active(*f.flow) && f.w.sessionState().journeyRandom()==before,"Dungeon accepted a skip key");
    }
    if(efx)check(XeenEntranceTestAccess::noNotice(*f.flow),"Audible entrance published an unsupported-audio notice");
    const auto context=f.flow->inputContext(f.flow->frame().presentation());
    check(!context.readyForAction && context.hideCursor,"Entrance input/cursor policy");
    for(unsigned i=0;i<4000 && !f.flow->canSave();++i) {
        if(const auto deadline=f.flow->nextPresentationDeadline())f.now=std::max(f.now,PresentationClock::ceilMilliseconds(*deadline));
        f.flow->beginCycle(++f.cycle);if(const auto frame=f.flow->updatePresentation())f.present(*frame);
    }
    check(!failures && !XeenEntranceTestAccess::active(*f.flow) && f.flow->canSave(),"Original entrance never returned to gameplay");
    check(xeen_state::sameCamera(f.c,xeenEntranceDestination(*xeenEntranceSite(action,camera),camera,XeenQuestItems(save.questItems))),"Original entrance destination/position/facing");
    check(f.w.sampleCell(f.c.mapId,f.c.x,f.c.y)->cell->stepped,"Entrance return marks its occupied physical cell");
    if(drain)check(f.p.encounterContext->lightAndResistances[0]==3,"Entrance reload and Event caller drain exactly twice");
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
        if(action==9 && !dark) {
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
void renderDarkside(training_test::Inputs &in,const std::filesystem::path &folder) {
    std::filesystem::create_directories(folder);
    for(const auto [map,action]:std::vector<std::pair<unsigned,unsigned>>{{3,8},{12,9},{4,13}})
        for(bool keyed:{false,true})for(bool efx:{false,true})for(bool night:{false,true}) {
        XeenCamera at;bool found=false;
        for(const auto &event:in.events.load({XeenSide::Darkside,std::uint16_t(map)}).records)
            if(event.opcode==17 && event.parameters==std::vector<std::uint8_t>{std::uint8_t(action)}) {
                at={{XeenSide::Darkside,std::uint16_t(map)},event.x,event.y,event.direction<4?XeenDirection(event.direction):XeenDirection::North};found=true;break;
            }
        check(found,"Darkside render site");
        auto seed=entranceSave(in,at,keyed,action);seed.journey->context->minutes=night?1260:600;
        XeenWorld world(in.mapLoader(),in.objectLoader());XeenPartyState party;XeenCamera camera;XeenGameFlags flags;
        XeenSaveState::restoreBeforeGameplay(seed,in.resources(),party,camera,flags,world,[](auto &,const auto &,const auto &,const auto &){});
        CloudsMapComposer composer;auto saved=composer.compose(in.assets,world,party,camera,{party.encounterContext->year});
        auto art=saved,zoom=saved;
        const auto bytes=in.assets.readArchiveResource("special.bin",XeenSceneArchive::DarksideOnly);
        std::size_t start=0;for(unsigned n=0;n<xeenEntranceCaptionRecord(action,keyed,XeenSide::Darkside);++n){while(bytes.at(start))++start;++start;}
        auto end=start;while(bytes.at(end))++end;const std::string caption(bytes.begin()+start,bytes.begin()+end);
        XeenEntranceAnimation a(action,keyed,[&](const char *name){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,XeenSceneArchive::DarksideOnly));},
            efx,efx?0:caption.size(),false,false,XeenSide::Darkside,night);
        XeenCombatRandom random(7);std::uint64_t now=0;unsigned phase=0;std::optional<XeenEntranceAnimation::Phase> previous;
        bool captionPicture=false;
        for(unsigned n=0;n<5000 && !a.finished();++n) {
            const auto command=a.advance(now,[&](unsigned lo,unsigned hi){return *random.draw(lo,hi);});
            if(!command){now=a.deadline();continue;}
            if(command->saveZoom)zoom=art;
            auto frame=command->restore?(command->phase==XeenEntranceAnimation::Phase::Rise?zoom:saved):art;
            for(const auto &sprite:command->sprites)in.assets.drawDialogSprite(frame,sprite.resource,sprite.frame,sprite.x,sprite.y,XeenSceneArchive::DarksideOnly,sprite.scale);
            if(command->caption) {
                const auto cursor=*command->caption,first=cursor>40?cursor-40:0;
                in.assets.drawDialogSprite(frame,"box.vga",0,36,189,XeenSceneArchive::DarksideOnly);
                XeenTextRenderOptions options;options.originalControls=true;options.bounds={0,0,320,200};
                frame=XeenTextRenderer(in.font).render(frame,std::string("\f35\3c\v190\t000")+caption.substr(first,cursor-first),options).pages.front();
            }
            const bool newPhase=previous!=command->phase;
            if(newPhase || (!captionPicture && command->caption && *command->caption>=20)) {
                if(newPhase)++phase;else captionPicture=true;
                const auto name=std::to_string(action)+(keyed?"-keyed":"-refused")+(efx?"-efx":"-text")+(night?"-night":"-day")+"-"+std::to_string(phase)+(newPhase?"":"-caption")+".ppm";
                std::ofstream file(folder/name,std::ios::binary);file<<"P6\n320 200\n255\n";
                for(auto pixel:frame.pixels)file.write(reinterpret_cast<const char *>(frame.palette.data()+3*pixel),3);
                check(bool(file),"Cannot write entrance frame");
            }
            previous=command->phase;art=std::move(frame);a.acquired(now);now=a.deadline();
        }
        check(a.finished(),"Rendered scene did not finish");
    }
}
void looseVocSceneWait(training_test::Inputs &in,const GameInstallation &installation) {
    using Bytes=std::vector<std::uint8_t>;
    const auto folder=std::filesystem::current_path()/("m94-loose-voc-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(folder);
    struct Cleanup {
        std::filesystem::path folder;
        ~Cleanup(){std::error_code error;std::filesystem::remove(folder/"sphinx13.voc",error);std::filesystem::remove(folder/"xeen.cc",error);std::filesystem::remove(folder,error);}
    } cleanup{folder};
    const auto voc=[](unsigned count,std::uint8_t value) {
        Bytes b(32+count,value);const auto word=count+9;b[0x1b]=word;b[0x1c]=word>>8;return b;
    };
    const auto archived=voc(400,150),loose=voc(12000,200),second=voc(300,140);
    sprite_test::archive(folder/"xeen.cc",{{"sphinx13.voc",archived},{"sphinx14.voc",second}});
    {std::ofstream out(folder/"sphinx13.voc",std::ios::binary);out.write(reinterpret_cast<const char *>(loose.data()),loose.size());check(bool(out),"Loose VOC fixture write");}
    GameInstallation synthetic;synthetic.root=folder;synthetic.xeenArchive=folder/"xeen.cc";
    XeenAssetSource archivedAssets(synthetic);
    check(archivedAssets.readArchiveResource("sphinx13.voc",XeenSceneArchive::Clouds)==archived,"Synthetic archived VOC fixture");
    const XeenAudio::Loader fallback=[&](const std::string &name,XeenSceneArchive archive) {
        if(name=="sphinx13.voc" || name=="sphinx14.voc")return archivedAssets.readArchiveResource(name,archive);
        return in.assets.readArchiveResource(name,archive);
    };
    const XeenAudio::Loader loader=[&](const std::string &name,XeenSceneArchive archive) {
        return XeenAudioTestAccess::resource(folder,name,archive,fallback);
    };
    const auto driver=std::make_shared<const Bytes>(in.assets.readArchiveResource("promus",XeenSceneArchive::IntroOnly));
    std::ifstream moduleFile(installation.uiModule->physicalPath,std::ios::binary);
    const Bytes module((std::istreambuf_iterator<char>(moduleFile)),{});
    XeenAudioConfig config;config.music=3;config.digital=10;
    const XeenCamera camera{5,12,8,XeenDirection::South};auto save=entranceSave(in,camera,false,11);save.soundOptions.efx=true;
    training_test::Fixture f(in,save);f.programState.audio=std::make_shared<XeenAudio>(config,driver,loader,module,0,false);
    const auto retained=f.programState.audio->retainDigital("sphinx13.voc",XeenSceneArchive::Clouds);
    const auto playedDuration=retained.sample->durationNanoseconds();
    check(retained.sample->pcm.size()==12000 && retained.sample->pcm.front()==200 && playedDuration==12000ULL*68000 &&
          playedDuration!=xeenVocDurationNanoseconds(archived),"Loose VOC did not override the archived sample");
    std::vector<std::pair<std::string,std::uint64_t>> durations,starts;
    std::optional<std::uint64_t> firstDeadline;
    f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive) {
        const auto duration=f.programState.audio->digitalDuration(name,archive);durations.emplace_back(name,duration);return duration;
    };
    f.flow->drawEntranceSprite=[&](auto &frame,const char *name,unsigned id,int x,int y,int scale){in.assets.drawDialogSprite(frame,name,id,x,y,XeenSceneArchive::Clouds,scale);};
    f.programState.audio->digitalSink([&](const auto &name,auto) {
        const auto now=f.flow->presentationNow();starts.emplace_back(name,now);
        if(name=="sphinx13.voc")firstDeadline=f.programState.audio->digitalDeadline(now);
    });
    f.act(InteractionAction{});check(XeenEntranceTestAccess::active(*f.flow),"Loose VOC fixture did not enter the original scene");
    for(unsigned i=0;i<4000 && starts.size()<2 && !f.flow->canSave();++i) {
        if(const auto deadline=f.flow->nextPresentationDeadline())f.now=std::max(f.now,PresentationClock::ceilMilliseconds(*deadline));
        f.flow->beginCycle(++f.cycle);if(const auto frame=f.flow->updatePresentation())f.present(*frame);
    }
    check(durations.size()>=2 && durations[0].first=="sphinx13.voc" && durations[0].second==playedDuration,
          "Entrance wait did not use the played loose VOC duration");
    check(starts.size()>=2 && starts[0].first=="sphinx13.voc" && starts[1].first=="sphinx14.voc" && firstDeadline &&
          *firstDeadline==starts[0].second+playedDuration && starts[1].second>=*firstDeadline && starts[1].second-*firstDeadline<=40000000ULL,
          "Entrance advanced before the loose sample finished or added an extra wait");
    std::cout<<"Loose VOC scene wait and playback duration passed\n";
}
}
int main(int argc,char **argv) {try {
    sites();darksideSites();darksideAnimation();captions();for(unsigned action=8;action<=11;++action)for(bool keyed:{false,true})if(action!=10 || keyed)animation(action,keyed);
    if(argc>1) {
        const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing original installation");
        training_test::Inputs in(*installation);
        for(unsigned action=8;action<=11;++action)for(bool keyed:{false,true})if(action!=10 || keyed)animation(action,keyed,&in);
        original(in,*installation,{24,10,9,XeenDirection::North},8,true);
        original(in,*installation,{24,10,9,XeenDirection::North},8,false,true);
        original(in,*installation,{24,10,9,XeenDirection::North},8,false,false,true,false,true);
        original(in,*installation,{23,5,13,XeenDirection::North},10,true);
        original(in,*installation,{23,5,13,XeenDirection::North},10,true,false,false);
        original(in,*installation,{5,12,8,XeenDirection::South},11,false);
        for(const auto [map,action]:std::vector<std::pair<unsigned,unsigned>>{{3,8},{12,9},{4,13}}) {
            bool found=false;
            for(const auto &r:in.events.load({XeenSide::Darkside,std::uint16_t(map)}).records)
                if(r.opcode==17 && r.parameters==std::vector<std::uint8_t>{std::uint8_t(action)}) {
                    const XeenCamera at{{XeenSide::Darkside,std::uint16_t(map)},r.x,r.y,r.direction<4?XeenDirection(r.direction):XeenDirection::North};
                    for(bool keyed:{false,true})for(bool efx:{false,true})original(in,*installation,at,action,keyed,false,efx);
                    found=true;break;
                }
            check(found,"Missing original Darkside entrance");
        }
        darksideAnimation(&in);
        if(argc>2)renderDarkside(in,std::filesystem::u8path(argv[2]));
        looseVocSceneWait(in,*installation);
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
