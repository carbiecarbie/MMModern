#include "XeenTestInstallation.h"
#include "formats/xeen/XeenVocDuration.h"
#include "XeenTrainingTestSupport.h"
#include "XeenMonsterTestAccess.h"
#include "XeenM42Evidence.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenCharacterRules.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include <algorithm>
#include <iostream>
#include <sstream>
using namespace training_test;
namespace mmodern {
struct XeenPartyInterfaceTestAccess {
 static auto shootOwners(const XeenEventFlow &f){return f._encounter->_shoot->owners;}
 static auto shootEligible(const XeenEventFlow &f){return f._encounter->_shoot->eligible;}
 static auto lanes(const XeenEventFlow &f){return f._encounter->_projectiles;}
 static bool reward(const XeenEventFlow &f){return f._encounter->monsterReward();}
 static auto rewardPhase(const XeenEventFlow &f){return f._encounter->_monsterReceipt->phase;}
 static unsigned rewardCursor(const XeenEventFlow &f){return f._encounter->_monsterReceipt->cursor;}
 static std::uint64_t rewardInputDeadline(const XeenEventFlow &f){return f._ordinary.deadline;}
 static auto rewardWait(const XeenEventFlow &f){return f._treasureWait;}
 static const auto &rewardRecord(const XeenEventFlow &f){return f._encounter->_monsterReceipt->records[f._encounter->_monsterReceipt->cursor];}
 static bool exchanging(const XeenEventFlow &f){return f._exchange;}
 static auto optionMember(const XeenEventFlow &f){return f._quickFightMember;}
 static bool restComplete(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Complete;}
 static bool restConfirm(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Confirm;}
};
}
namespace {
void pulse(Fixture &f){f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);}
void quiet(Fixture &f) {
 for(unsigned n=0;n<2000 && !f.flow->canSave();++n) {
  if(XeenPartyInterfaceTestAccess::restConfirm(*f.flow))f.act(DialogKeyAction{'y'});
  else if(XeenPartyInterfaceTestAccess::restComplete(*f.flow))f.act(AcknowledgeAction{});
  else pulse(f);
 }
 if(!f.flow->canSave())throw std::runtime_error("Party interface did not settle: "+f.flow->encounter()->notice());
}
XeenSaveSnapshot emptyCity(Inputs &in) {
 auto s=in.service();s.camera={28,10,9,XeenDirection::North};
 for(auto &a:s.map(28).monsters->slots)if(a.lifecycle==XeenActorLifecycle::Present) {
  a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
 }
 return s;
}
void exchange(Fixture &f,unsigned from,unsigned to,bool mouse=false) {
 f.act(SelectMemberAction{from});f.act(DialogKeyAction{'e'});
 check(XeenPartyInterfaceTestAccess::exchanging(*f.flow),"Exchange entry");
 const auto input=f.flow->inputContext(f.flow->frame().presentation()).dialog;
 constexpr int x[]{10,45,81,117,153,189};
 f.act(mouse?*input->click(x[to]+3,155):PlayerAction{DialogKeyAction{InputKey::F1+to}});
 check(!XeenPartyInterfaceTestAccess::exchanging(*f.flow) && f.flow->inventorySelection().source==to,"Exchange follows the owner");
 f.act(CancelInteractionAction{});quiet(f);
}
void exchangeAndSave(Inputs &in) {
 auto source=emptyCity(in);Fixture key(in,source),mouse(in,source);
 const auto original=key.snapshot();key.act(SelectMemberAction{0});
 const auto caller=key.flow->frame().presentation();const auto token=key.flow->displayedInput();key.act(DialogKeyAction{'e'});
 const auto dialog=key.flow->inputContext(key.flow->frame().presentation()).dialog;
 check(dialog && dialog->click(228,123) && !dialog->click(228,145) && !dialog->click(5,155) && !dialog->click(50,145),"Exchange DOS button/faces only");
 key.flow->handle(SelectMemberAction{1},token,caller);
 check(XeenPartyInterfaceTestAccess::exchanging(*key.flow),"Stale caller input must not swap");
 key.act(DialogKeyAction{InputKey::F1});check(XeenPartyInterfaceTestAccess::exchanging(*key.flow),"Self Exchange stays open");
 key.act(DialogKeyAction{InputKey::F1+6});check(XeenPartyInterfaceTestAccess::exchanging(*key.flow),"Missing Exchange stays open");
 key.act(CancelInteractionAction{});key.act(CancelInteractionAction{});quiet(key);
 check(XeenSaveFormat::encode(key.snapshot())==XeenSaveFormat::encode(original),"Exchange cancel is read-only");
 exchange(key,0,2);exchange(mouse,0,2,true);
 auto expected=original;std::swap(expected.activeRosterIds[0],expected.activeRosterIds[2]);
 check(XeenSaveFormat::encode(key.snapshot())==XeenSaveFormat::encode(expected),"Exchange changes order only");
 check(XeenSaveFormat::encode(mouse.snapshot())==XeenSaveFormat::encode(expected),"Exchange mouse/key parity");
 key.act(NavigationAction::StrafeLeft);quiet(key);
 auto changed=key.snapshot();for(unsigned n=0;n<30;++n){changed.characters[n].quickOption=n%4;changed.characters[n].currentSpell=n%39;}
 Fixture settings(in,changed);const auto saved=XeenSaveFormat::encode(settings.snapshot());Fixture loaded(in,XeenSaveFormat::decode(saved));
 check(saved==XeenSaveFormat::encode(loaded.snapshot()),"Exchange + Strafe + options exact reload/re-save");
 settings.act(WaitAction{});loaded.act(WaitAction{});quiet(settings);quiet(loaded);
 check(XeenSaveFormat::encode(settings.snapshot())==XeenSaveFormat::encode(loaded.snapshot()),"Generic next action after restore replays nothing");
 auto old=saved;old[8]=7;old[9]=0;bool rejected=false;
 try{(void)XeenSaveFormat::decode(old);}catch(const std::exception &e){rejected=std::string(e.what()).find("older")!=std::string::npos;}
 check(rejected,"v7 must be clearly rejected");
 XeenPartyState p;XeenCamera c;XeenGameFlags flags;XeenWorld world(in.mapLoader(),in.objectLoader());
 XeenSaveState::restoreBeforeGameplay(changed,in.resources(),p,c,flags,world,[](auto &,const auto &,const auto &,const auto &){});std::vector<std::uint8_t> ids(kXeenCombatOwners.begin(),kXeenCombatOwners.end());std::sort(ids.begin(),ids.end());unsigned permutations=0;
 do{p.party=XeenParty::fromRosterIds(ids);xeenValidateJourneyParty(p);++permutations;}while(std::next_permutation(ids.begin(),ids.end()));
 check(permutations==720,"All six-member permutations accepted");
 for(auto bad:std::vector<std::vector<std::uint8_t>>{{0,18,14,11,1,1},{0,18,14,11,1},{0,18,14,11,1,2}}) {
  p.party=XeenParty::fromRosterIds(bad);bool refused=false;try{xeenValidateJourneyParty(p);}catch(const std::invalid_argument &){refused=true;}check(refused,"Other membership rejected");
 }
 for(unsigned option:{4u,255u}){auto invalid=changed;invalid.characters[29].quickOption=option;bool refused=false;try{XeenSaveFormat::encode(invalid);}catch(...){refused=true;}check(refused,"Invalid quickOption rejected for inactive owner");}
 for(unsigned slot:{39u,254u}){auto invalid=changed;invalid.characters[29].currentSpell=slot;bool refused=false;try{XeenSaveFormat::encode(invalid);}catch(...){refused=true;}check(refused,"Invalid currentSpell rejected for inactive owner");}
}
void reorderedTimeAndRest(Inputs &in) {
 auto source=emptyCity(in);Fixture exchanged(in,source);exchange(exchanged,0,5);source=exchanged.snapshot();
 source.journey->context->minutes=479;
 for(unsigned i=0;i<6;++i){auto &c=source.characters[source.activeRosterIds[i]];c.conditions[3]=i%2;c.conditions[4]=(i+1)%2;}
 Fixture ticks(in,source);XeenConsequenceCharacters chars;XeenConsequenceInputs inputs;
 for(unsigned n=0;n<6;++n){const auto owner=source.activeRosterIds[n];chars[n]=source.characters[owner];inputs[n]=source.journey->supplements[owner].inputs;}
 XeenConditionTimeCandidate oracle(*source.journey->context,1,chars,inputs,&*source.journey->serviceEconomy);
 XeenCombatRandom rng(*source.journey->random);bool done=false;for(unsigned n=0;n<50 && !done;++n){XeenConsequenceDraw draw{rng,64,{}};done=oracle.service(draw);}check(done,"Condition oracle bounded");
 ticks.act(WaitAction{});quiet(ticks);
 for(unsigned n=0;n<6;++n){const auto owner=source.activeRosterIds[n];check(xeen_state::sameCharacter(ticks.p.roster.at(owner),oracle.characters[n]) && xeen_state::sameInputs(*ticks.p.roster.combatInputs(owner),oracle.inputs[n]),"Condition ticks read/write captured order");}
 check(ticks.w.sessionState().journeyRandom()==rng.continuation(),"Reordered condition tick RNG order");
 source=exchanged.snapshot();source.food=2;for(auto owner:source.activeRosterIds){source.characters[owner].currentHp=1;source.characters[owner].currentSp=0;}
 Fixture rest(in,source);rest.flow->loadRestDream=[&]{return in.assets.restDreamImage();};rest.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));};rest.act(RestAction{});quiet(rest);
 check(rest.p.food==0,"Scarce food consumed");
 for(unsigned n=0;n<6;++n){const auto &c=rest.p.roster.at(source.activeRosterIds[n]);check(c.currentHp==(n<2?XeenCharacterRules::maxHp(c,{rest.p.encounterContext->year}):1),"Rest feeds current positions and writes to owners");}
}
void ready(Fixture &f){for(unsigned n=0;n<1000;++n){if(auto combat=f.flow->encounter()->combat();combat && combat->phase()==XeenCombatPhase::PlayerReady && !combat->cast())return;pulse(f);}throw std::runtime_error("No combat PlayerReady");}
void engage(Fixture &f){f.act(NavigationAction::MoveForward);quiet(f);f.act(ShootAction{});quiet(f);f.act(NavigationAction::MoveForward);ready(f);}
void combatAndOptions(Inputs &in) {
 auto source=in.base();for(auto owner:source.activeRosterIds)source.journey->supplements[owner].inputs.speed={30,0};
 Fixture reordered(in,source);exchange(reordered,0,4);source=reordered.snapshot();
 Fixture f(in,source);engage(f);auto combat=f.flow->encounter()->combat();
 check(combat->participant()==0,"Reordered combat speed ties use first position");
 const auto actor=f.p.party.activeRosterIds()[0];const auto time=*f.p.encounterContext;const auto rng=f.w.sessionState().journeyRandom();
 std::vector<std::pair<int,int>> highlights;
 const auto draw=f.flow->drawDialogSprite;
 f.flow->drawDialogSprite=[&](auto &frame,const char *resource,unsigned id,int x,int y) {
  if(std::string_view(resource)=="global.icn" && id==8)highlights.emplace_back(x,y);
  draw(frame,resource,id,x,y);
 };
 const auto highlighted=[&](PlayerAction action,unsigned slot,const char *message) {
  highlights.clear();f.act(action);constexpr int x[]{10,45,81,117,153,189};
  check(highlights==std::vector<std::pair<int,int>>{{x[slot]-1,149}},message);
 };
 highlighted(QuickFightOptionsAction{},0,"Options initially draws exactly one actor highlight");check(XeenPartyInterfaceTestAccess::optionMember(*f.flow)==0,"Options starts on actual actor");
 highlighted(DialogKeyAction{InputKey::F1+2},2,"Changed Options selection draws only the selected highlight");
 check(XeenPartyInterfaceTestAccess::optionMember(*f.flow)==2,"Options changed selection member");
 highlighted(DialogKeyAction{InputKey::F1},0,"Options restores first selected highlight");
 auto option=f.p.roster.at(actor).quickOption;const auto stale=f.flow->frame().presentation();const auto token=f.flow->displayedInput();
 f.act(DialogKeyAction{'n'});check(f.p.roster.at(actor).quickOption==(unsigned(option)+1)%4,"Next immediately cycles option");
 f.flow->handle(DialogKeyAction{'n'},token,stale);check(f.p.roster.at(actor).quickOption==(unsigned(option)+1)%4,"Stale Next cannot repeat");
 const auto dialog=f.flow->inputContext(f.flow->frame().presentation()).dialog;
 f.act(*dialog->click(245,112));check(f.p.roster.at(actor).quickOption==2,"Options Next key/mouse parity");
 f.act(AcknowledgeAction{});check(!XeenPartyInterfaceTestAccess::optionMember(*f.flow) && *f.p.encounterContext==time && f.w.sessionState().journeyRandom()==rng && combat->participant()==0,"Options closes without undo or gameplay cost");
 f.act(QuickFightAction{});check(combat->result().operation==XeenCombatOperation::Block && combat->result().actingOwner==actor && (combat->result().blockedMembers&1),"Quick Block acts for current reordered owner only");
 ready(f);check(combat->participant()!=0,"Quick Fight advances one actor");
 f.act(QuickFightOptionsAction{});const auto other=*XeenPartyInterfaceTestAccess::optionMember(*f.flow);const auto otherOwner=f.p.party.activeRosterIds()[other];
 while(f.p.roster.at(otherOwner).quickOption!=3)f.act(DialogKeyAction{'n'});
 f.act(CancelInteractionAction{});f.act(QuickFightAction{});for(unsigned n=0;n<100 && combat->phase()!=XeenCombatPhase::PlayerReady;++n)pulse(f);
 if(combat->participants()&(1u<<other))throw std::runtime_error("Deterministic Quick Run fixture did not flee");
 ready(f);unsigned count=0,actorSlot=0;for(unsigned n=0;n<6;++n)if(combat->participants()&(1u<<n)){++count;if(n<unsigned(combat->participant()))++actorSlot;}
 check(actorSlot!=unsigned(combat->participant()),"Post-Run fixture distinguishes compressed actor slot");
 highlighted(QuickFightOptionsAction{},actorSlot,"Options-after-Run initially highlights only the compressed actor portrait");
 check(XeenPartyInterfaceTestAccess::optionMember(*f.flow)==std::size_t(combat->participant()),"Options-after-Run initially selects actual actor");
 highlighted(DialogKeyAction{InputKey::F1+unsigned(other)},unsigned(other),"Options-after-Run highlights pressed slot when selecting fled member");
 check(XeenPartyInterfaceTestAccess::optionMember(*f.flow)==other && !(combat->participants()&(1u<<other)),"Options-after-Run retains fled-member selection quirk");
 highlighted(DialogKeyAction{InputKey::F1},0,"Options-after-Run selection highlights the pressed portrait only");check(XeenPartyInterfaceTestAccess::optionMember(*f.flow)==0,"Options-after-Run keeps active-party indexing");
 highlighted(DialogKeyAction{InputKey::F1+count},0,"Options-after-Run empty slot retains single highlight");check(XeenPartyInterfaceTestAccess::optionMember(*f.flow)==0,"Options-after-Run rejects empty slot");
 highlighted(CancelInteractionAction{},actorSlot,"Closing Options restores single combat actor highlight");
 // End this legitimately entered combat, then persist the UI settings together with Exchange and Strafe.
 for(unsigned n=0;n<1500 && f.flow->encounter()->combat();++n) {
  const auto *c=f.flow->encounter()->combat();
  if(c->phase()==XeenCombatPhase::PlayerReady)f.act(RunAction{});else pulse(f);
 }
 check(!f.flow->encounter()->combat(),"Configured combat returns to Journey");quiet(f);f.act(NavigationAction::StrafeLeft);quiet(f);
 const auto configured=f.snapshot();check(configured.characters[actor].quickOption==2 && configured.activeRosterIds==source.activeRosterIds,"UI option changes survive combat with exchanged order");
 Fixture reload(in,configured);check(XeenSaveFormat::encode(reload.snapshot())==XeenSaveFormat::encode(configured),"Actual Exchange + Strafe + Options reload/re-save exact");
 f.act(WaitAction{});reload.act(WaitAction{});quiet(f);quiet(reload);check(XeenSaveFormat::encode(f.snapshot())==XeenSaveFormat::encode(reload.snapshot()),"Actual configured Journey resumes without replay");
 // Attack uses the same one-actor preparation as the ordinary command.
 source.characters[source.activeRosterIds[0]].quickOption=0;Fixture attack(in,source),quick(in,source);engage(attack);engage(quick);
 attack.act(AttackAction{});quick.act(QuickFightAction{});
 check(attack.flow->encounter()->combat()->result().actingOwner==quick.flow->encounter()->combat()->result().actingOwner && quick.flow->encounter()->combat()->result().operation==XeenCombatOperation::PlayerAttack,"Quick Attack ordinary acting owner");
 for(unsigned n=0;n<40 && attack.flow->encounter()->combat()->phase()!=XeenCombatPhase::PlayerReady;++n){pulse(attack);pulse(quick);}
 check(attack.w.sessionState().journeyRandom()==quick.w.sessionState().journeyRandom(),"Quick Attack ordinary RNG");
 for(unsigned owner=0;owner<30;++owner)check(xeen_state::sameCharacter(attack.p.roster.at(owner),quick.p.roster.at(owner)) && xeen_state::sameInputs(*attack.p.roster.combatInputs(owner),*quick.p.roster.combatInputs(owner)),"Reordered combat owner write-back parity");
}

