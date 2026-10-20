#include "tools/XeenTestSave.h"
#include "app/XeenEncounterFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenItemTransfer.h"
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
        XeenTemplePriceState templePrices;
        XeenEncounterFlow flow(templePrices,world,party,camera,flags,[]{return 0;},setup);
        present(flow);
        return XeenSaveState::capture(resources.signature,party,camera,flags,world);
    }
    XeenSaveSnapshot validated(const XeenSaveSnapshot &candidate) {
        XeenWorld world(mapLoader(),objectLoader());
        XeenPartyState party; XeenCamera camera; XeenGameFlags flags;
        XeenSaveState::restoreBeforeGameplay(candidate,resources,party,camera,flags,world,
            [this](auto &w,const auto &p,const auto &c,const auto &) {preflight(w,p,c);});
        XeenTemplePriceState templePrices;
        XeenEncounterFlow flow(templePrices,world,party,camera,flags,[]{return 0;},XeenJourneyRestoreTag{});
        present(flow);
        return XeenSaveState::capture(resources.signature,party,camera,flags,world);
    }
};

void prepare(XeenSaveSnapshot &snapshot,const std::string &preset) {
    auto &journey=*snapshot.journey;
    const auto first=snapshot.activeRosterIds.at(0),second=snapshot.activeRosterIds.at(1);
    auto &member=snapshot.characters[first];
    if(preset=="m79-playtest") {
        snapshot.camera={30,12,12,XeenDirection::North};snapshot.food=1;
        journey.treasure->gold=100000;journey.treasure->gems=5000;
        for(unsigned n=0;n<snapshot.activeRosterIds.size();++n) {
            const auto owner=snapshot.activeRosterIds[n];auto &c=snapshot.characters[owner];
            c.conditions.fill(0);c.permanentLevel=std::array<unsigned,3>{9,14,19}[n%3];c.temporaryLevel=0;
            journey.supplements[owner].inputs.experience=1000000000;
            c.awards.fill(0);if(n%2==0)for(unsigned award=0;award<6;++award)c.awards[award]=1;
            if(n>=4)c.characterClass=XeenCharacterClass::Sorcerer;
            if(const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass)) {
                c.hasSpells=true;c.learnedSpells.emplace();c.learnedSpells->fill(0);
                for(unsigned id:{1u,26u,42u,44u,71u})if(const auto slot=XeenLearnedSpellRules::slotForSpell(*category,id))c.learnedSpells->at(*slot)=1;
                c.currentSpell=*XeenLearnedSpellRules::slotForSpell(*category,n>=4?44:26);
            }
            c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};
            c.weapons[0]={0,1,0,0};c.armor[0]={0,1,0,0};
            if(n==0)c.weapons[0].state=0x40;
            if(n==1)c.armor[0].state=0x80;
            c.currentHp=std::max(1,XeenCharacterRules::maxHp(c,{journey.context->year})/2);
            c.currentSp=XeenCharacterRules::maxSp(c,{journey.context->year});
            if(n>=4) {
                c.lloydMap=n==4?49:73;c.lloydX=n==4?14:11;c.lloydY=n==4?8:7;c.lloydSide=0;
            }
        }
        return;
    }
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
    if(preset=="m78-playtest" || preset=="m78-one-holder") {
        // The original six at level 30, Obsidian gear equipped through the shared
        // equipment rule, every slot of each caster's list and the outdoor skills.
        // The one-holder variant keeps Mountaineer/Pathfinder on the first member.
        journey.treasure->gold=100000;journey.treasure->gems=5000;
        const bool one=preset=="m78-one-holder";
        const XeenCharacterRulesContext rules{std::uint32_t(journey.context->year)};
        XeenPartyState party;
        for(auto owner:snapshot.activeRosterIds)party.roster.at(owner)=snapshot.characters[owner];
        party.party=XeenParty::fromRosterIds(snapshot.activeRosterIds);
        using C=XeenInventoryCategory;
        const auto equip=[&](unsigned n,C category,std::initializer_list<unsigned> ids,std::uint8_t material) {
            auto &items=*xeenInventoryItems(party.roster.at(snapshot.activeRosterIds[n]),category);
            const auto slot=std::find_if(items.begin(),items.end(),[](const XeenItem &item) {return !item.id;});
            if(slot==items.end())throw std::logic_error("Play-test inventory is full");
            for(auto id:ids) {
                *slot={material,static_cast<std::uint8_t>(id),0,0};
                if(xeenSetEquipment(party,n,category,std::size_t(slot-items.begin()),XeenEquipmentOperation::Equip).status==XeenEquipmentStatus::Success)return;
            }
            *slot={};
        };
        for(unsigned n=0;n<snapshot.activeRosterIds.size();++n) {
            auto &c=party.roster.at(snapshot.activeRosterIds[n]);
            c.conditions.fill(0);c.permanentLevel=30;c.temporaryLevel=0;
            c.weapons={};c.armor={};c.accessories={};
            equip(n,C::Weapons,{1,13,12,7,24},58);equip(n,C::Weapons,{31,32,33},58);
            equip(n,C::Armor,{7,6,5,4,3,2,1},58);
            for(unsigned id:{8u,9u,10u,11u,13u})equip(n,C::Armor,{id},58);
            for(unsigned id:{1u,1u,10u,2u})equip(n,C::Accessories,{id},0);
            if(XeenLearnedSpellRules::categoryForClass(c.characterClass)) {
                c.hasSpells=true;c.learnedSpells.emplace();c.learnedSpells->fill(1);
            }
            c.skills[14]=1; // Swimmer
            // Exact Mountaineer/Pathfinder holders; Badger's original Pathfinder is cleared.
            const std::uint8_t holder=n==0 || (n==1 && !one);c.skills[9]=holder;c.skills[11]=holder;
            if(n==2)for(unsigned skill:{4u,6u,7u,8u,10u,15u,16u,17u})c.skills[skill]=1;
            c.currentHp=XeenCharacterRules::maxHp(c,rules);c.currentSp=XeenCharacterRules::maxSp(c,rules);
            snapshot.characters[snapshot.activeRosterIds[n]]=c;
        }
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
        const auto quote=xeenQuoteTraining(member,inputs,journey.treasure->gold,*journey.context,snapshot.camera.mapId);
        const auto xp=std::uint64_t(inputs.experience)+quote.missing;
        if (member.permanentLevel>=10 || xp>std::numeric_limits<std::uint32_t>::max())
            throw std::runtime_error("Fresh member cannot represent trainable XP");
        inputs.experience=static_cast<std::uint32_t>(xp);
        journey.treasure->gold=std::max<std::uint32_t>(journey.treasure->gold,quote.cost);
        if (xeenQuoteTraining(member,inputs,journey.treasure->gold,*journey.context,snapshot.camera.mapId).outcome!=XeenTrainingOutcome::Quoted)
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
    if (preset!="broken-armor" && preset!="train-ready" && preset!="injured-dead" && preset!="poisoned" && preset!="monster-control" && preset!="monster-damage" && preset!="party-healing" && preset!="m76-spells" && preset!="combat-items" && preset!="m78-playtest" && preset!="m78-one-holder" && preset!="m79-playtest")
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
    if(preset=="m79-playtest") {
        const auto actors=XeenActorApproach::actorsFromResources(inputs.maps.loadObjects(inputs.assets,30),inputs.resources.loadMonsterStatistics());
        auto &table=candidate.map(30).monsters;
        table.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
        for(const auto &actor:actors)table->slots.push_back({actor.x,actor.y,actor.lifecycle,actor.status,false,-1});
    }
    // Publish only a snapshot recaptured from restored, validated current owners.
    const auto snapshot=inputs.validated(candidate);
    XeenSaveFile::write(target,snapshot);
}
}
