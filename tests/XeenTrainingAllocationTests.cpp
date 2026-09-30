#include "XeenTrainingTestSupport.h"
#include <cstdlib>
#include <new>
#include <iostream>
namespace allocation_test {long countdown=-1;bool triggered=false;}
void *operator new(std::size_t size) {
    if(allocation_test::countdown>=0 && allocation_test::countdown--==0){allocation_test::countdown=-1;allocation_test::triggered=true;throw std::bad_alloc();}
    if(auto *p=std::malloc(size?size:1))return p;throw std::bad_alloc();
}
void operator delete(void *p) noexcept {std::free(p);}
void operator delete(void *p,std::size_t) noexcept {std::free(p);}
void *operator new[](std::size_t size){return ::operator new(size);}
void operator delete[](void *p) noexcept {::operator delete(p);}
void operator delete[](void *p,std::size_t) noexcept {::operator delete(p);}
using namespace training_test;
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: training-allocation <installation>");const auto installation=XeenInstallationDetector().detect(argv[1]);
        check(bool(installation),"original installation absent");Inputs in(*installation);const auto source=in.service(9);
        for(unsigned stage=0;stage<3;++stage) {
            bool finished=false;
            for(unsigned skipped=0;skipped<1000;++skipped) {
                Fixture fixture(in,source);fixture.enter();fixture.act(SelectMemberAction{1});fixture.act(AcknowledgeAction{});
                XeenTrainingTestAccess::consume(*fixture.flow);
                if(stage>0)XeenTrainingTestAccess::confirm(*fixture.flow);
                if(stage==2)while(!XeenTrainingTestAccess::level(*fixture.flow)){}
                allocation_test::triggered=false;allocation_test::countdown=skipped;
                try {
                    if(stage==0)XeenTrainingTestAccess::confirm(*fixture.flow);
                    if(stage==1)while(!XeenTrainingTestAccess::level(*fixture.flow)){}
                    if(stage==2)XeenTrainingTestAccess::depart(*fixture.flow);
                }catch(const std::bad_alloc &){}
                allocation_test::countdown=-1;
                const bool paid=fixture.p.monsterTreasure->gold==710;
                check(fixture.p.monsterTreasure->gold==(paid?710u:800u) && fixture.p.roster.at(18).permanentLevel==(paid?4:3) &&
                    fixture.p.roster.combatInputs(18)->experience==(paid?6000u:9000u) &&
                    fixture.p.roster.at(18).currentHp==(paid?64:int(source.characters[18].currentHp)),"allocation failure exposed partial level/payment/refill");
                const unsigned day=fixture.p.encounterContext->day;
                check(day==(stage==2?10u:paid?10u:9u) || (stage==2 && day==11),"allocation failure exposed partial member/departure day");
                check(!fixture.flow->canSave(),"allocation failure made debt saveable");
                if(!allocation_test::triggered){check(skipped>0,"allocation sweep covered no failures");finished=true;
                    std::cout<<"Training allocation stage "<<stage<<" passed "<<skipped<<" positions\n";break;}
            }
            check(finished,"Training allocation sweep bound exhausted");
        }
        return 0;
    }catch(const std::exception &e){allocation_test::countdown=-1;std::cerr<<e.what()<<'\n';return 1;}
}