unsigned spellSlot(XeenCharacterClass cls,unsigned id) {
 const auto category=XeenLearnedSpellRules::categoryForClass(cls);check(bool(category),"Caster category");
 for(unsigned slot=0;slot<39;++slot)if(XeenLearnedSpellRules::spellForSlot(*category,slot)==id)return slot;
 throw std::runtime_error("Spell absent from book");
}
XeenSaveSnapshot casterSource(Inputs &in,XeenCharacterClass cls,unsigned id) {
 auto s=in.base();std::swap(s.activeRosterIds[0],s.activeRosterIds[4]);const auto owner=s.activeRosterIds[0];
 for(auto id:s.activeRosterIds)s.journey->supplements[id].inputs.speed={30,0};
 auto &c=s.characters[owner];c.characterClass=cls;c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};c.hasSpells=true;c.learnedSpells.emplace();c.learnedSpells->fill(1);c.currentSp=300;c.quickOption=1;c.currentSpell=id==255?255:spellSlot(cls,id);
 s.journey->treasure->gems=1000;return s;
}
void quickCasting(Inputs &in) {
 using CP=XeenCombatCastPhase;
 auto source=casterSource(in,XeenCharacterClass::Cleric,26);const auto owner=source.activeRosterIds[0];
 Fixture cancel(in,source);engage(cancel);auto combat=cancel.flow->encounter()->combat();
 const auto sp=cancel.p.roster.at(owner).currentSp;const auto rng=cancel.w.sessionState().journeyRandom();
 cancel.act(QuickFightAction{});check(combat->cast() && combat->cast()->phase==CP::PartyTarget && cancel.p.roster.at(owner).currentSp==sp-1,"Quick First Aid retains target prompt and debit");
 cancel.act(CancelInteractionAction{});check(!combat->cast() && cancel.p.roster.at(owner).currentSp==sp,"Quick First Aid target cancel refunds and retires");
 ready(cancel);check(combat->participant()!=0 && cancel.w.sessionState().journeyRandom()==rng,"Quick target cancel spends actor turn without RNG");
 Fixture heal(in,source);engage(heal);combat=heal.flow->encounter()->combat();heal.act(QuickFightAction{});heal.act(SelectMemberAction{1});ready(heal);
 check(!combat->cast() && combat->participant()!=0 && heal.p.roster.at(owner).currentSp==sp-1,"Quick First Aid settles and advances once");
 // Browsing/cancelling keeps the remembered slot; accepting a selection publishes it before outer cancellation.
 Fixture remember(in,source);engage(remember);combat=remember.flow->encounter()->combat();const auto prior=remember.p.roster.at(owner).currentSpell;
 remember.act(CastSpellAction{});remember.act(DialogKeyAction{'n'});remember.act(DialogKeyAction{'1'});const auto chosen=XeenLearnedSpellRules::rows(remember.p.roster.at(owner)).front();
 check(chosen!=prior,"Spell browsing fixture");remember.act(CancelInteractionAction{});check(remember.p.roster.at(owner).currentSpell==prior,"Cancelled spell selection preserves remembered slot");
 remember.act(CancelInteractionAction{});remember.act(CastSpellAction{});remember.act(DialogKeyAction{'n'});remember.act(DialogKeyAction{'1'});remember.act(DialogKeyAction{'s'});
 check(remember.p.roster.at(owner).currentSpell==chosen,"Accepted spell selection is tracked immediately");remember.act(CancelInteractionAction{});remember.act(CancelInteractionAction{});
 check(!combat->cast() && remember.p.roster.at(owner).currentSpell==chosen,"Outer Cast cancellation keeps selection");
 source=casterSource(in,XeenCharacterClass::Sorcerer,45);Fixture arrow(in,source),manual(in,source);engage(arrow);engage(manual);
 arrow.act(QuickFightAction{});manual.act(CastSpellAction{});manual.act(DialogKeyAction{'c'});
 for(unsigned n=0;n<1000;++n){if(manual.flow->encounter()->combat()->cast() && manual.flow->encounter()->combat()->cast()->phase==CP::Result)break;pulse(arrow);pulse(manual);}
 check(manual.flow->encounter()->combat()->cast() && manual.flow->encounter()->combat()->cast()->phase==CP::Result,"Manual Arrow settles");manual.act(AcknowledgeAction{});ready(arrow);ready(manual);
 check(arrow.w.sessionState().journeyRandom()==manual.w.sessionState().journeyRandom(),"Quick Arrow uses ordinary RNG draws");
 for(unsigned id=0;id<30;++id)check(xeen_state::sameCharacter(arrow.p.roster.at(id),manual.p.roster.at(id)) && xeen_state::sameInputs(*arrow.p.roster.combatInputs(id),*manual.p.roster.combatInputs(id)),"Quick Arrow ordinary owner write-back");
 for(unsigned kind=0;kind<5;++kind) {
  auto s=casterSource(in,XeenCharacterClass::Sorcerer,kind==0?255:kind==1?45:kind==2?40:kind==3?42:20);
  if(kind==1)s.characters[s.activeRosterIds[0]].currentSp=0;
  if(kind==4)s.journey->treasure->gems=0;
  Fixture skip(in,s);engage(skip);combat=skip.flow->encounter()->combat();const auto before=skip.p.roster.at(s.activeRosterIds[0]);const auto cursor=skip.w.sessionState().journeyRandom();const auto minutes=skip.p.encounterContext->minutes;
  skip.act(QuickFightAction{});
  check(xeen_state::sameCharacter(before,skip.p.roster.at(s.activeRosterIds[0])) && cursor==skip.w.sessionState().journeyRandom() && minutes==skip.p.encounterContext->minutes,"Quick Cast refusal/skip has no debit, effect or RNG");
  if(kind==3)check(combat->participant()==0 && skip.flow->encounter()->notice().find("not supported yet")!=std::string::npos,"Unsupported Cast visibly retains actor");
  else {ready(skip);check(combat->participant()!=0 && !(combat->result().blockedMembers&1),"None/cost/forbidden Quick Cast spends one unblocked turn");}
 }
}
void m73QuickCasting(Inputs &in) {
 for(unsigned spell:{2u,17u,33u,38u,61u,62u,72u,73u}) {
  const auto cls=spell==17?XeenCharacterClass::Sorcerer:spell==33?XeenCharacterClass::Cleric:XeenCharacterClass::Druid;
  auto source=casterSource(in,cls,spell);Fixture quick(in,source),manual(in,source);engage(quick);engage(manual);
  auto &fight=*manual.flow->encounter()->combat();
  const auto owner=source.activeRosterIds[0];const auto before=manual.p.roster.at(owner);
  const auto time=*manual.p.encounterContext;const auto gems=manual.p.monsterTreasure->gems;
  const auto targets=xeenSpellTargets(manual.w,manual.c,manual.w.sessionState().monsters(23),fight.selectedTarget(),XeenLearnedSpellRules::group(spell));
  XeenCombatRandom expected(*manual.w.sessionState().journeyRandom());auto treasure=*manual.p.monsterTreasure;
  std::array<int,3> hp{};std::array<XeenActorStatus,3> statuses{};
  for(unsigned i=0;i<3 && targets[i];++i) {
   const auto &a=manual.w.sessionState().monsters(23).at(targets[i]->recordIndex);
   XeenSpellDamageCandidate effect(before.permanentLevel,before.temporaryLevel,a.statistics->magicResistance(),a.original.resourceId,
    *XeenLearnedSpellRules::supported(spell),a.statistics->physicalResistance(),a.status,a.statistics->monsterType());
   XeenConsequenceDraw draw{expected};check(effect.service(draw),"M73 parity expected cast did not finish");
   hp[i]=std::max(a.hp-effect.damage,0);statuses[i]=effect.status;
   if(!hp[i] && a.original.resourceId==6){XeenMonsterDropCandidate drop(treasure,a.id);check(drop.service(draw),"M73 parity drop did not finish");treasure=drop.treasure;}
  }
  quick.act(QuickFightAction{});manual.act(CastSpellAction{});manual.act(DialogKeyAction{'c'});
  for(unsigned n=0;n<1000;++n){if(fight.cast() && fight.cast()->phase==XeenCombatCastPhase::Result)break;pulse(quick);pulse(manual);}
  check(fight.cast() && fight.cast()->phase==XeenCombatCastPhase::Result,"M73 Cast result missing");
  check(!quick.flow->encounter()->combat()->cast(),"M73 Quick Fight did not retire cast");
  check(manual.w.sessionState().journeyRandom()==expected.continuation() && quick.w.sessionState().journeyRandom()==expected.continuation(),"M73 Cast/Quick Fight tape differs");
  const auto cost=XeenLearnedSpellRules::cost(spell,before.currentLevel());
  for(auto *f:{&manual,&quick}) {
   check(f->p.roster.at(owner).currentSp==before.currentSp-int(cost.sp) && f->p.monsterTreasure->gems==gems-cost.gems && *f->p.encounterContext==time,"M73 combat cost/time differs");
   for(unsigned i=0;i<3 && targets[i];++i){const auto &a=f->w.sessionState().monsters(23).at(targets[i]->recordIndex);check(a.hp==hp[i] && a.status==statuses[i],"M73 Cast/Quick Fight effect differs");}
  }
  manual.act(AcknowledgeAction{});ready(manual);ready(quick);
  check(fight.participant()!=0 && quick.flow->encounter()->combat()->participant()!=0,"M73 cast did not spend exactly the acting turn");
 }
}

