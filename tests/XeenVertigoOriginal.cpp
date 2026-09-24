// Read-only original-resource contract witness. Process traversal is separate.
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenVertigoRoute.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/CloudsMapComposer.h"
#include <iostream>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
}
int main(int argc,char **argv) {
 try {
  check(argc==2,"usage: mmodern_vertigo_original <original-installation>");
  const auto installation=XeenInstallationDetector().detect(argv[1]);
  check(installation && installation->hasDarkside(),"World of Xeen installation required");
  XeenAssetSource assets(*installation,320,200);XeenMapLoader maps;
  XeenEventLoader events([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
   if(!assets.hasInitialResource(name))return {};return assets.readInitialResource(name);
  });
  const auto mainland=events.load(23),city=events.load(28);
  xeenValidateVertigoRoute(mainland,city);
  const auto statistics=XeenMonsterFormat::parse(*assets.readCloudsMonsterStatisticsFromDarkArchive());
  const auto &slime=statistics.at(0);
  const auto resource=[&](const std::string &name) {
   return name.rfind("aaze",0)==0?assets.readArchiveResource(name):assets.readInitialResource(name);
  };
  XeenWorld manifestWorld([&](auto id){return maps.loadGeometryMap(assets,id);},
   [&](auto id){return maps.loadObjects(assets,id);});
  xeenValidateVertigoManifest(manifestWorld,city,statistics,resource);
  const auto reject=[&](auto &&operation) {bool failed=false;try {operation();}catch(const std::invalid_argument &){failed=true;}
   check(failed,"altered Vertigo immutable resource admitted");};
  for(const char *name:{"maze0028.dat","mazex109.dat","mazex110.dat","mazex111.dat","maze0028.mob","maze0028.evt","aaze0028.txt"}) {
   reject([&]{xeenValidateVertigoManifest(manifestWorld,city,statistics,[&](const std::string &requested){
    auto bytes=resource(requested);if(requested==name)bytes.at(bytes.size()/2)^=1;return bytes;
   });});
  }
  for(unsigned type:{0u,2u,73u}) {
   auto altered=statistics;altered.at(type).raw.at(20)^=1;
   reject([&]{xeenValidateVertigoManifest(manifestWorld,city,altered,resource);});
  }
  auto wrongCity=city;wrongCity.resourceName="maze0023.evt";
  reject([&]{xeenValidateVertigoManifest(manifestWorld,wrongCity,statistics,resource);});
  wrongCity=city;wrongCity.records.at(539).opcode=0;
  reject([&]{xeenValidateVertigoManifest(manifestWorld,wrongCity,statistics,resource);});
  XeenWorld alteredGeometry([&](auto id){auto map=maps.loadGeometryMap(assets,id);if(id==XeenMapIdentity(28))map.geometry.cells[0].rawAttributes^=1;return map;},
   [&](auto id){return maps.loadObjects(assets,id);});
  reject([&]{xeenValidateVertigoManifest(alteredGeometry,city,statistics,resource);});
  slime.validateSlime();
  assets.validateNormalMonster(0);assets.validateAttackMonster(0);
  const auto mob=maps.loadObjects(assets,28);
  const auto actors=XeenActorApproach::actorsFromResources(mob,statistics);
  check(actors.size()==46 && mob.entities.objects.size()==143,"original city MOB count");
  check(actors.at(35).original.x==15 && actors.at(35).original.y==4 &&
   actors.at(35).original.resourceId==0,"original entrance Slime slot");
  check(actors.at(36).original.resourceId==0,"original reset target Slime type");
  check(slime.raw[48]==1 && slime.raw[49]==0,"original loopAnimation/effect field provenance");
  // Artificial same-cell appearance fixture: original identities/statistics,
  // with other actors hidden to isolate MON/ATT pixels from geometry and objects.
  auto visualActors=actors;
  for(auto &actor:visualActors) actor.x=actor.y=-128;
  visualActors[35].x=15;visualActors[35].y=1;
  const XeenCamera visualCamera{28,15,1,XeenDirection::North};
  assets.loadPalette("mm4.pal");
  for(auto kind:{XeenMonsterSpriteKind::Normal,XeenMonsterSpriteKind::Attack})
  for(unsigned frame=0;frame<(kind==XeenMonsterSpriteKind::Normal?8u:4u);++frame)
  for(int phase=-1;phase<8;++phase) {
   XeenMonsterAppearance appearance{kind,static_cast<std::uint8_t>(frame)};
   appearance.identity=visualActors[35].id;
   const auto commands=XeenIndoorScene().buildActors(manifestWorld,visualCamera,visualActors,
    phase<0?std::nullopt:std::optional<std::uint64_t>(phase),appearance);
   check(commands.size()==1 && commands[0].actor()->identity==visualActors[35].id &&
    commands[0].actor()->kind==kind && commands[0].actor()->frame==frame &&
    commands[0].drawOptions().slimePalettePhase==-1,"Slime must retain native palette at every cosmetic phase");
   assets.loadRawFramebuffer("back.raw");const auto background=assets.snapshot();
   CloudsMapComposer().drawIndoorCommands(assets,commands);const auto actual=assets.snapshot();
   assets.loadRawFramebuffer("back.raw");
   XeenSpriteDrawOptions native;native.sceneClipped=true;native.bottomClipped=true;
   // Independent original setMonsterSprite placement and effect=0 oracle.
   assets.drawSprite(kind==XeenMonsterSpriteKind::Normal?"000.mon":"000.att",frame,-5,2,native);
   check(actual.pixels==assets.snapshot().pixels,"Slime pixels differ from original native MON/ATT");
   unsigned visible=0,green=0;
   for(unsigned pixel=0;pixel<actual.pixels.size();++pixel) if(actual.pixels[pixel]!=background.pixels[pixel]) {
    ++visible;const unsigned index=actual.pixels[pixel]*3;
    green+=actual.palette[index+1]>actual.palette[index] && actual.palette[index+1]>actual.palette[index+2];
   }
   check(visible>100 && green*2>visible,"Original Slime no longer has predominantly green appearance");
  }
  std::cout<<"Original Slime native MON/ATT palette across all frames and cosmetic phases passed\n";
  const auto chr=assets.readInitialResource("maze.chr");
  for(unsigned owner=0;owner<30;++owner) {
   const auto input=XeenCharacterFormat::parseCombatInputs(chr,owner,true,true,true);
   check(input.poisonResistance && input.poisonResistance->permanent==chr[354*owner+317] &&
    input.poisonResistance->temporary==chr[354*owner+318],"original poison supplement bytes");
  }
  // Artificial party/RNG controls against the checked original Slime record.
  XeenConsequenceCharacters characters;
  XeenConsequenceInputs inputs;
  for(unsigned owner=0;owner<6;++owner) {
   auto &c=characters[owner];c.rosterId=kXeenCombatOwners[owner];
   c.permanentLevel=3;c.birthYear=592;c.currentHp=50;
   c.intellect=c.personality=c.endurance={15,0};
   c.conditions[8]=1;
   inputs[owner].might=inputs[owner].speed=inputs[owner].accuracy={15,0};
   inputs[owner].luck=XeenAttributeValue{15,0};
   inputs[owner].resistances=XeenCombatResistances{};
   inputs[owner].poisonResistance=XeenAttributeValue{owner==0?80:0,0};
  }
  characters[5].conditions[13]=1;
  std::vector<XeenCombatRandom::Draw> tape;
  for(unsigned owner=0;owner<6;++owner) {
   tape.push_back({1,2,2});
   tape.push_back({1,owner==0?120u:40u,1});
   tape.push_back({1,owner==0?120u:40u,owner==0?1u:40u});
  }
  XeenCombatRandom rng(tape);
  XeenEnemyAttackCandidate attack(characters,inputs,slime,610,0x3f);
  bool done=false;
  for(unsigned step=0;step<30 && !done;++step){XeenConsequenceDraw budget{rng,1,{}};done=attack.service(budget);}
  check(done && rng.position()==18 && attack.result.injuryCount==6,
   "Slime two-save per-owner draw trace and bounded continuation");
  check(attack.result.damage==10 && attack.result.injuries[0].amount==0,
   "Successful repeated poison saves yield zero HP damage");
  for(unsigned owner=0;owner<6;++owner)
   check(!attack.characters[owner].conditions[8] && !attack.characters[owner].conditions[3] &&
    attack.result.injuries[owner].owner==kXeenCombatOwners[owner] &&
    attack.result.injuries[owner].amount==(owner==0?0:2),
    "Slime wakes and strikes all owners including a dead owner, without Poison condition");
  std::cout<<"Original M37 route, Slime, 46 city records, sprites and 30 poison inputs passed\n";
  return 0;
 } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
