#include "games/xeen/PromusDriver.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
#ifdef MMODERN_ROLMUS_ORACLE
// Present only in the capture-oracle executable, never in the game build.
unsigned rolmusOracleBX=8;
#endif
namespace {
// INTRO rolmus uses the same stream scheduler, but its dispatch targets and
// channel state belong to the selected MIDI resource, not to an OPL profile.
unsigned rolDispatch(const std::vector<std::uint8_t> &image,unsigned nibble,bool fx) {
 const auto at=(fx?0x1a0:0x180)+nibble*2;
 const unsigned target=image.at(at)|(unsigned(image.at(at+1))<<8);
 struct Handler {unsigned target,operation;};
 static constexpr Handler music[]{
  {0x8d3,0},{0x901,1},{0x913,2},{0x69a,16},{0x484,4},{0x698,5},
  {0x880,6},{0x6b6,8},{0x701,9},{0x788,10},{0x7ee,11},{0x8a0,12},
  {0x697,14},{0x927,15}};
 static constexpr Handler effect[]{
  {0x69a,16},{0x950,1},{0x962,2},{0x7bf,3},{0x870,4},{0x84e,5},
  {0x880,6},{0x6f0,8},{0x74c,9},{0x7ee,11},{0x8a0,12},{0x972,15}};
 if(fx){for(const auto &h:effect)if(target==h.target)return h.operation;}
 else {for(const auto &h:music)if(target==h.target)return h.operation;}
 throw std::runtime_error("Unsupported rolmus dispatch target");
}
constexpr std::uint64_t kReadNs=975,kWriteNs=725;
}
void PromusDriver::rolMpuReply(std::uint8_t value) {
 if(_mpuCount==_mpuReplies.size())throw std::runtime_error("Unexpected MPU response queue overflow");
 _mpuReplies[(_mpuHead+_mpuCount)%_mpuReplies.size()]=value;++_mpuCount;
}
void PromusDriver::rolMpuUpdate() {
 if(!_mpuReset || _midiCursor<_mpuResetDeadline)return;
 _mpuReset=false;
 if(_mpuPending>=0){_mpuPending=-1;rolMpuReply(0xfe);}
}
void PromusDriver::rolMpuPoll(unsigned mask) {
 for(unsigned remaining=0xffff;remaining;--remaining) {
  _midiCursor+=kReadNs;rolMpuUpdate();
  const bool ready=mask==0x40?_mpuPending<0:_mpuCount!=0;
  if(ready)return;
 }
 throw std::runtime_error("MPU reference polling counter exhausted");
}
void PromusDriver::rolMpuCommand(unsigned command) {
 if(command!=0xff && command!=0xdf && (command<0xd0 || command>0xd7))
  throw std::runtime_error("Unexpected MPU command");
 _midiCursor=std::max(_midiCursor,_midiNow);rolMpuUpdate();rolMpuPoll(0x40);
 _midiCursor+=kWriteNs;rolMpuUpdate();
 if(command==0xff) {
  _mpuReset=true;_mpuResetDeadline=_midiCursor+14000000;
  _mpuPending=-1;_mpuHead=_mpuCount=0;rolMpuReply(0xfe);
 }else if(_mpuReset)_mpuPending=int(command);
 else rolMpuReply(0xfe);
 rolMpuPoll(0x80);_midiCursor+=kReadNs;
 const auto reply=_mpuReplies[_mpuHead];_mpuHead=(_mpuHead+1)%_mpuReplies.size();--_mpuCount;
 if(reply!=0xfe)throw std::runtime_error("Unexpected MPU response");
}
void PromusDriver::rolMpuData() {
 rolMpuPoll(0x40);_midiCursor+=kWriteNs;rolMpuUpdate();
}
void PromusDriver::rolWord(std::size_t at,unsigned value) {
 _image.at(at)=std::uint8_t(value);_image.at(at+1)=std::uint8_t(value>>8);
}
void PromusDriver::rolShort(unsigned status,unsigned a,unsigned b,unsigned size) {
 rolMpuCommand(0xd0);for(unsigned i=0;i<size;++i)rolMpuData();
 if(!_midiSink)return;
 MidiWrite e{};e.tick=_ticks;e.timeNanoseconds=_midiCursor;e.interrupt=_interrupt;e.size=size;
 e.bytes[0]=std::uint8_t(status);e.bytes[1]=std::uint8_t(a);if(size==3)e.bytes[2]=std::uint8_t(b);
 _midiSink(e);
}
void PromusDriver::rolSysEx(const std::vector<std::uint8_t> &source,std::size_t &at,bool slow) {
 MidiWrite e{};e.tick=_ticks;e.interrupt=_interrupt;
 for(const auto b:std::array<std::uint8_t,5>{0xf0,0x41,0x10,0x16,0x12})e.bytes[e.size++]=b;
 unsigned checksum=0;
 // The driver's table and song streams contain the address and data followed
 // by F7; the checksum is computed over the address and data only.
 while(true) {
  if(at>=source.size())throw std::runtime_error("Truncated rolmus SysEx");
  const auto b=source[at++];if(b==0xf7)break;
  if(e.size+2>=e.bytes.size())throw std::runtime_error("Rolmus SysEx exceeds its bound");
  e.bytes[e.size++]=b;checksum+=b;
 }
 e.bytes[e.size++]=std::uint8_t((-checksum)&0x7f);e.bytes[e.size++]=0xf7;
 rolMpuCommand(0xdf);
 for(std::size_t i=0;i<e.size;++i) {
  if(slow && i>=5 && i+1<e.size){_midiCursor+=176*kReadNs;rolMpuUpdate();}
  rolMpuData();
 }
 e.timeNanoseconds=_midiCursor;if(_midiSink)_midiSink(e);
}
void PromusDriver::rolPitch(unsigned voice,unsigned pitch) {
 // The resource helper sends the word's high byte first, then its low byte.
 rolShort(0xe1+voice,(pitch>>8)&255,pitch&255);
}
void PromusDriver::rolInit() {
 if(!_resource || _resource->size()<0x10eb+181*2)throw std::runtime_error("Invalid INTRO rolmus resource");
 _image=*_resource;
 _music.bytes.reset();_music.pos=_music.start=_music.countdown=0;_music.playing=false;_music.stack.clear();_music.patches.fill(0);
 _fx.bytes.reset();_fx.pos=_fx.start=_fx.countdown=0;_fx.playing=false;_fx.stack.clear();_fx.patches.fill(0);
 _music.stack.reserve(16);_fx.stack.reserve(16);
 _fadeSpeed=0;_fadePhase=_image.at(0x27);_fadeRemaining=0;
 // Healthy MPU reset: FF completes after 14 ms on the render timeline. The
 // logical driver never waits for this deadline or for the synth.
 rolMpuCommand(0xff);
 rolShort(0xb9,7,_image.at(0x15d));
 for(int v=6;v>=0;--v)rolShort(0xb1+v,7,_image.at(0x15d));
 rolShort(0xb8,7,_image.at(0x15e));
 for(auto at:std::array<std::size_t,13>{0xa86,0xabc,0x10a3,0xbc1,0x10af,0xcbb,0x10bb,0xdb5,0x10c7,0xeaf,0x10d3,0xfa9,0x10df})
  rolSysEx(_image,at,false);
}
void PromusDriver::rolStop() {
 _music.playing=false;_fadeSpeed=0;
 for(int v=8;v>=0;--v)rolShort(0xb1+v,123,0);
}
void PromusDriver::rolCommand(unsigned c,unsigned argument) {
 if(c==0){rolStop();return;}
 if(c==1){
  _music.pos=_music.start=0;_music.playing=bool(_music.bytes);_music.countdown=0;_music.stack.clear();_fadeSpeed=0;
  // Both original program song wrappers pass zero for the note overlap flag.
  rolWord(0x165,0);
  for(int v=8;v>=0;--v)rolPitch(v,0x40);
  for(int v=8;v>=0;--v)rolShort(0xb1+v,10,63);
  for(int v=8;v>=0;--v)rolShort(0xb1+v,123,0);
  return;
 }
 if(c<256){_fadeSpeed=c;_fadeRemaining=127;return;}
 if(c==0x100){
  _image.at(0x15d)=std::uint8_t(argument);rolShort(0xb9,7,argument);
  for(int v=6;v>=0;--v)rolShort(0xb1+v,7,argument);
 }
}
unsigned PromusDriver::rolNote(unsigned n) const {
 const auto octave=(n&0xe0)>>2;return (_image.at(0x168+(n&31))+octave+(octave>>1))&127;
}
void PromusDriver::rolNoteOff(unsigned voice,unsigned note) {
 const auto old=voice==7?note:_image.at(0xfc+voice);
 _image.at(0xfc+voice)=255;rolShort(0x81+voice,old&127,127);
}
void PromusDriver::rolEffect(unsigned id) {
 if(id>180 || (id>=7 && id<=10 && _fx.playing))return;
 rolWord(0x11a,0);rolWord(0x11c,0);
 rolShort(0xb8,123,0);
 for(int v=8;v>=0;--v)rolPitch(v,0x40);
#ifdef MMODERN_ROLMUS_ORACLE
 _fx.bytes=Bytes(_resource,&_image);
#else
 _fx.bytes=_resource;
#endif
 _fx.pos=_fx.start=word(_image,0x10eb+id*2);
 _fx.playing=true;_fx.countdown=0;
}
bool PromusDriver::rolExecute(Stream &s,bool fx) {
 for(unsigned budget=0;budget<65536;++budget) {
  const auto cmd=byte(s),voice=cmd&15,op=rolDispatch(_image,cmd>>4,fx);
  switch(op) {
  case 16:break;
  case 0:if(s.stack.size()<16){const auto p=word(*s.bytes,s.pos);s.stack.push_back(s.pos+2);s.pos=p;}break;
  case 1:s.countdown=voice?voice:byte(s);return true;
  case 2:
   if(fx)_image.at(0x52+voice)=std::uint8_t(byte(s));
   else {s.pos+=24;_image.at(0x63+voice)=std::uint8_t(byte(s));byte(s);}break;
  case 3:{const auto value=byte(s);if(!_fadeSpeed){_image.at(0x14d+voice)=std::uint8_t(value);rolShort(0xb1+voice,11,value);}break;}
  case 4:
   if(fx)rolWord(0x10c+voice*2,0);
   else {const auto value=word(*s.bytes,s.pos);s.pos+=2;rolPitch(voice,value);}break;
  case 5:
   if(fx){const auto a=word(*s.bytes,s.pos),b=word(*s.bytes,s.pos+2);s.pos+=4;rolWord(0x10c+voice*2,a);rolWord(0x12c+voice*2,b);rolWord(0x20,0);}
   else {byte(s);byte(s);}break;
  case 6:{const auto value=byte(s);rolShort(0xb1+voice,10,value);break;}
  case 8:{const auto raw=byte(s),note=fx?rolNote(raw):(voice==8?raw:rolNote(raw));
   if(fx)rolNoteOff(voice,note);
   else {_image.at(0xfc+voice)=255;rolShort(0x81+voice,note&127,64);}break;}
  case 9:{const auto raw=byte(s),velocity=byte(s),note=voice==8?raw:rolNote(raw);
   if(!fx && voice!=8 && word(_image,0x165)!=1)rolNoteOff(voice,note);
   _image.at(0xfc+voice)=std::uint8_t(note&127);rolShort(0x91+voice,note&127,velocity);break;}
  case 10:{const auto selector=byte(s),value=byte(s);if(selector==0 && !_fadeSpeed){_image.at(0x14d+voice)=std::uint8_t(value);rolShort(0xb1+voice,11,value);}break;}
  case 11:rolSysEx(*s.bytes,s.pos,true);break;
  case 12:{const auto index=byte(s);rolShort(0xc1+voice,_image.at((fx?0x52:0x63)+index),0,2);break;}
  case 14:byte(s);byte(s);byte(s);break;
  case 15:
   if(voice==15){if(!fx && !s.stack.empty()){s.pos=s.stack.back();s.stack.pop_back();}else s.pos=s.start;}
   else {s.playing=false;return false;}break;
  }
 }
 throw std::runtime_error("Rolmus bytecode does not yield");
}
void PromusDriver::rolPostprocess() {
 if(_fadeSpeed) {
  const auto sum=_fadePhase+_fadeSpeed;_fadePhase=sum&255;
  if(sum>255) {
   if(--_fadeRemaining<0)rolStop();
   else {
    bool quiet=true;
    const auto fade=[&](unsigned v,unsigned channel) {
     auto &value=_image.at(0x14d+v);
     if(value>=40 && value<128){--value;quiet=false;rolShort(0xb1+channel,11,value);}
    };
#ifdef MMODERN_ROLMUS_ORACLE
    fade(rolmusOracleBX,8);
#else
    fade(8,8); // Approved fixed-BX approximation: docs/original-quirks.md.
#endif
    for(int v=6;v>=0;--v)fade(unsigned(v),unsigned(v));
    if(quiet)rolStop();
   }
  }
 }
 const auto delta=word(_image,0x11a);
 if(delta) {
  const auto counter=(word(_image,0x20)+1)&65535;rolWord(0x20,counter);
  if(counter==400){rolWord(0x11a,0);rolWord(0x20,0);}
  else {const auto value=(word(_image,0x13a)+delta)&65535;rolWord(0x13a,value);rolShort(0xe8,value&127,(value>>8)&127);}
 }
}
}
