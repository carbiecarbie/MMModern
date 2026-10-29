#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenEventValues.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenCombat.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace mmodern;
namespace mmodern {
struct XeenTrainingTestAccess {
 static void seed(XeenWorld &w,std::uint32_t state=7) {XeenJourneyRandomState r;r.state=state;w._sessionState._journeyRandom=r;}
};
}
namespace {
void check(bool v,const char *message) {if(!v)throw std::runtime_error(message);}

// Six members; member 0 a Paladin with a spell book, member 1 a Knight.
constexpr std::uint8_t kClasses[6]{1,0,3,4,8,5};
XeenPartyState party() {
 std::vector<std::uint8_t> roster(XeenRoster::kCharacterCount*XeenCharacter::kSerializedSize,0);
 for(unsigned i=0;i<6;++i) {
  const auto base=i*XeenCharacter::kSerializedSize;
  roster[base]='A';roster[base+1]=std::uint8_t('A'+i);
  roster[base+0x13]=kClasses[i];roster[base+0x23]=5;roster[base+0x14+2]=15;roster[base+0x14+6]=15;
  roster[base+0xa3]=kClasses[i]==0 || kClasses[i]==5?0:1;
  roster[base+342]=20;roster[base+344]=10;roster[base+346]=0x50;roster[base+347]=2;
 }
 std::vector<std::uint8_t> bytes(10,0xff);bytes.resize(XeenQuestItemFormat::kRequiredSize,0);
 bytes[0]=6;bytes[1]=6;for(unsigned i=0;i<6;++i)bytes[2+i]=std::uint8_t(i);
 auto p=XeenPartyLoader().loadFromResources(roster,bytes);
 for(unsigned i=0;i<6;++i) {
  XeenCombatInputs in;in.might={15,0};in.speed={14,0};in.accuracy={13,0};in.luck=XeenAttributeValue{12,0};
  in.resistances=XeenCombatResistances{};in.poisonResistance=XeenAttributeValue{0,0};in.experience=1000;
  XeenEventValueAccess::inputs(p,std::uint8_t(i))=in;
 }
 for(unsigned i=0;i<6;++i)p.roster.at(i).learnedSpells=XeenCharacter::XeenLearnedSpells{};
 XeenGameplayContext context;context.year=610;context.day=8;context.minutes=480;
 p.encounterContext=context;p.purse=XeenPurse{};p.worldFlags=XeenWorldFlags{};
 p.purse->gold=500;p.purse->gems=10;p.food=20;
 return p;
}
struct Owners {
 XeenPartyState p=party();XeenGameFlags flags;XeenCamera camera{{XeenSide::Clouds,50},5,5,XeenDirection::North};
 XeenEventScriptValues script;
 XeenEventValueOwners owners() {return {p,flags,camera,script};}
 XeenCharacter &c(unsigned i) {return p.roster.at(i);}
 XeenCombatInputs &in(unsigned i) {return *XeenEventValueAccess::inputs(p,std::uint8_t(i));}
};
XeenEventProbe probe(Owners &o,unsigned action,std::uint32_t value,unsigned member=0) {
 std::string reason;const auto r=xeenEventProbe(o.owners(),action,value,member,reason);
 if(!r)throw std::runtime_error("probe refused: "+reason);return *r;
}

// Operand widths and worked Clouds operands (decoder).
void widths() {
 const auto decode=[](std::uint8_t op,std::vector<std::uint8_t> p) {
  XeenEventRecord r;r.opcode=op;r.parameters=std::move(p);r.lengthField=std::uint8_t(5+r.parameters.size());
  return XeenEventDecoder::decode(r);
 };
 // maze0050.evt @574: If1 gold (mode 34, 32-bit) >= 2000 -> line 3.
 const auto gold=std::get<XeenDecodedEventInstruction>(decode(8,{0x22,0xd0,0x07,0,0,3})).operation;
 check(std::get<XeenEventConditional>(gold).value==2000 && std::get<XeenEventConditional>(gold).targetLine==3,"32-bit If operand");
 // maze0029.evt @4112: If1 minutes (mode 25, 16-bit) >= 301 -> line 2.
 const auto minutes=std::get<XeenDecodedEventInstruction>(decode(8,{0x19,0x2d,0x01,2})).operation;
 check(std::get<XeenEventConditional>(minutes).value==301,"16-bit If operand");
 // SetVar uses the same widths; maze0055.evt @335 is a byte mode (74, value 0).
 check(std::get<XeenEventSetVar>(std::get<XeenDecodedEventInstruction>(decode(27,{0x4a,0})).operation).mode==74,"byte SetVar");
 check(std::get<XeenEventSetVar>(std::get<XeenDecodedEventInstruction>(decode(27,{34,1,2,3,4})).operation).value==0x04030201,"32-bit SetVar");
 check(std::get<XeenEventSetVar>(std::get<XeenDecodedEventInstruction>(decode(27,{25,0x10,0x02})).operation).value==0x210,"16-bit SetVar");
 check(std::holds_alternative<XeenEventDecodeError>(decode(27,{34,1})),"truncated wide SetVar must not decode");
 // maze0073.evt @39: TakeOrGive_2 (77,0) then give (77,20).
 const auto test=std::get<XeenEventTakeOrGiveTest>(std::get<XeenDecodedEventInstruction>(decode(28,{0x4d,0,0x4d,0x14})).operation);
 check(test.opcode==28 && test.first.mode==77 && test.second.value==20,"TakeOrGive_2 operands");
 // maze0035.evt @2109 JumpRnd 10,10,10; maze0034.evt @2720 RndDamage 5,20 (third byte unread).
 const auto jump=std::get<XeenEventJumpRandom>(std::get<XeenDecodedEventInstruction>(decode(23,{10,10,10})).operation);
 check(jump.range==10 && jump.match==10 && jump.line==10,"JumpRnd operands");
 const auto damage=std::get<XeenEventRandomDamage>(std::get<XeenDecodedEventInstruction>(decode(33,{5,20,0})).operation);
 check(damage.type==5 && damage.maximum==20 && damage.unused==std::uint8_t(0),"RndDamage operands");
 check(std::get<XeenEventIfMapFlag>(std::get<XeenDecodedEventInstruction>(decode(42,{0xff,0x11})).operation).line==17,"IfMapFlag operands");
 check(std::get<XeenEventItemType>(std::get<XeenDecodedEventInstruction>(decode(45,{3})).operation).itemType==3,"ItemType operand");
 check(std::get<XeenEventSetCharacter>(std::get<XeenDecodedEventInstruction>(decode(15,{7})).operation).selector==7,"SetChar selectors decode");
}

void probes() {
 Owners o;
 const auto armor=probe(o,10,0).value;
 XeenLearnedSpellRules::applyPartyEffect(XeenLearnedSpell::Bless,*o.p.encounterContext,7);
 check(o.p.encounterContext->effects[XeenPartyEffect::Bless]==7 &&
  !o.p.encounterContext->effects[XeenPartyEffect::Heroism] && probe(o,10,0).value==armor+7,
  "Bless spell must reach the AC byte read by the Event owner");
 o.p.encounterContext->effects[XeenPartyEffect::Bless]=0;
 // A failed skill test leaves ifProc's local uninitialised (residue).
 check(probe(o,13,5).residue && !probe(o,99,5).residue && probe(o,99,5).value==0xffffffffu,"skill/party-skill failure values");
 o.c(0).skills[5]=1;check(!probe(o,13,5).residue && probe(o,13,5).value==5,"skill success");
 const XeenEventResidue offset{XeenEventResidue::Kind::RecordOffset,1393};
 check(xeenEventCompare({0,true},offset,7,XeenEventComparison::Equal)==false,"residue SI:DI never equals a different record offset");
 check(!xeenEventCompare({0,true},{XeenEventResidue::Kind::RecordOffset,7},7,XeenEventComparison::Equal),"matching offset is indeterminate");
 check(xeenEventCompare({0,true},{XeenEventResidue::Kind::Value,7},7,XeenEventComparison::Equal)==true,"previous test value carries");
 // maze0082.evt @790: If1 mode 37 reads temporary Might on map 82 (no full-Might substitute).
 o.camera.mapId={XeenSide::Clouds,82};o.in(0).might.temporary=3;
 check(probe(o,37,40).value==3,"mode 37 reads temporary Might without a map exception");
 // 69 is a flag, 93 a remainder, 76 the day.
 o.p.encounterContext->effects[XeenPartyEffect::Levitate]=5;check(probe(o,69,1).value==1,"Levitate reads as a flag");
 o.p.encounterContext->day=57;check(probe(o,93,0).value==7 && probe(o,76,0).value==57,"day of week is day%10");
 // 78/81 compare unsigned words: negative HP is not below maximum.
 check(probe(o,78,1).value==1,"HP at or below maximum");
 o.c(0).currentHp=-5;check(probe(o,78,1).value==0,"negative HP compares as a large unsigned word");
 check(probe(o,8,0).value==0xfffffffbu,"HP sign-extends");
 // 86..92 use the sheet rule: maze0027.evt mode 89 (Endurance).
 check(probe(o,89,0).value==unsigned(XeenCharacterRules::sheetStat(o.c(0),&o.in(0),3,{610})),"getStat reads the sheet value");
 check(probe(o,34,0).value==500 && probe(o,35,0).value==10 && probe(o,65,0).value==20 && probe(o,25,0).value==480,"party values");
 o.p.questItems=XeenCloudsQuestItems(XeenCloudsQuestItems::Counts{});
 auto counts=static_cast<XeenCloudsQuestItems::Counts>(o.p.questItems.counts());counts[26]=3;o.p.questItems=XeenCloudsQuestItems(counts);
 check(probe(o,105,2).value==3,"Megacredits counter (quest item 26)");
 std::string reason;check(!xeenEventProbe(o.owners(),107,1,0,reason),"Darkside character flags refuse");
 check(!xeenEventProbe(o.owners(),19,7,1,reason),"a non-caster's spell test uses a stale book and refuses");
 // Awards: nibbles, 73 and 81 remapped.
 o.c(2).awards[126]=1;check(!probe(o,15,73,2).residue && probe(o,15,62,2).residue,"award 73 is byte 62's high nibble");
}

void writes() {
 Owners o;const auto w=o.owners();
 // setValue (maze0055.evt @335: SetVar 74 value 0 clears every listed effect).
 o.p.encounterContext->effects[XeenPartyEffect::Levitate]=4;o.p.encounterContext->effects[XeenPartyEffect::WizardEye]=1;o.p.encounterContext->effects[XeenPartyEffect::WalkOnWater]=1;
 o.p.encounterContext->lightAndResistances[0]=9;o.p.encounterContext->lightAndResistances[5]=3;
 check(xeenEventSetValue(w,74,0,0).status==XeenEventWriteStatus::Applied,"SetVar 74");
 check(!o.p.encounterContext->effects[XeenPartyEffect::Levitate] && !o.p.encounterContext->effects[XeenPartyEffect::WizardEye] && !o.p.encounterContext->effects[XeenPartyEffect::WalkOnWater] &&
  !o.p.encounterContext->lightAndResistances[0] && !o.p.encounterContext->lightAndResistances[5],"SetVar 74 writes every field");
 check(xeenEventSetValue(w,74,2,0).status==XeenEventWriteStatus::Applied &&
  o.p.encounterContext->effects[XeenPartyEffect::WalkOnWater]==2 && o.p.encounterContext->effects[XeenPartyEffect::WizardEye]==2,"byte-valued party flags are admitted, as produced by Day spells");
 xeenEventSetValue(w,74,0,0); // Keep later grant/take checks independent of this assignment.
 check(xeenEventSetValue(w,84,2,0).status==XeenEventWriteStatus::Applied && o.camera.direction==XeenDirection::South,"SetVar 84 faces");
 check(xeenEventSetValue(w,83,4,0).status==XeenEventWriteStatus::Applied && o.script.damageType==4,"SetVar 83 damage type");
 check(xeenEventSetValue(w,25,1500,0).status==XeenEventWriteStatus::Unsupported,"minutes beyond a day refuse");
 xeenEventSetValue(w,37,0x1ff,1);check(o.in(1).might.temporary==0xff,"byte modes store the low byte");
 // Condition mode 18: 16 clears all, 6 sets one, others increment; 13..15 zero HP.
 xeenEventSetValue(w,18,3,0);xeenEventSetValue(w,18,3,0);check(o.c(0).conditions[3]==2,"condition increments");
 xeenEventSetValue(w,18,6,0);xeenEventSetValue(w,18,6,0);check(o.c(0).conditions[6]==1,"In Love is set");
 xeenEventSetValue(w,18,13,0);check(o.c(0).conditions[13]==1 && o.c(0).currentHp==0,"Dead zeroes HP");
 xeenEventSetValue(w,18,16,0);check(!o.c(0).conditions[3] && !o.c(0).conditions[13],"16 clears");
 // giveTake: grants saturate, takes wrap, sparkles follow 0x23278.
 o.c(3).personality.permanent=250;
 const auto give=xeenEventGive(w,47,20,3);check(o.c(3).personality.permanent==255 && give.sparkle,"stat grant saturates at 255 and sparkles");
 xeenEventTake(w,47,0x100+3,3);check(o.c(3).personality.permanent==252,"take subtracts the low byte");
 check(!xeenEventGive(w,12,1,3).sparkle && xeenEventGive(w,11,1,3).sparkle && xeenEventGive(w,34,1,3).sparkle,"sparkle table");
 check(o.p.purse->gold==501,"give 34 credits the purse");
 const auto taken=xeenEventTake(w,35,11,0);
 check(taken.status==XeenEventWriteStatus::NotEnough && taken.consumable==1 && o.p.purse->gems==10,"too few gems refuse unchanged");
 check(xeenEventTake(w,65,5,0).status==XeenEventWriteStatus::Applied && o.p.food==15,"food is taken");
 // Give 74 activates Walk on Water; take 74 clears it.
 xeenEventGive(w,74,1,0);check(o.p.encounterContext->effects[XeenPartyEffect::WalkOnWater]==1 && o.p.encounterContext->lightAndResistances[2]==1,"give 74");
 xeenEventTake(w,74,1,0);check(!o.p.encounterContext->effects[XeenPartyEffect::WalkOnWater] && !o.p.encounterContext->lightAndResistances[2],"take 74");
 // Awards: OR 1 on grant; removal ORs 0xf into the other nibble (original defect).
 o.c(4).awards[7]=2;xeenEventGive(w,15,7,4);check(o.c(4).awards[7]==3,"award grant ORs one");
 xeenEventTake(w,15,7,4);check(o.c(4).awards[7]==3 && o.c(4).awards[71]==15,"award removal sets the other nibble");
 check(!xeenEventGive(w,15,8,4).sparkle,"Convicted Thief has no sparkle");
 // Spells: a Paladin learns Clerical spells by ID; a Knight refuses.
 check(xeenEventGive(w,19,1,0).sparkle,"spell grant sparkles");
 check(xeenEventGive(w,19,1,1).status==XeenEventWriteStatus::Unsupported,"non-caster spell grant refuses");
 // Skills: a grant increments the stored byte and sparkles, whether or not the
 // skill's effect is implemented (Clouds grants 1, 6, 8, 9, 14..17 on maps 14, 19,
 // 30, 35, 37 and 49); a take clears it.
 for(unsigned skill:{1u,6u,7u,8u,9u,14u,15u,16u,17u}) {
  o.c(2).skills[skill]=0;const auto given=xeenEventGive(w,13,skill,2);
  check(given.status==XeenEventWriteStatus::Applied && given.sparkle && o.c(2).skills[skill]==1,"skill grant");
  check(xeenEventTake(w,13,skill,2).status==XeenEventWriteStatus::Applied && !o.c(2).skills[skill],"skill take");
 }
 check(xeenEventGive(w,13,18,2).status==XeenEventWriteStatus::Unsupported,"skill past the table refuses");
 // Take 21: the first matching item of the ID's category, then sorted.
 o.c(5).weapons[0].id=4;o.c(5).weapons[1].id=9;o.c(5).weapons[2].id=4;
 check(xeenEventTake(w,21,4,5).status==XeenEventWriteStatus::Applied && o.c(5).weapons[0].id==9 && o.c(5).weapons[1].id==4,"item take");
 check(xeenEventTake(w,21,20,5).status==XeenEventWriteStatus::Failed,"missing item fails");
 // Quest items are byte counters without a floor.
 check(xeenEventTake(w,21,90,5).status==XeenEventWriteStatus::Applied && o.p.questItems.at(8)==255,"quest take wraps");
}

// --- Interpreter-level operations on a detached synthetic map ---
XeenMap tile(XeenMapIdentity id) {
 XeenMap m;m.side=id.side;m.geometry.id=id.number;m.geometry.neighbors=std::array<std::uint16_t,4>{0,0,0,0};
 m.geometry.difficulties[3]=id.number==48?100:2;m.geometry.trapDamage=5; // Box difficulty and trap damage.
 for(auto &c:m.geometry.cells) {
  c.rawWord=0x1234;c.rawAttributes=0x59;c.flags=0x58;
  XeenIndoorWalls walls;walls.walls=std::array<std::uint8_t,4>{1,2,3,4};c.geometry=walls;c.surfaceIndex=1;
 }
 return m;
}
XeenObjectFile mob(XeenMapIdentity id) {
 XeenObjectFile f{id,"synthetic.mob",true,{}};
 f.entities.objects.push_back({9,9,0,0,7});f.entities.objects.push_back({6,6,0,0,16}); // Object 1 is a chest.
 f.entities.monsters.push_back({1,1,0,0,8});f.entities.monsters.push_back({100,100,0,0,8});
 return f;
}
XeenObjectFile chestFirst(XeenMapIdentity id) { // Object 0 is the chest.
 auto f=mob(id);std::swap(f.entities.objects[0],f.entities.objects[1]);return f;
}
struct World {
 XeenObjectFile (*objects)(XeenMapIdentity);XeenWorld source;std::unique_ptr<XeenWorld> w;
 World(XeenMapIdentity id=47,std::uint32_t seed=7,XeenObjectFile (*o)(XeenMapIdentity)=mob):objects(o),source{tile,o} {
  source.markEncounterSession(XeenEncounterEntry::Journey);XeenTrainingTestAccess::seed(source,seed);
  w=source.transitionCandidate();std::vector<XeenMonsterRecord> stats(9);
  for(auto &s:stats){s.raw[20]=20;s.raw[24]=1;s.raw[25]=16;s.raw[26]=1;s.raw[28]=2;}
  w->stageMapMonsters(objects(id),stats);
 }
};
XeenEventRecord row(unsigned line,unsigned op,std::vector<std::uint8_t> p={},std::size_t offset=0) {
 XeenEventRecord r;r.x=5;r.y=5;r.direction=4;r.line=std::uint8_t(line);r.opcode=std::uint8_t(op);r.parameters=std::move(p);
 r.lengthField=std::uint8_t(5+r.parameters.size());r.fileOffset=offset;return r;
}
struct Run {
 XeenEventExecutionStepResult result;std::vector<XeenPresentationRequest> requests;
};
Run run(XeenWorld &w,XeenPartyState &p,XeenGameFlags &flags,const std::vector<XeenEventRecord> &records,
  std::function<XeenPresentationResponse(const XeenPresentationRequest &)> respond={},XeenCamera camera={47,5,5,XeenDirection::North}) {
 XeenEventFile file{camera.mapId,"synthetic.evt",true,records};
 const auto scripts=[&](XeenMapIdentity){return XeenEventScript(file);};
 const auto texts=[](XeenMapIdentity id){XeenEventTextFile t;t.mapId=id;t.resourcePresent=true;t.strings={"Title","Body","Third"};return t;};
 Run out;XeenEventInterpreter vm;
 out.result=vm.begin(camera,p,flags,w,scripts,texts);
 for(unsigned n=0;n<64;++n) {
  auto *s=std::get_if<XeenEventExecutionSuspended>(&out.result);if(!s)break;
  out.requests.push_back(s->request);
  const auto answer=respond?respond(s->request):s->request.response==XeenPresentationResponseRequirement::Presented?
   XeenPresentationResponse(XeenPresentationResponse::Presented):XeenPresentationResponse(XeenPresentationResponse::Acknowledged);
  auto state=s->state;out.result=vm.resume(std::move(state),answer,p,w,scripts,texts);
 }
 if(const auto *e=std::get_if<XeenEventExecutionError>(&out.result))std::cerr<<"Event error: "<<e->message<<'\n';
 return out;
}
bool completed(const Run &r) {return std::holds_alternative<XeenEventExecutionCompleted>(r.result);}
XeenGameFlags finalFlags(const Run &r) {return std::get<XeenEventExecutionCompleted>(r.result).finalGameFlags;}

void randomOperations() {
 // JumpRnd (maze0035.evt @2109: 1..10, jump on 10): exactly one draw, bounds and equality.
 unsigned jumps=0;
 for(std::uint32_t seed=1;seed<=200;++seed) {
  World f(47,seed);auto p=party();XeenGameFlags flags;
  const auto before=*f.w->sessionState().journeyRandom();
  XeenCombatRandom expected(before);std::optional<std::uint32_t> value;while(!value)value=expected.draw(1,10);
  const auto r=run(*f.w,p,flags,{row(0,23,{10,10,10}),row(1,12,{0,0,20,1}),row(10,12,{0,0,20,2})});
  check(completed(r),"JumpRnd completes");
  check(*f.w->sessionState().journeyRandom()==expected.continuation(),"JumpRnd draws once from the Journey generator");
  const bool jumped=finalFlags(r).isSet(2);check(jumped==(*value==10) && finalFlags(r).isSet(1)!=jumped,"JumpRnd jumps only on equality");
  jumps+=jumped;
 }
 check(jumps>0 && jumps<200,"JumpRnd outcomes vary with the generator");
 // SelectRandomChar (maze0080.evt @3183): one 1..party-size draw selects the member.
 {
  World f(47,11);auto p=party();XeenGameFlags flags;
  XeenCombatRandom expected(*f.w->sessionState().journeyRandom());std::optional<std::uint32_t> member;
  while(!member)member=expected.draw(1,6);
  const auto r=run(*f.w,p,flags,{row(0,43),row(1,12,{0,0,16,100,0,0,0})});
  check(completed(r) && p.roster.combatInputs(std::uint8_t(*member-1))->experience==1100,"SelectRandomChar selects the drawn member");
  for(unsigned i=0;i<6;++i)if(i!=*member-1)check(p.roster.combatInputs(std::uint8_t(i))->experience==1000,"only the drawn member gains");
 }
 // SetChar 7 draws like SelectRandomChar; SetChar 2 (maze0079.evt @2746) only names the combat target.
 {
  World f(47,5);auto p=party();XeenGameFlags flags;
  const auto before=*f.w->sessionState().journeyRandom();
  const auto r=run(*f.w,p,flags,{row(0,15,{2}),row(1,12,{0,0,16,1,0,0,0})});
  check(completed(r) && *f.w->sessionState().journeyRandom()==before && p.roster.combatInputs(0)->experience==1001,
   "SetChar 2 keeps the first member and draws nothing");
 }
 // RndDamage (maze0034.evt @2720): the 1..20 draw precedes typed damage's own draws.
 {
  World f(47,9);auto p=party();XeenGameFlags flags;
  XeenCombatRandom expected(*f.w->sessionState().journeyRandom());std::optional<std::uint32_t> amount;
  while(!amount)amount=expected.draw(1,20);
  const auto after=expected.continuation();
  World g(47,9);auto q=party();
  g.w->retainEventRandom(after);
  g.w->applyEventDamage(q,*amount,XeenDamageType::Poison,0,false,false);
  const auto r=run(*f.w,p,flags,{row(0,33,{5,20,0})});
  check(completed(r) && p.roster.at(0).currentHp==q.roster.at(0).currentHp &&
   *f.w->sessionState().journeyRandom()==*g.w->sessionState().journeyRandom(),"RndDamage equals one draw then ordinary damage");
 }
}

void flowOperations() {
 // If loops: SetChar 0 makes If test every member, OR-ed (0x2721c).
 {
  World f;auto p=party();XeenGameFlags flags;
  p.roster.at(4).skills[5]=1;
  const auto r=run(*f.w,p,flags,{row(0,15,{0}),row(1,9,{13,5,5}),row(2,12,{0,0,20,1}),row(5,12,{0,0,20,2})});
  check(completed(r) && finalFlags(r).isSet(2) && !finalFlags(r).isSet(1),"If over the whole party ORs members");
 }
 {
  World f;auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{row(0,9,{13,5,5},1393),row(1,12,{0,0,20,1}),row(5,12,{0,0,20,2})});
  check(completed(r) && finalFlags(r).isSet(1),"A failed skill test falls through (residue differs from the operand)");
  World g;auto q=party();
  const auto indeterminate=run(*g.w,q,flags,{row(0,9,{13,5,5},5)});
  check(std::holds_alternative<XeenEventExecutionError>(indeterminate.result),"A residue equal to the operand is not guessed");
 }
 // TakeOrGive loop: SetChar 0 then XP to everyone, each sparkling once.
 {
  World f;auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{row(0,15,{0}),row(1,12,{0,0,16,0x10,0x27,0,0}),row(2,12,{0,0,16,1,0,0,0})});
  check(completed(r),"XP loop completes");
  for(unsigned i=0;i<6;++i)check(p.roster.combatInputs(std::uint8_t(i))->experience==1000+10000+1,"every member gains");
  unsigned sparkles=0;for(const auto &q:r.requests)sparkles+=q.kind==XeenPresentationKind::PortraitEffect;
  check(sparkles==6,"one sparkle per member per Event run");
 }
 // Gold grant ends the loop after the first member; the gold take refusal ends the Event.
 {
  World f;auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{row(0,15,{0}),row(1,12,{0,0,34,100,0,0,0}),row(2,12,{34,0xe8,0x03,0,0,0,0}),row(3,12,{0,0,20,1})});
  check(completed(r) && p.purse->gold==600 && !finalFlags(r).isSet(1),"gold given once; failed take ends the Event");
  check(r.requests.back().notEnough==std::uint8_t(0),"gold refusal message");
 }
 // maze0030.evt @2292 uses condition 3; the gated Clouds paths also give
 // conditions 2, 9 and 10 through the same TakeOrGive owner.
 for(const std::uint8_t condition:{2,3,9,10}) {
  World f;auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{row(0,12,{0,0,18,condition})});
  check(completed(r) && p.roster.at(0).conditions[condition]==1 && !p.roster.at(1).conditions[condition],"condition grant");
 }
 // maze0073.evt @39: TakeOrGive_2 (77,0)/(77,20): +20 AC only while the bonus is zero.
 {
  World f;auto p=party();XeenGameFlags flags;
  run(*f.w,p,flags,{row(0,28,{0x4d,0,0x4d,0x14})});
  run(*f.w,p,flags,{row(0,28,{0x4d,0,0x4d,0x14})});
  check(p.roster.combatInputs(0)->temporaryAc==20,"TakeOrGive_2 tests before giving");
 }
 // IfMapFlag: maze0050.evt @566 (monster 0) continues while present; 0xff tests every monster.
 {
  World f;auto p=party();XeenGameFlags flags;
  const auto present=run(*f.w,p,flags,{row(0,42,{0,15}),row(1,12,{0,0,20,1}),row(15,12,{0,0,20,2})});
  check(completed(present) && finalFlags(present).isSet(1) && !finalFlags(present).isSet(2),"present monster continues");
  const auto absent=run(*f.w,p,flags,{row(0,42,{1,15}),row(1,12,{0,0,20,1}),row(15,12,{0,0,20,2})});
  check(completed(absent) && finalFlags(absent).isSet(2),"parked monster jumps");
 }
 // NPC mode 0 (maze0027.evt @3096): Yes jumps, No continues.
 for(const bool yes:{true,false}) {
  World f;auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{row(0,5,{0,1,2,0,12}),row(1,12,{0,0,20,1}),row(12,12,{0,0,20,2})},
   [&](const XeenPresentationRequest &q) {
    if(q.response==XeenPresentationResponseRequirement::YesNo)return XeenPresentationResponse(yes?XeenPresentationResponse::Yes:XeenPresentationResponse::No);
    return XeenPresentationResponse(XeenPresentationResponse::Acknowledged);});
  check(completed(r) && finalFlags(r).isSet(yes?2:1) && !finalFlags(r).isSet(yes?1:2),"NPC confirmation branch");
 }
 // GiveMulti 101/106: treasure gems and party food, one draw each.
 {
  World f(47,3);auto p=party();XeenGameFlags flags;
  XeenCombatRandom expected(*f.w->sessionState().journeyRandom());std::optional<std::uint32_t> gems,food;
  while(!gems)gems=expected.draw(1,15);while(!food)food=expected.draw(1,6);
  const auto r=run(*f.w,p,flags,{row(0,20,{35,10,0,101,15,0,106,6,0})});
  check(completed(r) && p.purse->gems==10+10+*gems && p.food==20+*food,"GiveMulti random gems and food");
 }
 // ItemType 3 then a level-4 item (maze0025.evt @1516): a special-event Club.
 {
  World f(47,4);auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{row(0,45,{3}),row(1,20,{66,4,0,0,0,0})});
  bool club=false;
  for(unsigned i=0;i<6;++i)for(const auto &item:p.roster.at(i).weapons)club=club || item.id==3;
  check(completed(r) && club,"special-event item reaches the party");
 }
}

