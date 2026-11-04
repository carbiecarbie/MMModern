#ifndef MMODERN_CORE_PATH_TEXT_H
#define MMODERN_CORE_PATH_TEXT_H
#include <filesystem>
#include <string>
namespace mmodern {
// Filesystem paths remain bytes on POSIX; diagnostics retain valid UTF-8 and
// spell invalid bytes as hexadecimal, without changing the path used for I/O.
inline std::string xeenDiagnosticText(const std::string &text) {
#ifdef _WIN32
 return text;
#else
 std::string result;
 constexpr char hex[]="0123456789abcdef";
 for(std::size_t i=0;i<text.size();) {
  const auto first=static_cast<unsigned char>(text[i]);
  unsigned count=first<0x80?1:first>=0xc2 && first<=0xdf?2:first>=0xe0 && first<=0xef?3:first>=0xf0 && first<=0xf4?4:0;
  bool valid=count && i+count<=text.size();
  for(unsigned j=1;valid && j<count;++j) {
   const auto next=static_cast<unsigned char>(text[i+j]);
   valid=next>=0x80 && next<=0xbf;
   if(j==1 && ((first==0xe0 && next<0xa0) || (first==0xed && next>=0xa0) ||
     (first==0xf0 && next<0x90) || (first==0xf4 && next>=0x90)))valid=false;
  }
  if(valid){result.append(text,i,count);i+=count;}
  else {result+="\\x";result+=hex[first>>4];result+=hex[first&15];++i;}
 }
 return result;
#endif
}
inline std::string xeenPathText(const std::filesystem::path &path) {return xeenDiagnosticText(path.u8string());}
}
#endif
