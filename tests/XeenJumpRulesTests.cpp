#include "games/xeen/XeenJumpRules.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "formats/xeen/XeenAssetSource.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
XeenMap outdoor() {
    XeenMap m;m.geometry.id=23;m.geometry.flags2=0x8000;m.geometry.surfaceTypes[1]=1;
    for(auto &cell:m.geometry.cells)cell.geometry=XeenOutdoorLayers{1,0,0,0};
    return m;
}
XeenMap indoor() {
    XeenMap m;m.geometry.id=28;
    for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{};
    return m;
}
void rules() {
    const XeenCamera c{23,9,11,XeenDirection::East};
    auto m=outdoor();
    const auto run=[&] {XeenWorld world([&](auto){return m;});return xeenJumpPreflight(world,c);};
    auto ready=run();check(ready.admission==XeenJumpAdmission::Ready && ready.destination.x==11 && ready.destination.y==11 && ready.destination.direction==c.direction,"two-cell forward Jump");
    check(c.x==9 && c.y==11,"preflight moved camera");
    for(const int offset:{1,2}) {
        m=outdoor();xeenGet<XeenOutdoorLayers>(m.geometry.cells[11*16+9+offset].geometry).middle=1;
        check(run().admission==XeenJumpAdmission::Blocked,"both outdoor walls block");
    }
    m=outdoor();xeenGet<XeenOutdoorLayers>(m.geometry.cells[11*16+11].geometry).surface=0;
    check(run().admission==XeenJumpAdmission::Blocked,"zero destination surface blocks");
    for(const auto type:{5,6,10,13,15}) {
        m=outdoor();m.geometry.surfaceTypes[2]=type;xeenGet<XeenOutdoorLayers>(m.geometry.cells[11*16+11].geometry).surface=2;
        check(run().admission==(type==15?XeenJumpAdmission::Unsupported:XeenJumpAdmission::Ready),"Hazardous Jump uses shared arrival; ordinary space stays outside admitted footprint");
        xeenGet<XeenOutdoorLayers>(m.geometry.cells[11*16+10].geometry).middle=1;
        check(run().admission==XeenJumpAdmission::Blocked,"original wall failure precedes terrain support");
    }
    m=outdoor();
    // Destination is ordinary terrain, isolated from the saved mainland by
    // an original middle layer Jump itself does not block.
    for(int y=0;y<16;++y)xeenGet<XeenOutdoorLayers>(m.geometry.cells[y*16+10].geometry).middle=7;
    check(run().admission==XeenJumpAdmission::Unsupported,"destination outside saved walking component");
    m=indoor();const XeenCamera city{28,8,8,XeenDirection::East};
    const auto cityRun=[&] {XeenWorld world([&](auto){return m;});return xeenJumpPreflight(world,city);};
    check(cityRun().admission==XeenJumpAdmission::Ready,"indoor Jump");
    for(const int offset:{0,1}) {
        m=indoor();xeenGet<XeenIndoorWalls>(m.geometry.cells[8*16+8+offset].geometry).walls[1]=1;
        check(cityRun().admission==XeenJumpAdmission::Blocked,"both indoor directional walls block even below walk threshold");
    }
    m=indoor();m.geometry.surfaceTypes[1]=13;m.geometry.cells[8*16+10].surfaceIndex=1;
    check(cityRun().admission==XeenJumpAdmission::Ready,"Indoor cloud Jump reaches shared arrival preflight");
    m=indoor();XeenWorld world([&](auto){return m;});
    check(xeenJumpPreflight(world,{28,0,8,XeenDirection::West}).admission==XeenJumpAdmission::Blocked,"missing indoor ahead blocks original lookup");
    check(world.sessionState().exploredTiles().empty() && world.sessionState().barriers().empty(),"preflight mutated world overlays");
}
void original(const char *root) {
    const auto install=XeenInstallationDetector().detect(root);check(bool(install),"original installation required");
    XeenAssetSource assets(*install,320,200);XeenWorld world([&](auto id){return XeenMapLoader().loadGeometryMap(assets,id);});
    check(xeenJumpPreflight(world,{23,11,11,XeenDirection::East}).admission==XeenJumpAdmission::Unsupported,"original Jump onto space is precharge unsupported");
    check(xeenJumpPreflight(world,{23,9,11,XeenDirection::East}).admission==XeenJumpAdmission::Ready,"original mainland successful Jump");
    check(world.sessionState().exploredTiles().empty(),"original preflight changed exploration");
}
}
int main(int argc,char **argv) {try {rules();if(argc>1)original(argv[1]);std::cout<<"Jump original wall checks, terrain/save-area preflight and read-only behavior passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
