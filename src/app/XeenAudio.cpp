#include "app/XeenAudio.h"
#include "app/XeenFmSource.h"
#include "platform/sdl/SdlAudioDevice.h"
#include "platform/sdl/SdlAudioStream.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenDosText.h"
#include "formats/xeen/XeenVoc.h"
#include "games/xeen/XeenMap.h"
#include "core/PresentationClock.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <mutex>
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
 XeenSoundOptions options;
 XeenAudio::EffectSink effectSink;
 XeenAudio::DigitalSink digitalSink;
 std::array<std::string,6> eventVoices;
 std::array<unsigned,35> weaponEffects{};
 unsigned id;Loader loader;std::uint64_t elapsed=0,last=0,speechEnd=0;
 std::vector<std::string> maps,locations;std::array<unsigned,6> kinds{};
 std::unique_ptr<PromusDriver> clockDriver;
 std::shared_ptr<XeenFmSource> output;bool retainOutput;
 std::string current,loaded;std::optional<std::string> mapName;
 PromusDriver::Bytes buffer;
 struct Song {std::string name;XeenSceneArchive archive;unsigned fade;};
 std::optional<Song> pending,afterTitle;bool titleExit=false;
 std::uint64_t sample=0,phase=0;
 std::uint64_t digitalEnd=0; // Elapsed presentation time, survives clock rebases.
 // Producer publishes only the latest digital command. Each request owns its
 // decoded PCM; the consumer retains its own reference until stop/replacement.
 std::mutex digitalMutex;bool digitalChanged=false;
 std::shared_ptr<const XeenVocSample> digitalRequest;
 std::shared_ptr<const XeenVocSample> digitalVoice;
 std::uint64_t digitalPhase=0;
 std::unique_ptr<SdlAudioStream> fmStream;
 int mixRate=0;
 // Last member: close/join the callback before its driver, chip and queue die.
 std::unique_ptr<SdlAudioDevice> device;
 Impl(unsigned i,PromusDriver::Bytes resource,Loader l,std::vector<std::uint8_t> module,std::uint64_t n,bool deviceOutput,PromusDriver::Sink trace,std::shared_ptr<XeenFmSource> source):id(i),loader(std::move(l)),last(n),output(source?source:std::make_shared<XeenFmSource>()),retainOutput(bool(source)) {
  const XeenDosText text(module);maps=text.table("CLOUDS_MUSIC");locations=text.table("CLOUDS_LOCATION_MUSIC");
  for(unsigned w=0;w<weaponEffects.size();++w)weaponEffects[w]=60+module.at(0x4d660+w);
  // DAT 0x508cc: six far pointers into the original data segment.
  for(unsigned i=0;i<eventVoices.size();++i) {
   const auto at=0x508cc+i*4;
   if(module.at(at+2)!=0x1d || module.at(at+3)!=0x48)throw std::runtime_error("Invalid Event voice table segment");
   auto p=0x4d890+unsigned(module.at(at))+(unsigned(module.at(at+1))<<8);
   for(unsigned n=0;module.at(p);++n,++p) {
    if(n>=32)throw std::runtime_error("Invalid Event voice table name");
    eventVoices[i]+=char(module.at(p));
   }
  }
  for(unsigned k=0;k<6;++k){kinds[k]=module.at(0x4f10a+k);if(kinds[k]>=maps.size())throw std::runtime_error("Invalid Clouds music wall-kind table");}
  if(id==3) {
   clockDriver=std::make_unique<PromusDriver>(resource,std::move(trace));
  }else if(id>=1 && id<=9)std::cerr<<"Music device "<<id<<": not supported yet\n";
  output->initialize(id==3?resource:PromusDriver::Bytes{});
  device=std::make_unique<SdlAudioDevice>([this](auto *out,auto frames,int rate){render(out,frames,rate);},deviceOutput,
   [this](int rate){return prepareRender(rate);});
  retainOutput=retainOutput||device->active();
 }
 std::uint64_t logicalNow(std::uint64_t now) const {
  if(now<last) {
   if(last-now>elapsed)throw std::underflow_error("Audio action predates presentation clock origin");
   return elapsed-(last-now);
  }
  return PresentationClock::checkedAdd(elapsed,now-last);
 }
 std::uint64_t digitalTime(std::uint64_t now) {
  const auto time=logicalNow(now);
  if(now>last)advanceClock(now);
  return time;
 }
 void publishDigital(std::shared_ptr<const XeenVocSample> pcm) {
  std::shared_ptr<const XeenVocSample> retired;
  {
   std::lock_guard<std::mutex> lock(digitalMutex);
   retired=std::move(digitalRequest);digitalRequest=std::move(pcm);digitalChanged=true;
  }
 }
 bool prepareRender(int rate) {
  if(rate<=0)return false;
  if(mixRate)return mixRate==rate;
  auto stream=std::make_unique<SdlAudioStream>(49716,rate);
  if(!stream->valid())return false;
  fmStream=std::move(stream);mixRate=rate;return true;
 }
 void render(std::int16_t *stereo,std::size_t frames,int rate) {
  if(!fmStream || rate!=mixRate)throw std::logic_error("Audio mix rate was not prepared");
  // Bound request latency within a large callback as well as between callbacks.
  // Only FM uses XeenFmSource's queue/barrier; digital never enters that queue.
  for(std::size_t offset=0;offset<frames;) {
   const auto count=std::min<std::size_t>(128,frames-offset);
   auto *out=stereo+offset*2;
   fmStream->pull([this](auto *pcm,auto count){output->pull(pcm,count);},out,count);
   std::shared_ptr<const XeenVocSample> retired;
   {
    std::lock_guard<std::mutex> lock(digitalMutex);
    if(digitalChanged){retired=std::move(digitalVoice);digitalVoice=std::move(digitalRequest);digitalChanged=false;digitalPhase=0;}
   }
   retired.reset(); // Release potentially large PCM only after unlocking.
   // Exact rational zero-order hold at 1000000/68 Hz. Do not round the source
   // rate to an integer Hz or prebuffer converted VOC: stop/replacement must
   // discard the previous voice at the next mix block, including its tail.
   const auto denominator=68ULL*rate;
   for(std::size_t f=0;f<count;++f) {
    int digital=0;
    if(digitalVoice) {
     const auto index=digitalPhase/denominator;
     if(index<digitalVoice->pcm.size()) {
      digital=(int(digitalVoice->pcm[index])-128)*256;digitalPhase+=1000000;
     }else digitalVoice.reset();
    }
    // Part B adds the independently resampled CD stream here, before clipping.
    for(unsigned channel=0;channel<2;++channel)
     out[f*2+channel]=std::clamp(int(out[f*2+channel])+digital,-32768,32767);
   }
   offset+=count;
  }
 }
 void command(unsigned command) {if(clockDriver)clockDriver->command(command);if(retainOutput)output->command(command);}
 void loadBuffer(bool play) {if(clockDriver)clockDriver->load(buffer,play);if(retainOutput)output->load(buffer,play);}
 PromusDriver::Bytes read(const Song &song) {
  auto data=bytes(loader(song.name,song.archive));if(data->empty())throw std::runtime_error("Missing or empty original song: "+song.name);
  return data;
 }
 void load(const Song &song,bool play=true,PromusDriver::Bytes data={}) {
  buffer=data?std::move(data):read(song);loaded=song.name;
  loadBuffer(play);
 }
 void begin(Song song) {
  const auto name=lower(song.name);if(name==current)return;
  current=name;
  if(titleExit){afterTitle=std::move(song);return;}
  command(song.fade);pending=std::move(song);
 }
 void advanceClock(std::uint64_t now) {
  if(now<last)throw std::logic_error("Audio presentation clock moved backwards");
  elapsed=logicalNow(now);last=now;
  const auto target=nativeSample(elapsed),threshold=PromusDriver::rate*PromusDriver::divisor;
  while(sample<target) {
   const auto count=std::min(target-sample,(threshold-phase+PromusDriver::pit-1)/PromusDriver::pit);
   sample+=count;phase+=count*PromusDriver::pit;
   if(phase>=threshold){phase-=threshold;if(clockDriver)clockDriver->tick();}
  }
 }
 void advance(std::uint64_t now,std::uint64_t cdEnd) {
  advanceClock(now);speechEnd=cdEnd;device->check();output->check();
  if(pending && (!clockDriver || !clockDriver->fadeSpeed()) && now>=speechEnd) {
   // Read first: a missing song stays pending and fails every later advance,
   // so the main loop reports it even when an effect's advance swallowed it.
   auto data=read(*pending);
   if(retainOutput)output->fadeBarrier();
   auto song=std::move(*pending);pending.reset();
   if(titleExit) {
    if(options.music)command(0);load(song,false,std::move(data));titleExit=false;
    if(afterTitle){auto next=std::move(*afterTitle);afterTitle.reset();command(next.fade);pending=std::move(next);}
   }else load(song,options.music,std::move(data));
  }
 }
};
XeenAudio::XeenAudio(unsigned id,PromusDriver::Bytes driver,Loader l,std::vector<std::uint8_t> module,std::uint64_t now,bool device,PromusDriver::Sink trace,std::shared_ptr<XeenFmSource> output):
 _impl(new Impl(id,std::move(driver),std::move(l),std::move(module),now,device,std::move(trace),std::move(output))){}
