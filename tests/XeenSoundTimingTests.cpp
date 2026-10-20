#include "formats/xeen/XeenVocDuration.h"
#include "games/xeen/XeenCdSpeechTiming.h"
#include <iostream>

using namespace mmodern;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
std::vector<std::uint8_t> sample(unsigned count,unsigned constant=188) {
    const std::string signature="Creative Voice File\x1a";
    std::vector<std::uint8_t> bytes(signature.begin(),signature.end());
    bytes.resize(26);bytes[20]=26;
    bytes.push_back(1);
    const auto length=count+2;
    for(unsigned n=0;n<3;++n)bytes.push_back(length>>(8*n));
    bytes.push_back(constant);bytes.push_back(0);
    bytes.resize(bytes.size()+count,128);bytes.push_back(0);
    return bytes;
}
}
int main() {try {
    check(xeenCdSpeechDurationNanoseconds({0,0,0})==0,"All-zero CD request must be a no-op");
    check(xeenCdSpeechDurationNanoseconds({19,30,310})==3066666667ULL,"CD final digits are frames with 25-sector preroll");
    check(xeenCdSpeechDurationNanoseconds({19,370,2161})==18213333334ULL,"Gunther CD requested duration");
    check(xeenCdSpeechDurationNanoseconds({6,1118,1574})==5080000000ULL,"MMSSFF short carrier subrange");
    check(xeenVocDurationNanoseconds(sample(24478))==1664504000ULL,"dreams2 header duration");
    check(xeenVocDurationNanoseconds(sample(28923))==1966764000ULL,"laff1 header duration");
    check(xeenVocDurationNanoseconds(sample(33173))==2255764000ULL,"Clouds laughter header duration");
    check(xeenVocDurationNanoseconds(sample(14848))==1009664000ULL,"Tavern goodbye header duration");
    check(xeenVocDurationNanoseconds(sample(7,156))==700000ULL,"Header rate was replaced by fixed rate");
    auto continued=sample(2);continued.pop_back();
    continued.insert(continued.end(),{2,3,0,0,128,128,128,3,3,0,0,1,0,188,0});
    check(xeenVocDurationNanoseconds(continued)==7*68000ULL,"PCM continuation/silence timing");
    auto damagedDescriptor=sample(2);
    std::fill_n(damagedDescriptor.begin(),20,0);
    check(xeenVocDurationNanoseconds(damagedDescriptor)==2*68000ULL,"Descriptor text must not gate valid PCM timing");
    for(unsigned fault=0;fault<7;++fault) {
        auto broken=sample(2);
        std::fill_n(broken.begin(),20,0);
        if(fault==0)broken.resize(25);
        if(fault==1)broken[20]=255;
        if(fault==2)broken[31]=1;
        if(fault==3)broken.pop_back();
        if(fault==4)broken[27]=255;
        if(fault==5)broken[20]=25;
        if(fault==6)broken[26]=2; // Continuation before any rate is established.
        bool rejected=false;try{(void)xeenVocDurationNanoseconds(broken);}catch(const std::invalid_argument &){rejected=true;}
        check(rejected,"Malformed or unsupported VOC accepted");
    }
    std::cout<<"Silent sample timing passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
