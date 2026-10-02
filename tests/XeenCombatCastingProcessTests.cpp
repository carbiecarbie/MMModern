#include "XeenChildProcessTestSupport.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenStateEquality.h"
#include <set>
using namespace mmodern;
namespace fs=std::filesystem;
namespace {
std::vector<std::string> drawsAfter(const fs::path &log,const std::string &marker) {
    std::ifstream in(log);std::string line;bool begun=false;std::vector<std::string> draws;
    while(std::getline(in,line)){if(line.find(marker)!=std::string::npos)begun=true;else if(begun && line.rfind("DRAW ",0)==0)draws.push_back(line);}
    child_test::require(begun,"M39 trace checkpoint marker absent");return draws;
}
void equalActors(const std::vector<XeenSaveJourneyActor> &left,const std::vector<XeenSaveJourneyActor> &right) {
    child_test::require(left.size()==right.size(),"M39 region actor count differs");
    for(std::size_t i=0;i<left.size();++i) {
        const auto &a=left[i],&b=right[i];
        child_test::require(a.id==b.id && a.x==b.x && a.y==b.y && a.hp==b.hp && a.activated==b.activated &&
            a.lifecycle==b.lifecycle && a.status==b.status && a.accounted==b.accounted,"M39 field-level region actor comparison differs");
    }
}
void equal(const fs::path &a,const fs::path &b) {
    const auto left=XeenSaveFile::read(a),right=XeenSaveFile::read(b);
    child_test::require(XeenSaveFormat::encode(left)==XeenSaveFormat::encode(right),"M39 uninterrupted/restored exact bytes differ");
    for(unsigned i=0;i<30;++i)child_test::require(xeen_state::sameCharacter(left.characters[i],right.characters[i]) &&
        left.journey->supplements[i].owner==right.journey->supplements[i].owner &&
        xeen_state::sameInputs(left.journey->supplements[i].inputs,right.journey->supplements[i].inputs),"M39 field-level owner/supplement comparison differs");
    equalActors(left.journey->actors,right.journey->actors);
    child_test::require(left.journey->vertigoActors.has_value()==right.journey->vertigoActors.has_value(),"M39 retained city presence differs");
    if(left.journey->vertigoActors)equalActors(*left.journey->vertigoActors,*right.journey->vertigoActors);
    child_test::require(left.camera.mapId==right.camera.mapId && left.camera.x==right.camera.x && left.camera.y==right.camera.y &&
        left.camera.direction==right.camera.direction && left.questItems==right.questItems && left.questFlags==right.questFlags &&
        left.journey->schema==right.journey->schema && left.journey->contract==right.journey->contract &&
        left.journey->initializedMap==right.journey->initializedMap && left.journey->originalActorCount==right.journey->originalActorCount &&
        left.journey->regionalRecovery==right.journey->regionalRecovery,"M39 camera/domain/recovery/quest comparison differs");
    child_test::require(left.journey->random==right.journey->random && left.journey->context==right.journey->context &&
        left.activeRosterIds==right.activeRosterIds && left.gameFlags==right.gameFlags && left.disabledEvents==right.disabledEvents &&
        left.disabledObjects==right.disabledObjects && left.journey->treasure==right.journey->treasure,"M39 context/flags/RNG/treasure differs");
}
}
int main(int argc,char **argv) {
    try {
        child_test::require(argc==3,"usage: combat-casting-process <witness> <original-installation>");
        const auto exe=fs::absolute(argv[1]),game=fs::absolute(argv[2]);
        const unsigned schema=9,content=14;
        const auto dir=child_test::freshDirectory(fs::temp_directory_path()/"mmodern-m39-process");
        std::cout<<"M39 evidence: "<<dir<<'\n';
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
        std::set<std::pair<DWORD,std::uint64_t>> incarnations;
        const auto run=[&](const std::string &name,const std::string &branch,const std::string &stage,const fs::path &source={}) {
            const auto save=dir/(name+".mmsave");
            SetEnvironmentVariableW(L"MMODERN_M39_BRANCH",fs::path(branch).c_str());
            SetEnvironmentVariableW(L"MMODERN_M39_STAGE",fs::path(stage).c_str());
            std::vector<std::wstring> args;
            if(source.empty())args={L"--journey-region",L"--combat-seed",branch=="B" || branch=="C"?L"7":L"1",game.wstring(),L"--save-file",save.wstring()};
            else {fs::copy_file(source,save);args={L"--load-game",game.wstring(),save.wstring()};}
            const auto result=child_test::launch(exe,args,dir/(name+".log"),false,false,120000);
            child_test::require(incarnations.insert({result.pid,result.created}).second,"M39 distinct full-process incarnation missing");
            child_test::require(result.exit==0 && result.output.find("M39 PRODUCTION WITNESS PASSED")!=std::string::npos,"M39 production/process witness failed");
            if(!source.empty())child_test::require(result.output.find("M39 RESTORE EXACT BEFORE INPUT")!=std::string::npos,"M39 pre-input restore proof missing");
            return save;
        };
        const auto a=run("A","A","fresh");
        const auto orc=XeenSaveFile::read(dir/"A-A.mmsave");
        child_test::require(orc.journey && orc.journey->schema==schema && orc.journey->contract==content && orc.characters[1].currentSp==20 &&
            orc.characters[6].currentSp==25 && orc.journey->actors[9].accounted && orc.journey->actors[9].hp==0 &&
            orc.journey->treasure->gold==810 && orc.journey->context->minutes==511 && orc.journey->context->ctr24==2 &&
            orc.journey->random->state==82049198u && orc.journey->random->count==14,"M39 genuine Orc Quiet checkpoint differs");
        equal(a,run("A-from-orc","A","A",dir/"A-A.mmsave"));
        child_test::require(drawsAfter(dir/"A.log","M39 CHECKPOINT A ")==drawsAfter(dir/"A-from-orc.log","M39 RESTORE EXACT BEFORE INPUT"),"M39 mainland continuation RNG request trace differs");
        const auto city=XeenSaveFile::read(dir/"A-city.mmsave");
        child_test::require(city.journey->vertigoActors && city.characters[6].currentSp==23 && city.journey->actors[9].accounted &&
            city.characters[1].currentSp==20 && city.journey->random->state==2670584965u && city.journey->random->count==50 &&
            city.journey->context->minutes==570 && city.journey->context->ctr24==21,"M39 genuine two-region Slime checkpoint differs");
        equal(a,run("A-from-city","A","city",dir/"A-city.mmsave"));
        child_test::require(drawsAfter(dir/"A.log","M39 CHECKPOINT city ")==drawsAfter(dir/"A-from-city.log","M39 RESTORE EXACT BEFORE INPUT"),"M39 city continuation RNG request trace differs");
        run("A-reset","A","reset",dir/"A-city.mmsave");
        run("controls","controls","fresh");
        run("native","native","fresh");
        for(const auto branch:{"B","C"}) {
            const auto fresh=run(branch,branch,"fresh");
            const bool waking=std::string(branch)=="B";
            const auto settled=XeenSaveFile::read(dir/(std::string(branch)+"-"+branch+".mmsave"));
            child_test::require(settled.journey->schema==schema && settled.journey->contract==content &&
                settled.camera.mapId==XeenMapIdentity(23) && settled.camera.x==10 && settled.camera.y==12 &&
                settled.journey->context->minutes==(waking?592:591) && settled.journey->context->ctr24==16 &&
                settled.characters[1].currentSp==(waking?20:21) && settled.characters[6].currentSp==(waking?26:25) &&
                settled.journey->random->state==(waking?1073225241u:776403607u) && settled.journey->random->count==886+(waking?105u:86u) &&
                settled.journey->actors[15].hp==(waking?50:46) && !settled.journey->actors[15].accounted,
                "M39 unchanged seed7 production checkpoint differs");
            equal(fresh,run(std::string(branch)+"-restore",branch,"continued",dir/(std::string(branch)+"-"+branch+".mmsave")));
            child_test::require(drawsAfter(dir/(std::string(branch)+".log"),std::string("M39 CHECKPOINT ")+branch+" ")==drawsAfter(dir/(std::string(branch)+"-restore.log"),"M39 RESTORE EXACT BEFORE INPUT"),"M39 seed7 continuation RNG request trace differs");
        }
        std::cout<<"M39 ORIGINAL-RESOURCE / QUIET / FULL EXIT / EXACT RESTORE / CONTINUATION PASSED\n";
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
