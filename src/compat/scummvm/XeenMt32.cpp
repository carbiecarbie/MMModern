#include "compat/scummvm/XeenMt32.h"
#include "audio/softsynth/mt32/mt32emu.h"
#include "audio/softsynth/mt32/MidiEventQueue.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
namespace mmodern {
namespace {
// The public API exposes neither the queue cursor nor its actual mask.
// Explicit instantiations restrict private-member access to these three fields.
template<class Tag,typename Tag::type Member> struct PrivateMember {
 friend typename Tag::type mt32Member(Tag) {return Member;}
};
struct SynthQueue {
 using type=MT32Emu::MidiEventQueue * MT32Emu::Synth::*;
 friend type mt32Member(SynthQueue);
};
struct QueuePosition {
 using type=volatile MT32Emu::Bit32u MT32Emu::MidiEventQueue::*;
 friend type mt32Member(QueuePosition);
};
struct QueueMask {
 using type=const MT32Emu::Bit32u MT32Emu::MidiEventQueue::*;
 friend type mt32Member(QueueMask);
};
template struct PrivateMember<SynthQueue,&MT32Emu::Synth::midiQueue>;
template struct PrivateMember<QueuePosition,&MT32Emu::MidiEventQueue::startPosition>;
template struct PrivateMember<QueueMask,&MT32Emu::MidiEventQueue::ringBufferMask>;

std::filesystem::path playerRomDirectory() {
 PWSTR value=nullptr;
 if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&value))){CoTaskMemFree(value);return {};}
 const std::filesystem::path path(value);CoTaskMemFree(value);
 return path.empty()?std::filesystem::path{}:path/"MMModern"/"roms";
}
struct ImageDelete {void operator()(const MT32Emu::ROMImage *image) const {if(image)MT32Emu::ROMImage::freeROMImage(image);}};
using Image=std::unique_ptr<const MT32Emu::ROMImage,ImageDelete>;
struct Input {
 std::vector<std::uint8_t> bytes;std::unique_ptr<MT32Emu::ArrayFile> file;
 const MT32Emu::ROMInfo *info=nullptr;
};
struct Report final:MT32Emu::ReportHandler {
 void printDebug(const char *,va_list) override {}
};
std::vector<std::filesystem::path> romCandidates(const std::filesystem::path &overrideDirectory) {
 const auto directory=overrideDirectory.empty()?playerRomDirectory():overrideDirectory;
 std::vector<std::filesystem::path> paths;std::error_code ec;
 std::filesystem::directory_iterator it(directory,ec),end;
 for(;!ec && it!=end;it.increment(ec)) {
  if(it->is_regular_file(ec))paths.push_back(it->path());
  if(ec)ec.clear();
 }
 return paths;
}
}
struct XeenMt32::Impl {
 struct PendingMidi {std::uint16_t sysexBytes;};
 std::array<PendingMidi,4096> pending{};std::size_t head=0,count=0,sysexBytes=0;std::uint32_t readPosition=0;
 std::vector<std::unique_ptr<Input>> inputs;
 Image control,pcm;
 Report report;
 // The ROM images and their backing files must outlive the synth.
 std::unique_ptr<MT32Emu::Synth> synth;
 std::string error;
 explicit Impl(const std::vector<std::filesystem::path> &paths) {
  for(const auto &path:paths) {
   std::error_code ec;const auto size=std::filesystem::file_size(path,ec);if(ec)continue;
   if(size!=32768 && size!=65536 && size!=262144 && size!=524288)continue;
   std::ifstream in(path,std::ios::binary);if(!in)continue;
   auto input=std::make_unique<Input>();input->bytes.assign(std::istreambuf_iterator<char>(in),{});
   if(input->bytes.size()!=size)continue;
   input->file=std::make_unique<MT32Emu::ArrayFile>(input->bytes.data(),input->bytes.size());
   input->info=MT32Emu::ROMInfo::getROMInfo(input->file.get());
   if(input->info)inputs.push_back(std::move(input));
  }
  const auto find=[&](const char *name)->Input * {
   for(const auto &input:inputs)if(std::string(input->info->shortName)==name)return input.get();
   return nullptr;
  };
  const auto image=[&](const char *full,const char *first,const char *second)->Image {
   if(auto *one=find(full))return Image(MT32Emu::ROMImage::makeROMImage(one->file.get()));
   auto *a=find(first),*b=find(second);
   return a && b?Image(MT32Emu::ROMImage::makeROMImage(a->file.get(),b->file.get())):Image{};
  };
  control=image("ctrl_mt32_1_07","ctrl_mt32_1_07_a","ctrl_mt32_1_07_b");
  pcm=image("pcm_mt32","pcm_mt32_l","pcm_mt32_h");
  if(!control || !pcm || !control->getROMInfo() || !pcm->getROMInfo() ||
     std::string(control->getROMInfo()->shortName)!="ctrl_mt32_1_07" ||
     std::string(pcm->getROMInfo()->shortName)!="pcm_mt32") {
   error="MT-32 output not supported yet: place the MT-32 1.07 control and MT-32 PCM ROMs in Local AppData/MMModern/roms.";return;
  }
  synth=std::make_unique<MT32Emu::Synth>(&report);
  synth->preallocateReverbMemory(true);
  if(synth->setMIDIEventQueueSize(static_cast<MT32Emu::Bit32u>(pending.size()))!=pending.size()) {
   synth.reset();error="MT-32 output not supported yet: incompatible MIDI queue capacity.";return;
  }
  synth->configureMIDIEventQueueSysexStorage(65536);
  synth->setMIDIDelayMode(MT32Emu::MIDIDelayMode_IMMEDIATE);
  // Match the reference capture's mt32emu renderer settings.
  synth->setDACInputMode(MT32Emu::DACInputMode_NICE);
  synth->setNiceAmpRampEnabled(true);
  synth->setNicePanningEnabled(true);
  synth->setNicePartialMixingEnabled(true);
  synth->selectRendererType(MT32Emu::RendererType_BIT16S);
  if(!synth->open(*control,*pcm,MT32Emu::AnalogOutputMode_ACCURATE)) {
   synth.reset();error="MT-32 output not supported yet: the selected ROM pair could not be opened.";return;
  }
  const auto *queue=synth.get()->*mt32Member(SynthQueue{});
  if(!queue || (queue->*mt32Member(QueueMask{}))!=pending.size()-1) {
   synth.reset();error="MT-32 output not supported yet: incompatible MIDI queue capacity.";
  }
 }
};
XeenMt32::XeenMt32(const std::filesystem::path &path):_impl(new Impl(romCandidates(path))){}
XeenMt32::XeenMt32(const std::vector<std::filesystem::path> &paths):_impl(new Impl(paths)){}
XeenMt32::~XeenMt32()=default;
bool XeenMt32::available() const{return bool(_impl->synth);}
const std::string &XeenMt32::notice() const{return _impl->error;}
bool XeenMt32::hasHeadroom(std::size_t messages,std::size_t bytes) const {
 if(!_impl->synth)return true;
 // The core's 4096-slot ring leaves one slot empty. Keep its SysEx buffer
 // below half full so an upload also fits when the free space is split.
 const auto capacity=_impl->pending.size()-1;
 return _impl->count<=capacity && _impl->sysexBytes<=32768 &&
  messages<=capacity-_impl->count && bytes<=32768-_impl->sysexBytes;
}
void XeenMt32::send(const PromusDriver::MidiWrite &event) {
 if(!_impl->synth)return;
 const auto seconds=event.timeNanoseconds/1000000000,part=event.timeNanoseconds%1000000000;
 const auto timestamp=std::uint32_t(seconds*32000+part*32000/1000000000);
 bool accepted=false;
 if(event.bytes[0]==0xf0)accepted=_impl->synth->playSysex(event.bytes.data(),event.size,timestamp);
 else {
  unsigned message=event.bytes[0];if(event.size>1)message|=unsigned(event.bytes[1])<<8;if(event.size>2)message|=unsigned(event.bytes[2])<<16;
  accepted=_impl->synth->playMsg(message,timestamp);
 }
 if(!accepted)throw std::logic_error("MT-32 MIDI queue headroom invariant failed");
 const auto size=event.bytes[0]==0xf0?event.size:0;
 _impl->pending[(_impl->head+_impl->count)%_impl->pending.size()]={std::uint16_t(size)};
 ++_impl->count;_impl->sysexBytes+=size;
}
void XeenMt32::render(std::int16_t *stereo,std::size_t frames) {
 if(_impl->synth) {
  _impl->synth->render(stereo,static_cast<MT32Emu::Bit32u>(frames));
  // A note can stay queued after its timestamp while mt32emu aborts partials.
  // Observe the pinned queue cursor rather than guessing from sample time.
  const auto *queue=_impl->synth.get()->*mt32Member(SynthQueue{});
  const auto position=queue->*mt32Member(QueuePosition{});
  auto retired=(position-_impl->readPosition)&static_cast<std::uint32_t>(_impl->pending.size()-1);
  if(retired>_impl->count)throw std::logic_error("MT-32 MIDI queue cursor invariant failed");
  while(retired--) {
   _impl->sysexBytes-=_impl->pending[_impl->head].sysexBytes;
   _impl->head=(_impl->head+1)%_impl->pending.size();--_impl->count;
  }
  _impl->readPosition=position;
 }
 else std::fill_n(stereo,frames*2,0);
}
}
