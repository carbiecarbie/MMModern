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
void equal(const fs::path &a,const fs::path &b) {
    const auto left=XeenSaveFile::read(a),right=XeenSaveFile::read(b);
    child_test::require(XeenSaveFormat::encode(left)==XeenSaveFormat::encode(right),"M39 uninterrupted/restored exact bytes differ");
    for(unsigned i=0;i<30;++i)child_test::require(xeen_state::sameCharacter(left.characters[i],right.characters[i]) &&
        left.journey->supplements[i].owner==right.journey->supplements[i].owner &&
        xeen_state::sameInputs(left.journey->supplements[i].inputs,right.journey->supplements[i].inputs),"M39 field-level owner/supplement comparison differs");
    child_test::require(left.maps==right.maps,"M39 field-level per-map state comparison differs");
    child_test::require(left.camera.mapId==right.camera.mapId && left.camera.x==right.camera.x && left.camera.y==right.camera.y &&
        left.camera.direction==right.camera.direction && left.questItems==right.questItems && left.questFlags==right.questFlags &&
        left.journey->schema==right.journey->schema && left.journey->content==right.journey->content &&
        left.journey->worldFlags==right.journey->worldFlags,"M39 camera/domain/recovery/quest comparison differs");
    child_test::require(left.journey->random==right.journey->random && left.journey->context==right.journey->context &&
        left.activeRosterIds==right.activeRosterIds && left.gameFlags==right.gameFlags &&
        left.journey->purse==right.journey->purse,"M39 context/flags/RNG/treasure differs");
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
        child_test::require(orc.journey && orc.journey->schema==schema && orc.journey->content==content && orc.characters[1].currentSp==20 &&
            orc.characters[6].currentSp==25 && orc.map(23).monsters->slots[9].accounted && orc.map(23).monsters->slots[9].lifecycle==XeenActorLifecycle::Defeated &&
            orc.journey->purse->gold==810 && orc.journey->context->minutes==511 && orc.journey->context->ctr24==2 &&
            orc.journey->random->state==82049198u && orc.journey->random->count==14,"M39 genuine Orc Quiet checkpoint differs");
        equal(a,run("A-from-orc","A","A",dir/"A-A.mmsave"));
        child_test::require(drawsAfter(dir/"A.log","M39 CHECKPOINT A ")==drawsAfter(dir/"A-from-orc.log","M39 RESTORE EXACT BEFORE INPUT"),"M39 mainland continuation RNG request trace differs");
        // Completed city-entry Space contributes one minute and one opportunity;
        // the established Slime combat requests, draw cursor and SP stay exact.
        const auto city=XeenSaveFile::read(dir/"A-city.mmsave");
        child_test::require(city.map(28).monsters && city.characters[6].currentSp==23 && city.map(23).monsters->slots[9].accounted &&
            city.characters[1].currentSp==20 && city.journey->random->state==1391776747u && city.journey->random->count==23 &&
            city.journey->context->minutes==571 && city.journey->context->ctr24==22,"M39 genuine two-region Slime checkpoint differs");
        // M49: Slime hatred 16 selects one member per attack, not the entire
        // party (pinned combat.cpp:904-930). Two d2 poison strikes: the initial
        // save uses the Physical selector (M68, DOS 0x2d2e4: 1..25 and 1..27 for
        // these targets), then the poison save loop (1..40, 1..60) halves while it
        // succeeds and the damage stays nonzero; Seymour's bound Arrow save follows.
        child_test::require(drawsAfter(dir/"A.log","M39 CHECKPOINT A ")==std::vector<std::string>{
            "DRAW 0:5:5:87470069:15","DRAW 1:2:2:3385103793:16",
            "DRAW 1:25:13:891394312:17","DRAW 1:40:25:3323190024:18",
            "DRAW 0:5:3:321008529:19","DRAW 1:2:2:4283899417:20",
            "DRAW 1:27:3:2383559219:21","DRAW 1:60:6:3822316985:22",
            "DRAW 1:50:48:1391776747:23"},
            "M49 Slime single-target strike/save/Arrow RNG order differs");
        equal(a,run("A-from-city","A","city",dir/"A-city.mmsave"));
        child_test::require(drawsAfter(dir/"A.log","M39 CHECKPOINT city ")==drawsAfter(dir/"A-from-city.log","M39 RESTORE EXACT BEFORE INPUT"),"M39 city continuation RNG request trace differs");
        run("A-reset","A","reset",dir/"A-city.mmsave");
        run("controls","controls","fresh");
        run("native","native","fresh");
        for(const auto branch:{"B","C"}) {
            const auto fresh=run(branch,branch,"fresh");
            const bool waking=std::string(branch)=="B";
            const auto settled=XeenSaveFile::read(dir/(std::string(branch)+"-"+branch+".mmsave"));
            child_test::require(settled.journey->schema==schema && settled.journey->content==content &&
                settled.camera.mapId==XeenMapIdentity(23) && settled.camera.x==10 && settled.camera.y==12 &&
                settled.journey->context->minutes==(waking?594:591) && settled.journey->context->ctr24==16 &&
                settled.characters[1].currentSp==(waking?20:21) && settled.characters[6].currentSp==(waking?26:25) &&
                settled.journey->random->state==(waking?551172653u:2593204823u) && settled.journey->random->count==885+(waking?57u:29u) &&
                // Decision 8: the save holds no HP; the witness asserts the Toad's
                // live wound (43 or 35) before this checkpoint.
                settled.map(23).monsters->slots[15].lifecycle==XeenActorLifecycle::Present && !settled.map(23).monsters->slots[15].accounted,
                "M49 M71 ordinary Sleep/Arrow and Run checkpoint: catches wrong SP cost, draw publication, or combat accounting");
            // Decision 8 (M69): B and C save the Toad wounded and woken, which a
            // save cannot carry. The resumed run rebuilds it at MON HP, asleep,
            // and legitimately diverges from the uninterrupted one, so the two
            // continuations are not compared for equality.
            run(std::string(branch)+"-restore",branch,"continued",dir/(std::string(branch)+"-"+branch+".mmsave"));
            const auto log=[&](const std::string &name){std::ifstream in(dir/(name+".log"));return std::string{std::istreambuf_iterator<char>(in),{}};};
            const auto uninterrupted=log(branch),restored=log(std::string(branch)+"-restore");
            child_test::require(uninterrupted.find(std::string("M39 TOAD END HP=")+(waking?"43":"35")+" woken=1")!=std::string::npos,
                "M39 uninterrupted continuation lost the Toad's wound");
            child_test::require(restored.find("M39 DECISION8 TOAD HP=90 woken=0")!=std::string::npos &&
                restored.find("M39 TOAD END HP=90")!=std::string::npos,"M39 resumed continuation did not rebuild the Toad at MON HP");
        }
        std::cout<<"M39 ORIGINAL-RESOURCE / QUIET / FULL EXIT / EXACT RESTORE / CONTINUATION PASSED\n";
        child_test::removePassedRun(dir);return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
