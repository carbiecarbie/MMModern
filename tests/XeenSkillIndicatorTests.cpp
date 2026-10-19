#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenSkills.h"
#include "games/xeen/XeenTextRenderer.h"
#include <iostream>
using namespace mmodern;
namespace {
void check(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
constexpr XeenDirection kFacings[]{XeenDirection::North,XeenDirection::East,XeenDirection::South,XeenDirection::West};

// One active member suffices for Direction Sense (6), Spot Secret Doors (16)
// and Danger Sense (17), whatever that member's condition (0x215b9).
void skillCheck() {
    for(unsigned skill:{6u,16u,17u}) {
        XeenPartyState p;p.party=XeenParty::fromRosterIds({0,1,2});
        check(!xeenPartyHasSkill(p,skill),"Indicator skill without a holder");
        p.roster.at(29).skills[skill]=1;check(!xeenPartyHasSkill(p,skill),"Indicator skill from an inactive roster member");
        p.roster.at(2).skills[skill]=1;check(xeenPartyHasSkill(p,skill),"One holder does not light the indicator");
        p.roster.at(2).conditions[unsigned(XeenCondition::Unconscious)]=1;p.roster.at(2).currentHp=0;
        check(xeenPartyHasSkill(p,skill),"Unconscious holder no longer counts");
    }
}

// The 7x7 window of the movement pass (0x1fea9..0x2015c), before its
// attacking, asleep and line-of-sight tests.
void dangerSense() {
    const XeenCamera camera{23,9,11,XeenDirection::West};
    const auto near=[&](int dx,int dy,XeenActorStatus status=XeenActorStatus::Physical,bool activated=true,
            XeenActorLifecycle lifecycle=XeenActorLifecycle::Present) {
        XeenActor a;a.id={23,0};a.x=camera.x+dx;a.y=camera.y+dy;a.hp=10;a.status=status;a.activated=activated;a.lifecycle=lifecycle;
        return XeenActorApproach::dangerSense({a},camera);
    };
    check(!XeenActorApproach::dangerSense({},camera),"Danger Sense without monsters");
    for(int d:{-3,3}) {
        check(near(d,0) && near(0,d) && near(d,d) && near(d,-d),"Monster three cells away not sensed");
        const int far=d<0?d-1:d+1;
        check(!near(far,0) && !near(0,far) && !near(far,d) && !near(d,far),"Monster four cells away sensed");
    }
    check(near(0,0),"Monster in the party's cell not sensed");
    check(near(2,1,XeenActorStatus::Sleep,false) && near(1,2,XeenActorStatus::DragonSleep,false) &&
        near(-2,0,XeenActorStatus::Hypnotize) && near(0,-1,XeenActorStatus::BeastMaster),"Sleeping or unaware monster not sensed");
    check(!near(1,1,XeenActorStatus::Physical,true,XeenActorLifecycle::Defeated) &&
        !near(1,1,XeenActorStatus::Physical,true,XeenActorLifecycle::Disabled) &&
        !near(1,1,XeenActorStatus::Physical,true,XeenActorLifecycle::Unresolved),"Absent monster sensed");
}

// setMazeBits' thin-wall test (0x12303..0x1231e): facing wall 5 or 8 and a
// cell ahead whose full wall word is not 0x8888.
void thinWall() {
    const std::array<std::uint8_t,4> open{0,8,8,8},solid{8,8,8,8};
    for(unsigned value=0;value<16;++value) {
        const bool lit=value==5 || value==8;
        check(XeenIndoorScene::thinWall(std::uint8_t(value),open)==lit,"Thin-wall facing value test");
        check(!XeenIndoorScene::thinWall(std::uint8_t(value),solid) && !XeenIndoorScene::thinWall(std::uint8_t(value),std::nullopt),
            "Thin wall in front of a solid or missing cell");
    }
    check(!XeenIndoorScene::thinWall(std::nullopt,open),"Thin wall without the party's cell");
    // The same test through the scene's own wall sampling, for every facing.
    for(const auto facing:kFacings)for(unsigned value:{0u,1u,4u,5u,8u,9u})for(bool backed:{false,true}) {
        const int dx=facing==XeenDirection::East?1:facing==XeenDirection::West?-1:0;
        const int dy=facing==XeenDirection::North?1:facing==XeenDirection::South?-1:0;
        XeenWorld world([&](auto id) {XeenMap m;m.geometry.id=id.number;
            for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{};
            xeenGet<XeenIndoorWalls>(m.geometry.cells[8*16+8].geometry).walls.at(unsigned(facing))=value;
            if(backed)for(unsigned side=0;side<4;++side)xeenGet<XeenIndoorWalls>(m.geometry.cells[(8+dy)*16+8+dx].geometry).walls.at(side)=8;
            return m;});
        check(XeenIndoorScene().thinWall(world,{33,8,8,facing})==((value==5 || value==8) && !backed),"Scene thin-wall sampling");
    }
}

// Two counters (0x974e, 0x974f) advance once per border assembly, active or
// not, and are zeroed together, so the two indicators stay in phase.
void counters() {
    XeenScenePresentation presentation(1);
    check(!presentation.dangerSense && !presentation.thinWall && !presentation.textPrompt &&
        !presentation.spotDoorsFrame && !presentation.dangerSenseFrame,"Indicator state does not start clear");
    for(unsigned draw=0;draw<30;++draw) {
        const bool spotActive=draw%5!=0,dangerActive=draw>=7;
        const auto spot=XeenScenePresentation::animatedBorderFrame(spotActive,28,presentation.spotDoorsFrame);
        const auto danger=XeenScenePresentation::animatedBorderFrame(dangerActive,40,presentation.dangerSenseFrame);
        check(spot==(spotActive?28+draw%12:28) && danger==(dangerActive?40+draw%12:40),"Indicator frame left its own counter");
        check(presentation.spotDoorsFrame==presentation.dangerSenseFrame && presentation.spotDoorsFrame==(draw+1)%12,"Counters out of phase");
    }
}

XeenSaveSnapshot mainland(training_test::Inputs &in,std::optional<std::pair<int,int>> monster) {
    auto saved=in.service();saved.camera={23,9,11,XeenDirection::West};
    for(auto owner:saved.activeRosterIds) {auto &c=saved.characters[owner];c.conditions.fill(0);c.currentHp=200;}
    saved.characters[saved.activeRosterIds[3]].skills[17]=1;
    for(auto &a:saved.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    if(monster) {
        auto &a=saved.map(23).monsters->slots[5];a.x=monster->first;a.y=monster->second;
        a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
    }
    return saved;
}
void settle(training_test::Fixture &f) {
    for(unsigned n=0;n<1500 && !f.flow->canSave();++n) {
        f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);
    }
    check(f.flow->canSave(),"Mainland Wait did not settle");
}

void originalDangerSense(training_test::Inputs &in) {
    // Three cells east of the party, unaware: lit after the Wait's pass.
    {
        const auto saved=mainland(in,std::pair{12,11});training_test::Fixture f(in,saved);
        check(!f.w.scenePresentation().dangerSense,"Danger Sense lit before any movement pass");
        f.act(WaitAction{});settle(f);
        check(f.w.scenePresentation().dangerSense && xeenPartyHasSkill(f.p,17),"Danger Sense not lit next to a mainland monster");
        check(!f.flow->encounter()->combat(),"A distant monster started combat");
        // The indicator state is presentation only: the save round trip is exact.
        const auto bytes=XeenSaveFormat::encode(f.snapshot());training_test::Fixture restored(in,XeenSaveFormat::decode(bytes));
        check(bytes==XeenSaveFormat::encode(restored.snapshot()),"Danger Sense save/load not exact");
        check(!restored.w.scenePresentation().dangerSense,"Restored Danger Sense before its first movement pass");
        restored.act(WaitAction{});settle(restored);
        check(restored.w.scenePresentation().dangerSense,"Restored Danger Sense not recomputed by the first pass");
    }
    // No monster on the map: the pass clears a lit indicator.
    {
        const auto saved=mainland(in,std::nullopt);training_test::Fixture f(in,saved);
        f.w.scenePresentation().dangerSense=true;
        f.act(WaitAction{});settle(f);
        check(!f.w.scenePresentation().dangerSense,"Danger Sense lit far from every monster");
    }
    // Adjacent: combat's round start keeps it lit.
    {
        const auto saved=mainland(in,std::pair{8,11});training_test::Fixture f(in,saved);
        f.act(WaitAction{});
        for(unsigned n=0;n<1500 && (!f.flow->encounter()->combat() || f.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady);++n) {
            f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);
        }
        check(f.flow->encounter()->combat() && f.w.scenePresentation().dangerSense,"Danger Sense not lit in mainland combat");
    }
}

void originalBorder(training_test::Inputs &in) {
    // Rivercity (6,7) facing North: a plain wall with open space behind it.
    auto party=XeenPartyLoader().loadFromResources(in.chr,in.pty);
    check(xeenPartyHasSkill(party,16) && xeenPartyHasSkill(party,17) && !xeenPartyHasSkill(party,6),"Initial party skills changed");
    XeenWorld world(in.mapLoader(),in.objectLoader());const CloudsMapComposer composer;
    const auto compose=[&](XeenCamera camera) {return composer.compose(in.assets,world,party,camera,{kCloudsInitialYear});};
    compose({30,6,7,XeenDirection::North});check(world.scenePresentation().thinWall,"Rivercity thin wall not spotted");
    compose({23,9,11,XeenDirection::West});check(world.scenePresentation().thinWall,"Outdoors did not keep the last indoor value");
    compose({30,6,7,XeenDirection::East});check(!world.scenePresentation().thinWall,"Thin wall spotted without one ahead");
    compose({23,9,11,XeenDirection::West});check(!world.scenePresentation().thinWall,"Outdoors changed the last indoor value");
    // Direction Sense: the facing letter, absent without the skill or under a prompt.
    const IndexedFrame blank(320,200,std::vector<std::uint8_t>(320*200));auto &presentation=world.scenePresentation();
    check(composer.drawDirectionSense(in.assets,blank,party,{23,9,11,XeenDirection::North},presentation).pixels==blank.pixels,"Letter without Direction Sense");
    party.roster.at(party.party.activeRosterIds()[1]).skills[6]=1;
    std::vector<std::vector<std::uint8_t>> letters;
    for(unsigned i=0;i<4;++i) {
        const XeenCamera camera{23,9,11,kFacings[i]};
        XeenTextRenderOptions options;options.originalControls=true;options.bounds={0,0,320,200};
        const auto expected=XeenTextRenderer(in.font).render(blank,std::string("\x02\f08\x03" "c\v139\t116")+"NESW"[i]+"\fd\x01",options);
        const auto drawn=composer.drawDirectionSense(in.assets,blank,party,camera,presentation);
        check(drawn.pixels==expected.pages.front().pixels && drawn.pixels!=blank.pixels,"Direction Sense letter");
        for(const auto &other:letters)check(other!=drawn.pixels,"Two facings drew the same letter");
        letters.push_back(drawn.pixels);
        presentation.textPrompt=true;
        check(composer.drawDirectionSense(in.assets,blank,party,camera,presentation).pixels==blank.pixels,"Letter shown under a prompt");
        presentation.textPrompt=false;
    }
}

// The flow raises the prompt flag while Town Portal's digit prompt is open.
void originalPrompt(training_test::Inputs &in) {
    auto saved=mainland(in,std::nullopt);
    auto &c=saved.characters[saved.activeRosterIds[0]];c.miscellaneous={};c.miscellaneous[0]={1,55,5,0};
    training_test::Fixture f(in,saved);
    check(!f.w.scenePresentation().textPrompt,"Prompt flag set without a prompt");
    f.flow->mapName=[&in](auto id){return in.assets.mapName(id);};
    const auto key=[&](unsigned k) {
        const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;check(bool(input),"Items input missing");
        const auto action=input->key(k);check(bool(action),"Items key missing");f.act(*action);
    };
    f.act(SelectMemberAction{0});for(unsigned k:{unsigned('i'),unsigned('m'),unsigned('1'),unsigned('u')})key(k);
    for(unsigned n=0;n<50 && !f.w.scenePresentation().textPrompt;++n) {
        f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);
    }
    check(f.w.scenePresentation().textPrompt,"Town Portal digit prompt did not hide Direction Sense");
    key(InputKey::Escape);
    for(unsigned n=0;n<50 && f.w.scenePresentation().textPrompt;++n) {
        f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);
    }
    check(!f.w.scenePresentation().textPrompt,"Prompt flag kept after the prompt closed");
}
}
int main(int argc,char **argv) {try {
    std::ostringstream output;auto *before=std::cout.rdbuf(output.rdbuf());
    try {
        if(argc==2) {
            const auto installation=xeenTestInstallationDetector().detect(std::filesystem::u8path(argv[1]));
            check(bool(installation),"Original installation missing");
            training_test::Inputs in(*installation);
            originalDangerSense(in);originalBorder(in);originalPrompt(in);
        } else {skillCheck();dangerSense();thinWall();counters();}
    }catch(...){std::cout.rdbuf(before);throw;}
    std::cout.rdbuf(before);
    std::cout<<"Skill indicator sources, frames and letter passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
