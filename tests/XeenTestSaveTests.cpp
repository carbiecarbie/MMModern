#include "XeenTestInstallation.h"
#include "tools/XeenTestSave.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/CloudsMapComposer.h"
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
    check((name=="monster-control" || name=="monster-damage") || save_test::sameCamera(camera,xeenJourneyContent().entry),"fresh Journey camera changed");
    const auto first=party.party.activeRosterIds()[0],second=party.party.activeRosterIds()[1];
    const auto &member=party.roster.at(first);
    const auto gold=party.monsterTreasure->gold;
    const auto context=*party.encounterContext;
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
    if((name=="monster-control" || name=="monster-damage")) {
        check(camera.mapId==XeenMapIdentity(28) && camera.x==18 && camera.y==4,"Control fixture must stand in Vertigo");
        check(party.monsterTreasure->gems==1000,"Control fixture lacks gems");
        unsigned covered=0;
        const std::vector<unsigned> spells=name=="monster-damage" ? std::vector<unsigned>{0u,5u,11u,14u,18u,22u,23u,24u,25u,28u,29u,32u,35u,36u,37u,43u,46u,47u,48u,54u,63u,65u} : std::vector<unsigned>{2u,17u,33u,38u,61u,62u,72u,73u};
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
