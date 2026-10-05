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
        resources.loadInitialRegionalRecovery=[this] {return XeenQuestFlagFormat::parseRegionalRecovery(assets.readInitialResource("maze.pty"));};
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
        setup.regionalRecovery=resources.loadInitialRegionalRecovery();
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
    if (preset!="broken-armor" && preset!="train-ready" && preset!="injured-dead" && preset!="poisoned")
        throw std::invalid_argument("Unknown test-save preset: "+preset);
    const auto target=XeenSaveFile::resolve(output,installation.root);
    // Reuse the native alias/reparse-aware path guard for the repository too.
    try { (void)XeenSaveFile::resolve(target,std::filesystem::u8path(MMODERN_TEST_SAVE_SOURCE_DIR)); }
    catch (const std::exception &e) {throw std::runtime_error(std::string("Test saves must be outside the repository: ")+e.what());}
    if (std::filesystem::exists(target)) throw std::runtime_error("Test-save target already exists; choose a new path");
    Inputs inputs(installation);
    auto candidate=inputs.fresh();
    prepare(candidate,preset);
    // Publish only a snapshot recaptured from restored, validated current owners.
    const auto snapshot=inputs.validated(candidate);
    XeenSaveFile::write(target,snapshot);
}
}
