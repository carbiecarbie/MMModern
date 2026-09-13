#ifndef MMODERN_EXPEDITION_TEST_SUPPORT_H
#define MMODERN_EXPEDITION_TEST_SUPPORT_H
#include "XeenCombatGameplayTestSupport.h"
namespace expedition_fixture {
using namespace combat_test;
inline XeenMap terrain(){auto m=combat_test::map();for(int y=13;y<=15;++y)for(int x=0;x<=8;++x){auto &c=m.geometry.cells[y*16+x];if(y==15&&(x==5||x==7)){c.rawWord=0;c.rawAttributes=c.flags=0x40;c.geometry=XeenOutdoorLayers{0,0,0,0};}else{c.rawWord=0x31;c.rawAttributes=c.flags=0;c.geometry=XeenOutdoorLayers{1,3,0,0};}}return m;}
inline XeenObjectFile objects(){auto o=combat_test::objects();o.entities.objectTable[1]=26;o.entities.objects.push_back({5,14,1,0,26});o.entities.monsters[9]={6,14,0,0,8};o.entities.monsters[17]=o.entities.monsters[18]={8,15,0,0,9};o.entities.monsters[25]={1,13,0,0,9};return o;}
inline XeenEventFile events(){auto e=combat_test::events();e.records.resize(16);const unsigned op[]{0x20,0x29,0x09,0x0c,0x0e};const std::vector<Bytes> args{{0,3},{0},{0x2c,1,3},{0,0,0x15,0x64},{}};for(unsigned i=1;i<=5;++i){auto &r=e.records[i];r.x=5;r.y=14;r.direction=4;r.line=i-1;r.opcode=op[i-1];r.parameters=args[i-1];const unsigned offsets[]{7,15,22,31,41};r.fileOffset=offsets[i-1];r.lengthField=5+r.parameters.size();}return e;}
inline std::vector<XeenMonsterRecord> monsters(){auto m=combat_test::statistics();auto &z=m[9];z=m[8];z.raw[16]=44;z.raw[17]=1;z.raw[20]=30;z.raw[22]=2;z.raw[23]=4;z.raw[24]=2;z.raw[28]=4;z.raw[30]=7;z.raw[31]=5;z.raw[47]=9;return m;}
}
#endif
