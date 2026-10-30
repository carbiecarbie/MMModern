#include "app/XeenPlayerSupport.h"
#include <windows.h>
#include <shlobj.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
namespace {
unsigned folderQueries=0;
struct Terminal:std::streambuf {
 std::string bytes;unsigned writes=0,characters=0;
 std::streamsize xsputn(const char *s,std::streamsize n) override {++writes;bytes.append(s,static_cast<std::size_t>(n));return n;}
 int overflow(int c) override {if(c!=traits_type::eof()){++characters;bytes+=char(c);}return c;}
};
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
}
extern "C" HRESULT __real_SHGetKnownFolderPath(REFKNOWNFOLDERID,DWORD,HANDLE,PWSTR *);
extern "C" HRESULT __wrap_SHGetKnownFolderPath(REFKNOWNFOLDERID id,DWORD flags,HANDLE token,PWSTR *path) {
 ++folderQueries;return __real_SHGetKnownFolderPath(id,flags,token,path);
}
int main(int argc,char **argv){try{
 check(argc==2 || (argc==3 && std::string(argv[2])=="--console"),"usage: mmodern_player_log_tests <scratch> [--console]");
 const bool console=argc==3;
 const auto root=std::filesystem::absolute(std::filesystem::u8path(argv[1]));
 std::filesystem::create_directories(root);const auto file=root/"mmodern.log";
 SetEnvironmentVariableW(L"MMODERN_LOG_FILE",file.c_str());
 Terminal terminal;auto *out=std::cout.rdbuf(),*err=std::cerr.rdbuf();
 if(!console){std::cout.rdbuf(&terminal);std::cerr.rdbuf(&terminal);}
 const std::string line="Actor 12 map=23 (Clouds) 5 0 HP=15 active=0 lifecycle=0 status=0 accounted=0\n";
 std::string dump;for(unsigned n=0;n<1000;++n)dump+=line;
 const auto start=std::chrono::steady_clock::now();
 unsigned before=0,after=0,writesBefore=0,writesAfter=0;
 bool lineFlushed=false;
 {
  mmodern::XeenPlayerLog log;before=folderQueries;
  writesBefore=terminal.writes;std::cout<<dump;after=folderQueries;writesAfter=terminal.writes;
  std::ifstream visible(file,std::ios::binary);const std::string current((std::istreambuf_iterator<char>(visible)),{});
  lineFlushed=current.find(dump)!=std::string::npos;
  std::cout<<"part"<<"ial"<<'\n';std::cerr<<"diagnostic"<<'\n';
 }
 const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
 std::cout.rdbuf(out);std::cerr.rdbuf(err);
 std::ifstream input(file,std::ios::binary);const std::string recorded((std::istreambuf_iterator<char>(input)),{});
 check(recorded.find(dump)!=std::string::npos,"Lost log lines");
 check(lineFlushed,"Complete lines were not flushed while the log was alive");
 check(recorded.find("partial\n")!=std::string::npos && recorded.find("diagnostic\n")!=std::string::npos,"Lost split/std::cerr line");
 check(after==before,"Repeated Windows folder lookups during gameplay logging");
 if(!console)check(writesAfter-writesBefore==1 && terminal.characters==0,"Bulk diagnostics degraded to character-at-a-time console writes");
 if(!console)check(terminal.bytes.find(dump)!=std::string::npos,"Changed terminal output");
 std::ostringstream metrics;metrics<<"1000 gameplay diagnostic lines: "<<elapsed<<" seconds; folder queries during messages="<<(after-before)
  <<"; terminal bulk writes="<<terminal.writes<<"; terminal character writes="<<terminal.characters<<'\n';
 std::ofstream(root/"metrics.txt")<<metrics.str();std::cout<<metrics.str();
 return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
