#include "XeenCombatGameplayTestSupport.h"
#include "games/xeen/XeenTextInput.h"
#include "games/xeen/XeenEventContinuation.h"
#include "games/xeen/XeenStateEquality.h"
#include <iostream>
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include <map>
using namespace mmodern;
namespace {
void check(bool value,const std::string &message) {if(!value)throw std::runtime_error(message);}
XeenEventRecord record(unsigned x,unsigned y,unsigned line,unsigned opcode,std::vector<std::uint8_t> args={}) {
 XeenEventRecord r;r.x=x;r.y=y;r.line=line;r.direction=4;r.opcode=opcode;
 r.parameters=std::move(args);r.lengthField=5+r.parameters.size();return r;
}
struct Fixture {
 std::map<XeenMapIdentity,unsigned> loads;
 std::vector<XeenMapEntity> objects;
 XeenWorld world{[&](auto id){++loads[id];XeenMap m;m.side=id.side;m.geometry.id=id.number;
  m.geometry.runX=7;m.geometry.runY=8;return m;},[&](auto id){
   XeenObjectFile file;file.mapId=id;file.resourcePresent=true;file.entities.objects=objects;return file;}};
 XeenPartyState party;
 XeenEventInterpreter interpreter;
 XeenEventTextFile texts{28,"synthetic.txt",true,{"mirror","Primary","Alternate","Title"}};
 std::map<XeenMapIdentity,XeenEventScript> scripts;
 XeenEventInterpreter::ScriptProvider provider() {return [&](auto id){auto at=scripts.find(id);return at==scripts.end()?XeenEventScript({id,"synthetic.evt",true,{}}):at->second;};}
 auto textProvider() {return [&](auto id){auto t=texts;t.mapId=id;return t;};}
 void script(XeenMapIdentity id,std::vector<XeenEventRecord> records) {scripts.emplace(id,XeenEventFile{id,"synthetic.evt",true,std::move(records)});}
 auto begin(XeenMapIdentity map=28) {return interpreter.begin({map,1,1,XeenDirection::West},party,{},world,provider(),textProvider());}
 auto answer(XeenEventExecutionStepResult result,std::string text) {
  auto *p=std::get_if<XeenEventExecutionSuspended>(&result);check(p && p->request.word,"Expected word input");
  return interpreter.resume(std::move(p->state),XeenWordInput{std::move(text)},party,world,provider(),textProvider());
 }
};
void teleports() {
 for(unsigned opcode:{7,31})for(bool same:{false,true})for(bool run:{false,true}) {
  Fixture f;const unsigned target=same?28:23;
  f.script(28,{record(1,1,0,opcode,{std::uint8_t(target),std::uint8_t(run?255:4),5}),record(1,1,1,0x30)});
  const auto r=f.begin();const auto *done=std::get_if<XeenEventExecutionCompleted>(&r);
  check(done && done->finalCamera.mapId==XeenMapIdentity(target) && done->finalCamera.x==(run?7:4) &&
   done->finalCamera.y==(run?8:5) && done->finalCamera.direction==XeenDirection::West,"Direct/run teleport");
  check(f.loads[28]==1 && (same || f.loads[23]==1),"Same-map travel reloaded geometry");
 }
 Fixture f;f.party.party=XeenParty::fromRosterIds({0,1});
 f.party.roster.at(0).currentHp=f.party.roster.at(1).currentHp=10;
 f.objects={{1,1,0,0,1},{3,4,0,0,2}};
 f.script(28,{record(1,1,0,32,{0,0}),record(1,1,1,15,{0}),record(1,1,2,27,{83,5}),record(1,1,3,1,{1}),
  record(1,1,4,25,{100,100,0}),record(1,1,5,0x30),record(100,100,0,31,{28,3,4}),
  record(3,4,0,1,{1})});
 auto selection=f.begin();auto *choose=std::get_if<XeenEventExecutionSuspended>(&selection);
 check(choose && choose->request.kind==XeenPresentationKind::CharacterSelection,"Continue reset needs a member selection");
 auto before=f.interpreter.resume(std::move(choose->state),SelectedCharacter{1},f.party,f.world,f.provider(),f.textProvider());
 auto *source=std::get_if<XeenEventExecutionSuspended>(&before);
 check(source && source->state.activeCharacterIndex==1 && source->state.entireParty && source->state.scriptValues.damageType==5 &&
  source->state.selectedObject==XeenObjectIdentity{28,0},"Continue reset preconditions must be non-default");
 const auto r=f.interpreter.resume(std::move(source->state),XeenPresentationResponse::Presented,
  f.party,f.world,f.provider(),f.textProvider());const auto *p=std::get_if<XeenEventExecutionSuspended>(&r);
 check(p && p->state.callStack.empty() && p->state.logicalAddress.x==3 && p->state.logicalAddress.line==1 &&
  p->state.scriptValues.damageType==0 && p->state.activeCharacterIndex==0 && !p->state.entireParty &&
  p->state.selectedObject==XeenObjectIdentity{28,1},"Continue must reset member/damage and reselect the destination object");
 for(unsigned coordinate:{128,254}) {
  Fixture bad;bad.script(28,{record(1,1,0,7,{28,std::uint8_t(coordinate),1})});
  const auto result=bad.begin();const auto *error=std::get_if<XeenEventExecutionError>(&result);
  check(error && error->kind==XeenEventExecutionErrorKind::UnsupportedTeleportDestination,"Signed coordinates were converted to unsigned cells");
 }
}
void words() {
 for(const auto &word:{"PRIMARY","alternate","wrong",""}) {
  Fixture f;f.script(28,{record(1,1,0,21,{1,3,2,3}),record(1,1,1,18),record(1,1,3,1,{3})});
  auto prompt=f.begin();const auto &p=std::get<XeenEventExecutionSuspended>(prompt);
  check(p.request.title=="Title" && p.request.text=="Primary" && p.request.alternate=="Alternate","Word resources/title");
  XeenEventContinuation retained(p.state,true);auto changed=p.state;changed.pendingPresentation->request.alternate="changed";
  bool refused=false;try{retained.check(changed);}catch(const std::logic_error &){refused=true;}check(refused,"Word continuation preimage");
  const auto r=f.answer(std::move(prompt),word);
  check((std::string(word)=="PRIMARY" || std::string(word)=="alternate")?
   std::holds_alternative<XeenEventExecutionSuspended>(r):std::holds_alternative<XeenEventExecutionCompleted>(r),"Password success/failure fall-through");
  if(const auto *success=std::get_if<XeenEventExecutionSuspended>(&r))
   check(success->state.logicalAddress.line==4,"Alternate-only match must follow ordinary success, never the title operand");
 }
 for(unsigned map:{28,33})for(unsigned index:{0,38})for(int direction:{-1,2}) {
  Fixture f;f.texts.mirrors.resize(62);f.texts.mirrors[index]={"Destination",std::uint8_t(map),4,5,direction};
  f.script(map,{record(1,1,0,21,{0,2,0,3}),record(1,1,1,18),record(1,1,2,7,{0})});
  const auto r=f.answer(f.begin(map),"destination");const auto *done=std::get_if<XeenEventExecutionCompleted>(&r);
  check(done,"Mirror did not finish");const bool found=map==28 || index==38;
  check(done->finalCamera.x==(found?4:1) && done->finalCamera.direction==(found && direction==2?XeenDirection::South:XeenDirection::West),"Clouds mirror bounds/direction");
 }
 Fixture run;run.texts.mirrors={{"Run",28,0,0,-1}};
 run.script(28,{record(1,1,0,21,{0,2,0,0}),record(1,1,1,18),record(1,1,2,7,{0})});
 const auto r=run.answer(run.begin(),"run");check(std::get<XeenEventExecutionCompleted>(r).finalCamera.x==7,"Mirror run-location sentinel");
 Fixture absent;absent.script(28,{record(1,1,0,21,{1,3,21,21}),record(1,1,1,18),record(1,1,3,1,{3})});
 check(std::get<XeenEventExecutionSuspended>(absent.begin()).request.title=="mirror" &&
  std::holds_alternative<XeenEventExecutionSuspended>(absent.answer(absent.begin(),"mirror")),"Unused DOS text offsets alias entry zero");
 Fixture vow;vow.texts.strings[1]="iieeeoeeeouie";vow.script(28,{record(1,1,0,21,{1,3,0,0}),record(1,1,1,18),record(1,1,3,1,{3})});
 check(std::holds_alternative<XeenEventExecutionCompleted>(vow.answer(vow.begin(),"iieeeoeeaouie")),"Reference answer rewrite leaked into DOS comparison");
 Fixture sword;sword.texts.mirrors.resize(62);sword.texts.mirrors[58].name="Sword";
 sword.script(28,{record(1,1,0,21,{0,2,0,0}),record(1,1,1,18),record(1,1,2,0x30)});
 const auto reward=sword.answer(sword.begin(),"sword");const auto *pending=std::get_if<XeenEventExecutionSuspended>(&reward);
 check(pending && pending->state.reward.receipt.count==1 && pending->state.reward.receipt.entries[0].item.id==34,"Sword must enter ordinary treasure delivery and exit");
}
void swordCapacityAndEndings() {
 Fixture f;f.texts.mirrors.resize(62);f.texts.mirrors[58].name="Sword";
 f.script(28,{record(1,1,0,21,{0,2,0,3}),record(1,1,1,18),record(1,1,2,1,{3})});
 auto prompt=f.begin();auto &state=std::get<XeenEventExecutionSuspended>(prompt).state;
 for(unsigned i=0;i<10;++i)state.treasure.insert(XeenRewardCategory::Weapons,{0,1,0,0});
 const auto full=f.answer(std::move(prompt),"sword");const auto *p=std::get_if<XeenEventExecutionSuspended>(&full);
 check(p && p->state.logicalAddress.line==3 && p->state.treasure.full(XeenRewardCategory::Weapons),
  "Full sword treasure category must jump to success without another item");
 for(unsigned ending:{39,59,60,61}) {
  Fixture e;e.texts.mirrors.resize(62);e.texts.mirrors[ending].name="Ending";
  e.script(28,{record(1,1,0,21,{0,2,0,3}),record(1,1,1,18),record(1,1,2,18)});
  const auto result=e.answer(e.begin(),"ending");const auto *error=std::get_if<XeenEventExecutionError>(&result);
  check(error && error->message=="Ending mirror code is not supported yet","Ending codes must refuse before effects");
 }
}
void input() {
 const XeenFontFormat font(gameplay_test::fontBytes());XeenTextInput input;
 input.type("  hELLO wORLD",font,30,true);check(input.value()=="Hello World","DOS word capitalization");
 input.type(std::string(30,'a'),font,30,true);check(input.value().size()==30,"Word length 30");input.backspace();check(input.value().size()==29,"Backspace");
 IndexedFrame blank{320,200,std::vector<std::uint8_t>(64000)};XeenEventPresenter presenter(font);
 presenter.originalAssets(&dos_test::text(),{});XeenPresentationRequest request;request.kind=XeenPresentationKind::WordInput;
 request.response=XeenPresentationResponseRequirement::WordInput;request.word=XeenEventConfirmWord{};
 presenter.present(blank,request);const auto hit=presenter.input();
 check(hit.textEntry && !hit.click(60,170) && hit.rightClick().has_value() && !hit.key(InputKey::Up),"Word input must own keyboard and mouse without movement");
 presenter.handle(TextInputAction{"alpha"});presenter.handle(DialogKeyAction{8});
 const auto accepted=presenter.handle(DialogKeyAction{13});check(accepted.response && std::get<XeenWordInput>(accepted.response->value).text=="Alph","Enter accepts edited text");
 presenter.present(blank,request);presenter.handle(TextInputAction{"alpha"});
 const auto cancel=presenter.handle(*hit.rightClick());check(cancel.response && std::get<XeenWordInput>(cancel.response->value).text.empty(),"Right mouse cancels and clears");
 presenter.present(blank,request);presenter.handle(TextInputAction{"alpha"});
 const auto escape=presenter.handle(DialogKeyAction{27});check(escape.response && std::get<XeenWordInput>(escape.response->value).text.empty(),"Escape clears the word");
 presenter.present(blank,request);const auto empty=presenter.handle(DialogKeyAction{13});
 check(empty.response && std::get<XeenWordInput>(empty.response->value).text.empty(),"Empty Enter returns failure input");
}
void originalSession(const char *directory,XeenCamera start,const std::string &word,bool refused=false,
 std::optional<XeenCamera> destinationSuspension={}) {
 combat_gameplay_test::Harness h{std::filesystem::path(directory)};auto s=h.services();
 std::string renderFailure;unsigned portraitDraws=0;
 const XeenWorld *composedWorld=nullptr;XeenCamera composedCamera;
 const auto originalCompose=s.composeEncounter;
 s.composeEncounter=[&](auto &world,const auto &party,const auto &camera,auto phase,auto appearance) {
  composedWorld=&world;composedCamera=camera;return originalCompose(world,party,camera,phase,appearance);
 };
 if(destinationSuspension) {
  const auto initialFlags=s.initialFlags;s.initialFlags=[=]{auto flags=initialFlags();flags.clear(220);flags.set(221);return flags;};
  const auto configure=s.configureFlow;s.configureFlow=[&,configure](auto &flow,const auto &camera) {
   configure(flow,camera);flow.drawDialogSprite=[&](auto &frame,const char *name,auto index,auto x,auto y) {
    h.assets->drawDialogSprite(frame,name,index,x,y);
   };
  };
 }
 const auto catalogue=loadXeenItemCatalog(*h.assets);s.catalog=&catalogue.catalog;
 s.npcDraw=[&](auto &frame,auto portrait,auto index){
  try {h.assets->drawNpc(frame,portrait,index);++portraitDraws;}catch(const std::exception &e){renderFailure=e.what();throw;}
 };
 XeenEventTextLoader texts([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
  if(!h.assets->hasArchiveResource(name))return {};return h.assets->readArchiveResource(name);
 },[&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {return h.assets->readArchiveResource(name,XeenSceneArchive::DarksideOnly);});
 s.texts=[&](auto id){return texts.load(id);};
 s.resources.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(h.assets->readInitialResource("maze.pty"));};
 s.resources.loadInitialBankBalances=[&]{return XeenCharacterFormat::parseBankBalances(h.assets->readInitialResource("maze.pty"));};
 s.resources.loadInitialWorldFlags=[&]{return XeenQuestFlagFormat::parseWorldFlags(h.assets->readInitialResource("maze.pty"));};
 s.resources.loadLearnedSpellNames=[&]{return XeenLearnedSpellNames::parse(*h.assets->readLearnedSpellNamesFromDarkArchive());};
 s.resources.regionalManifest=[&](const auto &m,const auto &o,const auto &e,const auto &statistics) {
  xeenValidateRegionalManifest(m,o,e,statistics,h.assets->readInitialResource("maze0023.dat"),h.assets->readInitialResource("maze0023.mob"),h.assets->readInitialResource("maze0023.evt"));};
 s.eventFixture=start;s.saveUnavailable="disabled in the Event developer session";unsigned failures=0,prompts=0,destinationFrames=0,destinationRefreshes=0;
 bool wrongDestinationComposition=false,earlyTravelPublication=false;
 s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
  std::optional<XeenPresentationRequest> request;
  h.flow->reportManual=[&](const auto &result) {
   request.reset();if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&result)) {
    request=p->request;
    if(destinationSuspension && xeen_state::sameCamera(p->state.workingCamera,*destinationSuspension)) {
     ++destinationFrames;
     wrongDestinationComposition|=!composedWorld || composedWorld==h.world || !xeen_state::sameCamera(composedCamera,*destinationSuspension);
     earlyTravelPublication|=!xeen_state::sameCamera(*h.camera,start);
    }
   }
   if(const auto *e=std::get_if<XeenEventExecutionError>(&result)) {++failures;std::cerr<<"Original travel: "<<e->message<<'\n';}
  };
  const auto press=[&](const PlayerAction &action) {
   handler.framePresented(h.flow->frame().presentation());handler.completeInputHandoff(h.flow->frame().presentation());handler.beginCycle(++h.cycle);
   handler.withDisplayedInput(action,*handler.displayedInput());handler.framePresented(h.flow->frame().presentation());handler.completeInputHandoff(h.flow->frame().presentation());
  };
  const auto random=*h.world->sessionState().journeyRandom();const auto purse=*h.party->purse;
  const auto year=h.party->encounterContext->year;
  auto before=h.world->transitionCandidate();
  const auto swords=[&] {unsigned count=0;for(auto id:h.party->party.activeRosterIds())for(const auto &item:h.party->roster.at(id).weapons)if(item.id==34)++count;return count;};
  const auto swordsBefore=swords();
  press(InteractionAction{});
  for(unsigned n=0;n<1000 && h.flow->blocksGameplay();++n) {
   if(destinationSuspension && !destinationRefreshes && request && request->kind==XeenPresentationKind::NpcAcknowledgment) {
    h.flow->refresh(true);++destinationRefreshes;
    wrongDestinationComposition|=composedWorld==h.world || !xeen_state::sameCamera(composedCamera,*destinationSuspension);
   }
   // Poll the NPC key after its three-tick boundary, before idle can reset it.
   if(request && request->kind==XeenPresentationKind::NpcAcknowledgment)h.now+=200;
   if(request && request->kind==XeenPresentationKind::WordInput) {
    ++prompts;press(TextInputAction{word});press(DialogKeyAction{13});
   } else if(request && request->kind==XeenPresentationKind::CharacterSelection)press(SelectMemberAction{0});
   else if(request && request->kind==XeenPresentationKind::NpcAcknowledgment &&
    h.flow->presenter().pageIndex()+1<h.flow->presenter().pageCount())press(AcknowledgeAction{});
   else if(request && request->response==XeenPresentationResponseRequirement::YesNo) {
    if(destinationSuspension && request->kind==XeenPresentationKind::NpcAcknowledgment)press(NoAction{});
    else press(YesAction{});
   }
   else if(request && request->response==XeenPresentationResponseRequirement::Acknowledgment)press(AcknowledgeAction{});
   h.now+=125;handler.beginCycle(++h.cycle);idle();handler.framePresented(h.flow->frame().presentation());handler.completeInputHandoff(h.flow->frame().presentation());
  }
  if(h.flow->blocksGameplay())std::cerr<<"Original travel retained request kind "<<(request?int(request->kind):-1)<<" at map "<<start.mapId.number
   <<" pages "<<h.flow->presenter().pageIndex()<<'/'<<h.flow->presenter().pageCount()<<" portraits "<<portraitDraws
   <<" deadline "<<h.flow->presenter().npcTiming().deadline<<" now "<<h.now<<" failure "<<renderFailure<<'\n';
  check(!h.flow->blocksGameplay() && failures==unsigned(refused),"Original travel session did not settle");
  if(destinationSuspension)check(destinationFrames && destinationRefreshes && portraitDraws && !wrongDestinationComposition && !earlyTravelPublication,
   "Original map 30 railway must draw the destination candidate during its NPC suspension before travel publication");
  if(start.mapId==XeenMapIdentity(28) && start.x==12)check(h.camera->x==27 && h.camera->y==11 &&
   h.party->encounterContext->year==year+1,"Original display-case jail must execute same-map Continue");
  if(word=="i lost it")check(swords()==swordsBefore+1 && *h.world->sessionState().journeyRandom()==random,
   "Original sword path must deliver one sword without a Journey draw");
  if(start.mapId==XeenMapIdentity(28) && start.x==14) {
   check(prompts==1,"Mirror must ask exactly once");
   if(!refused) {
    auto expected=start;
    const auto mirrors=XeenEventTextLoader::parseMirrors(h.assets->readArchiveResource("xeenmirr.txt",XeenSceneArchive::DarksideOnly));
    for(const auto &mirror:mirrors)if(mirror.name==word && mirror.map) {
        expected.mapId=XeenMapIdentity(mirror.map);expected.x=mirror.x;expected.y=mirror.y;
        if(mirror.direction!=-1)expected.direction=XeenDirection(mirror.direction);
        if(!mirror.x && !mirror.y) {
            const auto &g=h.world->map(expected.mapId).geometry;expected.x=g.runX;expected.y=g.runY;
        }
        break;
    }
    check(xeen_state::sameCamera(*h.camera,expected),"Original mirror map/x/y/facing: "+word);
   }
   if(refused)check(xeen_state::sameCamera(*h.camera,start) && *h.world->sessionState().journeyRandom()==random &&
    *h.party->purse==purse && xeen_state::sameMapStates(h.world->sessionState().mapStates(),before->sessionState().mapStates()),"Refused original mirror leaked state/RNG");
   std::cout<<"PLAY "<<word<<" -> "<<h.camera->mapId.number<<" ("<<h.camera->x<<","<<h.camera->y<<") facing "<<unsigned(h.camera->direction)<<" refusal="<<failures<<'\n';
  }
  return true;
 };
 check(Application(s.program).playGameplay(s,{}, {},false,XeenEncounterEntry::Journey,7)==0,"Original travel Application session failed");
}
// Worked Clouds operands, preserving the original ConfirmWord bytes while
// isolating its two outcomes so the comparison is checked independently of
// later, out-of-scope map/service content.
void originalWords(combat_gameplay_test::Harness &h) {
 XeenEventTextLoader loader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
  if(!h.assets->hasArchiveResource(name))return {};return h.assets->readArchiveResource(name);
 });
 for(unsigned map:{54,69,73,82}) {
  const auto original=h.eventLoader->load(map);const auto texts=loader.load(map);
  for(const auto &r:original.records)if(r.opcode==21 && r.parameters[0]) {
   XeenEventFile isolated{map,original.resourceName,true,{r}};
   isolated.records.push_back(record(r.x,r.y,r.line+1,18));
   isolated.records.push_back(record(r.x,r.y,r.parameters[1],1,{0}));
   const auto textAt=[&](unsigned i)->std::string {const auto *p=texts.stringAt(i);return p?*p:texts.strings.front();};
   for(const auto &answer:{textAt(r.parameters[0]),r.parameters[2]?textAt(r.parameters[2]):textAt(r.parameters[0]),std::string("wrong"),std::string{}}) {
    if(answer.size()>30 || !std::all_of(answer.begin(),answer.end(),[](unsigned char c){return c>=32 && c<=127;}))continue;
    Fixture f;f.texts=texts;f.script(map,isolated.records);
    const auto scripts=f.provider();const auto tp=f.textProvider();
    const XeenCamera camera{map,r.x,r.y,r.direction<4?XeenDirection(r.direction):XeenDirection::North};
    auto pending=f.interpreter.begin(camera,f.party,{},f.world,scripts,tp,r.line);
    const auto result=f.answer(std::move(pending),answer);
    const bool matches=!answer.empty() && (answer==textAt(r.parameters[0]) || answer==(r.parameters[2]?textAt(r.parameters[2]):textAt(r.parameters[0])));
    const auto *success=std::get_if<XeenEventExecutionSuspended>(&result);
    check(bool(success)==matches,"Original Clouds password outcome: map "+std::to_string(map)+" line "+std::to_string(r.line));
    if(success)check(success->state.logicalAddress.line==r.parameters[1]+1,"Original alternate must use ordinary success line");
   }
   if(map==69)check(!texts.stringAt(21) && textAt(21)==texts.strings[0],"Map 69 missing alternate aliases text zero");
  }
 }
}
void original(const char *directory) {
 combat_gameplay_test::Harness h{std::filesystem::path(directory)};
 originalWords(h);
 const auto mirrors=XeenEventTextLoader::parseMirrors(h.assets->readArchiveResource("xeenmirr.txt",XeenSceneArchive::DarksideOnly));
 for(unsigned i=0;i<mirrors.size();++i) {const auto &m=mirrors[i];std::cout<<"MIRROR "<<i+1<<" "<<m.name<<" -> "<<unsigned(m.map)<<" ("<<m.x<<","<<m.y<<") dir "<<m.direction<<'\n';}
 for(unsigned id=1;id<=128;++id)for(const auto &r:h.eventLoader->load(id).records)if(r.opcode==21 || r.opcode==7 || r.opcode==31) {
  std::cout<<"SCRIPT "<<id<<" ("<<unsigned(r.x)<<","<<unsigned(r.y)<<") d="<<unsigned(r.direction)<<" line "<<unsigned(r.line)<<" op "<<unsigned(r.opcode)<<" args";
  for(auto b:r.parameters)std::cout<<" "<<unsigned(b);std::cout<<'\n';
  if(r.opcode==21 && r.parameters[0]) {XeenEventTextLoader t([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {if(!h.assets->hasArchiveResource(name))return {};return h.assets->readArchiveResource(name);});auto text=t.load(id);std::cout<<"PASSWORD primary="<<(text.stringAt(r.parameters[0])?*text.stringAt(r.parameters[0]):"[absent]")<<" alternate="<<(text.stringAt(r.parameters[2])?*text.stringAt(r.parameters[2]):"[absent]")<<'\n';}
 }
 for(const auto &word:{"vertigo","magic delta","toad meadow","ever blossom orchard","nightshadow","dragon tower","i lost it"})
  originalSession(directory,{28,14,10,XeenDirection::West},word);
 originalSession(directory,{69,9,15,XeenDirection::West},"wrong");
 originalSession(directory,{28,12,12,XeenDirection::North},"");
 // Actual railway records, quest flag 221 set / 220 clear: line 4 Continue,
 // destination (16,23) line 2 PlayCD, then line 3 NPC.
 originalSession(directory,{30,16,24,XeenDirection::North},"",false,XeenCamera{30,16,23,XeenDirection::North});
}
}
int main(int argc,char **argv) {try {teleports();words();swordCapacityAndEndings();input();if(argc>1)original(argv[1]);return 0;}
 catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
