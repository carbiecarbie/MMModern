#include "app/XeenFmSource.h"
#include "compat/scummvm/XeenOpl3.h"
#include <algorithm>
#include <atomic>
#include <list>
#include <exception>
#include <mutex>
#include <optional>
#include <stdexcept>
namespace mmodern {
struct XeenFmSource::Impl {
 enum class Kind {Load,Command,Effect,FadeBarrier};
 struct Operation {Kind kind;PromusDriver::Bytes bytes;unsigned value=0;};
 XeenOpl3 chip;PromusDriver::Sink trace;
 std::unique_ptr<PromusDriver> driver;
 // Nodes are allocated by the producer. Splicing into the consumer queue
 // transfers ownership without allocating in the audio callback.
 std::list<Operation> requests;std::mutex queue;
 // Only the consumer touches pending operations, fade state and driver time.
 std::list<Operation> pending;std::optional<std::uint64_t> barrierTick;
 std::uint64_t sample=0;std::atomic<std::uint64_t> publishedSample{0};
 mutable std::mutex errors;std::exception_ptr failure;
 bool initialized=false,failed=false;
 explicit Impl(PromusDriver::Sink sink):trace(std::move(sink)){}
 void append(Operation op) {
  if(!driver)return;
  std::lock_guard<std::mutex> lock(queue);requests.push_back(std::move(op));
 }
 void tickDue() {
  if(sample>=PromusDriver::sampleBoundary(driver->ticks()+1))driver->tick();
 }
 void apply() {
  while(!pending.empty()) {
   auto &op=pending.front();
   if(op.kind==Kind::FadeBarrier) {
    if(!barrierTick)barrierTick=driver->ticks();
    // Twice the longest fade in use: Inn's 128 eligible ticks.
    if(driver->fadeSpeed() && driver->ticks()-*barrierTick<256)return;
    barrierTick.reset();
   }else if(op.kind==Kind::Load)driver->load(std::move(op.bytes),op.value!=0);
   else if(op.kind==Kind::Command)driver->command(op.value);
   else driver->effect(op.value);
   pending.pop_front();
  }
 }
};
XeenFmSource::XeenFmSource(PromusDriver::Sink trace):_impl(new Impl(std::move(trace))){}
XeenFmSource::~XeenFmSource()=default;
void XeenFmSource::initialize(PromusDriver::Bytes resource) {
 if(_impl->initialized)throw std::logic_error("FM source already initialized");
 _impl->initialized=true;
 if(resource)_impl->driver=std::make_unique<PromusDriver>(std::move(resource),[self=_impl.get()](const auto &w){
  self->chip.write(w.reg,w.value);if(self->trace)self->trace(w);
 });
}
void XeenFmSource::load(PromusDriver::Bytes song,bool play) {_impl->append({Impl::Kind::Load,std::move(song),unsigned(play)});}
void XeenFmSource::command(unsigned value) {_impl->append({Impl::Kind::Command,{},value});}
void XeenFmSource::effect(unsigned id) {_impl->append({Impl::Kind::Effect,{},id});}
void XeenFmSource::fadeBarrier() {_impl->append({Impl::Kind::FadeBarrier,{}});}
void XeenFmSource::pull(std::int16_t *stereo,std::size_t frames) {
 auto &s=*_impl;std::fill_n(stereo,frames*2,0);
 if(s.failed)return;
 try {
  if(s.driver) {
   s.tickDue();
   {std::lock_guard<std::mutex> lock(s.queue);s.pending.splice(s.pending.end(),s.requests);}
   s.apply();
   for(std::size_t i=0;i<frames;++i,++s.sample) {
    s.tickDue();s.apply();s.chip.sample(stereo+i*2);
   }
  }else s.sample+=frames;
  s.publishedSample.store(s.sample,std::memory_order_relaxed);
 }catch(...) {
  s.failed=true;std::fill_n(stereo,frames*2,0);
  std::lock_guard<std::mutex> lock(s.errors);s.failure=std::current_exception();
 }
}
void XeenFmSource::check() const {
 std::lock_guard<std::mutex> lock(_impl->errors);if(_impl->failure)std::rethrow_exception(_impl->failure);
}
std::uint64_t XeenFmSource::samples() const {return _impl->publishedSample.load(std::memory_order_relaxed);}
}
