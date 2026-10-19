#include "tools/XeenTestSave.h"
#include "app/XeenEncounterFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenItemRewards.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenSaveState.h"
#include "platform/XeenSaveFile.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace mmodern::developer {
namespace {
// The same original-resource providers used by Application's Regional Journey.
// These are local tool inputs; no gameplay owner or new initialization rules.
struct Inputs {
    XeenAssetSource assets;
    XeenMapLoader maps;
    XeenEventLoader events;
    XeenEventTextLoader texts;
    XeenSaveState::Resources resources;

    explicit Inputs(const GameInstallation &installation) : assets(installation,320,200),
        events([this](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
            if (!assets.hasInitialResource(name)) return {};
            return assets.readInitialResource(name);
        }),
        texts([this](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
            if (!assets.hasArchiveResource(name)) return {};
            return assets.readArchiveResource(name);
        }) {
        resources.signature=XeenSaveFile::fingerprint(installation);
        resources.loadInitialParty=[this] {return XeenPartyLoader().loadInitialCloudsParty(assets);};
        resources.loadInitialCharacters=[this] {return assets.readInitialResource("maze.chr");};
        resources.loadInitialContext=[this] {return XeenGameplayContextFormat::parse(assets.readInitialResource("maze.pty"));};
        resources.loadInitialPurse=[this] {return XeenCharacterFormat::parseMonsterPurse(assets.readInitialResource("maze.pty"));};
        resources.loadInitialBankBalances=[this] {return XeenCharacterFormat::parseBankBalances(assets.readInitialResource("maze.pty"));};
        resources.loadInitialWorldFlags=[this] {return XeenQuestFlagFormat::parseWorldFlags(assets.readInitialResource("maze.pty"));};
        resources.loadEvents=[this](auto id) {return events.load(id);};
        resources.loadRegionalText=[this](auto id) {return texts.load(id);};
        resources.loadMonsterStatistics=[this] {
            const auto bytes=assets.readCloudsMonsterStatisticsFromDarkArchive();
            if (!bytes) throw std::runtime_error("Missing DARK.CC/xeen.mon");
            return XeenMonsterFormat::parse(*bytes);
        };
        resources.loadLearnedSpellNames=[this] {
            const auto bytes=assets.readLearnedSpellNamesFromDarkArchive();
            if (!bytes) throw std::runtime_error("Missing DARK.CC/spells.xen");
            return XeenLearnedSpellNames::parse(*bytes);
        };
        resources.regionalManifest=[this](const auto &map,const auto &mob,const auto &evt,const auto &mon) {
            xeenValidateRegionalManifest(map,mob,evt,mon,assets.readInitialResource("maze0023.dat"),
                assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));
        };
    }
    auto mapLoader() {return [this](auto id) {return maps.loadGeometryMap(assets,id);};}
    auto objectLoader() {return [this](auto id) {return maps.loadObjects(assets,id);};}
    void preflight(XeenWorld &world,const XeenPartyState &party,const XeenCamera &camera) {
        (void)CloudsMapComposer().compose(assets,world,party,camera,{party.encounterContext->year});
    }
    static void present(XeenEncounterFlow &flow) {
        if (!flow.prepareJourneyFrame(flow.ticket(),[]{}) || !flow.presentJourney(flow.ticket()))
            throw std::runtime_error("Test save did not reach a quiet Journey boundary");
    }
    XeenSaveSnapshot fresh() {
        auto party=resources.loadInitialParty();
        auto camera=xeenJourneyContent().entry;
        auto flags=XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
        XeenWorld world(mapLoader(),objectLoader());
        const auto characters=resources.loadInitialCharacters();
        const auto statistics=resources.loadMonsterStatistics();
        const auto mainland=resources.loadEvents(23);
        // An explicit valid Journey seed makes generated fixtures reproducible.
        XeenJourneySetup setup{characters,resources.loadInitialContext(),statistics,mainland,7,resources.regionalManifest};
        setup.purse=resources.loadInitialPurse();
        setup.bank=resources.loadInitialBankBalances();
        setup.worldFlags=resources.loadInitialWorldFlags();
        setup.regionalText=resources.loadRegionalText(23);
        setup.learnedNames=resources.loadLearnedSpellNames();
        setup.learnedNamesProvider=resources.loadLearnedSpellNames;
        setup.cityEventsProvider=[this] {return resources.loadEvents(28);};
        XeenEncounterFlow flow(world,party,camera,flags,[]{return 0;},setup);
        present(flow);
        return XeenSaveState::capture(resources.signature,party,camera,flags,world);
    }
    XeenSaveSnapshot validated(const XeenSaveSnapshot &candidate) {
        XeenWorld world(mapLoader(),objectLoader());
        XeenPartyState party; XeenCamera camera; XeenGameFlags flags;
        XeenSaveState::restoreBeforeGameplay(candidate,resources,party,camera,flags,world,
            [this](auto &w,const auto &p,const auto &c,const auto &) {preflight(w,p,c);});
        XeenEncounterFlow flow(world,party,camera,flags,[]{return 0;},XeenJourneyRestoreTag{});
        present(flow);
        return XeenSaveState::capture(resources.signature,party,camera,flags,world);
    }
};

