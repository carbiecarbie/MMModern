#include "app/XeenFmSource.h"
#include "compat/scummvm/XeenOpl3.h"
#include <algorithm>
#include <deque>
#include <mutex>
#include <stdexcept>
namespace mmodern {
struct XeenFmSource::Impl {
 XeenOpl3 chip;PromusDriver::Sink trace;
 std::deque<Write> requests;mutable std::mutex queue;
 std::uint64_t sample=0,through=0;
 explicit Impl(PromusDriver::Sink sink):trace(std::move(sink)){}
};
XeenFmSource::XeenFmSource(PromusDriver::Sink trace):_impl(new Impl(std::move(trace))){}
XeenFmSource::~XeenFmSource()=default;
void XeenFmSource::publish(std::uint64_t through,std::vector<Write> writes) {
 std::lock_guard<std::mutex> lock(_impl->queue);
 if(through<_impl->through)throw std::logic_error("FM publication moved backwards");
 for(auto &w:writes) {
  if(w.sample<_impl->sample || (!_impl->requests.empty() && w.sample<_impl->requests.back().sample))
   throw std::logic_error("FM register request moved backwards");
  _impl->requests.push_back(std::move(w));
 }
 _impl->through=through;
}
void XeenFmSource::pull(std::int16_t *stereo,std::size_t frames) {
 std::fill_n(stereo,frames*2,0);
 std::lock_guard<std::mutex> lock(_impl->queue);
 for(std::size_t i=0;i<frames && _impl->sample<_impl->through;++i,++_impl->sample) {
  while(!_impl->requests.empty() && _impl->requests.front().sample<=_impl->sample) {
   const auto w=_impl->requests.front();_impl->requests.pop_front();
   _impl->chip.write(w.reg.reg,w.reg.value);if(_impl->trace)_impl->trace(w.reg);
  }
  _impl->chip.sample(stereo+i*2);
 }
}
std::uint64_t XeenFmSource::samples() const {std::lock_guard<std::mutex> lock(_impl->queue);return _impl->sample;}
}
