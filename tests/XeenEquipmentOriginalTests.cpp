// M67: level-2 equipment bought at the Ironworks counts in combat, Shoot and
// Rest. Read-only original resources; the five-item Ironworks stock is an
// explicitly artificial arrangement of items level-2 generation can produce.
#include "XeenTestInstallation.h"
#include "XeenPurchaseTestSupport.h"
#include "formats/xeen/XeenVocDuration.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenEquipmentPurchase.h"
#include "games/xeen/XeenServiceEconomy.h"
#include <iostream>
using namespace purchase_test;
namespace mmodern {
struct XeenRestTestAccess {
 static bool active(const XeenEventFlow &f){return bool(f._encounter->_rest);}
 static bool confirm(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Confirm;}
 static bool complete(const XeenEventFlow &f){return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Complete;}
};
}
namespace {
using Category=XeenInventoryCategory;
// Level-2 Ironworks offers (merchant tables, column 0): metal 46-49, fire 1-3,
// AC attribute 116-117, HP attribute 105-106.
const XeenItem kSword{49,1,0,0};      // Long sword, metal 49: +2 hit, +2 damage.
const XeenItem kCrossbow{3,30,0,0};   // Crossbow, fire 3: +4 fire damage.
const XeenItem kArmor{47,1,0,0};      // Padded armor, metal 47: strength 2 + 1.
const XeenItem kAcRing{116,1,0,0};    // Ring, +2 armor class.
const XeenItem kHpRing{105,1,0,0};    // Ring, +4 maximum HP.
void pulse(Fixture &f){f.now+=100;f.flow->beginCycle(++f.cycle);if(const auto frame=f.flow->updatePresentation())f.present(*frame);}
void wire(Fixture &f){
 f.flow->loadRestDream=[&]{return f.in.assets.restDreamImage();};
 f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){return xeenVocDurationNanoseconds(f.in.assets.readArchiveResource(name,archive));};
}
template<class C> void finish(C &c,XeenCombatRandom &rng) {
 for(unsigned n=0;n<1000;++n){XeenConsequenceDraw draw{rng,64,{}};if(c.service(draw))return;}
 throw std::runtime_error("Oracle candidate did not complete");
}
std::size_t proficientMember(const XeenSaveSnapshot &s) {
 for(std::size_t member=0;member<s.activeRosterIds.size();++member) {
  const auto &c=s.characters[s.activeRosterIds[member]];
  if(xeenItemProficient(c,Category::Weapons,kSword.id) && xeenItemProficient(c,Category::Weapons,kCrossbow.id) &&
   xeenItemProficient(c,Category::Armor,kArmor.id))return member;
 }
 throw std::runtime_error("No member can use the level-2 purchases");
}
// Buy the five items through the original Ironworks screens and leave.
XeenSaveSnapshot buy(Inputs &in,std::size_t &member) {
 auto s=service(in,8,3000);member=proficientMember(s);
 auto &shop=s.journey->serviceEconomy->wares[0][0];
 for(auto &category:shop)for(auto &item:category)item={};
 shop[0][0]=kSword;shop[0][1]=kCrossbow;shop[1][0]=kArmor;shop[2][0]=kAcRing;shop[2][1]=kHpRing;
 for(const auto &[category,item]:{std::pair{0u,kSword},std::pair{0u,kCrossbow},std::pair{1u,kArmor},std::pair{2u,kAcRing},std::pair{2u,kHpRing}})
  check(xeenPossibleMerchantItem(2,category,item),"Fixture item outside level-2 Ironworks generation");
 xeenValidateCurrentServiceEconomy(*s.journey->serviceEconomy);
 Fixture f(in,s);f.enter();
 std::uint32_t expected=3000;
 const auto purchase=[&](Category category,unsigned slot,const XeenItem &item) {
  f.quote(category,0,member);f.act(YesAction{});
  check(XeenPurchaseTestAccess::result(*f.flow),"Level-2 purchase did not publish");
  expected-=*xeenEquipmentPurchasePrice(category,item);
  check(f.p.monsterTreasure->gold==expected,"Level-2 purchase price differs from the item rules");
  (void)slot;
 };
 purchase(Category::Weapons,0,kSword);purchase(Category::Weapons,0,kCrossbow);purchase(Category::Armor,0,kArmor);
 purchase(Category::Accessories,0,kAcRing);purchase(Category::Accessories,0,kHpRing);
 f.leave();check(f.p.encounterContext->day==9,"Ironworks departure day");
 auto bought=f.snapshot();
 const auto &c=bought.characters[bought.activeRosterIds[member]];
 unsigned found=0;for(const auto *items:{&c.weapons,&c.armor,&c.accessories})for(const auto &item:*items)
  if(xeenSameItem(item,XeenItem{kSword.material,kSword.id,0,0}) || xeenSameItem(item,XeenItem{kCrossbow.material,kCrossbow.id,0,0}) ||
   xeenSameItem(item,XeenItem{kArmor.material,kArmor.id,0,0}) || xeenSameItem(item,XeenItem{kAcRing.material,kAcRing.id,0,0}) ||
   xeenSameItem(item,XeenItem{kHpRing.material,kHpRing.id,0,0}))++found;
 check(found==5,"Five level-2 items delivered unequipped");
 return bought;
}
// Equip the purchases in the quiet snapshot: the Equip operation itself is
// covered by the Items census and tests; this witness measures contributions.
void equip(XeenSaveSnapshot &s,std::size_t member) {
 auto &c=s.characters[s.activeRosterIds[member]];
 for(auto &w:c.weapons)w.frame=0;
 for(auto &w:c.weapons){if(xeenSameItem(w,XeenItem{kSword.material,kSword.id,0,0}))w.frame=1;if(xeenSameItem(w,XeenItem{kCrossbow.material,kCrossbow.id,0,0}))w.frame=4;}
 for(auto &a:c.armor){if(a.frame==3)a.frame=0;if(xeenSameItem(a,XeenItem{kArmor.material,kArmor.id,0,0}))a.frame=3;}
 for(auto &a:c.accessories){if(a.frame==8)a.frame=0;if(xeenSameItem(a,XeenItem{kAcRing.material,kAcRing.id,0,0}) || xeenSameItem(a,XeenItem{kHpRing.material,kHpRing.id,0,0}))a.frame=8;}
 xeenValidateCompletedEquipment(c);
}
void capacities(Inputs &in,const XeenSaveSnapshot &equipped,std::size_t member) {
 Fixture f(in,equipped);
 xeenValidateJourneyMelee(f.p); // Refused before M67 for every one of these materials.
 const auto owner=f.p.party.activeRosterIds()[member];const auto &c=f.p.roster.at(owner);const auto &input=*f.p.roster.combatInputs(owner);
 const XeenCharacterRulesContext context{std::uint32_t(f.p.encounterContext->year)};
 auto plain=c;for(auto *items:{&plain.weapons,&plain.armor,&plain.accessories})for(auto &item:*items)item.frame=0;
 check(XeenCharacterRules::maxHp(c,context)==XeenCharacterRules::maxHp(plain,context)+4,"HP ring adds 4 maximum HP");
 // The member keeps any other armor it already wore (helm, shield...); compare
 // against the same character without the two new AC items.
 auto without=c;
 for(auto &a:without.armor)if(a.material==kArmor.material && a.id==kArmor.id)a.frame=0;
 for(auto &a:without.accessories)if(a.material==kAcRing.material && a.id==kAcRing.id)a.frame=0;
 const int baseline=XeenCharacterRules::combatArmorClass(without,input,context);
 check(baseline>0,"AC baseline above the zero clamp");
 check(XeenCharacterRules::combatArmorClass(c,input,context)==baseline+2+1+2,"AC: padded 2, metal 47 +1, ring +2");
 check(XeenCharacterRules::sheetArmorClass(c,&input,context)==XeenCharacterRules::combatArmorClass(c,input,context),"Sheet and combat AC agree");
 std::cout<<"Level-2 capacities: owner "<<unsigned(owner)<<" max HP "<<XeenCharacterRules::maxHp(c,context)<<" AC "<<XeenCharacterRules::combatArmorClass(c,input,context)<<'\n';
}
void rest(Inputs &in,const XeenSaveSnapshot &equipped,std::size_t member) {
 // (7,7) facing North is the quiet mainland cell xeen_rest_original selects:
 // no nearby or ranged monster interrupts the Rest.
 auto s=equipped;s.camera={23,7,7,XeenDirection::North};s.food=8;
 for(auto owner:s.activeRosterIds){s.characters[owner].currentHp=1;s.characters[owner].currentSp=0;}
 Fixture f(in,s);wire(f);f.act(RestAction{});
 for(unsigned n=0;n<1000;++n) {
  if(XeenRestTestAccess::confirm(*f.flow)){f.act(DialogKeyAction{'y'});continue;}
  if(XeenRestTestAccess::complete(*f.flow)){f.act(AcknowledgeAction{});continue;}
  if(f.flow->canSave())break;
  check(!f.flow->encounter()->combat(),"Rest fixture entered combat");pulse(f);
 }
 check(f.flow->canSave() && f.p.encounterContext->minutes==s.journey->context->minutes+480,"Rest with level-2 equipment did not complete");
 const auto owner=f.p.party.activeRosterIds()[member];const auto &c=f.p.roster.at(owner);
 const auto max=XeenCharacterRules::maxHp(c,{std::uint32_t(f.p.encounterContext->year)});
 check(c.currentHp==max && max>=5,"Rest refills to the maximum including the HP ring");
 std::cout<<"Rest with level-2 equipment: HP "<<c.currentHp<<'\n';
}
void shoot(Inputs &in,const XeenSaveSnapshot &equipped,std::size_t member) {
 auto s=equipped;s.camera={23,9,11,XeenDirection::West};
 auto &a=s.journey->actors[9];a.x=7;a.y=11;a.activated=true; // Spawn HP, as in fight().
 for(auto owner:s.activeRosterIds)s.characters[owner].currentHp=1000;
 Fixture f(in,s);const auto hp=f.w.sessionState().actors()[9].hp;const auto minutes=f.p.encounterContext->minutes;
 f.act(ShootAction{});
 for(unsigned n=0;n<400 && !f.flow->canSave();++n){check(!f.flow->encounter()->combat(),"Shoot fixture entered combat");pulse(f);}
 check(f.flow->canSave() && f.flow->encounter()->journeyRefusal().empty(),("Shoot with a fire crossbow refused or unfinished: "+f.flow->encounter()->journeyRefusal()).c_str());
 check(f.p.encounterContext->minutes==minutes+10,"Outdoor Shoot charge");
 const auto after=f.w.sessionState().actors()[9].hp;
 std::cout<<"Shoot with level-2 equipment: actor HP "<<hp<<" -> "<<after<<" (owner "<<unsigned(f.p.party.activeRosterIds()[member])<<")\n";
}
void fight(Inputs &in,const XeenSaveSnapshot &equipped,std::size_t member) {
 auto s=equipped;s.camera={23,9,11,XeenDirection::West};
 auto &a=s.journey->actors[9];a.x=8;a.y=11;a.activated=true; // Spawn HP: live actors stay within their base HP.
 const auto owner=s.activeRosterIds[member];
 for(auto id:s.activeRosterIds)s.characters[id].currentHp=1000;
 s.journey->supplements[owner].inputs.speed.permanent=50; // Artificial: the equipped member acts first.
 Fixture f(in,s);f.act(NavigationAction::MoveForward);
 for(unsigned n=0;n<200 && !f.flow->encounter()->combat();++n)pulse(f);
 const auto *combat=f.flow->encounter()->combat();check(combat,"Contact did not start combat");
 for(unsigned n=0;n<400 && combat->phase()!=XeenCombatPhase::PlayerReady;++n)pulse(f);
 check(combat->phase()==XeenCombatPhase::PlayerReady,"Combat did not reach the player turn");
 check(combat->participant()==int(member),"Fastest equipped member acts first");
 const auto &actor=f.w.sessionState().actors()[9];const int hpBefore=actor.hp;
 const auto &c=f.p.roster.at(owner);const auto &input=*f.p.roster.combatInputs(owner);
 XeenCombatRandom oracle(combat->random().continuation());
 XeenPhysicalPlayerCandidate candidate(c,input,*actor.statistics,actor.original.resourceId,f.p.encounterContext->year,false,f.p.encounterContext->difficulty);
 finish(candidate,oracle);
 f.act(AttackAction{});
 XeenCombatResult result;
 for(unsigned n=0;n<400;++n){result=f.flow->encounter()->combatResult();
  if(result.operation==XeenCombatOperation::PlayerAttack && result.status==XeenCombatStatus::Advanced && result.attackStage==XeenCombatAttackStage::None)break;pulse(f);}
 check(result.operation==XeenCombatOperation::PlayerAttack && result.actingOwner && *result.actingOwner==owner,"Equipped member's attack did not publish");
 check(result.damage==candidate.damage && result.elementalDamage==candidate.elemental && result.elementalFrame==candidate.elementalFrame,
  "Published attack differs from the metal-sword oracle on the same RNG continuation");
 const auto &after=f.w.sessionState().actors()[9];
 if(candidate.damage>=hpBefore) check(after.lifecycle==XeenActorLifecycle::Defeated,"Lethal metal-sword attack defeats the actor");
 else check(after.hp==hpBefore-candidate.damage,"Actor HP after the metal-sword attack");
 std::cout<<"Fight with level-2 equipment: owner "<<unsigned(owner)<<" hit="<<candidate.hit<<" damage "<<candidate.damage<<" (physical "<<candidate.physical<<", elemental "<<candidate.elemental<<")\n";
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"Equipment original usage: installation");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation absent");
 Inputs in(*installation);std::size_t member=0;
 auto equipped=buy(in,member);equip(equipped,member);
 capacities(in,equipped,member);fight(in,equipped,member);shoot(in,equipped,member);rest(in,equipped,member);
 std::cout<<"Original-resource level-2 Ironworks equipment in combat, Shoot and Rest passed\n";return 0;
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
