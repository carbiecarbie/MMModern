/* Derived from ScummVM InterfaceMinimap and MapDialog, revision
 * 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
 * Copyright ScummVM developers. GPL-3.0-or-later; see LICENSE.
 */
#include "games/xeen/XeenMapView.h"
#include "games/xeen/XeenWorld.h"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace mmodern {
namespace {
constexpr XeenTextRect full{0,0,320,200}, mini{223,9,317,74};
constexpr XeenTextRect outer{70,20,250,183}, inner{78,28,242,175};
struct Lookup {bool valid=false,stepped=false,outside=false;unsigned word=0,surface=0;const XeenMapGeometry *geometry=nullptr;};
Lookup lookup(XeenWorld &world,const XeenCamera &camera,int x,int y,bool eye=false) {
    const auto &primary=world.map(camera.mapId).geometry;
    if(x < -16 || y < -16 || x>=32 || y>=32)return {};
    const auto sample=world.sampleCell(camera.mapId,x,y);
    if(!sample)return primary.isOutdoors()?Lookup{true,true,true,15,0,&primary}:Lookup{};
    const auto &cell=*sample->cell;
    const unsigned surface=primary.isOutdoors()?unsigned(xeenGet<XeenOutdoorLayers>(cell.geometry).surface):unsigned(cell.surfaceIndex);
    // mazeLookup treats sky/space as visible without writing stepped bits.
    const auto type=primary.surfaceTypes[surface];
    return {true,bool(cell.stepped)||eye||type==10||type==15,false,unsigned(cell.rawWord),surface,sample->geometry};
}
unsigned layer(const Lookup &v,unsigned shift,unsigned mask=15) {
    // Beyond an outdoor edge mazeLookup returns SPACE itself, independent of shift.
    return v.outside?15:((v.word>>shift)&mask);
}
std::string tiles(const XeenMapGeometry &g) {
    if(g.isOutdoors())return "outdoor.til";
    constexpr std::array<const char *,6> names{"town.til","cave.til","towr.til","cstl.til","dung.til","scfi.til"};
    if(unsigned(g.wallKind)>=names.size())throw std::runtime_error("Invalid map tile wall kind");
    return names[g.wallKind];
}
int wallFrame(unsigned v,unsigned animation,bool top=false,bool leftEdge=false) {
    constexpr int frames[]{-1,18,34,22,16,2,30,32,2,20,28,14,4,16,24,26};
    if(v>=std::size(frames))return -1;
    // The partial left column omits grass (DOS table cs:0x1308 at 0x13ffe).
    if(leftEdge && v==2)return -1;
    int frame=frames[v];
    if(v==12)frame+=animation;
    return frame<0?frame:frame+int(top);
}
void append(std::vector<XeenMapDrawCommand> &out,const std::string &name,int f,int x,int y,bool small) {
    if(f>=0)out.push_back({name,unsigned(f),x,y,small?mini:full});
}
void outdoors(std::vector<XeenMapDrawCommand> &out,XeenWorld &world,const XeenCamera &camera,
        int cx,int cy,int size,int left,int top,bool small,const std::string &name,bool eye=false) {
    const int dx=small?3:7,dy=small?3:7;
    const auto &primary=world.map(camera.mapId).geometry;
    for(unsigned pass=0;pass<3;++pass)for(int row=0;row<size;++row)for(int col=0;col<size;++col) {
        const auto v=lookup(world,camera,cx-dx+col,cy+dy-row,eye);
        if(!v.valid || !v.stepped)continue;
        const auto value=layer(v,pass*4,pass==2?255:15);
        int frame;
        if(pass==0)frame=v.geometry->surfaceTypes[value];
        else if(pass==1) {frame=(small?primary:*v.geometry).wallTypes[value];if(!frame)continue;frame+=16;}
        else {if(!value)continue;frame=int(value)+32;}
        append(out,name,frame,left+col*10,top+row*8,small);
    }
}
void indoors(std::vector<XeenMapDrawCommand> &out,XeenWorld &world,const XeenCamera &camera,
        const XeenMapViewport &pt,int size,int left,int top,bool small,const std::string &name,std::uint64_t phase,bool eye=false) {
    const int dx=small?3:7,dy=small?3:7;
    const int animation=int((phase*2+(small?0:2))%8);
    const auto &primary=world.map(camera.mapId).geometry;
    // The original lookup returns 0x8888 for a missing cell, so a solid block whose
    // raw word is 0x8888 also skips ground and surface and gets the unexplored tile
    // (DOS 0x13db2, 0x13e30, 0x142fd); its walls still draw from the shifted word.
    const auto blank=[](const Lookup &v) {return !v.valid || v.word==0x8888;};
    const auto each=[&](const auto &draw) {
        for(int row=0;row<size;++row)for(int col=0;col<size;++col)
            draw(lookup(world,camera,pt.x-dx+col,pt.y+dy-row,eye),col,row);
    };
    each([&](const Lookup &v,int col,int row) {if(!blank(v) && v.stepped)append(out,name,0,left+col*10,top+row*8,small);});
    const auto surface=[&](int mx,int my,int xp,int yp) {
        const auto v=lookup(world,camera,mx,my,eye);
        if(!blank(v) && v.surface && v.stepped)append(out,name,36+primary.surfaceTypes[v.surface],xp,yp,small);
    };
    if(!small)for(int row=0;row<size;++row)surface(pt.x-8,pt.y+7-row,75,top+5+row*8);
    if(!small)surface(pt.x-8,pt.y+8,75,35);
    if(!small)for(int col=0;col<size;++col)surface(pt.x-7+col,pt.y+8,left+5+col*10,35);
    each([&](const Lookup &v,int col,int row) {
        if(!blank(v) && v.surface && v.stepped)append(out,name,36+primary.surfaceTypes[v.surface],left+5+col*10,top+5+row*8,small);
    });
    if(small) {
        surface(pt.x-4,pt.y+4,left-5,top-3);
        for(int row=0;row<size;++row)surface(pt.x-4,pt.y+3-row,left-5,top+5+row*8);
        for(int col=0;col<size;++col)surface(pt.x-3+col,pt.y+4,left+5+col*10,top-3);
    }
    // The partial left column and top row cover every row and column, bottom
    // row first (DOS 0x13fcc..0x1411c); the pin draws only one of each.
    for(int i=0;i<(small?7:15);++i) {
        const auto a=lookup(world,camera,pt.x-(small?4:8),pt.y-(small?3:8)+i,eye);
        if(a.valid && a.stepped)append(out,name,wallFrame(layer(a,12,small?65535:15),animation,false,true),small?left-15:70,top+(small?48:120)-i*8,small);
        const auto b=lookup(world,camera,pt.x-dx+i,pt.y+(small?4:8),eye);
        if(b.valid && b.stepped)append(out,name,wallFrame(layer(b,0),animation,true),left+i*10,top-8,small);
    }
    each([&](const Lookup &v,int col,int row) {
        // DOS draws the arrow before cell (4,4), over the party cell's own walls (0x1413c).
        if(small && col==4 && row==4)append(out,"global.icn",unsigned(camera.direction)+1,left+35,top+28,true);
        if(!small && ((phase?((phase-1)/3):0)&1) && pt.arrowX/10==col && 14-pt.arrowY/10==row)
            append(out,"global.icn",unsigned(camera.direction)+1,pt.arrowX+81,pt.arrowY+29,false);
        if(!v.valid || !v.stepped)return;
        append(out,name,wallFrame(layer(v,12,small?65535:15),animation),left+col*10,top+row*8,small);
        append(out,name,wallFrame(layer(v,0),animation,true),left+col*10,top+row*8,small);
    });
    each([&](const Lookup &v,int col,int row) {if(blank(v) || !v.stepped)append(out,name,1,left+col*10,top+row*8,small);});
}
void execute(IndexedFrame &frame,const std::vector<XeenMapDrawCommand> &commands,const XeenDialogSpriteDraw &draw) {
    if(!draw)return;
    for(const auto &c:commands) {
        auto painted=frame;draw(painted,c.resource.c_str(),c.frame,c.x,c.y);
        for(int y=std::max(0,c.clip.top);y<std::min(frame.height,c.clip.bottom);++y)
            for(int x=std::max(0,c.clip.left);x<std::min(frame.width,c.clip.right);++x)
                frame.pixels[y*frame.width+x]=painted.pixels[y*frame.width+x];
    }
}
IndexedFrame text(const IndexedFrame &base,const XeenFontFormat &font,const std::string &value,bool frame) {
    XeenTextRenderOptions o;o.originalControls=true;o.drawWindow=frame;o.windowBounds=outer;o.bounds=inner;o.x=inner.left;o.y=inner.top;
    auto r=XeenTextRenderer(font).render(base,value,o);
    if(r.pages.size()!=1 || !r.diagnostics.empty())throw std::runtime_error("Original Map layout overflow");
    return std::move(r.pages.front());
}
}
XeenMapViewport xeenMapViewport(XeenWorld &world,const XeenCamera &c) {
    const auto &g=world.map(c.mapId).geometry;
    XeenMapViewport p{c.x,c.y,74,69};
    if(c.x<8 && !g.neighbors[3]) {p.arrowX=c.x*10+4;p.x=7;}
    else if(c.x>23) {p.arrowX=std::uint8_t(c.x*10+100);p.x=23;}
    else if(c.x>8 && !g.neighbors[1]) {p.arrowX=std::uint8_t(c.x*10+4);p.x=7;}
    if(c.y<8 && !g.neighbors[2]) {p.arrowY=(15-c.y)*8+13;p.y=8;}
    else if(c.y>24) {p.arrowY=(31-c.y)*8+13;p.y=24;}
    else if(c.y>=8 && !g.neighbors[0]) {p.arrowY=(15-c.y)*8+13;p.y=8;}
    return p;
}
std::vector<XeenMapDrawCommand> xeenMapCommands(XeenWorld &world,const XeenCamera &camera,std::uint64_t phase) {
    std::vector<XeenMapDrawCommand> out;
    const auto &g=world.map(camera.mapId).geometry;const auto name=tiles(g);const auto p=xeenMapViewport(world,camera);
    if(g.isOutdoors()) {
        outdoors(out,world,camera,p.x,p.y,16,80,38,false,name);
        if((phase?((phase-1)/3):0)&1)append(out,"global.icn",unsigned(camera.direction)+1,p.arrowX+76,p.arrowY+25,false);
    } else {indoors(out,world,camera,p,16,80,38,false,name,phase);append(out,name,52,76,30,false);}
    return out;
}
std::vector<XeenMapDrawCommand> xeenMinimapCommands(XeenWorld &world,const XeenCamera &camera,bool on,std::uint64_t phase,bool eye) {
    std::vector<XeenMapDrawCommand> out;
    eye=eye && !on;
    if(!on && !eye) {append(out,"global.icn",5,232,9,true);return out;}
    const auto &g=world.map(camera.mapId).geometry;const auto name=tiles(g);
    if(g.isOutdoors()) {
        append(out,"global.icn",15,237,12,true);outdoors(out,world,camera,camera.x,camera.y,7,237,12,true,name,eye);
        append(out,"global.icn",unsigned(camera.direction)+1,267,36,true);
    } else indoors(out,world,camera,{camera.x,camera.y,0,0},7,237,12,true,name,phase,eye);
    append(out,"global.icn",6,223,3,true);return out;
}
IndexedFrame drawXeenMinimap(XeenWorld &world,const XeenCamera &camera,const IndexedFrame &base,bool on,
        const XeenDialogSpriteDraw &draw,std::uint64_t phase,bool eye) {
    auto result=base;execute(result,xeenMinimapCommands(world,camera,on,phase,eye),draw);return result;
}
IndexedFrame drawXeenMap(XeenWorld &world,const XeenCamera &camera,const IndexedFrame &base,const XeenFontFormat &font,
        const XeenDosText &dos,const XeenDialogSpriteDraw &draw,std::uint64_t phase,const std::string &mapName) {
    auto result=text(base,font,"",true);auto commands=xeenMapCommands(world,camera,phase);
    std::vector<XeenMapDrawCommand> final;
    if(!commands.empty() && ((!world.map(camera.mapId).geometry.isOutdoors()) || commands.back().resource=="global.icn")) {
        final.push_back(commands.back());commands.pop_back();
    }
    execute(result,commands,draw);
    // Window::frame follows tile drawing, while its fill happened on open.
    auto border=text(result,font,"",true);
    for(int y=inner.top;y<inner.bottom;++y)for(int x=inner.left;x<inner.right;++x)border.pixels[y*border.width+x]=result.pixels[y*result.width+x];
    execute(border,final,draw);
    const auto value=xeenDialogFormat(dos.scalar("MAP_TEXT"),{mapName,std::to_string(camera.x),std::to_string(camera.y),dos.table("DIRECTION_TEXT").at(unsigned(camera.direction))});
    return text(border,font,value,false);
}
}
