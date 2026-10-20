#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenArenaRules.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
#include <sstream>
using namespace mmodern;
using training_test::check;
namespace mmodern {
struct XeenArenaTestAccess {
    static std::optional<XeenPresentationRequest> request(const XeenEventFlow &f) {return f._pending && f._pending->state.pendingPresentation?std::optional<XeenPresentationRequest>(f._pending->state.pendingPresentation->request):std::nullopt;}
    static std::optional<XeenArenaPrompt> prompt(const XeenEventFlow &f) {
        return f._pending && f._pending->state.pendingPresentation?f._pending->state.pendingPresentation->request.arena:std::nullopt;
    }
    static const std::vector<std::string> awards(const XeenEventFlow &f) {return f.journalLines();}
};
}
namespace {
void rules() {
    for(auto pair:std::vector<std::pair<unsigned,unsigned>>{{1,1},{2,2},{3,3},{4,4},{11,0},{12,0},{13,0},{21,1},{22,2},{23,3},{101,1},{111,0}})
        check(xeenArenaSuffix(pair.first)==pair.second,"DOS ordinal suffix");
    for(unsigned level=1;level<=10;++level)for(unsigned roll=1;roll<=7;++roll) {
        auto expected=(level-1)*7+roll;if(expected>67)expected-=3;if(expected==59)expected=60;if(expected==28)expected=29;
        check(xeenArenaSpecies(level,roll)==expected,"Arena species adjustment");
    }
    XeenPartyState party;party.party=XeenParty::fromRosterIds({0,1,2,3,4,5});
    for(unsigned i=0;i<6;++i){party.roster.at(i).rosterId=i;party.roster.at(i).awards[9]=98;}
    check(!xeenArenaMaxed(party),"98 must allow a fight");
    check(xeenArenaAward(party)==99 && xeenArenaMaxed(party),"99 guard");
    for(unsigned i=0;i<6;++i)party.roster.at(i).awards[9]=0;
    party.roster.at(5).awards[9]=99;check(xeenArenaMaxed(party),"Guard must check every active member");
    party.roster.at(5).awards[9]=0;party.roster.at(7).awards[9]=255;
    check(!xeenArenaMaxed(party),"Inactive roster awards must not guard the Arena");
    party.roster.at(0).awards[9]=255;party.roster.at(1).awards[9]=100;
    check(xeenArenaAward(party)==0 && party.roster.at(1).awards[9]==101,"Victory must wrap the byte without saturation");
    for(unsigned limit:{10u,20u}) {
        check(xeenArenaNumber("",limit)==0 && xeenArenaNumber("0",limit)==0,"Numeric cancellation");
        check(xeenArenaNumber(std::to_string(limit),limit)==limit,"Inclusive prompt limit");
        check(!xeenArenaNumber(std::to_string(limit+1),limit) && !xeenArenaNumber("99",limit),"Prompt reprompt threshold");
        check(!xeenArenaNumber("123",limit) && !xeenArenaNumber("a",limit),"Numeric input shape");
    }
    check(xeenTownServiceKnown(85,6) && !xeenTownServiceKnown(28,6),"Arena map admission");
    for(unsigned action:{12u,13u})check(!xeenTownServiceKnown(85,action),"Deferred actions admitted");
}
void generation() {
    std::vector<XeenMonsterRecord> stats(70);
    for(unsigned i=0;i<stats.size();++i){stats[i].raw[20]=i+1;stats[i].raw[49]=i%2?2:0;}
    for(unsigned level:{1u,4u,9u,10u})for(unsigned count:{1u,20u}) {
        XeenCombatRandom random(731),oracle(731);
        auto draw=[&](unsigned lo,unsigned hi){return *random.draw(lo,hi);};
        const auto pool=xeenArenaPool(level,draw);
        for(unsigned n=0;n<5;++n) {
            unsigned species=*oracle.draw(1,7)+(level-1)*7;
            if(species>67)species-=3;if(species==59)species=60;if(species==28)species=29;
            check(pool[n]==species,"Pool must be chosen before all monster fields");
        }
        const auto monsters=xeenArenaMonsters(pool,count,stats,draw);unsigned effects=0;
        for(unsigned i=0;i<count;++i) {
            const auto &a=monsters[i].actor;const auto &anim=monsters[i].animation;
            unsigned x=*oracle.draw(3,11),y=*oracle.draw(2,10);
            if((x==5 || x==10) && (y==4 || y==8))y=5;
            const auto slot=*oracle.draw(0,4),species=pool[slot],frame=*oracle.draw(0,7);
            const auto effect=stats[species].animationEffect();const auto phase=effect?*oracle.draw(0,7):0;
            effects+=effect!=0;
            check(a.id==XeenMonsterIdentity{85,i} && a.x==int(x) && a.y==int(y),"Generated identity/position order");
            check(a.original.tableIndex==slot && a.original.resourceId==int(species) && a.hp==int(species+1),"Slot binding/HP");
            check(a.statistics->raw==stats[species].raw && a.activated && a.lifecycle==XeenActorLifecycle::Present && a.status==XeenActorStatus::Physical,"Awake MON actor");
            check(anim.frame==frame && anim.effect1==effect && anim.effect2==effect && anim.effect3==phase,"Frame/effect order and conditional draw");
        }
        check(random==oracle && random.position()==5+4*count+effects,"Generation draw count/continuation");
        std::cout<<"generation level="<<level<<" count="<<count<<" effects="<<effects<<" draws="<<random.position()<<'\n';
    }
    for(unsigned x:{5u,10u})for(unsigned y:{4u,8u}) {
        XeenCombatRandom r(std::vector<XeenCombatRandom::Draw>{{3,11,x},{2,10,y},{0,4,4},{0,7,7}});
        const auto generated=xeenArenaMonsters({2,2,2,2,2},1,stats,[&](unsigned lo,unsigned hi){return *r.draw(lo,hi);});
        check(generated[0].actor.y==5 && generated[0].animation.effect3==0 && r.position()==4,"Position correction/zero effect draw");
    }
}
using Fixture=training_test::Fixture;
void pump(Fixture &f,unsigned times=1) {
    for(unsigned i=0;i<times;++i) {
        if(const auto d=f.flow->nextPresentationDeadline())f.now=std::max(f.now,PresentationClock::ceilMilliseconds(*d));
        f.flow->beginCycle(++f.cycle);if(const auto next=f.flow->updatePresentation())f.present(*next);
    }
}
void answer(Fixture &f,PlayerAction action) {
    if(const auto d=f.flow->nextPresentationDeadline())f.now=std::max(f.now,PresentationClock::ceilMilliseconds(*d));
    f.act(std::move(action));pump(f,5);
}
void phase(Fixture &f,XeenArenaPhase p) {
    const auto prompt=XeenArenaTestAccess::prompt(*f.flow);
    if(!prompt || prompt->phase!=p) {const auto r=XeenArenaTestAccess::request(*f.flow);std::cerr<<"Arena phase want="<<int(p)<<" actual="<<(prompt?int(prompt->phase):-1)<<" kind="<<(r?int(r->kind):-1)<<" camera="<<unsigned(f.c.mapId.number)<<":"<<f.c.x<<","<<f.c.y<<","<<unsigned(f.c.direction)<<" refusal="<<f.flow->encounter()->journeyRefusal()<<"\n";}
    check(prompt && prompt->phase==p,"Unexpected Arena dialog phase");
    const auto request=XeenArenaTestAccess::request(*f.flow);
    const bool numeric=p==XeenArenaPhase::Level || p==XeenArenaPhase::Count;
    if(p!=XeenArenaPhase::Pool) {
        check(request->npc && request->npc->portraitId==27 && request->npc->confirmationMode==(numeric?2:p==XeenArenaPhase::Offer?0:1),
            "Battle Master confirmation mode");
        check(request->response==(numeric?XeenPresentationResponseRequirement::WordInput:p==XeenArenaPhase::Offer?
            XeenPresentationResponseRequirement::YesNo:XeenPresentationResponseRequirement::Acknowledgment),"Battle Master response requirement");
        if(numeric) {
            const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
            check(input && input->textEntry,"Numeric prompt added an acknowledgement before input");
        }
    }
}
XeenSaveSnapshot start(training_test::Inputs &in,unsigned award=0) {
    auto s=in.base();s.camera={6,13,4,XeenDirection::West};
    const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,6),in.statistics);
    auto &table=s.map(6).monsters;table.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
    for(const auto &a:actors)table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
    for(auto owner:s.activeRosterIds) {
        auto &c=s.characters[owner];c.conditions.fill(0);c.permanentLevel=30;c.currentHp=2000;
        c.awards[9]=0;
    }
    s.characters[s.activeRosterIds[0]].awards[9]=award;return s;
}
void walk(Fixture &,int,int,XeenDirection);
void setup(Fixture &f) {
    f.flow->loadArenaSprites=[&](std::uint8_t image){f.in.assets.validateNormalMonster(image);f.in.assets.validateAttackMonster(image);};
    f.flow->loadJournalText=[&](bool awards){return f.in.assets.readArchiveResource(awards?"award.bin":"qnotes.bin",XeenSceneArchive::DarksideOnly);};
    f.flow->drawDialogSprite=[&](auto &frame,const char *name,unsigned index,int x,int y) {
        f.in.assets.drawDialogSprite(frame,name,index,x,y,std::string_view(name)=="award.icn"?XeenSceneArchive::DarksideOnly:XeenSceneArchive::Current);
    };
    f.flow->reportManual=[](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r))throw std::runtime_error(e->message);};
    answer(f,NavigationAction::MoveForward);
    check(f.c.mapId==6 && f.c.x==12 && f.c.y==4,"Preset step did not reach Warzone entrance");
    check(f.w.sampleCell(6,12,4)->cell->rawAttributes==0,"Entrance must keep its original manual trigger");
    if(!XeenArenaTestAccess::request(*f.flow))answer(f,InteractionAction{});
    for(unsigned i=0;i<12 && f.c.mapId!=XeenMapIdentity(85);++i) {
        const auto request=XeenArenaTestAccess::request(*f.flow);
        if(request && request->response==XeenPresentationResponseRequirement::YesNo)answer(f,YesAction{});
        else answer(f,AcknowledgeAction{});
        pump(f,10);
    }
    if(f.c.mapId!=85) {const auto r=XeenArenaTestAccess::request(*f.flow);std::cerr<<"entrance pending="<<(r?int(r->kind):-1)<<" response="<<(r?int(r->response):-1)<<" refusal="<<f.flow->encounter()->journeyRefusal()<<'\n';}
    check(f.c.mapId==85 && f.c.x==10 && f.c.y==10,"Original Warzone entrance did not load map 85");
    walk(f,7,6,XeenDirection::North);
}
auto npc(training_test::Inputs &in) {return [&in](auto &frame,std::uint8_t id,std::size_t frameId){in.assets.drawNpc(frame,id,frameId);};}
void offer(Fixture &f) {
    answer(f,InteractionAction{});phase(f,XeenArenaPhase::Offer);
    const auto request=XeenArenaTestAccess::request(*f.flow);
    check(request->response==XeenPresentationResponseRequirement::YesNo && request->npc->confirmationMode==0,
        "Battle Master offer must request original Yes/No confirmation");
    // Verify the acquired pixels, not just the still-active mouse rectangles.
    // Ordinary scene rebasing used to erase these three original sprites.
    const auto buttons=[&] {
        auto expected=f.flow->frame();
        f.in.assets.drawDialogSprite(expected,"global.icn",7,232,74);
        f.in.assets.drawDialogSprite(expected,"confirm.icn",0,235,75);
        f.in.assets.drawDialogSprite(expected,"confirm.icn",2,260,75);
        check(expected.pixels==f.flow->frame().pixels,"Battle Master confirmation buttons are not visible");
    };
    buttons();pump(f,20);phase(f,XeenArenaPhase::Offer);buttons();
}
void dismiss(Fixture &f) {for(unsigned i=0;i<12 && XeenArenaTestAccess::prompt(*f.flow);++i)answer(f,AcknowledgeAction{});pump(f,20);}
void waitsForAcknowledgment(Fixture &f,XeenArenaPhase p) {
    phase(f,p);f.now+=20000;pump(f);phase(f,p);
    const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
    check(input && input->anyKey && input->anyClick,"Battle Master acknowledgement must accept ordinary keys/mouse");
}
void number(Fixture &f,const std::string &digits) {answer(f,TextInputAction{digits});answer(f,DialogKeyAction{InputKey::Enter});}
void generated(Fixture &f) {
    const auto time=*f.p.encounterContext;
    offer(f);answer(f,DialogKeyAction{'y'});phase(f,XeenArenaPhase::Level);
    number(f,"11");phase(f,XeenArenaPhase::Level);number(f,"1");phase(f,XeenArenaPhase::Count);
    number(f,"21");phase(f,XeenArenaPhase::Count);number(f,"1");waitsForAcknowledgment(f,XeenArenaPhase::Start);
    const auto before=f.w.sessionState().journeyRandom()->count;
    answer(f,AcknowledgeAction{});pump(f,20);
    check(f.w.sessionState().regionalActors(85).size()==1,"Arena generation did not publish a monster");
    check(f.w.sessionState().mapState(85)->originalMonsterCount==1,"Arena live count must match generated actors");
    const auto &a=f.w.sessionState().regionalActors(85).front();
    check(f.w.sessionState().journeyRandom()->count-before==9+(a.statistics->animationEffect()!=0),"Original generation gameplay draw budget");
    const auto &g=f.w.map(85).geometry;
    check(f.c.mapId==85 && f.c.x==g.runX && f.c.y==g.runY && f.c.direction==XeenDirection::East,"Generated exit must use Warzone run position");
    check(f.p.encounterContext->minutes==time.minutes && f.p.encounterContext->day==time.day && f.p.encounterContext->year==time.year,"Arena dialog charged service time");
}
XeenCamera travel(XeenWorld &world,XeenPartyState &party,const XeenCamera &from,const XeenCamera &to) {
    // An explicit Event-fixture travel control. The fight witness itself walks
    // and fights normally; these controls isolate unfinished/visit lifetimes.
    XeenEventRecord record;record.x=from.x;record.y=from.y;record.direction=4;
    record.opcode=7;record.parameters={std::uint8_t(to.mapId.number),std::uint8_t(to.x),std::uint8_t(to.y)};record.lengthField=8;
    const XeenEventScript script(XeenEventFile{from.mapId,"Arena travel fixture",true,{record}});
    const auto result=XeenEventInterpreter().begin(from,party,{},world,[&](auto){return script;},{});
    if(const auto *bad=std::get_if<XeenEventExecutionError>(&result))throw std::runtime_error(bad->message);
    check(std::holds_alternative<XeenEventExecutionCompleted>(result),"Fixture travel did not finish");
    return std::get<XeenEventExecutionCompleted>(result).finalCamera;
}
void unfinishedAndReentry(Fixture &f) {
    auto world=f.w.transitionCandidate();XeenPartyState party;world->copyEventParty(f.p,party);
    const auto master=travel(*world,party,f.c,{85,7,6,XeenDirection::East});
    const auto scripts=[&](auto id){return XeenEventScript(f.in.events.load(id));};
    const auto texts=[&](auto id){return f.in.texts.load(id);};
    const auto random=*world->sessionState().journeyRandom();
    const auto before=world->sessionState().regionalActors(85);
    auto result=XeenEventInterpreter().begin(master,party,{},*world,scripts,texts);
    const auto *request=std::get_if<XeenEventExecutionSuspended>(&result);
    check(request && request->request.arena && request->request.arena->phase==XeenArenaPhase::Unfinished &&
        request->request.textIndex==4 && request->request.text==texts(85).strings[4] && request->request.npc->portraitId==27 &&
        request->request.npc->confirmationMode==1 && request->request.response==XeenPresentationResponseRequirement::Acknowledgment,
        "Unfinished fight must use original Battle Master text 4 and acknowledgement (reference confirm=300)");
    result=XeenEventInterpreter().resume(std::move(request->state),XeenPresentationResponse::Acknowledged,party,*world,scripts,texts);
    check(std::holds_alternative<XeenEventExecutionCompleted>(result),"Unfinished fight did not exit");
    const auto camera=std::get<XeenEventExecutionCompleted>(result).finalCamera;
    const auto &g=world->map(85).geometry;
    check(camera.mapId==85 && camera.x==g.runX && camera.y==g.runY && camera.direction==XeenDirection::East,"Unfinished fight destination");
    check(world->sessionState().journeyRandom()==random && party.party.member(party.roster,0).awards[9]==0,"Unfinished fight changed award or random state");
    const auto &after=world->sessionState().regionalActors(85);
    check(after.size()==before.size() && after[0].hp==before[0].hp && after[0].activated,"Unfinished fight rebuilt the actor table");
    const auto outside=travel(*world,party,camera,{6,12,4,XeenDirection::East});
    check(!world->sessionState().mapState(85),"Leaving the Warzone retained its map state");
    travel(*world,party,outside,{85,10,10,XeenDirection::East});
    check(world->sessionState().hasRegionalActors(85) && world->sessionState().regionalActors(85).empty() &&
        world->sessionState().mapState(85)->originalMonsterCount==0,"Re-entered Warzone must have an empty table");
}
// Walk on the original geometry. No camera/actor substitution in the fight witness.
void walk(Fixture &f,int x,int y,XeenDirection direction) {
    for(unsigned steps=0;steps<80 && (f.c.x!=x || f.c.y!=y);++steps) {
        const auto dir=f.c.x<x?XeenDirection::East:f.c.x>x?XeenDirection::West:f.c.y<y?XeenDirection::North:XeenDirection::South;
        for(unsigned turn=0;turn<4 && f.c.direction!=dir;++turn)answer(f,NavigationAction::TurnRight);
        check(f.c.direction==dir,"Arena route could not turn");
        const auto before=f.c;answer(f,NavigationAction::MoveForward);
        check(f.c.x!=before.x || f.c.y!=before.y,"Arena walking route is blocked");
    }
    check(f.c.x==x && f.c.y==y,"Arena walking route incomplete");
    for(unsigned turn=0;turn<4 && f.c.direction!=direction;++turn)answer(f,NavigationAction::TurnRight);
    check(f.c.direction==direction,"Arena final facing could not turn");
}
void originals(training_test::Inputs &in) {
    for(bool mouse:{false,true}) {Fixture f(in,start(in),false,nullptr,{}, {},npc(in));setup(f);
        check(f.w.sessionState().regionalActors(85).empty(),"Warzone load must discard normal MOB monsters");
        offer(f);
        const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
        check(input && input->click(261,76) && input->button('n')->frame==2,"Battle Master No mouse button");
        answer(f,mouse?*input->click(261,76):*input->key('n'));waitsForAcknowledgment(f,XeenArenaPhase::Decline);dismiss(f);
        check(xeen_state::sameCamera(f.c,{6,12,4,XeenDirection::West}),"Decline destination");
    }
    for(bool cancelCount:{false,true})for(bool zero:{false,true}) {Fixture f(in,start(in),false,nullptr,{}, {},npc(in));setup(f);
        offer(f);
        const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
        check(input && input->click(236,76) && input->button('y')->frame==0,"Battle Master Yes mouse button");
        answer(f,zero?*input->click(236,76):*input->key('y'));phase(f,XeenArenaPhase::Level);
        if(cancelCount)number(f,"1");
        const auto before=*f.w.sessionState().journeyRandom();if(zero)number(f,"0");else answer(f,CancelInteractionAction{});pump(f,20);
        check(f.w.sessionState().regionalActors(85).empty() && *f.w.sessionState().journeyRandom()==before,"Cancel generated monsters or drew RNG");
        const auto &g=f.w.map(85).geometry;check(f.c.x==g.runX && f.c.y==g.runY && f.c.direction==XeenDirection::East,"Cancel destination");
    }
    {Fixture f(in,start(in),false,nullptr,{}, {},npc(in));setup(f);generated(f);unfinishedAndReentry(f);}
    {Fixture f(in,start(in,99),false,nullptr,{}, {},npc(in));setup(f);
        answer(f,InteractionAction{});waitsForAcknowledgment(f,XeenArenaPhase::Maxed);dismiss(f);
        const auto &g=f.w.map(28).geometry;check(f.c.mapId==28 && f.c.x==g.runX && f.c.y==g.runY && f.c.direction==XeenDirection::East,"Maxed destination");
    }
    {Fixture f(in,start(in,98),false,nullptr,{}, {},npc(in));setup(f);generated(f);
        // Original awake actor approaches via ordinary time/turns and is killed
        // through player combat commands, with all normal treasure settlement.
        for(unsigned n=0;n<200;++n) {
            const auto &a=f.w.sessionState().regionalActors(85).front();
            if(a.lifecycle==XeenActorLifecycle::Defeated && !f.flow->blocksGameplay())break;
            if(f.flow->encounter()->combat())answer(f,AttackAction{});
            else answer(f,NavigationAction::MoveForward);
            if(f.flow->inputContext(f.flow->frame().presentation()).dialog)answer(f,AcknowledgeAction{});
            pump(f,10);
        }
        check(f.w.sessionState().regionalActors(85).front().lifecycle==XeenActorLifecycle::Defeated,"Original Arena fight did not finish");
        walk(f,7,6,XeenDirection::North);answer(f,InteractionAction{});waitsForAcknowledgment(f,XeenArenaPhase::Victory);
        dismiss(f);
        const auto &g=f.w.map(28).geometry;check(f.c.mapId==28 && f.c.x==g.runX && f.c.y==g.runY && f.c.direction==XeenDirection::East,"Victory destination");
        for(unsigned i=0;i<6;++i)check(f.p.party.member(f.p.roster,i).awards[9]==(i?1:99),"Victory award count");
        auto saved=f.snapshot();check(!saved.find(85),"Victory save must exclude Warzone state");Fixture restored(in,saved,false,nullptr,{}, {},npc(in));
        check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(restored.snapshot()),"Arena awards current-format roundtrip");
        answer(f,SelectMemberAction{0});answer(f,DialogKeyAction{1015});
        const auto awards=XeenArenaTestAccess::awards(*f.flow);
        check(std::any_of(awards.begin(),awards.end(),[](const auto &line){return line.find("99")!=std::string::npos;}),"Existing Awards screen did not show Warzone count");
        answer(f,CancelInteractionAction{});answer(f,CancelInteractionAction{});
        // The earned 99 survives a new visit and takes the maximum-count exit.
        auto world=f.w.transitionCandidate();XeenPartyState party;world->copyEventParty(f.p,party);
        const auto master=travel(*world,party,f.c,{85,7,6,XeenDirection::East});
        const auto scripts=[&](auto id){return XeenEventScript(in.events.load(id));};
        const auto texts=[&](auto id){return in.texts.load(id);};
        auto result=XeenEventInterpreter().begin(master,party,{},*world,scripts,texts);
        auto *request=std::get_if<XeenEventExecutionSuspended>(&result);
        check(request && request->request.arena && request->request.arena->phase==XeenArenaPhase::Maxed,"Earned 99 did not block the next fight");
        result=XeenEventInterpreter().resume(std::move(request->state),XeenPresentationResponse::Acknowledged,party,*world,scripts,texts);
        check(std::holds_alternative<XeenEventExecutionCompleted>(result) && std::get<XeenEventExecutionCompleted>(result).finalCamera.mapId==28,"Repeated maxed exit did not load map 28");
    }
}
}
int main(int argc,char **argv) {try {
    rules();generation();
    if(argc>1) {const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation missing");training_test::Inputs in(*installation);std::ostringstream quiet;auto *previous=std::cout.rdbuf(quiet.rdbuf());try {originals(in);}catch(...){std::cout.rdbuf(previous);throw;}std::cout.rdbuf(previous);}
    std::cout<<"Arena tests passed\n";return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
