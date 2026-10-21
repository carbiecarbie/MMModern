#include "app/XeenAudio.h"
#include "app/XeenFmSource.h"
#include "platform/sdl/SdlAudioDevice.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenDosText.h"
#include "games/xeen/XeenMap.h"
#include "core/PresentationClock.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace mmodern {
namespace {
PromusDriver::Bytes bytes(std::vector<std::uint8_t> b){return std::make_shared<const std::vector<std::uint8_t>>(std::move(b));}
std::string lower(std::string s){for(auto &c:s)if(c>='A'&&c<='Z')c+='a'-'A';return s;}
std::uint64_t nativeSample(std::uint64_t ns) {return (ns/1000000000)*49716+((ns%1000000000)*49716+999999999)/1000000000;}
std::vector<std::uint8_t> read(const ReadOnlyDataFile &file) {
 auto in=file.open();if(in->size()>1024*1024)throw std::runtime_error("Audio resource exceeds its bound");
 std::vector<std::uint8_t> result(in->size());if(in->read(result.data(),result.size())!=result.size())throw std::runtime_error("Incomplete audio resource read");return result;
}
}
struct XeenAudio::Impl {
 unsigned id;Loader loader;std::uint64_t elapsed=0,last=0,speechEnd=0;
 std::vector<std::string> maps,locations;std::array<unsigned,6> kinds{};
 std::unique_ptr<PromusDriver> clockDriver;
 std::shared_ptr<XeenFmSource> output;bool retainOutput;
 std::vector<XeenFmSource::Write> writes;
 std::string current,loaded;std::optional<std::string> mapName;
 PromusDriver::Bytes buffer;
 struct Song {std::string name;XeenSceneArchive archive;unsigned fade;};
 std::optional<Song> pending,afterTitle;bool titleExit=false;
 std::uint64_t sample=0,phase=0;
 // Last member: close/join the callback before its driver, chip and queue die.
 std::unique_ptr<SdlAudioDevice> device;
 Impl(unsigned i,PromusDriver::Bytes resource,Loader l,std::vector<std::uint8_t> module,std::uint64_t n,bool deviceOutput,PromusDriver::Sink trace,std::shared_ptr<XeenFmSource> source):id(i),loader(std::move(l)),last(n),output(source?source:std::make_shared<XeenFmSource>()),retainOutput(bool(source)) {
  const XeenDosText text(module);maps=text.table("CLOUDS_MUSIC");locations=text.table("CLOUDS_LOCATION_MUSIC");
  for(unsigned k=0;k<6;++k){kinds[k]=module.at(0x4f10a+k);if(kinds[k]>=maps.size())throw std::runtime_error("Invalid Clouds music wall-kind table");}
  if(id==3) {
   clockDriver=std::make_unique<PromusDriver>(resource,[this,trace=std::move(trace)](const auto &w){
    writes.push_back({w.interrupt?PromusDriver::sampleBoundary(w.tick):nativeSample(elapsed),w});
    if(trace)trace(w);
   });
  }else if(id>=1 && id<=9)std::cerr<<"Music device "<<id<<": not supported yet\n";
  device=std::make_unique<SdlAudioDevice>([this](auto *out,auto frames){output->pull(out,frames);},deviceOutput);
  retainOutput=retainOutput||device->active();flush();
 }
 void flush() {
  if(retainOutput)output->publish(sample,std::move(writes));
  writes.clear();
 }
 void command(unsigned command) {if(clockDriver)clockDriver->command(command);}
 void load(const Song &song,bool play=true) {
  buffer=bytes(loader(song.name,song.archive));if(buffer->empty())throw std::runtime_error("Missing or empty original song: "+song.name);
  loaded=song.name;
  if(clockDriver)clockDriver->load(buffer,play);
 }
 void begin(Song song) {
  const auto name=lower(song.name);if(name==current)return;
  current=name;
  if(titleExit){afterTitle=std::move(song);return;}
  command(song.fade);pending=std::move(song);
 }
 void advance(std::uint64_t now,std::uint64_t cdEnd) {
  if(now<last)throw std::logic_error("Audio presentation clock moved backwards");
  elapsed=PresentationClock::checkedAdd(elapsed,now-last);last=now;speechEnd=cdEnd;device->check();
  const auto target=nativeSample(elapsed),threshold=PromusDriver::rate*PromusDriver::divisor;
  while(sample<target) {
   const auto count=std::min(target-sample,(threshold-phase+PromusDriver::pit-1)/PromusDriver::pit);
   sample+=count;phase+=count*PromusDriver::pit;
   if(phase>=threshold){phase-=threshold;if(clockDriver)clockDriver->tick();}
  }
  if(pending && (!clockDriver || !clockDriver->fadeSpeed()) && now>=speechEnd) {
   auto song=std::move(*pending);pending.reset();
   if(titleExit) {
    command(0);load(song,false);titleExit=false;
    if(afterTitle){auto next=std::move(*afterTitle);afterTitle.reset();command(next.fade);pending=std::move(next);}
   }else load(song);
  }
  flush();
 }
};
XeenAudio::XeenAudio(unsigned id,PromusDriver::Bytes driver,Loader l,std::vector<std::uint8_t> module,std::uint64_t now,bool device,PromusDriver::Sink trace,std::shared_ptr<XeenFmSource> output):
 _impl(new Impl(id,std::move(driver),std::move(l),std::move(module),now,device,std::move(trace),std::move(output))){}
