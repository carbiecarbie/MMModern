#ifndef MMODERN_TRAINING_TEST_SUPPORT_H
#define MMODERN_TRAINING_TEST_SUPPORT_H
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenVertigoRoute.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenSaveState.h"
#include "platform/XeenSaveFile.h"
#include "XeenSaveTestSupport.h"
namespace mmodern {
// Synthetic authority-limit controls; never used by an earned/process witness.
struct XeenTrainingTestAccess {
    static bool templeLobby(const XeenEventFlow &flow) {
        return flow._smithUi && flow._smithUi->mode==XeenEventFlow::SmithUi::Mode::Heal &&
            flow._smithUi->phase==XeenEventFlow::SmithUi::Phase::Lobby;
    }
    static bool templePending(const XeenEventFlow &flow) {
        return flow._encounter->_smith && flow._encounter->_smith->healPending &&
            flow._encounter->_smith->templeUpgrade;
    }
    static bool templeOneDayReserved(const XeenEventFlow &flow) {
        return flow._encounter->_smith && flow._encounter->_smith->departure &&
            flow._encounter->_smith->departure->complete() && !flow._encounter->_smith->paid;
    }
    static std::string templeText(const XeenEventFlow &flow) {return flow.smithText();}
    static std::uint64_t templeRevision(const XeenEventFlow &flow) {return flow._smithUi->revision;}
    static void templeRevision(XeenEventFlow &flow,std::uint64_t value) {
        flow._smithUi->revision=value;flow._smithRenderedRevision=value;
    }
    static std::string text(const XeenEventFlow &flow) {return flow.trainingText();}
    static bool quote(const XeenEventFlow &flow) {return flow._trainingUi && flow._trainingUi->phase==XeenEventFlow::TrainingUi::Phase::Quote;}
    static bool menu(const XeenEventFlow &flow) {return flow._trainingUi && flow._trainingUi->phase==XeenEventFlow::TrainingUi::Phase::Menu;}
    static std::size_t selected(const XeenEventFlow &flow) {return flow._trainingUi->member;}
    static void limit(XeenEventFlow &flow,unsigned counter,std::uint64_t value) {
        auto &encounter=*flow._encounter;
        switch(counter){case 0:encounter._generation=value;break;
        case 1:encounter._world._sessionState._journeyGeneration=value;break;
        case 2:flow._inputGeneration=value;if(encounter._training)encounter._training->input=value;
            if(encounter._smith)encounter._smith->input=value;break;
        case 3:encounter._boundary.epoch=value;break;
        case 4:encounter._training->operation=value;break;}
        encounter._journeyPreimage->adoptJourneyCoordination();flow._encounterFrame=encounter.ticket();
    }
    static bool room(XeenEventFlow &flow,unsigned steps,unsigned boundary) {return flow._encounter->smithCapacity(steps,boundary);}
    static void consume(XeenEventFlow &flow) {
        if(!flow._encounter->consumeTrainingFrame(flow._inputGeneration,flow._frame.presentation()))throw std::logic_error("Synthetic Training authority absent");
    }
    static void confirm(XeenEventFlow &flow) {flow._encounter->confirmTraining();}
    static bool level(XeenEventFlow &flow) {return flow._encounter->serviceTrainingLevel();}
    static void depart(XeenEventFlow &flow) {flow._encounter->departTraining();}
};
}
namespace training_test {
using namespace mmodern;
using save_test::check;
struct Inputs {
    XeenAssetSource assets;XeenMapLoader maps;XeenEventLoader events;XeenEventTextLoader texts;
    std::vector<std::uint8_t> chr,pty;std::vector<XeenMonsterRecord> statistics;
    XeenEventFile mainland,city;XeenLearnedSpellNames names;XeenFontFormat font;XeenSaveResourceSignature signature;
    explicit Inputs(const GameInstallation &installation):assets(installation,320,200),
        events([&](const auto &n)->std::optional<std::vector<std::uint8_t>>{if(!assets.hasInitialResource(n))return {};return assets.readInitialResource(n);}),
        texts([&](const auto &n)->std::optional<std::vector<std::uint8_t>>{if(!assets.hasArchiveResource(n))return {};return assets.readArchiveResource(n);}),
        font(assets.readArchiveResource("fnt")) {
        chr=assets.readInitialResource("maze.chr");pty=assets.readInitialResource("maze.pty");
        statistics=XeenMonsterFormat::parse(*assets.readCloudsMonsterStatisticsFromDarkArchive());
        mainland=events.load(23);city=events.load(28);names=XeenLearnedSpellNames::parse(*assets.readLearnedSpellNamesFromDarkArchive());
        signature=XeenSaveFile::fingerprint(installation);
    }
    auto mapLoader(){return [this](auto id){return maps.loadGeometryMap(assets,id);};}
    auto objectLoader(){return [this](auto id){return maps.loadObjects(assets,id);};}
    auto reader(){return [this](const std::string &n){return n.rfind("maze",0)==0?assets.readInitialResource(n):assets.readArchiveResource(n);};}
    XeenRegionalManifest regional(){return [this](const auto &m,const auto &o,const auto &e,const auto &s){
        xeenValidateRegionalManifest(m,o,e,s,assets.readInitialResource("maze0023.dat"),assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));};}
    XeenVertigoManifest vertigo(){return [this](auto &w,const auto &e,const auto &s){xeenValidateVertigoManifest(w,e,s,reader());};}
    XeenJourneySetup setup(std::uint16_t content=14){
        XeenJourneySetup value{chr,XeenGameplayContextFormat::parse(pty),statistics,mainland,7,content,regional()};
        value.purse=XeenCharacterFormat::parseMonsterPurse(pty);value.regionalRecovery=XeenQuestFlagFormat::parseRegionalRecovery(pty);
        value.regionalText=texts.load(23);value.learnedNames=names;value.learnedNamesProvider=[this]{return names;};
        value.vertigoManifest=vertigo();
        if(xeenJourneyContent(content).serviceDays())value.bank=XeenCharacterFormat::parseBankBalances(pty);
        value.cityEventsProvider=[this]{return city;};return value;
    }
    XeenSaveState::Resources resources(){
        XeenSaveState::Resources r;r.signature=signature;r.loadInitialCharacters=[this]{return chr;};
        r.loadEvents=[this](auto id){return events.load(id);};r.loadMonsterStatistics=[this]{return statistics;};
        r.regionalManifest=regional();r.vertigoManifest=vertigo();r.loadRegionalText=[this](auto id){return texts.load(id);};
        r.loadLearnedSpellNames=[this]{return names;};return r;
    }
    XeenSaveSnapshot base(std::uint16_t content=14){
        auto p=XeenPartyLoader().loadInitialCloudsParty(assets);auto c=xeenJourneyContent(content).entry;XeenGameFlags f;
        XeenWorld w(mapLoader(),objectLoader());auto value=setup(content);XeenEncounterFlow flow(w,p,c,f,[]{return 0;},value);
        check(flow.prepareJourneyFrame(flow.ticket(),[]{}) && flow.presentJourney(flow.ticket()),"synthetic base frame");
        return XeenSaveState::capture(signature,p,c,f,w);
    }
    XeenSaveSnapshot service(unsigned day=8,unsigned xp=9000){
        auto s=base();s.camera={28,10,11,XeenDirection::North};s.journey->context->day=day;
        const auto actors=XeenActorApproach::actorsFromResources(maps.loadObjects(assets,28),statistics);
        s.journey->vertigoActors.emplace();
        for(const auto &a:actors)s.journey->vertigoActors->push_back({a.id,a.x,a.y,a.hp,a.activated,a.lifecycle,a.status,false});
        auto &a=s.journey->vertigoActors->at(35);a.x=a.y=-128;a.hp=0;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
        s.journey->supplements[18].inputs.experience=xp;s.journey->supplements[1].inputs.experience=xp;
        return s;
    }
};
inline IndexedFrame frame(){IndexedFrame f;f.width=320;f.height=200;f.pixels.resize(64000);return f;}
// Clearly synthetic owner/date/XP fixtures restored through the canonical codec.
// They are independent of the earned original-resource/process witness.
struct Fixture {
    Inputs &in;XeenWorld w;XeenPartyState p;XeenCamera c;XeenGameFlags f;XeenEventSystem events;
    std::unique_ptr<XeenEventFlow> flow;std::uint64_t cycle=0,now=0;
    std::bitset<30> trained;
    Fixture(Inputs &in,const XeenSaveSnapshot &source,bool animated=false):in(in),w(in.mapLoader(),in.objectLoader()),
        events([&in](auto id){return XeenEventScript(in.events.load(id));},[&in](auto id){return in.texts.load(id);}) {
        XeenSaveState::restoreBeforeGameplay(XeenSaveFormat::decode(XeenSaveFormat::encode(source)),in.resources(),p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
        flow=std::make_unique<XeenEventFlow>(w,events,p,c,f,in.font,[](auto){return XeenEventFlow::Composition{frame(),false};},
            XeenEventPresenter::NpcDraw{},[this]{return now;},XeenEventPresenter::RandomFrame{},nullptr,
            [animated](auto,auto){return XeenEventFlow::Composition{frame(),animated};});
        flow->drawTrainingArt=[&in](auto &frame){in.assets.drawTraining(frame);};
        present(flow->frame());check(flow->canSave(),"synthetic restored service checkpoint not Quiet");
    }
    void present(const IndexedFrame &value){if(value.presentation())flow->framePresented(value.presentation());}
    void act(PlayerAction action){flow->beginCycle(++cycle);auto frame=flow->handle(action,flow->displayedInput(),flow->frame().presentation());present(frame);}
    void prepare(){for(unsigned n=0;n<1000;++n){flow->beginCycle(++cycle);const auto next=flow->updatePresentation();if(next){present(*next);return;}}throw std::runtime_error("Training bounded preparation never finished");}
    void enter(){trained.reset();act(InteractionAction{});prepare();check(w.sessionState().journeyActivity()==XeenJourneyActivity::Service,"Training service not admitted");}
    void train(unsigned member){
        const auto owner=p.party.activeRosterIds()[member];const int before=p.roster.at(owner).permanentLevel;
        const auto quote=xeenQuoteTraining(p.roster.at(owner),*p.roster.combatInputs(owner),p.monsterTreasure->gold,*p.encounterContext);
        const bool eligible=quote.outcome==XeenTrainingOutcome::Quoted && (trained.test(owner) || p.encounterContext->day<=97);
        act(SelectMemberAction{member});act(AcknowledgeAction{});
        if(eligible) {
            act(AcknowledgeAction{});prepare();
            if(p.roster.at(owner).permanentLevel>before){trained.set(owner);act(AcknowledgeAction{});}
        } else act(AcknowledgeAction{});
    }
    XeenSaveSnapshot snapshot(){return XeenSaveState::capture(in.signature,p,c,f,w);}
};
}
#endif
