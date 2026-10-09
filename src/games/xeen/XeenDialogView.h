#ifndef MMODERN_XEEN_DIALOG_VIEW_H
#define MMODERN_XEEN_DIALOG_VIEW_H
#include "games/xeen/XeenInventoryState.h"
#include "games/xeen/XeenTextRenderer.h"
#include "core/DialogInput.h"
#include "formats/xeen/XeenDosText.h"
#include <functional>
#include <string_view>
namespace mmodern {
using XeenDialogSpriteDraw = std::function<void(IndexedFrame &,const char *,unsigned,int,int)>;
enum class XeenDialogText {
    ExchangingInCombat, CursedItem, BackpackFull, NotProficient,
    EquippedAll, RemoveToEquip, Ring, Medal, InNoCondition,
    Hurry, UseInCombat, NoSpecialAbilities, CannotCastEngaged, WhichItem,
    PermanentlyDiscard, BuyForGold, ItemsTitle, MiscCategory, Charges, ItemNotBroken, PartyNeedsRest,
    RestComplete, PartyIsStarving, HitSpellPointsRestored, TooDangerousToRest, SomeCharsMayDie
};
std::string_view xeenDialogText(const XeenDosText &text,XeenDialogText);
std::string xeenDialogFormat(std::string_view,const std::vector<std::string> &);
DialogInput xeenSheetInput();
DialogInput xeenItemsInput(bool misc,bool selection=false);
enum class XeenLocationDialog { Smith, Training, Temple };
DialogInput xeenLocationInput(XeenLocationDialog);
DialogInput xeenBuyInput(bool repair=false);
std::string xeenLocationText(const XeenDosText &text,XeenLocationDialog,const XeenPartyState &,std::size_t);
std::uint32_t xeenTempleUncurseCost(const XeenCharacter &);
std::string xeenNotEnoughGold(const XeenDosText &text);
std::string xeenServiceConfirm(const XeenDosText &text,bool repair,const std::string &,std::uint32_t);
std::uint32_t xeenBuyDisplayCost(XeenInventoryCategory,const XeenItem &);
IndexedFrame drawXeenLocation(const XeenDosText &text,const IndexedFrame &,const IndexedFrame &,const XeenFontFormat &,
    XeenLocationDialog,const XeenPartyState &,std::size_t,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenBuy(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &,const XeenItemCatalog &,
    const XeenPartyState &,const XeenInventorySelection &,bool repair,const XeenDialogSpriteDraw &);
DialogInput xeenConfirmInput(bool large=false);
std::optional<bool> xeenConfirmAnswer(unsigned key);
struct XeenDialogPopup { std::string text; XeenTextRect bounds; };
XeenDialogPopup xeenSheetPopup(const XeenDosText &text,const XeenPartyState &,std::size_t,unsigned);
IndexedFrame drawXeenSheet(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &,const XeenPartyState &,
    std::size_t member,unsigned cursor,bool blink,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenItems(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &,const XeenItemCatalog &,
    const XeenPartyState &,const XeenInventorySelection &,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenPopup(const IndexedFrame &,const XeenFontFormat &,const XeenDialogPopup &);
IndexedFrame drawXeenErrorScroll(const IndexedFrame &,const XeenFontFormat &,const std::string &);
IndexedFrame drawXeenConfirm(const IndexedFrame &,const XeenFontFormat &,const std::string &,
    bool large,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenItemTarget(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &);
std::string xeenBackpackFull(const XeenDosText &text,XeenInventoryCategory,const std::string &);
IndexedFrame drawXeenItemSelection(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &,unsigned,
    const XeenDialogSpriteDraw &);
}
#endif
