#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "formats/xeen/XeenVocDuration.h"
#include <iostream>
#include <sstream>
using namespace training_test;
namespace mmodern {
struct XeenRestTestAccess {
    static bool rest(const XeenEventFlow &f){return bool(f._encounter->_rest);}
    static bool confirm(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Confirm;}
    static bool refused(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Refused;}
    static unsigned charges(const XeenEventFlow &f){return f._encounter->_rest?f._encounter->_rest->charges:0;}
    static bool teleport(const XeenEventFlow &f){return bool(f._teleportUi);}
    static bool complete(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Complete;}
    static bool dream(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Dream;}
    static void composer(XeenEventFlow &f,XeenEventFlow::TransitionCompose compose){f._transitionCompose=std::move(compose);}
    static std::string state(const XeenEventFlow &f) {
        const auto &e=*f._encounter;
        return " teleport="+std::to_string(bool(f._teleportUi))+" cast="+std::to_string(bool(e._casting))+
            " committed="+std::to_string(e._casting && e._casting->committed)+" done="+std::to_string(e._casting && e._casting->effectDone)+
            " ui="+(f._castingUi?std::to_string(unsigned(f._castingUi->phase)):"none")+" refusal="+(f._castingUi?f._castingUi->refusal:"")+" notice="+e.notice();
    }
};
}
namespace {
using R=XeenLearnedSpellRules;using S=XeenLearnedSpell;
std::string scenario;
void tick(Fixture &f){f.now+=125;f.flow->beginCycle(++f.cycle);if(auto image=f.flow->updatePresentation())f.present(*image);}
void key(Fixture &f,unsigned k){const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
    check(bool(input),"Travel input missing");const auto action=input->key(k);check(bool(action),"Travel key missing");f.act(*action);}
void wire(Fixture &f){f.flow->loadRestDream=[&]{return f.in.assets.restDreamImage();};f.flow->loadSampleDuration=[&](auto name,auto archive){return xeenVocDurationNanoseconds(f.in.assets.readArchiveResource(name,archive));};}
void quiet(Fixture &f){for(unsigned n=0;n<1500 && !f.flow->canSave();++n) {
    check(!f.flow->encounter()->combat(),"Travel unexpectedly entered combat");
    check(XeenRestTestAccess::charges(*f.flow)==0,"Super Shelter created an ordinary interruption opportunity");
    const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
    if(XeenRestTestAccess::confirm(*f.flow))key(f,'y');else if(input && input->anyKey)f.act(AcknowledgeAction{});
    tick(f);
}if(!f.flow->canSave())throw std::runtime_error("Travel never reached Quiet"+XeenRestTestAccess::state(*f.flow));}
XeenSaveSnapshot source(Inputs &in,unsigned spell,bool combat=false) {
    auto s=in.service();s.camera={23,9,11,XeenDirection::North};
    for(auto map:{23,28})for(auto &a:s.map(map).monsters->slots) {a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    for(auto id:s.activeRosterIds) {auto &c=s.characters[id];c.currentHp=1000;c.conditions.fill(0);c.miscellaneous={};c.weapons={};c.armor={};c.accessories={};
        s.journey->supplements[id].inputs.speed={int(180-id),0};}
    auto &c=s.characters[s.activeRosterIds[0]];c.characterClass=XeenCharacterClass::Sorcerer;c.hasSpells=true;
    c.currentSp=1000;c.permanentLevel=10;c.temporaryLevel=0;c.learnedSpells.emplace();c.learnedSpells->fill(0);
    c.currentSpell=*R::slotForSpell(XeenSpellCategory::Wizardry,spell);c.learnedSpells->at(c.currentSpell)=1;c.quickOption=1;
    s.journey->supplements[c.rosterId].inputs.speed={250,0};s.journey->context->minutes=600;s.journey->treasure->gems=1000;
    auto &a=s.map(23).monsters->slots.at(7);a.x=1;a.y=1;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
    if(combat){s.camera.direction=XeenDirection::West;auto &enemy=s.map(23).monsters->slots.at(5);enemy.x=8;enemy.y=11;enemy.lifecycle=XeenActorLifecycle::Present;enemy.accounted=false;}
    save_test::clearExplored(s);return s;
}
void roundtrip(Inputs &in,Fixture &f){const auto bytes=XeenSaveFormat::encode(f.snapshot());Fixture loaded(in,XeenSaveFormat::decode(bytes));check(bytes==XeenSaveFormat::encode(loaded.snapshot()),"Travel exact restore changed state");}
void cast(Fixture &f,bool item,unsigned distance=0) {
    if(item){f.act(SelectMemberAction{0});key(f,'i');key(f,'m');key(f,'1');key(f,'u');}
    else {f.act(CastSpellAction{});f.act(SelectMemberAction{0});key(f,'c');}
    if(XeenRestTestAccess::teleport(*f.flow)){if(distance)key(f,'0'+distance);key(f,InputKey::Enter);}
}
void exploration(Inputs &in) {
    for(auto pair:{std::pair{66u,48u},{21u,58u},{70u,39u},{69u,42u}})for(bool item:{false,true})for(bool restricted:{false,true}) {
        scenario="exploration "+std::to_string(pair.first)+" item "+std::to_string(item)+" restricted "+std::to_string(restricted);
        auto s=source(in,pair.first);const auto owner=s.activeRosterIds[0];
        if(pair.first==66){s.camera={23,12,10,XeenDirection::West};s.food=8;s.journey->context->effects.fill(7);s.journey->context->effects[XeenPartyEffect::AutomapOn]=1;}
        if(pair.first==69)s.camera.direction=XeenDirection::West;
        if(restricted)s.camera={28,18,4,XeenDirection::West};
        if(item)s.characters[owner].miscellaneous[0]={1,std::uint8_t(pair.second),2,0};
        const unsigned flag=pair.first==66?0x200:pair.first==21?0x40:pair.first==70?0x400:0x1000;
        const auto loader=[&](auto id){auto map=in.maps.loadGeometryMap(in.assets,id);if(restricted && id==XeenMapIdentity(28))map.geometry.flags|=flag;return map;};
        Fixture f(in,s,false,nullptr,loader);wire(f);XeenTrainingTestAccess::woundTravel(*f.flow);
        const auto rng=f.w.sessionState().journeyRandom();
        const auto original=f.c;cast(f,item,3);
        if(pair.first!=66 || restricted)check(f.w.sessionState().journeyRandom()==rng,"Travel handler drew RNG before the normal response");
        quiet(f);
        const auto cost=R::cost(pair.first,10);
        if(pair.first!=66 || restricted)check(f.p.roster.at(owner).currentSp==1000-int(item?0:cost.sp),"Failed or successful cast keeps fixed SP cost");
        check(f.p.monsterTreasure->gems==1000-(item?0:cost.gems),"Travel gems charged once");
        if(item)check(f.p.roster.at(owner).miscellaneous[0].state==1,"Travel charge spent on success/failure");
        if(restricted)check(f.c.x==original.x && f.c.y==original.y,"Restricted travel published movement");
        else if(pair.first==70)check(f.c.x==10 && f.c.y==12,"Mainland run X/Y outside combat");
        else if(pair.first==21)check(f.c.x==9 && f.c.y==12,"Etherealize one cell");
        else if(pair.first==69)check(f.c.x==6 && f.c.y==11,"Teleport mainland distance");
        check(f.c.direction==original.direction,"Travel changed facing");
        const auto &monster=f.w.sessionState().regionalActors(23).at(7);check(monster.hp==1 && monster.status==XeenActorStatus::Sleep,"Same-map spell reloaded live monsters");
        if(pair.first==66 && !restricted) {
            check(f.p.food==2 && f.p.encounterContext->minutes==600+480+(item?0:10),"Shelter single 480-minute Rest plus learned suffix");
            check(!f.p.encounterContext->effects[XeenPartyEffect::Levitate] && !f.p.encounterContext->effects[XeenPartyEffect::Bless],"Shelter reset before recovery");
        }
        roundtrip(in,f);
    }
    for(unsigned cancel:{0u,1u,2u})for(bool item:{false,true}) {
        auto s=source(in,69);const auto owner=s.activeRosterIds[0];if(item)s.characters[owner].miscellaneous[0]={1,42,2,0};
        Fixture f(in,s);if(item){f.act(SelectMemberAction{0});key(f,'i');key(f,'m');key(f,'1');key(f,'u');}else{f.act(CastSpellAction{});f.act(SelectMemberAction{0});key(f,'c');}
        check(XeenRestTestAccess::teleport(*f.flow),"Teleport original distance prompt absent");
        if(cancel==1)key(f,'0');key(f,cancel==2?InputKey::Escape:InputKey::Enter);quiet(f);
        check(f.c.x==9 && f.c.y==11 && f.p.roster.at(owner).currentSp==1000-(item?0:10),"Teleport cancel moved/refunded");
    }
}
void rest(Inputs &in) {
    auto s=source(in,66);s.camera={23,12,10,XeenDirection::West};
    Fixture ordinary(in,s);wire(ordinary);const auto before=XeenSaveFormat::encode(ordinary.snapshot());
    ordinary.act(RestAction{});check(XeenRestTestAccess::refused(*ordinary.flow),"Flagged mainland cell allowed ordinary Rest");
    ordinary.act(AcknowledgeAction{});quiet(ordinary);check(before==XeenSaveFormat::encode(ordinary.snapshot()),"Rest cell refusal spent resources/time/RNG");
    s.journey->supplements[s.activeRosterIds[0]].inputs.might.permanent=0;
    for(bool item:{false,true}) {
        auto danger=s;const auto owner=s.activeRosterIds[0];if(item)danger.characters[owner].miscellaneous[0]={1,48,2,0};
        Fixture f(in,danger);wire(f);const auto rng=f.w.sessionState().journeyRandom();cast(f,item);
        check(XeenRestTestAccess::confirm(*f.flow),"Shelter bypassed dying-risk warning");key(f,'n');quiet(f);
        check(f.p.food==s.food && f.p.encounterContext->minutes==600+(item?0:10) && f.w.sessionState().journeyRandom()==rng,"Shelter decline ran Rest");
        check(f.p.roster.at(owner).currentSp==1000-(item?0:15),"Shelter decline refunded cost");
    }
    auto starving=source(in,66);starving.camera={23,12,10,XeenDirection::West};starving.food=0;
    starving.journey->context->effects.fill(7);starving.journey->context->effects[XeenPartyEffect::AutomapOn]=1;
    Fixture noFood(in,starving);wire(noFood);cast(noFood,false);quiet(noFood);
    check(!noFood.p.food && !noFood.p.encounterContext->effects[XeenPartyEffect::Levitate] &&
        !noFood.p.encounterContext->effects[XeenPartyEffect::Bless] && noFood.p.roster.at(starving.activeRosterIds[0]).currentHp==1000 &&
        noFood.p.roster.at(starving.activeRosterIds[0]).currentSp==985,
        "Shelter zero-food reset runs before recovery eligibility");
    auto bypass=source(in,66);bypass.camera={28,18,4,XeenDirection::West};
    const auto restrictions=[&](auto id){auto m=in.maps.loadGeometryMap(in.assets,id);if(id!=XeenMapIdentity(23)) {
        m.geometry.flags|=0x4000;for(auto &cell:m.geometry.cells){cell.rawAttributes|=0x40;cell.flags|=0x40;}}
        return m;};
    Fixture both(in,bypass,false,nullptr,restrictions);wire(both);cast(both,false);quiet(both);
    check(both.p.encounterContext->minutes==1081,"Shelter did not bypass ordinary map and cell Rest refusals");
}
void unsupported(Inputs &in) {
    for(unsigned spell:{21u,69u})for(bool item:{false,true}) {
        scenario="unopened destination "+std::to_string(spell)+" item "+std::to_string(item);
        auto s=source(in,spell);unsigned distance=9;
        if(spell==21) {
            XeenWorld w(in.mapLoader(),in.objectLoader());const auto area=XeenMovement::component(w.map(23),9,11,{});bool found=false;
            for(int y=8;y<16 && !found;++y)for(int x=0;x<16 && !found;++x)if(area[y*16+x])for(unsigned d=0;d<4 && !found;++d) {
                const XeenCamera camera{23,x,y,XeenDirection(d)};
                if(R::travel(w,camera,S::Etherealize).unsupported) {s.camera=camera;found=true;}
            }
            check(found,"No original unopened Etherealize destination");
        }
        const auto owner=s.activeRosterIds[0];if(item)s.characters[owner].miscellaneous[0]={1,std::uint8_t(spell==21?58:42),2,0};
        Fixture f(in,s);const auto rng=f.w.sessionState().journeyRandom();
        if(item){f.act(SelectMemberAction{0});key(f,'i');key(f,'m');key(f,'1');key(f,'u');}else{f.act(CastSpellAction{});f.act(SelectMemberAction{0});key(f,'c');}
        if(spell==69){key(f,'0'+distance);key(f,InputKey::Enter);}
        check(f.c.x==s.camera.x && f.c.y==s.camera.y && f.p.roster.at(owner).currentSp==1000 &&
            f.p.monsterTreasure->gems==1000 && f.p.encounterContext->minutes==600 && f.w.sessionState().journeyRandom()==rng,
            "Unopened-area refusal spent position/SP/gems/time/RNG");
        if(item)check(f.p.roster.at(owner).miscellaneous[0].state==2,"Unopened destination spent an item charge");
    }
}
void shelterOrder(Inputs &in) {
    scenario="Shelter dream and nearby monsters";
    auto s=source(in,66);s.camera={23,12,10,XeenDirection::West};s.food=8;
    s.journey->context->effects[XeenPartyEffect::Levitate]=1;s.journey->random=XeenCombatRandom(17).continuation();
    auto &enemy=s.map(23).monsters->slots.at(5);enemy.x=11;enemy.y=10;enemy.lifecycle=XeenActorLifecycle::Present;enemy.accounted=false;
    Fixture f(in,s);wire(f);const auto actors=f.w.sessionState().regionalActors(23);cast(f,false);bool dream=false;
    for(unsigned n=0;!XeenRestTestAccess::complete(*f.flow);++n) {
        check(n<1500 && !f.flow->encounter()->combat() && !XeenRestTestAccess::charges(*f.flow),"Shelter interrupted/charged ordinary opportunities");
        if(XeenRestTestAccess::dream(*f.flow)) {dream=true;check(f.p.food==8 && f.p.encounterContext->effects[XeenPartyEffect::Levitate],"Shelter reset/food ran before dream");}
        tick(f);
    }
    check(dream && f.p.encounterContext->minutes==1080 && f.p.food==2 && !f.p.encounterContext->effects[XeenPartyEffect::Levitate],"Shelter dream/reset/food order");
    const auto &after=f.w.sessionState().regionalActors(23);for(unsigned n=0;n<actors.size();++n)
        check(actors[n].x==after[n].x && actors[n].y==after[n].y && actors[n].hp==after[n].hp && actors[n].status==after[n].status,"Shelter gave nearby monsters an opportunity");
    scenario="Shelter one changeTime call";
    auto partition=source(in,66);partition.characters[partition.activeRosterIds[1]].conditions[11]=2;
    Fixture single(in,partition);wire(single);cast(single,false);
    for(unsigned n=0;!XeenRestTestAccess::complete(*single.flow);++n){check(n<1500,"Shelter partition did not finish");tick(single);}
    // Reference-only per-call recovery: one Paralysis gate, then one dream draw.
    check(single.w.sessionState().journeyRandom()->count==partition.journey->random->count+2,
        "Shelter partitioned 480 minutes into multiple changeTime calls");
    scenario="Shelter cloud arrival after Levitate";
    s=source(in,66);s.journey->context->effects[XeenPartyEffect::Levitate]=1;
    for(auto owner:s.activeRosterIds)s.characters[owner].permanentLevel=100;
    const auto loader=[&](auto id){auto m=in.maps.loadGeometryMap(in.assets,id);if(id==XeenMapIdentity(23))m.geometry.surfaceTypes.fill(13);return m;};
    const auto manifest=[&](const auto &m,const auto &o,const auto &e,const auto &stats){auto original=m;
        original.geometry.surfaceTypes=in.maps.loadGeometryMap(in.assets,23).geometry.surfaceTypes;
        xeenValidateRegionalManifest(original,o,e,stats,in.assets.readInitialResource("maze0023.dat"),in.assets.readInitialResource("maze0023.mob"),in.assets.readInitialResource("maze0023.evt"));};
    Fixture cloud(in,s,false,nullptr,loader,manifest);wire(cloud);bool arrived=false;
    XeenRestTestAccess::composer(*cloud.flow,[&](auto &,const auto &party,const auto &,auto,auto){arrived=true;
        check(!party.encounterContext->effects[XeenPartyEffect::Levitate],"Shelter terrain ran before temporary reset");
        for(auto owner:s.activeRosterIds)check(party.roster.at(owner).currentHp==XeenCharacterRules::maxHp(party.roster.at(owner),{610})-100,"Shelter fall did not follow refill");
        return XeenEventFlow::Composition{frame(),false};});
    cast(cloud,false);for(unsigned n=0;!arrived;++n){check(n<1500,"Shelter cloud never reached existing fall owner");tick(cloud);}
}
void combat(Inputs &in) {
    for(bool quick:{false,true})for(unsigned spell:{66u,21u,69u,70u})for(bool occupied:{false,true}) {
        if(occupied && spell!=70)continue;
        scenario="combat "+std::to_string(spell)+" quick "+std::to_string(quick);
        auto s=source(in,spell,true);
        if(occupied){auto &a=s.map(23).monsters->slots.at(7);a.x=10;a.y=12;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;s.journey->random=XeenCombatRandom(1).continuation();}
        Fixture f(in,s);if(occupied)XeenTrainingTestAccess::woundTravel(*f.flow);f.act(WaitAction{});
        for(unsigned n=0;n<1000 && (!f.flow->encounter()->combat() || f.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n)tick(f);
        const auto fight=[&]()->const XeenCombat &{return *f.flow->encounter()->combat();};check(fight().participant()==0,"Travel caster initiative");
        const auto rng=fight().random();const auto minutes=f.p.encounterContext->minutes;
        const auto before=f.w.sessionState().regionalActors(23);
        f.act(quick?PlayerAction{QuickFightAction{}}:PlayerAction{CastSpellAction{}});
        if(fight().cast() && fight().cast()->phase==XeenCombatCastPhase::Confirm)key(f,'c');
        if(spell!=70){check(f.p.roster.at(s.activeRosterIds[0]).currentSp==1000 && f.c.x==9 && f.c.y==11 && fight().random()==rng,"Combat forbidden travel spent anything");continue;}
        for(unsigned n=0;n<1000 && f.c.x!=10;++n)tick(f);
        check(f.c.x==10 && f.c.y==12 && f.c.direction==XeenDirection::West,"Combat Time Distortion run position/facing");
        check(f.p.roster.at(s.activeRosterIds[0]).currentSp==992,"Combat Time Distortion SP");
        const auto &after=f.w.sessionState().regionalActors(23);for(unsigned n=0;n<before.size();++n)
            check(before[n].hp==after[n].hp && before[n].status==after[n].status,"Combat relocation reloaded monster HP/status");
        check(fight().participants()==0x3f,"Time Distortion issued Run or removed a participant");
        if(occupied){check(fight().contacts()[0] && fight().contacts()[0]->recordIndex==7,"Time Distortion did not recheck destination attack slots");continue;}
        for(unsigned n=0;n<1000 && f.flow->encounter()->combat();++n)tick(f);
        quiet(f);roundtrip(in,f);
        check(f.p.encounterContext->minutes==minutes+1 && f.p.monsterTreasure->gold==s.journey->treasure->gold &&
            f.p.monsterTreasure->gems==1000,"Relocation End changed normal time/reward bookkeeping");
    }
}
}
int main(int argc,char **argv){try{check(argc==2,"usage: travel-original <installation>");const auto detected=xeenTestInstallationDetector().detect(argv[1]);check(bool(detected),"Missing installation");Inputs in(*detected);
    std::ostringstream out;auto *old=std::cout.rdbuf(out.rdbuf());try{exploration(in);rest(in);unsupported(in);shelterOrder(in);combat(in);}catch(...){std::cout.rdbuf(old);throw;}std::cout.rdbuf(old);
    std::cout<<"M76 same-map travel, Shelter, Rest cell refusal, costs, items and exact restore passed\n";return 0;
}catch(const std::exception &e){std::cerr<<scenario<<": "<<e.what()<<'\n';return 1;}}
