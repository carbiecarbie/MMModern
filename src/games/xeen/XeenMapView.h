#ifndef MMODERN_XEEN_MAP_VIEW_H
#define MMODERN_XEEN_MAP_VIEW_H
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenNavigation.h"
#include <cstdint>
#include <string>
#include <vector>
namespace mmodern {
class XeenWorld;
struct XeenMapDrawCommand {
    std::string resource;
    unsigned frame;
    int x,y;
    XeenTextRect clip;
};
struct XeenMapViewport {int x,y,arrowX,arrowY;};
XeenMapViewport xeenMapViewport(XeenWorld &,const XeenCamera &);
std::vector<XeenMapDrawCommand> xeenMapCommands(XeenWorld &,const XeenCamera &,std::uint64_t phase=0);
std::vector<XeenMapDrawCommand> xeenMinimapCommands(XeenWorld &,const XeenCamera &,bool on,std::uint64_t phase=0,bool eye=false);
IndexedFrame drawXeenMap(XeenWorld &,const XeenCamera &,const IndexedFrame &,const XeenFontFormat &,
    const XeenDosText &,const XeenDialogSpriteDraw &,std::uint64_t phase,const std::string &mapName);
IndexedFrame drawXeenMinimap(XeenWorld &,const XeenCamera &,const IndexedFrame &,bool on,
    const XeenDialogSpriteDraw &,std::uint64_t phase=0,bool eye=false);
}
#endif
