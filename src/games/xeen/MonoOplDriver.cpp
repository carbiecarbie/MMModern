#include "games/xeen/PromusDriver.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
namespace {
// Targets are decoded resource offsets, not machine code. Resolve the selected
// resource's two dispatch tables to the behaviors implemented below.
unsigned monoDispatch(const std::vector<std::uint8_t> &image,unsigned nibble,bool fx,bool ad) {
 const auto at=(fx?0x1ea:0x1ca)+nibble*2;
 const unsigned target=image.at(at)|(unsigned(image.at(at+1))<<8);
 struct Handler {unsigned ad,blast,operation;};
 static constexpr Handler music[]{
  {0x789,0x77e,0},{0x7f2,0x7e7,1},{0x804,0x7f9,2},
  {0x56e,0x56c,16},{0x56c,0x56a,4},{0x56d,0x56b,6},
  {0x5d1,0x5cf,8},{0x613,0x611,9},{0x67c,0x67a,10},
  {0x562,0x560,11},{0x713,0x70b,12},{0x705,0x6fd,13},
  {0x6c5,0x6c3,14},{0x815,0x80a,15}};
 static constexpr Handler effect[]{
  {0x7cf,0x7c4,0},{0x7ac,0x7a1,1},{0x7be,0x7b3,2},
  {0x69f,0x69d,3},{0x56e,0x56c,16},{0x58a,0x588,6},
  {0x5b1,0x5af,7},{0x5ed,0x5eb,8},{0x645,0x643,9},
  {0x734,0x72c,12},{0x705,0x6fd,13},{0x6c5,0x6c3,14},{0x83a,0x82f,15}};
 if(fx){for(const auto &h:effect)if(target==(ad?h.ad:h.blast))return h.operation;}
 else {for(const auto &h:music)if(target==(ad?h.ad:h.blast))return h.operation;}
 throw std::runtime_error("Unsupported mono FM dispatch target");
}
}
void PromusDriver::monoInit() {
 if((_profile!=1 && _profile!=2 && _profile!=6) || !_resource ||
    _resource->size()<(_profile==1?0x975:0x94f)+181*2)
  throw std::runtime_error("Invalid INTRO mono FM driver resource");
 _image=*_resource;_music.bytes.reset();_music.pos=_music.start=_music.countdown=0;_music.playing=false;_music.stack.clear();_music.patches.fill(0);
 _fx.bytes.reset();_fx.pos=_fx.start=_fx.countdown=0;_fx.playing=false;_fx.stack.clear();_fx.patches.fill(0);
 _slides.fill(Slide{});_fadeSpeed=0;_fadeRemaining=0;
 _music.stack.reserve(16);_fx.stack.reserve(16);
 _fadePhase=_image.at(0x185);
 write(1,0x20);write(8,0);write(0xbd,0);
 monoStop();monoResetEffects();
}
void PromusDriver::restartProgram() {if(_profile!=3)monoInit();}
unsigned PromusDriver::monoFreq(unsigned v) const {return word(_image,0xfd+monoBase()+v*2);}
void PromusDriver::monoCache(unsigned v,unsigned f) {
 const auto at=0xfd+monoBase()+v*2;_image.at(at)=f;_image.at(at+1)=f>>8;
}
unsigned PromusDriver::monoNote(unsigned v) const {return word(_image,0x19a+(v&31)*2)+((v&0xe0)<<5);}
void PromusDriver::monoTone(unsigned v,unsigned f) {write(0xa0+v,f);write(0xb0+v,f>>8);}
void PromusDriver::monoLevel(unsigned v,unsigned a) {
 _image.at(0xf4+monoBase()+v)=a;
 write(0x40+_image.at(0x191+v),(_image.at(0xeb+monoBase()+v)&0xc0)|a);
}
void PromusDriver::monoStop() {
 _music.playing=false;_fadeSpeed=0;
 for(int v=6;v>=0;--v){monoCache(v,0);monoTone(v,0);}
}
void PromusDriver::monoResetEffects() {
 for(unsigned v=7;v<=8;++v) {
  if(v==7 && word(_image,15))continue;
  monoCache(v,0);monoTone(v,0);monoLevel(v,63);
 }
}
void PromusDriver::monoCommand(unsigned c,unsigned argument) {
 if(c==0){monoStop();return;}
 if(c==1){_music.pos=_music.start=0;_music.playing=bool(_music.bytes);_music.countdown=0;_music.stack.clear();_fadeSpeed=0;
  for(int v=6;v>=0;--v){monoCache(v,0);monoTone(v,0);}return;}
 if(c<256){if(_music.playing){_fadeSpeed=c;_fadeRemaining=63;}return;}
 if(c==0x100)_image.at(0x187)=argument;
}
void PromusDriver::monoEffect(unsigned id) {
 if(id>180 || (id>=7 && id<=10 && _fx.playing))return;
 _fx.bytes=_resource;_fx.pos=_fx.start=word(_image,(_profile==1?0x975:0x94f)+id*2);
 _fx.playing=true;_fx.countdown=0;_slides[7].enabled=_slides[8].enabled=false;monoResetEffects();
}
void PromusDriver::monoPatch(Stream &s,unsigned voice,unsigned index,bool fx) {
 if(index>15 || voice>8)throw std::runtime_error("Invalid mono FM instrument");
 auto p=word(_image,(_profile==1?0x11c:0x11e)+(fx?32:0)+index*2);
 if(!s.bytes || p>s.bytes->size() || s.bytes->size()-p<11)throw std::runtime_error("Truncated mono FM instrument");
 constexpr unsigned groups[]{0x20,0x40,0x60,0x80,0xe0};
 for(unsigned o=0;o<2;++o)for(unsigned g=0;g<5;++g) {
  unsigned value=s.bytes->at(p+o*5+g);
  if(o==1 && g==1) {
   _image.at(0xeb+monoBase()+voice)=value;
   value=(value+((std::uint8_t(0x7f-_image.at(0x187)))>>1))&255;
   if(value>63){value=63;if(_fadeSpeed)value=(value&0xc0)|_fadeRemaining;}
  }
  write(groups[g]+_image.at((o?0x191:0x188)+voice),value);
 }
 write(0xc0+voice,s.bytes->at(p+10));
}
bool PromusDriver::monoExecute(Stream &s,bool fx) {
 for(unsigned budget=0;budget<65536;++budget) {
  const auto cmd=byte(s),v=cmd&15,n=monoDispatch(_image,cmd>>4,fx,_profile==1);
  const auto compatibility=word(_image,15);
  const bool voice=v<9,allowed=voice && (v!=7 || compatibility==0);
  // The voice-7 guard is not uniform across FX handlers: the blastmus
  // level/note/slide paths compare the compatibility word with 2.
  const bool allowedTwo=voice && (v!=7 || (_profile==1?compatibility==0:compatibility!=2));
  switch(n) {
  case 16:break;
  case 0:if(s.stack.size()<16){const auto p=word(*s.bytes,s.pos);s.stack.push_back(s.pos+2);s.pos=p;}break;
  case 1:s.countdown=v?v:byte(s);return true;
  case 2:{const auto at=(_profile==1?0x11c:0x11e)+(fx?32:0)+2*v;_image.at(at)=s.pos;_image.at(at+1)=s.pos>>8;s.pos+=fx?11:26;break;}
  case 3:if(fx){const auto a=byte(s);if(allowedTwo && !_fadeSpeed)monoLevel(v,a);}break;
  case 4:case 5:if(!fx){byte(s);byte(s);}break;
  case 6:if(fx){const auto f=monoNote(byte(s));if(allowed){monoCache(v,f);monoTone(v,f);}}else byte(s);break;
  case 7:if(fx && voice){const auto f=monoFreq(v)&0xdfff;monoCache(v,f);write(0xb0+v,f>>8);}break;
  case 8:{const auto value=byte(s);if(fx){if(allowed){const auto f=monoNote(value);monoCache(v,f);monoTone(v,f);}}else if(v<7)monoTone(v,monoFreq(v)&0x1fff);break;}
  case 9:{const auto f=monoNote(byte(s));if(!fx)byte(s);if((fx?allowedTwo:voice && v<7)){monoTone(v,f);monoCache(v,f|0x2000);monoTone(v,f|0x2000);}break;}
  // INTRO music opcode A: admus 0x680 compares selector 2; blastmus and
  // its authenticated specmus alias compare 5 at 0x67e.
  case 10:if(!fx){const auto selector=byte(s),a=byte(s);if(voice && selector==(_profile==1?2u:5u) && !_fadeSpeed)monoLevel(v,a);}break;
  case 11:if(!fx)while(byte(s)!=0xf7){}break;
  // Blastmus 0x737 overwrites the preceding comparison flags, so its FX
  // patch command does not suppress voice 7 for any compatibility value.
  case 12:{const auto i=byte(s);if(fx?(voice && (v!=7 || _profile!=1 || compatibility==0)):(voice && v<7))monoPatch(s,v,i,fx);break;}
  case 13:if(voice)_slides[v].enabled=false;break;
  case 14:{if(_profile==1){_image.at(0x15e)=0;_image.at(0x15f)=0;}
   const auto r=byte(s),hi=byte(s),lo=byte(s);if(allowedTwo)_slides[v]={true,std::uint8_t(r),255,std::int16_t((hi<<8)|lo)};break;}
  case 15:if(v==15){if(!s.stack.empty()){s.pos=s.stack.back();s.stack.pop_back();}else s.pos=s.start;}
   else{s.playing=false;return false;}break;
  }
 }
 throw std::runtime_error("Mono FM bytecode does not yield");
}
void PromusDriver::monoPostprocess() {
 if(_fadeSpeed){const auto sum=_fadePhase+_fadeSpeed;_fadePhase=sum&255;
  if(sum>255){if(--_fadeRemaining<0)monoStop();else for(int v=6;v>=0;--v){const auto prev=_image.at(0xf4+monoBase()+v);if(prev<63){monoLevel(v,prev);_image.at(0xf4+monoBase()+v)=prev+1;}}}}
 if(_profile==1){const auto count=std::uint16_t(word(_image,0x15e)+1);_image.at(0x15e)=count;_image.at(0x15f)=count>>8;
  if(count>400){std::fill(_image.begin()+0x11d,_image.begin()+0x121,0);return;}}
 for(int v=8;v>=7;--v){const auto compatibility=word(_image,15);if(v==7 && (_profile==1?compatibility!=0:compatibility==2))continue;auto &s=_slides[v];if(!s.enabled)continue;
  const auto sum=unsigned(s.phase)+s.rate;s.phase=sum&255;if(sum<256)continue;
  const auto old=monoFreq(v);int f=std::int16_t(std::uint16_t((old&1023)+s.delta));unsigned octave=old&0x1c00;
  if(s.delta>=0 && f>=734){f=unsigned(std::uint16_t(f))>>1;if(!(f&1023))++f;octave=(octave+0x400)&0x1c00;}
  else if(s.delta<0 && f<=388){f=std::uint16_t(f*2);if(!(f&1023))--f;octave=(octave-0x400)&0x1c00;}
  const auto value=(f&1023)|octave|(old&0x2000);monoCache(v,value);monoTone(v,value);
 }
}
}
