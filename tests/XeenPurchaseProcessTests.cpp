#include "XeenChildProcessTestSupport.h"
#include "XeenM42Evidence.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <set>
#include <sstream>
#include <zlib.h>
using namespace mmodern;
namespace fs=std::filesystem;
using m42_test::check;
namespace {
std::string trace(const std::string &text,const std::string &start) {
    const auto begin=text.find("MARK "+start+"\n");check(begin!=std::string::npos,"M42 continuation marker absent");
    std::istringstream lines(text.substr(begin));std::string line,result;
    while(std::getline(lines,line))for(const auto *prefix:{"DRAW ","PUBLICATION ","PREPARATION ","OPERATION ","INTEREST ","INPUT ","TRANSFER ","EQUIPMENT ","COMBAT_INPUT ","ENEMY_CONSUMER ","PLAYER_CONSUMER "})
        if(line.rfind(prefix,0)==0){result+=line+'\n';break;}
    return result;
}
void equal(const fs::path &a,const fs::path &b) {
    const auto x=XeenSaveFile::read(a),y=XeenSaveFile::read(b);m40_test::equalFields(x,y);
    check(XeenSaveFormat::encode(x)==XeenSaveFormat::encode(y),"M42 uninterrupted/restored full encoded fields differ");
}
std::string drawsBetween(const std::string &text,const std::string &before,const std::string &after) {
    const auto begin=text.find("MARK "+before+"\n"),end=text.find("MARK "+after+"\n",begin);
    check(begin!=std::string::npos && end!=std::string::npos,"M42 independent draw interval absent");
    std::istringstream lines(text.substr(begin,end-begin));std::string line,result;
    while(std::getline(lines,line))if(line.rfind("DRAW ",0)==0)result+=line+'\n';return result;
}
void context(const XeenSaveSnapshot &s,unsigned day,std::uint32_t state,std::uint64_t count) {
    check(s.journey && s.journey->schema==9 && s.journey->content==14 && s.journey->context->year==610 &&
        s.journey->context->day==day && s.journey->context->minutes==803 && s.journey->random->algorithm==1 &&
        s.journey->random->state==state && s.journey->random->count==count && s.journey->serviceEconomy->bank.gold==0 &&
        s.journey->serviceEconomy->bank.gems==0,"M42 independent literal checkpoint fields differ");
}
}
int main(int argc,char **argv) {
    try {
        check(argc==3,"usage: purchase-process <witness> <installation>");
        const fs::path executable=fs::absolute(argv[1]),installation=argv[2];
        const auto directory=child_test::freshDirectory(executable.parent_path()/"m42-process");
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
        SetEnvironmentVariableW(L"MMODERN_M42_STAGE",L"fresh");
        SetEnvironmentVariableW(L"MMODERN_M42_CONTROL",nullptr);
        const auto save=directory/"fresh.mmsave";
        const auto fresh=child_test::launch(executable,{L"--journey-region",L"--combat-seed",L"7",installation.wstring(),L"--save-file",save.wstring()},directory/"fresh.log",false,false,180000);
        check(fresh.exit==0 && fresh.output.find("M42 PRODUCTION WITNESS PASSED")!=std::string::npos,"M42 fresh application witness failed; inspect fresh.log");
        const auto a=XeenSaveFile::read(directory/"fresh-A.mmsave"),b=XeenSaveFile::read(directory/"fresh-B.mmsave"),
            b1=XeenSaveFile::read(directory/"fresh-B1.mmsave"),c=XeenSaveFile::read(directory/"fresh-C.mmsave"),d=XeenSaveFile::read(directory/"fresh-D.mmsave");
        context(a,8,799325555,1101);context(b,9,799325555,1101);context(b1,9,799325555,1101);context(c,10,799325555,1101);context(d,11,2959920300u,2009);
        check(a.journey->treasure->gold==870 && b.journey->treasure->gold==670 && b1.journey->treasure->gold==670 && c.journey->treasure->gold==670 && d.journey->treasure->gold==670,"M42 earned/debited purse literal differs");
        m42_test::sameCategory(a.journey->serviceEconomy->wares[0][0][0],m42_test::weaponsBefore());
        m42_test::sameCategory(a.journey->serviceEconomy->wares[0][0][1],m42_test::armorBefore());
        for(const auto *s:{&b,&b1,&c})m42_test::sameCategory(s->journey->serviceEconomy->wares[0][0][1],m42_test::armorAfter());
        m42_test::StockOracle initial{7,0,{}};const auto initialStock=initial.generate();
        check(initial.state==1652828136 && initial.count==901 && m40_test::stockBytes(initialStock)==m40_test::stockBytes(*a.journey->serviceEconomy),"M42 full original stock differs from independent oracle");
        m42_test::StockOracle restock{799325555,1101,{}};const auto expectedStock=restock.generate();
        const auto stock=m40_test::stockBytes(*d.journey->serviceEconomy);
        check(stock==m40_test::stockBytes(expectedStock) && restock.state==2959920300u && restock.count==2009,"M42 restock bytes/RNG differ from independent oracle");
        check(drawsBetween(fresh.output,"C","D")==restock.trace,"M42 complete prepared-and-committed restock request/raw/rejection trace differs from independent oracle");
        check(crc32(0,stock.data(),stock.size())==0x79dec2de,"M42 secondary stock CRC differs");
        check(xeenSameItem(b.characters[0].armor[5],{0,3,0,0}) && xeenSameItem(b1.characters[18].armor[4],{0,3,0,3}) &&
            xeenSameItem(b1.characters[18].armor[0],{0,2,0,0}) && xeenSameItem(d.characters[18].armor[4],{0,3,0,3}),"M42 physical delivery/transfer/remove/equip/restock retention differs");
        std::set<std::pair<DWORD,std::uint64_t>> processes{{fresh.pid,fresh.created}};
        for(const auto *stage:{"A","B","B1","C","D"}) {
            const std::string name=stage;const auto path=directory/("restore"+name+".mmsave");
            fs::copy_file(directory/("fresh-"+name+".mmsave"),path);SetEnvironmentVariableW(L"MMODERN_M42_STAGE",fs::path(name).c_str());
            const auto restored=child_test::launch(executable,{L"--load-game",installation.wstring(),path.wstring()},directory/("restore"+name+".log"),false,false,180000);
            check(processes.insert({restored.pid,restored.created}).second,"M42 process incarnation reused");
            check(restored.exit==0 && restored.output.find("RESTORE EXACT BEFORE INPUT")!=std::string::npos &&
                restored.output.find("M42 PRODUCTION WITNESS PASSED")!=std::string::npos,"M42 new-process restoration/continuation failed");
            bool following=false;
            for(const auto *checkpoint:{"A","B","B1","C","D","E"}) {if(checkpoint==name){following=true;continue;}if(following)equal(directory/("fresh-"+std::string(checkpoint)+".mmsave"),directory/("restore"+name+"-"+checkpoint+".mmsave"));}
            check(trace(fresh.output,name)==trace(restored.output,name),"M42 operation/input/publication/consumer/raw-draw continuation trace differs");
        }
        const auto weaponSave=directory/"weapons.mmsave";fs::copy_file(directory/"fresh-A.mmsave",weaponSave);SetEnvironmentVariableW(L"MMODERN_M42_STAGE",L"weapons");
        const auto weapons=child_test::launch(executable,{L"--load-game",installation.wstring(),weaponSave.wstring()},directory/"weapons.log",false,false,180000);
        check(processes.insert({weapons.pid,weapons.created}).second && weapons.exit==0 && weapons.output.find("M42 PRODUCTION WITNESS PASSED")!=std::string::npos,"M42 actual repeated duplicate Weapons integration failed");
        const auto shootSave=directory/"shoot.mmsave";fs::copy_file(directory/"fresh-D.mmsave",shootSave);SetEnvironmentVariableW(L"MMODERN_M42_STAGE",L"shoot");
        const auto shoot=child_test::launch(executable,{L"--load-game",installation.wstring(),shootSave.wstring()},directory/"shoot.log",false,false,180000);
        check(processes.insert({shoot.pid,shoot.created}).second && shoot.exit==0 && shoot.output.find("M42 PRODUCTION WITNESS PASSED")!=std::string::npos,"M42 actual stock missile32 purchase/transfer/equip/Shoot integration failed");
        for(const auto *control:{"upload-admission","copy-admission","upload-purchase","copy-purchase","upload-departure","copy-departure"}) {
            const auto path=directory/(std::string(control)+".mmsave");fs::copy_file(directory/"fresh-A.mmsave",path);const auto before=m40_test::diskBytes(path);
            SetEnvironmentVariableW(L"MMODERN_M42_STAGE",L"A");SetEnvironmentVariableW(L"MMODERN_M42_CONTROL",fs::path(control).c_str());
            const auto result=child_test::launch(executable,{L"--load-game",installation.wstring(),path.wstring()},directory/(std::string(control)+".log"),false,false,180000);
            check(processes.insert({result.pid,result.created}).second && result.exit==4 && result.output.find("M42 NATIVE FAILURE PRESERVATION PASSED")!=std::string::npos && before==m40_test::diskBytes(path),"M42 native failure changed previous disk or paid prefix");
        }
        SetEnvironmentVariableW(L"MMODERN_M42_CONTROL",nullptr);
        SetEnvironmentVariableW(L"MMODERN_M42_STAGE",nullptr);
        std::cout<<"M42 actual seed7 earned Armor A/B/B1/C/D/E and duplicate Weapons integration; exact distinct-process restores and traces passed; evidence "<<directory.u8string()<<'\n';return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
