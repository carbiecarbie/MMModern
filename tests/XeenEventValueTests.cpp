#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenEventContinuation.h"
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
void twoSidedValues() {
 Owners o;
 for(auto side:{XeenSide::Clouds,XeenSide::Darkside}) {
  o.camera.mapId.side=side;const auto w=o.owners();
  for(unsigned i:{0u,255u})check(xeenEventGive(w,20,i,0).status==XeenEventWriteStatus::Applied,"side game flag grant");
  for(unsigned i:{0u,29u})check(xeenEventGive(w,104,i,0).status==XeenEventWriteStatus::Applied,"side quest flag grant");
  for(unsigned i:{0u,31u})check(xeenEventGive(w,107,i,0).status==XeenEventWriteStatus::Applied,"roster flag grant");
  for(unsigned i=0;i<85;++i) {o.p.questItems.eventGrant(i);check(probe(o,21,82+i).value==82+i,"shared quest item on either side");}
 }
 o.camera.mapId.side=XeenSide::Darkside;xeenEventTake(o.owners(),20,255,0);xeenEventTake(o.owners(),104,29,0);
 check(o.flags.isSet(255) && !o.flags.isSet(XeenSide::Darkside,255) && o.p.questFlags.isSet(29) && !o.p.questFlags.isSet(XeenSide::Darkside,29),"flag banks alias");
 check(o.p.questItems.at(26)==2 && probe(o,105,0).value==2 && !o.c(1).characterFlags[0],"shared state followed side or touched another member");
 o.p.party=XeenParty::fromRosterIds({1,0,2,3,4,5});check(probe(o,107,31,1).value==31 && !o.c(1).characterFlags[31],"character flags followed party slot");
 check(xeenEventGive(o.owners(),107,32,1).status==XeenEventWriteStatus::Unsupported,"character flag boundary");
}
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
 o.p.questItems=XeenQuestItems(XeenQuestItems::Counts{});
 auto counts=static_cast<XeenQuestItems::Counts>(o.p.questItems.counts());counts[26]=3;o.p.questItems=XeenQuestItems(counts);
 check(probe(o,105,2).value==3,"Megacredits counter (quest item 26)");
 std::string reason;check(probe(o,107,1).value==std::numeric_limits<std::uint32_t>::max(),"clear character flag test");
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
XeenPresentationResponse defaultAnswer(const XeenPresentationRequest &q) {
 return q.response==XeenPresentationResponseRequirement::Presented?
  XeenPresentationResponse(XeenPresentationResponse::Presented):XeenPresentationResponse(XeenPresentationResponse::Acknowledged);
}
// `draw` observes the interpreter's eventDraw; `edit` may change a suspended state before it resumes.
Run run(XeenWorld &w,XeenPartyState &p,XeenGameFlags &flags,const std::vector<XeenEventRecord> &records,
  std::function<XeenPresentationResponse(const XeenPresentationRequest &)> respond={},XeenCamera camera={47,5,5,XeenDirection::North},
  std::function<void(XeenWorld &,const XeenCamera &,bool)> draw={},
  std::function<void(XeenEventExecutionState &,const XeenPresentationRequest &)> edit={}) {
 XeenEventFile file{camera.mapId,"synthetic.evt",true,records};
 const auto scripts=[&](XeenMapIdentity){return XeenEventScript(file);};
 const auto texts=[](XeenMapIdentity id){XeenEventTextFile t;t.mapId=id;t.resourcePresent=true;t.strings={"Title","Body","Third"};return t;};
 Run out;XeenEventInterpreter vm;vm.eventDraw=std::move(draw);
 out.result=vm.begin(camera,p,flags,w,scripts,texts);
 for(unsigned n=0;n<64;++n) {
  auto *s=std::get_if<XeenEventExecutionSuspended>(&out.result);if(!s)break;
  out.requests.push_back(s->request);
  const auto answer=respond?respond(s->request):defaultAnswer(s->request);
  auto state=s->state;if(edit)edit(state,s->request);
  out.result=vm.resume(std::move(state),answer,p,w,scripts,texts);
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
   // No draw updated giveExt's frame; this MOB has no open chest, so the sprite stays.
   check(f.w->objectFrame({XeenMapIdentity(map),object})==std::optional<unsigned>(picked?1u:0u) &&
    f.w->effectiveObject({XeenMapIdentity(map),object}).resourceId==16,"giveExt's frame or sprite differs");
  }
  return;
 }
 throw std::runtime_error("no trap-free seed");
}