XeenAudio::~XeenAudio()=default;
std::shared_ptr<XeenAudio> XeenAudio::installed(const GameInstallation &installation,XeenAssetSource &assets,std::uint64_t now,bool device) {
 if(!installation.configuration)throw std::runtime_error("Missing installed WORLD/XEEN.CFG");
 auto cfg=read(*installation.configuration);if(cfg.size()<6)throw std::runtime_error("Truncated WORLD/XEEN.CFG");
 const unsigned id=cfg[4]|unsigned(cfg[5])<<8;
 const auto folder=installation.configuration->physicalPath.parent_path();
 // Retain a separate resource owner for program lifetime; never borrow a session.
 auto source=std::make_shared<XeenAssetSource>(installation);
 Loader loader=[source,folder](const std::string &name,XeenSceneArchive archive) {
  std::optional<std::filesystem::path> loose;
  for(const auto &entry:std::filesystem::directory_iterator(folder))if(lower(entry.path().filename().u8string())==lower(name)) {
   if(loose || !entry.is_regular_file())throw std::runtime_error("Ambiguous loose audio resource: "+name);loose=entry.path();
  }
  if(loose)return read(ReadOnlyDataFile::plain(*loose));
  try{return source->readArchiveResource(name,archive);}catch(const std::exception &e){throw std::runtime_error("Original audio resource "+name+": "+e.what());}
 };
 auto resource=id==3?bytes(loader("promus",XeenSceneArchive::IntroOnly)):PromusDriver::Bytes{};
 (void)assets;
 return std::make_shared<XeenAudio>(id,resource,std::move(loader),read(*installation.uiModule),now,device);
}
unsigned XeenAudio::deviceId() const{return _impl->id;}
bool XeenAudio::outputActive() const{return _impl->device->active();}
std::string XeenAudio::mapSong(const XeenMapGeometry &m) const {return _impl->maps.at(m.isOutdoors()?0:_impl->kinds.at(m.wallKind));}
std::string XeenAudio::locationSong(unsigned action) const{return _impl->locations.at(action);}
void XeenAudio::requestSong(std::string name,XeenSceneArchive archive,unsigned fade,std::uint64_t now) {advance(now,_impl->speechEnd);_impl->begin({std::move(name),archive,fade});_impl->flush();}
void XeenAudio::requestMap(const XeenMapGeometry &map,std::uint64_t now) {const auto name=mapSong(map);_impl->mapName=name;requestSong(name,XeenSceneArchive::Clouds,207,now);}
void XeenAudio::requestLocation(unsigned action,std::uint64_t now) {requestSong(locationSong(action),XeenSceneArchive::Clouds,223,now);}
void XeenAudio::requestInn(std::uint64_t now) {requestSong("inn.m",XeenSceneArchive::Clouds,128,now);}
void XeenAudio::enterTitle(std::uint64_t now) {advance(now);_impl->pending.reset();_impl->afterTitle.reset();_impl->titleExit=false;_impl->load({"newbrigh.m",XeenSceneArchive::DarksideOnly,0});_impl->flush();}
void XeenAudio::leaveTitle(std::uint64_t now) {advance(now);_impl->command(207);_impl->titleExit=true;_impl->pending=Impl::Song{"twnnitea.m",XeenSceneArchive::DarksideOnly,207};_impl->flush();}
void XeenAudio::effect(unsigned id,std::uint64_t now) {advance(now,_impl->speechEnd);if(_impl->clockDriver)_impl->clockDriver->effect(id);_impl->flush();}
void XeenAudio::advance(std::uint64_t now,std::uint64_t cdEnd) {_impl->advance(now,cdEnd);}
void XeenAudio::rebaseClock(std::uint64_t now) noexcept {_impl->last=now;_impl->speechEnd=0;}
std::optional<std::uint64_t> XeenAudio::deadline() const {
 if(!_impl->pending)return {};
 const auto tick=_impl->clockDriver?_impl->clockDriver->ticks()+1:PromusDriver::clockTicks(_impl->elapsed)+1;
 return std::max(PresentationClock::checkedAdd(_impl->last,PromusDriver::clockBoundary(tick)-_impl->elapsed),_impl->speechEnd);
}
const std::string &XeenAudio::currentName() const{return _impl->current;}
const std::string &XeenAudio::loadedName() const{return _impl->loaded;}
std::uint64_t XeenAudio::ticks() const{return _impl->clockDriver?_impl->clockDriver->ticks():PromusDriver::clockTicks(_impl->elapsed);}
bool XeenAudio::pending() const{return bool(_impl->pending);}
}