void prepare(XeenSaveSnapshot &snapshot,const std::string &preset) {
    auto &journey=*snapshot.journey;
    const auto first=snapshot.activeRosterIds.at(0),second=snapshot.activeRosterIds.at(1);
    auto &member=snapshot.characters[first];
    if(preset=="m76-spells") {
        snapshot.camera={28,18,4,XeenDirection::West};journey.treasure->gems=1000;
        constexpr unsigned spells[]{3,30,31,53,55,12,13,42,41,74,4,15,34,19,57,39,66,21,70,69,44,71};
        constexpr unsigned powers[]{22,30,25,29,14,49,50,1,20,32,18,34,23,63,53};
        for(unsigned i=0;i<2;++i) {
            auto &caster=snapshot.characters[snapshot.activeRosterIds[i]];
            caster.characterClass=i?XeenCharacterClass::Sorcerer:XeenCharacterClass::Cleric;
            caster.hasSpells=true;caster.permanentLevel=7;caster.temporaryLevel=0;
            caster.currentSp=1000;caster.conditions.fill(0);
            caster.learnedSpells.emplace();caster.learnedSpells->fill(0);
            const auto category=*XeenLearnedSpellRules::categoryForClass(caster.characterClass);
            for(auto id:spells)if(const auto slot=XeenLearnedSpellRules::slotForSpell(category,id))caster.learnedSpells->at(*slot)=1;
            caster.currentSpell=*XeenLearnedSpellRules::slotForSpell(category,i?53:3);
            for(auto *items:{&caster.weapons,&caster.armor,&caster.accessories})for(auto &item:*items)item.frame=0;
            caster.miscellaneous={};caster.currentHp=XeenCharacterRules::maxHp(caster,{journey.context->year});
        }
        for(unsigned i=0;i<std::size(powers);++i)
            snapshot.characters[snapshot.activeRosterIds[i/9]].miscellaneous[i%9]={1,static_cast<std::uint8_t>(powers[i]),5,0};
        auto &targets=snapshot.characters[snapshot.activeRosterIds[2]];
        targets.weapons={};targets.armor={};
        targets.weapons[0]={0,1,0,0};targets.armor[0]={0,1,0,0};
        targets.weapons[1]={37,2,0,0}; // Already enchanted.
        targets.weapons[2]={0,3,0x80,0};targets.weapons[3]={0,4,0x40,0};
        targets.weapons[4]={0,34,0,0};
        constexpr unsigned travelPowers[]{48,58,39,42,28,55};
        for(unsigned i=0;i<6;++i)targets.miscellaneous[i]={1,static_cast<std::uint8_t>(travelPowers[i]),5,0};
        for(unsigned i=0;i<3;++i) {
            auto &c=snapshot.characters[snapshot.activeRosterIds[i]];
            c.lloydMap=i==0?28:i==1?23:29;c.lloydX=i==1?9:18;c.lloydY=i==1?11:4;c.lloydSide=0;
            if(i==2) {c.characterClass=XeenCharacterClass::Sorcerer;c.hasSpells=true;c.currentSp=1000;c.learnedSpells.emplace();c.learnedSpells->fill(0);
                c.currentSpell=*XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Wizardry,44);c.learnedSpells->at(c.currentSpell)=1;}
        }
        auto &misc=snapshot.characters[second].miscellaneous;
        misc[6]={1,4,2,0};misc[7]={1,4,0x42,0};misc[8]={1,4,0x82,0};
        return;
    }
    if(preset=="combat-items") {
        snapshot.camera={23,9,11,XeenDirection::West};
        for(unsigned i=0;i<6;++i) {
            auto &c=snapshot.characters[snapshot.activeRosterIds[i]];
            c.conditions.fill(0);c.permanentLevel=10;
            c.currentHp=XeenCharacterRules::maxHp(c,{journey.context->year});
            c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};
            c.weapons[0]={0,1,0,0};c.weapons[1]={0,2,0,0};
            c.weapons[2]={0,3,0x40,0};c.weapons[3]={0,34,0,0};
            c.armor[0]={0,1,0,0};c.armor[1]={0,2,0,0};
            c.accessories[0]={0,1,0,0};
            c.miscellaneous[0]={1,3,5,0};c.miscellaneous[1]={1,9,5,0};
            c.miscellaneous[2]={1,55,5,0};c.miscellaneous[3]={1,4,1,0};
            if(i==5)for(unsigned slot=0;slot<9;++slot) {
                c.weapons[slot]={0,1,0,0};c.armor[slot]={0,1,0,0};
                c.accessories[slot]={0,1,0,0};c.miscellaneous[slot]={1,3,5,0};
            }
        }
        snapshot.questItems[0]=1;snapshot.questItems[17]=3;snapshot.questItems[26]=1;
        for(unsigned flag:{2u,3u,29u})snapshot.questFlags[flag]=true;
        for(unsigned flag:{1u,2u,23u,72u})journey.worldFlags->values[flag]=true;
        member.awards.fill(0);
        for(unsigned id=0;id<20;++id)member.awards[id]=id==9?23:1;
        member.awards[XeenCharacter::awardIndex(73)]=1;member.awards[XeenCharacter::awardIndex(81)]=1;
        // The three border indicators: Direction Sense (6), Spot Secret Doors (16), Danger Sense (17).
        snapshot.characters[snapshot.activeRosterIds[1]].skills[6]=1;
        snapshot.characters[snapshot.activeRosterIds[2]].skills[16]=1;
        snapshot.characters[snapshot.activeRosterIds[3]].skills[17]=1;
        return;
    }
    if(preset=="party-healing") {
        snapshot.camera={28,18,4,XeenDirection::West};journey.treasure->gems=1000;
        constexpr unsigned spells[]{6,7,8,9,16,49,52,56,58,64,67,68};
        constexpr unsigned powers[]{26,27,37,41,44,51,56,57,66,73};
        for(unsigned i=0;i<2;++i) {
            auto &caster=snapshot.characters[snapshot.activeRosterIds[i]];
            caster.characterClass=i?XeenCharacterClass::Druid:XeenCharacterClass::Cleric;
            caster.hasSpells=true;caster.currentSp=1000;caster.conditions.fill(0);
            caster.learnedSpells.emplace();caster.learnedSpells->fill(0);
            const auto category=*XeenLearnedSpellRules::categoryForClass(caster.characterClass);
            for(auto id:spells)if(const auto slot=XeenLearnedSpellRules::slotForSpell(category,id))caster.learnedSpells->at(*slot)=1;
            caster.currentSpell=*XeenLearnedSpellRules::slotForSpell(category,49);
            for(auto *items:{&caster.weapons,&caster.armor,&caster.accessories})for(auto &item:*items)item.frame=0;
            caster.miscellaneous={};
            caster.currentHp=XeenCharacterRules::maxHp(caster,{journey.context->year});
        }
        for(unsigned i=0;i<std::size(powers);++i)
            snapshot.characters[snapshot.activeRosterIds[i/9]].miscellaneous[i%9]={1,static_cast<std::uint8_t>(powers[i]),5,0};
        // Four patients carry the six conditions; the two casters stay usable.
        for(unsigned i=2;i<6;++i) {
            auto &patient=snapshot.characters[snapshot.activeRosterIds[i]];
            patient.conditions.fill(0);patient.currentHp=i<4?1:0;
        }
        auto &poison=snapshot.characters[snapshot.activeRosterIds[2]];poison.conditions[3]=5;poison.conditions[11]=1;
        auto &stone=snapshot.characters[snapshot.activeRosterIds[3]];stone.conditions[4]=5;stone.conditions[14]=1;stone.conditions[12]=1;stone.currentHp=0;
        snapshot.characters[snapshot.activeRosterIds[4]].conditions[13]=1;
        snapshot.characters[snapshot.activeRosterIds[5]].conditions[15]=1;
        snapshot.characters[snapshot.activeRosterIds[5]].conditions[12]=1;
        return;
    }
    if ((preset=="monster-control" || preset=="monster-damage")) {
        snapshot.camera={28,18,4,XeenDirection::West};
        journey.treasure->gems=1000;
        for(auto owner:snapshot.activeRosterIds) {
            auto &caster=snapshot.characters[owner];
            const auto category=XeenLearnedSpellRules::categoryForClass(caster.characterClass);
            if(!category)continue;
            caster.hasSpells=true;caster.currentSp=1000;
            if(!caster.learnedSpells)caster.learnedSpells.emplace();
            const std::vector<unsigned> spells=preset=="monster-damage" ? std::vector<unsigned>{0u,5u,11u,14u,18u,22u,23u,24u,25u,28u,29u,32u,35u,36u,37u,43u,46u,47u,48u,51u,54u,63u,65u} : std::vector<unsigned>{2u,17u,33u,38u,61u,62u,72u,73u};
            for(unsigned id:spells)
                if(const auto slot=XeenLearnedSpellRules::slotForSpell(*category,id))caster.learnedSpells->at(*slot)=1;
        }
        return;
    }
    if (preset=="broken-armor") {
        // Physical injury can break equipped armor; Temple Heal leaves that bit set.
        for (auto owner:snapshot.activeRosterIds) for (auto &item:snapshot.characters[owner].armor)
            if (item.id && item.frame && xeenQuoteArmorRepair(XeenInventoryCategory::Armor,item,journey.treasure->gold).outcome==XeenArmorRepairOutcome::Intact) {
                item.state|=0x80;
                return;
            }
        throw std::runtime_error("Fresh Journey has no supported equipped armor to break");
    }
    if (preset=="train-ready") {
        auto &inputs=journey.supplements.at(first).inputs;
        const auto quote=xeenQuoteTraining(member,inputs,journey.treasure->gold,*journey.context);
        const auto xp=std::uint64_t(inputs.experience)+quote.missing;
        if (member.permanentLevel>=10 || xp>std::numeric_limits<std::uint32_t>::max())
            throw std::runtime_error("Fresh member cannot represent trainable XP");
        inputs.experience=static_cast<std::uint32_t>(xp);
        journey.treasure->gold=std::max<std::uint32_t>(journey.treasure->gold,quote.cost);
        if (xeenQuoteTraining(member,inputs,journey.treasure->gold,*journey.context).outcome!=XeenTrainingOutcome::Quoted)
            throw std::runtime_error("Training preset is not eligible");
        return;
    }
    if (preset=="injured-dead") {
        // Use the gameplay injury rule for HP signs, conditions and broken armor.
        xeenApplyPhysicalInjury(member,member.currentHp,journey.context->year);
        auto &dead=snapshot.characters[second];
        xeenApplyPhysicalInjury(dead,dead.currentHp+XeenCharacterRules::maxHp(dead,{journey.context->year}),journey.context->year);
        return;
    }
    if (preset=="poisoned") {
        member.conditions[static_cast<unsigned>(XeenCondition::Poisoned)]=1;
        // The one-charge Misc record delivered by Myra's original antidote reward.
        for (auto owner:snapshot.activeRosterIds)
            if (xeenInsertMiscellaneous(snapshot.characters[owner],{10,37,1,0})) return;
        throw std::runtime_error("Fresh Journey has no room for an antidote");
    }
    throw std::invalid_argument("Unknown test-save preset: "+preset);
}
}

