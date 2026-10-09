// M53 Part B: broader Clouds maps are data/representation tests only.
// Independent graph/text hashes come from the documented CD deltas; no new
// playable-area admission or commercial resource fixture is introduced.
#include "XeenCdEventOracles.h"
#include "XeenTestInstallation.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenFontFormat.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenEventContinuation.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenWorld.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <set>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool b,const std::string &message) { if(!b)throw std::runtime_error(message); }
std::uint32_t crc(const std::vector<std::uint8_t> &bytes) {
 std::uint32_t c=0xffffffffu;
 for(auto b:bytes) {c^=b;for(unsigned i=0;i<8;++i)c=(c>>1)^((c&1)?0xedb88320u:0);}
 return c^0xffffffffu;
}
std::uint32_t crc(const std::string &s) { return crc(std::vector<std::uint8_t>(s.begin(),s.end())); }
std::optional<unsigned> target(const XeenEventRecord &r) {
 switch(r.opcode) {
 case 5:return r.parameters.size()==5 ? std::optional<unsigned>(4) : std::nullopt;
 case 8:case 9:case 10:case 23:case 25:return r.parameters.size()-1;
 case 24:return 0;
 case 42:case 54:return 1;
 default:return {};
 }
}
// Use production first-match lookup, independently checked against the floppy
// graph oracle for all four physical directions, including absent destinations.
std::string graph(const XeenEventScript &script) {
 std::vector<int> ordinal(script.records().size(),-1);int next=0;
 for(unsigned i=0;i<ordinal.size();++i)if(script.records()[i].opcode!=0x3c)ordinal[i]=next++;
 const auto resolve=[&](int x,int y,unsigned d,int line) {
  while(x>=0 && x<=255 && y>=0 && y<=255 && line<=255) {
   auto site=script.findInstructionIndex(x,y,static_cast<XeenDirection>(d),line);
   if(!site)return -1;
   if(script.records()[*site].opcode!=0x3c)return ordinal[*site];
   ++line;
  }
  return -1;
 };
 std::ostringstream out;
 for(const auto &r:script.records()) {
  if(r.opcode==0x3c)continue;
  auto operands=r.parameters;auto t=target(r);if(t)operands.at(*t)=0;
  out<<unsigned(r.x)<<','<<unsigned(r.y)<<','<<unsigned(r.direction)<<','<<unsigned(r.opcode)<<':';
  for(unsigned i=0;i<operands.size();++i)out<<(i?",":"")<<unsigned(operands[i]);
  for(unsigned d=0;d<4;++d)if(r.direction==d || r.direction==4) {
   if(t) {
    int x=r.x,y=r.y;
    if(r.opcode==25){x=static_cast<std::int8_t>(r.parameters.at(0));y=static_cast<std::int8_t>(r.parameters.at(1));}
    out<<'|'<<d<<'='<<resolve(x,y,d,r.parameters.at(*t));
   }
   out<<'/'<<d<<'='<<resolve(r.x,r.y,d,r.line+1);
  }
  out<<'\n';
 }
 return out.str();
}
XeenWorld world() {
 return XeenWorld([](XeenMapIdentity id){XeenMap m;m.geometry.id=id.number;m.side=id.side;m.geometry.flags2=0x8000;return m;});
}
XeenEventFile control(const XeenEventRecord &original) {
 auto r=original;r.x=r.y=1;r.direction=4;r.line=0;
 XeenEventFile f;f.mapId=1;f.resourceName="synthetic.evt";f.resourcePresent=true;f.records={r};
 r.line=1;r.opcode=0x12;r.parameters.clear();r.lengthField=5;f.records.push_back(r);return f;
}
void audio(const XeenEventRecord &original,const XeenFontFormat &font) {
 // The operand/continuation control relocates only the address, leaving every
 // actual inserted PlayCD operand intact. Full original graph is checked above.
 auto f=control(original);auto w=world();XeenPartyState p;XeenEventInterpreter vm;
 const auto provider=[&](auto){return XeenEventScript(f);};
 auto result=vm.begin({1,1,1,XeenDirection::North},p,{},w,provider,{});
 auto pending=std::get_if<XeenEventExecutionSuspended>(&result);
 check(pending && pending->request.kind==XeenPresentationKind::DeferredAudio &&
  pending->request.response==XeenPresentationResponseRequirement::Presented &&
  pending->request.text==(original.opcode==0x3c?"CD speech not supported yet":"Voice audio not supported yet") &&
  pending->state.instructionCount==1 && pending->state.logicalAddress.line==1,
  "Audio notice and exactly one sequential advance");
 XeenEventContinuation retained(pending->state);auto altered=pending->state;
 altered.pendingPresentation->request.source.opcode=0;
 bool rejected=false;try {retained.check(altered);}catch(const std::logic_error &){rejected=true;}
 check(rejected,"Audio continuation source preimage must remain guarded");
 unsigned clockCalls=0,randomCalls=0;
 XeenEventPresenter presenter(font,{},[&]{++clockCalls;return 0;},[&]{++randomCalls;return 0;});
 IndexedFrame base;base.width=320;base.height=200;base.pixels.resize(64000);
 const auto shown=presenter.present(base,pending->request);
 check(shown.response && *shown.response==XeenPresentationResponse::Presented && !presenter.blocksGameplay() &&
  presenter.pageCount()==1 && shown.frame.pixels!=base.pixels && !clockCalls && !randomCalls,
  "Deferred audio must be visible without acknowledgment, timing or RNG");
 // The notice must remain fully readable through NPC, main and bottom panels.
 for(const auto kind:{XeenPresentationKind::NpcAcknowledgment,XeenPresentationKind::MainWindowMessage,XeenPresentationKind::BottomWindowMessage}) {
  XeenEventPresenter layered(font,[](auto &,auto,auto){});
  auto strip=layered.present(base,pending->request).frame;
  XeenPresentationRequest panel;panel.kind=kind;panel.text="Synthetic text";
  if(kind==XeenPresentationKind::NpcAcknowledgment){panel.npc=XeenEventNpc{0,0,0,1,0};panel.response=XeenPresentationResponseRequirement::Acknowledgment;}
  const auto combined=layered.present(strip,panel).frame;
  check(std::equal(strip.pixels.begin(),strip.pixels.begin()+8*320,combined.pixels.begin()),"Event panel erased the deferred-audio notice");
 }
 const auto done=vm.resume(pending->state,*shown.response,p,w,provider,{});
 const auto complete=std::get_if<XeenEventExecutionCompleted>(&done);
 check(complete && complete->instructionCount==2 && complete->finalGameFlags.values()==XeenGameFlags{}.values() &&
  !p.monsterTreasure && w.sessionState().disabledEvents().empty() && w.sessionState().disabledObjects().empty(),
  "Audio continues to Exit without rewards or overlays");
 // Extra/truncated speech refuses before dispatch, with no partially published work.
 for(unsigned size=0;size<8;++size)if(size!=(original.opcode==0x3c?5u:1u)) {
  auto bad=f;bad.records[0].parameters.resize(size);
  const auto failed=vm.begin({1,1,1,XeenDirection::North},p,{},w,[&](auto){return XeenEventScript(bad);},{});
  const auto error=std::get_if<XeenEventExecutionError>(&failed);
  check(error && error->kind==XeenEventExecutionErrorKind::MalformedInstruction && error->instructionCount==0,
   "Malformed audio must refuse before any dispatch");
 }
 auto overflow=f;overflow.records[0].line=255;
 auto refused=vm.begin({1,1,1,XeenDirection::North},p,{},w,[&](auto){return XeenEventScript(overflow);},{},255);
 check(std::get<XeenEventExecutionError>(refused).kind==XeenEventExecutionErrorKind::LineOverflow,
  "Audio cannot wrap line 255 to zero");
}
void refusal(const XeenEventRecord &original) {
 auto f=control(original);auto w=world();XeenPartyState party;XeenEventInterpreter vm;
 const auto result=vm.begin({1,1,1,XeenDirection::North},party,{},w,[&](auto){return XeenEventScript(f);},{});
 const auto error=std::get_if<XeenEventExecutionError>(&result);
 check(error && error->instructionCount==0 && error->source && error->source->opcode==original.opcode &&
  !party.monsterTreasure && w.sessionState().disabledEvents().empty(),
  "First unsupported/malformed decoded mechanic must refuse before dispatch or effects");
}
void choice(const XeenEventRecord &original) {
 auto f=control(original);const auto targetLine=original.parameters.back();
 // Retain the original comparison and jump operand; provide synthetic terminals
 // only, so every supported original action-44 answer can be checked in isolation.
 if(targetLine!=1) {auto end=f.records.back();end.line=targetLine;f.records.push_back(end);}
 auto w=world();XeenPartyState party;XeenEventInterpreter vm;
 const auto provider=[&](auto){return XeenEventScript(f);};
 auto result=vm.begin({1,1,1,XeenDirection::North},party,{},w,provider,{});
 const auto pending=std::get_if<XeenEventExecutionSuspended>(&result);
 check(pending,"Original action-44 choice must suspend");
 const auto responses=pending->request.response==XeenPresentationResponseRequirement::YesNo ?
  std::vector<XeenPresentationResponse>{XeenPresentationResponse::Yes,XeenPresentationResponse::No} :
  std::vector<XeenPresentationResponse>{XeenPresentationResponse::Acknowledged};
 for(const auto response:responses) {
  const auto completed=vm.resume(pending->state,response,party,w,provider,{});
  check(std::holds_alternative<XeenEventExecutionCompleted>(completed),"Supported original choice must resolve its original jump/fallthrough");
 }
}
void map33(const XeenEventFile &f,const XeenEventTextFile &texts,XeenPartyState party) {
 auto w=world();XeenEventInterpreter vm;
 const auto scripts=[&](auto){return XeenEventScript(f);};const auto text=[&](auto){return texts;};
 auto first=vm.begin({33,2,8,XeenDirection::North},party,{},w,scripts,text);
 const auto request=std::get<XeenEventExecutionSuspended>(first);
 check(request.request.kind==XeenPresentationKind::CharacterSelection && request.request.verbIndex==0 &&
  request.request.textIndex==15 && request.request.text==*texts.stringAt(15),"CD map-33 search prompt and unchanged text 15");
 const auto cancelled=vm.resume(request.state,CharacterSelectionCancelled{},party,w,scripts,text);
 check(std::holds_alternative<XeenEventExecutionCompleted>(cancelled) && w.sessionState().disabledEvents().empty(),
  "Map-33 cancellation has no overlay");
 // The next real instruction is AlterEvent. Map 33 remains outside the
 // playable Journey; keep that existing refusal and prove no partial overlay.
 bool refused=false;
 try {
  const auto result=vm.resume(request.state,SelectedCharacter{0},party,w,scripts,text);
  if(const auto *error=std::get_if<XeenEventExecutionError>(&result)) {
   refused=error->kind==XeenEventExecutionErrorKind::PresentationFailed &&
    error->message=="AlterEvent replacement is unsupported" && error->logicalAddress.line==1;
  }
 }
 catch(const std::invalid_argument &e){refused=std::string(e.what())=="AlterEvent replacement is unsupported";}
 check(refused && w.sessionState().disabledEvents().empty(),"Map-33 first unsupported mechanic refuses without partial publication");
 // Selection-only representation control: replace the unsupported suffix with
 // its following display to observe the selected context without admitting it.
 auto selectable=f;
 const auto alter=*XeenEventScript(f).findInstructionIndex(2,8,XeenDirection::North,1);
 selectable.records[alter].opcode=1;selectable.records[alter].parameters={5};
 auto state=request.state;state.currentScript.emplace(selectable);
 const auto selected=vm.resume(state,SelectedCharacter{0},party,w,[&](auto){return XeenEventScript(selectable);},text);
 const auto shown=std::get<XeenEventExecutionSuspended>(selected);
 check(shown.state.activeCharacterIndex==0 && shown.request.textIndex==5,"CD search selection retains selected-character context");
}
}
int main(int argc,char **argv) try {
 check(argc==2,"usage: cd_event_original <CD installation>");
 const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Missing CD source");
 XeenAssetSource assets(*installation);
 const auto initial=[&](const std::string &n)->std::optional<std::vector<std::uint8_t>>{return assets.readInitialResource(n);};
 const auto archive=[&](const std::string &n)->std::optional<std::vector<std::uint8_t>>{return assets.readArchiveResource(n);};
 XeenFontFormat font(assets.readArchiveResource("fnt"));
 unsigned speech=0,records=0,fourOperand=0;
 for(const auto &expected:cd_event_oracle::maps) {
  const auto f=XeenEventLoader(initial).load(expected.id);XeenEventScript script(f);
  const auto context="CD map "+std::to_string(expected.id);
  check(f.resourcePresent && f.records.size()==expected.records,context+" record topology");
  check(crc(assets.readInitialResource(f.resourceName))==expected.raw,context+" actual record bytes/operands differ from audit");
  check(crc(graph(script))==expected.graph,context+" resolved branch/AlterEvent/fallthrough differs from documented floppy mapping");
  unsigned count=0;
  for(unsigned i=0;i<f.records.size();++i) {
   const auto &r=f.records[i];++records;
   const auto decoded=XeenEventDecoder::decode(r,{f.mapId,f.resourceName,i,true});
   if(std::holds_alternative<XeenEventDecodeError>(decoded))refusal(r);
   if((r.opcode==8 || r.opcode==9 || r.opcode==10) && r.parameters.size()==3 && r.parameters[0]==44 && r.parameters[1]<=1)
    choice(r);
   if(r.opcode==0x3c) {
    check(std::holds_alternative<XeenDecodedEventInstruction>(decoded),context+" inserted speech decode");
    audio(r,font);++count;++speech;
   }
   if(r.opcode==5 && r.parameters.size()==4) {
    check(std::holds_alternative<XeenEventDecodeError>(decoded),"Four-operand NPC must refuse safely, without invented branch");++fourOperand;
   }
  }
  check(count==expected.speech,context+" inserted speech count");
  if(expected.id==14 || expected.id==28) {
   const unsigned x=expected.id==14?8:15,y=expected.id==14?2:0;
   for(const auto &r:f.records)if(r.x==x && r.y==y && r.direction==2)
    check(r.opcode!=0x2f && r.opcode!=0x18,"CD protection and self-disable pair must not be synthesized");
  }
  if(expected.id==33)map33(f,XeenEventTextLoader(archive).load(33),XeenPartyLoader().loadInitialCloudsParty(assets));
 }
 std::set<unsigned> textMaps;
 for(const auto &expected:cd_event_oracle::texts) {
  const auto text=XeenEventTextLoader(archive).load(expected.map);const auto s=text.stringAt(expected.index);
  check(s && crc(*s)==expected.bytes,"Changed CD text/control stream map "+std::to_string(expected.map)+" entry "+std::to_string(expected.index));
  textMaps.insert(expected.map);
 }
 // Existing voice cues use the same generic nonblocking notice at any site/index.
 for(auto index:{0,10,255}) {XeenEventRecord r;r.opcode=0x28;r.parameters={static_cast<std::uint8_t>(index)};audio(r,font);}
 // Metadata-only archive view: never load Darkside maps, party or actors.
 auto darkView=*installation;darkView.cloudsData=installation->darksideData;
 darkView.xeenArchive=installation->darkArchive;
 XeenAssetSource darkMetadata(darkView);
 const auto mirror=darkMetadata.readArchiveResource("xeenmirr.txt");
 check(mirror.size()==1984 && mirror.size()%32==0 &&
  std::string(mirror.begin()+1952,mirror.begin()+1960)=="bloopers" &&
  mirror[1980]==0 && mirror[1981]==0 && mirror[1982]==0 && mirror[1983]==0,
  "DARK.CC bounded Clouds mirror table appends only the zero-destination Bloopers keyword");
 XeenEventRecord teleport;teleport.opcode=7;teleport.parameters={0};
 check(std::holds_alternative<XeenEventDecodeError>(XeenEventDecoder::decode(teleport)),"Map-zero mirror remains unsupported");
 check(textMaps.size()==20 && fourOperand>0,"Complete changed-text and four-operand NPC inventory");
 std::cout<<"26 CD scripts, "<<records<<" parsed records, "<<speech<<" inserted speech records; 49 entries in 20 text resources verified\n";
 return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
