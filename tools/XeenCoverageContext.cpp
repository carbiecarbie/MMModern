#include "XeenCoverageContext.h"
#include "XeenCoverage.h"
#include "app/XeenEventFlow.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenMapFormat.h"
#include "formats/xeen/XeenVoc.h"
#include "games/xeen/XeenPartyLoader.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "formats/xeen/XeenEventTextFormat.h"
#include <cstdio>
namespace mmodern::coverage {
namespace {
std::string resource(XeenMapIdentity id,const char *extension) {
    char name[32];std::snprintf(name,sizeof(name),id.number>=100?"mazex%03u.%s":"maze%04u.%s",unsigned(id.number),extension);return name;
}
}
OriginalContext::OriginalContext(const GameInstallation &installation, std::optional<XeenCamera> fixture):installation(installation),assets(installation),
    events([this](const std::string &name,XeenSide side)->std::optional<std::vector<std::uint8_t>> {
        if(!assets.hasInitialResource(name,side))return {};return assets.readInitialResource(name,side);
     }),texts([this](const std::string &name,XeenSide side)->std::optional<std::vector<std::uint8_t>> {
        const auto ar=side==XeenSide::Clouds?XeenSceneArchive::Clouds:XeenSceneArchive::DarksideOnly;if(!assets.hasSceneResource(name,ar))return {};return assets.readArchiveResource(name,ar);
    },[this](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
        return assets.readArchiveResource(name,XeenSceneArchive::DarksideOnly);
    }),world([this](auto id){return maps.loadGeometryMap(assets,id);},[this](auto id){return maps.loadObjects(assets,id);}) {
    if(fixture && fixture->mapId.side==XeenSide::Darkside) {
        OriginalContext base(installation);bindResources();
        auto saved=*base.initial;saved.camera=*fixture;saved.headCache.reset();
        saved.darksideSong=static_cast<std::uint8_t>(7*xeenMapMusicRow(maps.loadGeometryMap(assets,fixture->mapId).geometry));
        XeenSaveState::restoreBeforeGameplay(saved,resources,party,camera,flags,world,[](auto &,const auto &,const auto &,const auto &){});
        flow=std::make_unique<XeenEncounterFlow>(program,world,party,camera,flags,clock,XeenJourneyRestoreTag{});
        if(!flow->prepareJourneyFrame(flow->ticket(),[]{}) || !flow->presentJourney(flow->ticket()))throw std::runtime_error("Census Darkside presentation boundary failed");
        return;
    }
    party=XeenPartyLoader().loadInitialParty(assets);
    const auto chr=assets.readInitialResource("maze.chr"),pty=assets.readInitialResource("maze.pty");
    const auto statistics=XeenMonsterFormat::parse(*assets.readMonsterStatistics());
    if(fixture)camera=*fixture;
    const auto entryEvents=events.load(camera.mapId);
    XeenJourneySetup setup{chr,XeenGameplayContextFormat::parse(pty),statistics,entryEvents,7};
    setup.prepared=fixture.has_value();
    setup.eventFixture=fixture;
    setup.regionalManifest=[this](const auto &m,const auto &o,const auto &e,const auto &s) {
        xeenValidateRegionalManifest(m,o,e,s,assets.readInitialResource("maze0023.dat"),
            assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));
    };
    setup.darksideStatistics=XeenMonsterFormat::parse(*assets.readMonsterStatistics(XeenSide::Darkside));
    setup.purse=XeenCharacterFormat::parseMonsterPurse(pty);
    setup.worldFlags=XeenQuestFlagFormat::parseWorldFlags(pty);
    setup.bank=XeenCharacterFormat::parseBankBalances(pty);
    setup.regionalText=texts.load(23);
    setup.learnedNames=XeenLearnedSpellNames::parse(*assets.readLearnedSpellNamesFromDarkArchive());
    setup.learnedNamesProvider=[this]{return XeenLearnedSpellNames::parse(*assets.readLearnedSpellNamesFromDarkArchive());};
    setup.cityEventsProvider=[this]{return events.load(28);};
    setup.mainlandEventsProvider=[this]{return events.load(23);};
    setup.eventsProvider=[this](auto id){return events.load(id);};
    flow=std::make_unique<XeenEncounterFlow>(program,world,party,camera,flags,clock,setup);
    if(!flow->prepareJourneyFrame(flow->ticket(),[]{}) || !flow->presentJourney(flow->ticket()))
        throw std::runtime_error("Census New Game presentation boundary failed");
    bindResources();
    if(!fixture)initial=XeenSaveState::capture(resources.signature,party,camera,flags,world);
}
void OriginalContext::bindResources() {
    resources.signature=XeenSaveFile::fingerprint(installation);
    resources.loadInitialCharacters=[this]{return assets.readInitialResource("maze.chr");};
    resources.loadInitialParty=[this]{return XeenPartyLoader().loadInitialParty(assets);};
    resources.loadEvents=[this](auto id){return events.load(id);};
    resources.loadMonsterStatisticsForSide=[this](XeenSide side){return XeenMonsterFormat::parse(*assets.readMonsterStatistics(side));};
    resources.loadMonsterStatistics=[this]{return XeenMonsterFormat::parse(*assets.readMonsterStatistics());};
    resources.regionalManifest=[this](const auto &m,const auto &o,const auto &e,const auto &s) {
        xeenValidateRegionalManifest(m,o,e,s,assets.readInitialResource("maze0023.dat"),assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));
    };
    resources.loadRegionalText=[this](auto id){return text(id);};
    resources.loadLearnedSpellNames=[this]{return XeenLearnedSpellNames::parse(*assets.readLearnedSpellNamesFromDarkArchive());};
}
Support OriginalContext::replayRoot(XeenCamera entry) {
    try {
        auto snapshot=*initial;snapshot.camera=entry;snapshot.headCache.reset();
        if(entry.mapId.side==XeenSide::Darkside)snapshot.darksideSong=static_cast<std::uint8_t>(7*xeenMapMusicRow(maps.loadGeometryMap(assets,entry.mapId).geometry));
        XeenProgramState probe{mmodern::XeenWallClock::fixed(0)};
        XeenWorld w([this](auto id){return maps.loadGeometryMap(assets,id);},[this](auto id){return maps.loadObjects(assets,id);});
        XeenPartyState p;XeenCamera c;XeenGameFlags f;
        XeenSaveState::restoreBeforeGameplay(snapshot,resources,p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
        XeenEventSystem system([this](auto id){return XeenEventScript(events.load(id));},[this](auto id){return text(id);});
        const XeenFontFormat font(assets.readArchiveResource("fnt"));
        const auto catalogue=loadXeenItemCatalog(assets);
        const auto blank=[] {IndexedFrame frame;frame.width=320;frame.height=200;frame.pixels.resize(64000);return frame;};
        std::uint64_t now=0,cycle=0;
        XeenEventFlow session(probe,w,system,p,c,f,font,[&](auto){return XeenEventFlow::Composition{blank(),false};},
            {},[&]{return now;},{},&catalogue.catalog,[&](auto,auto){return XeenEventFlow::Composition{blank(),false};},
            nullptr,[&](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{blank(),false};});
        session.dialogText=&assets.uiText();
        session.drawDialogSprite=[this](auto &frame,const char *name,unsigned id,int x,int y){assets.drawDialogSprite(frame,name,id,x,y);};
        session.drawTrainingArt=[this](auto &frame){assets.drawTraining(frame);};
        session.drawSmithArt=[this](auto &frame){assets.drawSmith(frame);};
        session.drawTempleArt=[this](auto &frame){assets.drawTemple(frame);};
        session.drawTavernArt=[this](auto &frame){assets.drawTavern(frame);};
        session.drawBankArt=[this](auto &frame){assets.drawBank(frame);};
        session.drawGuildArt=[this](auto &frame){assets.drawGuild(frame);};
        session.tavernFoodOffer=[this](unsigned index){return assets.tavernFoodOffer(index);};
        session.loadSampleDuration=[this](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(assets.readArchiveResource(name,archive));};
        session.loadSpellDescriptions=[this]{return assets.readArchiveResource("spldesc.bin");};
        std::optional<Support> result;std::optional<XeenPresentationResponseRequirement> request;
        session.reportManual=[&](const XeenManualEventResult &value) {
            if(const auto *error=std::get_if<XeenEventExecutionError>(&value))result=Support{Status::Partial,error->message};
            if(std::holds_alternative<XeenManualEventNoEvent>(value) || std::holds_alternative<XeenManualSpecialInteractionUnsupported>(value))
                result=Support{Status::Partial,"No ordinary Event at this physical facing"};
            if(std::holds_alternative<XeenManualEventCompleted>(value))result=Support{Status::Supported,"Original Event completed through live Journey Flow publication"};
            if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&value))request=pending->request.response;
        };
        const auto present=[&](const IndexedFrame &frame){if(frame.presentation())session.framePresented(frame.presentation());};
        const auto act=[&](PlayerAction action){session.beginCycle(++cycle);present(session.handle(action,session.displayedInput(),session.frame().presentation()));};
        present(session.frame());act(InteractionAction{});
        for(unsigned step=0;step<512 && !result;++step) {
            if(w.sessionState().journeyActivity()==XeenJourneyActivity::Service)
                return {Status::Supported,"Original terminal handoff reached the live service owner"};
            now+=250;session.beginCycle(++cycle);
            if(const auto frame=session.updatePresentation())present(*frame);
            if(request) {
                const auto next=*request;
                if(next==XeenPresentationResponseRequirement::CharacterSelection)act(SelectMemberAction{0});
                else if(next==XeenPresentationResponseRequirement::YesNo)act(DialogKeyAction{'y'});
                else if(next==XeenPresentationResponseRequirement::Acknowledgment)act(AcknowledgeAction{});
            }
        }
        return result.value_or(Support{Status::Unclassified,"Live Flow probe did not reach completion or a service owner"});
    }catch(const std::exception &e) {return {Status::Unclassified,std::string("Live Flow probe: ")+e.what()};}
}
XeenEventTextFile OriginalContext::text(XeenMapIdentity id) {
    if(const auto found=textCache.find(id);found!=textCache.end())return found->second;
    return textCache.emplace(id,texts.load(id)).first->second;
}
}
