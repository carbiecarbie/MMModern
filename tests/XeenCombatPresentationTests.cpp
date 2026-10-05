#include "XeenRegionalTestSupport.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include <iostream>
#include <set>
using namespace regional_test;
namespace mmodern {
struct XeenCombatPresentationTestAccess {
 static void observe(XeenEncounterFlow &flow,std::shared_ptr<const XeenRegionalObservation> shots) {flow.observeRanged(std::move(shots));}
 static bool animate(XeenEncounterFlow &flow) {return flow.animateProjectiles();}
 static void presented(XeenEncounterFlow &flow) {flow.projectilesPresented();}
};
}
namespace {
XeenMap projectionMap(XeenMapIdentity id,bool outdoor) {
 auto m=map(id);if(!outdoor) {m.geometry.flags2=0;for(auto &cell:m.geometry.cells)cell.geometry=XeenIndoorWalls{};}
 return m;
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"usage: combat presentation tests <original-installation>");
 const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation missing");
 XeenAssetSource assets(*installation,320,200);assets.loadPalette("mm4.pal");
 const auto bytes=assets.readCloudsMonsterStatisticsFromDarkArchive();check(bool(bytes),"Missing original monster statistics");
 const auto statistics=XeenMonsterFormat::parse(*bytes);
 constexpr unsigned expectedPow[]{12,14,0,4,8,10,13},expectedDamage[]{0,6,1,2,3,4,5};
 for(unsigned type=0;type<7;++type) {check(xeenMonsterProjectile(type)==expectedPow[type],"Original projectile type table");check(xeenPortraitDamageFrame(type)==expectedDamage[type],"Original portrait damage table");}
 bool rejected=false;try{xeenMonsterProjectile(7);}catch(const std::invalid_argument &){rejected=true;}check(rejected,"Malformed projectile metadata accepted");
 std::set<unsigned> resources;unsigned users=0,commands=0;
 for(const auto &monster:statistics)if(monster.raw[32]) {
  ++users;const auto pow=xeenMonsterProjectile(monster.raw[29]);resources.insert(pow);
 }
 check(users>0,"No original ranged users inventoried");
 std::array<std::optional<XeenMonsterIdentity>,3> rank{};
 check(!xeenSplatAlternatePosition(rank),"Absent rank selector is literal -1");
 rank[1]=XeenMonsterIdentity{99,1};check(!xeenSplatAlternatePosition(rank),"Two-monster rank did not retain original selector");
 rank[2]=XeenMonsterIdentity{99,0};check(xeenSplatAlternatePosition(rank),"Original zero-index third actor selector");
 CloudsMapComposer composer;
 for(bool outdoor:{false,true}) {
  XeenWorld world([&](auto id){return projectionMap(id,outdoor);},[](auto id){return XeenObjectFile{id,"empty.mob",true,{}};});
  const XeenCamera camera{99,8,8,XeenDirection::North};
  for(unsigned pow:resources)for(bool enemy:{false,true})for(unsigned row=0;row<4;++row) {
   XeenMonsterAppearance appearance;
   for(unsigned lane=0;lane<6;++lane)appearance.projectiles.push_back({enemy,row,lane,3,{},pow});
   unsigned found=0;
   const auto verify=[&](const auto &stream) {
    for(const auto &c:stream)if(const auto *p=c.projectile()) {
     check(p->pow==pow&&p->row==row&&p->enemy==enemy,"Resource/direction lost in projection");
     const auto options=c.drawOptions();check(options.scaleIndex==4*row+(enemy?3:0)&&options.horizontalFlip==bool(p->lane%2)&&options.sceneClipped,"Projectile depth/mirroring/clipping");
     ++found;++commands;
    }
   };
   if(outdoor) {const auto stream=XeenOutdoorScene().build(world,camera,nullptr,nullptr,{},appearance);verify(stream);composer.drawOutdoorCommands(assets,stream);}
   else {const auto stream=XeenIndoorScene().build(world,camera,nullptr,nullptr,{},appearance);verify(stream);composer.drawIndoorCommands(assets,stream);}
   check(found==6,"Concurrent lanes silently skipped");
  }
  for(unsigned frame=0;frame<7;++frame)for(int damage:{0,1,9,10,99,100}) {
   auto &presentation=world.scenePresentation();presentation.splats={};presentation.hitSplat(1,damage,frame,true);
   unsigned found=0;const auto verify=[&](const auto &stream) {for(const auto &c:stream)if(const auto *s=c.splat()) {
    const auto o=c.drawOptions();check(s->frame==frame&&o.scaleIndex==(damage<10?5:0)&&o.enlarge==(damage>=100)&&o.bottomClipped&&o.sceneClipped,"Splat scaling/clipping");
    const int x=67+(frame?6:0);check(c.x==(damage>=100?x/3:x)&&c.y==(!frame&&damage>=100?60:73),"Front-rank alternate splat placement");++found;
   }};
   if(outdoor) {const auto stream=XeenOutdoorScene().build(world,camera);verify(stream);composer.drawOutdoorCommands(assets,stream);}
   else {const auto stream=XeenIndoorScene().build(world,camera);verify(stream);composer.drawIndoorCommands(assets,stream);}
   check(found==(damage>0?1u:0u),"Zero damage emitted a scene hit splat");
   for(unsigned tick=1;tick<=3;++tick)presentation.advanceFeedback(tick*100);
   check(!presentation.splats[1].duration,"Splat exceeded original three frames");
  }
 }
 XeenProjectileAppearance near{false,0,0,0,{}},far{false,0,1,3,{}};
 near.advance();far.advance();check(!near.active&&far.active&&far.row==1,"One lane termination stopped another");
 for(unsigned i=0;i<3;++i)far.advance();check(!far.active,"Miss/empty ray did not terminate after row three");
 XeenProjectileAppearance enemyNear{true,0,0,1,{}},enemyFar{true,2,1,3,{}};
 enemyNear.advance();enemyFar.advance();check(!enemyNear.active&&enemyFar.row==1&&enemyFar.active,"Mixed-distance enemy batch advanced sequentially");
 Fixture fixture;const auto before=XeenSaveFormat::encode(fixture.snapshot());auto &fx=fixture.w.scenePresentation();
 // Travel never publishes or flashes an injury. Each prepared injury has its
 // own portrait phase, including off-camera sources and omitted visible lanes.
 std::uint64_t now=0;fixture.clock=[&]{return now;};
 for(unsigned facing=0;facing<4;++facing) {
  fx.portraits={};auto shots=std::make_shared<XeenRegionalObservation>();shots->count=8;
  shots->stage=XeenRegionalObservation::Stage::Travel;
  for(unsigned i=0;i<shots->count;++i) {
   auto &shot=shots->shots[i];shot.source={23,i};shot.direction=static_cast<XeenDirection>(facing);shot.distance=3;
   if(i<6) {shot.attack.injuryCount=1;shot.attack.injuries[0].owner=kXeenCombatOwners[i];}
  }
  XeenCombatPresentationTestAccess::observe(*fixture.flow,shots);
  const auto lanes=fixture.flow->appearance().projectiles;
  check(lanes.size()==(fixture.camera.direction==static_cast<XeenDirection>(facing)?6u:0u),"Off-camera source emitted a visible lane");
  for(unsigned owner=0;owner<30;++owner) {
   check(!fx.portraits[owner].damageTicks,"Travel flashed damage before portrait phase");
  }
  // The seventh/eighth front-facing shots are omitted visually, but still
  // consume their own published injury receipts (distinct roster recipients).
  auto impact=std::make_shared<XeenRegionalObservation>();impact->stage=XeenRegionalObservation::Stage::Portrait;
  impact->impactSource=XeenMonsterIdentity{23,6};impact->impactOwner=29;
  XeenCombatPresentationTestAccess::observe(*fixture.flow,impact);
  check(fx.portraits[29].damageTicks==1,"Lane-cap omission suppressed published injury");
  XeenCombatPresentationTestAccess::presented(*fixture.flow);
  now+=100;fx.advanceFeedback(now);XeenCombatPresentationTestAccess::animate(*fixture.flow);
  for(const auto &p:fx.portraits)check(!p.damageTicks,"Projectile animation restarted published injury feedback");
 }
 for(unsigned owner=0;owner<30;++owner) {
  fx.spellEffect(owner);fx.spellEffect(owner);check(fx.portraits[owner].spellFrame==0,"Duplicate restarted/advanced effect");
  for(unsigned frame=0;frame<4;++frame) {check(fx.portraits[owner].spellFrame==frame,"Four-frame spell sequence");fx.advanceFeedback((frame+1)*100);}
  check(fx.portraits[owner].spellFrame==4,"Spell effect did not retire");
  for(unsigned type=0;type<7;++type) {fx.portraitDamage(owner,xeenPortraitDamageFrame(type));check(fx.portraits[owner].damageTicks==1,"Zero-damage hit missing feedback");fx.advanceFeedback(100);check(!fx.portraits[owner].damageTicks,"Damage feedback did not retire");}
 }
 for(unsigned member=0;member<6;++member) {
  const auto owner=fixture.p.party.activeRosterIds()[member];
  fx.portraits[owner]={member%4,member,1};
 }
 auto stripBytes=characterBytes();constexpr std::int16_t health[]{-1,0,1,2,8,120};
 for(unsigned member=0;member<6;++member) {
  const auto offset=kXeenCombatOwners[member]*354+342;
  const auto hp=std::uint16_t(health[member]);stripBytes[offset]=hp&255;stripBytes[offset+1]=hp>>8;
 }
 const auto stripParty=XeenPartyLoader().loadFromResources(stripBytes,partyBytes());
 constexpr int faceX[]{10,45,81,117,153,189},hpX[]{13,50,86,122,158,194};
 for(unsigned mask=0;mask<64;++mask) {
  const auto faces=CloudsUiComposer::buildPortraitPlacements(stripParty,mask),all=CloudsUiComposer::buildPortraitPlacements(stripParty);
  const auto hp=CloudsUiComposer::buildHpPlacements(stripParty,{610},mask),allHp=CloudsUiComposer::buildHpPlacements(stripParty,{610});unsigned slot=0;
  std::vector<unsigned> expectedMembers;
  for(unsigned member=0;member<6;++member)if(mask&(1u<<member)) {
   check(faces[slot].resourceName==all[member].resourceName&&faces[slot].x==faceX[slot]&&hp[slot].x==hpX[slot]&&hp[slot].frame==allHp[member].frame&&hp[slot].rosterId==stripParty.party.activeRosterIds()[member]&&hp[slot].partySlot==slot,"Departed portrait/HP/member mapping");
   expectedMembers.push_back(member);++slot;
  }
  check(faces.size()==slot&&hp.size()==slot,"Empty combat slots not restored");
  for(unsigned key=InputKey::F1;key<InputKey::F1+6;++key) {
   const auto compact=key-InputKey::F1;
   const auto click=xeenMainScreenClick(faceX[compact]+1,160,MainScreen::Combat);
   check(click&&std::holds_alternative<SelectMemberAction>(*click),"Portrait click lost native member action");
   const auto action=xeenMainScreenMemberKey(key);
   check(action&&std::holds_alternative<SelectMemberAction>(*action),"F1-F6 lost native member action");
   const auto keyboard=CloudsUiComposer::partyMemberAtSlot(stripParty,mask,std::get<SelectMemberAction>(*action).partyIndex);
   const auto mouse=CloudsUiComposer::partyMemberAtSlot(stripParty,mask,std::get<SelectMemberAction>(*click).partyIndex);
   const std::optional<std::size_t> expected=compact<expectedMembers.size()?std::optional<std::size_t>{expectedMembers[compact]}:std::nullopt;
   check(keyboard==expected&&mouse==expected,"F1-F6/click mapping includes departed or unused member");
  }
  for(int acting=-1;acting<6;++acting) {
   const auto overlays=CloudsUiComposer::buildPartyFeedbackPlacements(stripParty,fx,mask,acting);
   const bool highlighted=acting>=0&&(mask&(1u<<acting));
   check(overlays.size()==2*slot+unsigned(highlighted),"Departed highlight/effect or missing overlay");
   unsigned index=0;
   if(highlighted) {
    const auto position=std::find(expectedMembers.begin(),expectedMembers.end(),unsigned(acting))-expectedMembers.begin();
    const auto &p=overlays[index++];check(p.resourceName=="global.icn"&&p.frame==8&&p.x==faceX[position]-1&&p.y==149,"Acting highlight moved to wrong compact portrait");
   }
   for(unsigned n=0;n<slot;++n) {
    const auto &damage=overlays[index++],&spell=overlays[index++];const auto member=expectedMembers[n];
    check(damage.resourceName=="charpow.icn"&&damage.frame==member&&damage.x==faceX[n]&&damage.y==150&&spell.resourceName=="spellfx.icn"&&spell.frame==member%4&&spell.x==faceX[n]&&spell.y==150,"Roster effect remapped to departed member");
   }
  }
 }
 check(before==XeenSaveFormat::encode(fixture.snapshot()),"Feedback altered gameplay/RNG/save bytes");
 fx.spellEffect(0,1000);fx.advanceFeedback(1100);check(fx.portraits[0].spellFrame==1,"Independent healing prefix");
 fx.spellEffect(0,1100,true);check(fx.portraits[0].spellFrame==0,"addHitPoints did not reset duplicate-effect suppression");
 const auto cursor=assets.cursorImage();check(cursor.isValid()&&cursor.width>0&&cursor.height>0&&cursor.width<64&&cursor.height<64,"Original cursor dimensions");
 // Decode and draw every original healing frame through the same sprite
 // operation used by Flow's portrait overlays, with a real indexed surface.
 auto portraitSurface=assets.snapshot();
 for(unsigned frame=0;frame<4;++frame) {
  const auto pixels=portraitSurface.pixels;
  assets.drawDialogSprite(portraitSurface,"spellfx.icn",frame,10,150);
  check(portraitSurface.pixels!=pixels,"Original spell effect frame emitted no pixels");
 }
 check(std::any_of(cursor.pixels.begin(),cursor.pixels.end(),[](auto p){return p==0;})&&std::any_of(cursor.pixels.begin(),cursor.pixels.end(),[](auto p){return p!=0;}),"Cursor transparency/content");
 std::cout<<"Clouds ranged users="<<users<<" projectile resources="<<resources.size()<<" emitted concurrent commands="<<commands<<"; splats, 30 portrait recipients, shrinking strip, cursor and save isolation PASS\n";
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
