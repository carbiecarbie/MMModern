// Original-resource checkpoints exercise Application's actual F9/panel interception.
#include "XeenTrainingTestSupport.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "app/XeenTitleFlow.h"
#include <chrono>
#include <fstream>
#include <iterator>

using namespace mmodern;
using save_test::check;
namespace {
struct Sentinel {
    std::filesystem::path path=std::filesystem::temp_directory_path()/
        ("mmodern-food-bank-save-"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count())+".sav");
    Sentinel(){std::ofstream out(path,std::ios::binary);out<<"untouched M56 save target";check(bool(out),"Cannot create save sentinel");}
    ~Sentinel(){std::error_code error;std::filesystem::remove(path,error);}
    void unchanged() const {
        std::ifstream in(path,std::ios::binary);
        check(std::string(std::istreambuf_iterator<char>(in),{})=="untouched M56 save target","Blocked service save changed its target");
    }
};
void visit(training_test::Inputs &in,bool tavern,XeenSmithBoundary fault) {
    auto source=in.service(10);source.food=1;source.journey->treasure->gold=100;
    const int x=tavern?24:26,y=tavern?5:17;
    bool found=false;
    for(const auto &record:in.city.records)if(record.x==x && record.y==y && !record.line) {
        source.camera={28,x,y,record.direction==4?XeenDirection::North:XeenDirection(record.direction)};found=true;break;
    }
    check(found,"Original save-denial service entrance missing");
    Sentinel target;XeenEventFlow *flow=nullptr;const XeenPartyState *party=nullptr;
    unsigned providers=0,stages=0,panels=0,writes=0,denials=0,handoffs=0,faults=0;
    std::uint64_t now=0,cycle=0;
    auto resources=in.resources();
    const auto initial=resources.loadInitialParty;resources.loadInitialParty=[&]{++providers;return initial();};
    const auto characters=resources.loadInitialCharacters;resources.loadInitialCharacters=[&]{++providers;return characters();};
    const auto events=resources.loadEvents;resources.loadEvents=[&](auto id){++providers;return events(id);};
    const auto statistics=resources.loadMonsterStatistics;resources.loadMonsterStatistics=[&]{++providers;return statistics();};
    const auto regional=resources.loadRegionalText;resources.loadRegionalText=[&](auto id){++providers;return regional(id);};
    const auto names=resources.loadLearnedSpellNames;resources.loadLearnedSpellNames=[&]{++providers;return names();};
    XeenGameplayServices services{resources,[]{return XeenGameFlags{};},
        [&](auto id){++providers;return in.maps.loadGeometryMap(in.assets,id);},
        [&](auto id){++providers;return in.maps.loadObjects(in.assets,id);},
        [&](auto id){++providers;return in.texts.load(id);},in.font,
        [](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{training_test::frame(),false};}, {},
        [&](auto &configured,const auto &){
            flow=&configured;flow->dialogText=&in.assets.uiText();
            flow->drawTavernArt=[&](auto &image){in.assets.drawTavern(image);};
            flow->drawBankArt=[&](auto &image){in.assets.drawBank(image);};
            flow->tavernFoodOffer=[&](unsigned index) {return in.assets.tavernFoodOffer(index);};
            flow->drawDialogSprite=[&](auto &image,const char *resource,unsigned frame,int sx,int sy){in.assets.drawDialogSprite(image,resource,frame,sx,sy);};
        }, {},[&](auto &,auto &,const auto &live,auto &,const auto &){party=&live;}};
    services.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{training_test::frame(),false};};
    services.clock=[&]{return now;};
    services.restoreSnapshot=std::make_shared<XeenSaveSnapshot>(source);
    services.developerSavePath=target.path;
    services.observeSaveStage=[&](auto){++stages;};
    services.writeManaged=[&](auto,const auto &,const auto &){++writes;};
    services.panel=[&](const auto &,bool,bool,bool,auto,const auto &)->std::unique_ptr<XeenTitleFlow>{
        ++panels;throw std::runtime_error("Blocked service opened the control panel");
    };
    services.show=[&](const auto &first,const auto &handler,const auto &,const auto &idle,const auto &){
        IndexedFrame visible=first;
        const auto present=[&]{
            check(handler.acceptsFrame(visible.presentation()),"Save-denial test received stale frame");
            handler.framePresented(visible.presentation());handler.completeInputHandoff(visible.presentation());
        };
        present();check(flow && party && flow->canSave(),"Initial service checkpoint is not saveable");
        const auto denied=[&]{
            check(!flow->canSave(),"Application service boundary permits saving");
            const auto before=providers;const auto input=handler.displayedInput();check(bool(input),"Missing service input generation");
            check(!handler.withPresentedInput(SaveGameAction{},*input,visible.presentation()),"F9 published a service frame");
            check(!handler.withPresentedInput(ControlPanelAction{},*input,visible.presentation()),"Control panel published a service frame");
            check(providers==before && !stages && !panels && !writes,"Blocked save reached providers/capture/panel/write");
            target.unchanged();++denials;
        };
        flow->smithBoundary=[&](auto boundary){
            denied();if(boundary==XeenSmithBoundary::BeforeReservation)++handoffs;
            if(boundary==fault && !faults++)throw std::bad_alloc();
        };
        const auto send=[&](const PlayerAction &action){
            handler.beginCycle(++cycle);const auto input=handler.displayedInput();check(bool(input),"Missing Application input generation");
            if(auto next=handler.withPresentedInput(action,*input,visible.presentation()))visible=*next;
            present();
        };
        const auto pulse=[&]{now+=100;handler.beginCycle(++cycle);if(auto next=idle())visible=*next;present();};
        const auto dialog=[&]{return handler.inputContext(visible.presentation()).dialog;};
        const auto dismiss=[&]{if(dialog() && dialog()->anyKey)send(AcknowledgeAction{});};
        send(InteractionAction{});denied();
        for(unsigned n=0;n<1000 && !(dialog() && dialog()->key(tavern?'f':'d'));++n)pulse();
        check(dialog() && dialog()->key(tavern?'f':'d'),"Original Application service lobby missing");denied();
        const auto transact=[&]{
            if(tavern){send(DialogKeyAction{'f'});check(dialog() && dialog()->key('y'),"Application Food confirmation missing");denied();send(DialogKeyAction{'y'});}
            else {
                if(dialog()->key('d'))send(DialogKeyAction{'d'});
                denied();send(DialogKeyAction{'o'});check(dialog() && dialog()->textEntry,"Application amount entry missing");denied();
                send(TextInputAction{"10"});denied();send(DialogKeyAction{InputKey::Enter});
            }
            denied();dismiss();
        };
        transact();
        if(party->monsterTreasure->gold==100)transact(); // Before-publication failure retries the original attempt.
        check(party->monsterTreasure->gold==90,"Application transaction retry lost/repeated payment");
        for(unsigned n=0;n<100 && !flow->canSave();++n){
            denied();dismiss();if(!flow->canSave())send(CancelInteractionAction{});if(!flow->canSave())pulse();
        }
        check(faults && handoffs==1 && denials>5 && flow->canSave(),"Application service retry did not settle once into a saveable boundary");
        check(!flow->serviceSaveBlocked() && party->encounterContext->day==11,"Application save gate remained blocked after departure");
        target.unchanged();check(!stages && !panels && !writes,"Blocked Application save reached persistence");
        flow->smithBoundary={};return true;
    };
    check(Application{}.playGameplay(services,source.camera,target.path,true)==0,"Food/Bank Application save-denial scenario failed");
    target.unchanged();
}
}
void foodBankSaveChecks(const GameInstallation &installation) {
    training_test::Inputs inputs(installation);
    for(bool tavern:{false,true})for(auto fault:{XeenSmithBoundary::BeforePurchase,XeenSmithBoundary::AfterPurchase,
        XeenSmithBoundary::BeforeDeparture,XeenSmithBoundary::DeparturePublished,XeenSmithBoundary::AfterDeparture,
        XeenSmithBoundary::Return,XeenSmithBoundary::BeforeEventSettlement,XeenSmithBoundary::AfterEventSettlement})
        visit(inputs,tavern,fault);
}