// --- M96: the end-of-run chest rule (DOS 0x28d92..0x28e87) ---
// Picture-table slots: 0 sprite 7, 1 closed 16, 2 open 62 (the first), 3 closed
// 73, 4 open 119, 5 the lock object 58, 6 a later 62, 7 Darkside's closed 15, 8
// its open 57. Records: 0 a 7 at (9,9), 1 a 16 at (6,6), 2 a 73 at (7,7), 3 a
// 58 at (8,8), 4 a 16 at (10,10), 5 a 15 at (11,11).
XeenObjectFile chests(XeenMapIdentity id) {
 XeenObjectFile f{id,"chests.mob",true,{}};
 f.entities.objectTable=std::array<std::uint8_t,16>{7,16,62,73,119,58,62,15,57,255,255,255,255,255,255,255};
 for(const XeenMapEntity e:{XeenMapEntity{9,9,0,0,7},XeenMapEntity{6,6,1,0,16},XeenMapEntity{7,7,3,0,73},
  XeenMapEntity{8,8,5,0,58},XeenMapEntity{10,10,1,0,16},XeenMapEntity{11,11,7,0,15}})f.entities.objects.push_back(e);
 f.entities.monsters.push_back({1,1,0,0,8});f.entities.monsters.push_back({100,100,0,0,8});
 return f;
}
XeenObjectFile closedChests(XeenMapIdentity id) { // The same records without an open slot.
 auto f=chests(id);for(unsigned slot:{2u,4u,6u,8u})f.entities.objectTable[slot]=255;return f;
}
// The first seed whose lock draws spring no trap and pick map 47's box-2 lock with member 0.
std::uint32_t pickingSeed() {
 const int thievery=XeenCharacterRules::thievery(party().roster.at(0));
 for(std::uint32_t seed=1;seed<64;++seed) {
  XeenJourneyRandomState start;start.state=seed;XeenCombatRandom random(start);std::optional<std::uint32_t> trap,attempt;
  while(!trap)trap=random.draw(1,4);
  if(*trap==1)continue;
  while(!attempt)attempt=random.draw(1,20);
  if(thievery+int(*attempt)>=2)return seed;
 }
 throw std::runtime_error("no picking seed");
}
std::vector<XeenObjectOverride> overrides(const XeenWorld &w,XeenMapIdentity map=47) {
 const auto *s=w.sessionState().mapState(map);return s?s->objectOverrides:std::vector<XeenObjectOverride>{};
}
XeenEventRecord at(XeenEventRecord r,int x,int y) {r.x=std::uint8_t(x);r.y=std::uint8_t(y);return r;}
XeenCamera cell(int x,int y,XeenMapIdentity map=47) {return {map,x,y,XeenDirection::North};}
bool refused(const Run &r,const char *what) {
 const auto *e=std::get_if<XeenEventExecutionError>(&r.result);
 return e && e->message.find(what)!=std::string::npos;
}

