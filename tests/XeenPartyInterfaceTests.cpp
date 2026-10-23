#include "XeenTestInstallation.h"
#include "formats/xeen/XeenVoc.h"
#include "XeenTrainingTestSupport.h"
#include "XeenMonsterTestAccess.h"
#include "XeenM42Evidence.h"
#include "XeenM49CombatSupport.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenCharacterRules.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include <algorithm>
#include <iostream>
#include <sstream>
using namespace training_test;
namespace mmodern {
struct XeenPartyInterfaceTestAccess {
 static auto shootOwners(const XeenEventFlow &f){return f._encounter->_shoot->owners;}
 static auto shootEligible(const XeenEventFlow &f){return f._encounter->_shoot->volley.eligible;}
 static auto lanes(const XeenEventFlow &f){return f._encounter->_projectiles;}
 static bool reward(const XeenEventFlow &f){return f._encounter->monsterReward();}
 static auto rewardPhase(const XeenEventFlow &f){return f._encounter->_monsterReceipt->phase;}
 static unsigned rewardCursor(const XeenEventFlow &f){return f._encounter->_monsterReceipt->cursor;}
 static std::uint64_t rewardInputDeadline(const XeenEventFlow &f){return f._ordinary.deadline;}
 static auto rewardWait(const XeenEventFlow &f){return f._treasureWait;}
 static const auto &rewardRecord(const XeenEventFlow &f){return f._encounter->_monsterReceipt->receipt.entries[f._encounter->_monsterReceipt->cursor];}
 static const XeenEventExecutionState *event(const XeenEventFlow &f){return f._pending?&f._pending->state:nullptr;}
 static const XeenRewardReceipt &monsterReceipt(const XeenEventFlow &f){return f._encounter->_monsterReceipt->receipt;}
 static bool exchanging(const XeenEventFlow &f){return f._exchange;}
 static auto optionMember(const XeenEventFlow &f){return f._quickFightMember;}
 static bool restComplete(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Complete;}
 static bool restConfirm(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Confirm;}
};
}
namespace {
void pulse(Fixture &f){f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);}
// Acknowledges every phase of a monster delivery once its input is accepted.
void quietTreasure(Fixture &f) {
 for(unsigned n=0;n<200 && XeenPartyInterfaceTestAccess::reward(*f.flow);++n) {
  f.now=std::max(f.now,PresentationClock::ceilMilliseconds(XeenPartyInterfaceTestAccess::rewardInputDeadline(*f.flow)))+125;
  f.act(AcknowledgeAction{});pulse(f);
 }
 check(!XeenPartyInterfaceTestAccess::reward(*f.flow),"Monster delivery did not finish");
}
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
 s.journey->purse->gems=1000;return s;
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
 remember.act(CastSpellAction{});remember.act(DialogKeyAction{'n'});remember.act(DialogKeyAction{'1'});const auto chosen=XeenLearnedSpellRules::rows(remember.p.roster.at(owner),false,false,XeenMapIdentity(28)).front();
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
  auto s=casterSource(in,XeenCharacterClass::Sorcerer,kind==0?255:kind==1?45:kind==2?40:kind==3?44:20);
  if(kind==1)s.characters[s.activeRosterIds[0]].currentSp=0;
  if(kind==4)s.journey->purse->gems=0;
  Fixture skip(in,s);engage(skip);combat=skip.flow->encounter()->combat();const auto before=skip.p.roster.at(s.activeRosterIds[0]);const auto cursor=skip.w.sessionState().journeyRandom();const auto minutes=skip.p.encounterContext->minutes;
  skip.act(QuickFightAction{});
  check(xeen_state::sameCharacter(before,skip.p.roster.at(s.activeRosterIds[0])) && cursor==skip.w.sessionState().journeyRandom() && minutes==skip.p.encounterContext->minutes,"Quick Cast refusal/skip has no debit, effect or RNG");
  {ready(skip);check(combat->participant()!=0 && !(combat->result().blockedMembers&1),"None/cost/forbidden Quick Cast spends one unblocked turn");}
 }
}
void m73QuickCasting(Inputs &in) {
 for(unsigned spell:{2u,17u,33u,38u,61u,62u,72u,73u,0u,5u,11u,14u,18u,22u,23u,25u,28u,35u,36u,37u,43u,47u,48u,54u,63u,65u}) {
  const auto cls=XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Druidic,spell)?XeenCharacterClass::Druid:XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Wizardry,spell)?XeenCharacterClass::Sorcerer:XeenCharacterClass::Cleric;
  auto source=casterSource(in,cls,spell);Fixture quick(in,source),manual(in,source);engage(quick);engage(manual);
  auto &fight=*manual.flow->encounter()->combat();
  const auto owner=source.activeRosterIds[0];const auto before=manual.p.roster.at(owner);
  const auto time=*manual.p.encounterContext;const auto gems=manual.p.purse->gems;
  const auto targets=xeenSpellTargets(manual.w,manual.c,manual.w.sessionState().monsters(23),fight.selectedTarget(),XeenLearnedSpellRules::range(spell));
  XeenCombatRandom expected(*manual.w.sessionState().journeyRandom());auto treasure=manual.w.treasureStaging().value();
  std::array<int,12> hp{};std::array<XeenActorStatus,12> statuses{};
  XeenSpellDamagePreparation preparation(*XeenLearnedSpellRules::supported(spell),before.currentLevel());XeenConsequenceDraw preparationDraw{expected};check(preparation.service(preparationDraw),"Quick preparation incomplete");
  for(unsigned i=0;i<targets.size() && targets[i];++i) {
   const auto &a=manual.w.sessionState().monsters(23).at(targets[i]->recordIndex);
   XeenSpellDamageCandidate effect(before.permanentLevel,before.temporaryLevel,a.statistics->magicResistance(),a.original.resourceId,
    *XeenLearnedSpellRules::supported(spell),a.statistics->physicalResistance(),a.status,a.statistics->monsterType(),&preparation);
   XeenConsequenceDraw draw{expected};check(effect.service(draw),"M73 parity expected cast did not finish");
   hp[i]=std::max(a.hp-effect.damage,0);statuses[i]=effect.status;
   if(!hp[i]){XeenMonsterDropCandidate drop(treasure,*a.statistics,a.id.mapId);check(drop.service(draw),"M73 parity drop did not finish");treasure=drop.treasure;}
  }
  if(spell==48){XeenMoonRayHealing healing;XeenConsequenceDraw draw{expected};check(healing.service(draw,manual.p,time.year),"Combat Moon Ray healing incomplete");}
  quick.act(QuickFightAction{});manual.act(CastSpellAction{});manual.act(DialogKeyAction{'c'});
  for(unsigned n=0;n<1000;++n){if(fight.cast() && fight.cast()->phase==XeenCombatCastPhase::Result)break;pulse(quick);pulse(manual);}
  check(fight.cast() && fight.cast()->phase==XeenCombatCastPhase::Result,"M73 Cast result missing");
  check(!quick.flow->encounter()->combat()->cast(),"M73 Quick Fight did not retire cast");
  check(manual.w.sessionState().journeyRandom()==expected.continuation() && quick.w.sessionState().journeyRandom()==expected.continuation(),"M73 Cast/Quick Fight tape differs");
  const auto cost=XeenLearnedSpellRules::cost(spell,before.currentLevel());
  for(auto *f:{&manual,&quick}) {
   check(f->p.roster.at(owner).currentSp==before.currentSp-int(cost.sp) && f->p.purse->gems==gems-cost.gems && *f->p.encounterContext==time,"M73 combat cost/time differs");
   for(unsigned i=0;i<targets.size() && targets[i];++i){const auto &a=f->w.sessionState().monsters(23).at(targets[i]->recordIndex);check(a.hp==hp[i] && a.status==statuses[i],"M73 Cast/Quick Fight effect differs");}
  }
  const auto successor=fight.cast()->successorPhase;
  manual.act(AcknowledgeAction{});
  if(successor!=XeenCombatPhase::VictoryAwaitingEnd && successor!=XeenCombatPhase::DisengagementPending) {
   ready(manual);ready(quick);
   check(fight.participant()!=0 && quick.flow->encounter()->combat()->participant()!=0,"Cast did not spend exactly the acting turn");
  }
 }
}

