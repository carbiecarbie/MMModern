#ifndef MMODERN_TOOLS_XEEN_COVERAGE_SCREENS_H
#define MMODERN_TOOLS_XEEN_COVERAGE_SCREENS_H
// The sole manual support checklist. Class/file identifiers come from pinned
// ScummVM 6814ee9ba54582f5b5adcffab49efbbd8f589edd engines/mm/xeen/dialogs,
// interface.cpp, locations.cpp and worldofxeen/worldofxeen_menu.cpp.
namespace mmodern::coverage {
inline const char *screens() {
    return R"md(
## Screens — manual checklist

This is the one manual dimension, taken from the pinned ScummVM dialog set,
with the main screen, town locations and World/CD menu added from the same pin.
It is not inferred from original data and the guard cannot discover UI changes.
Update this checklist when a screen or button changes. A partial row names its
missing buttons or contexts. The list describes Clouds; Darkside has no gameplay
entry. Dialog identifiers below are reference identifiers, not commercial text.

| Reference dialog / screen | Status | Buttons and limitations | MMModern owner |
| --- | --- | --- | --- |
| Awards | supported | Sheet member only; nine lines, Up/Down arrows and keypad 8/2, mouse buttons, Exit | XeenCharacterDialogFlow |
| CopyProtection | unsupported | Version-specific word input; not a World CD gameplay requirement | — |
| CreateCharacterDialog | supported | Roll, Create, portrait, stats, class, exchange, name, Exit | XeenEventFlow (XeenCreateFlow.cpp) |
| Dismiss | supported | Member choice by F1-F6 or portrait, last-member refusal, Escape key and button | XeenEventFlow |
| PartyDialog | supported | Roster list, recruit 1-4, Up/Down, Delete, Remove, Create, F1-F6 sheet and Exit | XeenEventFlow (XeenInnFlow.cpp) |
| Quests | supported | Quest Items, Current Quests, Auto Notes; arrows/keypad, Page Up/Down, Home/End, mouse buttons, Exit; exploration and Items, including combat | XeenEventFlow |
| Choose123 / HowMuch | unsupported | Numeric Choose123 opcode 48 and TakeOrGive HowMuch unavailable | XeenEventInterpreter |
| SelectElement | supported | Fire, Electricity, Cold, Acid, cancel | XeenCastingUi |
| LloydsBeacon | supported | Set and Return across resource-backed Clouds primary maps; same-map Return retains actors | XeenCastingUi |
| Teleport | supported | Distance, cancel | XeenCastingUi |
| TownPortal | supported | Five towns, each town's run position, cancel and item use in combat | XeenCastingUi |
| IdentifyMonster / DetectMonsters | supported | Monster information, acknowledge | XeenCastingUi |
| Arena | supported | Battle Master level entry, Warzone, reward and exit | XeenEventFlow |
| PyramidLocation | unsupported | Switch sides | — |
| Reaper / Golem / Dwarf / Sphinx locations | supported | Keyed/refusal sequences, VOC speech and Effects-off captions | XeenEventFlow / XeenEntranceScene |
| OtherOptions / endings | partial | Clouds intro (C), Clouds ending (V after a win), Escape; Darkside intro (D) and the Darkside and World endings unavailable | XeenTitleFlow / XeenCutsceneFlow |
| Exploration main screen | supported | Movement, strafe, Shoot, Cast, Rest, Bash, Dismiss, Quests, Map, Info, Quick Ref, gem, portraits, Danger Sense/Spot Secret Doors/Direction Sense indicators | XeenEventFlow |
| Combat main screen | supported | Attack, Block, Run, Shoot effects, Cast, charged-item Use (including Town Portal), Quick Fight/options, targets, turns, Info, Quick Ref, skill indicators; Run uses the original map run position, including automatic Events after combat | XeenEncounterFlow |
| CharacterInfo | supported | Stats and popups, Items, Awards, Quick Ref, Exchange, Exit; original combat Exchange refusal | XeenCharacterDialogFlow |
| ControlPanel | supported | Save, Save As, Load, Quit, Mr Wizard (in combat too), Efx, Music, Speech, Text work | XeenEventFlow |
| ItemsDialog | supported | Categories, Equip, Remove, Discard, transfer, Use, Quest, Exit; free combat equipment/Discard/transfer, combat-party recipients, main-screen Use; original sheet Use refusal in combat; Recharge/Enchant/Gold supported | XeenInventoryFlow |
| Input / StringInput / NumericInput | partial | Save names, Bank amounts, Arena level, Event passwords and mirrors work; Choose123 and TakeOrGive HowMuch unavailable | XeenTitleFlow / XeenEventPresenter |
| SpellsDialog / CastSpell / SpellOnWho | supported | Select, Buy, Info, rows, scroll, Cast, New, member choice, cancel; all 76 Clouds spell effects | XeenCastingUi |
| Tavern | supported | Food, Drink, Tip, Rumors, member selection, Sign In and Exit work | XeenEventFlow |
| LocationMessage / NPC | partial | Acknowledge and Yes/No; other NPC modes unavailable | XeenEventPresenter |
| PleaseWait | unclassified | Reference helper; no separately identified equivalent screen | — |
| CreditsScreen | supported | Four original pages, acknowledge | XeenTitleFlow |
| DifficultyDialog | supported | Adventurer, Warrior | XeenTitleFlow |
| ExchangeDialog | supported | Member choice, Exit; original combat refusal | XeenCharacterDialogFlow |
| InfoDialog | supported | Date/time/effects, acknowledge | XeenCharacterDialogFlow |
| ItemSelectionDialog | supported | Rows, Exit for implemented item actions | XeenInventoryFlow |
| MapDialog | supported | Map, acknowledge | XeenEventFlow |
| MessageDialog / ErrorScroll / CantCast | supported | Message/refusal, acknowledge | XeenEventPresenter / XeenCastingUi |
| Confirm / YesNo | supported | Yes, No | XeenEventPresenter |
| QuickFight | supported | Next action, member, Exit; action effects retain combat restrictions | XeenEncounterFlow |
| QuickReferenceDialog | supported | Party summary, acknowledge | XeenCharacterDialogFlow |
| NotWhileEngaged | supported | Original combat refusal, acknowledge | XeenCastingUi |
| WhoWill | supported | Member choice, cancel | XeenEventPresenter |
| Bank | supported | Deposit, Withdraw, Gold, Gems, amount, Exit | XeenEventFlow |
| Temple | supported | Heal, Donation, Uncurse, member selection, Exit | XeenSmithFlow |
| Blacksmith | supported | Browse, Buy, Sell, Identify, Fix, Exit | XeenSmithFlow |
| Guild | supported | Buy Spells, Spell Info, member selection, Exit; catalogue and membership by map | XeenCastingUi |
| Training | supported | Train, member, Exit; cap by map | XeenTrainingFlow |
| World/CD main menu | supported | Start, Load, Credits, Other Options | XeenTitleFlow |
| Save/load chooser | supported | Slots, names, overwrite/cancel; original DOS presentation | XeenTitleFlow |

Reference caveats: ScummVM merges Save As into Save and uses its generic save
chooser. MMModern retains the DOS Save As button and original chooser. The
reference's control panel omits the CD Speech/Text toggles, included above from
MMModern's DOS UI. Abstract ButtonContainer, UIButton and SettingsBaseDialog are
not independent screens.
)md";
}
}
#endif
