#include "XeenTestInstallation.h"
#include "XeenPurchaseTestSupport.h"
#include <cstdlib>
#include <new>
#include <iostream>
namespace allocation_test {long countdown=-1;bool triggered=false;}
void *operator new(std::size_t size) {
    if(allocation_test::countdown>=0 && allocation_test::countdown--==0){allocation_test::countdown=-1;allocation_test::triggered=true;throw std::bad_alloc();}
    if(auto *p=std::malloc(size?size:1))return p;throw std::bad_alloc();
}
void operator delete(void *p) noexcept{std::free(p);}
void operator delete(void *p,std::size_t) noexcept{std::free(p);}
void *operator new[](std::size_t size){return ::operator new(size);}
void operator delete[](void *p) noexcept{::operator delete(p);}
void operator delete[](void *p,std::size_t) noexcept{::operator delete(p);}
using namespace purchase_test;
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: purchase-allocation <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"installation unavailable");
        Inputs in(*installation);const auto source=service(in);
        for(unsigned stage=0;stage<4;++stage) {
            bool finished=false;
            for(unsigned skipped=0;skipped<1000;++skipped) {
                Fixture f(in,source);f.enter();
                if(stage==0){f.buy();f.choose(XeenInventoryCategory::Armor,3);XeenPurchaseTestAccess::consume(*f.flow);}
                else {f.quote(XeenInventoryCategory::Armor,3);XeenPurchaseTestAccess::consume(*f.flow);}
                if(stage>=2)XeenPurchaseTestAccess::confirm(*f.flow);
                if(stage==3)XeenPurchaseTestAccess::directQuote(*f.flow,0,XeenInventoryCategory::Weapons,1);
                allocation_test::triggered=false;allocation_test::countdown=skipped;
                try {
                    if(stage==0)XeenPurchaseTestAccess::directQuote(*f.flow,0,XeenInventoryCategory::Armor,3);
                    else if(stage==2)XeenPurchaseTestAccess::depart(*f.flow);
                    else XeenPurchaseTestAccess::confirm(*f.flow);
                }catch(const std::bad_alloc &){}
                allocation_test::countdown=-1;
                const bool paid=f.p.monsterTreasure->gold==670;
                if(stage==0)check(f.p.monsterTreasure->gold==870 && !f.p.roster.at(0).armor[4].id &&
                    f.p.serviceEconomy->wares[0][0][1][3].id==3,"quote allocation failure changed a durable owner");
                if(stage==1)check(f.p.monsterTreasure->gold==(paid?670u:870u) && f.p.roster.at(0).armor[4].id==(paid?3:0) &&
                    f.p.serviceEconomy->wares[0][0][1][3].id==(paid?5:3) && XeenPurchaseTestAccess::reservation(*f.flow)==(paid?2u:1u),
                    "allocation exposed partial payment/delivery/depletion/reservation");
                if(stage==2)check(f.p.monsterTreasure->gold==670 && f.p.roster.at(0).armor[4].id==3 &&
                    f.p.serviceEconomy->wares[0][0][1][3].id==5 && (f.p.encounterContext->day==8 || f.p.encounterContext->day==9),
                    "departure allocation lost purchase or exposed partial day");
                if(stage==3) {
                    const bool twice=f.p.monsterTreasure->gold==610;
                    check(f.p.monsterTreasure->gold==(twice?610u:670u) && f.p.roster.at(0).armor[4].id==3 &&
                        f.p.serviceEconomy->wares[0][0][1][3].id==5 && f.p.serviceEconomy->wares[0][0][0][1].id==(twice?15:6) &&
                        XeenPurchaseTestAccess::reservation(*f.flow)==(twice?3u:2u),"later allocation lost prior purchase or published a partial second one");
                }
                check(!f.flow->canSave() && (stage==2 || f.p.encounterContext->day==8),"allocation made active debt Quiet/advanced optional time");
                if(stage!=2)check(XeenPurchaseTestAccess::reservationCurrent(*f.flow),"allocation invalidated existing mandatory reservation");
                if(!allocation_test::triggered){check(skipped>0,"allocation sweep covered no positions");finished=true;
                    std::cout<<"Purchase allocation stage "<<stage<<" passed "<<skipped<<" positions\n";break;}
            }
            check(finished,"purchase allocation sweep bound exhausted");
        }
        return 0;
    }catch(const std::exception &e){allocation_test::countdown=-1;std::cerr<<e.what()<<'\n';return 1;}
}