// M81 Part C: arriving over a cell with the Event flag calls the Event routine
// (0xd1a5) and clears WhoWill even when no record matches the new facing; an
// unflagged cell keeps it (approved: later passes are not modelled).
void whoWillArrival(Inputs &in) {
 auto source=in.service();
 for(auto &a:source.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 XeenWorld query(in.mapLoader());const auto &map=query.map(23);
 constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
 const auto standable=[&](int x,int y){
  for(unsigned d=0;d<4;++d)if(XeenMovement::localOutdoor(map,x,y,x+dx[d],y+dy[d],xeenJourneyContent().traversal)==XeenMovementResult::Moved)return true;
  return false;
 };
 for(bool flagged:{true,false}) {
  std::optional<XeenCamera> start;
  for(int y=1;y<15 && !start;++y)for(int x=1;x<15 && !start;++x) {
   const auto sample=query.sampleCell(23,x,y);
   if(!sample || bool(sample->cell->rawAttributes&kXeenAutomaticEventFlag)!=flagged || !standable(x,y))continue;
   // Neither the starting facing nor the one a right turn reaches runs a record.
   for(unsigned d=0;d<4 && !start;++d)
    if(!xeenRegionalEvent(in.mainland,XeenCamera{23,x,y,XeenDirection(d)}) && !xeenRegionalEvent(in.mainland,XeenCamera{23,x,y,XeenDirection((d+1)%4)}))
     start=XeenCamera{23,x,y,XeenDirection(d)};
  }
  check(start.has_value(),"No original map-23 cell and facings without Event records for the WhoWill arrival test");
  auto s=source;s.camera=*start;
  Fixture f(in,s);quiet(f);
  XeenTreasureStaging chosen;chosen.whoWill=3;XeenMonsterTestAccess::stageTreasureLive(*f.flow,f.w,chosen);
  f.act(NavigationAction::TurnRight);for(unsigned n=0;n<20;++n)pulse(f);quiet(f);
  check(f.w.treasureStaging().value().whoWill==(flagged?0:3),flagged?"Arrival over an Event cell kept WhoWill":"An unflagged cell cleared WhoWill");
 }
}
// M81 Part C: the 90 original Clouds records drive drops through their own
// fields (plan table): the IDs per drop field, Orc 10/0/1, Darzog 0/50/6. In the
// Arena every record stages nothing and draws nothing.
void originalTreasureRecords(Inputs &in) {
 check(in.statistics.size()==90,"Clouds xeen.mon holds 90 records");
 const std::array<std::vector<unsigned>,7> fields{std::vector<unsigned>{},{5,6,7},{10,17,18,74},{24,25,29,30,31,35,38,42,75},{43,45,50,76,80},{82,84,85},{87}};
 for(unsigned id=0;id<in.statistics.size();++id) {
  const auto &record=in.statistics[id];unsigned expected=0;
  for(unsigned field=1;field<fields.size();++field)if(std::count(fields[field].begin(),fields[field].end(),id))expected=field;
  check(record.dropLevel()==expected,("Original drop field differs for monster "+std::to_string(id)).c_str());
  XeenCombatRandom none(std::vector<XeenCombatRandom::Draw>{});XeenConsequenceDraw draw{none,64,{}};
  XeenMonsterDropCandidate arena(XeenTreasureStaging{},record,XeenMapIdentity(85));
  check(arena.service(draw) && !none.position() && arena.treasure==XeenTreasureStaging{},"An Arena record staged treasure or drew");
 }
 check(in.statistics[6].gold()==10 && !in.statistics[6].gems() && in.statistics[87].gems()==50 && !in.statistics[87].gold() && !in.statistics[89].dropLevel(),
  "Orc, Darzog and Lord Xeen treasure inputs");
}
// Kills original monsters of the given resources by melee or one group Sparks
// and follows their treasure through the real Flow receipt. Seed selection uses
// the ordinary attack rules, the original record (gold, gems, drop field) and
// the independent M71 makeItem oracle. No staged treasure or scripted drop.
// With fillMidway every pack is full but one slot of the first item's category:
// that item fills it and the rest are lost (0x2431b), the currency still credited.
void monsterDropReceipts(Inputs &in,XeenMapIdentity map,const std::vector<unsigned> &resources,unsigned first,bool group,unsigned wantItems,bool wantAccessory,bool fillMidway=false) {
 auto s=casterSource(in,XeenCharacterClass::Druid,62);
 const auto originals=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,map),in.statistics);
 if(!s.map(map).monsters) {
  // An unvisited map: its original table, every slot where the MOB places it.
  XeenSaveMonsterTable table;table.originalCount=std::uint16_t(originals.size());
  for(const auto &a:originals)table.slots.push_back({a.x,a.y,XeenActorLifecycle::Defeated,a.status,true,-1});
  s.map(map).monsters=table;
 }
 for(auto &a:s.map(map).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 std::vector<unsigned> slots;
 for(const auto resource:resources) {
  const auto before=slots.size();
  for(unsigned i=first;i<originals.size();++i)
   if(originals[i].original.resourceId==resource && originals[i].lifecycle!=XeenActorLifecycle::Disabled &&
    std::find(slots.begin(),slots.end(),i)==slots.end()){slots.push_back(i);break;}
  check(slots.size()==before+1,"Drop fixture resource absent from the original map");
 }
 const unsigned count=unsigned(slots.size());
 // The party acts before every target, so the oracle needs no monster turn.
 unsigned fastest=0;for(const auto slot:slots)fastest=std::max(fastest,originals[slot].statistics->speed());
 for(auto id:s.activeRosterIds){auto &speed=s.journey->supplements[id].inputs.speed;if(speed.permanent<=int(fastest))speed={int(fastest+1),0};}
 const auto records=in.events.load(map).records;
 XeenWorld query(in.mapLoader());std::bitset<256> common;common.set();
 for(const auto slot:slots)common &= xeenActorClosure(query.map(map),originals[slot]);
 bool placed=false;
 for(int y=0;y<16 && !placed;++y)for(int x=0;x<14 && !placed;++x)if(common[y*16+x] && common[y*16+x+1] && common[y*16+x+2]) {
  if(std::any_of(records.begin(),records.end(),[&](const auto &r){return r.y==y && (r.x==x+1 || r.x==x+2);}))continue;
  s.camera={map,x+2,y,XeenDirection::West};
  if(xeenPlayerRayRows(query,s.camera)<3)continue;
  for(const auto slot:slots){auto &a=s.map(map).monsters->slots[slot];a.x=x;a.y=y;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;}
  placed=true;
 }
 check(placed,"Original monster closure has no quiet contact approach");
 const auto owner=s.activeRosterIds[0];const auto &caster=s.characters[owner];
 auto approachActors=originals;
 for(auto &a:approachActors){a.x=a.y=-128;a.hp=0;a.lifecycle=XeenActorLifecycle::Defeated;a.activated=false;}
 for(const auto slot:slots){auto &a=approachActors[slot];a.x=s.camera.x-2;a.y=s.camera.y;a.hp=1;a.lifecycle=XeenActorLifecycle::Present;a.activated=true;}
 XeenConsequenceCharacters characters;XeenConsequenceInputs inputs;
 for(unsigned i=0;i<6;++i){characters[i]=s.characters[s.activeRosterIds[i]];inputs[i]=s.journey->supplements[s.activeRosterIds[i]].inputs;}
 auto contact=s.camera;--contact.x;
 constexpr unsigned chance[]{0,10,20,50,100,100,100};
 std::uint32_t gold=0,gems=0;
 for(const auto slot:slots){gold+=originals[slot].statistics->gold();gems+=originals[slot].statistics->gems();}
 std::vector<std::pair<unsigned,XeenItem>> items;
 std::optional<XeenJourneyRandomState> contactRandom,finalRandom;
 for(unsigned seed=1;seed<20000 && !finalRandom;++seed) {
  XeenCombatRandom random(seed);std::vector<std::pair<unsigned,XeenItem>> generated;bool lethal=true;
  XeenRegionalOpportunityCandidate approach(query.map(map),approachActors,contact,characters,inputs,s.journey->context->year,63);
  XeenConsequenceDraw approachDraw{random};check(approach.service(approachDraw),"Drop contact response oracle incomplete");
  const auto afterContact=random.continuation();
  for(unsigned i=0;i<count;++i) {
   const auto &a=originals[slots[i]];XeenConsequenceDraw draw{random};
   if(group) {
    XeenSpellDamageCandidate hit(caster.permanentLevel,caster.temporaryLevel,a.statistics->magicResistance(),a.original.resourceId,XeenLearnedSpell::Sparks,a.statistics->physicalResistance());
    check(hit.service(draw),"Drop spell seed oracle incomplete");lethal=hit.damage>=1;
   } else {
    XeenPhysicalPlayerCandidate hit(caster,s.journey->supplements[owner].inputs,*a.statistics,a.original.resourceId,s.journey->context->year,false,s.journey->context->difficulty);
    check(hit.service(draw),"Drop melee seed oracle incomplete");lethal=hit.hit && hit.damage>=1;
   }
   if(!lethal)break;
   const unsigned field=a.statistics->raw[45];
   m42_test::StockOracle oracle{random.state(),random.position()};
   if(field && oracle.draw(1,100)<=chance[field])generated.push_back(oracle.item(field));
   random=XeenCombatRandom(XeenJourneyRandomState{1,oracle.state,oracle.count});
  }
  const bool accessory=std::any_of(generated.begin(),generated.end(),[](const auto &v){return v.first==2;});
  if(lethal && generated.size()==wantItems && (!wantAccessory || accessory)) {
   s.journey->random=XeenCombatRandom(seed).continuation();contactRandom=afterContact;finalRandom=random.continuation();items=generated;
  }
 }
 check(finalRandom.has_value(),"No original item-drop seed with the requested drops");
 // Receipt delivery walks weapon, armor, accessory, then Misc staging,
 // preserving the target order within each category.
 std::stable_sort(items.begin(),items.end(),[](const auto &a,const auto &b){return a.first<b.first;});
 if(fillMidway) {
  for(auto id:s.activeRosterIds) {
   auto &c=s.characters[id];c.weapons.fill({0,1,0,0});c.armor.fill({0,1,0,0});c.accessories.fill({0,1,0,0});c.miscellaneous.fill({1,1,3,0});
  }
  auto &c=s.characters[owner];
  auto &free=items[0].first==0?c.weapons:items[0].first==1?c.armor:items[0].first==2?c.accessories:c.miscellaneous;free.back()={};
 }
 const std::size_t delivered=fillMidway?1:items.size();
 Fixture f(in,s);
 for(const auto slot:slots)XeenMonsterTestAccess::setLive(*f.flow,f.w,map,slot,1,true);
 const auto purseGold=f.p.purse->gold;const auto purseGems=f.p.purse->gems;
 f.act(NavigationAction::MoveForward);ready(f);
 check(f.flow->encounter()->combat()->participant()==0 && f.w.sessionState().journeyRandom()==contactRandom,("Contact approach: participant="+std::to_string(f.flow->encounter()->combat()->participant())+" rng="+std::to_string(f.w.sessionState().journeyRandom()->count)+" initial="+std::to_string(contactRandom->count)).c_str());
 if(group){f.act(CastSpellAction{});f.act(DialogKeyAction{'c'});}else f.act(AttackAction{});
 for(unsigned n=0;n<1000 && !XeenPartyInterfaceTestAccess::reward(*f.flow);++n) {
  const auto *combat=f.flow->encounter()->combat();
  if(combat && combat->cast() && combat->cast()->phase==XeenCombatCastPhase::Result)f.act(AcknowledgeAction{});
  else pulse(f);
 }
 check(XeenPartyInterfaceTestAccess::reward(*f.flow),"Successful drop did not reach real Flow receipt");
 for(const auto slot:slots)check(f.w.sessionState().monsters(map)[slot].lifecycle==XeenActorLifecycle::Defeated,"Every original target must die");
 check(f.w.sessionState().journeyRandom()==finalRandom,"Original attack/drop target order or makeItem RNG differs");
 check(f.w.treasureStaging().value().gold==gold && f.w.treasureStaging().value().gems==gems,"Staged record gold and gems differ");
 using Phase=XeenRewardPhase;
 for(unsigned i=0;i<delivered;++i)for(unsigned stage=0;stage<2;++stage) {
  const auto phase=stage?Phase::Found:Phase::Blank;
  check(XeenPartyInterfaceTestAccess::rewardPhase(*f.flow)==phase && XeenPartyInterfaceTestAccess::rewardCursor(*f.flow)==i,"Original receipt order differs");
  const auto record=XeenPartyInterfaceTestAccess::rewardRecord(*f.flow);
  check(record.owner==owner && xeenSameItem(record.item,items[i].second) &&
   unsigned(record.category)==items[i].first,"Found line lost original generated item/category/recipient");
  const auto wait=XeenPartyInterfaceTestAccess::rewardWait(*f.flow);
  check(wait && wait->acquired && wait->ticks==(stage?10u:1u) &&
   wait->deadline==PresentationClock::after(PresentationClock::fromMilliseconds(f.now),wait->ticks),"Receipt hold did not start on acquired frame with original ticks");
  const auto end=PresentationClock::ceilMilliseconds(wait->deadline);
  f.now=end-1;f.flow->beginCycle(++f.cycle);
  if(auto frame=f.flow->updatePresentation())f.present(*frame);
  check(XeenPartyInterfaceTestAccess::rewardPhase(*f.flow)==phase && XeenPartyInterfaceTestAccess::rewardCursor(*f.flow)==i,"Receipt advanced before its hold elapsed");
  f.now=std::max(end,PresentationClock::ceilMilliseconds(XeenPartyInterfaceTestAccess::rewardInputDeadline(*f.flow)));f.flow->beginCycle(++f.cycle);
  if(auto frame=f.flow->updatePresentation())f.present(*frame);
  check(f.w.sessionState().journeyRandom()==finalRandom && f.p.purse->gold==purseGold &&
   !f.flow->canSave(),"Receipt hold consumed RNG, credited early or exposed save");
 }
 check(XeenPartyInterfaceTestAccess::rewardPhase(*f.flow)==Phase::Receipt && !XeenPartyInterfaceTestAccess::rewardWait(*f.flow),"Timed item holds did not reach final acknowledgment");
 const auto &receipt=XeenPartyInterfaceTestAccess::monsterReceipt(*f.flow);
 for(std::size_t i=0;i<items.size();++i)check(bool(receipt.entries[i].owner)==(i<delivered) &&
  (i<delivered || receipt.entries[i].loss==XeenRewardLoss::GloballyFull),"Packs filling midway must lose exactly the later items");
 f.now+=125;f.act(AcknowledgeAction{});quiet(f);
 check(f.p.purse->gold==purseGold+gold && !f.w.treasureStaging().value().pending() && !f.w.treasureStaging().value().storedItems() &&
  f.w.sessionState().journeyRandom()==finalRandom && f.p.purse->gems==purseGems+gems-(group?1:0),"Final receipt repeated drop, item, gold, gems, gem cost or RNG");
 const auto &recipient=f.p.roster.at(owner);
 if(fillMidway)check(std::count_if(recipient.weapons.begin(),recipient.weapons.end(),[&](const auto &v){return xeenSameItem(v,items[0].second);})+
  std::count_if(recipient.armor.begin(),recipient.armor.end(),[&](const auto &v){return xeenSameItem(v,items[0].second);})+
  std::count_if(recipient.accessories.begin(),recipient.accessories.end(),[&](const auto &v){return xeenSameItem(v,items[0].second);})+
  std::count_if(recipient.miscellaneous.begin(),recipient.miscellaneous.end(),[&](const auto &v){return xeenSameItem(v,items[0].second);})==1,
  "The first item did not fill the last free slot");
 else for(unsigned category=0;category<4;++category) {
  const auto &actual=category==0?recipient.weapons:category==1?recipient.armor:category==2?recipient.accessories:recipient.miscellaneous;
  const auto occupied=[&](const XeenItem &v){return category==3?v.material!=0:v.id!=0;};
  check(std::count_if(actual.begin(),actual.end(),occupied)==
   std::count_if(items.begin(),items.end(),[&](const auto &v){return v.first==category;}),"Receipt did not deliver each generated item exactly once");
  for(const auto &item:items)if(item.first==category)check(std::count_if(actual.begin(),actual.end(),[&](const auto &v){return xeenSameItem(v,item.second);})==
   std::count_if(items.begin(),items.end(),[&](const auto &v){return v.first==category && xeenSameItem(v.second,item.second);}),"Delivered inventory differs from independent makeItem oracle");
 }
 const auto saved=XeenSaveFormat::encode(f.snapshot());Fixture restored(in,XeenSaveFormat::decode(saved));
 check(saved==XeenSaveFormat::encode(restored.snapshot()),"Successful original item receipt did not restore exactly");
 f.act(NavigationAction::TurnLeft);quiet(f);check(f.flow->canSave(),"Gameplay did not continue after original item receipt");
}
void originalMonsterDropReceipts(Inputs &in) {
 std::string failures;
 // Map 23 Orcs (field 1, slots 7-9 share a cell); map 3 Barbarians (field 4,
 // guaranteed) and a Ninja (field 3): higher levels, record gold, accessories
 // and multi-target order. Levels 2, 5 and 6 live on indoor maps; the rules
 // tapes cover them.
 for(bool group:{false,true})try {monsterDropReceipts(in,23,group?std::vector<unsigned>{6,6,6}:std::vector<unsigned>{6},7,group,group?2:1,false);}
  catch(const std::exception &e){failures+=(group?"Orc group: ":"Orc melee: ")+std::string(e.what())+"; ";}
 for(bool group:{false,true})try {monsterDropReceipts(in,3,group?std::vector<unsigned>{45,45}:std::vector<unsigned>{45},0,group,group?2:1,group);}
  catch(const std::exception &e){failures+=(group?"Map 3 group: ":"Map 3 melee: ")+std::string(e.what())+"; ";}
 try {monsterDropReceipts(in,3,{45,45},0,true,2,false,true);}catch(const std::exception &e){failures+="Map 3 full packs: "+std::string(e.what())+"; ";}
 // Map 3 Ninja (field 3, 50%): a level-3 drop.
 try {monsterDropReceipts(in,3,{24},0,false,1,false);}catch(const std::exception &e){failures+="Map 3 Ninja: "+std::string(e.what())+"; ";}
 if(!failures.empty())throw std::runtime_error(failures);
}
void orderedShootBarrierTreasure(Inputs &in) {
 auto source=in.base();Fixture exchanged(in,source);exchange(exchanged,0,5);source=exchanged.snapshot();Fixture shoot(in,source);shoot.act(ShootAction{});
 check(XeenPartyInterfaceTestAccess::shootOwners(*shoot.flow)==shoot.p.party.activeOrder(),"Shoot captures reordered owners");
 const auto eligible=XeenPartyInterfaceTestAccess::shootEligible(*shoot.flow);const auto lanes=XeenPartyInterfaceTestAccess::lanes(*shoot.flow);
 for(unsigned n=0;n<6;++n){bool missile=false;for(const auto &w:shoot.p.roster.at(source.activeRosterIds[n]).weapons)missile=missile || w.frame==4;check(eligible[n]==missile,"Shoot eligibility follows new positions");check(std::any_of(lanes.begin(),lanes.end(),[&](const auto &p){return p.lane==n;})==missile,"Shoot lane follows new position");}quiet(shoot);
 auto city=emptyCity(in);city.activeRosterIds=source.activeRosterIds;city.camera={28,20,1,XeenDirection::East};
 const unsigned selected=1;const auto owner=city.activeRosterIds[selected];city.characters[owner].permanentLevel=10;
 unsigned seed=1;for(;seed<10000;++seed){XeenCombatRandom r(seed);if(r.draw(1,4)!=1 && r.draw(1,20)>=10)break;}city.journey->random=XeenCombatRandom(seed).continuation();
 Fixture barrier(in,city);const auto xp=barrier.p.roster.combatInputs(owner)->experience;barrier.act(InteractionAction{});barrier.act(SelectMemberAction{selected});quiet(barrier);
 check(barrier.p.roster.combatInputs(owner)->experience==xp+100,"Barrier selection/unlock XP writes reordered owner");
 // The grate's WhoWill dialog (0xc794 -> 0x252d2) leaves its choice preferred:
 // later monster treasure goes first to that member (0x243d5).
 check(barrier.w.treasureStaging().value().whoWill==selected+1,"Barrier selection did not set WhoWill");
 {
  XeenTreasureStaging loot=barrier.w.treasureStaging().value();loot.gold=5;loot.armor[0]={0,2,0,0};loot.ready=true;
  XeenMonsterTestAccess::stageTreasureLive(*barrier.flow,barrier.w,loot);
  for(unsigned n=0;n<100 && !XeenPartyInterfaceTestAccess::reward(*barrier.flow);++n)pulse(barrier);
  check(XeenPartyInterfaceTestAccess::reward(*barrier.flow) && XeenPartyInterfaceTestAccess::rewardRecord(*barrier.flow).owner==owner,
   "Monster treasure after the grate did not prefer its WhoWill member");
  quietTreasure(barrier);
  XeenTreasureStaging left;left.whoWill=selected+1;
  check(barrier.w.treasureStaging().value()==left,"Delivery cleared WhoWill");
  // Space at a wall with no grate and no Event still calls the Event routine
  // (0xd2be), which clears WhoWill: later loot goes to the first member again.
  bool spaced=false;
  // The unlocked grate faces East; a turn over an Event cell may already clear,
  // so the grate's choice is restored before each Space.
  for(unsigned turn=0;turn<4 && !spaced;++turn) {
   barrier.act(NavigationAction::TurnLeft);quiet(barrier);
   if(barrier.c.direction==XeenDirection::East || xeenRegionalEvent(in.city,barrier.c))continue;
   XeenMonsterTestAccess::stageTreasureLive(*barrier.flow,barrier.w,left);
   barrier.act(InteractionAction{});quiet(barrier);
   spaced=!barrier.w.treasureStaging().value().whoWill;
  }
  check(spaced,"Space without a grate or Event did not clear WhoWill");
  loot.whoWill=0;XeenMonsterTestAccess::stageTreasureLive(*barrier.flow,barrier.w,loot);
  for(unsigned n=0;n<100 && !XeenPartyInterfaceTestAccess::reward(*barrier.flow);++n)pulse(barrier);
  check(XeenPartyInterfaceTestAccess::reward(*barrier.flow) &&
   XeenPartyInterfaceTestAccess::rewardRecord(*barrier.flow).owner==barrier.p.party.activeRosterIds()[0],
   "Loot after the WhoWill clear did not return to the first member");
  quietTreasure(barrier);
 }
 for(unsigned n=0;n<6;++n)if(n!=selected)check(barrier.p.roster.combatInputs(city.activeRosterIds[n])->experience==city.journey->supplements[city.activeRosterIds[n]].inputs.experience,"Barrier preserves other owners");
 // Defeated/accounted Orcs; their treasure is staged in the running program.
 source.camera={23,7,10,XeenDirection::North};for(auto &a:source.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 auto &survivor=source.map(23).monsters->slots[8];survivor.x=7;survivor.y=11;survivor.lifecycle=XeenActorLifecycle::Present;survivor.accounted=false;
 // Two Orcs' gold, the armor one dropped and the Misc item the other dropped.
 XeenTreasureStaging staged;staged.gold=20;staged.ready=true;staged.armor[0]={0,2,0,0};staged.miscellaneous[0]={2,9,7,0};
 for(bool full:{false,true}) {
  auto start=source;
  for(auto id:start.activeRosterIds) {
   auto &c=start.characters[id];c.armor={};c.miscellaneous={};
   if(full) {c.weapons.fill({0,1,0,0});c.armor.fill({0,1,0,0});c.accessories.fill({0,1,0,0});c.miscellaneous.fill({10,37,7,0});}
  }
  Fixture loot(in,start);const auto gold=loot.p.purse->gold;XeenMonsterTestAccess::stageTreasureLive(*loot.flow,loot.w,staged);
  loot.act(NavigationAction::TurnLeft);for(unsigned n=0;n<100 && !XeenPartyInterfaceTestAccess::reward(*loot.flow);++n)pulse(loot);
  check(XeenPartyInterfaceTestAccess::reward(*loot.flow),"Reordered treasure delivery admitted");
  using RewardPhase=XeenRewardPhase;
  const auto random=loot.w.sessionState().journeyRandom();
  const auto recipient=start.activeRosterIds[0];
  const unsigned phases=full?2:5;
  for(unsigned phase=0;phase<phases;++phase) {
   const auto expected=full?(phase?RewardPhase::Receipt:RewardPhase::Warning):
    phase==4?RewardPhase::Receipt:phase%2?RewardPhase::Found:RewardPhase::Blank;
   check(XeenPartyInterfaceTestAccess::reward(*loot.flow) &&
    XeenPartyInterfaceTestAccess::rewardPhase(*loot.flow)==expected &&
    loot.p.purse->gold==gold && !loot.flow->canSave() &&
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
   check(input && input->key('z') && input->click(0,0) && input->click(0,0),"Monster treasure key/either-mouse acknowledgment");
   // A response may interrupt an item hold after the original draw/input
   // boundary; every phase is still individually acquired by Fixture::act.
   loot.now=std::max(loot.now,PresentationClock::ceilMilliseconds(XeenPartyInterfaceTestAccess::rewardInputDeadline(*loot.flow)));
   loot.act(phase%2?*input->click(0,0):*input->key('z'));
   if(phase+1<phases) {
    const auto retained=XeenPartyInterfaceTestAccess::rewardPhase(*loot.flow);
    const auto cursor=XeenPartyInterfaceTestAccess::rewardCursor(*loot.flow);
    loot.flow->handle(AcknowledgeAction{},oldInput,oldFrame);
    check(XeenPartyInterfaceTestAccess::rewardPhase(*loot.flow)==retained &&
     XeenPartyInterfaceTestAccess::rewardCursor(*loot.flow)==cursor && loot.p.purse->gold==gold &&
     loot.w.sessionState().journeyRandom()==random,"Stale phase input cannot skip, credit treasure or consume RNG");
   }
  }
  check(!XeenPartyInterfaceTestAccess::reward(*loot.flow) && loot.p.purse->gold==gold+20,"Final acknowledgment credits retained currency exactly once");
 }
}
void indoorQuickRun(Inputs &in) {
 auto s=emptyCity(in);auto &actor=s.map(28).monsters->slots.at(0);actor.x=10;actor.y=10;actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;
 for(auto owner:s.activeRosterIds){s.characters[owner].quickOption=3;s.journey->supplements[owner].inputs.speed={30,0};}
 Fixture f(in,s);f.act(WaitAction{});ready(f);const auto rng=f.w.sessionState().journeyRandom();
 const auto &g=f.w.map(28).geometry;auto run=f.c;run.x=std::int8_t(std::uint8_t(g.runX));run.y=std::int8_t(std::uint8_t(g.runY));
 check(run.x!=f.c.x || run.y!=f.c.y,"Vertigo run position differs from the fight");
 // M85 Part 0: the landing Event is handled by the post-combat callers, never a Run refusal.
 {auto east=run;east.direction=XeenDirection::East;check(xeenRunDestination(f.w,in.city,east) && xeenRunDestination(f.w,in.city,run),"Vertigo Run admits both facings at the Event landing");}
 // Every member flees by Quick Fight Run; the party lands on Vertigo's own run position.
 for(unsigned n=0;n<2000 && f.flow->encounter()->combat();++n) {
  const auto *combat=f.flow->encounter()->combat();
  if(combat->phase()==XeenCombatPhase::PlayerReady && !combat->cast())f.act(QuickFightAction{});else pulse(f);
  check(f.flow->encounter()->notice().find("not supported yet")==std::string::npos,"Indoor Quick Run is admitted");
 }
 quiet(f);
 check(!f.flow->encounter()->combat() && f.c.mapId==run.mapId && f.c.x==run.x && f.c.y==run.y && f.c.direction==run.direction,
  "Indoor Run lands on the current map's run position, facing unchanged");
 check(f.w.sessionState().journeyRandom()!=rng,"Indoor Run draws its chance");
}
void mrWizard(Inputs &in) {
 for(bool city:{false,true}) {
  auto s=city?emptyCity(in):in.base();s.journey->purse->gems=25;
  Fixture f(in,s);
  const auto time=*f.p.encounterContext;const auto rng=f.w.sessionState().journeyRandom();const auto gold=f.p.purse->gold;
  const auto moved=f.flow->mrWizard();check(bool(moved),"Mr Wizard is admitted outside combat");f.present(*moved);
  const auto &g=f.w.map(28).geometry;
  check(f.c.mapId==XeenMapIdentity(28) && f.c.x==std::int8_t(std::uint8_t(g.runX)) && f.c.y==std::int8_t(std::uint8_t(g.runY)) &&
   f.c.direction==XeenDirection::East,"Mr Wizard lands on Vertigo's run position facing East");
  check(f.p.purse->gems==0 && f.p.purse->gold==gold,"Mr Wizard zeroes the gems only");
  check(*f.p.encounterContext==time && f.w.sessionState().journeyRandom()==rng,"Mr Wizard spends no time or draw");
  quiet(f);
  Fixture restored(in,f.snapshot());
  check(restored.c.mapId==f.c.mapId && restored.c.x==f.c.x && restored.c.y==f.c.y && restored.c.direction==f.c.direction &&
   restored.p.purse->gems==0,"Mr Wizard result saves and restores");
 }
}
// In combat the panel's Mr Wizard zeroes the gems and ends the fight at once;
// the Vertigo load follows combat's exit (DOS 0x1f28a, 0x2e989, 0x2ec3c).
void combatMrWizard(Inputs &in) {
 auto s=emptyCity(in);s.journey->purse->gems=25;auto &actor=s.map(28).monsters->slots.at(0);actor.x=10;actor.y=10;actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;
 const auto sleeper=s.activeRosterIds[5];s.characters[sleeper].conditions[unsigned(XeenCondition::Asleep)]=1;
 Fixture f(in,s);f.act(WaitAction{});ready(f);
 const auto time=*f.p.encounterContext;const auto rng=f.w.sessionState().journeyRandom();const auto gold=f.p.purse->gold;const auto fight=f.c;
 const auto moved=f.flow->mrWizard();check(bool(moved),"Mr Wizard is admitted at a combat turn");f.present(*moved);
 check(f.p.purse->gems==0 && f.p.purse->gold==gold,"Combat Mr Wizard zeroes the gems before the callers");
 check(*f.p.encounterContext==time && f.w.sessionState().journeyRandom()==rng,"Combat Mr Wizard spends no turn, time or draw");
 check(!f.flow->encounter()->combat() || f.flow->encounter()->combat()->phase()==XeenCombatPhase::Disengaged,"Combat Mr Wizard leaves combat at once");
 check(!f.flow->mrWizard(),"Mr Wizard cannot repeat while combat retires");
 for(unsigned n=0;n<200 && f.flow->encounter()->combat();++n)pulse(f);
 check(!f.flow->encounter()->combat(),"Combat Mr Wizard retires the fight");
 quiet(f);
 const auto &g=f.w.map(28).geometry;
 check(f.c.mapId==XeenMapIdentity(28) && f.c.x==std::int8_t(std::uint8_t(g.runX)) && f.c.y==std::int8_t(std::uint8_t(g.runY)) &&
  f.c.direction==XeenDirection::East && (f.c.x!=fight.x || f.c.y!=fight.y),"Combat Mr Wizard lands on Vertigo's run position facing East");
 check(f.p.purse->gems==0 && f.p.purse->gold==gold,"Combat Mr Wizard keeps the gold");
 check(!f.p.roster.at(sleeper).conditions[unsigned(XeenCondition::Dead)],"Without an escape nobody is abandoned");
 Fixture restored(in,f.snapshot());
 check(restored.c.mapId==f.c.mapId && restored.c.x==f.c.x && restored.c.y==f.c.y && restored.p.purse->gems==0,"Combat Mr Wizard result saves and restores");
}
// After an earlier escape the exit first lands on the current map's Run
// position (DOS 0x2eaf5..0x2eb69) and abandons a sleeping member; combat's
// delivery there (0x2eb85) pays staged gems, which the later load keeps.
void combatMrWizardEscape(Inputs &in) {
 XeenTreasureStaging treasure;treasure.gems=7;treasure.ready=true;
 for(std::uint32_t seed=1;seed<=80;++seed) {
  auto s=emptyCity(in);s.journey->purse->gems=25;s.journey->random=XeenCombatRandom(seed).continuation();
  auto &actor=s.map(28).monsters->slots.at(0);actor.x=10;actor.y=10;actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;
  const auto sleeper=s.activeRosterIds[5];s.characters[sleeper].conditions[unsigned(XeenCondition::Asleep)]=1;
  for(auto owner:s.activeRosterIds)s.characters[owner].currentHp=1000;
  Fixture f(in,s);XeenMonsterTestAccess::stageTreasureLive(*f.flow,f.w,treasure);
  f.act(WaitAction{});ready(f);f.act(RunAction{});
  for(unsigned n=0;n<1000;++n) {
   const auto *combat=f.flow->encounter()->combat();
   if(!combat || (combat->phase()==XeenCombatPhase::PlayerReady && !combat->cast()))break;
   pulse(f);
  }
  const auto *combat=f.flow->encounter()->combat();
  if(!combat || combat->phase()!=XeenCombatPhase::PlayerReady || combat->participants()==0x3f)continue;
  const auto moved=f.flow->mrWizard();check(bool(moved),"Mr Wizard is admitted after an escape");f.present(*moved);
  check(f.p.purse->gems==0,"The panel zeroes the gems before delivery");
  for(unsigned n=0;n<2000 && !f.flow->canSave();++n) {
   if(XeenPartyInterfaceTestAccess::reward(*f.flow))quietTreasure(f);else pulse(f);
  }
  check(f.flow->canSave(),"Escape Mr Wizard settles");
  const auto &g=f.w.map(28).geometry;
  check(f.c.mapId==XeenMapIdentity(28) && f.c.x==std::int8_t(std::uint8_t(g.runX)) && f.c.y==std::int8_t(std::uint8_t(g.runY)) &&
   f.c.direction==XeenDirection::East,"Escape Mr Wizard ends on Vertigo's run position facing East");
  check(f.p.purse->gems==treasure.gems,"Gems delivered at the Run landing survive the load");
  check(f.p.roster.at(sleeper).conditions[unsigned(XeenCondition::Dead)]==1,"The escape branch abandons the sleeping member");
  return;
 }
 throw std::runtime_error("No seed gave a successful escape before Mr Wizard");
}
void spellChecks(Inputs &in) {
 auto p=XeenPartyLoader().loadInitialCloudsParty(in.assets);auto c=p.roster.at(11);c.characterClass=XeenCharacterClass::Sorcerer;c.hasSpells=true;c.currentSp=300;c.permanentLevel=10;c.learnedSpells.emplace();for(auto &flag:*c.learnedSpells)flag=1;
 using Q=XeenQuickSpellCheck;c.currentSpell=255;check(XeenLearnedSpellRules::quickSpellCheck(c,0)==Q::NoSpell,"Quick Cast none");
 for(unsigned slot=0;slot<39;++slot) {
  c.currentSpell=slot;const auto id=XeenLearnedSpellRules::spellForSlot(*XeenLearnedSpellRules::categoryForClass(c.characterClass),slot);
  const auto result=XeenLearnedSpellRules::quickSpellCheck(c,1000);
  if(*id==45)check(result==Q::Supported,"Quick Cast supported arrow");
  if(*id==44)check(result==Q::CombatForbidden,"Beacon Quick Fight follows learned combat refusal");
  if(*id==40 || *id==75)check(result==Q::CombatForbidden,"Implemented exploration spell is forbidden in combat");
 }
 for(unsigned slot=0;slot<39;++slot)if(XeenLearnedSpellRules::spellForSlot(*XeenLearnedSpellRules::categoryForClass(c.characterClass),slot)==45)c.currentSpell=slot;
 c.currentSp=0;check(XeenLearnedSpellRules::quickSpellCheck(c,1000)==Q::InsufficientSp,"Quick Cast insufficient SP");
}
}
// The party on (8,2) facing the Root Event, with the first original monster
// and terrain cell ahead that keeps that monster in view without contact.
struct EventView { XeenSaveSnapshot source; std::size_t survivor=0; };
EventView eventView(Inputs &in) {
 auto source=in.service();source.camera={23,8,2,XeenDirection::North};
 for(auto &a:source.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 for(auto id:source.activeRosterIds)source.characters[id].armor={};
 const auto original=in.service();const auto &slots=original.map(23).monsters->slots;
 for(std::size_t slot=0;slot<slots.size();++slot)for(int y=5;y<=6;++y)for(int x=6;x<=10;++x) {
  if(std::abs(slots[slot].x-x)+std::abs(slots[slot].y-y)>8)continue;
  auto candidate=source;auto &survivor=candidate.map(23).monsters->slots[slot];
  survivor.x=x;survivor.y=y;survivor.lifecycle=XeenActorLifecycle::Present;survivor.accounted=false;
  try {
   Fixture probe(in,candidate);XeenTreasureStaging ready;ready.gold=1;
   XeenMonsterTestAccess::stageTreasureLive(*probe.flow,probe.w,ready);
   for(unsigned n=0;n<20;++n)pulse(probe);
   if(!XeenPartyInterfaceTestAccess::reward(*probe.flow) && probe.flow->canSave())return {candidate,slot};
  } catch(const std::exception &) {}
 }
 throw std::runtime_error("No original cell keeps a monster in view ahead of the treasure Event");
}
// Answers the Event's WhoWill and confirmation, records the Event and monster
// receipts, and stops once quiet (after the Event receipt unless turned away
// or walking).
struct EventReceipts { std::optional<XeenRewardReceipt> event,monster; };
void driveEventTreasure(Fixture &f,bool turned,EventReceipts &receipts,bool walking=false) {
 for(unsigned n=0;n<3000;++n) {
  const auto *state=XeenPartyInterfaceTestAccess::event(*f.flow);
  if(state && state->pendingPresentation) {
   check(!XeenPartyInterfaceTestAccess::reward(*f.flow),"Monster delivery began while the Event ran");
   const auto &request=state->pendingPresentation->request;
   if(request.kind==XeenPresentationKind::CharacterSelection)f.act(SelectMemberAction{0});
   else if(request.response==XeenPresentationResponseRequirement::YesNo)f.act(YesAction{});
   else {
    if(state->reward.phase==XeenRewardPhase::Receipt)receipts.event=state->reward.receipt;
    f.act(AcknowledgeAction{});pulse(f);
   }
   continue;
  }
  if(XeenPartyInterfaceTestAccess::reward(*f.flow)) {
   check(turned,"Ordinary delivery ignored the monster in view");
   if(XeenPartyInterfaceTestAccess::rewardPhase(*f.flow)==XeenRewardPhase::Receipt)
    receipts.monster=XeenPartyInterfaceTestAccess::monsterReceipt(*f.flow);
   f.act(AcknowledgeAction{});pulse(f);continue;
  }
  if(f.flow->canSave() && (turned || walking || receipts.event))return;
  pulse(f);
 }
 throw std::runtime_error("Event treasure order did not settle at "+std::to_string(f.c.x)+","+std::to_string(f.c.y)+
  (f.flow->encounter()->combat()?" in combat":"")+": "+f.flow->encounter()->notice());
}
unsigned carried(Fixture &f,XeenRewardCategory category,const XeenItem &item) {
 unsigned count=0;
 for(auto id:f.p.party.activeRosterIds())for(const auto &v:xeenRewardItems(f.p.roster.at(id),category))count+=xeenSameItem(v,item);
 return count;
}
// M81 Part B, original maze0023.evt: the Root Event ahead of (8,2), entered with
// Space (a confirmation, then the quest give). A monster in view defers ordinary delivery
// (0x24158) but not the delivery after the Events, which run in mode 9 (0x2414e).
// Monster loot is pending (a kill while that monster held an attack slot) or
// dormant (left by a full-party Run).
void eventTreasureOrder(Inputs &in) {
 const auto view=eventView(in);
 const XeenItem armor{0,2,0,0};
 for(bool dormant:{false,true}) {
  Fixture f(in,view.source);const auto gold=f.p.purse->gold;const auto roots=f.p.questItems.at(17);
  XeenTreasureStaging staged;staged.armor[0]=armor;if(!dormant){staged.gold=20;staged.ready=true;}
  XeenMonsterTestAccess::stageTreasureLive(*f.flow,f.w,staged);
  for(unsigned n=0;n<20;++n)pulse(f);
  check(!XeenPartyInterfaceTestAccess::reward(*f.flow),"A monster in view did not defer ordinary delivery");
  EventReceipts receipts;
  f.act(InteractionAction{});driveEventTreasure(f,false,receipts);
  check(receipts.event.has_value(),("Original treasure Event did not deliver: "+f.flow->encounter()->notice()).c_str());
  // Delivery order is weapons (the quest item, raw ID 99), then armor.
  check(receipts.event->count==2 && receipts.event->entries[0].quest && receipts.event->entries[0].item.id==99 &&
   xeenSameItem(receipts.event->entries[1].item,armor) && !receipts.event->gold && f.p.questItems.at(17)==roots+1,
   "The Event receipt must hold its quest item and the monster item, without monster gold");
  check(f.w.treasureStaging().value()==(dormant?XeenTreasureStaging{}:staged),"The Event did not restore exactly the stash");
  f.act(NavigationAction::TurnLeft);driveEventTreasure(f,true,receipts);
  const auto armors=carried(f,XeenRewardCategory::Armor,armor);
  if(dormant)check(!receipts.monster && armors==1 && f.p.purse->gold==gold,"Dormant monster items deliver once, with the Event treasure");
  else {
   check(receipts.monster && receipts.monster->count==1 && xeenSameItem(receipts.monster->entries[0].item,armor) && receipts.monster->gold==20,
    "Ordinary delivery after the Event did not hand the monster item over again");
   check(armors==2 && f.p.purse->gold==gold+20 && f.w.treasureStaging().value()==XeenTreasureStaging{},
    "Monster items must arrive twice and their gold once (original bug kept)");
  }
 }
}
// M81 Part C: the same order after a real kill. A Shoot kills an original Orc
// beside the monster in view, so its record gold and generated item stay
// pending (delivery deferred); Space then runs the treasure Event.
void liveKillEventTreasure(Inputs &in) {
 const auto view=eventView(in);
 const auto originals=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,23),in.statistics);
 std::size_t target=0;
 while(target<originals.size() && (target==view.survivor || originals[target].original.resourceId!=6))++target;
 check(target<originals.size(),"No second original Orc");
 std::optional<XeenSaveSnapshot> chosen;
 // The target stands in the Shoot line ahead, rows 1-3.
 for(std::uint32_t attempt=0;attempt<900 && !chosen;++attempt) {
  const std::uint32_t seed=attempt/3+1;const int row=3+int(attempt%3);
  auto s=view.source;auto &a=s.map(23).monsters->slots[target];
  a.x=8;a.y=row;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
  s.journey->random=XeenCombatRandom(seed).continuation();
  Fixture f(in,s);XeenMonsterTestAccess::setLive(*f.flow,f.w,23,target,1,true);
  f.act(ShootAction{});
  for(unsigned n=0;n<2000 && !f.flow->canSave() && !XeenPartyInterfaceTestAccess::reward(*f.flow);++n)pulse(f);
  const auto &staging=f.w.treasureStaging().value();
  const bool killed=f.w.sessionState().monsters(23)[target].lifecycle==XeenActorLifecycle::Defeated;
  if(killed && staging.storedItems() && staging.gold==10 && !XeenPartyInterfaceTestAccess::reward(*f.flow) && f.flow->canSave())chosen=s;
 }
 check(chosen.has_value(),"No Shoot seed kills the Orc with an item while the other stays in view");
 Fixture f(in,*chosen);XeenMonsterTestAccess::setLive(*f.flow,f.w,23,target,1,true);
 const auto gold=f.p.purse->gold;
 f.act(ShootAction{});
 for(unsigned n=0;n<2000 && !f.flow->canSave();++n)pulse(f);
 const auto staged=f.w.treasureStaging().value();
 check(staged.ready && staged.gold==10 && staged.storedItems(),"The live kill did not stage the Orc's treasure");
 XeenRewardCategory category=XeenRewardCategory::Weapons;XeenItem item;
 for(unsigned c=0;c<4;++c)for(const auto &v:staged.items(XeenRewardCategory(c)))
  if(!XeenTreasureStaging::emptySlot(v,XeenRewardCategory(c))){category=XeenRewardCategory(c);item=v;}
 const auto before=carried(f,category,item);
 EventReceipts receipts;
 f.act(InteractionAction{});driveEventTreasure(f,false,receipts);
 // Category then slot order: a weapon drop precedes the quest item it shares the array with.
 check(receipts.event && receipts.event->count==2 && receipts.event->entries[0].quest!=receipts.event->entries[1].quest &&
  xeenSameItem(receipts.event->entries[receipts.event->entries[0].quest?1:0].item,item) && !receipts.event->gold,
  "The Event receipt must hold its quest item and the killed Orc's item, without its gold");
 check(f.w.treasureStaging().value()==staged,"The Event did not restore the live kill's staging");
 f.act(NavigationAction::TurnLeft);driveEventTreasure(f,true,receipts);
 check(receipts.monster && receipts.monster->count==1 && xeenSameItem(receipts.monster->entries[0].item,item) && receipts.monster->gold==10,
  "Ordinary delivery did not hand the killed Orc's item over again with its gold");
 check(carried(f,category,item)==before+2 && f.p.purse->gold==gold+10 && f.w.treasureStaging().value()==XeenTreasureStaging{},
  "The killed Orc's item must arrive twice and its gold once (original bug kept)");
}
// One step toward (x,y) facing `facing` on map 23, never through a cell within
// one step of `avoid`; nothing once there.
std::optional<PlayerAction> stepAround(XeenWorld &world,const XeenCamera &camera,int x,int y,XeenDirection facing,int ax,int ay) {
 if(camera.x==x && camera.y==y)return camera.direction==facing?std::optional<PlayerAction>{}:std::optional<PlayerAction>{NavigationAction::TurnRight};
 const auto &map=world.map(23);constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
 std::array<int,256> first;first.fill(-1);std::queue<int> pending;
 const int start=camera.y*16+camera.x;first[start]=4;pending.push(start);
 while(!pending.empty() && first[y*16+x]<0) {
  const auto cell=pending.front();pending.pop();const int cx=cell%16,cy=cell/16;
  for(unsigned direction=0;direction<4;++direction) {
   const int nx=cx+dx[direction],ny=cy+dy[direction];
   if(nx<0 || nx>=16 || ny<0 || ny>=16 || first[ny*16+nx]>=0 || std::abs(nx-ax)+std::abs(ny-ay)<=1 ||
    XeenMovement::localOutdoor(map,cx,cy,nx,ny,xeenJourneyContent().traversal)!=XeenMovementResult::Moved)continue;
   first[ny*16+nx]=cell==start?int(direction):first[cell];pending.push(ny*16+nx);
  }
 }
 const auto direction=first[y*16+x];
 if(direction<0)throw std::runtime_error("No route around the sleeping Orc");
 return unsigned(camera.direction)==unsigned(direction)?PlayerAction{NavigationAction::MoveForward}:PlayerAction{NavigationAction::TurnRight};
}
// M81 Part C: an Orc killed in combat while the other Orc fights on, then the
// whole party Runs (0x2e932..0x2e946): its gold and readiness clear and the item
// stays dormant. Back at the treasure Event, that item is delivered once with
// the Event's own. The remaining Orc is put to sleep at a quiet boundary and
// the walk back keeps away from it; it is never the treasure source.
void runKillEventTreasure(Inputs &in) {
 auto source=eventView(in).source;
 for(auto &a:source.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 const auto originals=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,23),in.statistics);
 XeenWorld query(in.mapLoader());const auto &geometry=query.map(23).geometry;
 // Contact as in monsterDropReceipts: two original Orcs on a cell of their
 // common closure, the party two cells east facing West, far from the Event
 // and the run position, off Event cells.
 struct Fight { int x,y; std::array<std::size_t,2> orcs; };
 std::vector<Fight> fights;
 std::vector<std::size_t> all;
 for(std::size_t i=0;i<originals.size();++i)if(originals[i].original.resourceId==6)all.push_back(i);
 for(std::size_t i=0;i+1<all.size() && fights.size()<8;++i) {
  const auto common=xeenActorClosure(query.map(23),originals[all[i]]) & xeenActorClosure(query.map(23),originals[all[i+1]]);
  for(int y=0;y<16;++y)for(int x=0;x<14;++x) {
   if(!common[y*16+x] || !common[y*16+x+1] || !common[y*16+x+2])continue;
   if(std::abs(x-8)+std::abs(y-2)<6 || std::abs(x+2-int(geometry.runX))+std::abs(y-int(geometry.runY))<4)continue;
   if(std::any_of(in.mainland.records.begin(),in.mainland.records.end(),[&](const auto &r){return r.y==y && (r.x==x+1 || r.x==x+2);}))continue;
   if(xeenPlayerRayRows(query,XeenCamera{23,x+2,y,XeenDirection::West})<3)continue;
   fights.push_back({x,y,{all[i],all[i+1]}});x=14;y=16;
  }
 }
 check(!fights.empty(),"No quiet contact for two original Orcs away from the Event");
 std::string failures;
 for(std::uint32_t attempt=0;attempt<std::min<std::size_t>(240,30*fights.size());++attempt) {
  const auto &fight=fights[attempt%fights.size()];const std::uint32_t seed=attempt/std::uint32_t(fights.size())+1;
  const auto &orcs=fight.orcs;
  auto s=source;s.journey->random=XeenCombatRandom(seed).continuation();s.camera={23,fight.x+2,fight.y,XeenDirection::West};
  for(auto orc:orcs){auto &a=s.map(23).monsters->slots[orc];a.x=fight.x;a.y=fight.y;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;}
  bool arranged=false;
  try {
   Fixture f(in,s);for(auto orc:orcs)XeenMonsterTestAccess::setLive(*f.flow,f.w,23,orc,1,true);
   const auto gold=f.p.purse->gold;
   f.act(NavigationAction::MoveForward);ready(f);
   bool attacked=false;
   for(unsigned n=0;n<3000 && f.flow->encounter()->combat();++n) {
    const auto *combat=f.flow->encounter()->combat();
    if(combat->phase()==XeenCombatPhase::PlayerReady && !combat->cast()) {
     if(!attacked && combat->participant()==0){attacked=true;f.act(AttackAction{});}else f.act(RunAction{});
    } else pulse(f);
   }
   quiet(f);
   const auto &monsters=f.w.sessionState().monsters(23);
   const bool first=monsters[orcs[0]].lifecycle==XeenActorLifecycle::Defeated,second=monsters[orcs[1]].lifecycle==XeenActorLifecycle::Defeated;
   const auto staged=f.w.treasureStaging().value();
   if(first==second || !staged.dormant() || f.c.x!=geometry.runX || f.c.y!=geometry.runY)continue;
   const auto survivor=first?orcs[1]:orcs[0];const auto &rest=monsters[survivor];
   XeenRewardCategory category=XeenRewardCategory::Weapons;XeenItem item;
   for(unsigned c=0;c<4;++c)for(const auto &v:staged.items(XeenRewardCategory(c)))
    if(!XeenTreasureStaging::emptySlot(v,XeenRewardCategory(c))){category=XeenRewardCategory(c);item=v;}
   const auto before=carried(f,category,item);
   XeenMonsterTestAccess::statusLive(*f.flow,f.w,23,survivor,XeenActorStatus::Sleep);
   EventReceipts receipts;
   for(unsigned n=0;n<200;++n) {
    const auto step=stepAround(f.w,f.c,8,2,XeenDirection::North,rest.x,rest.y);
    if(!step)break;
    f.act(*step);driveEventTreasure(f,false,receipts,true);
    check(!f.flow->encounter()->combat(),"The walk back met the sleeping Orc");
   }
   // From here on a failure is a real one, not a rejected arrangement.
   arranged=true;
   check(f.c.x==8 && f.c.y==2 && f.c.direction==XeenDirection::North && !receipts.event,"The walk back reached the Event cell without treasure");
   check(f.w.treasureStaging().value()==staged,"Walking changed the dormant staging");
   f.act(InteractionAction{});driveEventTreasure(f,false,receipts);
   check(receipts.event && receipts.event->count==2 && receipts.event->entries[0].quest!=receipts.event->entries[1].quest &&
    xeenSameItem(receipts.event->entries[receipts.event->entries[0].quest?1:0].item,item) && !receipts.event->gold && !receipts.monster,
    "The Event receipt must deliver the dormant Orc item once with its own, without the Orc's gold");
   check(carried(f,category,item)==before+1 && f.p.purse->gold==gold && f.w.treasureStaging().value()==XeenTreasureStaging{},
    "A Run-dormant item must arrive once and its gold never");
   return;
  } catch(const std::exception &e) {if(arranged)throw;if(failures.size()<600)failures+=std::string(" ")+e.what();}
 }
 throw std::runtime_error("No seed kills one Orc with an item, lets the whole party Run and walk back:"+failures);
}
int main(int argc,char **argv){try{
 check(argc==2,"usage: party-interface-tests <installation>");auto game=xeenTestInstallationDetector().detect(argv[1]);check(bool(game),"Missing installation");Inputs in(*game);
 std::ostringstream captured;auto *before=std::cout.rdbuf(captured.rdbuf());
 try{const auto run=[&](const char *name,auto fn){try{fn(in);}catch(const std::exception &e){throw std::runtime_error(std::string(name)+": "+e.what());}};run("original treasure records",originalTreasureRecords);run("WhoWill arrival",whoWillArrival);run("original item drops",originalMonsterDropReceipts);run("Event treasure order",eventTreasureOrder);run("live kill Event treasure",liveKillEventTreasure);run("Run kill Event treasure",runKillEventTreasure);run("exchange",exchangeAndSave);run("time/rest",reorderedTimeAndRest);run("combat/options",combatAndOptions);run("quick casting",quickCasting);run("M73 quick casting",m73QuickCasting);run("ordered consequences",orderedShootBarrierTreasure);run("indoor quick Run",indoorQuickRun);run("Mr Wizard",mrWizard);run("combat Mr Wizard",combatMrWizard);run("combat Mr Wizard after escape",combatMrWizardEscape);run("spell checks",spellChecks);}catch(...){std::cout.rdbuf(before);throw;}
 std::cout.rdbuf(before);std::cout<<"Exchange, ordered ownership, Quick Fight, v8 and stale input passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