void chestRule() {
 const auto seed=pickingSeed();const auto give=row(0,20,{34,100,0,0,0,0,0,0,0});
 const XeenObjectIdentity chest{47,1},other{47,4};
 // Acceptance 1: closed at the receipt, open in the tail's draw; a later run sees 62.
 {
  World f(47,seed,chests);auto p=party();XeenGameFlags flags;bool receipt=false;std::vector<int> drawn;
  const auto r=run(*f.w,p,flags,{at(give,6,6)},[&](const XeenPresentationRequest &q) {
    if(q.kind==XeenPresentationKind::RewardReceipt) {
     receipt=true;
     check(f.w->effectiveObject(chest).resourceId==16 && overrides(*f.w).empty(),"the chest opened before its treasure");
    }
    return defaultAnswer(q);
   },cell(6,6),[&](XeenWorld &w,const XeenCamera &,bool){drawn.push_back(w.effectiveObject(chest).resourceId);});
  const auto values=overrides(*f.w);const auto object=f.w->effectiveObject(chest);
  check(completed(r) && receipt && !drawn.empty() && drawn.back()==62,"the tail's draw does not see the opened chest");
  check(values.size()==1 && values[0].id==chest && values[0].x==6 && values[0].y==6 && !values[0].flipped &&
   values[0].tableIndex==std::uint8_t(2) && object.resourceId==62 && object.tableIndex==2 && object.x==6 && object.y==6,
   "the chest does not take the first open slot in place");
  const auto random=*f.w->sessionState().journeyRandom();const auto frame=f.w->objectFrame(chest);
  check(completed(run(*f.w,p,flags,{at(give,6,6)},{},cell(6,6))) && *f.w->sessionState().journeyRandom()==random &&
   f.w->objectFrame(chest)==frame && p.purse->gold==700 && overrides(*f.w)==values,"an opened chest is picked again");
 }
 // Acceptance 2: a "Nothing here" after the change; resuming runs the tail once.
 {
  World f(47,seed,chests);auto p=party();XeenGameFlags flags;unsigned nothing=0;
  const auto r=run(*f.w,p,flags,{at(give,6,6)},[&](const XeenPresentationRequest &q) {
    if(q.kind==XeenPresentationKind::NothingHere) {
     ++nothing;const auto values=overrides(*f.w);
     check(values.size()==1 && values[0].tableIndex==std::uint8_t(2),"the rule did not apply before Nothing here");
    }
    return defaultAnswer(q);
   },cell(6,6),{},[&](XeenEventExecutionState &s,const XeenPresentationRequest &q) {
    // No Clouds pick reaches the notice: the executed marker, which GiveMulti
    // sets again when it resumes, is cleared at its treasure.
    if(q.kind==XeenPresentationKind::RewardReceipt)s.scriptExecuted=false;
    if(q.kind==XeenPresentationKind::NothingHere) {
     check(s.tailDone && !s.scriptValues.lockCounter,"the tail did not finish the rule");
     // Were the tail to run again, this second closed chest would open.
     s.scriptValues.lockCounter=1;s.selectedObject=other;
    }
   });
  check(completed(r) && nothing==1 && overrides(*f.w).size()==1 && f.w->effectiveObject(other).resourceId==16,
   "resuming applied the rule again");
 }
 // Acceptance 3: no change.
 {
  World f(47,seed,chests);auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{at(give,5,5)},{},cell(5,5),{},[](XeenEventExecutionState &s,const XeenPresentationRequest &q) {
   if(q.kind==XeenPresentationKind::RewardReceipt)s.scriptValues.lockCounter=1;});
  check(completed(r) && overrides(*f.w).empty(),"a counter without a selection changed an object");
 }
 {
  World f(48,seed,chests);auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{at(give,6,6)},{},cell(6,6,48));
  check(completed(r) && p.purse->gold==500 && overrides(*f.w,48).empty() && f.w->objectFrame({48,1})==0u,
   "a failed pick changed the chest");
 }
 {
  World f(47,seed,closedChests);auto p=party();XeenGameFlags flags;
  check(completed(run(*f.w,p,flags,{at(give,6,6)},{},cell(6,6))) && p.purse->gold==600 && overrides(*f.w).empty() &&
   f.w->effectiveObject(chest).resourceId==16,"a chest without an open slot changed");
 }
 {
  World f(47,seed,chests);auto p=party();XeenGameFlags flags;
  check(completed(run(*f.w,p,flags,{at(give,8,8)},{},cell(8,8))) && p.purse->gold==600 && overrides(*f.w).empty() &&
   f.w->objectFrame({47,3})==1u,"sprite 58 changed");
 }
 {
  const XeenMapIdentity dark{XeenSide::Darkside,47};
  World f(dark,seed,chests);auto p=party();XeenGameFlags flags;
  check(completed(run(*f.w,p,flags,{at(give,6,6)},{},cell(6,6,dark))) && p.purse->gold==600 && overrides(*f.w,dark).empty(),
   "a Darkside sprite 16 opened");
  World g(dark,seed,chests);auto q=party();
  check(completed(run(*g.w,q,flags,{at(give,11,11)},{},cell(11,11,dark))) && overrides(*g.w,dark).size()==1 &&
   overrides(*g.w,dark)[0].tableIndex==std::uint8_t(8) && g.w->effectiveObject({dark,5}).resourceId==57,
   "a Darkside sprite 15 did not take sprite 57");
 }
 {
  World f(47,seed,chests);auto p=party();XeenGameFlags flags;
  check(completed(run(*f.w,p,flags,{at(give,7,7)},{},cell(7,7))) && overrides(*f.w).size()==1 &&
   overrides(*f.w)[0].tableIndex==std::uint8_t(4) && f.w->effectiveObject({47,2}).resourceId==119,"a 73 did not take sprite 119");
 }
 {
  // A pick in a called script, the run ending inside another call (map 30's
  // central chests: CallEvent (80,80) picks and returns, CallEvent (81,81)
  // has no next line): the run's end still opens the chest.
  World f(47,seed,chests);auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{at(row(0,25,{80,80,0}),6,6),at(row(1,25,{81,81,0}),6,6),
   at(give,80,80),at(row(1,26),80,80),at(row(0,0),81,81)},{},cell(6,6));
  check(completed(r) && overrides(*f.w).size()==1 && overrides(*f.w)[0].id==chest &&
   overrides(*f.w)[0].tableIndex==std::uint8_t(2),"a run ending inside a call did not open the chest");
 }
 // A refused run never reaches the rule: Remove, any MoveObj, ExchObj (outside
 // the decoder subset), another teleport, a map change or a service while the
 // counter is positive.
 const std::vector<std::pair<XeenEventRecord,const char *>> refusals{
  {row(1,14),"Remove after a picked lock"},{row(1,11,{1,3,3}),"MoveObj after a picked lock"},
  {row(1,11,{0,3,3}),"MoveObj after a picked lock"},{row(1,51,{0,1}),"opcode 51"},
  {row(1,7,{47,2,2}),"Teleport after a picked lock"},{row(1,31,{48,2,2}),"Teleport after a picked lock"},
  {row(1,17,{1}),"Town service after a picked lock"}};
 for(const auto &refusal:refusals) {
  World f(47,seed,chests);auto p=party();XeenGameFlags flags;
  const auto r=run(*f.w,p,flags,{at(give,6,6),at(refusal.first,6,6)},{},cell(6,6));
  check(refused(r,refusal.second) && overrides(*f.w).empty(),"a refused run opened the chest");
 }
 {
  // The same-map restart zeroes the counter (0x27127): the chest selected at
  // (10,10) stays closed, as does the picked one.
  World f(47,seed,chests);auto p=party();XeenGameFlags flags;
  check(completed(run(*f.w,p,flags,{at(give,6,6),at(row(1,31,{47,10,10}),6,6)},{},cell(6,6))) && p.purse->gold==600 &&
   overrides(*f.w).empty(),"a TeleportAndContinue restart kept the lock counter");
 }
 // Acceptance 4 (live): MoveObj and Remove write x and y only.
 {
  World f(47,seed,chests);auto p=party();XeenGameFlags flags;
  check(completed(run(*f.w,p,flags,{at(give,6,6)},{},cell(6,6))),"the chest was not picked");
  const auto move=[&](int x,int y){return completed(run(*f.w,p,flags,{row(0,11,{1,std::uint8_t(x),std::uint8_t(y)})},{},cell(5,5)));};
  const auto index=[&]{const auto v=overrides(*f.w);return v.size()==1 && v[0].tableIndex==std::uint8_t(2);};
  check(move(3,3) && index() && f.w->effectiveObject(chest).x==3 && f.w->effectiveObject(chest).resourceId==62,"MoveObj lost the index");
  const bool flip=f.w->effectiveObjectFlip(chest);
  check(move(3,3) && index() && f.w->effectiveObjectFlip(chest)!=flip,"an in-place MoveObj lost the index");
  check(move(-128,-128) && index() && f.w->isObjectDisabled(chest),"a hidden MoveObj lost the index");
  check(move(6,6) && index() && f.w->selectObject(cell(6,6))==chest,"the chest did not return");
  // Remove disables its cell's records by index: record 0 stays the MoveObj's.
  check(completed(run(*f.w,p,flags,{at(row(0,0),9,9),at(row(0,14),6,6)},{},cell(6,6))) && index() &&
   f.w->sessionState().isObjectDisabled(chest) &&
   overrides(*f.w)[0].x==-128 && overrides(*f.w)[0].y==-128 && !overrides(*f.w)[0].flipped,"Remove lost the index");
  check(move(4,4) && index() && !f.w->sessionState().isObjectDisabled(chest) && f.w->effectiveObject(chest).resourceId==62,
   "MoveObj after Remove lost the index");
 }
 // Restore accepts the record's own index (stored as absent) or the first open
 // chest of its own closed sprite on its side, nothing else.
 {
  const XeenMapIdentity dark{XeenSide::Darkside,47};
  const auto accepts=[](XeenMapIdentity map,std::size_t record,std::uint8_t index) {
   XeenWorld w(tile,chests);
   try {w.restoreObjectOverrides({{{map,record},1,1,false,index}});} catch(const std::invalid_argument &) {return false;}
   const auto *s=w.sessionState().mapState(map);
   return s && s->objectOverrides.size()==1;
  };
  check(accepts(47,1,1) && accepts(47,1,2) && !accepts(47,1,6) && !accepts(47,1,16) && accepts(47,2,4) &&
   !accepts(47,0,2) && !accepts(47,3,2) && !accepts(dark,1,2) && accepts(dark,5,8) && !accepts(47,5,8),
   "restore's index rule differs");
  XeenWorld w(tile,chests);w.restoreObjectOverrides({{{47,1},1,1,false,std::uint8_t(1)}});
  check(!w.sessionState().mapState(47)->objectOverrides[0].tableIndex,"the record's own index was stored");
 }
 // Remove's override rule (live and Journey publication): only an index keeps one.
 {
  std::vector<XeenObjectOverride> values{{{47,1},6,6,true,std::uint8_t(2)},{{47,2},3,3,true}};
  xeenRemoveObjectOverride(values,{47,2});xeenRemoveObjectOverride(values,{47,1});
  check(values.size()==1 && values[0].x==-128 && values[0].y==-128 && !values[0].flipped && values[0].tableIndex==std::uint8_t(2),
   "Remove's override rule differs");
 }
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
 check(XeenQuestItems::afterEventGrant(254)==255 && XeenQuestItems::afterEventGrant(255)==0 &&
  XeenQuestItems::afterEventTake(0)==255 && XeenQuestItems::afterEventTake(1)==0,"quest byte rule");
}

