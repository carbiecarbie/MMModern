#ifndef MMODERN_XEEN_DIALOG_VIEW_H
#define MMODERN_XEEN_DIALOG_VIEW_H
#include "games/xeen/XeenInventoryState.h"
#include "games/xeen/XeenTextRenderer.h"
#include "core/DialogInput.h"
#include "formats/xeen/XeenDosText.h"
#include "games/xeen/XeenLearnedSpellRules.h"
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
DialogInput xeenExchangeInput();
DialogInput xeenQuickFightInput();
struct XeenSpellListUi {
    std::size_t top=0;
    std::optional<std::size_t> selection;
    void scroll(unsigned key,std::size_t count);
    std::optional<std::size_t> row(unsigned key,std::size_t count) const;
};
DialogInput xeenCastInput(bool switchCaster=true);
DialogInput xeenSpellListInput(bool selecting,bool switchCaster=true);
DialogInput xeenSpellTargetInput();
DialogInput xeenSpellConfirmInput(bool info=false);
IndexedFrame drawXeenCast(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const XeenCharacter &,std::size_t,const XeenLearnedSpellNames &,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenSpellList(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const XeenCharacter &,const XeenLearnedSpellNames &,const XeenSpellListUi &,std::uint32_t gold,
    bool guild,bool info,const XeenDialogSpriteDraw &,std::uint32_t year=610);
IndexedFrame drawXeenSpellConfirm(const IndexedFrame &,const XeenFontFormat &,const std::string &,bool info,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenSpellTarget(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &);
std::vector<std::string> xeenSpellDescriptions(const std::vector<std::uint8_t> &);
IndexedFrame drawXeenExchange(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenQuickFight(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,const XeenCharacter &,std::size_t,const XeenDialogSpriteDraw &);
DialogInput xeenItemsInput(bool misc,bool selection=false);
enum class XeenLocationDialog { Smith, Training, Temple, Tavern, Bank, Guild };
bool xeenVertigoGuildMember(const XeenPartyState &,std::size_t);
DialogInput xeenLocationInput(XeenLocationDialog);
DialogInput xeenBankInput();
DialogInput xeenFoodConfirmInput();
std::string xeenBankText(const XeenDosText &,const XeenPartyState &,bool withdrawal,bool refresh=false);
IndexedFrame drawXeenBankCurrency(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const XeenPartyState &,bool withdrawal,const XeenDialogSpriteDraw &,bool refresh=false);
IndexedFrame drawXeenBankAmount(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const std::string &digits,const XeenDialogSpriteDraw &,char cursor='|');
IndexedFrame drawXeenFoodOffer(const IndexedFrame &,const XeenFontFormat &,const std::string &offer,const XeenDialogSpriteDraw &);
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
// Member indices are the caller's current combat participants, or active party.
std::string xeenQuickReferenceText(const XeenDosText &,const XeenPartyState &,const std::vector<std::size_t> &members);
XeenDialogPopup xeenInfoPopup(const XeenDosText &,const XeenGameplayContext &);
IndexedFrame drawXeenQuickReference(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const XeenPartyState &,const std::vector<std::size_t> &members);
XeenDialogPopup xeenSheetPopup(const XeenDosText &text,const XeenPartyState &,std::size_t,unsigned);
IndexedFrame drawXeenSheet(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &,const XeenPartyState &,
    std::size_t member,unsigned cursor,bool blink,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenItems(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &,const XeenItemCatalog &,
    const XeenPartyState &,const XeenInventorySelection &,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenPopup(const IndexedFrame &,const XeenFontFormat &,const XeenDialogPopup &);
IndexedFrame drawXeenErrorScroll(const IndexedFrame &,const XeenFontFormat &,const std::string &,bool startupColors=false);
IndexedFrame drawXeenConfirm(const IndexedFrame &,const XeenFontFormat &,const std::string &,
    bool large,const XeenDialogSpriteDraw &,bool startupColors=false,bool stopAtBottom=false);
IndexedFrame drawXeenItemTarget(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &);
std::string xeenBackpackFull(const XeenDosText &text,XeenInventoryCategory,const std::string &);
IndexedFrame drawXeenItemSelection(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &,unsigned,
    const XeenDialogSpriteDraw &);
}
#endif
