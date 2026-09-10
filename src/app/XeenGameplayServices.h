#ifndef MMODERN_APP_XEEN_GAMEPLAY_SERVICES_H
#define MMODERN_APP_XEEN_GAMEPLAY_SERVICES_H
#include "app/XeenEventFlow.h"
#include "games/xeen/XeenSaveState.h"
#include "platform/sdl/SdlWindow.h"
namespace mmodern {
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
};
}
#endif
