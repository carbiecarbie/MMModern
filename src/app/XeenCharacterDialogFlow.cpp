#include "app/XeenEventFlow.h"
#include "games/xeen/CloudsUiComposer.h"
#include <algorithm>
namespace mmodern {
std::optional<std::size_t> XeenEventFlow::dialogMember(std::size_t index) const {
    const auto *combat=_encounter?_encounter->combat():nullptr;
    if(!combat) return index<_party.party.size()?std::optional<std::size_t>{index}:std::nullopt;
    return CloudsUiComposer::partyMemberAtSlot(_party,combat->participants(),index);
}
void XeenEventFlow::dialogError(std::string message) {
    _dialogError=std::move(message);
    if(reportText) reportText(*_dialogError);
}
std::shared_ptr<const DialogInput> XeenEventFlow::characterDialogInput() const {
    if(!inventoryOpen()) return {};
    DialogInput input;
    if(_statPopup || _dialogError) {input.anyKey=true;input.anyClick=true;}
    else if(_inventory.mode==XeenInventoryMode::UseTarget) {
        constexpr int x[]{10,45,81,117,153,189};
        for(unsigned i=0;i<6;++i) {input.hits.push_back({x[i],150,x[i]+32,182,InputKey::F1+i});input.keys.push_back(InputKey::F1+i);}
        input.keys.push_back(InputKey::Escape);
    } else if(_sheet && !_itemsVisible) input=xeenSheetInput();
    else input=xeenItemsInput(_inventory.category==XeenInventoryCategory::Miscellaneous,_itemOption.has_value());
    return std::make_shared<const DialogInput>(std::move(input));
}
IndexedFrame XeenEventFlow::drawCharacterDialog(const IndexedFrame &base) const {
    if(_inventory.mode==XeenInventoryMode::UseTarget) return drawXeenItemTarget(base,_inventoryFont);
    auto frame=_sheet && !_itemsVisible?
        drawXeenSheet(base,_inventoryFont,_party,_inventory.source,_sheet->cursor,_sheet->blink,drawDialogSprite):
        drawXeenItems(base,_inventoryFont,_catalog,_party,_inventory,drawDialogSprite);
    if(_statPopup) frame=drawXeenPopup(frame,_inventoryFont,*_statPopup);
    if(_itemOption) frame=drawXeenItemSelection(frame,_inventoryFont,*_itemOption,drawDialogSprite);
    if(_dialogError) frame=drawXeenErrorScroll(frame,_inventoryFont,*_dialogError);
    return frame;
}
IndexedFrame XeenEventFlow::handleCharacterDialog(const PlayerAction &action) {
    if(_dialogError || _statPopup) {
        _dialogError.reset();_statPopup.reset();
        drawInventory();return _frame;
    }
    unsigned key=0;
    if(const auto *dialog=std::get_if<DialogKeyAction>(&action)) key=dialog->key;
    else if(const auto *member=std::get_if<SelectMemberAction>(&action)) key=InputKey::F1+member->partyIndex;
    else if(std::holds_alternative<CancelInteractionAction>(action)) key=InputKey::Escape;
    else if(std::holds_alternative<AcknowledgeAction>(action)) key=InputKey::Enter;
    if(!_sheet || _itemsVisible) return handleInventory(key?PlayerAction{DialogKeyAction{key}}:action);
    if(key>=InputKey::F1 && key<InputKey::F1+6) {
        if(const auto member=dialogMember(key-InputKey::F1)) {
            advanceInventoryEpoch();_inventory.source=*member;_inventory.sourceOwner=_party.party.activeRosterIds()[*member];
            _inventory.slot.reset();_inventory.record={};
        }
    } else if(key==InputKey::Escape) {closeInventory();_frame=_inventoryUnderlay;return _frame;}
    else if(key==InputKey::Up && _sheet->cursor>0) --_sheet->cursor;
    else if(key==InputKey::Down && _sheet->cursor<20) ++_sheet->cursor;
    else if(key==InputKey::Left && _sheet->cursor>=5) _sheet->cursor-=5;
    else if(key==InputKey::Right && _sheet->cursor<=15) _sheet->cursor+=5;
    else if(key==InputKey::Enter || (key>=1001 && key<=1020)) {
        if(key>=1001) _sheet->cursor=key-1001;
        // The original permits the cursor to move to cell 20, which has no
        // glyph. Avoid its expandStat assertion while retaining navigation.
        if(_sheet->cursor<20) {
            if(_sheet->cursor==14) dialogError("Awards: not supported yet");
            else _statPopup=xeenSheetPopup(_party,_inventory.source,_sheet->cursor);
        }
    } else if(key=='i') {
        _itemsVisible=true;advanceInventoryEpoch();_inventory.category=XeenInventoryCategory::Weapons;
        _inventory.slot.reset();_inventory.record={};
    } else if(key=='q') dialogError("Quick Reference: not supported yet");
    else if(key=='e') dialogError(_encounter && _encounter->combat()?std::string(xeenDialogText(XeenDialogText::ExchangingInCombat)):"Exchange: not supported yet");
    drawInventory();return _frame;
}
void XeenEventFlow::performItemOption(unsigned option) {
    if(!validInventorySource(true) || !_inventory.slot) return;
    if(option<2) {
        // M47 Part A: maintainer-approved viewing only during combat. Combat
        // equipment publication remains future Tier A work (milestone plan).
        if(_encounter && _encounter->combat()) {dialogError("Equipment in combat: not supported yet");return;}
        handleEquipment(option==0?XeenEquipmentOperation::Equip:XeenEquipmentOperation::Remove);return;
    }
    if(option==3) {dialogError("Discard: not supported yet");return;}
    if(option!=2) return;
    const auto &c=_party.roster.at(*_inventory.sourceOwner);
    if(_encounter && _encounter->combat()) {
        dialogError(std::string(xeenDialogText(XeenDialogText::UseInCombat)));return;
    }
    if(_camera.mapId.number==0) {dialogError(std::string(xeenDialogText(XeenDialogText::Hurry)));return;}
    if(!c.canAct()) {dialogError(xeenDialogFormat(xeenDialogText(XeenDialogText::InNoCondition),{c.name}));return;}
    if(!_inventory.record.id || (_inventory.record.state&0xc0) || !(_inventory.record.state&63)) {
        dialogError(xeenDialogFormat(xeenDialogText(XeenDialogText::NoSpecialAbilities),{_catalog.describe(_inventory.category,_inventory.record).displayName}));return;
    }
    if(!journey() || !XeenAntidoteUse::eligible(_inventory.record)) {dialogError("Item effect: not supported yet");return;}
    const auto certificate=_equipmentSelection;
    if(!certificate || !validEquipmentSelection(*certificate)) {invalidateInventorySelection();return;}
    XeenEncounterFlow::ItemUseSelection selection;
    selection.epoch=certificate->epoch;selection.membership=certificate->membership;
    selection.membershipSize=certificate->membershipSize;selection.sourceIndex=certificate->sourceActiveIndex;
    selection.sourceOwner=certificate->resolvedOwner;selection.category=certificate->category;
    selection.slot=certificate->physicalSlot;selection.record=certificate->selectedRecord;
    const auto generation=_encounter->beginItemUse(_encounter->ticket(),selection,_inventoryLease,_certificateLease);
    if(!generation) {invalidateInventorySelection();dialogError("Item use: not supported yet");return;}
    advanceInventoryEpoch();_itemUseGeneration=*generation;_inventory.mode=XeenInventoryMode::UseTarget;
    _sheet.reset();_itemsVisible=true;
}
}
