#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "app/XeenAudio.h"
#include "formats/xeen/XeenVoc.h"
#include <fstream>
#include <iostream>
#include <set>
#include <tuple>
using namespace training_test;
namespace mmodern {
struct XeenDigitalSitesTestAccess {
 static void weapon(const XeenEncounterFlow &flow){const_cast<XeenEncounterFlow &>(flow).fmWeaponEffect();}
 static void miss(const XeenEncounterFlow &flow){const_cast<XeenEncounterFlow &>(flow).digitalMiss();}
};
}
namespace {
struct Site {const char *offset,*names,*owner,*status;};
// Independent Part 0 physical inventory, including the maintainer's exclusions.
const Site sites[]={
 {"0xb112","fire.voc","XeenEventFlow","implemented"},
 {"0xb294","xeenlaff.voc","XeenEventFlow","implemented"},
 {"0xbfcf","elect.voc","Title lightning","gap: step 15"},
 {"0x116a0","scream.voc","Fall presentation","gap"},
 {"0x1170c","unnh.voc","Landing presentation","gap"},
 {"0x1abf7","trainin2.voc","XeenTrainingFlow","implemented"},
 {"0x1aedc","coina.voc","XeenSmithFlow","implemented"},
 {"0x1af54","ahh.voc","XeenSmithFlow / Event bridge","implemented"},
 {"0x1b0c9","gulp.voc","XeenSmithFlow","implemented"},
 {"0x1b31e","thankyou.voc","XeenSmithFlow","implemented"},
 {"0x1b469","thankyou.voc","XeenSmithFlow","implemented"},
 {"0x1b8d4","trainin0.voc, trainin1.voc, guild11.voc","XeenTrainingFlow / XeenSmithFlow","implemented"},
 {"0x1c7cb","coina.voc","XeenSmithFlow","implemented"},
 {"0x1d5c7","banker.voc, whaddayo.voc, guild10.voc, hello.voc, maywe2.voc, training.voc","XeenServiceAnimation via XeenEventFlow","implemented"},
 {"0x1d6b1","goodbye.voc","XeenSmithFlow","implemented"},
 {"0x1e66d","dreams2.voc","XeenEventFlow / XeenRestFlow","implemented"},
 {"0x1e6af","laff1.voc","XeenEventFlow / XeenRestFlow","implemented"},
 {"0x2875a","ahh.voc, whereto.voc, gulp.voc, null.voc, scream.voc, laff1.voc","Event interpreter / XeenEventFlow","implemented"},
 {"0x298ab","sphinx10.voc, sphinx11.voc, sphinx12.voc, sphinx13.voc, sphinx14.voc","XeenEntranceScene","implemented"},
 {"0x29e9f","ogre.voc","XeenEntranceScene","implemented"},
 {"0x29f97","ogre.voc","XeenEntranceScene","implemented"},
 {"0x2a06d","golem15.voc, golem13.voc","XeenEntranceScene","implemented"},
 {"0x2a250","ogre.voc","XeenEntranceScene","implemented"},
 {"0x2a2a2","golem16.voc, golem14.voc","XeenEntranceScene","implemented"},
 {"0x2abfd","reaper12.voc, reaper14.voc","XeenEntranceScene","implemented"},
 {"0x2ad63","reaper13.voc, reaper15.voc","XeenEntranceScene","implemented"},
 {"0x2b5f9","dwarf10.voc, dwarf11.voc, dwarf12.voc","XeenEntranceScene","implemented"},
 {"0x2c111","pow0.voc, pow1.voc, pow2.voc, pow3.voc, pow4.voc, pow5.voc","XeenCombat impact / XeenAudio","implemented"},
 {"0x2c172","miss.voc","Player damage candidates / XeenAudio","implemented"},
 {"0x2d802","monster record +0x33","XeenEncounterFlow::observeCombat","implemented"},
 {"0x34881","guild11.voc","XeenSmithFlow","implemented"},
 {"0x34d32","guild12.voc","XeenSmithFlow","implemented"}
};
std::string lower(std::string name){for(auto &c:name)if(c>='A' && c<='Z')c+='a'-'A';return name;}
void inventory(Inputs &in,const char *path) {
 std::ifstream file(path);std::string rules((std::istreambuf_iterator<char>(file)),{});
 check(std::size(sites)==32,"Clouds/shared physical VOC count");unsigned gaps=0;
 for(const auto &site:sites) {
  const auto row="| `"+std::string(site.offset)+"` | "+site.names+" | Efx | "+site.owner+" | "+site.status+" |";
  check(rules.find(row)!=rules.npos,("Missing Part 0 site/name/gate/owner: "+std::string(site.offset)).c_str());
  gaps+=std::string(site.status).find("gap")==0;
 }
 check(gaps==3,"Title/fall/landing exclusions must stay explicit");
 std::set<std::string> names;
 for(const auto &record:in.statistics) {
  auto end=std::find(record.raw.begin()+0x33,record.raw.end(),0);
  const auto name=lower(std::string(record.raw.begin()+0x33,end)+".voc");names.insert(name);
  check(!decodeXeenVoc(in.assets.readArchiveResource(name,XeenSceneArchive::Clouds)).pcm.empty(),"Missing monster attack sample");
 }
 check(in.statistics.size()==90 && names.size()==69,"Original monster voice inventory differs");
 unsigned rows=0;std::set<unsigned> indices;
 for(unsigned map=1;map<=85;++map)for(const auto &record:in.events.load(map).records)if(record.opcode==40) {
  check(record.parameters.size()==1 && record.parameters[0]<6,"Original Clouds voice index outside the six-name table");
  ++rows;indices.insert(record.parameters[0]);
 }
 check(rows==130 && indices==std::set<unsigned>{0,1,2,3,4},"Original Clouds voice row/index inventory differs");
}
using Trace=std::vector<std::tuple<std::string,std::uint64_t,std::uint64_t>>;
void attach(Fixture &f,const GameInstallation &installation,bool device,Trace &trace) {
 f.programState.audio=XeenAudio::installed(installation,f.in.assets,0,device);
 check(f.programState.audio->outputActive()==device,"Dummy device did not open");
 f.programState.audio->digitalSink([&](const auto &name,auto){trace.emplace_back(lower(name),f.now,f.w.sessionState().journeyRandom()->count);});
 f.flow->loadSampleDuration=[&](const char *name,auto archive){return xeenVocDurationNanoseconds(f.in.assets.readArchiveResource(name,archive));};
}
void tick(Fixture &f) {f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);}
bool heard(const Trace &trace,const std::string &name) {return std::any_of(trace.begin(),trace.end(),[&](const auto &v){return std::get<0>(v)==name;});}
using Result=std::pair<std::vector<std::uint8_t>,Trace>;
Result service(Inputs &in,const GameInstallation &installation,unsigned kind,bool enabled,bool device) {
 auto start=in.service();start.soundOptions.efx=enabled;
 if(kind==3)start.food=0;
 if(kind==4)start.characters[start.activeRosterIds[0]].currentHp=1;
 if(kind==2) {
  auto &member=start.characters[start.activeRosterIds[0]];member.weapons={};member.armor={};member.accessories={};member.miscellaneous={};
  member.characterClass=XeenCharacterClass::Cleric;member.hasSpells=true;member.learnedSpells.emplace();member.currentSpell=255;
  member.awards[0]=1;member.learnedSpells->fill(0);start.journey->purse->gold=10000;
 }
 const auto record=std::find_if(in.city.records.begin(),in.city.records.end(),[&](const auto &r){return r.opcode==0x11 && r.parameters==std::vector<std::uint8_t>{std::uint8_t(kind)};});
 check(record!=in.city.records.end(),"Original service record absent");
 start.camera={28,record->x,record->y,record->direction<4?XeenDirection(record->direction):XeenDirection::North};
 Fixture f(in,start);Trace trace;attach(f,installation,device,trace);
 f.flow->drawSmithArt=[&](auto &image){in.assets.drawSmith(image);};
 f.flow->drawBankArt=[&](auto &image){in.assets.drawBank(image);};
 f.flow->drawGuildArt=[&](auto &image){in.assets.drawGuild(image);};
 f.flow->drawTempleArt=[&](auto &image){in.assets.drawTemple(image);};
 f.flow->drawTavernArt=[&](auto &image){in.assets.drawTavern(image);};
 f.flow->tavernFoodOffer=[&](unsigned text){return in.assets.tavernFoodOffer(text);};
 f.flow->tavernString=[&](std::size_t index){return in.assets.tavernString(index);};
 f.enter();
 if(kind==5) {
  f.act(DialogKeyAction{'t'});check(enabled?heard(trace,"trainin0.voc"):trace.empty(),"Training refusal digital start/Efx gate");
  f.train(1);check(enabled?heard(trace,"trainin2.voc"):trace.empty(),"Training digital success/Efx gate");
 }
 if(kind==0) {
  f.act(DialogKeyAction{'d'});f.act(DialogKeyAction{'o'});f.act(DialogKeyAction{'5'});f.act(AcknowledgeAction{});
  check(enabled?heard(trace,"coina.voc"):trace.empty(),"Bank coin digital start/Efx gate");f.act(CancelInteractionAction{});
 }
 if(kind==2) {
  f.act(SelectMemberAction{1});check(enabled?heard(trace,"guild11.voc"):trace.empty(),"Guild rejection digital start/Efx gate");
  f.act(SelectMemberAction{0});f.act(DialogKeyAction{'b'});f.act(DialogKeyAction{'1'});f.act(YesAction{});
  check(enabled?heard(trace,"guild12.voc"):trace.empty(),"Guild purchase digital start/Efx gate");f.act(CancelInteractionAction{});
 }
 if(kind==4) {
  f.act(DialogKeyAction{'h'});f.prepare();check(enabled?heard(trace,"ahh.voc"):trace.empty(),"Temple digital start/Efx gate");
  f.act(DialogKeyAction{'d'});check(enabled?heard(trace,"coina.voc"):trace.empty(),"Temple donation digital start/Efx gate");
 }
 if(kind==3) {
  f.act(DialogKeyAction{'f'});f.act(YesAction{});
  check(enabled?heard(trace,"thankyou.voc"):trace.empty(),"Food digital start/Efx gate");
  f.act(DialogKeyAction{'d'});check(enabled?heard(trace,"gulp.voc"):trace.empty(),"Drink digital start/Efx gate");f.act(AcknowledgeAction{});
  const auto prior=trace.size();f.act(DialogKeyAction{'t'});
  check(enabled?(trace.size()==prior+1 && std::get<0>(trace.back())=="thankyou.voc"):trace.empty(),"Tavern tip digital start/Efx gate");
  f.act(AcknowledgeAction{});
 }
 f.act(CancelInteractionAction{});
 if(kind==3 && enabled) {
  check(heard(trace,"goodbye.voc") && !f.flow->canSave(),"Tavern did not replace and wait");
  const auto deadline=f.programState.audio->digitalDeadline(f.now*1000000ULL);
  check(bool(deadline),"Goodbye logical deadline absent");
  f.now=PresentationClock::ceilMilliseconds(*deadline)-1;
  f.flow->beginCycle(++f.cycle);check(!f.flow->updatePresentation(),"Tavern departed before logical voice completion");
  // Stopping the program voice makes the existing departure proceed even
  // though the Service's original visual sample interval has not elapsed.
  f.programState.audio->stopDigital(f.now*1000000ULL);
 }
 for(unsigned n=0;n<1000 && !f.flow->canSave();++n)tick(f);
 check(f.flow->canSave(),"Service failed to settle");
 check(enabled?heard(trace,XeenServiceAnimation::greeting(XeenServiceArt(kind))):trace.empty(),"Service greeting/Efx gate");
 return {XeenSaveFormat::encode(f.snapshot()),trace};
}
Result combat(Inputs &in,const GameInstallation &installation,bool enabled,bool device) {
 auto start=in.service();start.camera={23,9,11,XeenDirection::West};start.soundOptions.efx=enabled;
 start.journey->random=XeenCombatRandom(17).continuation();
 for(auto &a:start.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
 auto &a=start.map(23).monsters->slots[5];a.x=8;a.y=11;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;
 Fixture f(in,start);Trace trace;attach(f,installation,device,trace);
 std::set<std::uint64_t> attacks;bool guarded=true,preHp=true;
 f.programState.audio->digitalSink([&](const auto &requested,auto){
  const auto name=lower(requested);trace.emplace_back(name,f.now,f.w.sessionState().journeyRandom()->count);
  const auto *combat=f.flow->encounter()->combat();
  if(name.rfind("pow",0)==0) {
   if(combat) {
    const auto &result=combat->result();
    preHp=preHp && result.attackStage==XeenCombatAttackStage::PreHp &&
     f.w.sessionState().regionalActors(result.monster.mapId).at(result.monster.recordIndex).hp==result.actorHpBefore;
   }
   return;
  }
  if(name=="miss.voc")return;
  guarded=guarded && combat && combat->result().attackStage==XeenCombatAttackStage::MonsterPose1 && attacks.insert(combat->result().generation).second;
 });
 f.act(WaitAction{});unsigned turns=0;
 for(unsigned i=0;i<2500;++i) {
  if(const auto *c=f.flow->encounter()->combat();c && c->phase()==XeenCombatPhase::PlayerReady){f.act(AttackAction{});++turns;}
  const auto ui=f.flow->inputContext(f.flow->frame().presentation());if(ui.dialog && ui.dialog->anyKey)f.act(AcknowledgeAction{});
  tick(f);if(turns && !f.flow->encounter()->combat() && f.flow->canSave())break;
 }
 check(turns && !f.flow->encounter()->combat() && f.flow->canSave(),"Original combat did not finish");
 check(guarded,"Monster voice repeated outside guarded first pose");
 check(preHp,"Positive impact VOC followed HP publication");
 if(enabled) {
  check(std::any_of(trace.begin(),trace.end(),[](const auto &v){return std::get<0>(v).rfind("pow",0)==0;}),"No positive impact VOC");
  check(heard(trace,"miss.voc"),"No guarded miss VOC");check(!attacks.empty(),"No monster attack voice");
 }else check(trace.empty(),"Combat VOC bypassed Efx gate");
 auto after=f.snapshot();after.soundOptions.efx=true;
 return {XeenSaveFormat::encode(after),trace};
}
void cappedTraining(Inputs &in,const GameInstallation &installation,bool enabled) {
 auto start=in.service();start.soundOptions.efx=enabled;start.characters[start.activeRosterIds[0]].permanentLevel=10;
 Fixture f(in,start);Trace trace;attach(f,installation,false,trace);f.enter();f.act(DialogKeyAction{'t'});
 check(enabled?heard(trace,"trainin1.voc"):trace.empty(),"Training cap digital start/Efx gate");
}
void sampleErrors(Inputs &in,const GameInstallation &installation) {
 std::ifstream moduleFile(installation.uiModule->physicalPath,std::ios::binary);
 std::vector<std::uint8_t> module((std::istreambuf_iterator<char>(moduleFile)),{});
 for(bool missing:{true,false})for(bool miss:{true,false}) {
  Fixture f(in,in.service());unsigned reads=0;
  f.programState.audio=std::make_shared<XeenAudio>(0,PromusDriver::Bytes{},[&](const std::string &name,auto archive) {
   if(name=="miss.voc" || name.rfind("pow",0)==0) {
    ++reads;if(missing)throw std::runtime_error("Missing digital sample");
    return std::vector<std::uint8_t>{0};
   }
   return in.assets.readArchiveResource(name,archive);
  },module,0,false);
  bool raised=false;
  try {if(miss)XeenDigitalSitesTestAccess::miss(*f.flow->encounter());else XeenDigitalSitesTestAccess::weapon(*f.flow->encounter());}
  catch(const std::exception &){raised=true;}
  check(raised && reads==1,"Missing/corrupt combat sample was silently skipped");
 }
}
void commands(Inputs &in,const GameInstallation &installation) {
 auto audio=XeenAudio::installed(installation,in.assets,0,false);std::vector<std::string> names;
 audio->digitalSink([&](const auto &name,auto){names.push_back(lower(name));});
 for(unsigned i=0;i<6;++i)audio->eventVoice(i,0);
 check(names==std::vector<std::string>{"ahh.voc","whereto.voc","gulp.voc","null.voc","scream.voc","laff1.voc"},"DAT Event VOC table differs");
 const auto end=*audio->digitalDeadline(0);
 check(!audio->titleTeardownReady(end-1) && audio->titleTeardownReady(end),"Title teardown did not poll exact logical busy");
 audio->startDigital("ahh.voc",XeenSceneArchive::Clouds,end);
 auto options=XeenSoundOptions{};options.efx=false;audio->observeOptions(options);
 check(audio->titleTeardownReady(end),"Efx off must mask title teardown busy");
 audio->stopDigital(end);options.efx=true;audio->observeOptions(options);
 check(audio->digitalBusy(end),"Masked title teardown/stop cancelled running voice");
}
void event(Inputs &in,const GameInstallation &installation) {
 const auto original=std::find_if(in.city.records.begin(),in.city.records.end(),[](const auto &r){return r.opcode==40;});
 check(original!=in.city.records.end(),"Original Vertigo PlayEventVoc row absent");
 for(bool enabled:{true,false}) {
  auto start=in.service();start.soundOptions.efx=enabled;
  auto cue=*original;cue.x=10;cue.y=11;cue.direction=4;cue.line=0;
  auto exit=cue;exit.line=1;exit.opcode=0x12;exit.parameters.clear();exit.lengthField=5;
  XeenEventFile script{28,"maze0028.evt",true,{cue,exit}};
  Fixture f(in,start,false,&script);Trace trace;attach(f,installation,false,trace);
  // Part 0: click isolates the sound opcode; Space also has its surviving Wait.
  const auto before=f.w.sessionState().journeyRandom();f.act(InteractionAction{false});f.finishDeparture();
  check(f.flow->canSave() && f.now==0 && f.w.sessionState().journeyRandom()==before,"PlayEventVoc introduced a wait, notice or RNG draw");
  check(enabled?trace.size()==1:trace.empty(),"Original PlayEventVoc row did not publish once through Efx");
 }
}
void missGuards(Inputs &in,const GameInstallation &installation) {
 auto audio=XeenAudio::installed(installation,in.assets,0,false);std::vector<std::string> ordered;
 audio->digitalSink([&](const auto &name,auto){ordered.push_back(name);});
 audio->effectSink([&](unsigned id){ordered.push_back(std::to_string(id));});
 XeenCharacter character;character.permanentLevel=3;character.characterClass=XeenCharacterClass::Cleric;
 character.currentHp=100;character.intellect=character.personality=character.endurance={15,0};
 XeenCombatInputs inputs;inputs.might=inputs.speed=inputs.accuracy={15,0};inputs.luck=XeenAttributeValue{15,0};inputs.resistances=XeenCombatResistances{};
 XeenMonsterRecord monster;monster.raw[22]=255;monster.raw[24]=1;monster.raw[25]=3;monster.raw[26]=1;monster.raw[28]=10;monster.raw[31]=5;
 auto finish=[&](auto &candidate,std::vector<XeenCombatRandom::Draw> tape,bool miss) {
  ordered.clear();XeenCombatRandom random(tape);
  XeenConsequenceDraw draw{random,64,{},[&](unsigned id){audio->effect(id,0);},{},[&]{audio->missEffect(0);}};
  check(candidate.service(draw) && random.position()==tape.size(),"Digital miss changed candidate RNG");
  const auto before=ordered;check(candidate.service(draw) && ordered==before,"Completed candidate repeated digital miss");
  check(miss?ordered==std::vector<std::string>{"miss.voc","6"}:ordered.empty(),"Guarded miss VOC/FM ordering differs");
 };
 XeenPhysicalPlayerCandidate melee(character,inputs,monster,1,610,false,XeenDifficulty::Adventurer);
 finish(melee,{{1,20,1}},true);
 character.weapons[0].id=30;character.weapons[0].frame=4;
 XeenPhysicalPlayerCandidate missile(character,inputs,monster,1,610,true,XeenDifficulty::Adventurer);
 finish(missile,{{1,2,1},{1,2,1},{1,2,1},{1,20,1}},false);
 XeenSpellDamageCandidate refused(3,0,100,1,XeenLearnedSpell::MagicArrow);
 finish(refused,{{1,103,1}},false);
 XeenSpellDamageCandidate zero(3,0,0,89,XeenLearnedSpell::MagicArrow);
 finish(zero,{},true);
}
}
int main(int argc,char **argv) {try {
 check(argc==3,"usage: digital-sites <game data> <audio rules>");
 const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Original installation absent");
 Inputs in(*installation);inventory(in,argv[2]);sampleErrors(in,*installation);commands(in,*installation);event(in,*installation);missGuards(in,*installation);
 for(bool enabled:{true,false}) {
  cappedTraining(in,*installation,enabled);
  for(unsigned kind:{0u,1u,2u,3u,4u,5u})check(service(in,*installation,kind,enabled,false)==service(in,*installation,kind,enabled,true),"Device changed service trace/time/RNG");
  const auto silent=combat(in,*installation,enabled,false),device=combat(in,*installation,enabled,true);
  check(silent==device,"Device changed combat requests/time/RNG");
  if(enabled)check(silent.first==combat(in,*installation,false,false).first,"VOC publication changed combat RNG/order");
 }
 std::cout<<"32 inventoried sites: 29 wired, title/fall/landing excluded; original services and combat traces device-independent\n";
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
