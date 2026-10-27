#include "formats/xeen/XeenVoc.h"
#include "games/xeen/XeenCdSpeechTiming.h"
#include "formats/xeen/XeenAssetSource.h"
#include "XeenTestInstallation.h"
#include <algorithm>
#include <iostream>

using namespace mmodern;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
std::vector<std::uint8_t> sample(unsigned word,unsigned pcmBytes) {
    // Independently specify the driver's length word and available bytes.
    std::vector<std::uint8_t> bytes(0x20+pcmBytes,128);
    bytes[0x1b]=word;bytes[0x1c]=word>>8;
    return bytes;
}
}
int main(int argc,char **argv) {try {
    check(xeenCdSpeechDurationNanoseconds({0,0,0})==0,"All-zero CD request must be a no-op");
    check(xeenCdSpeechDurationNanoseconds({19,30,310})==3066666667ULL,"CD final digits are frames with 25-sector preroll");
    check(xeenCdSpeechDurationNanoseconds({19,370,2161})==18213333334ULL,"Gunther CD requested duration");
    check(xeenCdSpeechDurationNanoseconds({6,1118,1574})==5080000000ULL,"MMSSFF short carrier subrange");
    check(xeenVocDurationNanoseconds(sample(24480,24478))==24471*68000ULL,"dreams2 driver duration");
    check(xeenVocDurationNanoseconds(sample(28925,28923))==28916*68000ULL,"laff1 driver duration");
    check(xeenVocDurationNanoseconds(sample(33175,33173))==33166*68000ULL,"Clouds laughter driver duration");
    check(xeenVocDurationNanoseconds(sample(14850,14848))==14841*68000ULL,"Tavern goodbye driver duration");
    auto fixed=sample(13,12); // Programmed count 3, plays four samples.
    fixed[0x20]=0;fixed[0x21]=127;fixed[0x22]=128;fixed[0x23]=255;
    for(unsigned constant:{120,156,172,173,181,188}) {
        fixed[30]=constant;fixed[31]=99;fixed[20]=255;fixed[26]=9;fixed[29]=255;
        check(xeenVocDurationNanoseconds(fixed)==272000,"VOC metadata changed fixed timing");
        auto decoded=decodeXeenVoc(fixed);
        check(decoded.pcm==std::vector<std::uint8_t>({0,127,128,255}),"PCM offset/count or unsigned bytes changed");
        check(decoded.durationNanoseconds()==272000,"Decoded and logical durations differ");
    }
    check(decodeXeenVoc(sample(10,1)).pcm.size()==1,"Count zero still transfers one sample");
    check(decodeXeenVoc(sample(9,65536)).pcm.size()==65536,"Length subtraction must wrap as a 16-bit word");
    check(xeenVocSampleCount(sample(521,512),XeenDmaDigital::Blast)==508,"blastsnd L-14 count");
    check(xeenVocDurationNanoseconds(sample(521,512),XeenDmaDigital::Blast)==508*68000ULL,"blastsnd duration");
    check(decodeXeenVoc(sample(14,1),XeenDmaDigital::Blast).pcm.size()==1,"blastsnd zero DMA count");
    check(decodeXeenVoc(sample(13,65536),XeenDmaDigital::Blast).pcm.size()==65536,"blastsnd count wraps at 16 bits");
    bool shortBlast=false;
    try{(void)decodeXeenVoc(sample(521,507),XeenDmaDigital::Blast);}catch(const std::invalid_argument &){shortBlast=true;}
    check(shortBlast,"blastsnd accepted a short transfer");
    for(unsigned fault=0;fault<3;++fault) {
        auto broken=sample(13,4);
        if(fault==0)broken.resize(31);
        if(fault==1)broken.pop_back();
        if(fault==2)broken[0x1b]=14;
        bool rejected=false;try{(void)xeenVocDurationNanoseconds(broken);}catch(const std::invalid_argument &){rejected=true;}
        check(rejected,"Truncated prosnd layout accepted");
        rejected=false;try{(void)decodeXeenVoc(broken);}catch(const std::invalid_argument &){rejected=true;}
        check(rejected,"Decoder accepted truncated prosnd layout");
    }
    if(argc==2) {
        const auto installation=xeenTestInstallationDetector().detect(std::filesystem::u8path(argv[1]));
        check(bool(installation),"No original installation");XeenAssetSource assets(*installation);
        for(const auto &entry:std::vector<std::pair<const char *,unsigned>>{
            {"sphinx13.voc",188},{"blob.voc",120},{"boom1.voc",181},{"roc.voc",172},{"snakeman.voc",173}}) {
            const auto bytes=assets.readArchiveResource(entry.first,XeenSceneArchive::Clouds);
            check(bytes.at(30)==entry.second,"Original header rate differs from audited resource");
            const auto decoded=decodeXeenVoc(bytes);
            const unsigned word=unsigned(bytes.at(27))+256*unsigned(bytes.at(28));
            check(word>=10 && decoded.pcm.size()==word-9,"Original transfer count differs");
            check(decoded.durationNanoseconds()==(word-9)*68000ULL,"Original fixed-rate duration differs");
            check(std::equal(decoded.pcm.begin(),decoded.pcm.end(),bytes.begin()+32),"Original PCM offset differs");
            if(std::string(entry.first)=="sphinx13.voc") {
                check(word==39245 && decoded.pcm.size()==39236,"Sphinx independently audited count differs");
                check(!std::equal(bytes.begin(),bytes.begin()+8,"Creative"),"Sphinx damaged descriptor was repaired");
            }
        }
        const auto crodo1=assets.readArchiveResource("crodo1.voc",XeenSceneArchive::CloudsProgram);
        const auto crodo2=assets.readArchiveResource("crodo2.voc",XeenSceneArchive::CloudsProgram);
        check(xeenVocDurationNanoseconds(crodo1,XeenDmaDigital::Blast)==3981740000ULL &&
              xeenVocDurationNanoseconds(crodo2,XeenDmaDigital::Blast)==3794944000ULL,"Original Crodo blastsnd deadlines");
    }
    std::cout<<"Digital decoder and timing passed\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
