#include "XeenTestInstallation.h"
#include "tools/XeenTestSave.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenSkills.h"
#include "games/xeen/XeenItemServiceRules.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <fstream>
#include <iostream>
#include "TestTreeRemoval.h"

using namespace mmodern;
using save_test::check;
using save_test::rejects;
namespace fs=std::filesystem;

namespace mmodern {
struct XeenPartyInterfaceTestAccess {
    static bool reward(const XeenEventFlow &f) {return f._encounter->monsterReward();}
};
}
namespace {
void pulse(training_test::Fixture &f) {f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);}
struct TemporaryDirectory {
    fs::path path=fs::temp_directory_path()/
        ("mmodern-test-save-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    TemporaryDirectory() {check(fs::create_directory(path),"temporary directory already exists");}
    ~TemporaryDirectory() {removeTestTree(path);}
};

void paths(const GameInstallation &installation,const fs::path &repository,const fs::path &directory) {
    const auto output=directory/"refused.mmsave";
    rejects([&] {developer::generateTestSave(installation,"unknown",output);},"Unknown");
    check(!fs::exists(output),"unknown preset wrote a file");
    rejects([&] {developer::generateTestSave(installation,"poisoned",installation.root/"refused.mmsave");},"commercial game installation");
    rejects([&] {developer::generateTestSave(installation,"poisoned",repository/"refused.mmsave");},"outside the repository");
    rejects([&] {developer::generateTestSave(installation,"poisoned",repository/"tools"/"refused.mmsave");},"outside the repository");
    rejects([&] {developer::generateTestSave(installation,"poisoned",directory/"bad.txt");},".mmsave extension");
    rejects([&] {developer::generateTestSave(installation,"poisoned",directory/"absent"/"refused.mmsave");});
    {std::ofstream file(output);file<<"preserve existing file";}
    rejects([&] {developer::generateTestSave(installation,"poisoned",output);},"already exists");
    std::ifstream file(output);std::string contents;std::getline(file,contents);
    check(contents=="preserve existing file","existing file was overwritten");
}

void preset(const GameInstallation &installation,const std::string &name,const fs::path &directory,const fs::path &existing={}) {
    mmodern::XeenProgramState programState{mmodern::XeenWallClock::fixed(0)};
    const auto output=existing.empty()?directory/(name+".mmsave"):existing;
    if(existing.empty())developer::generateTestSave(installation,name,output);
    const auto saved=XeenSaveFile::read(XeenSaveFile::resolve(output,installation.root));
    // Independent original-data providers, followed by the normal startup restore.
    training_test::Inputs inputs(installation);
    XeenPartyState party;XeenCamera camera;XeenGameFlags flags;
    XeenWorld world(inputs.mapLoader(),inputs.objectLoader());
    XeenSaveState::restoreBeforeGameplay(saved,inputs.resources(),party,camera,flags,world,
        [&](auto &w,const auto &p,const auto &c,const auto &) {
            (void)CloudsMapComposer().compose(inputs.assets,w,p,c,{p.encounterContext->year});
        });
    XeenEncounterFlow flow(programState,world,party,camera,flags,[]{return 0;},XeenJourneyRestoreTag{});
    check(flow.prepareJourneyFrame(flow.ticket(),[]{}) && flow.presentJourney(flow.ticket()),"restored preset is not quiet");
    const auto captured=XeenSaveState::capture(inputs.signature,party,camera,flags,world);
    check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(captured),"restore replayed or changed state");
    if(name=="m95-witch-skull") {
        check(save_test::sameCamera(camera,{54,10,6,XeenDirection::North}),"M95 start at the Witch Tower skull");
        check(!party.encounterContext->effects[XeenPartyEffect::WizardEye] && xeenPartyHasSkill(party,4),
            "M95 starts without Wizard Eye and has Cartography");
        std::optional<std::size_t> caster;
        std::size_t member=0;
        for(auto id:party.party.activeRosterIds()) {
            const auto &c=party.roster.at(id);
            check(c.canAct() && c.permanentLevel==30 && c.currentHp>0,"M95 strong, healthy party");
            const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
            if(!category){++member;continue;}
            const auto slot=XeenLearnedSpellRules::slotForSpell(*category,unsigned(XeenLearnedSpell::WizardEye));
            if(slot && c.learnedSpells && c.learnedSpells->at(*slot) && c.currentSpell==*slot &&
               c.currentSp>=XeenLearnedSpellRules::cost(unsigned(XeenLearnedSpell::WizardEye),c.currentLevel()).sp)caster=member;
            ++member;
        }
        check(bool(caster),"M95 can cast Wizard Eye");
        const auto events=inputs.events.load(54);
        check(std::any_of(events.records.begin(),events.records.end(),[](const auto &r) {
            return r.x==10 && r.y==6 && r.line==0 && (unsigned(r.direction)==unsigned(XeenDirection::East) || unsigned(r.direction)==4);
        }),"M95 skull Event faces East from the start cell");
        const auto &actors=world.sessionState().regionalActors(54);
        check(std::any_of(actors.begin(),actors.end(),[](const auto &a){return a.lifecycle==XeenActorLifecycle::Present;}),
            "M95 retains the original Witch Tower monsters");
        training_test::Fixture visit(inputs,saved);
        visit.act(CastSpellAction{});visit.act(SelectMemberAction{*caster});visit.act(DialogKeyAction{'c'});
        for(unsigned n=0;n<1000 && !visit.flow->canSave();++n)pulse(visit);
        check(visit.flow->canSave() && visit.p.encounterContext->effects[XeenPartyEffect::WizardEye],
            "M95 Wizard Eye casts and returns to exploration");
        visit.act(NavigationAction::TurnRight);
        for(unsigned n=0;n<1000 && !visit.flow->canSave();++n)pulse(visit);
        check(visit.flow->canSave() && visit.c.direction==XeenDirection::East,"M95 Right faces the skull");
        visit.act(InteractionAction{});
        check(!visit.flow->canSave() && visit.flow->presentationGeneration(),"M95 Space starts the skull Event");
        return;
    }
    if(name.rfind("m90-",0)==0) {
        const bool gatePreset=name=="m90-newcastle" || name=="m90-golem" || name=="m90-illusion" || name=="m90-high-magic" ||
            name=="m90-darzog" || name=="m90-clouds" || name=="m90-castle" || name=="m90-winterkill";
        const auto expected=name=="m90-newcastle"?XeenCamera{12,11,12,XeenDirection::South}:
            name=="m90-golem"?XeenCamera{20,4,4,XeenDirection::South}:
            name=="m90-illusion"?XeenCamera{45,7,14,XeenDirection::North}:
            name=="m90-high-magic"?XeenCamera{12,6,15,XeenDirection::South}:
            name=="m90-darzog"?XeenCamera{15,4,13,XeenDirection::North}:
            name=="m90-clouds"?XeenCamera{27,15,2,XeenDirection::North}:
            name=="m90-castle"?XeenCamera{27,16,29,XeenDirection::North}:XeenCamera{32,8,13,XeenDirection::North};
        // The Part D spots are bound by the Event check below; the gates keep their fixed cells.
        if(gatePreset)check(save_test::sameCamera(camera,expected),"M90 preset gate position");
        const auto overrides=[&](unsigned map) {
            for(const auto &state:saved.maps)if(state.id.number==map)return state.events.size();
            return std::size_t{0};
        };
        if(name=="m90-emerson-short")check(flags.isSet(45) && party.questItems.at(0)==1 && party.questItems.at(26)==4,"Deed with four credits");
        if(name=="m90-emerson-walls")check(flags.isSet(45) && !flags.isSet(43) && party.questItems.at(26)==5,"Walls payment");
        if(name=="m90-taxman-walls")check(flags.isSet(43) && !party.questItems.at(5) && !overrides(12),"Walls built, stone 5 pending");
        if(name=="m90-emerson-keep")check(flags.isSet(43) && !flags.isSet(44) && party.questItems.at(5)==1 && party.questItems.at(26)==5 && overrides(64)==1,"Keep payment");
        if(name=="m90-taxman-keep")check(flags.isSet(44) && !party.questItems.at(6) && overrides(12)==1,"Keep built, stone 6 pending");
        if(name=="m90-artemus")check(party.questFlags.isSet(22) && party.questItems.at(6)==1 && !party.questItems.at(27) && overrides(64)==2,"Permit pending");
        if(name=="m90-emerson-dungeon")check(party.questItems.at(27)==1 && !flags.isSet(54) && party.questItems.at(26)==5,"Dungeon payment");
        if(name=="m90-taxman-dungeon")check(flags.isSet(54) && overrides(12)==2,"Dungeon built");
        if(name=="m90-laboratory" || name=="m90-sword")check(flags.isSet(54),"Dungeon open");
        if(name=="m90-mirabeth")check(!party.questItems.at(7) && !party.questFlags.isSet(11),"Mirabeth first visit");
        if(name=="m90-mirabeth-elixir")check(party.questItems.at(10)==1 && party.questFlags.isSet(11),"Elixir for Mirabeth");
        if(name=="m90-golem-terrors" || name=="m90-credit-terrors")check(party.questItems.at(5)==1,"Thousand Terrors stone");
        if(name=="m90-golem-stone" || name=="m90-credit-golem")check(party.questItems.at(6)==1,"Golem stone");
        if(name=="m90-slick-trade")for(unsigned doll:{29u,30u,31u,32u})check(party.questItems.at(doll)==1,"Four dolls");
        if(name.rfind("m90-winterkill-",0)==0)check(party.questFlags.isSet(18) && overrides(32)==1,"Winterkill quest accepted");
        if(name=="m90-winterkill-final")check(flags.isSet(8) && flags.isSet(73) && !flags.isSet(1),"Last wave sent");
        if(name.rfind("m90-winterkill-wave",0)==0 || name=="m90-winterkill-final")
            for(const auto &state:saved.maps)if(state.id.number==32 && state.monsters)for(const auto &actor:state.monsters->slots)
                check(actor.lifecycle!=XeenActorLifecycle::Present,"Winterkill cleared before Randon");
        std::cout<<"Loaded "<<name<<" map "<<camera.mapId.number<<" ("<<camera.x<<","<<camera.y<<") facing "<<unsigned(camera.direction)<<"\n";
        // Each start must bind an original line-0 Event, including its facing.
        // Clouds starts facing away from its South-facing return stair.
        const auto events=inputs.events.load(camera.mapId);
        bool gate=false;
        const auto facing=name=="m90-clouds"?XeenDirection::South:XeenDirection(camera.direction);
        for(const auto &r:events.records)gate|=r.x==camera.x && r.y==camera.y && r.line==0 &&
            (unsigned(r.direction)==unsigned(facing) || unsigned(r.direction)==4);
        check(gate,"M90 start must match the original Event cell and facing");
        for(auto id:party.party.activeRosterIds())check(party.roster.at(id).canAct() && party.roster.at(id).permanentLevel==30,"M90 level-30 party");
        if(name=="m90-newcastle")check(party.purse->gold==50000 && party.questItems.at(26)==15 &&
            party.roster.at(party.party.activeRosterIds()[0]).awards[13] && !flags.isSet(45),"Newcastle deed, credits and Crodo prerequisites");
        if(name=="m90-golem")check(party.questItems.at(7)==1 && party.questItems.at(5)==1 && party.questItems.at(6)==1 && !party.questItems.at(26),"Golem keys without credits");
        if(name=="m90-illusion")check(party.purse->gems==300 && !party.questItems.at(3),"Skull purchase prerequisites");
        if(name=="m90-high-magic")check(party.questItems.at(3)==1,"High Magic key");
        if(name=="m90-darzog")check(party.questItems.at(2)==1,"Darzog key");
        if(name=="m90-clouds")check(party.questFlags.isSet(23) && party.purse->gold>=1100 && party.purse->gems>=110,"Both currencies for Slick's games");
        if(name=="m90-castle")check(party.questItems.at(28)==1 && party.roster.at(party.party.activeRosterIds()[0]).weapons[0].id==34,"Cupie Doll and Slayer Sword");
        if(name=="m90-winterkill")check(!party.questFlags.isSet(18) && !flags.isSet(0) && !flags.isSet(1) && !flags.isSet(3) && !flags.isSet(8),"Winterkill before its quest");
        return;
    }
    if(name.rfind("m92-",0)==0) {
        const auto expected=name=="m92-lord-xeen"?XeenCamera{78,5,8,XeenDirection::North}:XeenCamera{78,4,9,XeenDirection::West};
        check(save_test::sameCamera(camera,expected) && !party.cloudsCompleted,"M92 before-ending position and completion");
        check(party.roster.at(party.party.activeRosterIds()[0]).weapons[0].id==34,"M92 Slayer Sword equipped");
        if(name=="m92-sixth-mirror")check(party.questItems.at(8)==1 && flags.isSet(60) && !party.questFlags.isSet(23),"M92 Scepter and killed Lord");
        return;
    }
    if(name.rfind("m89-",0)==0) {
        const auto expected=name=="m89-lord-xeen"?XeenCamera{78,5,8,XeenDirection::North}:
            name=="m89-clan-king"?XeenCamera{41,30,25,XeenDirection::North}:
            name=="m89-draco"?XeenCamera{29,1,13,XeenDirection::North}:
            name=="m89-dragons"?XeenCamera{15,0,12,XeenDirection::North}:XeenCamera{8,2,7,XeenDirection::North};
        check(save_test::sameCamera(camera,expected),"M89 preset position");
        const auto cell=world.sampleCell(camera.mapId,camera.x,camera.y);check(bool(cell),"M89 starting cell");
        auto step=camera;
        check(XeenMovement().apply(world,step,NavigationAction::MoveForward,XeenMovement::capabilities(party))==XeenMovementResult::Moved,
            "M89 fight cell must be reachable on its original surface");
        for(auto id:party.party.activeRosterIds())check(party.roster.at(id).canAct() && party.roster.at(id).permanentLevel==30,"M89 strong party");
        if(name=="m89-lord-xeen")check(party.roster.at(party.party.activeRosterIds()[0]).weapons[0].id==34 &&
            party.roster.at(party.party.activeRosterIds()[0]).weapons[0].frame==13 && party.questFlags.isSet(23),"M89 equipped Slayer Sword and main quest");
        if(name=="m89-clan-king")check(flags.isSet(9) && flags.isSet(10) && !flags.isSet(11) && party.questFlags.isSet(1),"M89 Gunther quest state");
        if(name=="m89-draco")check(party.encounterContext->minutes==1260 && flags.isSet(113) && flags.isSet(125) && flags.isSet(137),"M89 Draco coffin preconditions");
        if(name=="m89-dragons")check(party.questFlags.isSet(17) && party.encounterContext->effects[XeenPartyEffect::WalkOnWater] && !flags.isSet(56),"M89 Medin quest and lake access");
        return;
    }
    if(name=="m88-roster" || name.rfind("m87-",0)==0) {
        const bool shangri=name=="m87-shangrila";
        check(camera.mapId==XeenMapIdentity(shangri?49:28) && camera.x==(shangri?14:24) && camera.y==(shangri?12:4) && camera.direction==XeenDirection::North,"Inn preset outside tavern door");
        check(party.party.size()==(name=="m87-three"?3:6) && !party.insideInn,"Inn preset active count");
        if(name=="m87-three")check(xeenInnList(party,28).size()==6,"three dismissed members remain listed in Vertigo");
        if(name=="m88-roster") {
            check(xeenInnList(party,28).size()==22,"nearly full Inn roster");
            for(unsigned id=0;id<24;++id)check(party.roster.at(id).name.empty()==(id==12 || id==21),"only Gnome male and Human female slots free");
        }
        return;
    }
    if(name.rfind("m86-playtest-",0)==0) {
        XeenConsequenceCharacters characters;for(unsigned i=0;i<6;++i)characters[i]=party.roster.at(party.party.activeOrder()[i]);
        const auto terrain=world.prepareTerrainArrival(camera,characters,*party.encounterContext);
        if(name=="m86-playtest-fall") {
            check(camera.mapId==26 && camera.x==5 && camera.y==1 && terrain && terrain.falling && terrain.surface==13 &&
                terrain.transition.destination.mapId==12,"M86 actual Clouds fall terrain/destination");
            for(const auto &c:characters)check(c.currentHp>100,"M86 fall must survive");
        } else if(name=="m86-playtest-moon-ray") {
            const auto &caster=party.roster.at(party.party.activeOrder()[1]);
            const auto slot=*XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Clerical,48);
            check(caster.learnedSpells && caster.learnedSpells->at(slot) && caster.currentSpell==slot &&
                caster.currentSp>=XeenLearnedSpellRules::cost(48,caster.currentLevel()).sp,"M86 Moon Ray caster/book/SP");
            const auto &orc=world.sessionState().regionalActors(23).at(5);
            check(camera.mapId==23 && camera.x==9 && camera.y==11 && camera.direction==XeenDirection::West &&
                orc.x==8 && orc.y==11 && orc.lifecycle==XeenActorLifecycle::Present,"M86 Moon Ray monster in reach");
        } else {
            check(terrain && !terrain.falling && party.encounterContext->effects[XeenPartyEffect::Levitate] &&
                xeenPartyHasSkill(party,16) && xeenPartyHasSkill(party,17),"M86 border abilities/terrain");
            check(xeenRegionalService(inputs.events.load(camera.mapId),camera).has_value(),"M86 border preset is not next to a service");
            check(XeenActorApproach::dangerSense(world.sessionState().regionalActors(camera.mapId),camera),"M86 Danger Sense has no nearby monster");
        }
        return;
    }
    if(name.rfind("m85-playtest-",0)==0) {
        const bool trap=name=="m85-playtest-trap";
        const XeenCamera expected=trap?XeenCamera{55,7,8,XeenDirection::East}:XeenCamera{28,13,4,XeenDirection::East};
        check(save_test::sameCamera(camera,expected),"M85 preset location");
        const auto original=XeenActorApproach::actorsFromResources(inputs.maps.loadObjects(inputs.assets,camera.mapId),inputs.statistics);
        const auto slot=trap?8u:35u;const auto &actor=world.sessionState().regionalActors(camera.mapId).at(slot);
        check(actor.x==original.at(slot).x && actor.y==original.at(slot).y && actor.lifecycle==XeenActorLifecycle::Present,"M85 original reachable monster position");
        const auto cell=world.sampleCell(camera.mapId,camera.x,camera.y);
        check(cell && (cell->cell->rawAttributes&kXeenAutomaticEventFlag),"M85 flagged witness");
        std::array<XeenCharacter,6> characters;
        const auto owners=party.party.activeOrder();for(unsigned n=0;n<6;++n)characters[n]=party.roster.at(owners[n]);
        const auto terrain=world.prepareTerrainArrival(camera,characters,*party.encounterContext);
        check(terrain && !terrain.falling,"M85 witness terrain cannot make the party fall");
        // The original-data dispatch test walks these exact monsters to each party
        // and checks the party remains on the map (map 26 would fall).
        for(auto owner:party.party.activeRosterIds())check(party.roster.at(owner).currentHp==1000 && party.roster.at(owner).canAct(),"M85 healthy party");
        return;
    }
    if(name.rfind("m84-playtest-",0)==0) {
        const XeenCamera expected=name=="m84-playtest-trap"?XeenCamera{55,7,8,XeenDirection::North}:
            name=="m84-playtest-label"?XeenCamera{28,13,4,XeenDirection::West}:XeenCamera{25,27,3,XeenDirection::North};
        check(save_test::sameCamera(camera,expected),"M84 preset location");
        const auto cell=world.sampleCell(camera.mapId,camera.x,camera.y);
        check(cell && (cell->cell->rawAttributes&kXeenAutomaticEventFlag),"M84 preset must start on a flagged cell");
        for(auto owner:party.party.activeRosterIds())check(party.roster.at(owner).currentHp==1000 && party.roster.at(owner).canAct(),"M84 healthy party");
        for(const auto &state:captured.maps)if(state.monsters)for(const auto &actor:state.monsters->slots)
            check(actor.lifecycle==XeenActorLifecycle::Defeated && actor.accounted,"M84 quiet monster state");
        if(name=="m84-playtest-save") {
            const auto records=inputs.events.load(camera.mapId).records;
            check(std::none_of(records.begin(),records.end(),[&](const auto &r){return r.x==camera.x && r.y==camera.y;}),"M84 record-less save cell");
        }
        return;
    }
    check((name=="monster-control" || name=="monster-damage" || name=="party-healing" || name=="m76-spells" || name=="m79-playtest" || name=="m80-playtest" || name=="m80-arena" || name=="m80-arena-maxed" || name=="m81-drops" || name=="m81-fullpacks" || name=="m81-event") || save_test::sameCamera(camera,xeenJourneyContent().entry),"fresh Journey camera changed");
    const auto first=party.party.activeRosterIds()[0],second=party.party.activeRosterIds()[1];
    const auto &member=party.roster.at(first);
    const auto gold=party.purse->gold;
    const auto context=*party.encounterContext;
    if(name=="m81-drops" || name=="m81-fullpacks" || name=="m81-event") {
        const auto map=camera.mapId;
        const auto originals=XeenActorApproach::actorsFromResources(inputs.maps.loadObjects(inputs.assets,map),inputs.statistics);
        const auto &slots=saved.map(map).monsters->slots;
        bool shooter=false;
        for(auto owner:party.party.activeRosterIds()) {
            const auto &c=party.roster.at(owner);
            check(c.permanentLevel==30 && c.canAct() && (!c.learnedSpells || c.currentSp==1000),"M81 party strength/SP");
            for(const auto &w:c.weapons)shooter=shooter || w.frame==4;
        }
        check(shooter && xeenPlayerRayRows(world,camera)>=3,"M81 preset cannot Shoot ahead");
        if(name=="m81-event") {
            check(camera.mapId==23 && camera.x==8 && camera.y==2 && camera.direction==XeenDirection::North,"M81 Event preset position");
            for(std::size_t slot=0;slot<slots.size();++slot) {
                const auto &a=slots[slot];const bool kept=slot==0 || slot==7;
                check((a.lifecycle==XeenActorLifecycle::Present)==kept && a.accounted!=kept,"M81 Event preset monsters");
                if(kept)check(originals[slot].original.resourceId==6 && a.x==(slot?6:8) && a.y==(slot?5:4),"M81 Event preset Orc cells");
            }
            // The first Shoot after loading kills Orc 0 with an item; Orc 7 in view
            // keeps ordinary delivery deferred for the Root Event.
            training_test::Fixture f(inputs,saved);const auto random=f.w.sessionState().journeyRandom();
            for(unsigned n=0;n<20;++n)pulse(f);
            check(f.flow->canSave() && f.w.sessionState().journeyRandom()==random,"M81 Event preset idles with a draw");
            f.act(ShootAction{});
            for(unsigned n=0;n<2000 && !f.flow->canSave() && !XeenPartyInterfaceTestAccess::reward(*f.flow);++n)pulse(f);
            const auto &staging=f.w.treasureStaging().value();
            check(f.w.sessionState().monsters(23)[0].lifecycle==XeenActorLifecycle::Defeated && staging.ready && staging.gold==10 &&
                staging.storedItems() && !XeenPartyInterfaceTestAccess::reward(*f.flow) && f.flow->canSave(),"M81 Event preset Shoot is not a deferred item kill");
            return;
        }
        check(camera.mapId==3 && camera.direction==XeenDirection::West,"M81 drop preset position");
        std::vector<unsigned> group;
        for(std::size_t slot=0;slot<slots.size();++slot)if(slots[slot].x==camera.x-2 && slots[slot].y==camera.y && slots[slot].lifecycle==XeenActorLifecycle::Present)
            group.push_back(originals[slot].original.resourceId);
        std::sort(group.begin(),group.end());
        check(group==std::vector<unsigned>{24,45,45},"M81 drop preset Barbarians and Ninja two cells ahead");
        unsigned free=0;
        for(unsigned n=0;n<6;++n) {
            auto c=party.roster.at(party.party.activeRosterIds()[n]);
            for(unsigned category=0;category<4;++category) {
                const auto &items=xeenRewardItems(c,XeenRewardCategory(category));
                const auto empty=std::count_if(items.begin(),items.end(),[&](const XeenItem &v){return XeenTreasureStaging::emptySlot(v,XeenRewardCategory(category));});
                if(name=="m81-drops")check(empty>=3,"M81 drop preset packs lack room");
                free+=unsigned(empty);
            }
            if(name=="m81-fullpacks")check(!c.weapons.back().id==(n==1),"M81 full-pack free slot");
        }
        if(name=="m81-fullpacks")check(free==1 && !xeenPacksGloballyFull(party),"M81 full-pack preset must leave exactly one slot");
        return;
    }
    if(name=="m80-arena" || name=="m80-arena-maxed") {
        check(camera.mapId==6 && camera.x==13 && camera.y==4 && camera.direction==XeenDirection::West,"Arena preset position");
        check(!world.sessionState().hasRegionalActors(85),"Arena preset contains Warzone state");
        for(auto owner:party.party.activeRosterIds()) {
            const auto &c=party.roster.at(owner);
            check(c.awards[9]==(name=="m80-arena-maxed" && owner==first?98:0),"Arena preset award");
            check(c.permanentLevel==30 && c.canAct() && c.currentHp>0,"Arena preset party strength");
        }
        return;
    }
    if(name=="m80-playtest") {
        check(camera.mapId==24 && camera.x==10 && camera.y==9 && camera.direction==XeenDirection::North,"M80 entrance position");
        for(unsigned item=1;item<=7;++item)check(party.questItems.at(item)==1,"M80 Clouds key missing");
        for(unsigned item=8;item<XeenQuestItems::kCount;++item)check(party.questItems.at(item)==0,"M80 extra quest item");
        bool travel=false;
        for(auto owner:party.party.activeRosterIds()) {
            const auto &c=party.roster.at(owner);
            const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
            bool spells=category.has_value();
            for(const auto spell:{XeenLearnedSpell::Jump,XeenLearnedSpell::LloydsBeacon}) {
                const auto slot=category?XeenLearnedSpellRules::slotForSpell(*category,static_cast<unsigned>(spell)):std::nullopt;
                spells=spells && slot && XeenLearnedSpellRules::known(c,*slot);
            }
            bool portal=false;
            for(const auto &item:c.miscellaneous)portal=portal || (item.material && (item.state&63)==63 &&
                xeenItemUseSpell(item.id)==XeenLearnedSpell::TownPortal);
            travel=travel || (spells && portal && c.canAct() && c.currentSp>=10000);
        }
        check(travel && party.purse->gems>=5000,"M80 travel caster/SP/gems missing");
        return;
    }
    if(name=="m79-playtest") {
        check(camera.mapId==30 && camera.x==12 && camera.y==12,"M79 Rivercity position");
        check(gold==100000 && party.purse->gems==5000 && party.food==1,"M79 purse/food");
        for(unsigned n=0;n<6;++n) {
            const auto owner=party.party.activeRosterIds()[n];const auto &c=party.roster.at(owner);
            check(c.permanentLevel==std::array<unsigned,3>{9,14,19}[n%3] && party.roster.combatInputs(owner)->experience==1000000000,"M79 Training readiness");
            check(c.currentHp>0 && c.currentHp<XeenCharacterRules::maxHp(c,{context.year}),"M79 injuries");
            for(unsigned award=0;award<6;++award)check(bool(c.awards[award])==(n%2==0),"M79 mixed Guild membership");
            if(c.learnedSpells)check(std::count(c.learnedSpells->begin(),c.learnedSpells->end(),0)>0,"M79 partial books");
            if(n>=4) {
                check(XeenLearnedSpellRules::known(c,*XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Wizardry,71)) &&
                    XeenLearnedSpellRules::known(c,*XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Wizardry,44)),"M79 travel spells");
                check(c.lloydMap==(n==4?49:73) && c.lloydSide==0 && c.lloydX==(n==4?14:11) && c.lloydY==(n==4?8:7),"M79 service beacons");
                check(bool(world.sampleCell(XeenMapIdentity(c.lloydMap),c.lloydX,c.lloydY)),"M79 beacon cell absent");
            }
        }
        check(member.weapons[0].state==0x40 && party.roster.at(second).armor[0].state==0x80,"M79 cursed/broken items");
        return;
    }
    if(name=="m78-playtest" || name=="m78-one-holder") {
        const bool one=name=="m78-one-holder";
        check(gold==100000 && party.purse->gems==5000,"M78 purse");
        for(unsigned n=0;n<6;++n) {
            const auto &c=party.roster.at(party.party.activeRosterIds()[n]);
            check(c.canAct() && c.permanentLevel==30 && c.currentHp==XeenCharacterRules::maxHp(c,{context.year}),"M78 level-30 party");
            check(c.weapons[0].material==58 && c.weapons[0].frame && c.armor[0].material==58 && c.armor[0].frame,"M78 Obsidian gear equipped");
            check(c.skills[14]==1,"M78 Swimmer");
            const bool holder=n==0 || (n==1 && !one);
            check(c.skills[9]==holder && c.skills[11]==holder,"M78 Mountaineer/Pathfinder holders");
        }
        return;
    }
    if (name=="broken-armor") {
        bool found=false;
        for (auto owner:party.party.activeRosterIds()) for (const auto &item:party.roster.at(owner).armor)
            if (item.id && item.frame && (item.state&0x80)) {
                found=true;
                check(xeenPrepareArmorRepair(item,gold).outcome==XeenArmorRepairOutcome::Repaired,"broken armor cannot be fixed");
            }
        check(found,"no equipped broken armor");
    } else if (name=="train-ready") {
        check(xeenQuoteTraining(member,*party.roster.combatInputs(first),gold,context,XeenMapIdentity(28)).outcome==XeenTrainingOutcome::Quoted,
            "member lacks XP/gold to train");
        check(xeenPrepareTraining(party,first,context,XeenMapIdentity(28)).result.outcome==XeenTrainingOutcome::Trained,"training candidate refused");
    } else if (name=="injured-dead") {
        const auto &dead=party.roster.at(second);
        check(member.currentHp==0 && member.worstCondition()==XeenCondition::Unconscious,"first member is not unconscious");
        check(dead.currentHp<=-XeenCharacterRules::maxHp(dead,{context.year}) && dead.worstCondition()==XeenCondition::Dead,
            "second member is not Dead");
        check(xeenPrepareTempleHeal(party,first,context,XeenMapIdentity(28)).result.outcome==XeenTempleHealOutcome::Healed,"Temple refuses unconscious member");
        check(xeenPrepareTempleHeal(party,second,context,XeenMapIdentity(28)).result.outcome==XeenTempleHealOutcome::Healed,"Temple refuses Dead member");
        unsigned survivors=0;
        for (auto owner:party.party.activeRosterIds()) survivors+=party.roster.at(owner).canAct();
        check(survivors==4,"injury preset lacks four survivors");
    } else if (name=="poisoned") {
        check(member.conditions[static_cast<unsigned>(XeenCondition::Poisoned)]==1,"member is not Poisoned");
        bool found=false;
        for (auto owner:party.party.activeRosterIds()) if (party.roster.at(owner).canAct())
            for (const auto &item:party.roster.at(owner).miscellaneous) found=found || XeenAntidoteUse::eligible(item);
        check(found,"no usable Misc antidote");
    }
    if(name=="combat-items") {
        check(camera.mapId==23 && camera.x==9 && camera.y==11,"Combat item preset mainland position");
        for(auto owner:party.party.activeRosterIds()) {
            const auto &c=party.roster.at(owner);
            check(c.canAct() && c.weapons[0].id && c.armor[0].id && c.miscellaneous[0].id==3,"Combat item preset lacks usable gear/damage power");
        }
        check(party.questItems.at(0)==1 && party.questItems.at(17)==3 && party.questItems.at(26)==1 &&
            party.questFlags.isSet(2) && party.questFlags.isSet(3) && party.questFlags.isSet(29) &&
            party.worldFlags->values[1] && party.worldFlags->values[72],"Combat preset journal sources");
        check(member.awards[9]==23 && member.awards[17] && member.awards[126] && member.awards[127] &&
            xeenAwardLines(inputs.assets.uiText(),member,xeenJournalStrings(inputs.assets.readArchiveResource("award.bin",XeenSceneArchive::DarksideOnly),true)).size()>9,
            "Combat preset needs scrollable awards");
        check(member.weapons[2].state==0x40 && member.weapons[3].id==34,"Combat preset lacks curse/Slayer Sword");
        check(member.miscellaneous[1].id==9 && member.miscellaneous[2].id==55,"Combat preset lacks healing/Portal powers");
        const auto &full=party.roster.at(party.party.activeRosterIds()[5]);
        check(full.weapons[8].id && full.armor[8].id && full.accessories[8].id && full.miscellaneous[8].material,"Combat preset pack not full");
        check(xeenPartyHasSkill(party,6) && xeenPartyHasSkill(party,16) && xeenPartyHasSkill(party,17),"Combat preset lacks indicator skills");
        check(XeenActorApproach::dangerSense(world.sessionState().regionalActors(23),camera),"Combat preset has no monster within Danger Sense range");
        return;
    }
    if(name=="m76-spells") {
        check(camera.mapId==28 && camera.x==18 && camera.y==4,"M76 preset position");
        check(party.purse->gems==1000,"M76 preset gems");
        for(unsigned id:{3u,30u,31u,53u,55u,12u,13u,42u,41u,74u,4u,15u,34u,19u,57u,39u,66u,21u,70u,69u,44u,71u}) {
            bool found=false;
            for(auto owner:{first,second}) {
                const auto &caster=party.roster.at(owner);const auto category=*XeenLearnedSpellRules::categoryForClass(caster.characterClass);
                const auto slot=XeenLearnedSpellRules::slotForSpell(category,id);
                found=found || (slot && XeenLearnedSpellRules::known(caster,*slot) && caster.currentSp==1000 && caster.currentLevel()==7);
            }
            check(found,"M76 preset book knowledge/current level/SP");
        }
        for(unsigned power:{22u,30u,25u,29u,14u,49u,50u,1u,20u,32u,18u,34u,23u,63u,53u}) {
            bool found=false;for(auto owner:{first,second})for(const auto &item:party.roster.at(owner).miscellaneous)found=found || (item.id==power && (item.state&63)==5);
            check(found,"M76 preset charged power");
        }
        const auto &targets=party.roster.at(party.party.activeRosterIds()[2]);
        for(unsigned i=0;i<6;++i)check(targets.miscellaneous[i].id==std::array<unsigned,6>{48,58,39,42,28,55}[i] &&
            targets.miscellaneous[i].state==5,"M76 travel charged powers");
        check(targets.weapons[0].id && !targets.weapons[0].material && !targets.weapons[0].state &&
            targets.armor[0].id && !targets.armor[0].material && !targets.armor[0].state,"M76 plain enchant targets");
        check(targets.weapons[1].material && targets.weapons[2].state==0x80 && targets.weapons[3].state==0x40 && targets.weapons[4].id==34,"M76 refusal and ID-34 targets");
        const auto &misc=party.roster.at(second).miscellaneous;
        for(unsigned i=0;i<3;++i) {
            const auto &c=party.roster.at(party.party.activeRosterIds()[i]);
            check(c.lloydMap==(i==0?28:i==1?23:29) && c.lloydSide==0 && c.lloydX==(i==1?9:18) && c.lloydY==(i==1?11:4),"M76 preset distinct character beacons");
        }
        check(misc[6].state==2 && misc[7].state==0x42 && misc[8].state==0x82 && misc[5].id==53,"M76 ordinary/cursed/broken Recharge and excluded power-53 targets");
        const auto &nearby=saved.map(28).monsters->slots.at(0);
        check(nearby.x==17 && nearby.y==4 && nearby.lifecycle==XeenActorLifecycle::Present,"M76 nearby monster");
        return;
    }
    if(name=="party-healing") {
        check(camera.mapId==28 && camera.x==18 && camera.y==4,"Healing preset position");
        check(party.roster.at(first).characterClass==XeenCharacterClass::Cleric && party.roster.at(second).characterClass==XeenCharacterClass::Druid,"Healing preset casters");
        check(party.purse->gems==1000,"Healing preset gems");
        for(unsigned id:{6u,7u,8u,9u,16u,49u,52u,56u,58u,64u,67u,68u}) {
            const auto &caster=party.roster.at(first);const auto slot=XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Clerical,id);
            check(slot && XeenLearnedSpellRules::known(caster,*slot) && caster.currentSp==1000,"Healing preset spell knowledge/SP");
        }
        for(unsigned condition:{3u,4u,11u,14u,13u,15u}) {
            bool found=false;for(auto owner:party.party.activeRosterIds())found=found || bool(party.roster.at(owner).conditions[condition]);
            check(found,"Healing preset missing condition");
        }
        for(unsigned power:{26u,27u,37u,41u,44u,51u,56u,57u,66u,73u}) {
            bool found=false;for(auto owner:{first,second})for(const auto &item:party.roster.at(owner).miscellaneous)found=found || (item.id==power && item.state==5);
            check(found,"Healing preset missing charged item");
        }
        return;
    }
    if((name=="monster-control" || name=="monster-damage")) {
        check(camera.mapId==XeenMapIdentity(28) && camera.x==18 && camera.y==4,"Control fixture must stand in Vertigo");
        check(party.purse->gems==1000,"Control fixture lacks gems");
        unsigned covered=0;
        const std::vector<unsigned> spells=name=="monster-damage" ? std::vector<unsigned>{0u,5u,11u,14u,18u,22u,23u,24u,25u,28u,29u,32u,35u,36u,37u,43u,46u,47u,48u,51u,54u,63u,65u} : std::vector<unsigned>{2u,17u,33u,38u,61u,62u,72u,73u};
        for(unsigned id:spells) {
            bool known=false;
            for(auto owner:party.party.activeRosterIds()) {
                const auto &c=party.roster.at(owner);const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
                if(category)if(auto slot=XeenLearnedSpellRules::slotForSpell(*category,id))
                    known=known || (XeenLearnedSpellRules::known(c,*slot) && c.currentSp==1000);
            }
            covered+=known;
        }
        check(covered==spells.size(),"Monster fixture does not cover its spells");return;
    }
    // Revert only the documented preset fields and compare against a fresh Journey.
    // This also checks actor/world/economy/time/RNG preservation, not just symptoms.
    auto baseline=inputs.base();
    baseline.gameFlags=XeenGameFlagsLoader().loadInitialFlags(inputs.assets).values();
    auto unmodified=captured;
    if (name=="broken-armor") {
        for (auto owner:unmodified.activeRosterIds) unmodified.characters[owner].armor=baseline.characters[owner].armor;
    } else if (name=="train-ready") {
        unmodified.journey->supplements[first]=baseline.journey->supplements[first];
        unmodified.journey->purse->gold=baseline.journey->purse->gold;
    } else if (name=="injured-dead") {
        unmodified.characters[first]=baseline.characters[first];unmodified.characters[second]=baseline.characters[second];
    } else {
        unmodified.characters[first].conditions=baseline.characters[first].conditions;
        for (auto owner:unmodified.activeRosterIds) unmodified.characters[owner].miscellaneous=baseline.characters[owner].miscellaneous;
    }
    save_test::sameSnapshot(unmodified,baseline);
}
}

int main(int argc,char **argv) {
    try {
        check(argc==4 || argc==5 || (argc==6 && std::string(argv[4])=="--export"),"usage: test <installation> <preset|paths> <repository> [existing-save | --export build-save]");
        const auto installation=xeenTestInstallationDetector().detect(fs::u8path(argv[1]));
        check(bool(installation),"original installation missing");
        TemporaryDirectory directory;
        const std::string name=argv[2];
        if (name=="paths") paths(*installation,fs::u8path(argv[3]),directory.path);
        else preset(*installation,name,directory.path,argc==5?fs::u8path(argv[4]):fs::path{});
        // The developer generator retains its repository guard. Export only the
        // freshly generated and independently validated test fixture for CTest.
        if(argc==6)fs::copy_file(directory.path/(name+".mmsave"),fs::u8path(argv[5]),fs::copy_options::overwrite_existing);
        std::cout<<"Test-save "<<name<<" passed\n";return 0;
    } catch (const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
