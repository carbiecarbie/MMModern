#include "XeenTestInstallation.h"
#include "tools/XeenTestSave.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenSkills.h"
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

namespace {
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

void preset(const GameInstallation &installation,const std::string &name,const fs::path &directory) {
    const auto output=directory/(name+".mmsave");
    developer::generateTestSave(installation,name,output);
    const auto saved=XeenSaveFile::read(XeenSaveFile::resolve(output,installation.root));
    // Independent original-data providers, followed by the normal startup restore.
    training_test::Inputs inputs(installation);
    XeenPartyState party;XeenCamera camera;XeenGameFlags flags;
    XeenWorld world(inputs.mapLoader(),inputs.objectLoader());
    XeenSaveState::restoreBeforeGameplay(saved,inputs.resources(),party,camera,flags,world,
        [&](auto &w,const auto &p,const auto &c,const auto &) {
            (void)CloudsMapComposer().compose(inputs.assets,w,p,c,{p.encounterContext->year});
        });
    XeenEncounterFlow flow(world,party,camera,flags,[]{return 0;},XeenJourneyRestoreTag{});
    check(flow.prepareJourneyFrame(flow.ticket(),[]{}) && flow.presentJourney(flow.ticket()),"restored preset is not quiet");
    const auto captured=XeenSaveState::capture(inputs.signature,party,camera,flags,world);
    check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(captured),"restore replayed or changed state");
    check((name=="monster-control" || name=="monster-damage" || name=="party-healing" || name=="m76-spells" || name=="m79-playtest") || save_test::sameCamera(camera,xeenJourneyContent().entry),"fresh Journey camera changed");
    const auto first=party.party.activeRosterIds()[0],second=party.party.activeRosterIds()[1];
    const auto &member=party.roster.at(first);
    const auto gold=party.monsterTreasure->gold;
    const auto context=*party.encounterContext;
    if(name=="m79-playtest") {
        check(camera.mapId==30 && camera.x==12 && camera.y==12,"M79 Rivercity position");
        check(gold==100000 && party.monsterTreasure->gems==5000 && party.food==1,"M79 purse/food");
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
    if (name=="broken-armor") {
        bool found=false;
        for (auto owner:party.party.activeRosterIds()) for (const auto &item:party.roster.at(owner).armor)
            if (item.id && item.frame && (item.state&0x80)) {
                found=true;
                check(xeenPrepareArmorRepair(item,gold).outcome==XeenArmorRepairOutcome::Repaired,"broken armor cannot be fixed");
            }
        check(found,"no equipped broken armor");
    } else if (name=="train-ready") {
        check(xeenQuoteTraining(member,*party.roster.combatInputs(first),gold,context).outcome==XeenTrainingOutcome::Quoted,
            "member lacks XP/gold to train");
        check(xeenPrepareTraining(party,first,context).result.outcome==XeenTrainingOutcome::Trained,"training candidate refused");
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
        check(party.monsterTreasure->gems==1000,"M76 preset gems");
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
        check(party.monsterTreasure->gems==1000,"Healing preset gems");
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
        check(party.monsterTreasure->gems==1000,"Control fixture lacks gems");
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
    baseline.gameFlags=XeenGameFlagsLoader().loadInitialCloudsFlags(inputs.assets).values();
    auto unmodified=captured;
    if (name=="broken-armor") {
        for (auto owner:unmodified.activeRosterIds) unmodified.characters[owner].armor=baseline.characters[owner].armor;
    } else if (name=="train-ready") {
        unmodified.journey->supplements[first]=baseline.journey->supplements[first];
        unmodified.journey->treasure->gold=baseline.journey->treasure->gold;
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
        check(argc==4,"usage: test <installation> <preset|paths> <repository>");
        const auto installation=xeenTestInstallationDetector().detect(fs::u8path(argv[1]));
        check(bool(installation),"original installation missing");
        TemporaryDirectory directory;
        const std::string name=argv[2];
        if (name=="paths") paths(*installation,fs::u8path(argv[3]),directory.path);
        else preset(*installation,name,directory.path);
        std::cout<<"Test-save "<<name<<" passed\n";return 0;
    } catch (const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
