// Read-only original-resource control of production domain services. No SDL acceptance.
#include "XeenCombatTestSupport.h"
#include "app/XeenEncounterFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenSaveState.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
using namespace combat_test;
int main(int argc,char **argv) {
 try {
  check(argc==2,"usage: mmodern_expedition_original <installation>");
  auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"installation");
  XeenAssetSource assets(*installation);XeenMapLoader maps;
  XeenEventLoader loader([&](const std::string &name)->std::optional<Bytes>{if(!assets.hasInitialResource(name))return {};return assets.readInitialResource(name);});
  const auto chr=assets.readInitialResource("maze.chr");
  const auto ctx=XeenGameplayContextFormat::parse(assets.readInitialResource("maze.pty"));
  auto raw=assets.readCloudsMonsterStatisticsFromDarkArchive();check(bool(raw),"MON");
  auto mon=XeenMonsterFormat::parse(*raw);auto evt=loader.load(20);
  for(auto image:{8,9}){assets.validateNormalMonster(image);assets.validateAttackMonster(image);mon[image].validateCombat();}
  for(unsigned schedule=0;schedule<4;++schedule) {
   auto p=XeenPartyLoader().loadInitialCloudsParty(assets);const auto original=p.roster.characters();
   XeenWorld w([&](auto id){return maps.loadGeometryMap(assets,id);},[&](auto id){return maps.loadObjects(assets,id);});
   auto c=xeenJourneyContent(2).entry;XeenGameFlags f;XeenEventPresenter::Clock clock=[]{return 0;};
   XeenEncounterFlow flow(w,p,c,f,clock,XeenJourneySetup{chr,ctx,mon,evt,schedule==3?78u:1u,2});
   constexpr int hp[]{36,48,36,40,21,15},sp[]{6,0,6,0,21,27},luck[]{12,14,10,17,14,15};
   constexpr int levels[]{3,3,3,4,3,3},xp[]{1000,2000,1000,1000,2000,1000},might[]{17,19,15,14,12,8},speed[]{16,16,15,15,14,14},accuracy[]{15,16,12,18,13,15};
   for(unsigned i=0;i<6;++i){auto id=kXeenCombatOwners[i];const auto &input=*p.roster.combatInputs(id);check(p.roster.at(id).permanentLevel==levels[i]&&input.experience==unsigned(xp[i])&&input.might.permanent==might[i]&&input.speed.permanent==speed[i]&&input.accuracy.permanent==accuracy[i],"prepared level/residual XP/physical inputs");check(p.roster.at(id).currentHp==hp[i]&&p.roster.at(id).currentSp==sp[i],"prepared HP/SP");check(p.roster.combatInputs(id)->luck->permanent==luck[i],"original Luck");check(xeen_state::sameItemCategory(p.roster.at(id).weapons,original[id].weapons),"prepared original weapons");}
   for(unsigned id=0;id<30;++id){check(p.roster.combatInputs(id)&&p.roster.combatInputs(id)->luck,"thirty explicit supplements");const auto &a=p.roster.at(id),&b=original[id];check(xeen_state::sameItemCategory(a.weapons,b.weapons)&&xeen_state::sameItemCategory(a.armor,b.armor)&&xeen_state::sameItemCategory(a.accessories,b.accessories)&&xeen_state::sameItemCategory(a.miscellaneous,b.miscellaneous),"all original item arrays exact");if(std::find(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),id)==kXeenCombatOwners.end())check(xeen_state::sameCharacter(a,b),"inactive original owner unchanged");}
   const std::vector<std::vector<unsigned>> spells{{21},{},{1,20},{},{1,14,21},{0,22,25}};
   for(unsigned i=0;i<6;++i){std::vector<unsigned> known;for(unsigned j=0;j<39;++j)if(chr[kXeenCombatOwners[i]*354+121+j])known.push_back(j);check(known==spells[i],"original spell-knowledge observation only");}
   std::vector<unsigned> ends;bool triple=false;
   const auto present=[&]{if(w.sessionState().journeyActivity()==XeenJourneyActivity::Presentation)check(flow.prepareJourneyFrame(flow.ticket(),[] {})&&flow.presentJourney(flow.ticket()),"headless frame boundary");};
   const auto finish=[&]{
    if(w.sessionState().journeyActivity()==XeenJourneyActivity::Attachment) {
     check(flow.attachJourney(flow.ticket(),[]{}),"attach");
     std::cout<<"contact "<<c.x<<','<<c.y<<" minute "<<p.encounterContext->minutes<<'\n';
     for(unsigned n=0;n<2000 && flow.combat() && flow.combat()->phase()!=Phase::Victory;++n) {
      auto *combat=flow.combat();auto r=combat->phase()==Phase::PlayerReady ? combat->command(combat->ticket(),Command::Attack) : combat->service(combat->ticket());
      if(combat->contacts()[2] && combat->contacts()[0]==XeenMonsterIdentity{20,17} && combat->contacts()[1]==XeenMonsterIdentity{20,18} && combat->contacts()[2]==XeenMonsterIdentity{20,25}){if(!triple)check(p.encounterContext->minutes==533&&w.sessionState().actors()[25].hp==17,"seed78 HP-preserving triple joining checkpoint");triple=true;}
      check(r.status!=Status::Failed&&r.status!=Status::Defeat&&r.status!=Status::SupportStopped,"combat continuation");
     }
     check(flow.combat()&&flow.combat()->phase()==Phase::Victory,"group End");
     std::cout<<"End "<<p.encounterContext->minutes<<" RNG "<<w.sessionState().journeyRandom()->count<<'\n';
     ends.push_back(p.encounterContext->minutes);
     check(flow.retireJourney(flow.ticket()),"retirement");
    } present();
   };
   present();
   auto signature=XeenSaveResourceSignature{{1,2},XeenArchiveFingerprint{3,4}};
   const auto roundtrip=[&]{if(!flow.journeyQuiet())return;auto saved=XeenSaveState::capture(signature,p,c,f,w);auto bytes=XeenSaveFormat::encode(saved);auto base=saved;base.journey.reset();check(bytes.size()-XeenSaveFormat::encode(base).size()==1366,"schema2 suffix");check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes))==bytes,"codec exact");
    XeenPartyState restored;XeenCamera camera;XeenGameFlags flags;XeenWorld world([&](auto id){return maps.loadGeometryMap(assets,id);},[&](auto id){return maps.loadObjects(assets,id);});
    XeenSaveState::Resources resources{signature,{},[&](auto id){return loader.load(id);},{},{},[&]{return mon;}};
    XeenSaveState::restoreBeforeGameplay(saved,resources,restored,camera,flags,world,[](auto &,const auto &,const auto &,const auto &){});
    XeenEncounterFlow bound(world,restored,camera,flags,clock,XeenJourneyRestoreTag{});
    check(bound.prepareJourneyFrame(bound.ticket(),[] {})&&bound.presentJourney(bound.ticket()),"restored binding");
    check(XeenSaveFormat::encode(XeenSaveState::capture(signature,restored,camera,flags,world))==bytes,"guarded current-value roundtrip");
   };
   roundtrip();
   const auto action=[&](XeenEncounterAction a){auto r=flow.journeyAction(flow.ticket(),a);check(r.outcome!=XeenEncounterOutcome::Stopped,"action domain");finish();if(!flow.combat()&&w.sessionState().journeyActivity()!=XeenJourneyActivity::Quiet){flow.journeyPulse(flow.ticket());finish();} if(schedule<2){while(flow.state().pending()){flow.journeyPulse(flow.ticket());finish();}}};
   for(unsigned i=0;i<5;++i){action(XeenEncounterAction::Forward);roundtrip();}
   if(schedule==1)for(unsigned i=0;i<3;++i)action(XeenEncounterAction::Wait);
   action(XeenEncounterAction::Left);action(XeenEncounterAction::Left);
   for(unsigned i=0;i<5;++i)action(XeenEncounterAction::Forward);
   while(flow.state().pending()){flow.journeyPulse(flow.ticket());finish();}
   roundtrip();
   if(schedule==0)check(ends==std::vector<unsigned>{491,522}&&p.encounterContext->minutes==582&&w.sessionState().actors()[17].x==8&&w.sessionState().actors()[18].x==8,"separated original encounters and quiet survivors");
   if(schedule==1)check(ends==std::vector<unsigned>{491,522,565}&&p.encounterContext->minutes==615&&p.roster.at(1).currentHp==5&&p.roster.at(1).conditions[4]==3,"original cumulative pair/Disease route");
   if(schedule==2)check(ends==std::vector<unsigned>{532}&&p.roster.at(1).currentHp==12&&p.roster.at(1).conditions[4]==2&&w.sessionState().actors()[17].x==7&&w.sessionState().actors()[18].x==7,"original mixed route and survivors");
   if(schedule==3)check(ends==std::vector<unsigned>{536}&&triple,"original joined triple route");
   std::cout<<"return "<<p.encounterContext->minutes<<" HP "<<p.roster.at(1).currentHp<<" D "<<unsigned(p.roster.at(1).conditions[4])<<'\n';
  }
  return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
