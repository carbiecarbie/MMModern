#include "core/PresentationClock.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
using mmodern::PresentationClock;
namespace {
void check(bool v,const char *message){if(!v)throw std::runtime_error(message);}
}
int main(){try {
    using C=PresentationClock;
    const auto first=C::boundary(1),second=C::boundary(2);
    check(first==54925439 && second==109850878,"BIOS rational boundary");
    check(C::tick(first-1)==0 && C::tick(first)==1,"integer tick edge");
    check(C::after(0,2)==second && C::after(first-1,2)==second,"reset samples current integer phase");
    check(C::after(first,2)==C::boundary(3),"reset at increment");
    const auto outer=C::after(0,1),draw=C::after(0,2);
    check(draw>=outer && draw==second,"outer one tick overlaps nested draw");
    check(C::after(100000000000ULL,2)>100000000000ULL,"late wake resets after work");
    check(C::boundary(182065)==C::scale && C::tick(C::scale)==182065,"no accumulated millisecond rounding");
    std::uint64_t now=0;
    for(unsigned step:{4u,2u}) {
        unsigned writes=1,level=0;
        while(level<128) {
            const auto deadline=C::retraceAfter(now);
            check(deadline>now,"separate retrace edge");
            now=deadline+(writes==3?1000000000ULL:0); // One late wake, one write.
            level=std::min(128u,level+step);++writes;
        }
        check(writes==(step==4?33u:65u),"every clamped palette level retained");
    }
    check(C::retraceAfter(999999999)==1000000000,"70 Hz boundary crosses second");
    bool overflow=false;try{C::fromMilliseconds(~std::uint64_t{0});}catch(const std::overflow_error &){overflow=true;}
    check(overflow,"clock overflow fails closed");
    std::cout<<"Phase resets, nested waits, rational BIOS clock, retraces and late wakes passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