void generateTestSave(const GameInstallation &installation,const std::string &preset,
        const std::filesystem::path &output) {
    if (preset!="broken-armor" && preset!="train-ready" && preset!="injured-dead" && preset!="poisoned" && preset!="monster-control" && preset!="monster-damage" && preset!="party-healing" && preset!="m76-spells" && preset!="combat-items")
        throw std::invalid_argument("Unknown test-save preset: "+preset);
    const auto target=XeenSaveFile::resolve(output,installation);
    // Reuse the native alias/reparse-aware path guard for the repository too.
    try { (void)XeenSaveFile::resolve(target,std::filesystem::u8path(MMODERN_TEST_SAVE_SOURCE_DIR)); }
    catch (const std::exception &e) {throw std::runtime_error(std::string("Test saves must be outside the repository: ")+e.what());}
    if (std::filesystem::exists(target)) throw std::runtime_error("Test-save target already exists; choose a new path");
    Inputs inputs(installation);
    auto candidate=inputs.fresh();
    prepare(candidate,preset);
    if((preset=="monster-control" || preset=="monster-damage" || preset=="party-healing" || preset=="m76-spells")) {
        const auto actors=XeenActorApproach::actorsFromResources(inputs.maps.loadObjects(inputs.assets,28),inputs.resources.loadMonsterStatistics());
        auto &table=candidate.map(28).monsters;
        table.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
        for(const auto &actor:actors)table->slots.push_back({actor.x,actor.y,actor.lifecycle,actor.status,false,-1});
        if(preset=="m76-spells") {
            auto &nearby=table->slots.at(0);nearby.x=17;nearby.y=4;
            nearby.lifecycle=XeenActorLifecycle::Present;nearby.accounted=false;
        }
    }
    // Publish only a snapshot recaptured from restored, validated current owners.
    const auto snapshot=inputs.validated(candidate);
    XeenSaveFile::write(target,snapshot);
}
}
