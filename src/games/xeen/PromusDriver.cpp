#include "games/xeen/PromusDriver.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
unsigned PromusDriver::word(const std::vector<std::uint8_t> &b,std::size_t p) const {
 if(p>=b.size() || b.size()-p<2)throw std::runtime_error("Truncated promus resource or song");
 return b[p]|unsigned(b[p+1])<<8;
}
unsigned PromusDriver::byte(Stream &s) {
 if(!s.bytes || s.pos>=s.bytes->size())throw std::runtime_error("Truncated promus bytecode");
 return (*s.bytes)[s.pos++];
}
PromusDriver::PromusDriver(Bytes resource,Sink sink):_resource(std::move(resource)),_sink(std::move(sink)) {
 if(!_resource || _resource->size()<0xadc+181*2)throw std::runtime_error("Invalid INTRO.CC promus resource");
 for(unsigned i=0;i<18;++i)_frequency[i]=_resource->at(0x10d+i);
 for(unsigned b=0;b<2;++b)for(unsigned v=0;v<9;++v) {
  _carrier[b][v]=_resource->at(0xe9+b*9+v);
  _levels[b][v]=_resource->at(0xfb+b*9+v);
 }
 _fadePhase=_resource->at(0x193);
 // Init 0x2a1..0x342; values are register semantics, never commercial tables.
 write(0x105,0);write(1,0x20);write(0x101,0x20);
 write(8,0);write(0x108,0);write(0xbd,0);write(0x1bd,0);write(0x105,1);
 for(unsigned v=0;v<9;++v)write(0xc0+v,0x20);
 for(unsigned v=0;v<9;++v)write(0x1c0+v,0x10);
 write(0x104,0);
}
std::uint64_t PromusDriver::sampleBoundary(std::uint64_t t) {
 constexpr auto n=rate*divisor;
 const auto whole=t/pit,part=(t%pit*n+pit-1)/pit;
 if(whole>(UINT64_MAX-part)/n)throw std::overflow_error("Promus sample clock exhausted");
 return whole*n+part;
}
std::uint64_t PromusDriver::clockTicks(std::uint64_t ns) {
 const auto seconds=ns/1000000000,cycles=seconds*pit;
 return cycles/divisor+((cycles%divisor)*1000000000+(ns%1000000000)*pit)/(1000000000*divisor);
}
std::uint64_t PromusDriver::clockBoundary(std::uint64_t t) {
 if(t>UINT64_MAX/divisor)throw std::overflow_error("Promus clock exhausted");
 const auto cycles=t*divisor,seconds=cycles/pit;
 const auto part=((cycles%pit)*1000000000+pit-1)/pit;
 if(seconds>(UINT64_MAX-part)/1000000000)throw std::overflow_error("Promus clock exhausted");
 return seconds*1000000000+part;
}
void PromusDriver::write(unsigned reg,unsigned value) {if(_sink)_sink({_ticks,reg,std::uint8_t(value),_interrupt});}
unsigned PromusDriver::frequency(unsigned v) const {return _frequency[v*2]|unsigned(_frequency[v*2+1])<<8;}
void PromusDriver::cache(unsigned i,unsigned f) {_frequency.at(i)=f;_frequency.at(i+1)=f>>8;}
unsigned PromusDriver::note(unsigned n) const {return word(*_resource,0x1b6+(n&31)*2)+((n&0xe0)<<5);}
void PromusDriver::tone(unsigned v,unsigned f) {
 write(0xa0+v,f);write(0xb0+v,f>>8);write(0x1a0+v,f);write(0x1b0+v,f>>8);
}
void PromusDriver::level(unsigned b,unsigned v,unsigned attenuation) {
 _levels[b][v]=attenuation;
 write(b*256+0x40+_resource->at(0x1ad+v),(_carrier[b][v]&0xc0)|attenuation);
}
void PromusDriver::stopMusic() {
 _music.playing=false;_fadeSpeed=0;
 for(unsigned b=0;b<2;++b)for(int v=6;v>=0;--v){cache(v*2,0);write(b*256+0xa0+v,0);write(b*256+0xb0+v,0);}
}
void PromusDriver::resetEffects() {
 for(unsigned b=0;b<2;++b)for(int v=8;v>=7;--v) {
  cache(v*2,0);write(b*256+0xa0+v,0);write(b*256+0xb0+v,0);level(b,v,63);
 }
}
void PromusDriver::load(Bytes song,bool start) {_music.bytes=std::move(song);if(start)command(1);}
void PromusDriver::command(unsigned c,unsigned argument) {
 if(c==0){stopMusic();return;}
 if(c==1){_music.pos=0;_music.start=0;_music.playing=bool(_music.bytes);_music.countdown=0;_music.stack.clear();_fadeSpeed=0;return;}
 if(c<256){_fadeSpeed=c;_fadeRemaining=63;return;}
 if(c==0x100){const unsigned h=((argument&0xf0)+0x60)&255;_mixerLevel=std::uint8_t(h+(h>>4));}
 // 0xffe0 is exposed by fadeSpeed(); PAS hardware mixing is separate from OPL.
}
void PromusDriver::effect(unsigned id) {
 if(id>180 || (id>=7 && id<=10 && _fx.playing))return;
 _fx.bytes=_resource;_fx.pos=_fx.start=word(*_resource,0xadc+id*2);
 _fx.playing=true;_fx.countdown=0;_slides[7].enabled=_slides[8].enabled=false;resetEffects();
}
void PromusDriver::patch(Stream &s,unsigned v,unsigned index) {
 if(v>8 || index>15)throw std::runtime_error("Invalid promus instrument");
 const auto p=s.patches[index];
 if(p>s.bytes->size() || s.bytes->size()-p<22)throw std::runtime_error("Truncated promus instrument");
 constexpr unsigned groups[]{0x20,0x40,0x60,0x80,0xe0};
 for(unsigned b=0;b<2;++b) {
  const auto at=p+b*11;
  for(unsigned o=0;o<2;++o)for(unsigned g=0;g<5;++g) {
   auto value=(*s.bytes)[at+o*5+g];
   if(o==1 && g==1) {
    _carrier[b][v]=value;
    if(b==0 && _fadeSpeed)value=(value&0xc0)|_fadeRemaining;
   }
   write(b*256+groups[g]+_resource->at((o?0x1ad:0x1a4)+v),value);
  }
  write(b*256+0xc0+v,(*s.bytes)[at+10]|(b?0x10:0x20));
 }
}
bool PromusDriver::execute(Stream &s,bool fx) {
 // Guard corrupt overrides; original streams surveyed in Part 0 always yield.
 for(unsigned budget=0;budget<65536;++budget) {
  const auto cmd=byte(s),v=cmd&15,n=cmd>>4;
  const bool voice=v<9,allowed=voice&&(v!=7 || word(*_resource,15)!=2);
  switch(n) {
  case 0:
   if(!fx && s.stack.size()<16){const auto p=word(*s.bytes,s.pos);s.stack.push_back(s.pos+2);s.pos=p;}break;
  case 1:s.countdown=v?v:byte(s);return true;
  case 2:s.patches[v]=s.pos;s.pos+=fx?22:26;break;
  case 3:case 10:
   if((fx && n==3)||(!fx && n==10)) {
    const auto selector=byte(s),a=byte(s);
    if(allowed && !_fadeSpeed && (selector==3||selector==4))level(selector-3,v,a);
   }break;
  case 4:case 5:if(!fx){byte(s);byte(s);}break;
  case 6:
   if(fx) {const auto f=note(byte(s));if(allowed){cache(v,f);tone(v,f);}} // DOS 0x722's byte-index defect.
   else byte(s);
   break;
  case 7:
   if(fx && voice){const auto f=frequency(v)&~0x2000;cache(v*2,f);write(0xb0+v,f>>8);write(0x1b0+v,f>>8);}break;
  case 8:
   {const auto value=byte(s);if(fx){if(allowed){const auto f=note(value);cache(v*2,f);tone(v,f);}}
    else if(v<7)tone(v,frequency(v)&0x1fff);}break;
  case 9:
   {auto f=note(byte(s));if(!fx)byte(s);
    if(allowed && (fx||v<7)){tone(v,f);f|=0x2000;cache(v*2,f);tone(v,f);}}break;
  case 11:if(!fx)while(byte(s)!=0xf7){}break;
  case 12:{const auto i=byte(s);if(allowed && (fx||v<7))patch(s,v,i);}break;
  case 13:if(voice)_slides[v].enabled=false;break;
  case 14:
   {const auto r=byte(s),hi=byte(s),lo=byte(s);
    if(allowed)_slides[v]={true,std::uint8_t(r),255,std::int16_t((hi<<8)|lo)};}break;
  case 15:
   if(v==15) {
    if(!fx && !s.stack.empty()){s.pos=s.stack.back();s.stack.pop_back();}
    else s.pos=s.start;
   }else{s.playing=false;return false;}
   break;
  }
 }
 throw std::runtime_error("Promus bytecode does not yield");
}
void PromusDriver::postprocess() {
 if(_fadeSpeed) {
  const auto sum=_fadePhase+_fadeSpeed;_fadePhase=sum&255;
  if(sum>255) {
   if(--_fadeRemaining<0)stopMusic();
   else for(int v=6;v>=0;--v)for(unsigned b=0;b<2;++b)
    if(_levels[b][v]!=63) {
     const auto previous=_levels[b][v];level(b,v,previous);_levels[b][v]=std::uint8_t(previous+1);
    }
  }
 }
 for(int v=8;v>=7;--v) {
  if(v==7 && word(*_resource,15)==2)continue;
  auto &s=_slides[v];if(!s.enabled)continue;
  const auto sum=unsigned(s.phase)+s.rate;s.phase=sum&255;if(sum<256)continue;
  const auto old=frequency(v);int f=std::int16_t(std::uint16_t((old&1023)+s.delta));unsigned octave=old&0x1c00;
  if(s.delta>=0 && f>=734){f=unsigned(std::uint16_t(f))>>1;if(!(f&1023))++f;octave=(octave+0x400)&0x1c00;}
  else if(s.delta<0 && f<=388){f=std::uint16_t(f*2);if(!(f&1023))--f;octave=(octave-0x400)&0x1c00;}
  const auto value=(f&1023)|octave|(old&0x2000);cache(v*2,value);tone(v,value);
 }
}
void PromusDriver::tick() {
 struct Scope {bool &flag;~Scope(){flag=false;}} scope{_interrupt};_interrupt=true;
 ++_ticks;
 const auto due=[](Stream &s){return s.playing && (!s.countdown || --s.countdown==0);};
 if(due(_music)){if(!execute(_music,false))return;}
 else if(due(_fx)){if(!execute(_fx,true))return;}
 postprocess();
}
}