// A locked chest (sprite 16, object 1 or object 0): 1..4 trap draw, then thievery + 1..20.
void lockPicking() {
 // Without a selected object giveExt skips the lock (0x23597): no draw.
 {
  World f;auto p=party();XeenGameFlags flags;const auto before=*f.w->sessionState().journeyRandom();
  const auto r=run(*f.w,p,flags,{row(0,20,{34,100,0,0,0,0,0,0,0})});
  check(completed(r) && p.purse->gold==600 && *f.w->sessionState().journeyRandom()==before,"no selected object, no lock");
 }
 for(std::uint32_t seed=1;seed<64;++seed) {
  XeenJourneyRandomState start;start.state=seed;XeenCombatRandom expected(start);std::optional<std::uint32_t> trap,attempt;
  while(!trap)trap=expected.draw(1,4);
  if(*trap==1)continue; // The trap's own damage draws are covered by RndDamage.
  while(!attempt)attempt=expected.draw(1,20);
  // The selection byte is one-based (0x98d9), so object 0 is locked like any other.
  for(const std::size_t object:{1u,0u})for(const unsigned map:{47u,48u}) {
   World f(map,seed,object?mob:chestFirst);auto p=party();XeenGameFlags flags;
   auto chest=row(0,20,{34,100,0,0,0,0,0,0,0});chest.x=chest.y=6;
   const auto r=run(*f.w,p,flags,{chest},{},XeenCamera{XeenMapIdentity(map),6,6,XeenDirection::North});
   const int box=map==48?100:2;
   const bool picked=XeenCharacterRules::thievery(p.roster.at(0))+int(*attempt)>=box;
   check(completed(r) && *f.w->sessionState().journeyRandom()==expected.continuation(),"lock draws once each in order");
   bool message=false;for(const auto &q:r.requests)message=message || (q.lockPicked==picked && q.title==p.roster.at(0).name);
   check(message,"lock message names the recipient");
   check(p.purse->gold==(picked?600u:500u),"a failed pick ends the Event before the gold");
   check(p.roster.combatInputs(0)->experience==(picked?1000u+5u*2u*10u:1000u),"a pick adds level x difficulty x 10 experience");
   check(f.w->objectFrame({XeenMapIdentity(map),object})==std::optional<unsigned>(picked?1u:0u),"the chest stays open only when picked");
  }
  return;
 }
 throw std::runtime_error("no trap-free seed");
}