XeenAudio::~XeenAudio()=default;
std::shared_ptr<XeenAudio> XeenAudio::installed(const GameInstallation &installation,XeenAssetSource &,std::uint64_t now,bool device) {
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
 return std::make_shared<XeenAudio>(id,resource,std::move(loader),read(*installation.uiModule),now,device);
}
unsigned XeenAudio::deviceId() const{return _impl->id;}
bool XeenAudio::outputActive() const{return _impl->device->active();}
std::string XeenAudio::mapSong(const XeenMapGeometry &m) const {return _impl->maps.at(m.isOutdoors()?0:_impl->kinds.at(m.wallKind));}
std::string XeenAudio::locationSong(unsigned action) const{return _impl->locations.at(action);}
void XeenAudio::requestSong(std::string name,XeenSceneArchive archive,unsigned fade,std::uint64_t now) {advance(now,_impl->speechEnd);_impl->begin({std::move(name),archive,fade});}
void XeenAudio::requestMap(const XeenMapGeometry &map,std::uint64_t now) {const auto name=mapSong(map);_impl->mapName=name;requestSong(name,XeenSceneArchive::Clouds,207,now);}
void XeenAudio::requestLocation(unsigned action,std::uint64_t now) {requestSong(locationSong(action),XeenSceneArchive::Clouds,223,now);}
void XeenAudio::requestInn(std::uint64_t now) {requestSong("inn.m",XeenSceneArchive::Clouds,128,now);}
void XeenAudio::enterTitle(std::uint64_t now) {advance(now);_impl->pending.reset();_impl->afterTitle.reset();_impl->titleExit=false;_impl->load({"newbrigh.m",XeenSceneArchive::DarksideOnly,0},_impl->options.music);}
void XeenAudio::leaveTitle(std::uint64_t now) {advance(now);if(_impl->options.music)_impl->command(207);_impl->titleExit=true;_impl->pending=Impl::Song{"twnnitea.m",XeenSceneArchive::DarksideOnly,207};}
bool XeenAudio::titleTeardownReady(std::uint64_t now) {
 advance(now,_impl->speechEnd);
 return !digitalBusy(now); // DAT 0xb933..0xb947, before releasing session resources.
}
void XeenAudio::effect(unsigned id,std::uint64_t now) {
 // Use the action's existing clock sample, without querying a gameplay clock.
 advance(std::max(now,_impl->last),_impl->speechEnd);
 if(!_impl->options.efx)return;
 if(_impl->effectSink)try {_impl->effectSink(id);}catch(...) {}
 if(_impl->clockDriver)_impl->clockDriver->effect(id);
 if(_impl->retainOutput)_impl->output->effect(id);
}
void XeenAudio::effectSink(EffectSink sink) {_impl->effectSink=std::move(sink);}
void XeenAudio::weaponEffect(unsigned weapon,std::uint64_t now) {
 const auto fx=_impl->weaponEffects.at(weapon);
 stopDigital(now); // DAT 0x2c0b2, before FM and VOC, before HP publication.
 effect(fx,now);
 startDigital("pow"+std::to_string(fx-60)+".voc",XeenSceneArchive::Clouds,now);
}
void XeenAudio::missEffect(std::uint64_t now) {
 startDigital("miss.voc",XeenSceneArchive::Clouds,now);effect(6,now);
}
void XeenAudio::eventVoice(unsigned index,std::uint64_t now) {
 // Part 0's resource inventory: the final table member is DARK's laff1;
 // null.voc and the other Clouds members come from XEEN.
 startDigital(_impl->eventVoices.at(index),index==5?XeenSceneArchive::Darkside:XeenSceneArchive::Clouds,now);
}
void XeenAudio::startDigital(const std::string &name,XeenSceneArchive archive,std::uint64_t now) {
 auto &s=*_impl;
 if(!s.options.efx)return;
 const auto actionTime=s.digitalTime(now);
 auto pcm=std::make_shared<const XeenVocSample>(decodeXeenVoc(s.loader(name,archive)));
 const auto end=PresentationClock::checkedAdd(actionTime,pcm->durationNanoseconds());
 s.digitalEnd=end;s.publishDigital(std::move(pcm));
 if(s.digitalSink)try {s.digitalSink(name,archive);}catch(...) {}
}
void XeenAudio::stopDigital(std::uint64_t now) {
 auto &s=*_impl;
 if(!s.options.efx)return;
 s.digitalTime(now);s.digitalEnd=0;s.publishDigital({});
}
bool XeenAudio::digitalBusy(std::uint64_t now) const {
 const auto &s=*_impl;return s.options.efx && s.logicalNow(now)<s.digitalEnd;
}
std::optional<std::uint64_t> XeenAudio::digitalDeadline(std::uint64_t now) const {
 if(!digitalBusy(now))return {};
 return PresentationClock::checkedAdd(now,_impl->digitalEnd-_impl->logicalNow(now));
}
void XeenAudio::digitalSink(DigitalSink sink) {_impl->digitalSink=std::move(sink);}
void XeenAudio::render(std::int16_t *out,std::size_t frames,int rate) {_impl->render(out,frames,rate);}
bool XeenAudio::prepareRender(int rate) {return _impl->prepareRender(rate);}
void XeenAudio::observeOptions(XeenSoundOptions value) {_impl->options=value;}
void XeenAudio::toggleMusic(bool on,std::uint64_t now) {
 advance(now,_impl->speechEnd);_impl->options.music=on;
 if(on) {
  _impl->pending.reset();_impl->afterTitle.reset();_impl->titleExit=false;
  if(_impl->buffer)_impl->loadBuffer(true);
 }else _impl->command(0);
}
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
std::uint64_t XeenAudio::samples() const{return _impl->sample;}
bool XeenAudio::pending() const{return bool(_impl->pending);}
}