void originalMonsterDropReceipts(Inputs &in) {
 std::string failures;
 for(bool group:{false,true})try {
  auto s=casterSource(in,XeenCharacterClass::Druid,62);
  for(auto &a:s.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
  const auto originals=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,23),in.statistics);
  const unsigned count=group?3:1;const std::array<unsigned,3> slots{7,8,9};
  XeenWorld query(in.mapLoader());std::bitset<256> common;common.set();
  for(unsigned i=0;i<count;++i){check(originals[slots[i]].original.resourceId==6,"Drop fixture must use original Orc slots");common &= xeenActorClosure(query.map(23),originals[slots[i]]);}
  bool placed=false;
  for(int y=0;y<16 && !placed;++y)for(int x=0;x<14 && !placed;++x)if(common[y*16+x] && common[y*16+x+1] && common[y*16+x+2]) {
   if(std::any_of(in.mainland.records.begin(),in.mainland.records.end(),[&](const auto &r){return r.y==y && (r.x==x+1 || r.x==x+2);}))continue;
   s.camera={23,x+2,y,XeenDirection::West};
   if(xeenPlayerRayRows(query,s.camera)<3)continue;
   for(unsigned i=0;i<count;++i){auto &a=s.map(23).monsters->slots[slots[i]];a.x=x;a.y=y;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;}
   placed=true;
  }
  check(placed,"Original Orc closure has no quiet contact approach");
  // Seed selection uses the ordinary attack rules and the independent M71
  // makeItem oracle. No staged treasure or scripted drop is injected.
  const auto owner=s.activeRosterIds[0];const auto &caster=s.characters[owner];
  auto approachActors=originals;
  for(auto &a:approachActors){a.x=a.y=-128;a.hp=0;a.lifecycle=XeenActorLifecycle::Defeated;a.activated=false;}
  for(unsigned i=0;i<count;++i){auto &a=approachActors[slots[i]];a.x=s.camera.x-2;a.y=s.camera.y;a.hp=1;a.lifecycle=XeenActorLifecycle::Present;a.activated=true;}
  XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
  for(unsigned i=0;i<6;++i){characters[i]=s.characters[s.activeRosterIds[i]];inputs[i]=s.journey->supplements[s.activeRosterIds[i]].inputs;}
  auto contact=s.camera;--contact.x;
  std::vector<std::pair<unsigned,XeenItem>> items;
  std::optional<XeenJourneyRandomState> contactRandom,finalRandom;
  for(unsigned seed=1;seed<10000 && !finalRandom;++seed) {
   XeenCombatRandom random(seed);std::vector<std::pair<unsigned,XeenItem>> generated;bool lethal=true;
   XeenRegionalOpportunityCandidate approach(query.map(23),approachActors,contact,characters,inputs,s.journey->context->year,63);
   XeenConsequenceDraw approachDraw{random};check(approach.service(approachDraw),"Drop contact response oracle incomplete");
   const auto afterContact=random.continuation();
   for(unsigned i=0;i<count;++i) {
    const auto &a=originals[slots[i]];XeenConsequenceDraw draw{random};
    if(group) {
     XeenSpellDamageCandidate hit(caster.permanentLevel,caster.temporaryLevel,a.statistics->magicResistance(),6,XeenLearnedSpell::Sparks,a.statistics->physicalResistance());
     check(hit.service(draw),"Drop spell seed oracle incomplete");lethal=hit.damage>=1;
    } else {
     XeenPhysicalPlayerCandidate hit(caster,s.journey->supplements[owner].inputs,*a.statistics,6,s.journey->context->year,false,s.journey->context->difficulty);
     check(hit.service(draw),"Drop melee seed oracle incomplete");lethal=hit.hit && hit.damage>=1;
    }
    if(!lethal)break;
    m42_test::StockOracle oracle{random.state(),random.position()};
    if(oracle.draw(1,100)<=10)generated.push_back(oracle.item(1));
    random=XeenCombatRandom(XeenJourneyRandomState{1,oracle.state,oracle.count});
   }
   if(lethal && generated.size()==(group?2u:1u)) {
    s.journey->random=XeenCombatRandom(seed).continuation();contactRandom=afterContact;finalRandom=random.continuation();items=generated;
   }
  }
  check(finalRandom.has_value(),"No successful original Orc item-drop seed");
  // Receipt delivery walks weapon, armor, then Misc staging, preserving the
  // target order within each category. Level-1 makeItem generates no accessory.
  std::stable_sort(items.begin(),items.end(),[](const auto &a,const auto &b){return a.first<b.first;});
  Fixture f(in,s);
  for(unsigned i=0;i<count;++i)XeenMonsterTestAccess::setLive(*f.flow,f.w,23,slots[i],1,true);
  const auto gold=f.p.monsterTreasure->gold;const auto gems=f.p.monsterTreasure->gems;
  f.act(NavigationAction::MoveForward);ready(f);
  check(f.flow->encounter()->combat()->participant()==0 && f.w.sessionState().journeyRandom()==contactRandom,("Contact approach: participant="+std::to_string(f.flow->encounter()->combat()->participant())+" rng="+std::to_string(f.w.sessionState().journeyRandom()->count)+" initial="+std::to_string(contactRandom->count)).c_str());
  if(group){f.act(CastSpellAction{});f.act(DialogKeyAction{'c'});}else f.act(AttackAction{});
  for(unsigned n=0;n<1000 && !XeenPartyInterfaceTestAccess::reward(*f.flow);++n) {
   const auto *combat=f.flow->encounter()->combat();
   if(combat && combat->cast() && combat->cast()->phase==XeenCombatCastPhase::Result)f.act(AcknowledgeAction{});
   else pulse(f);
  }
  check(XeenPartyInterfaceTestAccess::reward(*f.flow),"Successful Orc drop did not reach real Flow receipt");
  for(unsigned i=0;i<count;++i)check(f.w.sessionState().monsters(23)[slots[i]].lifecycle==XeenActorLifecycle::Defeated,"Group spell must kill every original Orc");
  check(f.w.sessionState().journeyRandom()==finalRandom,"Original attack/drop target order or makeItem RNG differs");
  using Phase=XeenMonsterDeliveryCandidate::Phase;
  for(unsigned i=0;i<items.size();++i)for(unsigned stage=0;stage<2;++stage) {
   const auto phase=stage?Phase::Found:Phase::Blank;
   check(XeenPartyInterfaceTestAccess::rewardPhase(*f.flow)==phase && XeenPartyInterfaceTestAccess::rewardCursor(*f.flow)==i,"Original receipt order differs");
   const auto record=XeenPartyInterfaceTestAccess::rewardRecord(*f.flow);
   check(record.recipient==owner && xeenSameItem(record.production.item,items[i].second) &&
    (record.miscellaneous?3u:record.armor?1u:0u)==items[i].first,"Found line lost original generated item/category/recipient");
   const auto wait=XeenPartyInterfaceTestAccess::rewardWait(*f.flow);
   check(wait && wait->acquired && wait->ticks==(stage?10u:1u) &&
    wait->deadline==PresentationClock::after(PresentationClock::fromMilliseconds(f.now),wait->ticks),"Receipt hold did not start on acquired frame with original ticks");
   const auto end=PresentationClock::ceilMilliseconds(wait->deadline);
   f.now=end-1;f.flow->beginCycle(++f.cycle);
   if(auto frame=f.flow->updatePresentation())f.present(*frame);
   check(XeenPartyInterfaceTestAccess::rewardPhase(*f.flow)==phase && XeenPartyInterfaceTestAccess::rewardCursor(*f.flow)==i,"Receipt advanced before its hold elapsed");
   f.now=std::max(end,PresentationClock::ceilMilliseconds(XeenPartyInterfaceTestAccess::rewardInputDeadline(*f.flow)));f.flow->beginCycle(++f.cycle);
   if(auto frame=f.flow->updatePresentation())f.present(*frame);
   check(f.w.sessionState().journeyRandom()==finalRandom && f.p.monsterTreasure->gold==gold &&
    !f.flow->canSave(),"Receipt hold consumed RNG, credited early or exposed save");
  }
  check(XeenPartyInterfaceTestAccess::rewardPhase(*f.flow)==Phase::Final && !XeenPartyInterfaceTestAccess::rewardWait(*f.flow),"Timed item holds did not reach final acknowledgment");
  f.now+=125;f.act(AcknowledgeAction{});quiet(f);
  check(f.p.monsterTreasure->gold==gold+10*count && !f.p.monsterTreasure->pending() && !f.p.monsterTreasure->storedItems() &&
   f.w.sessionState().journeyRandom()==finalRandom && f.p.monsterTreasure->gems==gems-(group?1:0),"Final receipt repeated drop, item, gold, gem cost or RNG");
  const auto &recipient=f.p.roster.at(owner);
  for(unsigned category:{0u,1u,3u}) {
   const auto &actual=category==0?recipient.weapons:category==1?recipient.armor:recipient.miscellaneous;
   check(std::count_if(actual.begin(),actual.end(),[](const auto &v){return v.id!=0;})==
    std::count_if(items.begin(),items.end(),[&](const auto &v){return v.first==category;}),"Receipt did not deliver each generated item exactly once");
   for(const auto &item:items)if(item.first==category)check(std::count_if(actual.begin(),actual.end(),[&](const auto &v){return xeenSameItem(v,item.second);})==
    std::count_if(items.begin(),items.end(),[&](const auto &v){return v.first==category && xeenSameItem(v.second,item.second);}),"Delivered inventory differs from independent makeItem oracle");
  }
  const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
  check(saved==XeenSaveFormat::encode(restored.snapshot()),"Successful original item receipt did not restore exactly");
  f.act(NavigationAction::TurnLeft);quiet(f);check(f.flow->canSave(),"Gameplay did not continue after original item receipt");
 }catch(const std::exception &e){failures+=(group?"group: ":"melee: ")+std::string(e.what())+"; ";}
 if(!failures.empty())throw std::runtime_error(failures);
}
void orderedShootBarrierTreasure(Inputs &in) {
 auto source=in.base();Fixture exchanged(in,source);exchange(exchanged,0,5);source=exchanged.snapshot();Fixture shoot(in,source);shoot.act(ShootAction{});
 check(XeenPartyInterfaceTestAccess::shootOwners(*shoot.flow)==shoot.p.party.activeOrder(),"Shoot captures reordered owners");
 const auto eligible=XeenPartyInterfaceTestAccess::shootEligible(*shoot.flow);const auto lanes=XeenPartyInterfaceTestAccess::lanes(*shoot.flow);
 for(unsigned n=0;n<6;++n){bool missile=false;for(const auto &w:shoot.p.roster.at(source.activeRosterIds[n]).weapons)missile=missile || w.frame==4;check(eligible[n]==missile,"Shoot eligibility follows new positions");check(std::any_of(lanes.begin(),lanes.end(),[&](const auto &p){return p.lane==n;})==missile,"Shoot lane follows new position");}quiet(shoot);
 auto city=emptyCity(in);city.activeRosterIds=source.activeRosterIds;city.camera={28,20,1,XeenDirection::East};
 const unsigned selected=0;const auto owner=city.activeRosterIds[selected];city.characters[owner].permanentLevel=10;
 unsigned seed=1;for(;seed<10000;++seed){XeenCombatRandom r(seed);if(r.draw(1,4)!=1 && r.draw(1,20)>=10)break;}city.journey->random=XeenCombatRandom(seed).continuation();
 Fixture barrier(in,city);const auto xp=barrier.p.roster.combatInputs(owner)->experience;barrier.act(InteractionAction{});barrier.act(SelectMemberAction{selected});quiet(barrier);
 check(barrier.p.roster.combatInputs(owner)->experience==xp+100,"Barrier selection/unlock XP writes reordered owner");
 for(unsigned n=1;n<6;++n)check(barrier.p.roster.combatInputs(city.activeRosterIds[n])->experience==city.journey->supplements[city.activeRosterIds[n]].inputs.experience,"Barrier preserves other owners");
 // Canonical defeated/accounted Orc provenance, restored before gameplay.
 source.camera={23,7,10,XeenDirection::North};for(auto &a:source.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 auto &survivor=source.map(23).monsters->slots[8];survivor.x=7;survivor.y=11;survivor.lifecycle=XeenActorLifecycle::Present;survivor.accounted=false;
 source.journey->treasure->pendingMask=512;source.journey->treasure->pendingGold=10;source.journey->treasure->armor[0]={9,{0,2,0,0}};
 source.journey->treasure->pendingMask|=8;source.journey->treasure->pendingGold=20;
 source.journey->treasure->miscellaneous[0]={3,{2,9,7,0}};save_test::bindTreasureSources(source);
 for(bool full:{false,true}) {
  auto start=source;
  for(auto id:start.activeRosterIds) {
   auto &c=start.characters[id];c.armor={};c.miscellaneous={};
   if(full) {c.weapons.fill({0,1,0,0});c.armor.fill({0,1,0,0});c.accessories.fill({0,1,0,0});c.miscellaneous.fill({10,37,7,0});}
  }
  Fixture loot(in,start);const auto gold=loot.p.monsterTreasure->gold;
  loot.act(NavigationAction::TurnLeft);for(unsigned n=0;n<100 && !XeenPartyInterfaceTestAccess::reward(*loot.flow);++n)pulse(loot);
  check(XeenPartyInterfaceTestAccess::reward(*loot.flow),"Reordered treasure delivery admitted");
  using RewardPhase=XeenMonsterDeliveryCandidate::Phase;
  const auto random=loot.w.sessionState().journeyRandom();
  const auto recipient=start.activeRosterIds[0];
  const unsigned phases=full?2:5;
  for(unsigned phase=0;phase<phases;++phase) {
   const auto expected=full?(phase?RewardPhase::Final:RewardPhase::Warning):
    phase==4?RewardPhase::Final:phase%2?RewardPhase::Found:RewardPhase::Blank;
   check(XeenPartyInterfaceTestAccess::reward(*loot.flow) &&
    XeenPartyInterfaceTestAccess::rewardPhase(*loot.flow)==expected &&
    loot.p.monsterTreasure->gold==gold && !loot.flow->canSave() &&
    loot.w.sessionState().journeyRandom()==random,"Warning/item/final phases preserve currency, RNG and save fence");
   if(!full) {
    // Transfer writes the original tail before Blank; Found compacts it.
    XeenItemCategory armor{},miscellaneous{};
    armor[phase==0?8:0]={0,2,0,0};
    if(phase>=2)miscellaneous[phase==2?8:0]={2,9,7,0};
    check(xeen_state::sameItemCategory(loot.p.roster.at(recipient).armor,armor) &&
     xeen_state::sameItemCategory(loot.p.roster.at(recipient).miscellaneous,miscellaneous),
     "Existing records publish one item at a time to first current owner");
    for(unsigned n=1;n<6;++n) {
     const auto owner=start.activeRosterIds[n];
     check(xeen_state::sameItemCategory(loot.p.roster.at(owner).armor,start.characters[owner].armor) &&
      xeen_state::sameItemCategory(loot.p.roster.at(owner).miscellaneous,start.characters[owner].miscellaneous),
      "Treasure does not use default first owner");
    }
   }
   const auto oldFrame=loot.flow->frame().presentation();const auto oldInput=loot.flow->displayedInput();
   const auto input=loot.flow->inputContext(oldFrame).dialog;
   check(input && input->key('z') && input->click(0,0) && input->rightClick(),"Monster treasure key/either-mouse acknowledgment");
   // A response may interrupt an item hold after the original draw/input
   // boundary; every phase is still individually acquired by Fixture::act.
   loot.now=std::max(loot.now,PresentationClock::ceilMilliseconds(XeenPartyInterfaceTestAccess::rewardInputDeadline(*loot.flow)));
   loot.act(phase%2?*input->rightClick():*input->key('z'));
   if(phase+1<phases) {
    const auto retained=XeenPartyInterfaceTestAccess::rewardPhase(*loot.flow);
    const auto cursor=XeenPartyInterfaceTestAccess::rewardCursor(*loot.flow);
    loot.flow->handle(AcknowledgeAction{},oldInput,oldFrame);
    check(XeenPartyInterfaceTestAccess::rewardPhase(*loot.flow)==retained &&
     XeenPartyInterfaceTestAccess::rewardCursor(*loot.flow)==cursor && loot.p.monsterTreasure->gold==gold &&
     loot.w.sessionState().journeyRandom()==random,"Stale phase input cannot skip, credit treasure or consume RNG");
   }
  }
  check(!XeenPartyInterfaceTestAccess::reward(*loot.flow) && loot.p.monsterTreasure->gold==gold+20,"Final acknowledgment credits retained currency exactly once");
 }
}
void unsupportedQuickRun(Inputs &in) {
 auto s=emptyCity(in);auto &actor=s.map(28).monsters->slots.at(0);actor.x=10;actor.y=10;actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;
 for(auto owner:s.activeRosterIds){s.characters[owner].quickOption=3;s.journey->supplements[owner].inputs.speed={30,0};}
 Fixture f(in,s);f.act(WaitAction{});ready(f);const auto *combat=f.flow->encounter()->combat();const auto member=combat->participant();const auto rng=f.w.sessionState().journeyRandom();const auto time=*f.p.encounterContext;
 f.act(QuickFightAction{});check(combat->participant()==member && combat->phase()==XeenCombatPhase::PlayerReady && f.w.sessionState().journeyRandom()==rng && *f.p.encounterContext==time && f.flow->encounter()->notice().find("not supported yet")!=std::string::npos,"Unsupported indoor Quick Run visibly retains turn/time/RNG");
}
void spellChecks(Inputs &in) {
 auto p=XeenPartyLoader().loadInitialCloudsParty(in.assets);auto c=p.roster.at(11);c.characterClass=XeenCharacterClass::Sorcerer;c.hasSpells=true;c.currentSp=300;c.permanentLevel=10;c.learnedSpells.emplace();for(auto &flag:*c.learnedSpells)flag=1;
 using Q=XeenQuickSpellCheck;c.currentSpell=255;check(XeenLearnedSpellRules::quickSpellCheck(c,0)==Q::NoSpell,"Quick Cast none");
 for(unsigned slot=0;slot<39;++slot) {
  c.currentSpell=slot;const auto id=XeenLearnedSpellRules::spellForSlot(*XeenLearnedSpellRules::categoryForClass(c.characterClass),slot);
  const auto result=XeenLearnedSpellRules::quickSpellCheck(c,1000);
  if(*id==45)check(result==Q::Supported,"Quick Cast supported arrow");
  if(*id==42)check(result==Q::Unsupported,"Unsupported system refuses");
  if(*id==40 || *id==75)check(result==Q::CombatForbidden,"Implemented exploration spell is forbidden in combat");
 }
 for(unsigned slot=0;slot<39;++slot)if(XeenLearnedSpellRules::spellForSlot(*XeenLearnedSpellRules::categoryForClass(c.characterClass),slot)==45)c.currentSpell=slot;
 c.currentSp=0;check(XeenLearnedSpellRules::quickSpellCheck(c,1000)==Q::InsufficientSp,"Quick Cast insufficient SP");
}
}
int main(int argc,char **argv){try{
 check(argc==2,"usage: party-interface-tests <installation>");auto game=xeenTestInstallationDetector().detect(argv[1]);check(bool(game),"Missing installation");Inputs in(*game);
 std::ostringstream captured;auto *before=std::cout.rdbuf(captured.rdbuf());
 try{const auto run=[&](const char *name,auto fn){try{fn(in);}catch(const std::exception &e){throw std::runtime_error(std::string(name)+": "+e.what());}};run("original item drops",originalMonsterDropReceipts);run("exchange",exchangeAndSave);run("time/rest",reorderedTimeAndRest);run("combat/options",combatAndOptions);run("quick casting",quickCasting);run("M73 quick casting",m73QuickCasting);run("ordered consequences",orderedShootBarrierTreasure);run("indoor quick Run",unsupportedQuickRun);run("spell checks",spellChecks);}catch(...){std::cout.rdbuf(before);throw;}
 std::cout.rdbuf(before);std::cout<<"Exchange, ordered ownership, Quick Fight, v8 and stale input passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
