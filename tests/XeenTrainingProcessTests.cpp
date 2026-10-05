#include "XeenChildProcessTestSupport.h"
#include "XeenM40Evidence.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenServiceDay.h"
#include <sstream>
#include <zlib.h>
using namespace mmodern;
namespace fs=std::filesystem;
using m40_test::check;
namespace {
std::string trace(const std::string &text,const std::string &start) {
    const auto begin=text.find("MARK "+start+"\n");check(begin!=std::string::npos,"M41 trace marker absent");
    std::istringstream lines(text.substr(begin));std::string line,result;
    while(std::getline(lines,line))if(line.rfind("DRAW ",0)==0 || line.rfind("DAY ",0)==0 || line.rfind("INTEREST ",0)==0 || line.rfind("INPUT ",0)==0)
        result+=line+'\n';
    return result;
}
void equal(const fs::path &a,const fs::path &b) {
    const auto x=XeenSaveFile::read(a),y=XeenSaveFile::read(b);m40_test::equalFields(x,y);
    check(XeenSaveFormat::encode(x)==XeenSaveFormat::encode(y),"M41 uninterrupted/restored full encoded state differs");
}
}
int main(int argc,char **argv) {
    try {
        check(argc==3,"usage: training-process <witness> <installation>");
        const fs::path executable=fs::absolute(argv[1]),installation=argv[2];
        const auto directory=child_test::freshDirectory(executable.parent_path()/"m41-process");
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");SetEnvironmentVariableW(L"MMODERN_M41_STAGE",L"fresh");
        const auto save=directory/"fresh.mmsave";
        const auto fresh=child_test::launch(executable,{L"--journey-region",L"--combat-seed",L"7",installation.wstring(),L"--save-file",save.wstring()},directory/"fresh.log",false,false,120000);
        check(fresh.exit==0 && fresh.output.find("M41 PRODUCTION WITNESS PASSED")!=std::string::npos,"M41 fresh witness failed");
        const auto a=XeenSaveFile::read(directory/"fresh-A.mmsave"),b=XeenSaveFile::read(directory/"fresh-B.mmsave");
        const auto bytes=m40_test::stockBytes(*b.journey->serviceEconomy);
        check(crc32(0,bytes.data(),bytes.size())==0x7b58546c,"M41 stock literal CRC mismatch");
        // Independent detached repeated-one-day continuation from complete A,
        // compared to every byte and draw/request trace from the production path.
        XeenServiceDayCandidate member1(*a.journey->context,*a.journey->serviceEconomy,*a.journey->random);
        XeenServiceDayCandidate member2(member1.context(),member1.economy(),member1.continuation());
        XeenServiceDayCandidate departure(member2.context(),member2.economy(),member2.continuation());
        while(!departure.service()){}
        check(departure.context()==*b.journey->context && departure.economy()==*b.journey->serviceEconomy &&
            departure.continuation()==*b.journey->random,"M41 full one-day chain bytes/context/RNG differ");
        for(const auto *stage:{L"A",L"B"}) {
            const std::string narrow=stage[0]=='A'?"A":"B";
            const auto restoredSave=directory/("restore"+narrow+".mmsave");
            fs::copy_file(directory/("fresh-"+narrow+".mmsave"),restoredSave);
            SetEnvironmentVariableW(L"MMODERN_M41_STAGE",stage);
            const auto restored=child_test::launch(executable,{L"--load-game",installation.wstring(),restoredSave.wstring()},directory/("restore"+narrow+".log"));
            check(restored.exit==0 && restored.output.find("RESTORE EXACT BEFORE INPUT")!=std::string::npos,"M41 full-exit/pre-input restore failed");
            check(fresh.pid!=restored.pid || fresh.created!=restored.created,"M41 process identity reused");
            if(narrow=="A")equal(directory/"fresh-B.mmsave",directory/"restoreA-B.mmsave");
            equal(directory/"fresh-C.mmsave",directory/("restore"+narrow+"-C.mmsave"));
            check(trace(fresh.output,narrow)==trace(restored.output,narrow),"M41 bounded/raw/rejection/input/service/interest continuation trace differs");
        }
        SetEnvironmentVariableW(L"MMODERN_M41_STAGE",nullptr);
        for(const auto *control:{L"upload-admission",L"copy-admission",L"upload-level",L"copy-level",L"upload-departure",L"copy-departure"}) {
            const fs::path name(control);const auto path=directory/(name.string()+".mmsave");
            fs::copy_file(directory/"fresh-A.mmsave",path);const auto before=m40_test::diskBytes(path);
            SetEnvironmentVariableW(L"MMODERN_M41_STAGE",L"A");SetEnvironmentVariableW(L"MMODERN_M41_CONTROL",control);
            const auto failed=child_test::launch(executable,{L"--load-game",installation.wstring(),path.wstring()},directory/(name.string()+".log"));
            check(failed.exit==4 && failed.output.find("M41 NATIVE FAILURE PRESERVATION PASSED")!=std::string::npos &&
                before==m40_test::diskBytes(path),"M41 native failure changed previous save or lost committed prefix");
        }
        SetEnvironmentVariableW(L"MMODERN_M41_STAGE",nullptr);SetEnvironmentVariableW(L"MMODERN_M41_CONTROL",nullptr);
        unsigned clockProbe=0;
        for(const std::string path:{"scheduling","cosmetic"})for(const std::string field:{"bank-gold","bank-gems","stock","presence"}) {
            const auto control="clock-"+path+"-"+field;const auto file=directory/(control+".mmsave");const bool afterTraining=clockProbe++==0;
            fs::copy_file(directory/(afterTraining?"fresh-A.mmsave":"fresh-B.mmsave"),file);
            SetEnvironmentVariableW(L"MMODERN_M41_STAGE",afterTraining?L"A":L"B");SetEnvironmentVariableW(L"MMODERN_M41_CONTROL",fs::path(control).c_str());
            const auto failed=child_test::launch(executable,{L"--load-game",installation.wstring(),file.wstring()},directory/(control+".log"));
            check(failed.exit==4 && failed.output.find("M41 POST-TRAINING CLOCK ABA MONOTONIC")!=std::string::npos,"M41 combat-clock ABA evidence absent");
            equal(directory/"fresh-B.mmsave",file);
        }
        for(const std::string control:{"clock-scheduling-bank-gold-throw","clock-cosmetic-presence-throw","presentation-stock"}) {
            const auto file=directory/(control+".mmsave");fs::copy_file(directory/"fresh-B.mmsave",file);
            SetEnvironmentVariableW(L"MMODERN_M41_STAGE",L"B");SetEnvironmentVariableW(L"MMODERN_M41_CONTROL",fs::path(control).c_str());
            const auto failed=child_test::launch(executable,{L"--load-game",installation.wstring(),file.wstring()},directory/(control+".log"));
            check(failed.exit==4 && failed.output.find("M41 POST-TRAINING CLOCK ABA MONOTONIC")!=std::string::npos,"M41 exceptional/presentation ABA evidence absent");
            equal(directory/"fresh-B.mmsave",file);
        }
        SetEnvironmentVariableW(L"MMODERN_M41_STAGE",nullptr);SetEnvironmentVariableW(L"MMODERN_M41_CONTROL",nullptr);
        std::cout<<"M41 exact A/B full-process restore and continuation passed; evidence "<<directory.u8string()<<'\n';return 0;
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
