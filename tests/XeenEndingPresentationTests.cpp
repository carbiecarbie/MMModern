#include "formats/xeen/XeenCompressedFrame.h"
#include "app/XeenCutsceneFlow.h"
#include "app/XeenAudio.h"
#include "games/xeen/XeenTextRenderer.h"
#include "platform/sdl/SdlWindow.h"
#include <fstream>
#include <algorithm>
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCutsceneData.h"
#include "XeenTestInstallation.h"
#include <windows.h>
#include <bcrypt.h>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <limits>
#include <cmath>
#include <map>
using namespace mmodern;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::string sha256(const std::vector<std::uint8_t> &bytes) {
	BCRYPT_ALG_HANDLE algorithm = nullptr;
	BCRYPT_HASH_HANDLE hash = nullptr;
	DWORD objectLength = 0, hashLength = 0, received = 0;
	std::vector<std::uint8_t> object;
	std::vector<std::uint8_t> digest;
	auto cleanup = [&] {
		if (hash) BCryptDestroyHash(hash);
		if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
	};
	try {
		check(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
			nullptr, 0) >= 0, "cannot open SHA-256 provider");
		check(BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &received, 0) >= 0,
			"cannot query SHA-256 object length");
		check(BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
			reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength), &received, 0) >= 0,
			"cannot query SHA-256 digest length");
		object.resize(objectLength);
		digest.resize(hashLength);
		check(BCryptCreateHash(algorithm, &hash, object.data(), objectLength,
			nullptr, 0, 0) >= 0, "cannot create SHA-256 hash");
		check(bytes.size() <= static_cast<std::size_t>(std::numeric_limits<ULONG>::max()),
			"reference file is too large for SHA-256 check");
		check(BCryptHashData(hash, const_cast<PUCHAR>(bytes.data()),
			static_cast<ULONG>(bytes.size()), 0) >= 0, "cannot update SHA-256 hash");
		check(BCryptFinishHash(hash, digest.data(), hashLength, 0) >= 0,
			"cannot finish SHA-256 hash");
		cleanup();
	} catch (...) {
		cleanup();
		throw;
	}
	std::ostringstream out;
	out << std::hex << std::setfill('0');
	for (const auto byte : digest)
		out << std::setw(2) << static_cast<unsigned>(byte);
	return out.str();
}


