#include "XeenTestInstallation.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenWorld.h"
#include <fstream>
#include <iostream>

using namespace mmodern;
int main(int argc,char **argv) {
 try {
  if(argc!=3)throw std::runtime_error("usage: darkside-scenery <installation> <frame.ppm>");
  const auto installation=xeenTestInstallationDetector().detect(argv[1]);
  if(!installation)throw std::runtime_error("Original installation absent");
  XeenAssetSource assets(*installation,320,200);XeenMapLoader loader;
  XeenWorld world([&](auto id){return loader.loadGeometryMap(assets,id);},[&](auto id){return loader.loadObjects(assets,id);});
  const XeenMapIdentity id{XeenSide::Darkside,53};
  const auto &map=world.map(id);
  if(map.geometry.isOutdoors() || map.geometry.surfaceTypes[6]!=10)throw std::runtime_error("Original tower edge no longer uses sky");
  const XeenCamera camera{id,8,15,XeenDirection::North};
  const auto commands=XeenIndoorScene().build(world,camera);
  unsigned sky=0;for(const auto &command:commands)if(const auto *geometry=std::get_if<XeenIndoorGeometryDraw>(&command.content))sky+=geometry->resourceName=="sky.srf";
  if(!sky)throw std::runtime_error("Tower edge omitted the original slot-six sky surface");
  auto party=XeenPartyLoader().loadInitialParty(assets);
  const auto frame=CloudsMapComposer().compose(assets,world,party,camera,{610},nullptr,0);
  std::ofstream out(argv[2],std::ios::binary);out<<"P6\n"<<frame.width<<' '<<frame.height<<"\n255\n";
  for(auto pixel:frame.pixels)out.write(reinterpret_cast<const char *>(frame.palette.data()+3*pixel),3);
  if(!out)throw std::runtime_error("Cannot write tower edge frame");
  std::cout<<"Darkside tower 53 (8,15) North: "<<sky<<" sky surface commands; frame "<<argv[2]<<'\n';
  return 0;
 }catch(const std::exception &e){std::cerr<<"Darkside scenery: "<<e.what()<<'\n';return 1;}
}
