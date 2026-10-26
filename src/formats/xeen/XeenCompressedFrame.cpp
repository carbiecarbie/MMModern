#include "formats/xeen/XeenCompressedFrame.h"
#include <array>
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace mmodern {
namespace {
[[noreturn]] void bad() {throw std::runtime_error("Malformed Xeen compressed frame");}
class Bits {
 const std::vector<std::uint8_t> &_bytes;
 std::size_t _position=4;
 unsigned _word=0,_remaining=0;
public:
 explicit Bits(const std::vector<std::uint8_t> &bytes):_bytes(bytes) {}
 unsigned bit() {
  if(!_remaining) {
   if(_position==_bytes.size())bad();
   _word=unsigned(_bytes[_position++])<<8;_remaining=8;
   if(_position<_bytes.size()){_word|=_bytes[_position++];_remaining=16;}
  }
  const auto b=_word>>15;_word=(_word<<1)&65535;--_remaining;return b;
 }
 unsigned take(unsigned count) {unsigned n=0;while(count--)n=(n<<1)|bit();return n;}
};
}
std::vector<std::uint8_t> decodeXeenCompressedFrame(const std::vector<std::uint8_t> &bytes) {
 if(bytes.size()<4 || (unsigned(bytes[2])<<8|bytes[3])!=64000)bad();
 constexpr unsigned leaves=314,nodes=2*leaves-1,root=nodes-1;
 std::array<unsigned,nodes+1> frequency{};
 std::array<unsigned,nodes> child{};
 std::array<unsigned,nodes+leaves> parent{};
 for(unsigned i=0;i<leaves;++i){frequency[i]=1;child[i]=nodes+i;parent[nodes+i]=i;}
 for(unsigned i=0,j=leaves;j<=root;i+=2,++j) {
  frequency[j]=frequency[i]+frequency[i+1];child[j]=i;parent[i]=parent[i+1]=j;
 }
 frequency[nodes]=std::numeric_limits<unsigned>::max();parent[root]=0;
 std::array<std::uint8_t,4096> history{};
 for(unsigned i=0;i<history.size();++i)history[i]=bytes[i&1];
 unsigned cursor=4036;Bits input(bytes);
 std::vector<std::uint8_t> output;output.reserve(64000);
 const auto emit=[&](unsigned b){output.push_back(std::uint8_t(b));history[cursor]=std::uint8_t(b);cursor=(cursor+1)&4095;};
 while(output.size()<64000) {
  auto c=child[root];while(c<nodes)c=child[c+input.bit()];c-=nodes;
  if(c>=leaves)bad();
  if(frequency[root]==0x8000) {
   unsigned j=0;
   for(unsigned i=0;i<nodes;++i)if(child[i]>=nodes){frequency[j]=(frequency[i]+1)/2;child[j++]=child[i];}
   for(unsigned i=0;j<nodes;i+=2,++j) {
    const auto sum=frequency[i]+frequency[i+1];unsigned k=j;
    while(k && sum<frequency[k-1]){frequency[k]=frequency[k-1];child[k]=child[k-1];--k;}
    frequency[k]=sum;child[k]=i;
   }
   for(unsigned i=0;i<nodes;++i){const auto k=child[i];parent[k]=i;if(k<nodes)parent[k+1]=i;}
  }
  auto p=parent[c+nodes];
  do {
   const auto f=++frequency[p];
   if(f>frequency[p+1]) {
    auto other=p+1;while(f>frequency[other+1])++other;
    frequency[p]=frequency[other];frequency[other]=f;
    auto a=child[p],b=child[other];parent[a]=other;if(a<nodes)parent[a+1]=other;
    parent[b]=p;if(b<nodes)parent[b+1]=p;
    std::swap(child[p],child[other]);p=other;
   }
   p=parent[p];
  }while(p);
  if(c<256){emit(c);continue;}
  auto prefix=input.take(8);unsigned upper,length;
  if(prefix<32){upper=0;length=3;}
  else if(prefix<80){upper=1+(prefix-32)/16;length=4;}
  else if(prefix<144){upper=4+(prefix-80)/8;length=5;}
  else if(prefix<192){upper=12+(prefix-144)/4;length=6;}
  else if(prefix<240){upper=24+(prefix-192)/2;length=7;}
  else {upper=48+prefix-240;length=8;}
  for(unsigned i=2;i<length;++i)prefix=(prefix<<1)|input.bit();
  auto from=(cursor-((upper<<6)|(prefix&63))-1)&4095;
  const auto count=c-253;if(output.size()+count>64000)bad();
  for(unsigned i=0;i<count;++i){emit(history[from]);from=(from+1)&4095;}
 }
 return output;
}
}
