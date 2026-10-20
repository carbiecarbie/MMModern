#ifndef MMODERN_XEEN_DIALOG_VIEW_H
#define MMODERN_XEEN_DIALOG_VIEW_H
#include "games/xeen/XeenTempleHeal.h"
#include "games/xeen/XeenTempleHeal.h"
#include "games/xeen/XeenInventoryState.h"
#include "games/xeen/XeenMapIdentity.h"
#include "games/xeen/XeenTextRenderer.h"
#include "core/DialogInput.h"
#include "formats/xeen/XeenDosText.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <functional>
#include <array>
#include <string_view>
namespace mmodern {
using XeenDialogSpriteDraw = std::function<void(IndexedFrame &,const char *,unsigned,int,int)>;
struct XeenSpellInformation;
IndexedFrame drawXeenSpellInformation(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const XeenSpellInformation &,const XeenDialogSpriteDraw &);


struct XeenTreasureLine {
    std::string recipient, itemName;
    bool quest=false;
};
// Display values copied from existing delivery records, with no mutation owner.
struct XeenTreasureDisplay {
	enum class Stage { Pages, Blank, Found, Final };
    std::uint32_t gold=0,gems=0;
    std::vector<XeenTreasureLine> items;
    bool fullPacks=false;
	Stage stage=Stage::Pages;
};
std::string xeenTreasureText(const XeenDosText &,const XeenTreasureDisplay &,std::size_t page);
IndexedFrame drawXeenTreasure(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const XeenTreasureDisplay &,std::size_t page,bool warning=false);
// Pure Clouds death composition. Playback, sound gates and return routing
// belong to M63; these functions are not connected to the live defeat screen.
IndexedFrame drawXeenDeathFire(const IndexedFrame &savedGameplay,unsigned evenFrame,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenDeathFinal(const IndexedFrame &,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenDeathLaugh(const IndexedFrame &savedFinal,std::optional<unsigned> frame,const XeenDialogSpriteDraw &);
struct XeenCombatPanelRow {
    std::string name;
    int hp=0, maxHp=0;
};
unsigned xeenCombatHealthColor(int hp,int maxHp);
std::string xeenCombatPanelText(const XeenDosText &,const std::array<XeenCombatPanelRow,3> &);
IndexedFrame drawXeenCombatPanel(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const std::array<XeenCombatPanelRow,3> &,std::optional<unsigned>,const XeenDialogSpriteDraw &);
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
enum class XeenJournalPage { Items, Quests, Notes, Awards };
// Display snapshot only. Darkside counters/quests are zero in the Clouds owner.
struct XeenQuestDisplay {
    std::array<std::uint32_t,85> items{};
    std::array<bool,57> quests{};
    std::array<bool,129> notes{};
};
XeenQuestDisplay xeenQuestDisplay(const XeenPartyState &);
struct XeenJournalUi {
    XeenJournalPage page=XeenJournalPage::Items;
    std::size_t top=0;
    void key(unsigned,std::size_t count);
};
std::vector<std::string> xeenJournalStrings(const std::vector<std::uint8_t> &,bool awards);
std::vector<std::string> xeenQuestLines(const XeenDosText &,const XeenQuestDisplay &,
    XeenJournalPage,const std::vector<std::string> &notes);
std::vector<std::string> xeenAwardLines(const XeenDosText &,const XeenCharacter &,
    const std::vector<std::string> &awards);
DialogInput xeenJournalInput(bool awards);
std::string xeenJournalText(const XeenDosText &,const XeenJournalUi &,
    const std::vector<std::string> &,const XeenCharacter *member=nullptr);
IndexedFrame drawXeenJournal(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const XeenJournalUi &,const std::vector<std::string> &,const XeenDialogSpriteDraw &,
    const XeenCharacter *member=nullptr);
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
DialogInput xeenElementInput();
IndexedFrame drawXeenProtectionCorners(const IndexedFrame &,const XeenGameplayContext &,const XeenDialogSpriteDraw &);
std::optional<unsigned> xeenElementSelection(unsigned key);
IndexedFrame drawXeenElement(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,const XeenDialogSpriteDraw &);
DialogInput xeenSpellConfirmInput(bool info=false);
IndexedFrame drawXeenCast(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const XeenCharacter &,std::size_t,const XeenLearnedSpellNames &,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenSpellList(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,
    const XeenCharacter &,const XeenLearnedSpellNames &,const XeenSpellListUi &,std::uint32_t gold,
    bool guild,bool info,const XeenDialogSpriteDraw &,std::uint32_t year=610,XeenMapIdentity map=XeenMapIdentity(28));
IndexedFrame drawXeenSpellConfirm(const IndexedFrame &,const XeenFontFormat &,const std::string &,bool info,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenSpellTarget(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &);
std::vector<std::string> xeenSpellDescriptions(const std::vector<std::uint8_t> &);
IndexedFrame drawXeenExchange(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenQuickFight(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,const XeenCharacter &,std::size_t,const XeenDialogSpriteDraw &);
DialogInput xeenItemsInput(bool misc,bool selection=false);
DialogInput xeenItemSpellInput(unsigned mode,bool selection=false);
enum class XeenLocationDialog { Smith, Training, Temple, Tavern, Bank, Guild };
// Missing action implementations shared by runtime feedback and coverage tools.
// Conditional eligibility (for example no cursed items) remains with the caller.
std::optional<std::string> xeenLocationUnsupportedAction(XeenLocationDialog,unsigned key);
bool xeenGuildMember(const XeenPartyState &,std::size_t,XeenMapIdentity);
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
DialogInput xeenBuyInput(XeenItemService);
std::string xeenLocationText(const XeenDosText &text,XeenLocationDialog,const XeenPartyState &,std::size_t,
    XeenMapIdentity,bool templeBlessed=false,std::string_view tavernStatus={});
std::string xeenTempleText(const XeenDosText &,const XeenPartyState &,std::size_t,
    const XeenTempleSurcharges &,bool templeBlessed=false);
std::string xeenNotEnoughGold(const XeenDosText &text);
// Party::notEnough (0x1e8bb): consumable 0 gold, 1 gems, 2 food, in the party.
std::string xeenNotEnough(const XeenDosText &text,unsigned consumable);
std::string xeenServiceConfirm(const XeenDosText &text,bool repair,const std::string &,std::uint32_t);
std::string xeenServiceConfirm(const XeenDosText &,XeenItemService,const std::string &,std::uint32_t);
std::string xeenIdentifiedDetails(const XeenDosText &,const XeenCharacter &,XeenInventoryCategory,const XeenItem &,const XeenLearnedSpellNames &);
IndexedFrame drawXeenIdentifiedDetails(const IndexedFrame &,const XeenFontFormat &,const std::string &);
std::uint32_t xeenBuyDisplayCost(XeenInventoryCategory,const XeenItem &);
IndexedFrame drawXeenLocation(const XeenDosText &text,const IndexedFrame &,const IndexedFrame &,const XeenFontFormat &,
    XeenLocationDialog,const XeenPartyState &,std::size_t,XeenMapIdentity,const XeenDialogSpriteDraw &,bool templeBlessed=false,std::string_view tavernStatus={},
    const XeenTempleSurcharges *templePrices=nullptr);
IndexedFrame drawXeenTavernPopup(const IndexedFrame &,const XeenFontFormat &,const std::string &);
IndexedFrame drawXeenBuy(const XeenDosText &text,const IndexedFrame &,const XeenFontFormat &,const XeenItemCatalog &,
    const XeenPartyState &,const XeenInventorySelection &,bool repair,const XeenDialogSpriteDraw &);
IndexedFrame drawXeenBuy(const XeenDosText &,const IndexedFrame &,const XeenFontFormat &,const XeenItemCatalog &,
    const XeenPartyState &,const XeenInventorySelection &,XeenItemService,const XeenDialogSpriteDraw &,XeenMapIdentity map=XeenMapIdentity(28));
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
    const XeenPartyState &,const XeenInventorySelection &,const XeenDialogSpriteDraw &,unsigned mode=0);
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
