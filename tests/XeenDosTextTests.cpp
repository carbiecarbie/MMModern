#include "XeenDosTextTestSupport.h"
#include "formats/xeen/XeenDosText.h"
#include "games/xeen/XeenInstallationDetector.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>
using namespace mmodern;
namespace {
void check(bool v,const char *message) {if(!v)throw std::runtime_error(message);}
using dos_test::u16; using dos_test::fixture;
template<class F> void rejects(F mutate) {
 auto b=fixture();mutate(b);bool failed=false;
 try {XeenDosText t(b);}catch(const std::invalid_argument &) {failed=true;}
 check(failed,"malformed UI module was published");
}
void synthetic() {
 const auto b=fixture();XeenDosText a(b),again(b);
 for(const auto &f:XeenDosText::layout())check(a.table(f.name)==again.table(f.name),"deterministic fields");
 check(a.table("CLASS_NAMES").at(10).empty(),"intentional empty class");
 check(a.scalar("DAY_SINGULAR").empty(),"interior empty day suffix");
 for(const auto &layout:XeenDosText::buttonLayouts())check(a.buttons(layout.name).size()==layout.count,"button table count");
 rejects([](auto &b){u16(b,0x4f162,320);});
 rejects([](auto &b){b[0x4f1bc]=199;});
 rejects([](auto &b){b[0x4f1cc]=0;});
 rejects([](auto &b){b[0x4f1ec]=0;});
 rejects([](auto &b){b[0x4f1fc]=2;});
 rejects([](auto &b){u16(b,0x4f162+8,0);});
 rejects([](auto &b){b[0]=0;});
 rejects([](auto &b){u16(b,8,2);}); // Packed/root load layout.
 rejects([](auto &b){b.resize(0x55000);});
 rejects([](auto &b){u16(b,24,0x56bf);});
 rejects([](auto &b){u16(b,6,0xffff);});
 rejects([](auto &b){u16(b,10,2);u16(b,12,1);});
 rejects([](auto &b){u16(b,22,0xffff);});
 rejects([](auto &b){u16(b,14,0xffff);u16(b,16,0xffff);});
 rejects([](auto &b){u16(b,0x20,0xffff);u16(b,0x22,0xffff);});
 rejects([](auto &b){u16(b,0x50726,0x481e);});
 rejects([](auto &b){u16(b,0x50724,0xffff);});
 rejects([](auto &b){b[0x54116]='?';});
 rejects([](auto &b){b[0x54109]=0;});
 rejects([](auto &b){b[0x51bbb]=0;});
 rejects([](auto &b){b[0x50aae]='%';b[0x50aaf]='n';});
 rejects([](auto &b){b[0x54108]=3;b[0x54109]='x';});
 rejects([](auto &b){b[0x54108]=9;b[0x54109]='x';});
 rejects([](auto &b){b[0x54108]=12;b[0x54109]='4';b[0x5410a]='0';});
 bool unknown=false;try {a.scalar("NO_FIELD");}catch(const std::out_of_range &){unknown=true;}
 check(unknown,"unknown field accepted");
}
void original(const char *root,const char *ui) {
 const auto installation=XeenInstallationDetector(ui && *ui?std::filesystem::path{ui}:std::filesystem::path{}).detect(root);
 check(installation && installation->uiModule,"required DOS UI module is missing");
 auto in=installation->uiModule->open();std::vector<std::uint8_t> b(in->size());
 check(in->read(b.data(),b.size())==b.size(),"DOS UI module short read");XeenDosText text(b);
 for(const auto &f:XeenDosText::layout()) {
  const auto &values=text.table(f.name);check(values.size()==f.count,"original table count");
  auto at=f.begin;
  for(unsigned i=0;i<f.count;++i) {
   if(f.pointers)at=0x4d890+(b[f.begin+i*4]|(unsigned(b[f.begin+i*4+1])<<8));
   std::size_t end=at;while(end<b.size() && b[end])++end;
   check(end<b.size() && values[i]==std::string(reinterpret_cast<const char *>(b.data()+at),end-at),"field bytes differ from DOS source");
   if(!f.pointers)at=end+1;
  }
 }
 const auto charges=text.scalar("FMT_CHARGES");
 check(charges.size()>3 && charges[0]==3 && charges[1]=='r' && charges[2]==9,"DOS Charges alignment must be single r");
}
}
int main(int argc,char **argv) {try {synthetic();if(argc>1)original(argv[1],argc>2?argv[2]:nullptr);std::cout<<"Bounded DOS fields, tables, controls and malformed layouts passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
