#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenEventTrigger.h"
#include <iostream>
#include <sstream>
using namespace training_test;
namespace {
void summaries(Inputs &in) {
 auto p=XeenPartyLoader().loadInitialCloudsParty(in.assets);const auto &text=in.assets.uiText();
 p.monsterTreasure.emplace();p.monsterTreasure->gold=321;p.monsterTreasure->gems=123;
 const std::vector<std::size_t> members{0,1,2,3,4,5};
 for(unsigned food:{0u,17u,18u,35u,36u,90u}) {
  p.food=food;const auto days=food/6/3;
  const auto result=xeenQuickReferenceText(text,p,members);
  std::vector<std::string> args(8);args.insert(args.end(),{std::to_string(unsigned(p.monsterTreasure->gold)),std::to_string(unsigned(p.monsterTreasure->gems)),std::to_string(days),std::string(text.scalar(days==1?"DAY_SINGULAR":"DAY_PLURAL"))});
  const auto footer=xeenDialogFormat(text.scalar("QUICK_REFERENCE"),args);
  check(result.find(footer.substr(footer.find('\v',footer.find('\v')+1)))!=std::string::npos,"Quick Reference food rounding/suffix");
 }
 auto &c=p.roster.at(p.party.activeRosterIds()[0]);c.temporaryLevel=2;c.currentHp=0;c.currentSp=0;c.conditions[13]=1;
 const auto result=xeenQuickReferenceText(text,p,{0});
 const auto &className=text.table("CLASS_NAMES").at(unsigned(c.characterClass));
 const auto &condition=text.table("CONDITION_NAMES").at(unsigned(c.worstCondition()));
 const auto expected=xeenDialogFormat(text.scalar("QUICK_REF_LINE"),{"24","1",c.name,
  className.substr(0,1),className.substr(1,1),className.substr(2,1),"2",std::to_string(unsigned(c.permanentLevel)),
  "6","0","6","0",std::to_string(XeenCharacterRules::statColor(XeenCharacterRules::sheetArmorClass(c,nullptr,{610}),XeenCharacterRules::sheetArmorClass(c,nullptr,{610},true))),
  std::to_string(XeenCharacterRules::sheetArmorClass(c,nullptr,{610})),"6",condition.substr(0,1),condition.substr(1,1),condition.substr(2,1),condition.substr(3,1)});
 check(result.find(expected)!=std::string::npos,"Quick Reference permanent level, current-level color, HP/SP, AC and worst condition");
 check(xeenQuickReferenceText(text,p,{2,4}).find(p.party.member(p.roster,0).name)==std::string::npos &&
  xeenQuickReferenceText(text,p,{2,4}).find(p.party.member(p.roster,4).name)!=std::string::npos,"Quick Reference participant subset/order");
 XeenWorld world(in.mapLoader(),in.objectLoader());XeenCamera camera{28,18,4,XeenDirection::West};
 const auto base=CloudsMapComposer().compose(in.assets,world,p,camera,{610});
 const auto quick=drawXeenQuickReference(text,base,in.font,p,members);
 check(quick.isValid() && quick.pixels!=base.pixels,"CD Quick Reference rendering");
 for(int y=146;y<200;++y)for(int x=0;x<320;++x)check(quick.pixels[y*320+x]==base.pixels[y*320+x],"Quick Reference retains portraits/underlay");
 XeenGameplayContext ctx;ctx.year=610;
 for(unsigned minutes:{0u,1u,719u,720u,779u,780u,1439u})for(unsigned day:{0u,1u,9u,10u}) {
  ctx.minutes=minutes;ctx.day=day;const unsigned hour=minutes/60;
  const auto popup=xeenInfoPopup(text,ctx);
  check(popup.bounds.bottom==112 && popup.text==xeenDialogFormat(text.scalar("GAME_INFORMATION"),{
   std::string(text.scalar("WORLD_GAME_TEXT")),text.table("WEEK_DAY_STRINGS")[day%10],std::to_string(hour>12?hour-12:hour?hour:12),
   std::to_string(minutes%60),hour>11?"p":"a",std::to_string(day),"610",""}),"Info title, clock boundaries and ten-day weekday");
  check(drawXeenPopup(base,in.font,popup).isValid(),"CD Info without effects");
 }
 constexpr const char *fields[]{"LIGHT_COUNT_TEXT","FIRE_RESISTANCE_TEXT","ELECTRICITY_RESISTANCE_TEXT","COLD_RESISTANCE_TEXT","POISON_RESISTANCE_TEXT","CLAIRVOYANCE_TEXT","LEVITATE_TEXT","WALK_ON_WATER_TEXT"};
 for(unsigned mask=1;mask<256;++mask) {
  for(auto &v:ctx.effects)v=0;for(auto &v:ctx.lightAndResistances)v=0;std::string effects;unsigned rows=0;
  for(unsigned bit=0;bit<8;++bit)if(mask&(1u<<bit)) {
   std::vector<std::string> args;
   if(bit==0){ctx.lightAndResistances[0]=7;args={"7"};}
   else {
    args={std::string(1,rows?'\1':'\n'),std::string(text.scalar("INFO_ALIGN_LEFT"))};
    if(bit!=7)args.push_back(std::string(text.scalar("INFO_ALIGN_RIGHT")));
    if(bit<5){ctx.lightAndResistances[bit+1]=bit+10;args.push_back(std::to_string(bit+10));}
    else {constexpr unsigned indices[]{0,3,4};ctx.effects[indices[bit-5]]=1;}
   }
   effects+=xeenDialogFormat(text.scalar(fields[bit]),args);++rows;
  }
  const auto popup=xeenInfoPopup(text,ctx);
  check(popup.bounds.bottom==125+int(rows)*9 && popup.text.substr(popup.text.size()-effects.size())==effects,"Info effect order, spacing, values and height");
  check(drawXeenPopup(base,in.font,popup).isValid(),"CD Info effect combination rendering");
 }
 const auto all=xeenInfoPopup(text,ctx).text;
 ctx.lightAndResistances[1]=99;for(unsigned i:{1u,2u,5u,6u,7u,8u})ctx.effects[i]=1;
 check(xeenInfoPopup(text,ctx).text==all,"Info must exclude other effect fields");
}
void readonly(Inputs &in) {
 auto source=in.service();source.camera={28,10,9,XeenDirection::North};Fixture f(in,source,true);
 const auto before=XeenSaveFormat::encode(f.snapshot());
 for(auto action:{PlayerAction{QuickReferenceAction{}},PlayerAction{InfoAction{}}}) {
  const auto caller=f.flow->frame().presentation();const auto input=f.flow->displayedInput();
  f.act(action);const auto opened=f.flow->frame().presentation();
  auto context=f.flow->inputContext(opened);
  check(context.dialog && context.dialog->anyKey && context.dialog->anyClick && !context.acceptsQueuedInput && f.flow->handlesEscape() && !f.flow->canSave(),"Summary strict modal input");
  f.flow->handle(NavigationAction::MoveForward,input,caller);
  check(f.flow->frame().presentation()==opened,"Caller input closed summary");
  const auto phase=f.w.scenePresentation().wallPhase;
  for(unsigned n=0;n<20;++n){f.now+=100;f.flow->beginCycle(++f.cycle);if(auto next=f.flow->updatePresentation())f.present(*next);}
  check(std::holds_alternative<InfoAction>(action)?f.w.scenePresentation().wallPhase>phase:f.w.scenePresentation().wallPhase==phase,"Info animates; Quick Reference retains underlay");
  f.act(*context.dialog->click(0,0));
  check(!f.flow->inputContext(f.flow->frame().presentation()).dialog,"Summary close returns to main screen");
  check(XeenSaveFormat::encode(f.snapshot())==before,"Summary changed saved state/time/RNG");
  const auto returned=f.flow->frame().presentation();
  f.flow->handle(NavigationAction::MoveForward,input,opened);
  check(f.flow->frame().presentation()==returned,"Summary stale dismissal leaked movement");
 }
 f.act(SelectMemberAction{1});const auto selection=f.flow->inventorySelection();
 f.act(DialogKeyAction{'q'});f.act(AcknowledgeAction{});
 check(f.flow->inventoryOpen() && f.flow->inventorySelection().sourceOwner==selection.sourceOwner,"Quick Reference returns to selected sheet member");
 f.act(CancelInteractionAction{});check(XeenSaveFormat::encode(f.snapshot())==before,"Sheet summary changed saved state");
}
void strafe(Inputs &in) {
 auto source=in.service();source.camera={28,10,9,XeenDirection::West};
 // Real topology/actors; compare ordinary and lateral publication before the
 // same retained monster countdown. Facing is the only intended difference.
 auto forward=source;forward.camera.direction=XeenDirection::North;
 Fixture lateral(in,source),ordinary(in,forward);
 lateral.act(NavigationAction::StrafeRight);ordinary.act(NavigationAction::MoveForward);
 check(lateral.c.x==ordinary.c.x && lateral.c.y==ordinary.c.y && lateral.c.direction==XeenDirection::West && lateral.c.y==10,
  "Journey strafe retains facing and reaches ordinary destination");
 check(*lateral.p.encounterContext==*ordinary.p.encounterContext &&
  lateral.w.sessionState().journeyRandom()==ordinary.w.sessionState().journeyRandom() &&
  lateral.flow->encounter()->state().pending()==ordinary.flow->encounter()->state().pending(),"Strafe ordinary time/ctr24/countdown/RNG parity");
 check(lateral.p.encounterContext->minutes==source.journey->context->minutes+1,"Indoor strafe charges one minute");
 const auto settle=[](Fixture &f) {
  for(unsigned n=0;n<100 && !f.flow->canSave();++n){f.now+=100;f.flow->beginCycle(++f.cycle);if(auto next=f.flow->updatePresentation())f.present(*next);}
  check(f.flow->canSave(),"Strafe countdown did not settle to Quiet");
 };
 settle(lateral);settle(ordinary);auto normalized=ordinary.snapshot();normalized.camera.direction=source.camera.direction;
 save_test::sameSnapshot(lateral.snapshot(),normalized);
 auto outdoors=in.base();outdoors.camera.direction=XeenDirection::East;
 for(auto &a:outdoors.journey->actors)if(a.lifecycle==XeenActorLifecycle::Present)a.activated=true;
 auto towards=outdoors;towards.camera.direction=XeenDirection::North;
 Fixture side(in,outdoors),ahead(in,towards);side.act(NavigationAction::StrafeLeft);ahead.act(NavigationAction::MoveForward);
 check(side.p.encounterContext->minutes==outdoors.journey->context->minutes+10,"Outdoor strafe charges ten minutes");
 settle(side);settle(ahead);normalized=ahead.snapshot();normalized.camera.direction=outdoors.camera.direction;
 save_test::sameSnapshot(side.snapshot(),normalized);
 // Find a real blocked wall and verify the Journey uses its ordinary blocked path.
 XeenWorld world(in.mapLoader(),in.objectLoader());XeenMovement movement;bool wall=false,event=false;
 for(int y=0;y<32 && (!wall || !event);++y)for(int x=0;x<32 && (!wall || !event);++x)for(unsigned d=0;d<4;++d) {
  XeenCamera from{28,x,y,XeenDirection(d)},target=from;
  const auto moved=movement.apply(world,target,NavigationAction::StrafeRight);
  if(!wall && moved==XeenMovementResult::BlockedByWall) {
   auto s=source;s.camera=from;
   for(auto &a:*s.journey->vertigoActors)if(a.lifecycle==XeenActorLifecycle::Present)a.activated=true;
   Fixture a(in,s);const auto context=*a.p.encounterContext;
   const auto rng=a.w.sessionState().journeyRandom();a.act(NavigationAction::StrafeRight);
   check(a.c.x==from.x && a.c.y==from.y && a.c.direction==from.direction && *a.p.encounterContext==context &&
    a.w.sessionState().journeyRandom()==rng,"Blocked Vertigo strafe time/ctr24/RNG");wall=true;
  }
  const auto sample=world.sampleCell(28,target.x,target.y);
  if(!event && moved==XeenMovementResult::Moved && sample && (sample->cell->rawAttributes&kXeenAutomaticEventFlag) && xeenRegionalEvent(in.city,target)) {
   auto s=source;s.camera=from;
   // Synthetic defeated actors isolate the real Event from incidental combat.
   for(auto &a:*s.journey->vertigoActors)if(a.lifecycle==XeenActorLifecycle::Present) {
    a.x=a.y=-128;a.hp=0;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
   }
   Fixture a(in,s);bool ran=false;
   a.flow->reportAutomatic=[&](const auto &result){ran=!std::holds_alternative<XeenAutomaticEventNoTrigger>(result);};
   // Vertigo's existing detached Event flow reports its converted result
   // through the manual-result callback even for an automatic trigger.
   a.flow->reportManual=[&](const auto &result){ran=!std::holds_alternative<XeenManualEventNoEvent>(result);};
   a.act(NavigationAction::StrafeRight);
   check(a.c.direction==from.direction && a.flow->encounter()->actionResult().automaticEvent,"Strafe automatic Event lookup retains facing");
   for(unsigned n=0;n<100 && !ran;++n){a.now+=100;a.flow->beginCycle(++a.cycle);if(auto next=a.flow->updatePresentation())a.present(*next);}
   check(ran,"Strafe must execute the automatic Event after its ordinary monster opportunities");event=true;
  }
 }
 check(wall && event,"Vertigo wall/Event strafe witnesses absent");
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"usage: interface-tests <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing installation");
 Inputs in(*installation);std::ostringstream captured;auto *prior=std::cout.rdbuf(captured.rdbuf());
 try{summaries(in);readonly(in);strafe(in);}catch(...){std::cout.rdbuf(prior);throw;}
 std::cout.rdbuf(prior);std::cout<<"Strafe, read-only summaries, DOS layouts and stale input passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
