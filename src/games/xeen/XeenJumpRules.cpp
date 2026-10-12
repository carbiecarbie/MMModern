/* Jump checks derived from ScummVM spells.cpp, revision
 * 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
 * Copyright ScummVM developers. GPL-3.0-or-later; see LICENSE.
 */
#include "games/xeen/XeenJumpRules.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenJourneyContent.h"
#include <stdexcept>
namespace mmodern {
XeenJumpPreflight xeenJumpPreflight(XeenWorld &world,const XeenCamera &camera) {
    constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
    const auto d=unsigned(XeenDirection(camera.direction));
    if(d>=4)throw std::invalid_argument("Invalid Jump direction");
    XeenJumpPreflight result{XeenJumpAdmission::Blocked,camera};
    const auto &map=world.map(camera.mapId);
    const int x=camera.x+2*dx[d],y=camera.y+2*dy[d];
    const auto ahead=world.sampleCell(camera.mapId,camera.x+dx[d],camera.y+dy[d]);
    const auto destination=world.sampleCell(camera.mapId,x,y);
    if(map.geometry.isOutdoors()) {
        if(ahead && xeenGet<XeenOutdoorLayers>(ahead->cell->geometry).middle==1)return result;
        if(destination) {
            const auto &layers=xeenGet<XeenOutdoorLayers>(destination->cell->geometry);
            if(layers.middle==1 || layers.surface==0)return result;
        }
        // Absent outdoor cells are original space, whose wall is not a blocker.
    } else {
        const auto current=world.sampleCell(camera.mapId,camera.x,camera.y);
        if(!current || !ahead || wallAt(*current->cell,XeenDirection(camera.direction)) ||
            wallAt(*ahead->cell,XeenDirection(camera.direction)))return result;
    }
    result.admission=XeenJumpAdmission::Unsupported;
    if(!destination)return result;
    const unsigned surface=map.geometry.isOutdoors() ?
        unsigned(xeenGet<XeenOutdoorLayers>(destination->cell->geometry).surface) : unsigned(destination->cell->surfaceIndex);
    const auto type=map.geometry.surfaceTypes.at(surface);
    // Maintainer-approved M60 restriction: terrain-arrival consequences are
    // deferred. Do not charge a spell whose destination cannot yet be handled.
    if(type==5 || type==6 || type==10 || type==13 || type==15)return result;
    if(map.geometry.isOutdoors()) {
        if(camera.mapId!=XeenMapIdentity(23) || destination->mapId!=XeenMapIdentity(23) ||
            x<0 || x>=16 || y<0 || y>=16 ||
            !XeenMovement::component(map,9,11,xeenJourneyContent().traversal)[y*16+x])return result;
        result.destination.mapId=destination->mapId;
        result.destination.x=destination->x;result.destination.y=destination->y;
    } else {
        if(camera.mapId!=XeenMapIdentity(28) || !xeenIndoorCoordinate(x,y))return result;
        result.destination.x=x;result.destination.y=y;
    }
    result.admission=XeenJumpAdmission::Ready;
    return result;
}
}
