// M87 DOS target draws shift the genuine repair route and its stock/combat cursor.
#include "XeenChildProcessTestSupport.h"
#include "XeenM40Evidence.h"
#include "XeenM42Evidence.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <set>
using namespace mmodern;
namespace fs=std::filesystem;
namespace {
std::vector<std::string> drawsAfter(const fs::path &log,const std::string &marker) {
    std::ifstream input(log);std::string line;bool begun=false;std::vector<std::string> draws;
    while(std::getline(input,line)){if(line.find(marker)!=std::string::npos)begun=true;else if(begun && line.rfind("DRAW ",0)==0)draws.push_back(line);}
    child_test::require(begun,"M40 trace checkpoint marker absent");return draws;
}
void equal(const fs::path &left,const fs::path &right) {
    const auto a=XeenSaveFile::read(left),b=XeenSaveFile::read(right);
    child_test::require(XeenSaveFormat::encode(a)==XeenSaveFormat::encode(b),"M40 uninterrupted/restored encoded bytes differ");
    m40_test::equalFields(a,b);
}
void independentTrace(const std::vector<std::string> &actual) {
    // Literal request/value oracle from the DOS single-target poison rule (M68:
    // target, d2, the Physical-selector initial save 1..B(Luck)+L+20, then the
    // poison save 1..resistance+40 drawn once even at zero damage), independent of
    // production draw conversion/generation. Algorithm-1 steps reconstruct raw.
    // M87: four six-participant attacks each draw R(1,6), then R(0,4).
    // Selected lanes 2,4,3,3 determine the two saving-throw bounds per strike.
    const unsigned lo[]{1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1};
    const unsigned hi[]{6,4,2,22,42,6,4,2,24,47,6,4,2,27,60,6,4,2,27,60,50};
    const unsigned value[]{2,2,2,20,16,5,4,1,8,46,2,3,1,21,35,4,3,1,18,58,40};
    static_assert(sizeof(hi)/sizeof(*hi)==21 && sizeof(value)/sizeof(*value)==21 && sizeof(lo)/sizeof(*lo)==21);
    child_test::require(actual.size()==21,"M40 post-stock trace raw-attempt count differs");
    std::uint32_t raw=290695566u;std::string canonical;
    for(unsigned i=0;i<21;++i) {
        raw^=raw<<13;raw^=raw>>17;raw^=raw<<5;
        const auto line="DRAW "+std::to_string(lo[i])+":"+std::to_string(hi[i])+":"+std::to_string(value[i])+":"+std::to_string(raw)+":"+std::to_string(2238+i);
        child_test::require(actual[i]==line,"M40 independent request/raw/value trace differs");canonical+=line+'\n';
    }
    // Every old secondary trace-hash mutation is caught by the complete
    // independent request/value/raw-state comparison above.
}
}
int main(int argc,char **argv) {
    try {
        child_test::require(argc==3,"usage: mmodern_service_day_process_tests <CLI-witness> <original-installation>");
        const auto exe=fs::absolute(argv[1]),game=fs::absolute(argv[2]);
        const auto dir=child_test::freshDirectory(fs::temp_directory_path()/"mmodern-m40-process");
        std::cout<<"M40 original-resource/process evidence: "<<dir<<'\n';
        SetEnvironmentVariableW(L"SDL_VIDEODRIVER",L"dummy");SetEnvironmentVariableW(L"SDL_RENDER_DRIVER",L"software");
        std::set<std::pair<DWORD,std::uint64_t>> incarnations;
        const auto run=[&](const std::string &name,const std::string &stage,const fs::path &source={},const std::string &control="",const std::string &branch="production") {
            const auto save=dir/(name+".mmsave");
            SetEnvironmentVariableW(L"MMODERN_M40_STAGE",fs::path(stage).c_str());
            SetEnvironmentVariableW(L"MMODERN_M40_CONTROL",control.empty()?nullptr:fs::path(control).c_str());
            SetEnvironmentVariableW(L"MMODERN_M40_BRANCH",fs::path(branch).c_str());
            std::vector<std::wstring> args;
            if(source.empty())args={L"--journey-region",L"--combat-seed",L"1114",game.wstring(),L"--save-file",save.wstring()};
            else {fs::copy_file(source,save);args={L"--load-game",game.wstring(),save.wstring()};}
            const auto before=source.empty()?std::vector<std::uint8_t>{}:XeenSaveFormat::encode(XeenSaveFile::read(save));
            const auto result=child_test::launch(exe,args,dir/(name+".log"),false,false,120000);
            child_test::require(incarnations.insert({result.pid,result.created}).second,"M40 full process incarnation reused");
            if(!source.empty())child_test::require(result.output.find("M40 RESTORE EXACT BEFORE INPUT")!=std::string::npos,"M40 exact field/pre-input/no replay proof absent");
            if(control.rfind("upload-",0)==0 || control.rfind("copy-",0)==0) {
                child_test::require(result.exit==4 && result.output.find("M40 NATIVE FAILURE PRESERVATION PASSED")!=std::string::npos &&
                    before==XeenSaveFormat::encode(XeenSaveFile::read(save)),"M40 native presentation failure changed disk or lost guarded prefix");
            } else if(control.rfind("aba-",0)==0) {
                child_test::require(result.exit==4 && before==XeenSaveFormat::encode(XeenSaveFile::read(save)),"M40 mutation/reversion failure did not preserve disk");
            } else child_test::require(result.exit==0 && result.output.find(!control.empty()?"M40 SYNTHETIC FAULT CONTINUATION PASSED":branch=="empty"?"M40 EMPTY DEPARTURE WITNESS PASSED":"M40 PRODUCTION WITNESS PASSED")!=std::string::npos,"M40 original-resource/process continuation failed");
            return save;
        };
        // Focused iteration may reuse an already certified original-resource A
        // checkpoint. The normal CTest invocation always runs the whole route.
        if(const auto focused=std::getenv("MMODERN_M40_ONLY_CONTROL")) {
            const auto source=std::getenv("MMODERN_M40_CONTROL_SOURCE");
            const auto expected=std::getenv("MMODERN_M40_CONTROL_EXPECTED");
            child_test::require(source && expected,"M40 focused control needs certified A and uninterrupted final snapshots");
            const auto result=run("focused","A",fs::path(source),focused);
            if(std::string(focused).rfind("fail-",0)==0 || std::string(focused)=="recursive")equal(result,fs::path(expected));
            std::cout<<"M40 FOCUSED SYNTHETIC CONTROL PASSED\n";return 0;
        }
        if(const auto branch=std::getenv("MMODERN_M40_ONLY_BRANCH")) {
            const auto source=std::getenv("MMODERN_M40_CONTROL_SOURCE"),expected=std::getenv("MMODERN_M40_CONTROL_EXPECTED");
            child_test::require(source && expected && std::string(branch)=="empty","M40 focused branch needs certified snapshots and the empty branch");
            const auto result=run("focused","A",fs::path(source),"",branch);
            for(const std::string label:{"empty11","empty20","empty21"}) {
                equal(result,run("focused-restore-"+label,label,dir/("focused-"+label+".mmsave"),"",branch));
                child_test::require(drawsAfter(dir/"focused.log","M40 CHECKPOINT "+label+" ")==drawsAfter(dir/("focused-restore-"+label+".log"),"M40 RESTORE EXACT BEFORE INPUT"),
                    "M40 focused later-service exact trace differs");
            }
            std::cout<<"M40 FOCUSED ORIGINAL-RESOURCE BRANCH PASSED\n";return 0;
        }
        const auto uninterrupted=run("production","fresh");
        const auto a=XeenSaveFile::read(dir/"production-A.mmsave"),b=XeenSaveFile::read(dir/"production-B.mmsave"),
            c=XeenSaveFile::read(dir/"production-C.mmsave"),d=XeenSaveFile::read(dir/"production-D.mmsave"),e=XeenSaveFile::read(dir/"production-E.mmsave");
        // Ten outdoor well Spaces plus indoor entry add 101 minutes and
        // eleven counters. Reset exit/entry later adds 11 minutes/two counters,
        // crossing midnight at E; the same fixed inputs still define A..D.
        // Part 0: WhoWill numeric selection and map-load clear suppress Space Wait.
        constexpr unsigned prefixMinutes=0,prefixSteps=0;
        constexpr unsigned resetEndMinutes=1349;
        child_test::require(a.journey && a.journey->schema==9 && a.journey->content==14 && a.journey->serviceEconomy &&
            a.journey->context->day==8 && a.journey->context->minutes==1305+prefixMinutes && a.journey->context->ctr24==(14+prefixSteps)%24 &&
            a.journey->purse->gold==840 && a.characters[1].armor[0].state==128 && a.characters[1].armor[1].state==128 &&
            a.journey->random->state==1372052976u && a.journey->random->count==1337,"M40 A original-resource checkpoint differs");
        child_test::require(b.journey->context->day==11 && b.journey->context->minutes==1305+prefixMinutes && b.journey->purse->gold==830 &&
            b.characters[1].armor[0].state==0 && b.characters[1].armor[1].state==128 && b.journey->random->state==290695566u && b.journey->random->count==2237,"M40 B generating departure checkpoint differs");
        child_test::require(c.journey->context->day==11 && c.journey->context->minutes==1319+prefixMinutes && c.journey->context->ctr24==(4+prefixSteps)%24 &&
            d.journey->context->day==12 && d.journey->context->minutes==1319+prefixMinutes && d.journey->purse->gold==829 && d.characters[1].armor[1].state==0 &&
            c.journey->random==d.journey->random,"M40 C/D later navigation/repair checkpoint differs");
        child_test::require(e.journey->context->day==12+resetEndMinutes/1440 && e.journey->context->minutes==resetEndMinutes%1440 && e.journey->context->ctr24==prefixSteps%24 &&
            e.journey->purse->gold==829 && e.characters[1].currentSp==16 && e.characters[6].currentSp==24 &&
            e.journey->random->state==2602426289u && e.journey->random->count==2258 && e.map(28).monsters->slots.size()==46 &&
            e.map(28).monsters->slots.at(36).lifecycle==XeenActorLifecycle::Defeated,"M40 E reset-Slime combat/casting checkpoint differs");
        // Mutation: incorrect item/material/charges, extra/missing generation
        // draws, duplicate restock or a partial RNG/stock publication.
        m42_test::StockOracle freshOracle{1114u,0,{}};const auto initialStock=freshOracle.generate();
        m42_test::StockOracle restockOracle{a.journey->random->state,a.journey->random->count,{}};
        const auto restocked=restockOracle.generate();
        child_test::require(m40_test::stockBytes(*a.journey->serviceEconomy)==m40_test::stockBytes(initialStock) &&
            m40_test::stockBytes(*b.journey->serviceEconomy)==m40_test::stockBytes(restocked) &&
            b.journey->random->state==restockOracle.state && b.journey->random->count==restockOracle.count &&
            b.journey->serviceEconomy==e.journey->serviceEconomy,"M40 independent stock/RNG/retention differs");
        std::ifstream generationLog(dir/"production.log");std::string generationLine,observedGeneration;bool generating=false;
        while(std::getline(generationLog,generationLine)) {
            if(generationLine.rfind("M40 CHECKPOINT B ",0)==0)break;
            if(generationLine.rfind("M40 CHECKPOINT A ",0)==0)generating=true;
            else if(generating && generationLine.rfind("DRAW ",0)==0)observedGeneration+=generationLine+'\n';
        }
        generationLog.close();
        child_test::require(observedGeneration==restockOracle.trace,"M40 generation request/raw/value trace differs");
        independentTrace(drawsAfter(dir/"production.log","M40 CHECKPOINT D "));
        for(const std::string label:{"A","B","C","D","E"}) {
            const auto restored=run("restore-"+label,label,dir/("production-"+label+".mmsave"));equal(uninterrupted,restored);
            const auto expected=drawsAfter(dir/"production.log","M40 CHECKPOINT "+label+" ");
            const auto actual=drawsAfter(dir/("restore-"+label+".log"),"M40 RESTORE EXACT BEFORE INPUT");
            child_test::require(expected==actual,"M40 uninterrupted/restored complete request/raw/value trace differs");
        }
        for(const std::string control:{"fail-before-reservation","fail-after-reservation","fail-stock-complete","fail-bank-prepared","fail-before-admission","fail-after-admission","fail-after-repair","fail-before-departure","fail-departure-published","fail-after-departure","fail-return","fail-before-event-settlement","fail-after-event-settlement","recursive"})
            equal(uninterrupted,run(control,"A",dir/"production-A.mmsave",control));
        for(const std::string control:{"aba-bank-gold","aba-bank-gems","aba-stock","aba-presence"})run(control,"A",dir/"production-A.mmsave",control);
        for(const std::string control:{"upload-admission","upload-repair","upload-departure","copy-admission","copy-repair","copy-departure"})run(control,"A",dir/"production-A.mmsave",control);
        const auto empty=run("empty","A",dir/"production-A.mmsave","","empty");
        const auto empty11=XeenSaveFile::read(dir/"empty-empty11.mmsave"),empty20=XeenSaveFile::read(dir/"empty-empty20.mmsave"),empty21=XeenSaveFile::read(dir/"empty-empty21.mmsave");
        child_test::require(empty11.journey->context->day==11 && empty20.journey->context->day==20 && empty21.journey->context->day==21 &&
            empty11.journey->random==empty20.journey->random && empty21.journey->random->count>empty20.journey->random->count,"M40 later genuine empty-service trigger dates/RNG differ");
        for(const std::string label:{"empty11","empty20","empty21"}) {
            equal(empty,run("empty-restore-"+label,label,dir/("empty-"+label+".mmsave"),"","empty"));
            child_test::require(drawsAfter(dir/"empty.log","M40 CHECKPOINT "+label+" ")==drawsAfter(dir/("empty-restore-"+label+".log"),"M40 RESTORE EXACT BEFORE INPUT"),
                "M40 later generating service uninterrupted/restored full raw/request trace differs");
        }
        std::cout<<"M40 ORIGINAL-RESOURCE A-E / QUIET F9 / FULL EXIT / EXACT PRE-INPUT RESTORE / IDENTICAL CONTINUATION PASSED\n";
        std::cout<<"M40 SYNTHETIC FAILURE/RETRY AND NEW ECONOMY MUTATION/REVERSION CONTROLS PASSED\n";
        child_test::removePassedRun(dir);return 0;
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
