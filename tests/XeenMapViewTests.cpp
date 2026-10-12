#include "games/xeen/XeenMapView.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "formats/xeen/XeenAssetSource.h"
#include "XeenDosTextTestSupport.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool v,const char *m) {if(!v)throw std::runtime_error(m);}
XeenMap indoor(XeenMapIdentity id) {
 XeenMap m;m.geometry.id=id.number;m.side=id.side;
 for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{};
 return m;
}
bool has(const std::vector<XeenMapDrawCommand> &c,const char *r,unsigned f,int x,int y) {
 return std::any_of(c.begin(),c.end(),[&](const auto &v){return v.resource==r && v.frame==f && v.x==x && v.y==y;});
}
void masks() {
 auto m=indoor(1);auto &cell=m.geometry.cells[8*16+8];cell.stepped=true;cell.surfaceIndex=2;cell.rawWord=0x1003;
 m.geometry.surfaceTypes[2]=6;
 XeenWorld w([m](auto){return m;});XeenCamera c{1,8,8,XeenDirection::East};
 const auto off=xeenMinimapCommands(w,c,false);
 check(off.size()==1 && has(off,"global.icn",5,232,9),"off must draw original logo");
 const auto commands=xeenMinimapCommands(w,c,true);
 check(has(commands,"town.til",0,267,36),"explored default terrain placement");
 check(has(commands,"town.til",42,272,41),"surface overlay placement");
 check(has(commands,"town.til",18,267,36) && has(commands,"town.til",23,267,36),"north/west wall frames");
 check(has(commands,"town.til",1,237,12) && !has(commands,"town.til",1,267,36),"unexplored masks");
 const auto big=xeenMapCommands(w,c);
 check(has(big,"town.til",0,150,94) && !has(big,"town.til",1,150,94),"shared exploration mask");
 check(has(big,"town.til",52,76,30),"indoor map frame sprite");
 w.markEncounterSession(XeenEncounterEntry::Journey);auto opened=w.transitionCandidate();
 opened->setBarrier({1,8,8,XeenDirection::North},13,true);
 const auto door=xeenMinimapCommands(*opened,c,true);
 check(has(door,"town.til",16,267,36) && !has(door,"town.til",18,267,36),"effective opened barrier rendering");
 check(w.map(1).geometry.cells[8*16+8].rawWord==0x1003,"map renderer changed original barrier");
 check(w.map(1).geometry.cells[0].stepped==false && w.map(1).geometry.cells[8*16+8].stepped,"drawing changed exploration");
 IndexedFrame frame(320,200,std::vector<std::uint8_t>(64000,7));
 const auto painted=drawXeenMinimap(w,c,frame,false,[](auto &f,const char *,unsigned,int,int){std::fill(f.pixels.begin(),f.pixels.end(),99);});
 check(painted.pixels[0]==7 && painted.pixels[9*320+223]==99 && painted.pixels[74*320+223]==7,"minimap outer-window clipping");
}
void eye() {
 auto m=indoor(28);m.geometry.cells[8*16+8].rawWord=0x1003;
 XeenWorld w([m](auto){return m;});XeenCamera c{28,8,8,XeenDirection::North};
 const auto reveal=xeenMinimapCommands(w,c,false,0,true);
 check(has(reveal,"town.til",0,267,36) && has(reveal,"town.til",18,267,36),"Eye reveals indoor terrain and walls");
 check(!has(reveal,"town.til",1,267,36) && !has(reveal,"global.icn",5,232,9),"Eye replaces logo and masks");
 const auto suppressed=xeenMinimapCommands(w,c,true,0,true);
 check(has(suppressed,"town.til",1,267,36),"automap suppresses Eye reveal");
 check(has(xeenMapCommands(w,c),"town.til",1,150,94),"full Map ignores Eye");
 check(xeenMinimapCommands(w,c,false).size()==1,"cleared Eye returns logo");
 check(w.sessionState().exploredTiles().empty() && !w.map(28).geometry.cells[8*16+8].stepped,"Eye writes no exploration");
 m.geometry.flags2=0x8000;m.geometry.surfaceTypes[1]=1;
 for(auto &cell:m.geometry.cells) {cell.geometry=XeenOutdoorLayers{1,0,0,0};cell.rawWord=1;}
 XeenWorld outside([m](auto){return m;});
 check(has(xeenMinimapCommands(outside,c,false,0,true),"outdoor.til",1,267,36),"Eye reveals outdoor terrain");
 check(!has(xeenMinimapCommands(outside,c,true,0,true),"outdoor.til",1,267,36),"outdoor automap suppresses Eye");
 check(outside.sessionState().exploredTiles().empty(),"outdoor Eye writes no exploration");
}
void seams() {
 auto a=indoor(1),b=indoor(2);a.geometry.neighbors[1]=2;b.geometry.neighbors[3]=1;
 b.geometry.cells[8*16].stepped=true;b.geometry.cells[8*16].rawWord=0x2000;
 XeenWorld w([=](auto id){return id.number==1?a:b;});XeenCamera c{1,15,8,XeenDirection::North};
 const auto commands=xeenMinimapCommands(w,c,true);
 check(has(commands,"town.til",0,277,36) && has(commands,"town.til",34,277,36),"indoor neighbor seam");
 auto v=xeenMapViewport(w,{1,0,0,XeenDirection::North});
 check(v.x==7 && v.y==8 && v.arrowX==4 && v.arrowY==133,"southwest map edge positioning");
 v=xeenMapViewport(w,{1,31,31,XeenDirection::North});
 check(v.x==23 && v.y==24 && v.arrowX==154 && v.arrowY==13,"32x32 map edge positioning and byte arrow");
}
void outdoor() {
 auto m=indoor(1);m.geometry.flags2=0x8000;
 for(auto &cell:m.geometry.cells)cell.geometry=XeenOutdoorLayers{};
 auto &cell=m.geometry.cells[8*16+8];cell.geometry=XeenOutdoorLayers{2,3,4,0};cell.rawWord=0x432;cell.stepped=true;
 m.geometry.surfaceTypes[2]=5;m.geometry.wallTypes[3]=7;m.geometry.surfaceTypes[15]=15;
 XeenWorld w([m](auto){return m;});XeenCamera c{1,8,8,XeenDirection::West};
 const auto commands=xeenMinimapCommands(w,c,true);
 check(has(commands,"outdoor.til",5,267,36) && has(commands,"outdoor.til",23,267,36) && has(commands,"outdoor.til",36,267,36),"outdoor three terrain layers");
 check(has(commands,"global.icn",4,267,36),"outdoor direction marker");
 const auto edge=xeenMinimapCommands(w,{1,0,0,XeenDirection::North},true);
 check(has(edge,"outdoor.til",15,237,12),"outdoor edge space rendering");
 check(!has(xeenMapCommands(w,c,0),"global.icn",4,160,94),"Map marker starts off");
 const auto p=xeenMapViewport(w,c);check(has(xeenMapCommands(w,c,4),"global.icn",4,p.arrowX+76,p.arrowY+25),"Map marker blinks after original counter boundary");
}
void original(const char *root) {
 const auto install=XeenInstallationDetector().detect(root);check(bool(install),"original installation required");XeenAssetSource assets(*install,320,200);
 XeenWorld w([&](auto id){return XeenMapLoader().loadGeometryMap(assets,id);});
 const auto font=XeenFontFormat(assets.readArchiveResource("fnt"));
 const auto draw=[&](IndexedFrame &f,const char *r,unsigned n,int x,int y){assets.drawDialogSprite(f,r,n,x,y);};
 assets.loadPalette("mm4.pal");assets.loadRawFramebuffer("back.raw");const auto base=assets.snapshot();
 for(const auto c:{XeenCamera{28,18,4,XeenDirection::West},XeenCamera{23,9,11,XeenDirection::North}}) {
  const auto name=assets.mapName(c.mapId);check(!name.empty() && name.size()<=32,"bounded original map name");
  const auto big=drawXeenMap(w,c,base,font,assets.uiText(),draw,4,name);
  const auto mini=drawXeenMinimap(w,c,base,true,draw,2);
  check(big.isValid() && mini.isValid() && big.pixels!=base.pixels,"original mapping renderer");
  check(w.sessionState().barriers().empty(),"render wrote barrier overlay");
 }
}
}
int main(int argc,char **argv) {try {masks();eye();seams();outdoor();if(argc>1)original(argv[1]);std::cout<<"Map masks, layers, seams, edges, markers, clipping and original resource rendering passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
