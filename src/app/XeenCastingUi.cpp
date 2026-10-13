#include "app/XeenEventFlow.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenJumpRules.h"
#include <algorithm>

namespace mmodern {
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
void XeenEventFlow::initializeCaster() {
    auto &ui=*_castingUi;
    if(_lastCaster && *_lastCaster<_party.party.size())ui.caster=*_lastCaster;
    else for(std::size_t i=_party.party.size();i>0;--i)if(_party.party.member(_party.roster,i-1).hasSpells){ui.caster=i-1;break;}
    _lastCaster=ui.caster;
}
std::shared_ptr<const DialogInput> XeenEventFlow::castingDialogInput() const {
    DialogInput input;
    if(_castingUi) {
        using P=CastingUi::Phase;const auto &ui=*_castingUi;
        if(!ui.refusal.empty())input.anyKey=input.anyClick=input.keyMouseWait=true;
        else if(ui.phase==P::New)input=xeenSpellListInput(true);
        else if(ui.phase==P::Cast)input=xeenCastInput();
        else if(ui.phase==P::ChooseTarget)input=xeenSpellTargetInput();
    } else if(_encounter && _encounter->combat())if(const auto v=_encounter->combat()->cast()) {
        using P=XeenCombatCastPhase;
        if(!v->refusal.empty() || (v->phase==P::Result && v->result.failed))input.anyKey=input.anyClick=input.keyMouseWait=true;
        else if(v->phase==P::Learned)input=xeenSpellListInput(true,false);
        else if(v->phase==P::Confirm)input=xeenCastInput(false);
        else if(v->phase==P::PartyTarget)input=xeenSpellTargetInput();
    }
    return std::make_shared<const DialogInput>(std::move(input));
}
IndexedFrame XeenEventFlow::drawCasting(const IndexedFrame &base) const {
    const auto &names=*_encounter->journeySavePreimage().learnedNames;
    if(_castingUi) {
        using P=CastingUi::Phase;const auto &ui=*_castingUi;
        const auto &c=_party.party.member(_party.roster,ui.caster);
        if(ui.phase==P::Settling)return ui.refusal.empty()?base:drawXeenErrorScroll(base,_inventoryFont,std::string(dosText().scalar("SPELL_FAILED")));
        auto frame=ui.phase==P::ChooseTarget?drawXeenSpellTarget(dosText(),base,_inventoryFont):
            drawXeenCast(dosText(),base,_inventoryFont,c,ui.caster,names,drawDialogSprite);
        if(ui.phase==P::New)frame=drawXeenSpellList(dosText(),frame,_inventoryFont,c,names,ui.list,_party.monsterTreasure->gold,false,false,drawDialogSprite,_party.encounterContext->year);
        if(!ui.refusal.empty())frame=drawXeenErrorScroll(frame,_inventoryFont,refusal(dosText(),c,names,ui.refusal));
        return frame;
    }
    const auto observed=_encounter->combat()->cast();if(!observed)return base;const auto &v=*observed;
    using P=XeenCombatCastPhase;const auto &c=_party.roster.at(v.owner);
    if(v.phase==P::Result && v.result.failed)return drawXeenErrorScroll(base,_inventoryFont,std::string(dosText().scalar("SPELL_FAILED")));
    if(v.phase!=P::Learned && v.phase!=P::Confirm && v.phase!=P::PartyTarget)return base;
    unsigned portrait=0;for(unsigned i=0;i<v.participant;++i)portrait+=bool(_encounter->combat()->participants()&(1u<<i));
    auto frame=v.phase==P::PartyTarget?drawXeenSpellTarget(dosText(),base,_inventoryFont):
        drawXeenCast(dosText(),base,_inventoryFont,c,portrait,names,drawDialogSprite);
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
                appearance.projectile->pow=xeenSpellProjectile(*XeenLearnedSpellRules::supported(*id));
                if(!_encounterCompose(_ordinary.phase,appearance).frame.isValid())throw std::runtime_error("Invalid Arrow preflight");
            }
        }
        return names;
    };
    if(!_encounter->respondCombatCast(action,input,responseFrame(),prepare))return frameCopy();
    return renderEncounter();
}
IndexedFrame XeenEventFlow::handleCasting(const PlayerAction &action,std::uint64_t input) {
    if(!_castingUi || !_encounter->castingFrameCurrent(input,responseFrame()))return frameCopy();
    auto &ui=*_castingUi;using P=CastingUi::Phase;const auto key=castKey(action);
    if(!ui.refusal.empty()){ui.refusal.clear();return renderEncounter();}
    if(ui.phase==P::Settling)return frameCopy();
    if(key>=InputKey::F1 && key<InputKey::F1+_party.party.size() && ui.phase!=P::ChooseTarget) {
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
        if(XeenLearnedSpellRules::target(id)==XeenSpellTarget::Contact) {
          XeenRestoreGuard::Providers providers(_encounter->journeySavePreimage(),_world);
          XeenMonsterAppearance appearance;appearance.projectile=XeenProjectileAppearance{false,0,0,3,{},xeenSpellProjectile(*XeenLearnedSpellRules::supported(id))};
          if(!_encounterCompose(_ordinary.phase,appearance).frame.isValid())throw std::runtime_error("Invalid spell projectile preflight");
          _encounter->journeySavePreimage().check();
        }
        if(!_encounter->confirmCasting(_encounter->ticket(),ui.caster,c.currentSpell,input,responseFrame()))return frameCopy();
        if(XeenLearnedSpellRules::target(id)==XeenSpellTarget::Party || XeenLearnedSpellRules::target(id)==XeenSpellTarget::Relocation) {
            if(!_encounter->publishAwaken(_encounter->ticket()))throw std::logic_error("Awaken publication failed");
            ui.phase=P::Settling;
            if(_encounter->_casting->failed)ui.refusal="failed";
        } else if(XeenLearnedSpellRules::target(id)==XeenSpellTarget::Contact)ui.phase=P::Settling;
        else ui.phase=P::ChooseTarget;
        return renderEncounter();
    }
    if(ui.phase==P::ChooseTarget) {
        const bool escape=key==InputKey::Escape;
        if(!escape && !(key>=InputKey::F1 && key<InputKey::F1+_party.party.size()))return frameCopy();
        if(!_encounter->respondCastingTarget(_encounter->ticket(),escape?std::nullopt:std::optional<std::size_t>{key-InputKey::F1},input,responseFrame()))return frameCopy();
        ui.phase=P::Settling;if(_encounter->_casting->failed)ui.refusal="failed";return renderEncounter();
    }
    return frameCopy();
}
} // namespace mmodern
