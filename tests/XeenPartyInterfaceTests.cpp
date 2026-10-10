#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
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
 static void credit(XeenEventFlow &f){f._encounter->acknowledgeMonsterReward();}
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
 for(auto &a:*s.journey->vertigoActors)if(a.lifecycle==XeenActorLifecycle::Present) {
  a.hp=0;a.x=a.y=-128;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;
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
 Fixture rest(in,source);rest.flow->loadRestDream=[&]{return in.assets.restDreamImage();};rest.act(RestAction{});quiet(rest);
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
 f.act(QuickFightOptionsAction{});check(XeenPartyInterfaceTestAccess::optionMember(*f.flow)==0,"Options starts on actual actor");
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
 ready(f);f.act(QuickFightOptionsAction{});unsigned count=0;for(unsigned n=0;n<6;++n)if(combat->participants()&(1u<<n))++count;
 f.act(DialogKeyAction{InputKey::F1});check(XeenPartyInterfaceTestAccess::optionMember(*f.flow)==0,"Options-after-Run keeps active-party indexing");
 f.act(DialogKeyAction{InputKey::F1+count});check(XeenPartyInterfaceTestAccess::optionMember(*f.flow)==0,"Options-after-Run rejects empty slot");
 f.act(CancelInteractionAction{});
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
 remember.act(CastSpellAction{});remember.act(NavigationAction::MoveBackward);remember.act(NavigationAction::MoveBackward);const auto chosen=combat->cast()->slot;
 check(chosen!=prior,"Spell browsing fixture");remember.act(CancelInteractionAction{});check(remember.p.roster.at(owner).currentSpell==prior,"Cancelled spell selection preserves remembered slot");
 remember.act(CastSpellAction{});remember.act(NavigationAction::MoveBackward);remember.act(NavigationAction::MoveBackward);remember.act(AcknowledgeAction{});
 check(remember.p.roster.at(owner).currentSpell==chosen,"Accepted spell selection is tracked immediately");remember.act(CancelInteractionAction{});remember.act(CancelInteractionAction{});
 check(!combat->cast() && remember.p.roster.at(owner).currentSpell==chosen,"Outer Cast cancellation keeps selection");
 source=casterSource(in,XeenCharacterClass::Sorcerer,45);Fixture arrow(in,source),manual(in,source);engage(arrow);engage(manual);
 arrow.act(QuickFightAction{});manual.act(CastSpellAction{});for(unsigned n=0;n<source.characters[source.activeRosterIds[0]].currentSpell;++n)manual.act(NavigationAction::MoveBackward);manual.act(AcknowledgeAction{});manual.act(AcknowledgeAction{});manual.act(AcknowledgeAction{});
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
 source.camera={23,7,10,XeenDirection::North};for(auto &a:source.journey->actors){a.x=a.y=-128;a.hp=0;a.activated=false;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 auto &survivor=source.journey->actors[8];survivor.x=7;survivor.y=11;survivor.hp=in.base().journey->actors[8].hp;survivor.activated=true;survivor.lifecycle=XeenActorLifecycle::Present;survivor.accounted=false;
 source.journey->treasure->pendingMask=512;source.journey->treasure->pendingGold=10;source.journey->treasure->armor[0]={9,{0,2,0,0}};
 for(auto id:source.activeRosterIds)source.characters[id].armor={};Fixture loot(in,source);const auto gold=loot.p.monsterTreasure->gold;
 loot.act(NavigationAction::TurnLeft);for(unsigned n=0;n<100 && !XeenPartyInterfaceTestAccess::reward(*loot.flow);++n)pulse(loot);check(XeenPartyInterfaceTestAccess::reward(*loot.flow),"Reordered treasure delivery admitted");
 check(loot.p.roster.at(source.activeRosterIds[0]).armor[0].id==2,"Treasure delivered to first current owner");
 for(unsigned n=1;n<6;++n)check(!loot.p.roster.at(source.activeRosterIds[n]).armor[0].id,"Treasure does not use default first owner");
 XeenPartyInterfaceTestAccess::credit(*loot.flow);check(loot.p.monsterTreasure->gold==gold+10,"Treasure credit once");
}
void unsupportedQuickRun(Inputs &in) {
 auto s=emptyCity(in);auto &actor=s.journey->vertigoActors->at(0);actor.x=10;actor.y=10;actor.hp=in.service().journey->vertigoActors->at(0).hp;actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;actor.activated=true;
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
  if(*id==40)check(result==Q::CombatForbidden,"Ordinary combat-forbidden cast spends turn");
 }
 for(unsigned slot=0;slot<39;++slot)if(XeenLearnedSpellRules::spellForSlot(*XeenLearnedSpellRules::categoryForClass(c.characterClass),slot)==45)c.currentSpell=slot;
 c.currentSp=0;check(XeenLearnedSpellRules::quickSpellCheck(c,1000)==Q::InsufficientSp,"Quick Cast insufficient SP");
}
}
int main(int argc,char **argv){try{
 check(argc==2,"usage: party-interface-tests <installation>");auto game=xeenTestInstallationDetector().detect(argv[1]);check(bool(game),"Missing installation");Inputs in(*game);
 std::ostringstream captured;auto *before=std::cout.rdbuf(captured.rdbuf());
 try{const auto run=[&](const char *name,auto fn){try{fn(in);}catch(const std::exception &e){throw std::runtime_error(std::string(name)+": "+e.what());}};run("exchange",exchangeAndSave);run("time/rest",reorderedTimeAndRest);run("combat/options",combatAndOptions);run("quick casting",quickCasting);run("ordered consequences",orderedShootBarrierTreasure);run("indoor quick Run",unsupportedQuickRun);run("spell checks",spellChecks);}catch(...){std::cout.rdbuf(before);throw;}
 std::cout.rdbuf(before);std::cout<<"Exchange, ordered ownership, Quick Fight, v8 and stale input passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
