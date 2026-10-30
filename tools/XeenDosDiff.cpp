#include "tools/XeenDosDiff.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace mmodern::developer {
namespace {
DosBytes read(const std::filesystem::path &path) {
 std::ifstream f(path,std::ios::binary);
 if(!f) throw std::runtime_error("Cannot read DOS archive: "+path.u8string());
 return {std::istreambuf_iterator<char>(f),{}};
}
std::string suffix(const std::string &slot) {
 if(slot=="cur") return ".CUR";
 if(slot.empty() || slot.size()>2 || !std::all_of(slot.begin(),slot.end(),[](unsigned char c){return c>='0' && c<='9';}))
  throw std::invalid_argument("DOS slot must be 1..10 or cur");
 const unsigned n=std::stoul(slot);
 if(n<1 || n>10) throw std::invalid_argument("DOS slot must be 1..10 or cur");
 std::ostringstream out; out<<std::setw(2)<<std::setfill('0')<<n<<".WOX"; return out.str();
}
using Index=std::map<std::uint16_t,const DosMember *>;
Index index(const DosMembers &m) {Index result; for(const auto &v:m) if(!result.emplace(v.id,&v).second) throw std::invalid_argument("Duplicate DOS member ID"); return result;}
std::map<std::uint16_t,std::set<std::string>> names(const DosMembers &a,const DosMembers &b) {
 std::map<std::uint16_t,std::set<std::string>> result;
 for(const auto *members:{&a,&b}) for(const auto &m:*members) {
  if(!m.name.empty()) result[m.id].insert(m.name);
  for(const char *n:{"maze.nam","maze.chr","maze.pty"}) if(m.id==dosMemberId(n)) result[m.id].insert(n);
  if(m.bytes.size()==892) {
   unsigned id=m.bytes[768]|unsigned(m.bytes[769])<<8;
   const auto n=dosMapName(id,"dat");
   if(id && id<=999 && dosMemberId(n)==m.id) {
    result[m.id].insert(n);
    for(const char *ext:{"mob","evt"}) {const auto name=dosMapName(id,ext);result[dosMemberId(name)].insert(name);}
   }
  }
 }
 // Without a DAT payload, name a member from the canonical map names; a hash
 // collision lists every candidate.
 static const auto canonical=[] {
  std::map<std::uint16_t,std::set<std::string>> table;
  for(unsigned id=1;id<=255;++id) for(const char *ext:{"dat","mob","evt"}) {const auto name=dosMapName(id,ext);table[dosMemberId(name)].insert(name);}
  return table;
 }();
 for(const auto *members:{&a,&b}) for(const auto &m:*members) if(!result.count(m.id)) {
  const auto found=canonical.find(m.id); if(found!=canonical.end()) result[m.id]=found->second;
 }
 return result;
}
bool markers(const DosExportPair &pair,const char *label,std::ostream &out) {
 const auto n=dosMemberId("maze.nam"); auto a=index(pair.clouds), b=index(pair.darkside);
 if(!a.count(n) || !b.count(n) || a[n]->bytes.size()<=30 || b[n]->bytes.size()<=30) {
  out<<label<<": missing or truncated NAM marker\n"; return false;
 }
 const auto x=a[n]->bytes[30], d=b[n]->bytes[30];
 if(!x || !d || x==255 || d==255 || x!=d) {
  out<<label<<": invalid pair markers "<<unsigned(x)<<"/"<<unsigned(d)<<"\n"; return false;
 }
 return true;
}
}
DosExportPair readDosPair(const std::filesystem::path &dir,const std::string &slot) {
 const auto s=suffix(slot); return {readDosArchive(read(dir/("XEEN"+s))),readDosArchive(read(dir/("DARK"+s)))};
}
bool diffDosPairs(const DosExportPair &a,const DosExportPair &b,bool resaveCheck,std::ostream &out) {
 bool ok=markers(a,"A",out); ok=markers(b,"B",out)&&ok;
 for(unsigned side=0;side<2;++side) {
  const auto &am=side?a.darkside:a.clouds, &bm=side?b.darkside:b.clouds;
  const auto ai=index(am), bi=index(bm);
  const auto known=names(am,bm);
  std::map<std::uint16_t,bool> ids; for(const auto &v:ai)ids[v.first]=true; for(const auto &v:bi)ids[v.first]=true;
  for(const auto &entry:ids) {
   const auto id=entry.first; auto name=known.find(id);
   std::ostringstream label; label<<(side?"DARK":"XEEN")<<" ID 0x"<<std::hex<<std::uppercase<<std::setw(4)<<std::setfill('0')<<id<<" ";
   if(name==known.end()) label<<"<unknown>";
   else {bool first=true;for(const auto &n:name->second) {if(!first)label<<" / ";label<<n;first=false;}}
   auto x=ai.find(id), y=bi.find(id);
   if(x==ai.end() || y==bi.end()) {out<<label.str()<<": "<<(x==ai.end()?"added":"missing")<<" member\n";ok=false;continue;}
   const auto &xb=x->second->bytes,&yb=y->second->bytes;
   if(xb.size()!=yb.size()) {out<<label.str()<<": length "<<xb.size()<<" -> "<<yb.size()<<"\n";ok=false;}
   for(std::size_t i=0;i<std::min(xb.size(),yb.size());++i) if(xb[i]!=yb[i]) {
    out<<label.str()<<" offset "<<i<<": "<<unsigned(xb[i])<<" -> "<<unsigned(yb[i])<<"\n";
    const bool allowed=(id==dosMemberId("maze.nam") && i<29) || (side && id==dosMemberId("maze.pty") && i>=654 && i<658);
    if(resaveCheck && !allowed) ok=false;
   }
   for(std::size_t i=std::min(xb.size(),yb.size());i<std::max(xb.size(),yb.size());++i)
    out<<label.str()<<" offset "<<i<<": "<<(i<xb.size()?std::to_string(xb[i]):"<missing>")<<" -> "<<(i<yb.size()?std::to_string(yb[i]):"<missing>")<<"\n";
  }
 }
 if(resaveCheck) out<<(ok?"Re-save check passed\n":"Re-save check failed\n");
 return ok;
}
}
