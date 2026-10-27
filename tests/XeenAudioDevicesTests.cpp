#include "app/XeenAudioMix.h"
#include "formats/xeen/XeenAudioConfig.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace mmodern;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
bool near(float actual,float expected) {return std::abs(actual-expected)<1e-6f;}
void select(XeenCardMixer &m,std::uint16_t port,std::uint8_t reg,std::uint8_t value) {
 m.write(port+4,reg);m.write(port+5,value);
}
void config() {
 constexpr const char *music[]{"nullmus","admus","blastmus","promus","rolmus","canmus","specmus","ibmmus","wavemus","tapmus"};
 constexpr const char *dat[]{"nullsnd","ibmsnd","adsnd","nullsnd","nullsnd","source","blastsnd","prosnd","specsnd","covxsnd","prosnd"};
 constexpr const char *exe[]{"nullsnd","ibmsnd","adsnd","rolsnd","cansnd","source","blastsnd","prosnd","specsnd","covsnd","prosnd"};
 XeenAudioConfig c;
 for(unsigned id=0;id<12;++id) {
  c.music=id;c.digital=id;
  check(c.musicDriver()==music[id<10?id:0],"Music selection table");
  check(c.digitalDriver(XeenAudioProgram::Dat)==dat[id<11?id:0],"DAT digital selection table");
  check(c.digitalDriver(XeenAudioProgram::Exe)==exe[id<11?id:0],"EXE digital selection table");
  check(c.unavailableExeDigital()==(id==3 || id==4 || id==9),"EXE unavailable selection");
  check(c.dmaDigital()==(id==6 || id==7 || id==10),"DMA selection");
  check(c.captions(true)==(id<2) && c.captions(false),"Raw-id caption gate");
 }
 c.music=c.digital=65535;
 check(c.musicDriver()=="nullmus" && c.digitalDriver(XeenAudioProgram::Dat)=="nullsnd" &&
       c.digitalDriver(XeenAudioProgram::Exe)=="nullsnd" && !c.captions(true),"Out-of-range defaults or raw-id gate");
 std::vector<std::uint8_t> bytes(29);
 const auto put=[&](unsigned at,unsigned value){bytes[at]=value;bytes[at+1]=value>>8;};
 put(0,6);put(2,0x240);put(4,3);put(6,0x220);put(8,5);put(0x14,1);put(0x18,3);
 c=XeenAudioConfig::parse(bytes);
 check(c.digital==6 && c.digitalPort==0x240 && c.music==3 && c.musicPort==0x220 && c.irq==5 && c.mixer==1 && c.dma==3 && c.digitalDma()==1,"CFG fields or fixed blastsnd DMA");
 c.digital=7;check(c.digitalDma()==3,"prosnd must use CFG DMA");
 c.digital=10;check(c.digitalDma()==3,"prosnd id 10 must use CFG DMA");
 bool refused=false;try{XeenAudioConfig::parse(std::vector<std::uint8_t>(5));}catch(const std::runtime_error &){refused=true;}
 check(refused,"Truncated CFG accepted");
}
void mixer(XeenCardMixer::Model model) {
 XeenCardMixer m(model,0x240);
 auto g=m.gains();
 check(near(g[0],1) && near(g[2],kXeenSourceGain) && near(g[4],1),"Mixer reset gain");
 m.initialize(false,0x240);check(near(m.gains()[2],kXeenSourceGain),"Mixer flag ignored");
 m.initialize(true,0x220);check(near(m.gains()[2],kXeenSourceGain),"Wrong-address mixer init");
 m.initialize(true,0x240);g=m.gains();
 const float expected=model==XeenCardMixer::Model::SbPro?std::pow(10.0f,-11.0f/20):
                      model==XeenCardMixer::Model::Sb16?std::pow(10.0f,-24.0f/20):1.0f;
 check(near(g[0],1) && near(g[2],expected*kXeenSourceGain) && near(g[4],1),"Mixer init gains");
 select(m,0x220,0x26,0x00);check(near(m.gains()[0],1),"Mixer accepted wrong port");
 select(m,0x240,0x26,0x77);select(m,0x240,0x28,0x77);
 if(model==XeenCardMixer::Model::Sb2 || model==XeenCardMixer::Model::None)
  check(near(m.gains()[0],1) && near(m.gains()[4],1),"No-mixer ports affected gain");
 else {
  check(m.gains()[0]<1 && m.gains()[4]<1,"Mixer register updates ignored");
  const float channel=m.gains()[0];select(m,0x240,0x22,0x77);select(m,0x240,0x04,0x77);
  check(near(m.gains()[0],channel*channel) && near(m.gains()[2],channel*channel*kXeenSourceGain),"Master or DAC register update");
  const auto previous=m.gains();select(m,0x240,0x2e,0x00);select(m,0x240,0x42,0x00);
  check(m.gains()==previous,"Line or unsupported register changed output gains");
  select(m,0x240,0x00,0x00);
  check(near(m.gains()[0],1) && near(m.gains()[2],kXeenSourceGain) && near(m.gains()[4],1),"Mixer reset register");
 }
}
}
int main(int argc,char **argv) {try {
 check(argc==2,"Expected config, none, sb2, sbpro or sb16");const std::string mode=argv[1];
 if(mode=="config")config();
 else if(mode=="none")mixer(XeenCardMixer::Model::None);
 else if(mode=="sb2")mixer(XeenCardMixer::Model::Sb2);
 else if(mode=="sbpro")mixer(XeenCardMixer::Model::SbPro);
 else if(mode=="sb16")mixer(XeenCardMixer::Model::Sb16);
 else throw std::invalid_argument("Unknown audio-device test");
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