void specialItem() {
 XeenCombatRandom random(5);
 XeenItemCandidate special(4,std::optional<int>(43));
 for(;;) {XeenConsequenceDraw draw{random,64,{}};if(special.service(draw))break;}
 check(special.category()==1 && special.item().id==8,"ItemType 43 is armor 8 (shield)");
}

// B5 / R11: generated Misc items retain the base kind in material, while
// generation replaces the ID with the spell or effect.
void miscMaterial() {
 XeenCombatRandom random(5);
 XeenItemCandidate generated(1,std::optional<int>(64));
 for(;;) {XeenConsequenceDraw draw{random,64,{}};if(generated.service(draw))break;}
 const auto item=generated.item();
 check(generated.category()==3 && item.material==4 && item.id!=item.material && item.id<=21,
  "B5 fixture must be a generated Misc item with different material and ID");
 Owners o;o.c(0).miscellaneous[0]=item;
 check(probe(o,21,60+item.material).value==60+item.material,"B5 If 21 must find Misc material");
 check(probe(o,21,60+item.id).value==0xffffffffu,"B5 If 21 must not find Misc ID");
}

// Random currency/food words (0x23770..0x237de) and the helper's signed word divisor (0x45ba3..0x45bac).
void wordDraws() {
 check(!xeenEventWordDrawBound(0) && !xeenEventWordDrawBound(0x10000),"a zero word divides by zero");
 check(*xeenEventWordDrawBound(1)==1 && *xeenEventWordDrawBound(65537)==1,"only the low word is the argument");
 check(*xeenEventWordDrawBound(0x7fff)==0x7fff && *xeenEventWordDrawBound(0x8000)==0x8000 &&
  *xeenEventWordDrawBound(0x8001)==0x7fff && *xeenEventWordDrawBound(0xffff)==1,"bit-15 words use the divisor's magnitude");
 check(xeenEventSignedWord(1)==1 && xeenEventSignedWord(0x7fff)==0x7fff && xeenEventSignedWord(0x8000)==0xffff8000u,
  "drawn words are sign-extended before the 32-bit addition");
 // Quest counters are bytes in both directions.
 check(XeenCloudsQuestItems::afterEventGrant(254)==255 && XeenCloudsQuestItems::afterEventGrant(255)==0 &&
  XeenCloudsQuestItems::afterEventTake(0)==255 && XeenCloudsQuestItems::afterEventTake(1)==0,"quest byte rule");
}

} // namespace

int main(int argc,char **argv) {
 try {
  if(argc==2 && std::string(argv[1])=="b5") {miscMaterial();return 0;}
  if(argc!=1)throw std::runtime_error("Unknown Event value test mode");
  widths();probes();writes();randomOperations();flowOperations();lockPicking();specialItem();miscMaterial();wordDraws();
 } catch(const std::exception &e) {std::cerr<<"Event value tests failed: "<<e.what()<<'\n';return 1;}
 std::cout<<"Event value tests passed\n";
 return 0;
}
