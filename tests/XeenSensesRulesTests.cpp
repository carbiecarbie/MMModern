#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenTerrainRules.h"
#include "games/xeen/XeenRestRules.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenScenePresentation.h"
#include "games/xeen/XeenEventValues.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
void handlers() {
    using R=XeenLearnedSpellRules;using Q=XeenQuickSpellCheck;
    constexpr unsigned ids[]{42,41,74,4,15,34},powers[]{1,20,32,18,34,23},sp[]{1,5,7,5,6,5};
    for(unsigned n=0;n<6;++n) {
        const auto id=ids[n];const auto spell=*R::supported(id);
        check(R::supportedIn(id,true)==spell && xeenItemUseSpell(powers[n])==spell && xeenItemUseSpell(powers[n],true)==spell,"Book/item exploration/combat dispatch");
        check(R::cost(id,7).sp==sp[n] && R::cost(id,7).gems==(id==4?2u:0u),"Fixed senses cost");
        XeenCharacter c;c.characterClass=id==74?XeenCharacterClass::Cleric:XeenCharacterClass::Sorcerer;
        c.hasSpells=true;c.permanentLevel=7;c.learnedSpells.emplace();
        c.currentSpell=*R::slotForSpell(*R::categoryForClass(c.characterClass),id);
        c.learnedSpells->at(c.currentSpell)=1;c.currentSp=sp[n];
        check(R::quickSpellCheck(c,2)==Q::Supported,"Quick Fight admitted");
        --c.currentSp;check(R::check(c,c.currentSpell,2,false)==Q::InsufficientSp,"SP guard");
        ++c.currentSp;if(id==4)check(R::quickSpellCheck(c,1)==Q::InsufficientGems,"Gem guard");
        c.learnedSpells->at(c.currentSpell)=0;check(R::quickSpellCheck(c,2)==Q::Unsupported,"Knowledge guard");
        XeenGameplayContext ctx;ctx.effects.fill(99);ctx.lightAndResistances.fill(100);
        R::applyPartyEffect(spell,ctx,255);
        if(id==42)check(ctx.lightAndResistances[0]==101,"Light increments rather than assigns level");
        if(id==41)check(ctx.effects[XeenPartyEffect::Levitate]==1 && ctx.effects[XeenPartyEffect::WizardEye]==99,"Levitate counter slot, independent of Wizard Eye");
        if(id==74)check(ctx.effects[XeenPartyEffect::WalkOnWater]==1,"Walk on Water flag");
        if(id==4)check(ctx.effects[XeenPartyEffect::Clairvoyance]==1,"Clairvoyance flag");
        xeenResetPartyTemps(ctx);
        for(auto i:kXeenPartyEffects)if(i!=XeenPartyEffect::AutomapOn)check(!ctx.effects[i],"Temporary reset clears senses");
        check(ctx.effects[XeenPartyEffect::AutomapOn]==99,"Temporary reset preserves automap-on");
        check(!ctx.lightAndResistances[0],"Temporary reset clears light");
    }
    check(R::prepareEffect(XeenLearnedSpell::Light,XeenPartyState{}, {},610).stepped,"Light marks stepped on illuminated maps");
    XeenGameplayContext ctx;
    for(unsigned before:{0u,255u,32767u,65534u,65535u}) {
        ctx.lightAndResistances[0]=before;R::applyPartyEffect(XeenLearnedSpell::Light,ctx);
        check(ctx.lightAndResistances[0]==((before+1)&65535),"DOS Light word wrap");
    }
}
void movementAndArrival() {
    XeenPartyState holders;holders.party=XeenParty::fromRosterIds({0,1,2});
    holders.roster.at(0).conditions[13]=1; // Dead holders count.
    holders.roster.at(1).conditions[12]=1; // Unconscious holders count.
    XeenMapGeometry outdoor;outdoor.flags2=0x8000;outdoor.surfaceTypes[0]=0;
    XeenMapCell destination;destination.geometry=XeenOutdoorLayers{};
    for(unsigned skill:{9u,11u})for(unsigned count:{0u,1u,2u}) {
        for(unsigned id=0;id<3;++id)holders.roster.at(id).skills[skill]=id<count?1:0;
        const auto caps=XeenMovement::capabilities(holders);
        check((skill==9?caps.mountaineer:caps.pathfinder)==(count==2),"Traversal counts two holders regardless of conditions");
        for(unsigned wall:skill==9?std::vector<unsigned>{1,7,9,10,12}:std::vector<unsigned>{3,6}) {
            xeenGet<XeenOutdoorLayers>(destination.geometry).middle=wall;
            check((XeenMovement::outdoorDestination(outdoor,destination,caps)==XeenMovementResult::Moved)==(count==2),
                "Every skill wall requires two holders and bypasses its surface check");
            XeenWorld crossing([&](auto id) {XeenMap m;m.geometry.id=id.number;m.geometry=outdoor;m.geometry.id=id.number;
                m.geometry.cells.fill(destination);return m;});
            for(auto action:{NavigationAction::MoveForward,NavigationAction::MoveBackward,NavigationAction::StrafeLeft,NavigationAction::StrafeRight}) {
                XeenCamera c{23,8,8,XeenDirection::North};
                check((XeenMovement().apply(crossing,c,action,caps)==XeenMovementResult::Moved)==(count==2),"Strafe/backward share the skill check");
                if(count<2)check(c.x==8 && c.y==8 && c.mapId==XeenMapIdentity(23) && c.direction==XeenDirection::North,"Refused skill move changed camera");
            }
        }
    }
    xeenGet<XeenOutdoorLayers>(destination.geometry).middle=5;
    check(XeenMovement::outdoorDestination(outdoor,destination,{false,false,true,true})==XeenMovementResult::BlockedBySurface,
        "Clouds wall five retains its surface check");
    XeenMapGeometry map;map.flags2=0x8000; // outdoor geometry
    XeenMapCell cell;cell.geometry=XeenOutdoorLayers{};
    for(unsigned surface:{0u,8u,15u})for(bool swim:{false,true})for(bool walk:{false,true}) {
        map.surfaceTypes[0]=surface;
        const bool allowed=surface==0?(swim || walk):surface==8?walk:false;
        check((XeenMovement::outdoorDestination(map,cell,{swim,walk,false})==XeenMovementResult::Moved)==allowed,"Shallow/deep/space capability combinations");
    }
    for(bool swim:{false,true})for(bool walk:{false,true}) {
        XeenWorld world([](auto id) {XeenMap m;m.side=id.side;m.geometry.id=id.number;m.geometry.difficulties[0]=10;
            for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{};
            m.geometry.cells[6*16+5].surfaceIndex=4;return m;});
        XeenCamera c{23,5,5,XeenDirection::North};
        check((XeenMovement().apply(world,c,NavigationAction::MoveForward,{swim,walk,false})==XeenMovementResult::Moved)==(swim || walk),"Indoor raw slot four accepts Swimming or Walk on Water");
    }
    XeenPartyState party;party.party=XeenParty::fromRosterIds({0,1});party.encounterContext.emplace();
    party.roster.at(0).skills[14]=1;
    check(!XeenMovement::capabilities(party).swimming,"Swimming needs every active member");
    party.roster.at(1).skills[14]=1;party.roster.at(1).conditions[13]=1;
    check(XeenMovement::capabilities(party).swimming,"Swimming has no condition filter");
    party.encounterContext->effects[XeenPartyEffect::WalkOnWater]=7;check(XeenMovement::capabilities(party).walkOnWater,"Nonzero water flag");
    XeenCamera camera{23,5,5,XeenDirection::West};
    for(unsigned surface:{10u,13u}) {
        XeenWorld world([&](auto id) {XeenMap m;m.side=id.side;m.geometry.id=id.number;m.geometry.flags2=0x8000;
            m.geometry.surfaceTypes.fill(surface);for(auto &c:m.geometry.cells)c.geometry=XeenOutdoorLayers{};return m;});
        XeenGameplayContext context;context.effects[XeenPartyEffect::WizardEye]=1;context.effects[XeenPartyEffect::WalkOnWater]=1;
        std::array<XeenCharacter,6> characters{};
        check(world.prepareTerrainArrival(camera,characters,context).falling,"Wizard Eye and Walk on Water do not protect cloud/sky");
        context.effects[XeenPartyEffect::Levitate]=1;
        check(world.prepareTerrainArrival(camera,characters,context).falling==(surface==10),"World arrival reads Levitate, not Wizard Eye");
        xeenResetPartyTemps(context);
        check(world.prepareTerrainArrival(camera,characters,context).falling,"Rest/dawn reset restores cloud fall");
        party.encounterContext=context;
        XeenGameFlags flags;XeenEventScriptValues script;
        const XeenEventValueOwners owners{party,flags,camera,script};
        check(xeenEventSetValue(owners,69,1,0).status==XeenEventWriteStatus::Applied,
            "Event 69 must set Levitate");
        check(party.encounterContext->effects[XeenPartyEffect::Levitate]==1 &&
            !party.encounterContext->effects[XeenPartyEffect::Clairvoyance] &&
            world.prepareTerrainArrival(camera,characters,*party.encounterContext).falling==(surface==10),
            "Event 69 Levitate reaches the cloud consumer, while sky still falls");
    }
    for(bool levitating:{false,true}) {
        check(xeenTerrainArrival(13,true,false,levitating,camera).falling==!levitating,"Only Levitate protects cloud");
        check(xeenTerrainArrival(10,true,false,levitating,camera).falling,"Sky ignores Levitate");
        check(xeenTerrainArrival(5,true,false,levitating,camera).damage==100,"Levitate gives no lava immunity");
        check(!xeenTerrainArrival(0,true,false,levitating,camera).damage && !xeenTerrainArrival(8,true,false,levitating,camera).damage,"Water has no arrival damage");
        check(xeenTerrainFallDestination(camera).mapId==23,"Unconditional scripted fall has no Levitate guard");
        check(xeenRestTerrain(13,true,false,levitating)==0,"Rest admits cloud, then clears Levitate before arrival");
    }
    auto dark=camera;dark.mapId={XeenSide::Darkside,35};
    for(unsigned surface:{10u,13u}) {
        const auto clear=xeenTerrainArrival(surface,true,false,false,dark,false);
        const auto set=xeenTerrainArrival(surface,true,false,false,dark,true);
        const auto clouds=xeenTerrainArrival(surface,true,false,false,camera,true);
        check(clear.falling && clear.damage==100 && !set.falling && !set.damage && clouds.falling && clouds.damage==100,
            "Darkside flag 118 must suppress falling and damage only on Darkside");
    }
    check(!xeenTerrainArrival(5,true,false,false,dark,true).damage,"Darkside flag 118 also suppresses lava damage");
    check(xeenTerrainArrival(15,true,false,false,dark,true).dead,"Darkside flag 118 preserves space death");
    check(xeenTerrainArrival(6,true,false,false,dark,true).minutes==170,"Darkside flag 118 preserves desert time");
}
void information() {
    using R=XeenLearnedSpellRules;
    const XeenCamera camera{23,8,8,XeenDirection::East};
    std::vector<XeenActor> actors(7);
    for(unsigned i=0;i<actors.size();++i) {
        auto &a=actors[i];a.id={23,i};a.x=5;a.y=11;
        a.statistics.emplace();a.hp=1234+i;a.statistics->raw[22]=20+i;
        a.statistics->raw[24]=2+i;a.statistics->raw[30]=i;
    }
    actors[4].x=11;actors[4].y=5;actors[5].x=12;actors[5].y=8;actors[6].x=8;actors[6].y=8;
    std::array<std::optional<XeenMonsterIdentity>,3> slots{};
    auto result=*R::information(XeenLearnedSpell::DetectMonster,camera,actors,slots);
    check(result.detection && !result.failed && result.facing==XeenDirection::East,"Detection empty/heading admission");
    check(result.grid[0]==3 && result.grid[48]==1 && result.grid[24]==1,"North-up positions and three-marker cap");
    unsigned count=0;for(auto n:result.grid)count+=n;check(count==5,"Detection excludes position beyond three cells");
    check(R::information(XeenLearnedSpell::IdentifyMonster,camera,actors,slots)->failed,"Identify empty slots fails independently of combat mode");
    slots[1]=actors[2].id;slots[2]=actors[0].id;
    result=*R::information(XeenLearnedSpell::IdentifyMonster,camera,actors,slots);
    check(!result.failed && !result.monsters[0] && result.monsters[1] && result.monsters[2],"Identify preserves up to three occupied slot positions");
    const auto &m=*result.monsters[1];
    check(m.name==actors[2].statistics->name() && m.hp==1236 && m.ac==22 && m.attacks==4 && m.special==2,"Identify reports current HP and original name/AC/attacks/special");
}
void border() {
    unsigned phase=0;
    for(unsigned n=1;n<=24;++n)check(XeenScenePresentation::levitateBorderFrame(n>12,phase)==(n>12?16+(n-1)%12:16),"Levitate cycles even while inactive");
    for(unsigned phase=0;phase<4;++phase)for(unsigned left=0;left<3;++left)for(unsigned right=0;right<3;++right) {
        const auto frames=XeenScenePresentation::clairvoyanceFrames(left,right,phase);
        check(frames.first==(left==2?0:phase+(left==0?4:0)),"Left numeric hint frames");
        check(frames.second==(right==2?8:phase+(right==0?8:12)),"DOS right hint-zero frames 8..11");
        check(XeenScenePresentation::clairvoyanceFrames(left,right,phase,true).second==52,"Overlapping window right statue");
    }
}
void lighting() {
    using P=XeenScenePresentation;
    for(unsigned flags:{0u,0x4000u,0x8000u,0xc000u})for(unsigned light:{0u,1u,65535u})
        check(P::blackView(flags,light)==(bool(flags&0x4000) && light==0),"Only the dark flag with zero light makes a black view");
    for(bool pending:{false,true})for(unsigned attr:{0u,0x20u})for(unsigned light:{0u,1u,2u,65535u})
        check(P::checkedLight(light,attr,pending)==light-unsigned(pending && attr && light),"Drain spends one nonzero light word at a pending check");
    check(!XeenLearnedSpellRules::prepareEffect(XeenLearnedSpell::Light,{}, {},610,{},true).stepped,"Light in blackness does not mark stepped");
    check(XeenLearnedSpellRules::prepareEffect(XeenLearnedSpell::Light,{}, {},610,{},false).stepped,"Light in a clear view marks stepped");
    XeenWorld world([](auto id) {XeenMap m;m.geometry.id=id.number;m.geometry.flags2=0xc000;
        for(auto &cell:m.geometry.cells) {cell.geometry=XeenOutdoorLayers{};cell.rawAttributes=0x20;}return m;});
    XeenPartyState p;p.encounterContext.emplace();p.encounterContext->lightAndResistances[0]=2;
    XeenCamera camera{23,8,8,XeenDirection::North};
    check(world.lightAfterScenePreparation(p,camera).light==2,"Fresh/restored presentation never invents a step");
    world.scenePresentation().markLightingStep();
    p.encounterContext->lightAndResistances[0]=world.lightAfterScenePreparation(p,camera).light;
    check(p.encounterContext->lightAndResistances[0]==1 && !world.sceneIsBlack(p,camera),"First stepped drain remains clear");
    world.scenePresentation().lightingPrepared=true;
    check(world.lightAfterScenePreparation(p,camera).light==1,"Retry retains its prepared counter");
    world.scenePresentation().completeLightingDraw();
    check(world.lightAfterScenePreparation(p,camera).light==1,"Redraw spends no light");
    world.scenePresentation().markLightingStep();
    p.encounterContext->lightAndResistances[0]=world.lightAfterScenePreparation(p,camera).light;
    check(world.sceneIsBlack(p,camera),"Drain to zero makes the scene black immediately");
}
}
int main() {try {handlers();movementAndArrival();information();border();lighting();std::cout<<"Senses, traversal and lighting rules passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
