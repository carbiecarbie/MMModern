#include <algorithm>
#include <array>
#include <fstream>
#include <string>
#include <vector>
int main(int argc,char **argv) {
 if(argc!=3)return 1;
 const std::string mode=argv[2];
 const std::array<unsigned,6> counts{7,41,14,11,22,74};
 std::vector<unsigned char> block;
 const auto token=[&](const std::string &s){block.insert(block.end(),s.begin(),s.end());block.push_back(0);};
 token(mode=="oversized"?std::string(64,'x'):mode=="empty-scalar"?"":"\01432broken ");token("\01409cursed ");token("of ");
 std::array<std::vector<std::string>,6> tables;
 std::size_t size=block.size()+24;
 for(unsigned t=0;t<6;++t)for(unsigned i=0;i<counts[t];++i){auto s=i?"t"+std::to_string(t)+"-"+std::to_string(i):"";size+=s.size()+1;tables[t].push_back(s);}
 auto remaining=22438-20680-size;
 for(auto &table:tables)for(unsigned i=1;i<table.size() && remaining;++i){const auto add=std::min<std::size_t>(63-table[i].size(),remaining);table[i].append(add,'x');remaining-=add;}
 for(unsigned t=0;t<6;++t) {
  block.insert(block.end(),{0,0,0,static_cast<unsigned char>(mode=="bad-count" && !t?8:counts[t])});
  for(unsigned i=0;i<tables[t].size();++i)token(mode=="reserved" && !t && !i?"bad":mode=="required" && !t && i==1?"":tables[t][i]);
 }
 if(mode=="end-offset")block.push_back(1); // Extent also checked by a missing final terminator.
 std::vector<unsigned char> bytes(mode=="truncated"?35064:35065);
 std::copy(block.begin(),block.end(),bytes.begin()+20680);
 if(mode=="end-offset")bytes[22437]=1;
 std::ofstream file(argv[1],std::ios::binary);file.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());return file?0:1;
}