void frames(XeenAssetSource &assets) {
 // Part 0 frm-decode.json: output hashes only; never original payload bytes.
 const std::pair<const char *,const char *> expected[]{
  {"prec0001.frm","eaf531fc1315c768652ffbaab93c5f8b1430945ce608841832c117927836c40a"},
  {"prec0002.frm","0b363222e4c7ec765a8d33f0706fd4a68f4a2f5cef4493f21e990c731c0a8eeb"},
  {"prec0003.frm","f7ea22968b2e23d945b95cd8c59ba1fd3e13f230708d2764c03ecfdeb6518201"},
  {"prec0004.frm","b859b49c3b8458f9fa3625176ee1fc662fda3c54113ebdade3ae6fcd84b6ce6b"},
  {"prec0005.frm","a091e2661223bc31cf0eb68fa0f79d47dd2fc604148748c678091626811a2912"},
  {"prec0006.frm","d8cb3c166a0aa5ff8fd14c8678d3d91c9a51758903681fdc2b93130983f10695"},
  {"prec0007.frm","f2890e34e540d63acbf49e26d60353bffb095e4820398dcffb6ea45404b172c3"},
  {"prec0008.frm","9672150104be1818f6e0fb5e4ce83b9977437ea62bd8773a4a4e2a2c04dcb5a1"},
  {"prec0009.frm","5458b7e42038ad572bafe7ee3f86b53dc72ff8a5d7f95ddf0353dfcc6c94f1cd"},
  {"prec0010.frm","11cfe68c2576894206949c5e08ad5288dd14afab25ba86e3420a1d437ae7f93c"},
  {"prec0011.frm","69cda13c2f511657776b5598a364b4ce8196dc138737bc597cb5fc83309887fe"},
  {"prec0012.frm","29212ca70f4372adb721f2b6ea8c05245b2a2d48ba91f548f5418a566b0ac902"},
  {"prec0013.frm","04f0464310e356c99edc4e44d20eb868899b25c97f2f56e5cfaa47948985a021"},
  {"prec0014.frm","5329ad166f05d353cd0459ee047e3368206cae3384dc9ae168fffec93545b936"},
  {"prec0015.frm","df83bc6ac8c8ce0f304c45cbc03afb69a4e44a4efa478659d8393d54fc0114e1"},
  {"prec0016.frm","d11f52c24f9eb87149fd2af17988e8fed080cd9997feb3c54561d4aedb7398b8"},
  {"prec0017.frm","215c4cfd2d0217d512e5cf41d89b08f382ef9eb4df2e8704fc900a6cdb623f60"},
  {"prec0018.frm","095107dad2cab0abfcc4e63d7bdbbd0c8721302cf17a3dffbc9689cb4183c31b"},
  {"prec0019.frm","92ed93ab4059a590ea157d9760cac66b350a49b4398dc47660763bf4ee7a35cd"},
  {"prec0020.frm","45282ba346e97525f5ce6677ce6eaff203a96ca591e504135a954f08d8cb050e"},
  {"prec0021.frm","a94ce7efc9afefb53add5d82e32d58fc93132b55b03766ee4b86a1a33025fef4"},
  {"prec0022.frm","e34eff7aaa71781843377de68853a0bfdb2c9c5ce91dc36f161536ef6718ddb1"},
  {"prec0023.frm","a242feeec2d034c84987cf363d5a86de4084673759dbbcd561d3b3d0801fccd1"},
  {"prec0024.frm","79e07cf85e54077b05f67aa3336a12cb04ab4b9b93ab777d60e3a5b05b54c100"},
  {"prec0025.frm","4d6e5e2adbed4ecb68f9fe3ad37d6b54d4a1e408d39e873495e4a647acce2e39"},
  {"prec0026.frm","363481eb1d9ed8761992b0bb8c9a6585b6a2a0704207484c61190da473af4576"},
  {"prec0027.frm","814fb2f356ed0b18d2519aa19d42d967d3307f7a0d35bb0a55cc6c58b41b3b7a"},
  {"prec0028.frm","2a7eda3634d48d4a14f457752e310c3dab30179ab8291801e50af03207f983fc"},
  {"prec0029.frm","bfd1887b6f0d0bdc51c69df63f60293b82ba13bf34af6329972c5825c099a8cb"},
  {"prec0030.frm","e2cef8923c372cff2e5c4ed5bf76053d2c0817198408f44766fda68a7f059cf0"},
  {"prec0031.frm","d7e9837b46ee288ec80455e0efd164a5a42ac7e8defc974e117d1c88941524e6"},
  {"prec0032.frm","84d9e288de65cdcba8e52d313e9008ea6406e1a7620f68be8e4bdf76f86a37c0"},
  {"prec0033.frm","34d3d7bd0ed189b88e7539727364f348c008f70028aee28c21a79a9c5afe9f8e"},
  {"prec0034.frm","75a03c6ae11363899fb6c4f60d74d2f6d38e454cc66d70a3b8b86f62b3f62a52"},
  {"prec0035.frm","9a8881e9ae0d79a312416a90389c8d1a5c01dfbba424e15562cc108e0a9e0998"},
  {"prec0036.frm","0cbc0d1c88d19fb492accf8c4d021aace9a644dc873496845d2389715eee8ed1"},
  {"prec0037.frm","dbf9cfdc512747c2c4ed021fa3bf726204ad08f190b077c315df6d5a9652141c"},
  {"prec0038.frm","d9637373097d04d0cd5e22a45a49768c7a73157a123fe6a893a62e1a053fd264"},
  {"prec0039.frm","c5118b095199de756dbf8e629698fe3c99da5f06e38794e0ef8b3d5d8f1ebcbe"},
  {"prec0040.frm","e5b201a036949214a35dee8765b1b9f73e07ac0c86348ed03572ddc5b98a574b"},
  {"prec0041.frm","0a96c6ff495210ae57482a46a7c7ccecbfcad06ee46e9b731c43ce4c614de6e8"},
  {"vort01.frm","cc4168aedb34f9e5c3e30fa8b42d4fe085c0f34e9bf333651e3696292c1189dc"},
  {"vort02.frm","6af629cae0a2f2fecef97aff307b79cd1fd88e0189a1f76bd16d24b861d65e38"},
  {"vort03.frm","bcbb1cc67da6d84a0839076cc67f7419b0e62691f621d99248d85a425287b510"},
  {"vort04.frm","f61e108191c607f1524c62e3533984fd5589983b3f10afdc3650c8bb38d59ab1"},
  {"vort05.frm","c50202c8fee35ef4bc5b10e2fc5abb3dd75c6285ca352077982d8c1650c427fd"},
  {"vort06.frm","b7315ee93e476cf68f863fa685153f2a06d807381390205afc23bd29760dab1d"},
  {"vort07.frm","590a3290f4f88a27f22f9a7abbb93b376c75ffbb05082f0b9b43084294c203f6"},
  {"vort08.frm","91d357073f7a4234fb714b48d650d8dc8fc6bdb79e39ee77f6e58994907d64f6"},
  {"vort09.frm","76cd9db4db7de181ddc4b41cecb6401e3e4ab5858a1a810523c59def3e79b184"},
  {"vort10.frm","113b2607434203e3ee76acd0a283fead2aa72784e202028198228ea4807c9ae3"},
  {"vort11.frm","fe3cc9c29f5114843fc2134ce4d8fc7c6fb1eeeca93334327112f71cd4849892"},
  {"vort12.frm","5cce56d0f8a8d729b4099ed3847e575711cd9d9bf0b1dbbd143a7948a8e99356"},
  {"vort13.frm","e0abb4e9e946c97639f214bc6bc5ea04cfd67edeecb2f2880dfb9da433a3792e"},
  {"vort14.frm","6de680143dad80eee26009469a62d13d61087365eab6640fa1fbd22a34147e42"},
  {"vort15.frm","1f6879b6ce9970ce040bc7be0dee5ec92023b08059b00ac254a7460ed85653c2"},
  {"vort16.frm","a9627c171e92acf4e5686dbefdf4612a00433698f9b0a5480b82810dfd2da05b"},
  {"vort17.frm","2d1cedbac09beca814a8ecfd824f9dd370ab2daa265f0e6d1c049471c053e9bd"},
  {"vort18.frm","5f5d593647d7f99e5618a54a98b900fce6d4eaa96f8d745a30891e83907444d9"},
  {"vort19.frm","1f284878fda0a9022ea4853f0cf88569dd02106be05bbd873dd1ae8e6fd81594"},
  {"vort20.frm","662b29020d62f5ce4dc55df66798f316a7233af83d13f6ce5e40477aae1273c1"},
 };
 unsigned partial=0;
 for(const auto &[name,digest]:expected) {
  auto bytes=assets.readArchiveResource(name,XeenSceneArchive::CloudsProgram);
  const auto image=decodeXeenCompressedFrame(bytes);
  const auto actual=sha256(image);
  if(actual!=digest)throw std::runtime_error(std::string("Frame SHA-256: ")+name+" expected "+digest+" actual "+actual);
  partial+=bytes.size()%2;
  // Removing a byte that contains required bits must fail, including the 24
  // bounded final-word cases. No absent low byte may be silently consumed.
  bytes.resize(bytes.size()-2);bool refused=false;
  try{decodeXeenCompressedFrame(bytes);}catch(const std::runtime_error &){refused=true;}
  check(refused,"Consumed unavailable compressed-frame bits");
 }
 check(partial==24,"Expected 24 final partial-word resources");
 for(const auto &bad:std::vector<std::vector<std::uint8_t>>{{},{0,0,250},{0,0,0,0},{0,0,250,0}}) {
  bool refused=false;try{decodeXeenCompressedFrame(bad);}catch(const std::runtime_error &){refused=true;}
  check(refused,"Malformed compressed frame admitted");
 }
}
void randomSource() {
 for(unsigned v=0;v<65536;++v) {
  check(XeenPresentationRandom::reduce(v,20,29)==20+(v&32767)%10,"Inclusive digit range");
  check(XeenPresentationRandom::reduce(v,7,7)==7,"Singleton random range");
 }
 XeenPresentationRandom arithmetic(XeenWallClock::fixed(0),{0,0,0});
 check(arithmetic.mix({0,0,0,0})==0xb000,"Time source first carry vector");
 check(arithmetic.mix({0,0,0,0})==0x7010,"Time source feedback/borrow vector");
 const auto seconds=xeenDosTime({2026,10,26,23,59,59});
 XeenWallClock::Sample sample{seconds,99,xeenDosLocalTime(seconds)};unsigned reads=0;
 auto clock=XeenWallClock::withHundredths([&]{++reads;return sample;});
 XeenPresentationRandom first(clock,{0x1234,0x5678,0x9abc}),second(clock,{0x1234,0x5678,0x9abc});
 for(unsigned i=0;i<100;++i)check(first.range(0,32766)==second.range(0,32766),"Fixed-clock sequence is not reproducible");
 check(reads==200,"Time source did not take one coherent sample per draw");
 XeenPresentationRandom centiseconds(clock,{0,0,0}),whole(XeenWallClock::fixed(seconds),{0,0,0});
 check(centiseconds.range(0,32766)!=whole.range(0,32766),"Wall-clock hundredths were discarded");
 sample={seconds+1,0,xeenDosLocalTime(seconds+1)};check(first.range(1,6)==second.range(1,6),"Midnight rollover is not deterministic");
 XeenPresentationRandom a(XeenWallClock::fixed(seconds),{0,0,0}),b(clock,{0,0,0});
 check(a.range(0,32766)!=b.range(0,32766),"Injected wall time does not affect presentation source");
 bool refused=false;try{XeenPresentationRandom::reduce(0,7,6);}catch(const std::invalid_argument &){refused=true;}
 check(refused,"Invalid random range accepted");
 for(const XeenCivilTime civil: {XeenCivilTime{2026,4,5,2,30,45},XeenCivilTime{2039,2,1,17,45,23}}) {
  auto host=XeenWallClock::withHundredths([=]{return XeenWallClock::Sample::fromCivil(civil,73);});
  check(xeenDosLocalTime(host.now()).hour!=civil.hour,"Civil-time regression fixture does not expose the inverse loss");
  XeenPresentationRandom actual(host,{0x1234,0x5678,0x9abc}),registers(XeenWallClock::fixed(0),{0x1234,0x5678,0x9abc});
  for(unsigned i=0;i<16;++i)check(actual.range(0,32766)==XeenPresentationRandom::reduce(
   registers.mix({unsigned(civil.hour),unsigned(civil.minute),unsigned(civil.second),73}),0,32766),"RNG lost host civil time at DST or after 2038");
 }
}
void kinds() {
 using C=XeenCutscene;using I=C::Segment::Input;
 std::uint64_t now=0;C scene;
 const auto add=[&](const char *name,C::Frame f,I input=I::Ignore,unsigned count=1){
  C::Segment s;s.name=name;s.input=input;s.frames=count;s.compose=[f](const auto &){return f;};scene.segments.push_back(std::move(s));
 };
 add("merge",{C::Raw{"a"},C::Store{"a"},C::Raw{"b"},C::Store{"b"},C::HorizontalMerge{"a","b",319}},I::Ignore,2);
 add("rotation",{C::Rotate{"a",321}});
 add("scroll",{C::Level{32}},I::Scroll,100);
 scene.segments.back().terminal={C::Level{128}};
 add("page",{},I::Keyboard);add("after",{});
 XeenCutsceneFlow::Services s;s.clock=[&]{return now;};s.draw=[](auto &,const auto &){};s.audio=[](const auto &,auto){};s.digitalBusy=[](auto){return false;};
 s.initial=IndexedFrame{320,200,std::vector<std::uint8_t>(64000)};s.initial->palette.fill(100);
 s.read=[](const std::string &name){std::vector<std::uint8_t> b(64000);for(unsigned y=0;y<200;++y)for(unsigned x=0;x<320;++x)b[y*320+x]=name=="a"?x%251:252;return b;};
 XeenCutsceneFlow f(scene,s);
 const auto acquire=[&]{f.presented(f.frame().presentation());f.completeInput(f.frame().presentation());};acquire();
 const auto step=[&]{auto next=f.nextPresentationDeadline();check(bool(next),"Missing primitive deadline");now=std::max(now,*next);if(f.update())acquire();};
 const auto until=[&](const char *name){for(unsigned g=0;g<1000 && (f.segmentName()!=name || !f.inputContext(f.frame().presentation()).readyForAction);++g)step();check(f.segmentName()==name,"Primitive not reached");};
 check(f.frame().pixels[0]==68 && f.frame().pixels[1]==252 && f.frame().pixels[320]==68,"Horizontal merge crossed rows");
 f.handle(CancelInteractionAction{},f.frame().presentation());f.handle(DialogKeyAction{32},f.frame().presentation());
 until("rotation");check(f.frame().pixels[0]==1 && f.frame().pixels[319]==0,"320-column rotation");
 until("scroll");step();check(f.segmentName()=="scroll","Ignored animation key leaked into scroll");
 f.handle(DialogKeyAction{27},f.frame().presentation());
 for(unsigned g=0;g<10 && f.frame().palette[0]!=100;++g)step();
 check(f.segmentName()=="scroll" && f.frame().palette[0]==100,"Scroll did not publish its terminal image");
 // A terminal scroll frame is acquired before the caller's next segment.
 for(unsigned g=0;g<10 && f.segmentName()=="scroll";++g)step();
 until("page");check(!f.nextPresentationDeadline(),"Keyboard hold did not wait");
 check(!f.inputContext(f.frame().presentation()).acceptsQueuedInput,"Page retained queued input");
 check(!f.inputContext(f.frame().presentation()).dialog->click(5,5),"Mouse acknowledged keyboard hold");
 f.update();check(f.segmentName()=="page","Scroll key leaked into acknowledgement");
 f.handle(DialogKeyAction{27},f.frame().presentation());step();until("after");step();
 check(f.done(),"Escape did not acknowledge keyboard page");
 const auto c=XeenCutsceneData::captions({'a',0,0,'b',0});
 check(c.offsets==std::vector<std::uint16_t>({0,2,3}) && c.records.size()==3 && c.record(2,2)=="b","Every-NUL caption offsets");
}
void cueSites(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions) {
 auto p=XeenCutscene::cloudsEnding(data,captions,XeenWallClock::fixed(0),71001722);unsigned vortex=1;
 for(const auto &s:p.segments) {
  int table=-2;
  if(s.name=="ending pyramid")table=-1;
  else if(s.name.find("castle destruction ")==0)table=0;
  else if(s.name=="Alamar appears")table=1;
  else if(s.name.find("Alamar voice ")==0)table=2;
  else if(s.name=="Alamar disappears")table=3;
  if(table==-2)continue;
  const unsigned count=s.frames?s.frames:21; // Speech also crosses the wrap.
  for(unsigned i=0;i<count;++i) {
   const auto f=s.compose({i,i*3,i*3,true});
   check(std::get<XeenCutscene::Compressed>(f.front()).resource==data.ending.vortex[vortex-1],"Vortex image cycle");
   vortex=vortex%20+1;const auto slot=table==0?vortex:vortex-1;
   const unsigned expected=table<0 || slot>=20?0:data.ending.vortexFm[table][slot];unsigned actual=0;
   for(const auto &op:f)if(const auto *a=std::get_if<XeenCutscene::Audio>(&op);a && a->kind==XeenCutscene::Audio::Fm)actual=a->value;
   check(actual==expected,"Vortex cue used the image index instead of the incremented DOS counter");
  }
 }
}
void delayedPublication(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions) {
 const auto program=XeenCutscene::cloudsEnding(data,captions,XeenWallClock::fixed(0),71001722);
 for(const char *name:{"mirror room pan","score heading","score random digits","score settling digits"}) {
  const auto found=std::find_if(program.segments.begin(),program.segments.end(),[&](const auto &s){return s.name==name;});
  check(found!=program.segments.end(),"Missing delay-test segment");XeenCutscene p;p.segments.push_back(*found);auto &s=p.segments.back();
  const bool heading=std::string(name)=="score heading";s.enter.clear();s.entering={};s.finished={};s.published={};s.frames=heading?0:2;
  unsigned composed=0;s.compose=[&](const auto &)->XeenCutscene::Frame{++composed;return {};};
  std::uint64_t now=0;XeenCutsceneFlow::Services services;services.clock=[&]{return now;};services.read=[](const auto &){return std::vector<std::uint8_t>{};};
  services.draw=[](auto &,const auto &){};services.audio=[](const auto &,auto){};services.digitalBusy=[](auto){return false;};
  XeenCutsceneFlow f(std::move(p),services);now=PresentationClock::boundary(3);f.presented(f.frame().presentation());f.completeInput(f.frame().presentation());f.update();
  check(heading?f.done():composed==2,"Publication delay restarted an elapsed EXE hold");
 }
}
// A developer preview uses the same score-page segments and renderer, without
// replaying three minutes of preceding scenes or touching saves/settings.
XeenCutscene scorePages(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions) {
 using C=XeenCutscene;using E=XeenCutsceneData::Ending;
 auto program=C::cloudsEnding(data,captions,XeenWallClock::fixed(0),71001722);
 const auto find=[&](const char *name)->C::Segment {
  const auto i=std::find_if(program.segments.begin(),program.segments.end(),[&](const auto &s){return s.name==name;});
  check(i!=program.segments.end(),"Missing score preview segment");return *i;
 };
 // Set the final digits through the production settling operation.
 find("score settling digits").compose({9,0,0,false});
 C pages;auto first=find("score page 1");
 first.enter={C::Palette{data.ending.resources[E::MirrorPalette]},C::Raw{data.ending.resources[E::MirrorRaw]},
  C::Sprite{data.ending.resources[E::Sky]},C::Store{"sky"}};
 pages.segments={std::move(first),find("score page 2")};return pages;
}
void picture(const IndexedFrame &frame,const std::filesystem::path &path) {
 std::ofstream out(path,std::ios::binary);out<<"P6\n320 200\n255\n";
 for(auto p:frame.pixels)out.write(reinterpret_cast<const char *>(frame.palette.data()+3*p),3);
 check(bool(out),"Cannot save score picture");
}
void pageLayout(const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions,const XeenFontFormat &font) {
 // Exercise EXE SHR/ADC at both implicit and explicit center anchors,
 // while retaining the existing DAT rounding unless explicitly requested.
 XeenTextRenderOptions rounding;rounding.bounds={71,68,246,152};rounding.x=71;rounding.y=68;
 rounding.originalControls=true;rounding.alignment=XeenTextAlignment::Center;
 IndexedFrame base{320,200,std::vector<std::uint8_t>(64000)};
 for(const int anchor:{71,159}) {
  rounding.x=anchor;rounding.exeCenterRounding=false;
  const auto dat=XeenTextRenderer(font).render(base,"Press a Key",rounding);
  rounding.exeCenterRounding=true;const auto exe=XeenTextRenderer(font).render(base,"Press a Key",rounding);
  if(exe.writeX!=dat.writeX+1 || exe.writeX!=195)throw std::runtime_error("EXE centered run: anchor="+std::to_string(anchor)+" dat="+std::to_string(dat.writeX)+" exe="+std::to_string(exe.writeX)+" width="+std::to_string(XeenTextRenderer(font).textWidth("Press a Key",XeenFontSize::Normal)));
 }
 const auto pages=scorePages(data,captions);
 for(const auto &segment:pages.segments) {
  const auto frame=segment.compose({0,0,0,false});const auto &text=std::get<XeenCutscene::Text>(frame.back());
  // FILE 0xa353..0xa363: (63,60), 191x100. Window 28 margin=8 at
  // CS:0222 (FILE 0x12962); 0x19540..0x19583 computes the inner rectangle.
  check(text.x==71 && text.y==68 && text.left==71 && text.top==68 && text.right==246 && text.bottom==152,"Score window inner geometry");
  XeenTextRenderOptions o;o.bounds={text.left,text.top,text.right,text.bottom};o.x=text.x;o.y=text.y;o.originalControls=true;o.startupColors=true;o.exeCenterRounding=true;
  IndexedFrame blank{320,200,std::vector<std::uint8_t>(64000)};
  const auto rendered=XeenTextRenderer(font).render(blank,text.value,o);
  check(rendered.diagnostics.empty() && rendered.pages.size()==1,"Score text rendering diagnostics");
  // DS 21db/2263: color 23, left, v000/t000; color 33, centered,
  // v070/t000. 0x189ec..0x189ff adds inner top. 0x18aa6 advances 10.
  const auto prompt=text.value.find(char(12),1);check(prompt!=std::string::npos,"Score prompt control missing");
  const auto body=text.value.substr(0,prompt);
  const auto bodyResult=XeenTextRenderer(font).render(blank,body,o);
  check(bodyResult.writeY==98 && bodyResult.writeColor==23,"Score body is not four ten-pixel lines in color 23");
  check(rendered.writeY==138 && rendered.writeColor==33,"Score prompt offset/color");
  const auto promptWidth=XeenTextRenderer(font).textWidth("Press a Key",XeenFontSize::Normal);
  // 0x18810..0x18818: add one, subtract measured width, SHR/ADC.
  check(promptWidth==71 && rendered.writeX==195,"Score prompt starts at x=124 (EXE upward centering)");
  auto expected=bodyResult.pages.front();
  XeenTextRenderOptions promptOptions=o;promptOptions.y=138;promptOptions.alignment=XeenTextAlignment::Center;promptOptions.colorIndex=33;
  expected=XeenTextRenderer(font).render(expected,"Press a Key",promptOptions).pages.front();
  check(rendered.pages.front().pixels==expected.pixels,"Score prompt geometry differs from explicit placement");
  check(std::count(expected.pixels.begin(),expected.pixels.end(),128)>0,"Score body startup palette entry 128");
  int left=320,right=0,top=200,bottom=0;unsigned pixels=0;
  for(int y=0;y<200;++y)for(int x=0;x<320;++x)if(expected.pixels[y*320+x]==189) {
   ++pixels;left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);
  }
  check(left==124 && right==193 && top==138 && bottom==146 && pixels==194,"Score prompt differs from DOS 253/255 s pixel bounds");
  if(segment.name=="score page 1") {
   // DOS capture 253 s: wrapping must leave exactly these four lines.
   const std::string lines="Please send this score to\nthe Ancient's Headquarters\nwhere you'll be added to the\nHall of Legends!";
   auto lineOptions=o;lineOptions.colorIndex=23;
   const auto explicitLines=XeenTextRenderer(font).render(blank,lines,lineOptions);
   check(bodyResult.pages.front().pixels==explicitLines.pages.front().pixels,"Score page one DOS wrapping");
  }
 }
}
void previewPages(const GameInstallation &installation,XeenAssetSource &assets,const XeenFontFormat &font,
 const XeenCutsceneData &data,const XeenCutsceneData::Captions &captions,const std::filesystem::path &out,bool show) {
 const auto clock=[] {return SdlWindow::presentationNanoseconds();};
 auto audio=XeenAudio::installed(installation,assets,clock(),false);
 auto services=XeenCutsceneFlow::original(assets,font,*audio,clock,false,XeenSceneArchive::CloudsEnding);
 // Snapshot each actual page using the normal flow before opening the preview.
 auto snapshotServices=services;std::uint64_t now=0;snapshotServices.clock=[&]{return now;};
 XeenCutsceneFlow snapshots(scorePages(data,captions),std::move(snapshotServices));
 for(unsigned page=1;page<=2;++page) {
  picture(snapshots.frame(),out/("score page "+std::to_string(page)+".ppm"));
  snapshots.presented(snapshots.frame().presentation());snapshots.completeInput(snapshots.frame().presentation());
  if(page==2)break;
  snapshots.handle(DialogKeyAction{27},snapshots.frame().presentation());
  bool acquired=false;
  for(unsigned guard=0;guard<10 && !acquired;++guard) {
   if(const auto next=snapshots.nextPresentationDeadline())now=std::max(now,*next);
   if(snapshots.update())acquired=snapshots.segmentName()=="score page 2";
  }
  check(acquired,"Score preview did not acquire its second page");
 }
 if(!show)return;
 XeenCutsceneFlow flow(scorePages(data,captions),std::move(services));
 SdlWindow::FrameUpdateHandler h=[&](const auto &a){return flow.handle(a,flow.frame().presentation());};
 h.withPresentedInput=[&](const auto &a,auto,const auto &p){return flow.handle(a,p);};
 h.protectAllKeys=true;h.ignoreMouse=true;h.cutsceneControlQuit=true;
 h.inputContext=[&](const auto &p){return flow.inputContext(p);};
 h.acceptsFrame=[&](const auto &p){return flow.acceptsFrame(p);};h.acceptsInputFrame=[&](const auto &p){return flow.acceptsInput(p);};
 h.framePresented=[&](const auto &p){flow.presented(p);};h.completeInputHandoff=[&](const auto &p){flow.completeInput(p);};
 h.frameCurrent=[&]{return flow.current();};h.finished=[&]{return flow.done();};
 h.failed=[&]{flow.close();};h.closed=[&]{flow.close();};
 h.nextPresentationDeadline=[&]{return flow.nextPresentationDeadline();};
 SdlWindow().showInteractive(flow.frame(),"MMModern - score pages",h,[]{return true;},[&]{return flow.update();});
}
struct Run {
 std::uint64_t now=17000000;std::shared_ptr<XeenAudio> audio;std::unique_ptr<XeenCutsceneFlow> flow;
 std::vector<XeenCutsceneFlow::Trace> trace;std::vector<XeenCutscene::Audio> requests;
 bool keys=false,slow=false;std::filesystem::path pictures;std::map<std::string,bool> pictured;
 std::uint64_t acknowledge=0;std::string page;
 Run(const GameInstallation &installation,XeenAssetSource &assets,const XeenFontFormat &font,const XeenCutsceneData &data,
     const XeenCutsceneData::Captions &captions,bool inject=false,bool delay=false,std::uint32_t score=71001722):keys(inject),slow(delay) {
  audio=XeenAudio::installed(installation,assets,now,false);audio->observeOptions({});
  auto s=XeenCutsceneFlow::original(assets,font,*audio,[&]{return now;},data.digitalDevice>=2,XeenSceneArchive::CloudsEnding);
  const auto play=s.audio;s.audio=[&,play](const auto &a,auto t){requests.push_back(a);play(a,t);};
  s.trace=[&](const auto &t){trace.push_back(t);};
  flow=std::make_unique<XeenCutsceneFlow>(XeenCutscene::cloudsEnding(data,captions,XeenWallClock::fixed(1782878400),score),std::move(s));acquire();
 }
 void acquire() {
  flow->presented(flow->frame().presentation());flow->completeInput(flow->frame().presentation());
  if(keys && flow->segmentName().find("score page ")!=0 && flow->segmentName().find("scroll")==std::string::npos){flow->handle(DialogKeyAction{32},flow->frame().presentation());flow->handle(CancelInteractionAction{},flow->frame().presentation());
   check(!flow->inputContext(flow->frame().presentation()).dialog->click(80,80),"Animation accepted click");}
  const auto name=flow->segmentName();
  if(!pictures.empty() && (name=="castle initial hold" || name=="ending pyramid" || name=="king voice 1" || name=="score page 1" || name=="score page 2" || name=="parade 0 idle 0") && !pictured[name]) {
   pictured[name]=true;std::ofstream out(pictures/(name+".ppm"),std::ios::binary);out<<"P6\n320 200\n255\n";
   const auto &f=flow->frame();for(auto p:f.pixels)out.write(reinterpret_cast<const char *>(f.palette.data()+3*p),3);
  }
 }
 void finish() {
  for(unsigned guard=0;guard<100000 && !flow->done();++guard) {
   if(flow->segmentName().find("score page ")==0 && flow->inputContext(flow->frame().presentation()).readyForAction) {
    if(page!=flow->segmentName()){page=flow->segmentName();acknowledge=17000000+std::uint64_t(((page=="score page 1"?252.0:255.7)-51.15)*1e9);}
    if(now>=acknowledge)flow->handle(DialogKeyAction{27},flow->frame().presentation());
   }
   auto next=flow->nextPresentationDeadline();check(bool(next),"ending deadline");now=std::max(now,*next);
   const auto &o=*flow->cutscene().observations;
   if(slow && flow->segmentName()=="ending throne" && o.throneOffsets.size()==1 && !o.panStride)now=PresentationClock::after(now,3);
   audio->advance(now);if(flow->update())acquire();
  }
  check(flow->done(),"ending did not finish");
 }
 std::uint64_t first(const char *name,unsigned iteration=0) const {
  const auto i=std::find_if(trace.begin(),trace.end(),[&](const auto &t){return t.event=="flip" && t.segment==name && t.iteration==iteration;});
  check(i!=trace.end(),"Missing ending marker");return i->nanoseconds;
 }
};
}
int main(int argc,char **argv){try {
 check(argc==3 || (argc==4 && (std::string(argv[3])=="--score-pages" || std::string(argv[3])=="--score-pictures")),"ending presentation test arguments");
 const auto installation=*xeenTestInstallationDetector().detect(argv[1]);
 XeenAssetSource assets(installation,320,200);
 const auto data=XeenCutsceneData::installed(installation);
 const auto captions=XeenCutsceneData::captions(assets.readArchiveResource(data.resources[XeenCutsceneData::Caption],XeenSceneArchive::CloudsEnding));
 const XeenFontFormat font(assets.readArchiveResource("fnt",XeenSceneArchive::DarksideOnly));
 const std::filesystem::path out=argv[2];std::filesystem::create_directories(out);
 if(argc==4){pageLayout(data,captions,font);previewPages(installation,assets,font,data,captions,out,std::string(argv[3])=="--score-pages");return 0;}
 frames(assets);
 check(captions.record(8,300).size()==159 && captions.record(10,200).size()==74,"DOS NUL caption indexing");
 for(const auto &r:data.ending.resources)check(!assets.readArchiveResource(r,XeenSceneArchive::CloudsEnding).empty(),r.c_str());
 for(const auto &r:data.ending.powers)check(!assets.readArchiveResource(r,XeenSceneArchive::CloudsEnding).empty(),r.c_str());
 for(const auto &r:data.ending.parade){check(assets.spriteFrameCount(r.idle,XeenSceneArchive::CloudsEnding)>0,"parade idle");check(assets.spriteFrameCount(r.attack,XeenSceneArchive::CloudsEnding)>0,"parade attack");check(!assets.readArchiveResource(r.voice,XeenSceneArchive::CloudsEnding).empty(),"parade voice");}
 randomSource();kinds();cueSites(data,captions);delayedPublication(data,captions);pageLayout(data,captions,font);
 Run run(installation,assets,font,data,captions);run.pictures=out;run.finish();
 std::ofstream trace(out/"trace.tsv");for(const auto &t:run.trace)trace<<t.event<<'\t'<<t.segment<<'\t'<<t.iteration<<'\t'<<t.nanoseconds<<'\t'<<t.tick<<'\t'<<t.reset<<'\n';
 std::ofstream timing(out/"timing.txt");
 timing<<"First castle is bracketed by 51.100 (black) and 51.200 (visible) in ENDING.avi; 49.553 is the DAT scroll, not the castle.\n";
 timing<<"Zero load latency; 70 Hz publication; real VOC durations. Values aligned to castle midpoint 51.150 s.\n";
 struct Marker {const char *name;unsigned iteration;double video;};
 for(const auto &m:std::vector<Marker>{{"ending pyramid",0,59.341},{"castle destruction 0",6,62.965},
     {"castle destruction 5",7,66.261},{"ending Later",0,82.7},{"ending throne",0,89.25},{"king scale 0",0,94.969}}) {
  const auto actual=51.150+double(run.first(m.name,m.iteration)-run.first("ending castle"))/1e9;
  timing<<m.name<<'['<<m.iteration<<"] video="<<m.video<<" model="<<actual<<" delta="<<actual-m.video<<'\n';
  if(std::abs(actual-m.video)>0.5)throw std::runtime_error(std::string("Ending timing: ")+m.name+" actual="+std::to_string(actual));
 }
 const auto &o=*run.flow->cutscene().observations;
 for(const char *name:{"Alamar voice 1","king voice 1"}) {
  const auto composed=std::find_if(run.trace.begin(),run.trace.end(),[&](const auto &t){return t.event=="compose" && t.segment==name;});
  check(composed!=run.trace.end() && run.first(name)-composed->nanoseconds<20000000,"Speech waited before its first flip");
  const auto next=std::find_if(run.trace.begin(),run.trace.end(),[&](const auto &t){return t.event=="compose" && t.segment==name && t.iteration==1;});
  check(next!=run.trace.end() && next->tick-composed->tick>=3,"Speech advanced before its three-tick hold");
 }
 check(o.panStride==1 && o.throneOffsets.size()==320,"Measured throne pan");
 check(o.paradeSelectors.size()==73,"Parade omitted a DOS selector");
 check(std::count_if(run.trace.begin(),run.trace.end(),[](const auto &t){return t.event=="flip" && t.segment.find("score heading")==0;})>20,"Score heading did not redraw the rotating sky inside its ticks");
 for(unsigned n=0;n<73;++n)check(o.paradeSelectors[n]==data.ending.parade[n].selector,"Parade selector order");
 check(o.settledDigits==std::vector<unsigned>({9,8,7,6,5,4,3,2,1,0}),"Rightmost digit did not settle first");
 const auto scoreCheck=[&](const auto &obs,const std::string &score) {
  check(obs.scoreFrames.size()==40,"Score frame counts");
  for(unsigned i=0;i<40;++i)for(unsigned n=0;n<10;++n) {
   const auto value=obs.scoreFrames[i][n];check(value>=20 && value<=29,"Random digit range");
   if(i>=30 && n>=39-i)check(value==20+unsigned(score[n]-'0'),"Settled digit did not come from outcome");
  }
 };scoreCheck(o,"0071001722");
 check(!o.kingMouths.empty() && std::all_of(o.kingMouths.begin(),o.kingMouths.end(),[](auto n){return n>=1 && n<=6;}),"King mouth range");
 std::vector<unsigned> mixers;for(const auto &r:run.requests)if(r.kind==XeenCutscene::Audio::Mixer)mixers.push_back(r.value);
 check(mixers==std::vector<unsigned>({75,95,95}),"Mixer sites and cleanup");
 Run input(installation,assets,font,data,captions,true);input.finish();
 check(input.now==run.now && input.flow->cutscene().observations->kingMouths==o.kingMouths,"Animation input changed playback or RNG");
 Run replay(installation,assets,font,data,captions,false,false,1234567890);replay.finish();scoreCheck(*replay.flow->cutscene().observations,"1234567890");
 check(replay.flow->cutscene().observations->kingMouths==o.kingMouths,"Final score changed presentation random draws");
 Run slow(installation,assets,font,data,captions,false,true);slow.finish();check(slow.flow->cutscene().observations->panStride==3,"Throne stride was fixed");
 auto silent=data;silent.digitalDevice=0;Run captionRun(installation,assets,font,silent,captions);captionRun.finish();
 const auto &co=*captionRun.flow->cutscene().observations;
 check(co.captionRecords==std::vector<unsigned>({8,10,10}),"DOS ending caption mismatch not preserved");
 check(std::all_of(co.captionWidths.begin(),co.captionWidths.end(),[](auto n){return n<=40;}),"Caption strip wider than forty characters");
 check(co.captionAdvances.size()==captions.record(8,300).size()+2*captions.record(10,200).size()-3,"Caption did not reach both terminators");
 check(captionRun.audio->digitalBusy(captionRun.now)==false,"No-device run started digital sound");
 auto threshold=XeenCutscene::cloudsEnding(silent,captions,XeenWallClock::fixed(0),71001722);
 const auto voice=std::find_if(threshold.segments.begin(),threshold.segments.end(),[](const auto &s){return s.name=="king voice 1";});
 voice->entering({0,100,0,false});voice->compose({0,101,1,false});
 check(threshold.observations->captionAdvances.empty(),"Caption advanced before two ticks");
 voice->compose({1,102,2,false});voice->compose({2,103,3,false});voice->compose({3,104,4,false});
 check(threshold.observations->captionAdvances==std::vector<std::uint64_t>({102,104}),"Caption threshold/reset changed");
 for(const auto &m:std::vector<Marker>{{"mirror room",0,103.0},{"mirror sky saved",0,107.938},{"parade closing scroll",0,240.860},
   {"score heading",0,243.157},{"score closing scroll",0,255.927},{"ending closing scroll",0,261.963},{"ending shutdown",0,263.076}}) {
  const auto actual=51.150+double(run.first(m.name,m.iteration)-run.first("ending castle"))/1e9;
  timing<<m.name<<" video="<<m.video<<" model="<<actual<<" delta="<<actual-m.video<<'\n';
  if(std::abs(actual-m.video)>0.5)throw std::runtime_error(std::string("Ending timing: ")+m.name+" actual="+std::to_string(actual));
 }
 std::cout<<"Ending fake clock finished at "<<double(run.now)/1e9<<" s; acknowledgements at video 252.0 and 255.7 s\n";
 std::cout<<"61 compressed frame hashes and bounded EOF checks passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
