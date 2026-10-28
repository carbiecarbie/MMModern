#include "app/XeenFmSource.h"
#include "compat/scummvm/XeenOpl3.h"
#include "compat/scummvm/XeenMt32.h"
#include <algorithm>
#include <atomic>
#include <list>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
namespace mmodern {
struct XeenFmSource::Impl {
 enum class Kind {Load,Command,Effect,FadeBarrier,Restart};
 struct Operation {Kind kind;PromusDriver::Bytes bytes;unsigned value=0,argument=0;};
 XeenOpl3 chip;PromusDriver::Sink trace;
 PromusDriver::MidiSink midiTrace;PromusDriver::MpuCommandSink mpuTrace;std::filesystem::path romDirectory;
 std::unique_ptr<XeenMt32> mt32;
 std::unique_ptr<PromusDriver> driver;
 // Nodes are allocated by the producer. Splicing into the consumer queue
 // transfers ownership without allocating in the audio callback.
 std::list<Operation> requests;std::mutex queue;
 std::atomic<std::size_t> queued{0};std::atomic<std::uint64_t> deferrals{0};
 // Only the consumer touches pending operations, fade state and driver time.
 std::list<Operation> pending;std::optional<std::uint64_t> barrierTick;
 std::uint64_t sample=0;std::atomic<std::uint64_t> publishedSample{0};
 mutable std::mutex errors;std::exception_ptr failure;
 bool initialized=false,failed=false;
 unsigned profile=3;
 explicit Impl(PromusDriver::Sink sink,std::filesystem::path roms,PromusDriver::MidiSink midi,PromusDriver::MpuCommandSink mpu):trace(std::move(sink)),midiTrace(std::move(midi)),mpuTrace(std::move(mpu)),romDirectory(std::move(roms)){}
 void append(Operation op) {
  if(!driver)return;
  std::lock_guard<std::mutex> lock(queue);
  requests.push_back(std::move(op));
  queued.fetch_add(1,std::memory_order_relaxed);
 }
 void tickDue() {
  const auto tick=driver->ticks()+1;
  const auto boundary=profile==4?(tick/PromusDriver::pit)*(48000*PromusDriver::divisor)+
   ((tick%PromusDriver::pit)*(48000*PromusDriver::divisor)+PromusDriver::pit-1)/PromusDriver::pit:PromusDriver::sampleBoundary(tick);
  if(sample>=boundary){if(profile==4)driver->midiTime(sample/48000*1000000000ULL+(sample%48000)*1000000000ULL/48000);driver->tick();}
 }
 void apply() {
  while(!pending.empty()) {
   auto &op=pending.front();
   // Restart emits 32 messages including 1688 upload bytes; a song load
   // emits 27 shorts. Retain additional space for the next driver tick and
   // the synth's processing of notes that must abort an existing partial.
   if(profile==4 && op.kind!=Kind::FadeBarrier && !mt32->hasHeadroom(1024,1688)) {
    deferrals.fetch_add(1,std::memory_order_relaxed);return;
   }
   if(op.kind==Kind::FadeBarrier) {
    if(!barrierTick)barrierTick=driver->ticks();
    // Twice the longest fade in use: Inn's 128 eligible ticks.
    if(driver->fadeSpeed() && (profile!=3 || driver->ticks()-*barrierTick<256))return;
    barrierTick.reset();
   }else if(op.kind==Kind::Restart)driver->restartProgram();
   else if(op.kind==Kind::Load)driver->load(std::move(op.bytes),op.value!=0);
   else if(op.kind==Kind::Command)driver->command(op.value,op.argument);
   else driver->effect(op.value);
   pending.pop_front();queued.fetch_sub(1,std::memory_order_relaxed);
  }
 }
};
XeenFmSource::XeenFmSource(PromusDriver::Sink trace,std::filesystem::path roms,PromusDriver::MidiSink midi,PromusDriver::MpuCommandSink mpu):_impl(new Impl(std::move(trace),std::move(roms),std::move(midi),std::move(mpu))){}
XeenFmSource::~XeenFmSource()=default;
void XeenFmSource::initialize(PromusDriver::Bytes resource,unsigned profile) {
 if(_impl->initialized)throw std::logic_error("FM source already initialized");
 _impl->initialized=true;
 _impl->profile=profile;
 if(resource) {
  if(profile==4)_impl->mt32=std::make_unique<XeenMt32>(_impl->romDirectory);
  _impl->driver=std::make_unique<PromusDriver>(std::move(resource),[self=_impl.get()](const auto &w){
   self->chip.write(w.reg,w.value);if(self->trace)self->trace(w);
  },profile,[self=_impl.get()](const auto &m){
   if(self->mt32)self->mt32->send(m);if(self->midiTrace)self->midiTrace(m);
  },_impl->mpuTrace);
 }
}
void XeenFmSource::load(PromusDriver::Bytes song,bool play) {_impl->append({Impl::Kind::Load,std::move(song),unsigned(play)});}
void XeenFmSource::command(unsigned value,unsigned argument) {_impl->append({Impl::Kind::Command,{},value,argument});}
void XeenFmSource::effect(unsigned id) {_impl->append({Impl::Kind::Effect,{},id});}
void XeenFmSource::restartProgram() {_impl->append({Impl::Kind::Restart,{}});}
void XeenFmSource::fadeBarrier() {_impl->append({Impl::Kind::FadeBarrier,{}});}
void XeenFmSource::pull(std::int16_t *stereo,std::size_t frames) {
 auto &s=*_impl;std::fill_n(stereo,frames*2,0);
 if(s.failed)return;
 try {
  if(s.driver) {
   s.tickDue();
   {std::lock_guard<std::mutex> lock(s.queue);s.pending.splice(s.pending.end(),s.requests);}
   if(s.profile==4)s.driver->midiTime(s.sample/48000*1000000000ULL+(s.sample%48000)*1000000000ULL/48000);
   s.apply();
   if(s.profile==4) {
    for(std::size_t i=0;i<frames;) {
     s.tickDue();s.driver->midiTime(s.sample/48000*1000000000ULL+(s.sample%48000)*1000000000ULL/48000);s.apply();
     const auto tick=s.driver->ticks()+1,n=48000*PromusDriver::divisor;
     const auto boundary=(tick/PromusDriver::pit)*n+((tick%PromusDriver::pit)*n+PromusDriver::pit-1)/PromusDriver::pit;
     const auto count=std::min<std::size_t>(frames-i,std::max<std::uint64_t>(1,boundary-s.sample));
     s.mt32->render(stereo+i*2,count);s.sample+=count;i+=count;
    }
   }else for(std::size_t i=0;i<frames;++i,++s.sample) {s.tickDue();s.apply();s.chip.sample(stereo+i*2);}
  }else s.sample+=frames;
  s.publishedSample.store(s.sample,std::memory_order_relaxed);
 }catch(...) {
  s.failed=true;std::fill_n(stereo,frames*2,0);
  std::lock_guard<std::mutex> lock(s.errors);s.failure=std::current_exception();
 }
}
const std::string &XeenFmSource::notice() const {static const std::string empty;return _impl->mt32?_impl->mt32->notice():empty;}
void XeenFmSource::check() const {
 std::lock_guard<std::mutex> lock(_impl->errors);if(_impl->failure)std::rethrow_exception(_impl->failure);
}
std::uint64_t XeenFmSource::samples() const {return _impl->publishedSample.load(std::memory_order_relaxed);}
std::uint64_t XeenFmSource::midiDeferrals() const {return _impl->deferrals.load(std::memory_order_relaxed);}
std::size_t XeenFmSource::pendingOperations() const {return _impl->queued.load(std::memory_order_relaxed);}
}
