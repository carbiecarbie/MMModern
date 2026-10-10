#ifndef MMODERN_APP_XEEN_GAMEPLAY_SERVICES_H
#define MMODERN_APP_XEEN_GAMEPLAY_SERVICES_H
#include "app/XeenEventFlow.h"
#include "games/xeen/XeenSaveState.h"
#include "platform/sdl/SdlWindow.h"
#include "app/XeenSession.h"
namespace mmodern {
class XeenTitleFlow;
// Borrowed resource/presentation providers for Application's one startup path.
// Production binds original assets and SDL; synthetic tests bind bounded fixtures.
// No live gameplay owner or saved snapshot is stored here.
struct XeenGameplayServices {
 XeenSaveState::Resources resources;
 std::function<XeenGameFlags()> initialFlags;
 XeenWorld::MapLoader maps;
 XeenWorld::ObjectLoader objects;
 XeenEventSystem::TextProvider texts;
 const XeenFontFormat &font;
 std::function<XeenEventFlow::Composition(XeenWorld &, const XeenPartyState &, const XeenCamera &, std::uint64_t)> compose;
 XeenEventPresenter::NpcDraw npcDraw;
 std::function<void(XeenEventFlow &, const XeenCamera &)> configureFlow;
 using Show = std::function<bool(const IndexedFrame &, const SdlWindow::FrameUpdateHandler &,
   const std::function<bool()> &, const SdlWindow::IdleFrameHandler &, const std::function<std::string()> &)>;
 Show show;
 // Optional synchronous acceptance observer. References are borrowed only for
 // playGameplay's lifetime. Party/flags are read-only; mutable world/events
 // permit cache invalidation and camera permits disclosed checkpoint positioning.
 // Never supplies startup state or handles a save request.
 std::function<void(XeenWorld &, XeenEventSystem &, const XeenPartyState &,
   XeenCamera &, const XeenGameFlags &)> observeGameplay;
 XeenEventPresenter::Clock clock = {};
 const XeenItemCatalog *catalog = nullptr;
 std::function<void(std::uint8_t)> validateEncounterSprite;
 std::function<void(std::uint8_t)> validateCombatSprite;
 std::function<XeenEventFlow::Composition(XeenWorld &, const XeenPartyState &, const XeenCamera &,
   std::uint64_t, XeenMonsterAppearance)> composeEncounter;
 enum class SaveStage { Capture, Preflight, Write };
 std::function<void(SaveStage)> observeSaveStage;
 // Optional deterministic sampling seam; called once for an unseeded fresh game or prepared Journey.
 std::function<std::uint32_t()> sampleJourneySeed;
 // Select original initialization; absent for prepared Journey and restore.
 std::optional<XeenDifficulty> originalStart;
 std::function<XeenCamera()> loadInitialCamera;
 // Title Load retains the validated value; it never reopens the selected path.
 std::shared_ptr<const XeenSaveSnapshot> restoreSnapshot;
 std::optional<std::string> saveName;
 std::optional<unsigned> currentSlot,initialSlot;
 // Explicit CLI loose target remains independent of the panel's current slot.
 std::optional<std::filesystem::path> developerSavePath;
 // Called with a scratch-validated New candidate before any presented frame.
 // Retry/cancel UI retains this candidate and performs no new initialization.
 std::function<bool(const XeenSaveSnapshot &,const std::function<void()> &)> publishInitial;
 std::function<std::unique_ptr<XeenTitleFlow>(const IndexedFrame &,bool,bool,bool,
   std::optional<unsigned>,const std::string &)> panel;
 std::function<void(unsigned,const XeenSaveSnapshot &,const std::function<void()> &)> writeManaged;
 XeenSessionOutcome *outcome=nullptr;
};
// Application's persistence transaction, also usable by internal domain tests.
// The target is already installation-checked by the caller. Refusal precedes
// providers and file work; no public Journey entry is implied by this seam.
void xeenSaveGameplay(const XeenGameplayServices &, XeenWorld &, XeenPartyState &,
	XeenCamera &, XeenGameFlags &, XeenEventFlow &, const std::filesystem::path &,
	const XeenSaveState::Preflight &, std::function<void()> *nestedSourceCheck = nullptr,
 const std::function<void(const XeenSaveSnapshot &,const std::function<void()> &)> &writer = {});
}
#endif
