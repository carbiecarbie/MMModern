#ifndef MMODERN_SAVE_GAMEPLAY_TEST_SUPPORT_H
#define MMODERN_SAVE_GAMEPLAY_TEST_SUPPORT_H
#include "XeenSaveTestSupport.h"
#include "XeenRemoveTestSupport.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "games/xeen/XeenPartyLoader.h"
#include "platform/XeenSaveFile.h"
#include <map>
namespace gameplay_test {
using namespace mmodern;
using save_test::Bytes; using save_test::check; using save_test::sameSnapshot;
using remove_test::record;
inline Bytes fontBytes(){Bytes b(XeenFontFormat::kMinimumSize);for(int c=0;c<128;++c){b[0x1000+c]=6;b[0x1080+c]=3;for(int y=0;y<8;++y)b[c*16+y*2]=0x55;}return b;}
struct Fixture {
 XeenFontFormat font{fontBytes()};
 XeenPartyState initial;
 XeenSaveResourceSignature signature{{99,88},{}};
 std::map<XeenMapIdentity,std::vector<XeenEventRecord>> scripts;
 bool automatic=false, failCompose=false, invalidFrame=false;
 unsigned eventReads=0,mapReads=0,objectReads=0,compositions=0;
 XeenEventFlow *flow=nullptr;
 XeenWorld *world=nullptr;
 std::vector<IndexedFrame> frames;
 XeenEventTextFile text{1,"test.txt",true,{"Title","Message"}};
 Fixture(){
  Bytes roster(30*XeenCharacter::kSerializedSize),party(782);
  party[0]=party[1]=2;party[2]=0;party[3]=1;for(int i=4;i<10;++i)party[i]=255;
  initial=XeenPartyLoader().loadFromResources(roster,party);
  initial.roster.at(0).currentHp=10; initial.roster.at(1).currentHp=10;
  scripts[1]={record(1,1,0,0x12)};scripts[2]={record(1,1,0,0x12)};
 }
 XeenGameplayServices services(){return {
  {signature,[&]{return initial;},[&](XeenMapIdentity id){++eventReads;return XeenEventFile{id,"test.evt",true,scripts.at(id)};}},
  []{return XeenGameFlags{};},
  [&](XeenMapIdentity id){++mapReads;auto m=remove_test::map(id);m.geometry.cells[17].rawAttributes=automatic?0x10:0;m.geometry.surfaceTypes[0]=1;return m;},
  [&](XeenMapIdentity id){++objectReads;XeenObjectFile o{id,"test.mob",true,{}};o.entities.objects={{1,1,0,0,111},{2,1,0,0,112}};return o;},
  [&](XeenMapIdentity id){auto t=text;t.mapId=id;return t;},font,
  [&](XeenWorld &w,const XeenPartyState &p,const XeenCamera &c){
   if(failCompose)throw std::runtime_error("injected first-frame failure");
   ++compositions;w.map(c.mapId);world=&w;
   IndexedFrame f;f.width=320;f.height=200;f.pixels.resize(64000);
   f.pixels[0]=c.mapId.number;f.pixels[1]=c.x;f.pixels[2]=c.y;f.pixels[3]=static_cast<unsigned>(c.direction);
   f.pixels[4]=p.questItems.at(17);f.pixels[5]=p.questFlags.isSet(2);f.pixels[6]=p.roster.at(0).currentHp;
   f.pixels[10]=w.isObjectDisabled({c.mapId,0});f.pixels[11]=w.isObjectDisabled({c.mapId,1});
   if(invalidFrame)f.width=0;frames.push_back(f);return f;
  },[](IndexedFrame &f,std::uint8_t,std::size_t){f.pixels[100]=77;},
  [&](XeenEventFlow &f,const XeenCamera &){flow=&f;f.reportAutomatic=[](const auto &r){if(std::holds_alternative<XeenEventExecutionError>(r))throw std::runtime_error("automatic error");};},{}
 };}
 XeenSaveSnapshot saved(){auto s=save_test::sample();s.resources=signature;s.camera={1,1,1,XeenDirection::North};s.characters=initial.roster.characters();s.activeRosterIds={0,1};s.questItems.fill(0);s.questFlags.fill(false);s.gameFlags.fill(false);s.disabledObjects.clear();s.disabledEvents.clear();return s;}
};
}
#endif
