#include "app/XeenEventFlow.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenJourneyCapture.h"
#include <algorithm>
namespace mmodern {
namespace {
constexpr unsigned classKey=1024;
constexpr char attributeKeys[]{'m','i','p','e','s','a','l'};
struct CreateBusy {bool &value;explicit CreateBusy(bool &v):value(v){value=true;}~CreateBusy(){value=false;}};
IndexedFrame writeCreate(const IndexedFrame &base,const XeenFontFormat &font,const std::string &text,
    XeenTextRenderOptions options) {
    options.originalControls=true;
    auto result=XeenTextRenderer(font).render(base,text,options);
    if(result.pages.size()!=1 || !result.diagnostics.empty())throw std::runtime_error("Original Create text overflow");
    return std::move(result.pages.front());
}
XeenTextRenderOptions screenText() {
    XeenTextRenderOptions o;o.bounds={0,0,320,200};o.colorIndex=4;return o;
}
}
std::optional<std::array<int,7>> XeenEncounterFlow::innRoll(const Ticket &entry) {
    if(!_party.insideInn || !current(entry) || _busy || _combat || !_boundary.quiet() || !journeyCapacity())return {};
    CreateBusy busy(_busy);_journeyPreimage->check();
    XeenCombatRandom random(*_world.sessionState().journeyRandom());
    const auto values=xeenCreateRoll(random);
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);
    prepared->prepareJourneyRandom(random.continuation());
    _journeyPreimage->check();
    _world._sessionState._journeyRandom=random.continuation();
    ++_generation;prepared->adoptMutationBoundary();_journeyPreimage.swap(prepared);
    return values;
}
void XeenEventFlow::rollCreate() {
    auto &c=*_innUi->create;
    const auto rolled=_encounter->innRoll(_encounter->ticket());
    if(!rolled)throw std::logic_error("Create roll authority unavailable");
    c.attributes=*rolled;c.arrow=xeenCreateArrow(xeenCreateClasses(c.attributes));c.committed.reset();
    _encounterFrame=_encounter->ticket();
}
bool XeenEventFlow::restartCreate() {
    auto &c=*_innUi->create;c.slots=xeenCreateSlots(_party.roster);c.slot=0;c.committed.reset();c.exchange.reset();c.naming=false;
    if(c.slots.empty())return false;
    rollCreate();refreshCreateDetails();return true;
}
IndexedFrame XeenEventFlow::beginCreate() {
    auto &ui=*_innUi;ui.remove=false;ui.selected.reset();ui.create.emplace();
    ui.closing=_frame;ui.phase=InnUi::Phase::FadeToCreate;ui.fade=128;
    ui.deadline=PresentationClock::retraceAfter(presentationNow());return renderCreate();
}
void XeenEventFlow::refreshCreateDetails() {
    auto &c=*_innUi->create;
    const auto ticket=_encounter->ticket();
    if(!_encounter->prepareJourneyFrame(ticket,[&] {
        if(!c.background.isValid()) {
            if(!loadCreateScreen)throw std::runtime_error("Missing original Create screen provider");
            c.background=loadCreateScreen();c.details=c.background;
        }
        auto &frame=c.details;
        const auto draw=[&](unsigned i,int x,int y){drawDialogSprite(frame,"create.icn",i,x,y);};
        for(unsigned i=0;i<7;++i){draw(10+2*i,168,19+24*i);draw(50+i,195,31+24*i);}
        for(unsigned i=0;i<10;++i)draw(24+2*i,227,19+11*i);
        draw(57,62,148);draw(58,62,158);draw(59,62,168);draw(61,220,19);draw(64,220,155);draw(65,220,170);
        draw(0,132,98);draw(2,132,128);draw(4,132,158);draw(6,86,98);draw(8,86,120);
        const auto id=c.slots.at(c.slot);
        auto face=xeenDeletedCharacter(id).portraitResourceName();
        if(!face)throw std::logic_error("Missing Create portrait");
        drawDialogSprite(frame,face->c_str(),0,27,102);
        const auto details=xeenCreateDetails(dosText(),id,c.attributes,c.committed);
        frame=writeCreate(frame,_inventoryFont,xeenDialogFormat(dosText().scalar("CREATE_LABELS"),{details}),screenText());
        if(c.arrow)draw(63,220,21+11**c.arrow);
    }))throw std::runtime_error("Create detail preparation failed");
    _encounterFrame=_encounter->ticket();
}
std::shared_ptr<const DialogInput> XeenEventFlow::createInput() const {
    const auto &ui=*_innUi;DialogInput input;
    if(!ui.feedback.empty()){input.anyKey=input.anyClick=input.keyMouseWait=true;}
    else if(ui.phase==InnUi::Phase::Create) {
        const auto &c=*ui.create;
        if(c.naming) {input.textEntry=true;input.keys={InputKey::Escape,InputKey::Enter,8,InputKey::Left};}
        else {
            const auto button=[&](int x,int y,int width,int height,unsigned key,const char *resource=nullptr,unsigned frame=0) {
                input.hits.push_back({x,y,x+width,y+height,key,resource?std::optional<InputButton>{{resource,frame,x,y}}:std::nullopt});input.keys.push_back(key);
            };
            for(unsigned i=0;i<7;++i)button(168,19+24*i,24,20,attributeKeys[i],"create.icn",10+2*i);
            if(c.exchange)button(118,58,24,20,InputKey::Escape,"create2.icn",0);
            else {
                button(132,98,24,20,'r',"create.icn",0);button(132,128,24,20,'c',"create.icn",2);
                button(132,158,24,20,InputKey::Escape,"create.icn",4);button(86,98,24,20,InputKey::Up,"create.icn",6);
                button(86,120,24,20,InputKey::Down,"create.icn",8);
                for(unsigned i=0;i<10;++i)button(227,19+11*i,12,10,classKey+i);
                input.keys.insert(input.keys.end(),{InputKey::Left,InputKey::Right,InputKey::Enter,InputKey::Space});
                input.anyKey=input.retainKey=true;
            }
        }
    }
    return std::make_shared<const DialogInput>(std::move(input));
}
IndexedFrame XeenEventFlow::renderCreate(bool cosmetic) {
    auto &ui=*_innUi;IndexedFrame returned;
    if(!_encounter->prepareJourneyFrame(_encounter->ticket(),[&] {
        auto frame=ui.phase==InnUi::Phase::FadeToCreate?ui.closing:
            ui.phase==InnUi::Phase::FadeInInn?ui.opening:ui.create->details.isValid()?ui.create->details:ui.closing;
        if(ui.phase!=InnUi::Phase::FadeToCreate && ui.phase!=InnUi::Phase::FadeInInn && ui.create->details.isValid()) {
            const auto &c=*ui.create;
            drawDialogSprite(frame,"dice.vga",7,12,11);
            for(const auto &d:c.dice)drawDialogSprite(frame,"dice.vga",d.frame,d.x,d.y);
            if(c.exchange) {
                drawDialogSprite(frame,"create.icn",11+2**c.exchange,168,19+24**c.exchange);
                XeenTextRenderOptions o;o.drawWindow=true;o.windowBounds={15,15,161,91};o.bounds={23,23,153,83};o.x=23;o.y=23;o.colorIndex=4;
                const auto label=*c.exchange==3?std::string(dosText().scalar("CREATE_ENDURANCE")):
                    dosText().table("ATTRIBUTE_NAMES").at(*c.exchange<3?*c.exchange:*c.exchange-1);
                frame=writeCreate(frame,_inventoryFont,xeenDialogFormat(dosText().scalar("CREATE_EXCHANGE"),{label}),o);
                drawDialogSprite(frame,"create2.icn",0,118,58);
            }
            if(c.naming) {
                XeenTextRenderOptions o;o.originalControls=true;o.colorIndex=4;o.drawWindow=true;
                o.windowBounds={52,149,268,198};o.bounds={60,157,260,190};o.x=60;o.y=157;
                frame=writeCreate(frame,_inventoryFont,std::string(dosText().scalar("CREATE_NAME")),o);
                o.drawWindow=false;o.x=60;o.y=177;
                const auto layout=xeenDialogFormat(dosText().scalar("NAME_INPUT_LAYOUT"),{"200"});
                frame=c.name.render(frame,_inventoryFont,layout,o);
            }
        }
        if(!ui.feedback.empty()) {
            XeenTextRenderOptions o;o.drawWindow=true;o.windowBounds={52,149,268,198};o.bounds={60,157,260,190};o.x=60;o.y=157;o.colorIndex=4;
            frame=writeCreate(frame,_inventoryFont,xeenDialogFormat(dosText().scalar("ERROR_SCROLL"),{ui.feedback}),o);
        }
        if(ui.phase==InnUi::Phase::FadeToCreate || ui.phase==InnUi::Phase::FadeInCreate ||
            ui.phase==InnUi::Phase::FadeFromCreate || ui.phase==InnUi::Phase::FadeInInn)
            frame.scaleVgaPalette(ui.fade);
        _frame=std::move(frame);returned=_frame;sealFrame(returned);
    },cosmetic))throw std::runtime_error("Create presentation failed");
    if(!cosmetic)++_inputGeneration;
    _encounterFrame=_encounter->ticket();_handoffPending=!cosmetic;_cosmeticPending=cosmetic;ui.acquired=false;return returned;
}
void XeenEventFlow::saveCreateReturn() {
    auto &ui=*_innUi;ui.phase=InnUi::Phase::CreateSave;
    // The no-free-slot return never displayed Create: retain black under a
    // recoverable write error as well as through the following return fade.
    if(!ui.create->details.isValid())std::fill(ui.closing.pixels.begin(),ui.closing.pixels.end(),0);
    try {forcedInnSave();}
    catch(const std::exception &e) {
        _encounter->journeySavePreimage().check();
        const std::string error=e.what();
        ui.feedback=error.find("Windows error 112")!=std::string::npos || error.find("Windows error 39")!=std::string::npos?
            std::string(dosText().scalar("SAVE_AS_SPACE")):error;return;
    }
    ui.top=0;ui.townOnly=false;ui.phase=InnUi::Phase::FadeFromCreate;
    ui.fade=128;
    ui.deadline=PresentationClock::retraceAfter(presentationNow());
}
std::optional<IndexedFrame> XeenEventFlow::updateCreate() {
    auto &ui=*_innUi;
    if(_cosmeticPending)return {};
    if(!ui.feedback.empty() || ui.phase==InnUi::Phase::CreateSave)return {};
    if(ui.phase==InnUi::Phase::Create) {
        auto &c=*ui.create;
        if(c.naming) {if(c.name.animate(_clock()))return renderCreate(true);return {};}
        if(c.exchange || presentationNow()<ui.deadline)return {};
        xeenAdvanceCreateDice(c.dice);return renderCreate(true);
    }
    if(presentationNow()<ui.deadline)return {};
    const bool out=ui.phase==InnUi::Phase::FadeToCreate || ui.phase==InnUi::Phase::FadeFromCreate;
    if(out && ui.fade)ui.fade-=4;
    else if(!out && ui.fade<128)ui.fade+=4;
    else if(ui.phase==InnUi::Phase::FadeToCreate) {
        if(!restartCreate())saveCreateReturn();
        else {xeenAdvanceCreateDice(ui.create->dice);ui.phase=InnUi::Phase::FadeInCreate;ui.fade=0;}
    } else if(ui.phase==InnUi::Phase::FadeFromCreate) {
        if(!_encounter->prepareJourneyFrame(_encounter->ticket(),[&] {
            ui.opening=loadInnScreen(_party);
            ui.opening=drawXeenInn(dosText(),ui.opening,_inventoryFont,_party,xeenInnList(_party,_camera.mapId),0,drawDialogSprite);
        }))throw std::runtime_error("Create return preparation failed");
        ui.phase=InnUi::Phase::FadeInInn;ui.fade=0;
    } else if(ui.phase==InnUi::Phase::FadeInCreate) {ui.phase=InnUi::Phase::Create;}
    else {ui.phase=InnUi::Phase::Menu;ui.create.reset();ui.opening={};return renderInn();}
    ui.deadline=PresentationClock::retraceAfter(presentationNow());return renderCreate();
}
IndexedFrame XeenEventFlow::handleCreate(const PlayerAction &action) {
    auto &ui=*_innUi;CreateBusy dispatch(_dispatching);
    if(!ui.feedback.empty()) {
        ui.feedback.clear();if(ui.phase==InnUi::Phase::CreateSave)saveCreateReturn();
        else xeenAdvanceCreateDice(ui.create->dice);
        return renderCreate();
    }
    if(ui.phase!=InnUi::Phase::Create)return frameCopy();
    auto &c=*ui.create;unsigned key=0;
    if(const auto *k=std::get_if<DialogKeyAction>(&action))key=k->key;
    if(std::holds_alternative<CancelInteractionAction>(action))key=InputKey::Escape;
    if(c.naming) {
        if(const auto *typed=std::get_if<TextInputAction>(&action)) {
            c.name.type(typed->text,_inventoryFont,10,true);c.name.keyRedraw();return renderCreate();
        }
        if(key==8 || key==InputKey::Left){c.name.backspace();c.name.keyRedraw();return renderCreate();}
        if(key!=InputKey::Enter && key!=InputKey::Escape)return frameCopy();
        if(key==InputKey::Enter && !c.name.value().empty()) {
            auto record=xeenCreatedCharacter(c.slots.at(c.slot),c.name.value(),*c.committed,c.attributes,_camera.mapId,*_party.encounterContext);
            if(!_encounter->innRecord(*_encounterFrame,std::move(record.first),std::move(record.second),_party.party.activeRosterIds()))return frameCopy();
            _encounterFrame=_encounter->ticket();
            if(!restartCreate()){saveCreateReturn();return renderCreate();}
        } else c.naming=false;
    } else {
        std::optional<unsigned> attribute;
        for(unsigned i=0;i<7;++i)if(key==unsigned(attributeKeys[i]))attribute=i;
        if(c.exchange) {
            if(key==InputKey::Escape)c.exchange.reset();
            else if(attribute && attribute!=c.exchange) {
                std::swap(c.attributes[*attribute],c.attributes[*c.exchange]);c.exchange.reset();
                c.committed.reset();c.arrow=xeenCreateArrow(xeenCreateClasses(c.attributes));refreshCreateDetails();
            } else return frameCopy();
        } else if(attribute){c.exchange=attribute;return renderCreate();}
        else if(key==InputKey::Escape){saveCreateReturn();return renderCreate();}
        else if(key=='r' || key==InputKey::Space){rollCreate();refreshCreateDetails();}
        else if(key==InputKey::Up || key==InputKey::Down) {
            c.slot=xeenMoveCreateSlot(c.slot,c.slots.size(),key==InputKey::Up?-1:1);refreshCreateDetails();
        } else if(key==InputKey::Left || key==InputKey::Right) {
            c.arrow=xeenMoveCreateArrow(xeenCreateClasses(c.attributes),c.arrow,key==InputKey::Left?-1:1);refreshCreateDetails();
        } else if(key==InputKey::Enter || (key>=classKey && key<classKey+10)) {
            const auto selected=key==InputKey::Enter?c.arrow:std::optional<unsigned>{key-classKey};
            if(selected && xeenCreateClasses(c.attributes)[*selected]){c.committed=selected;refreshCreateDetails();}
        } else if(key=='c') {
            if(!c.committed)ui.feedback=dosText().scalar("CREATE_NO_CLASS");
            else {c.naming=true;c.name.clear();c.name.begin();}
            return renderCreate();
        } else _encounter->fmEffect(21);
    }
    xeenAdvanceCreateDice(c.dice);return renderCreate();
}
}
