#include "XeenChildProcessTestSupport.h"
#include "XeenM40Evidence.h"
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
    // Literal request/value oracle from the accepted plan, independent of
    // production draw conversion/generation. Algorithm-1 steps reconstruct raw.
    const unsigned hi[]{2,47,47,2,40,40,2,42,42,2,60,2,47,47,2,40,40,2,47,47,2,40,40,2,42,42,2,60,2,47,47,2,40,40,2,47,47,2,40,40,2,42,42,2,60,60,2,47,47,2,40,40,2,47,47,2,40,40,2,42,42,2,60,60,2,47,47,2,40,40,50};
    const unsigned value[]{1,26,44,2,29,21,2,34,23,1,2,1,29,47,2,29,2,1,18,35,1,13,7,2,11,24,1,5,2,11,39,1,20,2,2,19,31,1,7,25,1,7,18,2,52,38,2,45,36,1,24,34,1,11,24,2,34,22,1,5,40,1,50,30,1,29,37,1,15,6,21};
    static_assert(sizeof(hi)/sizeof(*hi)==71 && sizeof(value)/sizeof(*value)==71);
    child_test::require(actual.size()==71,"M40 post-stock trace raw-attempt count differs");
    std::uint32_t raw=3686439625u;std::string canonical;
    for(unsigned i=0;i<71;++i) {
        raw^=raw<<13;raw^=raw>>17;raw^=raw<<5;
        const auto line="DRAW 1:"+std::to_string(hi[i])+":"+std::to_string(value[i])+":"+std::to_string(raw)+":"+std::to_string(2110+i);
        child_test::require(actual[i]==line,"M40 independent request/raw/value trace differs");canonical+=line+'\n';
    }
    child_test::require(m40_test::sha256({canonical.begin(),canonical.end()})=="cf2803fc50d6165d49bc92c942c1ca77abbc7489707821af72340ec9d8c0f6a1","M40 accepted independent trace digest differs");
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
            if(source.empty())args={L"--journey-region",L"--combat-seed",L"3626689381",game.wstring(),L"--save-file",save.wstring()};
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
        child_test::require(a.journey && a.journey->schema==9 && a.journey->content==14 && a.journey->serviceEconomy &&
            a.journey->context->day==8 && a.journey->context->minutes==584 && a.journey->context->ctr24==2 &&
            a.journey->treasure->gold==810 && a.characters[6].armor[0].state==128 && a.characters[6].armor[1].state==128 &&
            a.journey->random->state==2732157854u && a.journey->random->count==1203,"M40 A original-resource checkpoint differs");
        child_test::require(b.journey->context->day==11 && b.journey->context->minutes==584 && b.journey->treasure->gold==808 &&
            b.characters[6].armor[0].state==0 && b.characters[6].armor[1].state==128 && b.journey->random->state==3686439625u && b.journey->random->count==2109,"M40 B generating departure checkpoint differs");
        child_test::require(c.journey->context->day==11 && c.journey->context->minutes==598 && c.journey->context->ctr24==16 &&
            d.journey->context->day==12 && d.journey->context->minutes==598 && d.journey->treasure->gold==807 && d.characters[6].armor[1].state==0 &&
            c.journey->random==d.journey->random,"M40 C/D later navigation/repair checkpoint differs");
        child_test::require(e.journey->context->day==12 && e.journey->context->minutes==628 && e.journey->context->ctr24==12 &&
            e.journey->treasure->gold==807 && e.characters[1].currentSp==14 && e.characters[6].currentSp==24 &&
            e.journey->random->state==2018868320u && e.journey->random->count==2180 && e.journey->vertigoActors->size()==52 &&
            e.journey->vertigoActors->at(36).lifecycle==XeenActorLifecycle::Defeated,"M40 E reset-Slime combat/casting checkpoint differs");
        child_test::require(m40_test::sha256(m40_test::stockBytes(*a.journey->serviceEconomy))=="39cbe3234d1701fc7859afbb31a5e48f7d41407c75b4fa2364f3ee87c9143b18" &&
            m40_test::sha256(m40_test::stockBytes(*b.journey->serviceEconomy))=="b2d744b92079134a10dc16c73ef4ec90e738a5b7d46b8e90229c5f240b9d6cc9" &&
            b.journey->serviceEconomy==e.journey->serviceEconomy,"M40 independent stock hash/retention differs");
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
        return 0;
    }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
