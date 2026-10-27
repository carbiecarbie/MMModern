#include "app/XeenAudio.h"
#include "app/XeenAudioMix.h"
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
#include <thread>
#include <condition_variable>
#include <chrono>
namespace mmodern {
namespace {
PromusDriver::Bytes bytes(std::vector<std::uint8_t> b){return std::make_shared<const std::vector<std::uint8_t>>(std::move(b));}
std::string lower(std::string s){for(auto &c:s)if(c>='A'&&c<='Z')c+='a'-'A';return s;}
std::uint64_t nativeSample(std::uint64_t ns) {return (ns/1000000000)*49716+((ns%1000000000)*49716+999999999)/1000000000;}
XeenAudioConfig legacyConfig(unsigned id) {XeenAudioConfig result;result.music=std::uint16_t(id);return result;}
XeenCardMixer::Model cardModel(const XeenAudioConfig &c) {
 if(c.music==3)return XeenCardMixer::Model::Sb16;
 if(c.digital==6)return XeenCardMixer::Model::Sb2;
 if(c.digital==7)return XeenCardMixer::Model::SbPro;
 if(c.digital==10)return XeenCardMixer::Model::Sb16;
 return XeenCardMixer::Model::None;
}
std::uint16_t mixerAddress(const XeenAudioConfig &c) {return c.music==3?c.musicPort:c.digitalPort;}
std::uint16_t mixerWriter(const XeenAudioConfig &c) {
 return c.music==3 || c.music==0 || c.music>=10 || c.deferredMusic()?c.musicPort:c.digitalPort;
}
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
 unsigned id;XeenAudioConfig config;Loader loader;std::uint64_t elapsed=0,last=0;
 std::vector<std::string> maps,locations;std::array<unsigned,6> kinds{};
 std::unique_ptr<PromusDriver> clockDriver;
 std::shared_ptr<XeenFmSource> output;bool retainOutput;
 std::string current,loaded;std::optional<std::string> mapName;
 PromusDriver::Bytes buffer;
 struct Song {std::string name;XeenSceneArchive archive;unsigned fade;};
 std::optional<Song> pending,afterTitle;bool titleExit=false;
 std::uint64_t sample=0,phase=0;
 std::uint64_t digitalEnd=0; // Elapsed presentation time, survives clock rebases.
 bool digitalFailure=false,digitalNotice=false;
 std::uint64_t cdEnd=0;
 unsigned cdDisc=1;
 std::optional<XeenEventCdSpeech> cdCue;
 std::vector<XeenCdExtent> cdTracks;
 std::string cdNotice,outputNotice;
 bool requiresDevice=false;
 std::mutex cdMutex;bool cdChanged=false;
 std::condition_variable cdWake;
 struct CdJob {std::uint64_t generation;XeenCdRequest request;};
 std::optional<CdJob> cdJob;
 std::uint64_t cdGeneration=0;
 bool cdClosing=false,cdDecoding=false;
 CdDecoder cdDecoder;
 std::thread cdWorker;
 std::shared_ptr<const std::vector<std::int16_t>> cdRequest,cdVoice;
 std::size_t cdCursor=0;
 // Producer publishes only the latest digital command. Each request owns its
 // decoded PCM; the consumer retains its own reference until stop/replacement.
 std::mutex digitalMutex;bool digitalChanged=false;
 std::shared_ptr<const XeenVocSample> digitalRequest;
 std::shared_ptr<const XeenVocSample> digitalVoice;
 std::uint64_t digitalPhase=0;
 // Card mixer: main-thread register state, gains published per mix block.
 XeenCardMixer card;bool mixerFlag=true;
 std::mutex mixerMutex;bool mixerChanged=false;
 std::array<float,6> mixerRequest=XeenCardMixer{}.gains(),mixerGains=mixerRequest;
 std::unique_ptr<SdlAudioStream> fmStream;
 std::unique_ptr<SdlAudioStream> cdStream;
 int mixRate=0;
 // Last member: close/join the callback before its driver, chip and queue die.
 std::unique_ptr<SdlAudioDevice> device;
 Impl(XeenAudioConfig c,PromusDriver::Bytes resource,Loader l,std::vector<std::uint8_t> module,std::uint64_t n,bool deviceOutput,PromusDriver::Sink trace,std::shared_ptr<XeenFmSource> source,CdDecoder decoder):id(c.music),config(c),loader(std::move(l)),last(n),output(source?source:std::make_shared<XeenFmSource>()),retainOutput(bool(source)),cdDecoder(std::move(decoder)),card(cardModel(c),mixerAddress(c)),mixerFlag(c.mixer!=0) {
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
  if(config.deferredDigital()){std::cerr<<"Digital device "<<config.digital<<": not supported yet\n";digitalNotice=true;}
  output->initialize(id==3?resource:PromusDriver::Bytes{});
  startProgram();
  device=std::make_unique<SdlAudioDevice>([this](auto *out,auto frames,int rate){render(out,frames,rate);},deviceOutput,
   [this](int rate){return prepareRender(rate);},[this]{removed();});
  requiresDevice=deviceOutput;
  retainOutput=retainOutput||device->active();
  cdWorker=std::thread([this]{decodeCd();});
 }
 ~Impl() {
  device.reset(); // Join the callback before releasing any consumer references.
  {std::lock_guard<std::mutex> lock(cdMutex);cdClosing=true;}
  cdWake.notify_one();
  if(cdWorker.joinable())cdWorker.join();
 }
 void decodeCd() {
  // This extra owner prevents the callback from freeing a large buffer on
  // replacement or EOF. Reclaim unreferenced PCM only on this worker.
  std::vector<std::shared_ptr<const std::vector<std::int16_t>>> owners;
  for(;;) {
   std::optional<CdJob> job;
   {
    std::unique_lock<std::mutex> lock(cdMutex);
    cdWake.wait_for(lock,std::chrono::milliseconds(20),[&]{return cdClosing || cdJob.has_value();});
    if(cdClosing)return;
    job=std::move(cdJob);cdJob.reset();
   }
   owners.erase(std::remove_if(owners.begin(),owners.end(),[](const auto &p){return p.use_count()==1;}),owners.end());
   if(!job)continue;
   std::shared_ptr<const std::vector<std::int16_t>> pcm;
   std::string error;
   try {
    pcm=std::make_shared<const std::vector<std::int16_t>>(cdDecoder(job->request));
    if(pcm->empty())throw std::runtime_error("Empty CD cue");
   } catch(const std::exception &e) {error=std::string("CD speech unavailable: ")+e.what();}
   std::shared_ptr<const std::vector<std::int16_t>> retired;
   {
    std::lock_guard<std::mutex> lock(cdMutex);
    if(cdClosing || job->generation!=cdGeneration)continue;
    cdDecoding=false;cdNotice=std::move(error);
    if(cdNotice.empty()) {
     owners.push_back(pcm);
     retired=std::move(cdRequest);cdRequest=std::move(pcm);cdChanged=true;
    }
   }
  }
 }
 void cancelCd() {
  std::shared_ptr<const std::vector<std::int16_t>> retired;
  {
   std::lock_guard<std::mutex> lock(cdMutex);
   ++cdGeneration;cdJob.reset();cdDecoding=false;cdNotice.clear();
   retired=std::move(cdRequest);cdChanged=true;
  }
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
 void publishMixer() {
  const auto gains=card.gains();
  std::lock_guard<std::mutex> lock(mixerMutex);mixerRequest=gains;mixerChanged=true;
 }
 void startProgram() {card.initialize(mixerFlag,mixerWriter(config));publishMixer();}
 void publishDigital(std::shared_ptr<const XeenVocSample> pcm) {
  std::shared_ptr<const XeenVocSample> retired;
  {
   std::lock_guard<std::mutex> lock(digitalMutex);
   retired=std::move(digitalRequest);digitalRequest=std::move(pcm);digitalChanged=true;
  }
 }
 bool prepareRender(int rate) {
  if(rate<=0)return false;
  if(mixRate==rate)return true;
  if(mixRate) {
   if(!fmStream->reconfigure(rate) || !cdStream->reconfigure(rate))return false;
   digitalPhase=digitalPhase*std::uint64_t(rate)/mixRate;mixRate=rate;return true;
  }
  auto stream=std::make_unique<SdlAudioStream>(49716,rate,kXeenFmGain);
  auto cd=std::make_unique<SdlAudioStream>(44100,rate,kXeenCdGain);
  if(!stream->valid() || !cd->valid())return false;
  fmStream=std::move(stream);cdStream=std::move(cd);mixRate=rate;return true;
 }
 void removed() {
  // Main thread, old callback joined. Keep the song/chip and logical deadlines.
  // Interrupted digital voices stay silent until their next explicit start.
  {std::lock_guard<std::mutex> lock(digitalMutex);digitalRequest.reset();digitalVoice.reset();digitalChanged=false;}
  cancelCd();cdVoice.reset();cdStream->clear();
  if(elapsed<cdEnd)outputNotice="CD speech interrupted by output removal; text restored.";
 }
 void render(std::int16_t *stereo,std::size_t frames,int rate) {
  if(!fmStream || rate!=mixRate)throw std::logic_error("Audio mix rate was not prepared");
  // Bound request latency within a large callback as well as between callbacks.
  // Only FM uses XeenFmSource's queue/barrier; digital never enters that queue.
  for(std::size_t offset=0;offset<frames;) {
   const auto count=std::min<std::size_t>(128,frames-offset);
   auto *out=stereo+offset*2;
   fmStream->pull([this](auto *pcm,auto count){output->pull(pcm,count);},out,count);
   output->check(); // Make a renderer failure silence every source in this callback.
   std::shared_ptr<const XeenVocSample> retired;
   {
    std::lock_guard<std::mutex> lock(digitalMutex);
    if(digitalChanged){retired=std::move(digitalVoice);digitalVoice=std::move(digitalRequest);digitalChanged=false;digitalPhase=0;}
   }
   retired.reset(); // Release potentially large PCM only after unlocking.
   {
    std::lock_guard<std::mutex> lock(mixerMutex);
    if(mixerChanged){mixerGains=mixerRequest;mixerChanged=false;}
   }
   // Worker owners pin both old and new PCM; dropping a callback reference
   // here or at EOF cannot destroy the allocation.
   std::shared_ptr<const std::vector<std::int16_t>> retiredCd;
   bool changed=false;
   {
    std::lock_guard<std::mutex> lock(cdMutex);
    if(cdChanged){retiredCd=std::move(cdVoice);cdVoice=std::move(cdRequest);cdChanged=false;cdCursor=0;changed=true;}
   }
   if(changed)cdStream->clear();
   std::array<std::int16_t,256> cd{};
   cdStream->pull([this](auto *pcm,auto frames) {
    std::fill_n(pcm,frames*2,0);
    if(cdVoice) {
     const auto count=std::min(frames*2,cdVoice->size()-cdCursor);
     std::copy_n(cdVoice->data()+cdCursor,count,pcm);cdCursor+=count;
     if(cdCursor==cdVoice->size())cdVoice.reset();
    }
   },cd.data(),count);
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
    for(unsigned channel=0;channel<2;++channel)
     out[f*2+channel]=mixXeenAudio(out[f*2+channel],digital,cd[f*2+channel],mixerGains,channel);
   }
   offset+=count;
  }
 }
 void command(unsigned command,unsigned argument=0) {if(clockDriver)clockDriver->command(command,argument);if(retainOutput)output->command(command,argument);}
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
 void advance(std::uint64_t now) {
  advanceClock(now);
  const auto notice=device->check();
  if(!notice.empty()){retainOutput=false;outputNotice=notice;std::cerr<<notice<<'\n';}
  if(pending && (!clockDriver || !clockDriver->fadeSpeed()) && elapsed>=cdEnd) {
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
XeenAudio::XeenAudio(unsigned id,PromusDriver::Bytes driver,Loader l,std::vector<std::uint8_t> module,std::uint64_t now,bool device,PromusDriver::Sink trace,std::shared_ptr<XeenFmSource> output,CdDecoder decoder):
 XeenAudio(legacyConfig(id),std::move(driver),std::move(l),std::move(module),now,device,std::move(trace),std::move(output),std::move(decoder)){}
XeenAudio::XeenAudio(XeenAudioConfig config,PromusDriver::Bytes driver,Loader l,std::vector<std::uint8_t> module,std::uint64_t now,bool device,PromusDriver::Sink trace,std::shared_ptr<XeenFmSource> output,CdDecoder decoder):
 _impl(new Impl(config,std::move(driver),std::move(l),std::move(module),now,device,std::move(trace),std::move(output),std::move(decoder))){}
XeenAudio::~XeenAudio()=default;
std::vector<std::uint8_t> XeenAudio::readProgramResource(const std::filesystem::path &folder,const std::string &name,XeenSceneArchive archive,const Loader &fallback) {
 std::optional<std::filesystem::path> loose;
 for(const auto &entry:std::filesystem::directory_iterator(folder))if(lower(entry.path().filename().u8string())==lower(name)) {
  if(loose || !entry.is_regular_file())throw std::runtime_error("Ambiguous loose audio resource: "+name);loose=entry.path();
 }
 if(loose)return read(ReadOnlyDataFile::plain(*loose));
 return fallback(name,archive);
}
std::shared_ptr<XeenAudio> XeenAudio::installed(const GameInstallation &installation,XeenAssetSource &,std::uint64_t now,bool device) {
 if(!installation.configuration)throw std::runtime_error("Missing installed WORLD/XEEN.CFG");
 const auto cfg=XeenAudioConfig::parse(read(*installation.configuration));
 if(!installation.uiModule)throw std::runtime_error("Missing installed WORLD/XEEN.DAT");
 const auto folder=installation.uiModule->physicalPath.parent_path();
 // Retain a separate resource owner for program lifetime; never borrow a session.
 auto source=std::make_shared<XeenAssetSource>(installation);
 Loader loader=[source,folder](const std::string &name,XeenSceneArchive archive) {
  return readProgramResource(folder,name,archive,[source](const auto &selected,auto from) {
   try{return source->readArchiveResource(selected,from);}catch(const std::exception &e){throw std::runtime_error("Original audio resource "+selected+": "+e.what());}
  });
 };
 auto resource=bytes(loader(std::string(cfg.musicDriver()),XeenSceneArchive::IntroOnly));
 (void)loader(std::string(cfg.digitalDriver(XeenAudioProgram::Dat)),XeenSceneArchive::IntroOnly);
 auto result=std::make_shared<XeenAudio>(cfg,resource,std::move(loader),read(*installation.uiModule),now,device);
 result->cdTracks(inspectXeenCdTracks(installation.cdTracks));return result;
}
unsigned XeenAudio::deviceId() const{return _impl->id;}
unsigned XeenAudio::digitalDeviceId() const{return _impl->config.digital;}
bool XeenAudio::hasDmaDigital() const{return _impl->config.dmaDigital();}
unsigned XeenAudio::digitalDma() const{return _impl->config.digitalDma();}
bool XeenAudio::entranceCaptions(bool efx) const{return _impl->config.captions(efx);}
bool XeenAudio::unavailableExeDigital() const{return _impl->config.unavailableExeDigital();}
bool XeenAudio::outputActive() const{return _impl->device->active();}
std::string XeenAudio::mapSong(const XeenMapGeometry &m) const {return _impl->maps.at(m.isOutdoors()?0:_impl->kinds.at(m.wallKind));}
std::string XeenAudio::locationSong(unsigned action) const{return _impl->locations.at(action);}
void XeenAudio::requestSong(std::string name,XeenSceneArchive archive,unsigned fade,std::uint64_t now) {advance(now);_impl->begin({std::move(name),archive,fade});}
void XeenAudio::fadeSong(unsigned command,std::uint64_t now) {
 if(command==1 || command>255)throw std::invalid_argument("Invalid song fade command");
 advance(now);_impl->pending.reset();_impl->afterTitle.reset();_impl->titleExit=false;
 _impl->command(command);
 if(command==0)_impl->current.clear(); // A later explicit start of this song must reload it.
}
void XeenAudio::mixer(unsigned level,std::uint64_t now) {
 if(level>255)throw std::invalid_argument("Invalid EXE music mixer level");
 advance(now);_impl->command(0x100,level);
 // Only promus implements command 0x100; it rewrites FM register 0x26.
 if(_impl->clockDriver){
  const auto port=_impl->config.musicPort;_impl->card.write(std::uint16_t(port+4),0x26);
  _impl->card.write(std::uint16_t(port+5),_impl->clockDriver->mixerLevel());_impl->publishMixer();
 }
}
void XeenAudio::processMixer(bool flag) {_impl->mixerFlag=flag;_impl->card=XeenCardMixer(cardModel(_impl->config),mixerAddress(_impl->config));_impl->startProgram();}
void XeenAudio::startProgram(std::uint64_t now) {advance(now);_impl->startProgram();}
bool XeenAudio::startProgram(std::uint64_t now,XeenAudioProgram program) {
 advance(now);
 if(program==XeenAudioProgram::Exe && unavailableExeDigital()) {
  if(!_impl->digitalNotice){std::cerr<<"Digital device "<<_impl->config.digital<<": not supported yet\n";_impl->digitalNotice=true;}
  return false;
 }
 _impl->startProgram();return true;
}
std::array<float,6> XeenAudio::mixerGains() const {return _impl->card.gains();}
XeenAudio::RetainedDigital XeenAudio::retainDigital(const std::string &name,XeenSceneArchive archive) {
 if(!hasDmaDigital())return {name,archive,{}};
 const auto driver=_impl->config.digital==6?XeenDmaDigital::Blast:XeenDmaDigital::Pro;
 return {name,archive,std::make_shared<const XeenVocSample>(decodeXeenVoc(_impl->loader(name,archive),driver))};
}
std::uint64_t XeenAudio::digitalDuration(const std::string &name,XeenSceneArchive archive) {
 if(!hasDmaDigital())return 0;
 return xeenVocDurationNanoseconds(_impl->loader(name,archive),_impl->config.digital==6?XeenDmaDigital::Blast:XeenDmaDigital::Pro);
}
void XeenAudio::requestMap(const XeenMapGeometry &map,std::uint64_t now) {const auto name=mapSong(map);_impl->mapName=name;requestSong(name,XeenSceneArchive::Clouds,207,now);}
void XeenAudio::requestLocation(unsigned action,std::uint64_t now) {requestSong(locationSong(action),XeenSceneArchive::Clouds,223,now);}
void XeenAudio::requestInn(std::uint64_t now) {requestSong("inn.m",XeenSceneArchive::Clouds,128,now);}
void XeenAudio::enterTitle(std::uint64_t now) {advance(now);_impl->pending.reset();_impl->afterTitle.reset();_impl->titleExit=false;_impl->load({"newbrigh.m",XeenSceneArchive::DarksideOnly,0},_impl->options.music);}
void XeenAudio::leaveTitle(std::uint64_t now) {advance(now);if(_impl->options.music)_impl->command(207);_impl->titleExit=true;_impl->pending=Impl::Song{"twnnitea.m",XeenSceneArchive::DarksideOnly,207};}
bool XeenAudio::titleTeardownReady(std::uint64_t now) {
 advance(now);
 return !digitalBusy(now); // DAT 0xb933..0xb947, before releasing session resources.
}
void XeenAudio::effect(unsigned id,std::uint64_t now) {
 // Use the action's existing clock sample, without querying a gameplay clock.
 advance(std::max(now,_impl->last));
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
 if(!_impl->options.efx || !hasDmaDigital())return;
 startDigital(retainDigital(name,archive),now);
}
void XeenAudio::startDigital(const RetainedDigital &voice,std::uint64_t now) {
 auto &s=*_impl;if(!s.options.efx || !hasDmaDigital())return;
 if(!voice.sample || voice.sample->pcm.empty())throw std::invalid_argument("Empty retained digital sample");
 const auto actionTime=s.digitalTime(now);
 s.digitalEnd=PresentationClock::checkedAdd(actionTime,voice.sample->durationNanoseconds());
 s.publishDigital(voice.sample);
 if(s.digitalSink)try {s.digitalSink(voice.name,voice.archive);}catch(...) {}
}
void XeenAudio::stopDigital(std::uint64_t now) {
 auto &s=*_impl;
 if(!s.options.efx || !hasDmaDigital())return;
 s.digitalTime(now);s.digitalEnd=0;s.publishDigital({});
}
bool XeenAudio::digitalBusy(std::uint64_t now) const {
 auto &s=*_impl;
 if(!s.options.efx || !hasDmaDigital())return false;
 if(s.config.digital==6 && s.digitalFailure){s.digitalFailure=false;s.digitalEnd=0;return false;}
 return s.logicalNow(now)<s.digitalEnd;
}
void XeenAudio::latchDigitalFailureForTest() {_impl->digitalFailure=true;}
std::optional<std::uint64_t> XeenAudio::digitalDeadline(std::uint64_t now) const {
 if(!digitalBusy(now))return {};
 return PresentationClock::checkedAdd(now,_impl->digitalEnd-_impl->logicalNow(now));
}
void XeenAudio::digitalSink(DigitalSink sink) {_impl->digitalSink=std::move(sink);}
void XeenAudio::cdTracks(std::vector<XeenCdExtent> tracks) {_impl->cdTracks=std::move(tracks);}
void XeenAudio::retainCdCue(XeenEventCdSpeech cue) {_impl->cdCue=cue;}
std::optional<XeenEventCdSpeech> XeenAudio::retainedCdCue() const {return _impl->cdCue;}
void XeenAudio::selectCdDisc(unsigned disc,std::uint64_t now) {
 auto &s=*_impl;if(!s.options.speech || disc==s.cdDisc)return;
 stopCd(now);s.cdDisc=disc;s.cdCue.reset();
}
bool XeenAudio::startCd(std::uint64_t now) {
 auto &s=*_impl;
 if(!s.options.speech || !s.cdCue || (!s.cdCue->track && !s.cdCue->start && !s.cdCue->finish))return false;
 const auto time=s.digitalTime(now);
 const auto request=mapXeenCdCue(s.cdTracks,s.cdDisc,*s.cdCue);
 s.cdEnd=PresentationClock::checkedAdd(time,request.duration);
 s.cancelCd();
 if(request.refused)return false;
 if(s.requiresDevice && !outputActive())return false;
 {
  std::lock_guard<std::mutex> lock(s.cdMutex);
  if(!request.error.empty()) {s.cdNotice="CD speech unavailable: "+request.error;return false;}
  s.cdDecoding=true;s.cdJob=Impl::CdJob{s.cdGeneration,request};
 }
 s.cdWake.notify_one();return true;
}
void XeenAudio::stopCd(std::uint64_t now) {auto &s=*_impl;s.digitalTime(now);s.cdEnd=0;s.cancelCd();}
bool XeenAudio::cdBusy(std::uint64_t now) const {return _impl->logicalNow(now)<_impl->cdEnd;}
std::optional<std::uint64_t> XeenAudio::cdDeadline(std::uint64_t now) const {
 if(!cdBusy(now))return {};
 return PresentationClock::checkedAdd(now,_impl->cdEnd-_impl->logicalNow(now));
}
std::string XeenAudio::takeCdNotice() {
 if(!_impl->outputNotice.empty()){auto text=std::move(_impl->outputNotice);_impl->outputNotice.clear();return text;}
 std::lock_guard<std::mutex> lock(_impl->cdMutex);auto text=std::move(_impl->cdNotice);_impl->cdNotice.clear();return text;
}
bool XeenAudio::cdDecodePending() const {std::lock_guard<std::mutex> lock(_impl->cdMutex);return _impl->cdDecoding;}
void XeenAudio::render(std::int16_t *out,std::size_t frames,int rate) {_impl->render(out,frames,rate);}
bool XeenAudio::prepareRender(int rate) {return _impl->prepareRender(rate);}
void XeenAudio::observeOptions(XeenSoundOptions value) {_impl->options=value;}
XeenSoundOptions XeenAudio::soundOptions() const {return _impl->options;}
void XeenAudio::toggleMusic(bool on,std::uint64_t now) {
 advance(now);_impl->options.music=on;
 if(on) {
  _impl->pending.reset();_impl->afterTitle.reset();_impl->titleExit=false;
  if(_impl->buffer)_impl->loadBuffer(true);
 }else _impl->command(0);
}
void XeenAudio::advance(std::uint64_t now) {_impl->advance(now);}
void XeenAudio::rebaseClock(std::uint64_t now) noexcept {_impl->last=now;}
std::optional<std::uint64_t> XeenAudio::deadline() const {
 if(!_impl->pending)return {};
 auto result=_impl->last;
 if(_impl->clockDriver) {
  const auto tick=_impl->clockDriver->ticks()+1;
  result=PresentationClock::checkedAdd(_impl->last,PromusDriver::clockBoundary(tick)-_impl->elapsed);
 }
 if(auto cd=cdDeadline(_impl->last))result=std::max(result,*cd);
 return result;
}
const std::string &XeenAudio::currentName() const{return _impl->current;}
const std::string &XeenAudio::loadedName() const{return _impl->loaded;}
std::uint64_t XeenAudio::ticks() const{return _impl->clockDriver?_impl->clockDriver->ticks():PromusDriver::clockTicks(_impl->elapsed);}
std::uint64_t XeenAudio::samples() const{return _impl->sample;}
bool XeenAudio::pending() const{return bool(_impl->pending);}
}
