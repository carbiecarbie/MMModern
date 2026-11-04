#include "XeenPortableTestSupport.h"
#ifdef _WIN32
#include <shellapi.h>
#endif
int main(int argc,char **argv){try {
 using namespace child_test;
 if(argc>1 && std::string(argv[1])=="--child") {
#ifdef _WIN32
  int count=0;auto *raw=CommandLineToArgvW(GetCommandLineW(),&count);
  const bool correct=raw && count==3 && fs::path(raw[2]).u8string()==u8"space ç 游";
  if(raw)LocalFree(raw);
  require(correct,"UTF-8 argv changed");
#else
  require(argc==3 && std::string(argv[2])==u8"space ç 游","UTF-8 argv changed");
#endif
  const auto *value=std::getenv("MMODERN_PROCESS_TEST_VALUE");
  require(std::getenv("SDL_VIDEODRIVER") && std::string(std::getenv("SDL_VIDEODRIVER"))=="dummy" && std::getenv("SDL_AUDIODRIVER") && std::string(std::getenv("SDL_AUDIODRIVER"))=="dummy","SDL environment not inherited");
  require(value && std::string(value)=="inherited","environment not inherited");
  std::cout<<"stdout inherited\n";std::cerr<<"stderr inherited\n";return 33;
 }
 if(argc>1 && std::string(argv[1])=="--hang") {std::this_thread::sleep_for(std::chrono::seconds(30));return 0;}
 const auto directory=freshDirectory(fs::current_path()/"child-process-tests");
 portable_test::Environment video("SDL_VIDEODRIVER"),audio("SDL_AUDIODRIVER");
 require(portable_test::setEnvironment("SDL_VIDEODRIVER","dummy") && portable_test::setEnvironment("SDL_AUDIODRIVER","dummy"),"set SDL environment");
 portable_test::Environment restore("MMODERN_PROCESS_TEST_VALUE");
 require(portable_test::setEnvironment("MMODERN_PROCESS_TEST_VALUE","inherited"),"set inherited environment");
 const auto exe=fs::absolute(fs::u8path(argv[0]));
 const auto result=launch(exe,{"--child",u8"space ç 游"},directory/"child.log");
 require(result.exit==33 && result.pid && result.created && result.output.find("stdout inherited")!=std::string::npos && result.output.find("stderr inherited")!=std::string::npos,"child result evidence");
 const auto second=launch(exe,{"--child",u8"space ç 游"},directory/"second.log");
 require(second.exit==33 && (result.pid!=second.pid || result.created!=second.created),"two launches reused a process incarnation");
 bool refused=false;try{launch(exe,{"--child",u8"space ç 游"},directory/"child.log");}catch(const std::exception &){refused=true;}
 require(refused,"existing log overwritten");
 std::ifstream retained(directory/"child.log");require(std::string(std::istreambuf_iterator<char>(retained),{})==result.output,"exclusive log bytes changed");
 retained.close();
 std::ostringstream evidence;auto *console=std::cout.rdbuf(evidence.rdbuf());
 bool timedOut=false;try{launch(exe,{"--hang"},directory/"timeout.log",false,false,100);}catch(const std::exception &){timedOut=true;}
 std::cout.rdbuf(console);std::cout<<evidence.str();
 require(timedOut,"hung child not killed");
 std::istringstream identity(evidence.str());std::string label;unsigned killed=0;identity>>label>>killed;
 require(label=="PID" && killed,"timeout has no child PID evidence");
#ifdef _WIN32
 // Windows has no zombie state; a retained kernel object must be terminated.
 const auto process=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,killed);
 if(process) {
  DWORD code=0;const bool stopped=WaitForSingleObject(process,0)==WAIT_OBJECT_0 && GetExitCodeProcess(process,&code) && code==99;
  CloseHandle(process);require(stopped,"timed-out child still running");
 } else require(GetLastError()==ERROR_INVALID_PARAMETER,"cannot establish terminated child identity");
#else
 errno=0;require(::waitpid(static_cast<pid_t>(killed),nullptr,WNOHANG)==-1 && errno==ECHILD,"timed-out child was not reaped");
 errno=0;require(::kill(static_cast<pid_t>(killed),0)==-1 && errno==ESRCH,"timed-out PID still exists");
 require(!fs::exists("/proc/"+std::to_string(killed)),"timed-out child left a zombie");
 require(evidence.str().find("exit 137")!=std::string::npos,"timeout did not terminate with SIGKILL");
#endif
 removePassedRun(directory);std::cout<<"Portable process argv, environment, identity, exclusive logs and timeout passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