void prefixLightWrites() {
 for(unsigned mode:{70u,74u}) {
  Owners f;std::uint16_t prefix=3;auto owners=f.owners();owners.prefixLight=&prefix;
  auto &light=f.p.encounterContext->lightAndResistances[0];light=2; // One detached scene drain.
  const unsigned assigned=mode==70?2u:0x101u;
  check(xeenEventSetValue(owners,mode,assigned,0).status==XeenEventWriteStatus::Applied,"Prefix light Set");
  const std::uint16_t value=mode==70?2:1;
  check(light==value && prefix==value,"Set replaces a prior drain, even without a numeric change");
  --light; // A later scene check must not alter the Event-only projection.
  check(xeenEventGive(owners,mode,2,0).status==XeenEventWriteStatus::Applied && prefix==value+2 && light==value+1,
   "Give applies the resolved operand independently of scene drains");
  check(xeenEventTake(owners,mode,4,0).status==XeenEventWriteStatus::Applied &&
   prefix==std::uint16_t(value-2) && light==std::uint16_t(value-3),"Take projection wraps as a DOS word");
  prefix=65535;light=65534;
  check(xeenEventGive(owners,mode,1,0).status==XeenEventWriteStatus::Applied && prefix==0 && light==65535,
   "Prefix Give wrap preserves the detached drain difference");
  check(xeenEventTake(owners,mode,1,0).status==XeenEventWriteStatus::Applied && prefix==65535 && light==65534,
   "Prefix Take wrap preserves the detached drain difference");
 }
 Owners f;std::uint16_t prefix=7;auto owners=f.owners();owners.prefixLight=&prefix;
 f.p.encounterContext->effects[XeenPartyEffect::AutomapOn]=2;
 const auto before=*f.p.encounterContext;
 check(xeenEventSetValue(owners,74,2,0).status==XeenEventWriteStatus::Unsupported &&
  prefix==7 && *f.p.encounterContext==before,"Refused collective Set cannot change the projection");
 // Give 85 clears temporary party words; a later portrait publishes that reset.
 for(auto side:{XeenSide::Clouds,XeenSide::Darkside}) {
 World world;auto p=party();XeenGameFlags flags;
 for(unsigned i:{55u,155u,222u,231u}){flags.set(i);flags.set(XeenSide::Darkside,i);}
 p.encounterContext->lightAndResistances[0]=3;
 bool reset=false;
 const auto result=run(*world.w,p,flags,{row(0,12,{0,0,85,1}),row(1,22,{1,0,0})},
  {},{{side,47},5,5,XeenDirection::North},{},[&](auto &state,const auto &) {
   if(!reset) {
    XeenEventContinuation retained(state);auto altered=state;altered.prefixLight=1;
    bool rejected=false;try {retained.check(altered);}catch(const std::exception &) {rejected=true;}
    check(rejected,"Continuation protects the Event-only prefix light projection");
   }
   check(state.prefixLight==0 && p.encounterContext->lightAndResistances[0]==0,
    "Year reset clears both actual and publishable light across every portrait suspension");reset=true;
  });
 check(completed(result) && reset && p.encounterContext->year==611,"Year reset reaches its following portrait");

 const auto yearFlags=finalFlags(result);
 for(unsigned i:{55u,155u,222u,231u})check(!yearFlags.isSet(i) && yearFlags.isSet(XeenSide::Darkside,i),"Year reset used the camera bank instead of Clouds");
 }
}

} // namespace

int main(int argc,char **argv) {
 try {
  if(argc==2 && std::string(argv[1])=="b5") {miscMaterial();return 0;}
  if(argc!=1)throw std::runtime_error("Unknown Event value test mode");
  twoSidedValues();widths();probes();writes();prefixLightWrites();randomOperations();flowOperations();lockPicking();chestRule();specialItem();miscMaterial();wordDraws();
 } catch(const std::exception &e) {std::cerr<<"Event value tests failed: "<<e.what()<<'\n';return 1;}
 std::cout<<"Event value tests passed\n";
 return 0;
}
