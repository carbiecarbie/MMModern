#ifndef MMODERN_XEEN_INN_DOOR_H
#define MMODERN_XEEN_INN_DOOR_H
#include "games/xeen/XeenNavigation.h"
#include <optional>
namespace mmodern {
inline std::optional<XeenCamera> xeenInnDoor(XeenMapIdentity town) {
    if(town.side==XeenSide::Darkside) {
        switch(town.number) {
        case 29:return XeenCamera{town,17,24,XeenDirection::West};
        case 31:return XeenCamera{town,14,13,XeenDirection::East};
        case 37:return XeenCamera{town,9,4,XeenDirection::South};
        default:return {};
        }
    }
    switch(town.number) {
    case 28:return XeenCamera{town,18,4,XeenDirection::West};
    case 30:return XeenCamera{town,19,16,XeenDirection::West};
    case 49:return XeenCamera{town,11,12,XeenDirection::West};
    default:return {};
    }
}
inline bool xeenAtInnDoor(const XeenCamera &camera) {
    const auto door=xeenInnDoor(camera.mapId);
    return door && camera.x==door->x && camera.y==door->y && camera.direction==door->direction;
}
}
#endif
