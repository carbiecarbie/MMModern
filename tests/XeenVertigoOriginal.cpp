// Read-only original-resource content witness. Process traversal is separate.
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenActorApproach.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/CloudsMapComposer.h"
#include "platform/XeenSaveFile.h"
#include "games/xeen/XeenCharacterRules.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <fstream>

using namespace mmodern;
namespace {
void check(bool ok,const char *message) { if(!ok)throw std::runtime_error(message); }
void targetingOracle(const std::vector<XeenMonsterRecord> &records) {
 // Independent translation of pinned doMonsterTurn's selector. Include all
 // Clouds records even when their damage/ability is not admitted for gameplay.
 for(const auto &record:records)for(unsigned mask=0;mask<64;++mask)for(unsigned disabled:{0u,21u,63u}) {
  XeenConsequenceCharacters party;
  std::vector<unsigned> participating,able;
  for(unsigned i=0;i<6;++i) {
   party[i].characterClass=static_cast<XeenCharacterClass>(i);
   party[i].race=i%2?XeenRace::Dwarf:XeenRace::Human;
   if(disabled&(1u<<i))party[i].conditions[12+i%4]=1;
   else if(i==2)party[i].conditions[8]=1;
   if(mask&(1u<<i)){participating.push_back(i);if(!(disabled&(1u<<i)))able.push_back(i);}
  }
  unsigned expected=0;std::vector<XeenCombatRandom::Draw> tape;
  const unsigned hates=record.raw[25];
  if(hates==15)expected=mask;
  else if(!participating.empty()) {
   unsigned selected=6;
   if(hates!=1)for(auto i:able) {
    bool match=false;
    switch(hates) {
     case 0:case 2:case 3:case 4:case 5:case 6:case 7:case 8:case 9:
      match=unsigned(party[i].characterClass)==hates;break;
     case 12:match=party[i].race==XeenRace::Dwarf;break;
    }
    if(match){selected=i;break;}
   }
   if(selected==6) {tape.push_back({0,unsigned(participating.size()-1),0});selected=participating.front();}
   if(disabled&(1u<<selected)) {
    if(able.empty())selected=6;
    else {tape.push_back({0,unsigned(able.size()-1),unsigned(able.size()-1)});selected=able.back();}
   }
   if(selected<6)expected=1u<<selected;
  }
  // Count is decoded independently from byte 24; selector-only coverage does
  // not require or silently admit a record's unsupported damage ability.
  check(record.attacks()==record.raw[24],"Original attack-count field differs");
  for(unsigned attack=0;attack<std::max(1u,unsigned(record.raw[24]));++attack) {
   XeenCombatRandom rng(tape);XeenMonsterTargetCandidate selector(party,record.hatred(),mask);
   bool done=false;for(unsigned chunk=0;chunk<5 && !done;++chunk){XeenConsequenceDraw draw{rng,1,{}};done=selector.service(draw);}
   check(done && selector.mask==expected && rng.position()==tape.size(),"Original selector differs from independent oracle");
  }
 }
}
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
  const auto statistics=XeenMonsterFormat::parse(*assets.readCloudsMonsterStatisticsFromDarkArchive());
  targetingOracle(statistics);
  if(const auto path=std::getenv("MMODERN_M49_READ_SAVE")) {
   // Read-only evidence of the saved equipment/stat inputs used by a volley.
   const auto saved=XeenSaveFile::read(path);check(bool(saved.journey),"Evidence save lacks Journey inputs");
   for(auto owner:kXeenCombatOwners) {
    std::cout<<"EVIDENCE_AC "<<unsigned(owner)<<' '<<XeenCharacterRules::combatArmorClass(saved.characters[owner],
        saved.journey->supplements[owner].inputs,{saved.journey->context->year})<<'\n';
    std::cout<<"EVIDENCE_MAXHP "<<unsigned(owner)<<' '<<XeenCharacterRules::maxHp(saved.characters[owner],
        {saved.journey->context->year})<<'\n';
   }
  }
  if(const auto path=std::getenv("MMODERN_M49_MONSTERS")) {
   std::ofstream out(path);check(bool(out),"Cannot create local monster-rule evidence");
   const auto mainlandActors=XeenActorApproach::actorsFromResources(maps.loadObjects(assets,23),statistics);
   for(const auto &a:mainlandActors) {
    const auto &m=*a.statistics;
    out<<a.id.recordIndex<<' '<<a.original.resourceId<<' '<<m.experience()<<' '<<m.attacks()<<' '<<m.hatred()
       <<' '<<m.strikes()<<' '<<m.damageDie()<<' '<<m.damageType()<<' '<<m.hitParameter()<<' '<<unsigned(m.raw[30])<<'\n';
   }
  }
  const auto &slime=statistics.at(0);
  const auto resource=[&](const std::string &name) {
   return name.rfind("maze",0)==0?assets.readInitialResource(name):assets.readArchiveResource(name);
  };
  XeenWorld manifestWorld([&](auto id){return maps.loadGeometryMap(assets,id);},
   [&](auto id){return maps.loadObjects(assets,id);});
  // Geometry oracle uses the original decoded tiles directly, independently
  // of World sampling. Enumerate every cell/facing, including tile seams.
  const auto originalCell=[&](int x,int y)->const XeenMapCell & {
   const unsigned tile=y>=16?(x>=16?111:110):(x>=16?109:28);
   return manifestWorld.map(tile).geometry.cells[(y%16)*16+x%16];
  };
  constexpr int deltaX[]{0,1,0,-1},deltaY[]{1,0,-1,0};
  for(int y=0;y<32;++y)for(int x=0;x<32;++x)for(unsigned facing=0;facing<4;++facing) {
   const int nx=x+deltaX[facing],ny=y+deltaY[facing];
   const unsigned tile=y>=16?(x>=16?111:110):(x>=16?109:28);
   const auto &source=originalCell(x,y);
   const auto wall=(source.rawWord>>(12-4*facing))&15;
   const auto expected=nx<0||nx>=32||ny<0||ny>=32?XeenMovementResult::BlockedByMapBoundary:
    wall>=manifestWorld.map(tile).geometry.difficulties[0]?XeenMovementResult::BlockedByWall:
    originalCell(nx,ny).surfaceIndex==4?XeenMovementResult::BlockedBySurface:XeenMovementResult::Moved;
   XeenCamera c{28,x,y,XeenDirection(facing)};
   check(XeenMovement().apply(manifestWorld,c,NavigationAction::MoveForward)==expected&&c.mapId==28&&
    c.x==(expected==XeenMovementResult::Moved?nx:x)&&c.y==(expected==XeenMovementResult::Moved?ny:y),
    "Whole Vertigo navigation differs from original tile wall/surface/boundary oracle");
  }
  std::bitset<1024> reached;std::vector<unsigned> queue{15};reached.set(15);
  for(unsigned cursor=0;cursor<queue.size();++cursor)for(unsigned facing=0;facing<4;++facing) {
   XeenCamera c{28,int(queue[cursor]%32),int(queue[cursor]/32),XeenDirection(facing)};
   if(XeenMovement().apply(manifestWorld,c,NavigationAction::MoveForward)!=XeenMovementResult::Moved)continue;
   const unsigned cell=c.y*32+c.x;if(!reached[cell]) {reached.set(cell);queue.push_back(cell);}
  }
  check(reached.count()==424,"Original closed-barrier geometry component differs");
  std::cout<<"Original 1024 cells/four facings, seams and 424-cell closed-barrier component passed\n";
  slime.validateAttackCapabilities();
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
    commands[0].drawOptions().monsterEffectFlags==0,"Slime must retain native palette at every cosmetic phase");
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
  for(unsigned owner=0;owner<2;++owner) {
   tape.push_back({0,5,owner});
   tape.push_back({1,2,2});
   tape.push_back({1,owner==0?120u:40u,1});
   tape.push_back({1,owner==0?120u:40u,owner==0?1u:40u});
  }
  XeenCombatRandom rng(tape);
  XeenEnemyAttackCandidate attack(characters,inputs,slime,610,0x3f);
  bool done=false;
  std::vector<XeenCombatDamage> injuries;
  for(unsigned step=0;step<30 && !done;++step){XeenConsequenceDraw budget{rng,1,{}};
   if(attack.service(budget)) {
    for(unsigned i=0;i<attack.result.injuryCount;++i)injuries.push_back(attack.result.injury(i));
    done=!attack.nextAttack();
   }
  }
  check(done && rng.position()==8 && injuries.size()==2,
   "Slime two random attacks and per-target saves with bounded continuation");
  check(injuries[1].amount==2 && injuries[0].amount==0,
   "Successful repeated poison saves yield zero HP damage");
  for(unsigned owner=0;owner<2;++owner)
   check(!attack.characters[owner].conditions[8] && !attack.characters[owner].conditions[3] &&
    injuries[owner].owner==kXeenCombatOwners[owner] &&
    injuries[owner].amount==(owner==0?0:2),
    "Slime wakes and strikes only its two selected targets without Poison condition");
  for(unsigned owner=2;owner<6;++owner)
   check(attack.characters[owner].conditions==characters[owner].conditions && attack.characters[owner].currentHp==50,
    "Slime leaves unselected and dead members untouched");
  // The same detached poison path must execute each resource-defined city
  // profile, without a species fingerprint or an animation admission gate.
  for(unsigned type:{0u,2u,73u})for(bool save:{false,true}) {
   const auto &record=statistics.at(type);record.validateAttackCapabilities();
   auto party=characters;
   for(auto &member:party)member.currentHp=100;
   std::vector<XeenCombatRandom::Draw> draws;
   int expected=0;
   for(unsigned ordinal=0;ordinal<record.attacks();++ordinal) {
    draws.push_back({0,5,0});
    for(unsigned strike=0;strike<record.strikes();++strike)
     draws.push_back({1,record.damageDie(),record.damageDie()});
    int damage=record.strikes()*record.damageDie();
    draws.push_back({1,120,save?1u:120u});
    if(save)damage/=2;
    while(damage>0) {
     draws.push_back({1,120,save?1u:120u});
     if(!save)break;
     damage/=2;
    }
    expected+=damage;
   }
   XeenCombatRandom random(draws);
   XeenEnemyAttackCandidate candidate(party,inputs,record,610,63);
   unsigned completed=0;
   do {
    bool finished=false;
    for(unsigned chunk=0;chunk<100&&!finished;++chunk) {
     XeenConsequenceDraw draw{random,1,{}};finished=candidate.service(draw);
    }
    check(finished,"Original poison attack did not finish bounded work");++completed;
   }while(candidate.nextAttack());
   check(completed==record.attacks()&&random.position()==draws.size()&&
    candidate.characters[0].currentHp==100-expected&&!candidate.characters[0].conditions[8]&&
    !candidate.characters[0].conditions[3],"Original city poison dice/targets/saves/wake/HP differ");
   for(unsigned owner=1;owner<6;++owner)
    check(candidate.characters[owner].currentHp==100&&candidate.characters[owner].conditions==party[owner].conditions,
     "Original poison profile changed an unselected member");
  }
  std::cout<<"Original Slime, Doom Bug and Breeder Slime poison attacks passed\n";
  std::cout<<"Original M37 route, Slime, 46 city records, sprites and 30 poison inputs passed\n";
  return 0;
 } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
