#include "games/xeen/XeenPresentationRandom.h"
namespace mmodern {
unsigned XeenPresentationRandom::reduce(std::uint16_t value,unsigned lo,unsigned hi) {
 if(lo>hi || hi-lo>=32767)throw std::invalid_argument("Invalid presentation random range");
 return (value&0x7fff)%(hi-lo+1)+lo;
}
std::uint16_t XeenPresentationRandom::mix(Time t) {
 if(t.hour>=24 || t.minute>=60 || t.second>=60 || t.hundredth>=100 || t.midnight>255)
  throw std::invalid_argument("Invalid presentation random time");
 // EXE 0x1cc27..0x1cc61. Each operation retains its 16-bit carry/borrow.
 unsigned carry=0;
 const auto add=[&](std::uint16_t &a,unsigned b){const auto n=unsigned(a)+b+carry;a=std::uint16_t(n);carry=n>>16;};
 const auto sub=[&](std::uint16_t &a,unsigned b){const auto n=b+carry;carry=unsigned(a)<n;a=std::uint16_t(unsigned(a)-n);};
 const auto rotate=[&](std::uint16_t &a){const auto low=a&1;a=std::uint16_t((a>>1)|(carry<<15));carry=low;};
 std::uint16_t ax=0x2c00|t.midnight,cx=t.hour*256+t.minute,dx=t.second*256+t.hundredth;
 dx^=_words[0]; // XOR clears carry before the first ADC.
 add(cx,_words[1]);sub(ax,_words[2]);
 add(cx,cx);add(ax,ax);add(cx,cx);rotate(dx);
 add(cx,cx);rotate(dx);add(cx,cx);add(ax,ax);rotate(dx);add(cx,cx);
 add(_words[0],dx);sub(_words[1],cx);_words[2]^=ax;carry=0;
 add(ax,cx);sub(ax,dx);return ax;
}
unsigned XeenPresentationRandom::range(unsigned lo,unsigned hi) {
 const auto sample=_clock.sample();const auto &t=sample.civil;
 return reduce(mix({unsigned(t.hour),unsigned(t.minute),unsigned(t.second),sample.hundredth}),lo,hi);
}
}
