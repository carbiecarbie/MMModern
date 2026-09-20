#pragma once
// Test-only literal rule oracle, independently expressed from ScummVM Xeen
// combat.cpp/interface_scene.cpp at 6814ee9ba54582f5b5adcffab49efbbd8f589edd.
// Never used by production and never replaces live-resource Flow witnesses.
#include "games/xeen/XeenStateEquality.h"
#include <algorithm>
#include <stdexcept>
namespace regional_test {
using namespace mmodern;
inline bool visible(const XeenCamera &c,const XeenActor &a) {
 const int dx=a.x-c.x,dy=a.y-c.y;
 const int forward[]={dy,dx,-dy,-dx};
 const int lateral[]={dx,-dy,-dx,dy};
 const int f=forward[unsigned(c.direction)],s=lateral[unsigned(c.direction)];
 constexpr int widths[]{0,1,1,2};
 return f>=0 && f<=3 && std::abs(s)<=widths[f];
}
inline bool terrain(const XeenMap &m,int x,int y) {
 if(x<0 || y<0 || x>=16 || y>=16)throw std::runtime_error("Oracle unsupported local terrain");
 const auto l=std::get<XeenOutdoorLayers>(m.geometry.cells[y*16+x].geometry);
 constexpr bool ordinary[]{true,false,true,true,true,true,true,false,true,false,false,true,false,true,true,false};
 const auto s=m.geometry.surfaceTypes[l.surface];
 return ordinary[l.middle] ? s!=0 && s!=8 && s!=15 : l.middle<=m.geometry.difficulties[0];
}
inline bool ray(const XeenMap &m,const XeenCamera &c,const XeenActor &a) {
 if(a.x!=c.x && a.y!=c.y)return false;
 constexpr bool transparent[]{true,false,true,false,true,true,false,false,true,false,false,true,false,true,true,false};
 const int sx=(a.x>c.x)-(a.x<c.x),sy=(a.y>c.y)-(a.y<c.y);
 for(int x=c.x+sx,y=c.y+sy;x!=c.x || y!=c.y;x+=sx,y+=sy) {
  const auto &cell=m.geometry.cells[y*16+x];
  if(sx==1 ? bool(cell.rawWord&8) : !transparent[std::get<XeenOutdoorLayers>(cell.geometry).middle])return false;
  if(x==a.x && y==a.y)break;
 }
 return true;
}
struct Oracle {
 std::vector<XeenActor> actors;
 XeenCamera camera;
 unsigned minutes=480,ctr24=0,pending=0;
 bool stopped=false;
 Oracle(std::vector<XeenActor> a,XeenCamera c):actors(std::move(a)),camera(c) {activate();}
 void activate(){for(auto &a:actors)a.activated=a.activated || visible(camera,a);}
 bool move(const XeenMap &map) {
  auto next=actors;std::array<bool,19> moved{};
  for(unsigned pass=0;pass<2;++pass)for(int y=camera.y+3;y>=camera.y-3;--y)for(int x=camera.x-3;x<=camera.x+3;++x)
   for(unsigned i=0;i<next.size();++i) {
    auto &a=next[i];if(a.x!=x || a.y!=y || !a.activated || moved[i])continue;
    if(a.original.resourceId==6 && (a.x!=camera.x || a.y!=camera.y) && ray(map,camera,a))return false;
    const int tx=(camera.x>x)-(camera.x<x),ty=(camera.y>y)-(camera.y<y);
    std::pair<int,int> first,second;
    if(unsigned(camera.direction)%2==0){first={tx,tx?0:ty};second={ty?0:tx,ty};}
    else {first={ty?0:tx,ty};second={tx,tx?0:ty};}
    auto dest=first;
    if(!terrain(map,x+dest.first,y+dest.second)){dest=second;if(!terrain(map,x+dest.first,y+dest.second))continue;}
    const int nx=x+dest.first,ny=y+dest.second;
    if(std::count_if(next.begin(),next.end(),[&](const auto &v){return v.x==nx && v.y==ny;})>=3)continue;
    a.x=nx;a.y=ny;moved[i]=true;
   }
  actors=std::move(next);return true;
 }
 void action(char input) {
  if(input=='L')camera.direction=static_cast<XeenDirection>((unsigned(camera.direction)+3)%4);
  else if(input=='R')camera.direction=static_cast<XeenDirection>((unsigned(camera.direction)+1)%4);
  else {constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};camera.x+=dx[unsigned(camera.direction)];camera.y+=dy[unsigned(camera.direction)];minutes+=10;pending=3;}
  ctr24=(ctr24+1)%24;
 }
 void pulse(const XeenMap &map) {
  if(pending && --pending==0 && !move(map)){stopped=true;return;}
  activate();
 }
 void compare(const XeenWorld &w,const XeenCamera &c,const XeenGameplayContext &ctx)const {
  if(!xeen_state::sameCamera(camera,c) || minutes!=ctx.minutes || ctr24!=ctx.ctr24 || actors.size()!=w.sessionState().actors().size())throw std::runtime_error("Reference camera/time/actor count mismatch");
  for(unsigned i=0;i<actors.size();++i)if(!xeen_state::sameActor(actors[i],w.sessionState().actors()[i]))throw std::runtime_error("Reference complete actor mismatch at record "+std::to_string(i));
 }
};
}
