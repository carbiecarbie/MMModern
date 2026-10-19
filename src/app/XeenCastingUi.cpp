#include "app/XeenEventFlow.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenJumpRules.h"
#include <algorithm>

namespace mmodern {
bool XeenEventFlow::beginTravelDialog(XeenLearnedSpell spell,std::uint8_t owner,bool item) {
    XeenSpellTravel rule;TravelUi ui;
    {XeenRestoreGuard::Providers providers(_encounter->journeySavePreimage(),_world);
    rule=XeenLearnedSpellRules::travel(_world,_camera,spell,0,&_party.roster.at(owner));
    _encounter->journeySavePreimage().check();
    if(rule.failed)return false;
    ui.spell=spell;ui.owner=owner;ui.item=item;ui.digits.begin();
    if(spell==XeenLearnedSpell::LloydsBeacon || spell==XeenLearnedSpell::TownPortal) {
        if(!mapName)throw std::logic_error("Travel map name provider missing");
        for(unsigned i=0;i<(spell==XeenLearnedSpell::TownPortal?5u:1u);++i) {
            const auto b=rule.beacon;
            ui.names[i]=mapName(spell==XeenLearnedSpell::TownPortal?XeenMapIdentity(28+i):XeenMapIdentity(static_cast<XeenSide>((*b)[3]),(*b)[0]));
            _encounter->journeySavePreimage().check();
        }
    }
    }
    if(spell==XeenLearnedSpell::LloydsBeacon && !_encounter->initializeBeacon(owner))return false;
    _travelUi=std::move(ui);return true;
}
IndexedFrame XeenEventFlow::drawTravelDialog(const IndexedFrame &base) const {
    const auto &ui=*_travelUi;
    if(ui.spell==XeenLearnedSpell::LloydsBeacon || ui.spell==XeenLearnedSpell::TownPortal) {
        const auto &c=_party.roster.at(ui.owner);
        XeenTextRenderOptions options;options.originalControls=true;options.drawWindow=true;
        const bool beacon=ui.spell==XeenLearnedSpell::LloydsBeacon;
        options.windowBounds=beacon?XeenTextRect{226,0,320,146}:XeenTextRect{80,28,256,148};
        options.bounds=beacon?XeenTextRect{234,8,312,138}:XeenTextRect{88,36,248,140};options.x=options.bounds.left;options.y=options.bounds.top;options.stopAtBottom=true;
        const auto text=beacon?xeenDialogFormat(dosText().scalar("LLOYDS_BEACON"),{ui.names[0],std::to_string(c.lloydX),std::to_string(c.lloydY)}):
            xeenDialogFormat(dosText().scalar("TOWN_PORTAL"),{ui.names[0],ui.names[1],ui.names[2],ui.names[3],ui.names[4]});
        // Window 10 redraws the protection corners and covers the right statue,
        // as the learned cast window does beneath it.
        auto underlay=base;
        if(beacon && ui.item) {underlay=drawXeenProtectionCorners(base,*_party.encounterContext,drawDialogSprite);if(drawDialogSprite)drawDialogSprite(underlay,"border.icn",52,215,32);}
        auto rendered=XeenTextRenderer(_inventoryFont).render(underlay,text,options);
        if(rendered.pages.size()!=1 || !rendered.diagnostics.empty())throw std::runtime_error("Travel dialog layout overflow");
        auto frame=std::move(rendered.pages.front());
        if(beacon && drawDialogSprite) {drawDialogSprite(frame,"lloyds.icn",2,242,108);drawDialogSprite(frame,"lloyds.icn",0,281,108);}
        if(!beacon) {
            // Input::getString continues at Window 20's saved write position,
            // clears 160 pixels, and centers the digit. FontSurface clips this
            // final input row to the window, including the animated cursor.
            options.drawWindow=false;options.x=rendered.writeX;options.y=rendered.writeY;
            options.size=rendered.writeSize;options.colorIndex=rendered.writeColor;
            const auto checked=[](const XeenTextRenderResult &r) {
                if(r.pages.size()!=1 || std::any_of(r.diagnostics.begin(),r.diagnostics.end(),[](const auto &d){return d!="Original dialog text clipped vertically";}))
                    throw std::runtime_error("Town Portal input layout overflow");
            };
            auto digits=XeenTextRenderer(_inventoryFont).render(frame,"\x03l\t000\x04" "160\x03" "c"+ui.digits.value(),options);checked(digits);
            options.x=digits.writeX;options.y=digits.writeY;options.alignment=XeenTextAlignment::Left;
            auto cursor=XeenTextRenderer(_inventoryFont).render(digits.pages.front(),std::string(1,char(ui.digits.cursor())),options);checked(cursor);
            frame=std::move(cursor.pages.front());
        }
        if(!ui.refusal.empty())frame=drawXeenErrorScroll(frame,_inventoryFont,ui.refusal);
        return frame;
    }
    XeenTextRenderOptions options;options.originalControls=true;options.drawWindow=true;
    options.windowBounds={52,149,268,198};options.bounds={60,157,260,190};options.x=60;options.y=157;
    const auto prompt=xeenDialogFormat(dosText().scalar("HOW_MANY_SQUARES"),
        {dosText().table("DIRECTION_TEXT").at(unsigned(XeenDirection(_camera.direction)))});
    auto frame=_travelUi->digits.render(base,_inventoryFont,prompt,options);
    if(!_travelUi->refusal.empty())frame=drawXeenErrorScroll(base,_inventoryFont,_travelUi->refusal);
    return frame;
}
IndexedFrame XeenEventFlow::handleTravelDialog(const PlayerAction &action,std::uint64_t input) {
    auto &ui=*_travelUi;
    if(!ui.refusal.empty()) {ui.refusal.clear();return renderEncounter();}
    const auto *key=std::get_if<DialogKeyAction>(&action);
    const unsigned value=key?key->key:std::holds_alternative<CancelInteractionAction>(action)?InputKey::Escape:
        std::holds_alternative<AcknowledgeAction>(action)?InputKey::Enter:0;
    if(ui.spell==XeenLearnedSpell::LloydsBeacon) {
        const unsigned choice=value=='s' || value=='t'?1:value=='r'?2:0;
        if(!choice && value!=InputKey::Escape)return frameCopy();
        {XeenRestoreGuard::Providers providers(_encounter->journeySavePreimage(),_world);
            const auto rule=XeenLearnedSpellRules::travel(_world,_camera,ui.spell,choice,&_party.roster.at(ui.owner));
            _encounter->journeySavePreimage().check();
            if(rule.unsupported) {ui.refusal="Spell not supported yet";return renderEncounter();}}
        const bool item=ui.item;_travelUi.reset();_travelChoice=choice;
        IndexedFrame result;if(item){performItemOption(2);result=renderEncounter();}else result=handleCasting(DialogKeyAction{'c'},input);
        _travelChoice.reset();return result;
    }
    if(const auto *typed=std::get_if<TextInputAction>(&action)) {
        if(typed->text.size()==1 && typed->text[0]>='0' && typed->text[0]<='9')ui.digits.type(typed->text,_inventoryFont,1);
        ui.digits.keyRedraw();
        return renderEncounter();
    }
    if(value>='0' && value<='9') {ui.digits.type(std::string(1,char(value)),_inventoryFont,1);ui.digits.keyRedraw();return renderEncounter();}
    if(value==8 || value==127) {ui.digits.backspace();ui.digits.keyRedraw();return renderEncounter();}
    if(value!=InputKey::Enter && value!=InputKey::Escape)return frameCopy();
    const unsigned distance=value==InputKey::Escape || ui.digits.value().empty()?0:unsigned(ui.digits.value()[0]-'0');
    {XeenRestoreGuard::Providers providers(_encounter->journeySavePreimage(),_world);
        if(ui.spell==XeenLearnedSpell::TownPortal && distance>5) {ui.digits.clear();ui.digits.begin();return renderEncounter();}
        const auto travel=XeenLearnedSpellRules::travel(_world,_camera,ui.spell,distance,&_party.roster.at(ui.owner));
        _encounter->journeySavePreimage().check();
        if(travel.unsupported) {ui.refusal="Spell not supported yet";return renderEncounter();}
    }
    const bool item=ui.item;_travelUi.reset();_travelChoice=distance;
    // Hold payment until destination admission; cancellation is a paid no-op.
    IndexedFrame result;
    if(item) {performItemOption(2);result=renderEncounter();}
    else result=handleCasting(DialogKeyAction{'c'},input);
    _travelChoice.reset();return result;
}
namespace {
unsigned castKey(const PlayerAction &action) {
    if(const auto *key=std::get_if<DialogKeyAction>(&action))return key->key;
    if(const auto *member=std::get_if<SelectMemberAction>(&action))return InputKey::F1+unsigned(member->partyIndex);
    if(std::holds_alternative<CancelInteractionAction>(action))return InputKey::Escape;
    if(std::holds_alternative<AcknowledgeAction>(action))return InputKey::Enter;
    if(const auto *nav=std::get_if<NavigationAction>(&action)) {
        if(*nav==NavigationAction::MoveForward)return InputKey::Up;
        if(*nav==NavigationAction::MoveBackward)return InputKey::Down;
    }
    return 0;
}
std::string refusal(const XeenDosText &text,const XeenCharacter &c,const XeenLearnedSpellNames &names,const std::string &code) {
    if(code=="disabled")return xeenDialogFormat(text.scalar("IN_NO_CONDITION"),{c.name});
    const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
    const auto id=category?XeenLearnedSpellRules::spellForSlot(*category,c.currentSpell):std::nullopt;
    const auto &name=names.names[id.value_or(76)];
    if(code=="SP" || code=="gems")return xeenDialogFormat(text.scalar("NOT_ENOUGH_TO_CAST"),{code=="SP"?text.table("STAT_NAMES").at(11):text.table("CONSUMABLE_NAMES")[1],name});
    if(code=="engaged")return xeenDialogFormat(text.scalar("CANT_CAST_WHILE_ENGAGED"),{name});
    return code;
}
}
namespace {
unsigned itemMode(XeenLearnedSpell spell) {return spell==XeenLearnedSpell::EnchantItem?6:spell==XeenLearnedSpell::RechargeItem?4:11;}
}
void XeenEventFlow::openSpellItems(XeenLearnedSpell spell,std::size_t member) {
    _spellItemsUi.emplace();auto &ui=*_spellItemsUi;ui.spell=spell;
    ui.selection.mode=XeenInventoryMode::Browse;ui.selection.source=member;
    ui.selection.sourceOwner=_party.party.activeRosterIds().at(member);
    ui.selection.category=spell==XeenLearnedSpell::RechargeItem?XeenInventoryCategory::Miscellaneous:XeenInventoryCategory::Weapons;
}
std::shared_ptr<const DialogInput> XeenEventFlow::spellItemsInput() const {
    DialogInput input;const auto &ui=*_spellItemsUi;
    if(!ui.error.empty())input.anyKey=input.anyClick=input.keyMouseWait=true;
    else if(!ui.waiting)input=xeenItemSpellInput(itemMode(ui.spell),ui.choosing);
    return std::make_shared<const DialogInput>(std::move(input));
}
IndexedFrame XeenEventFlow::drawSpellItems(const IndexedFrame &base) const {
    const auto &ui=*_spellItemsUi;
    auto frame=drawXeenItems(dosText(),base,_inventoryFont,_catalog,_party,ui.selection,drawDialogSprite,itemMode(ui.spell));
    if(ui.choosing)frame=drawXeenItemSelection(dosText(),frame,_inventoryFont,ui.spell==XeenLearnedSpell::EnchantItem?4:ui.spell==XeenLearnedSpell::RechargeItem?5:6,drawDialogSprite);
    if(!ui.error.empty())frame=drawXeenErrorScroll(frame,_inventoryFont,ui.error);
    return frame;
}
void XeenEventFlow::syncSpellItems() {
    if(!_spellItemsUi || !_spellItemsUi->waiting)return;
    bool done=false,failed=false;
    if(_encounter->combat()) {
        const auto cast=_encounter->combat()->cast();done=!cast || cast->phase==XeenCombatCastPhase::Result;
        failed=cast && done && cast->result.failed;
    } else if(_encounter->_casting) {done=_encounter->_casting->effectDone;failed=_encounter->_casting->failed;}
    else if(_encounter->_itemUse) {done=_encounter->_itemUse->effectDone;failed=_encounter->_itemUse->failed;}
    if(!done)return;
    if(failed) {
        auto &ui=*_spellItemsUi;const auto failure=std::string(dosText().scalar("SPELL_FAILED"));
        ui.error=ui.spell==XeenLearnedSpell::EnchantItem?xeenDialogFormat(dosText().scalar("NOT_ENCHANTABLE"),{failure}):
            ui.spell==XeenLearnedSpell::RechargeItem?xeenDialogFormat(dosText().scalar("NOT_RECHARGABLE"),{failure}):"\v012\t000\x03" "c"+failure;
        return;
    }
    _spellItemsUi.reset();
    if(_castingUi)_castingUi->phase=CastingUi::Phase::Settling;
    else if(inventoryOpen())closeInventory();
}
IndexedFrame XeenEventFlow::handleSpellItems(const PlayerAction &action,std::uint64_t input) {
    auto &ui=*_spellItemsUi;const auto key=castKey(action);
    if(!ui.error.empty()) {
        _spellItemsUi.reset();
        if(_castingUi)_castingUi->phase=CastingUi::Phase::Settling;
        else if(_encounter->combat())return handleCombatCasting(AcknowledgeAction{},input);
        else closeInventory();
        return renderEncounter();
    }
    if(ui.waiting)return frameCopy();
    auto &selection=ui.selection;
    if(key==InputKey::Escape && ui.choosing) {ui.choosing=false;return renderEncounter();}
    if(!ui.choosing && (key=='w' || key=='a' || key=='c' || key=='m')) {
        selection.category=key=='w'?XeenInventoryCategory::Weapons:key=='a'?XeenInventoryCategory::Armor:key=='c'?XeenInventoryCategory::Accessories:XeenInventoryCategory::Miscellaneous;
        selection.slot.reset();return renderEncounter();
    }
    std::optional<XeenSpellItemSelection> picked;
    if(key>='1' && key<='9') {
        const auto slot=key-'1';const auto &item=xeenInventoryItems(_party.party.member(_party.roster,selection.source),selection.category)->at(slot);
        if(selection.category==XeenInventoryCategory::Miscellaneous?!item.material:!item.id)return frameCopy();
        if(!ui.choosing) {selection.slot=selection.slot==unsigned(slot)?std::nullopt:std::optional<std::size_t>{unsigned(slot)};return renderEncounter();}
        picked=XeenSpellItemSelection{selection.category,unsigned(slot),item};
    } else if(key==(ui.spell==XeenLearnedSpell::EnchantItem?'e':ui.spell==XeenLearnedSpell::RechargeItem?'r':'g')) {
        if(!selection.slot) {ui.choosing=true;return renderEncounter();}
        picked=XeenSpellItemSelection{selection.category,*selection.slot,xeenInventoryItems(_party.party.member(_party.roster,selection.source),selection.category)->at(*selection.slot)};
    } else if(key!=InputKey::Escape)return frameCopy();
    ui.waiting=true;ui.choosing=false;
    if(_encounter->combat()) {
        const auto response=picked?PlayerAction{DialogKeyAction{2000+unsigned(picked->category)*9+unsigned(picked->slot)}}:PlayerAction{CancelInteractionAction{}};
        return handleCombatCasting(response,input);
    }
    if(!_encounter->respondSpellItem(_encounter->ticket(),picked,input,responseFrame())) {ui.waiting=false;return frameCopy();}
    return renderEncounter();
}
void XeenEventFlow::initializeCaster() {
    auto &ui=*_castingUi;
    if(_lastCaster && *_lastCaster<_party.party.size())ui.caster=*_lastCaster;
    else for(std::size_t i=_party.party.size();i>0;--i)if(_party.party.member(_party.roster,i-1).hasSpells){ui.caster=i-1;break;}
    _lastCaster=ui.caster;
}
std::shared_ptr<const DialogInput> XeenEventFlow::castingDialogInput() const {
    if(_spellItemsUi)return spellItemsInput();
    DialogInput input;
    if(_castingUi) {
        using P=CastingUi::Phase;const auto &ui=*_castingUi;
        if((ui.phase==P::Settling && _encounter->_casting && _encounter->_casting->information) || !ui.refusal.empty())input.anyKey=input.anyClick=input.keyMouseWait=true;
        else if(ui.phase==P::New)input=xeenSpellListInput(true);
        else if(ui.phase==P::Cast)input=xeenCastInput();
        else if(ui.phase==P::ChooseTarget)input=xeenSpellTargetInput();
        else if(ui.phase==P::ChooseElement)input=xeenElementInput();
    } else if(_encounter && _encounter->combat())if(const auto v=_encounter->combat()->cast()) {
        using P=XeenCombatCastPhase;
        if(!v->refusal.empty() || (v->phase==P::Result && (v->result.failed || v->information)))input.anyKey=input.anyClick=input.keyMouseWait=true;
        else if(v->phase==P::Learned)input=xeenSpellListInput(true,false);
        else if(v->phase==P::Confirm)input=xeenCastInput(false);
        else if(v->phase==P::PartyTarget)input=xeenSpellTargetInput();
        else if(v->phase==P::Element)input=xeenElementInput();
    }
    return std::make_shared<const DialogInput>(std::move(input));
}
IndexedFrame XeenEventFlow::drawCasting(const IndexedFrame &base) const {
    if(_spellItemsUi)return drawSpellItems(base);
    const auto underlay=drawXeenProtectionCorners(base,*_party.encounterContext,drawDialogSprite);
    const auto &names=*_encounter->journeySavePreimage().learnedNames;
    if(_castingUi) {
        using P=CastingUi::Phase;const auto &ui=*_castingUi;
        const auto &c=_party.party.member(_party.roster,ui.caster);
        if(ui.phase==P::Settling && _encounter->_casting && _encounter->_casting->information)return drawXeenSpellInformation(dosText(),base,_inventoryFont,*_encounter->_casting->information,drawDialogSprite);
        if(ui.phase==P::Settling)return ui.refusal.empty()?base:drawXeenErrorScroll(base,_inventoryFont,std::string(dosText().scalar("SPELL_FAILED")));
        if(ui.phase==P::ChooseElement) {
            auto sidebar=c;sidebar.currentSp=_encounter->_casting->originalSp;
            const auto cast=drawXeenCast(dosText(),underlay,_inventoryFont,sidebar,ui.caster,names,drawDialogSprite);
            return drawXeenElement(dosText(),cast,_inventoryFont,drawDialogSprite);
        }
        auto frame=ui.phase==P::ChooseTarget?drawXeenSpellTarget(dosText(),underlay,_inventoryFont):
            drawXeenCast(dosText(),underlay,_inventoryFont,c,ui.caster,names,drawDialogSprite);
        if(ui.phase==P::New)frame=drawXeenSpellList(dosText(),frame,_inventoryFont,c,names,ui.list,_party.monsterTreasure->gold,false,false,drawDialogSprite,_party.encounterContext->year);
        if(!ui.refusal.empty())frame=drawXeenErrorScroll(frame,_inventoryFont,refusal(dosText(),c,names,ui.refusal));
        return frame;
    }
    const auto observed=_encounter->combat()->cast();if(!observed)return base;const auto &v=*observed;
    using P=XeenCombatCastPhase;const auto &c=_party.roster.at(v.owner);
    if(v.phase==P::Result && v.information)return drawXeenSpellInformation(dosText(),base,_inventoryFont,*v.information,drawDialogSprite);
    if(v.phase==P::Result && v.result.failed)return drawXeenErrorScroll(base,_inventoryFont,std::string(dosText().scalar("SPELL_FAILED")));
    if(v.phase==P::Element) {
        if(v.quick)return drawXeenElement(dosText(),base,_inventoryFont,drawDialogSprite);
        unsigned portrait=0;for(unsigned i=0;i<v.participant;++i)portrait+=bool(_encounter->combat()->participants()&(1u<<i));
        auto sidebar=c;sidebar.currentSp=v.result.spBefore;
        const auto cast=drawXeenCast(dosText(),underlay,_inventoryFont,sidebar,portrait,names,drawDialogSprite);
        return drawXeenElement(dosText(),cast,_inventoryFont,drawDialogSprite);
    }
    if(v.phase!=P::Learned && v.phase!=P::Confirm && v.phase!=P::PartyTarget)return base;
    unsigned portrait=0;for(unsigned i=0;i<v.participant;++i)portrait+=bool(_encounter->combat()->participants()&(1u<<i));
    auto frame=v.phase==P::PartyTarget?drawXeenSpellTarget(dosText(),underlay,_inventoryFont):
        drawXeenCast(dosText(),underlay,_inventoryFont,c,portrait,names,drawDialogSprite);
    if(v.phase==P::Learned)frame=drawXeenSpellList(dosText(),frame,_inventoryFont,c,names,{v.top,v.selection},_party.monsterTreasure->gold,false,false,drawDialogSprite,_party.encounterContext->year);
    if(!v.refusal.empty())frame=drawXeenErrorScroll(frame,_inventoryFont,refusal(dosText(),c,names,v.refusal));
    return frame;
}
IndexedFrame XeenEventFlow::handleCombatCasting(const PlayerAction &action,std::uint64_t input) {
    const auto prepare=[&] {
        if(!_encounter->_learnedNamesProvider)throw std::runtime_error("Missing learned names provider");
        auto names=_encounter->_learnedNamesProvider();
        if(const auto cast=_encounter->combat()->cast();cast && cast->phase==XeenCombatCastPhase::Confirm) {
            const auto &c=_party.roster.at(cast->owner);const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
            const auto id=category?XeenLearnedSpellRules::spellForSlot(*category,c.currentSpell):std::nullopt;
            if(id && XeenLearnedSpellRules::target(*id)==XeenSpellTarget::Contact) {
                XeenMonsterAppearance appearance;appearance.projectile=XeenProjectileAppearance{false,0,0,0,{}};
                appearance.projectile->target=_encounter->combat()->selectedTarget();
                for(unsigned type=2;type<=(*id==18?5u:2u);++type) {
                    appearance.projectile->pow=xeenSpellProjectile(*XeenLearnedSpellRules::supported(*id),type);
                    if(!_encounterCompose(_ordinary.phase,appearance).frame.isValid())throw std::runtime_error("Invalid spell preflight");
                }
            }
        }
        return names;
    };
    if(!_encounter->respondCombatCast(action,input,responseFrame(),prepare))return frameCopy();
    if(const auto cast=_encounter->combat()->cast();cast && cast->phase==XeenCombatCastPhase::Items && !_spellItemsUi)
        openSpellItems(static_cast<XeenLearnedSpell>(cast->result.spell),*cast->itemMember);
    return renderEncounter();
}
IndexedFrame XeenEventFlow::handleCasting(const PlayerAction &action,std::uint64_t input) {
    if(!_castingUi || !_encounter->castingFrameCurrent(input,responseFrame()))return frameCopy();
    auto &ui=*_castingUi;using P=CastingUi::Phase;const auto key=castKey(action);
    if(!ui.refusal.empty()){ui.refusal.clear();return renderEncounter();}
    if(ui.phase==P::Settling) {
        if(_encounter->_casting && _encounter->_casting->information) {
            _encounter->journeySavePreimage().check();
            _encounter->_casting->information.reset();
            return renderEncounter();
        }
        return frameCopy();
    }
    if(key>=InputKey::F1 && key<InputKey::F1+_party.party.size() && ui.phase!=P::ChooseTarget && ui.phase!=P::ChooseElement) {
        ui.caster=key-InputKey::F1;_lastCaster=ui.caster;return renderEncounter();
    }
    if(ui.phase==P::New) {
        if(key==InputKey::Escape){ui.phase=P::Cast;ui.list.selection.reset();return renderEncounter();}
        const auto &c=_party.party.member(_party.roster,ui.caster);
        const auto rows=XeenCharacterRules::maxSp(c,{_party.encounterContext->year})?XeenLearnedSpellRules::rows(c):std::vector<std::size_t>{};
        ui.list.scroll(key,rows.size());
        if(const auto row=ui.list.row(key,rows.size()))ui.list.selection=*row;
        if((key=='s' || key==InputKey::Enter) && ui.list.selection && *ui.list.selection<rows.size()) {
            if(!_encounter->rememberSpell(_encounter->ticket(),ui.caster,rows[*ui.list.selection]))return frameCopy();
            ui.phase=P::Cast;ui.list.selection.reset();
        }
        return renderEncounter();
    }
    if(ui.phase==P::Cast) {
        if(key==InputKey::Escape) {
            if(!_encounter->cancelCasting(_encounter->ticket(),input,responseFrame()))return frameCopy();
            _castingUi.reset();return renderEncounter();
        }
        if(key=='n'){ui.phase=P::New;ui.list={};return renderEncounter();}
        if(key!='c')return frameCopy();
        const auto &c=_party.party.member(_party.roster,ui.caster);if(!c.canAct()){ui.refusal="disabled";return renderEncounter();}
        const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
        const auto selected=category?XeenLearnedSpellRules::spellForSlot(*category,c.currentSpell):std::nullopt;
        const auto id=selected.value_or(76);
        if(id==40) {
          XeenRestoreGuard::Providers providers(_encounter->journeySavePreimage(),_world);
          const auto jump=xeenJumpPreflight(_world,_camera);_encounter->journeySavePreimage().check();
          if(jump.admission==XeenJumpAdmission::Unsupported) {ui.refusal="Spell not supported yet";return renderEncounter();}
        }
        using Q=XeenQuickSpellCheck;const auto admission=XeenLearnedSpellRules::check(c,c.currentSpell,_party.monsterTreasure->gems,false);
        if(admission!=Q::Supported) {
            ui.refusal=admission==Q::NoSpell?"":admission==Q::InsufficientSp?"SP":admission==Q::InsufficientGems?"gems":"Spell not supported yet";
            return renderEncounter();
        }
        if((id==44 || id==71 || id==69) && !_travelChoice &&
            beginTravelDialog(*XeenLearnedSpellRules::supported(id),c.rosterId,false))return renderEncounter();
        if(XeenLearnedSpellRules::target(id)==XeenSpellTarget::Contact) {
          XeenRestoreGuard::Providers providers(_encounter->journeySavePreimage(),_world);
          for(unsigned type=2;type<=(id==18?5u:2u);++type) {
           XeenMonsterAppearance appearance;appearance.projectile=XeenProjectileAppearance{false,0,0,3,{},xeenSpellProjectile(*XeenLearnedSpellRules::supported(id),type)};
           if(!_encounterCompose(_ordinary.phase,appearance).frame.isValid())throw std::runtime_error("Invalid spell projectile preflight");
          }
          _encounter->journeySavePreimage().check();
        }
        if(!_encounter->confirmCasting(_encounter->ticket(),ui.caster,c.currentSpell,input,responseFrame(),_travelChoice.value_or(0))) {
            if(!_encounter->_journeyRefusal.empty())ui.refusal=_encounter->_journeyRefusal;
            return renderEncounter();
        }
        if(XeenLearnedSpellRules::target(id)==XeenSpellTarget::Party || XeenLearnedSpellRules::target(id)==XeenSpellTarget::Relocation) {
            if(!_encounter->publishAwaken(_encounter->ticket()))throw std::logic_error("Awaken publication failed");
            ui.phase=P::Settling;
            if(_encounter->_casting->failed)ui.refusal="failed";
        } else if(XeenLearnedSpellRules::target(id)==XeenSpellTarget::Contact)ui.phase=P::Settling;
        else ui.phase=XeenLearnedSpellRules::target(id)==XeenSpellTarget::Element?P::ChooseElement:P::ChooseTarget;
        return renderEncounter();
    }
    if(ui.phase==P::ChooseElement) {
        const auto element=xeenElementSelection(key);
        if(key!=InputKey::Escape && !element)return frameCopy();
        if(!_encounter->respondCastingTarget(_encounter->ticket(),element,input,responseFrame()))return frameCopy();
        ui.phase=P::Settling;return renderEncounter();
    }
    if(ui.phase==P::ChooseTarget) {
        const bool escape=key==InputKey::Escape;
        if(!escape && !(key>=InputKey::F1 && key<InputKey::F1+_party.party.size()))return frameCopy();
        if(!_encounter->respondCastingTarget(_encounter->ticket(),escape?std::nullopt:std::optional<std::size_t>{key-InputKey::F1},input,responseFrame()))return frameCopy();
        if(!escape && XeenLearnedSpellRules::target(static_cast<std::uint8_t>(_encounter->_casting->spell))==XeenSpellTarget::Item) {
            openSpellItems(_encounter->_casting->spell,key-InputKey::F1);ui.phase=P::Items;
        } else {ui.phase=P::Settling;if(_encounter->_casting->failed)ui.refusal="failed";}
        return renderEncounter();
    }
    return frameCopy();
}
} // namespace mmodern
