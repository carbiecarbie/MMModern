#ifndef MMODERN_CHILD_PROCESS_TEST_SUPPORT_H
#define MMODERN_CHILD_PROCESS_TEST_SUPPORT_H
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <cerrno>
extern char **environ;
#endif
#include <chrono>
#include <thread>
#include <sstream>
#include <charconv>
#include "../src/core/PathText.h"
#include "TestTreeRemoval.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <algorithm>
#ifndef MMODERN_TEST_UI_DATA
#define MMODERN_TEST_UI_DATA ""
#endif
namespace child_test {
namespace fs = std::filesystem;
struct Result { unsigned exit; std::string output; unsigned pid; std::uint64_t created; };
inline void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
inline unsigned processId() {
#ifdef _WIN32
 return GetCurrentProcessId();
#else
 return static_cast<unsigned>(getpid());
#endif
}
inline std::uint64_t ticks() {
 return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
// Retain prior evidence even when a PID is reused.
inline fs::path freshDirectory(const fs::path &prefix) {
 auto run=prefix;run+="-"+std::to_string(processId())+"-"+std::to_string(ticks())+"-";
 for(std::uint64_t attempt=0;;++attempt) {
  auto path=run;path+=std::to_string(attempt);
  if(fs::create_directory(path))return path;
 }
}
// Call only on success: a failing run throws or returns first and keeps its
// working directory as evidence. Removal problems never fail a passed test.
// Another process (a virus scanner, the search indexer) may briefly hold a
// file the run has just written; removeTestTree retries for a while.
inline void removePassedRun(const fs::path &directory) {
 if (!removeTestTree(directory)) std::cerr << "could not remove passed run " << mmodern::xeenPathText(directory) << '\n';
}
#ifdef _WIN32
struct Window { DWORD pid; HWND handle = nullptr; };
inline BOOL CALLBACK findWindow(HWND window, LPARAM data) {
 auto &target = *reinterpret_cast<Window *>(data); DWORD pid = 0;
 GetWindowThreadProcessId(window, &pid);
 wchar_t title[512]{}; GetWindowTextW(window, title, 512);
 if (pid == target.pid && std::wstring(title).find(L"MMModern - Map") == 0) target.handle = window;
 return TRUE;
}
#endif
// Bounded child launch extracted from XeenSaveCliTests. Used only by tests;
// no process framework or game-state transport.
// The timeout only detects a hung child; it is generous so that parallel CTest
// jobs never turn machine load into a failure. Each exit line reports the
// elapsed time, so a run log shows where a process test spends it.
inline Result launch(const fs::path &exe, const std::vector<std::string> &args,
 const fs::path &log, bool closeNativeWindow = false, bool escapeExits = false, unsigned timeoutMs = 300000,
 bool isolateAudio = true, bool supplyGameUi = true) {

 auto launchArgs=args;
 struct Fixture {fs::path path;~Fixture(){if(!path.empty()){std::error_code ec;fs::remove(path,ec);}}} fixture;
 const auto has=[&](const char *option){return std::find(args.begin(),args.end(),option)!=args.end();};
 const bool gameplay=has("--new-game") || has("--journey-region") || has("--load-game") ||
  has("--event-fixture") || has("--monster-fixture") ||
  (!args.empty() && args[0].rfind("--",0)!=0 && fs::is_directory(fs::u8path(args[0])));
 if(isolateAudio && gameplay && !has("--xeen-cfg")) {
  // Process tests must not follow a player's installed MT-32 selection to
  // Local AppData. Use a synthetic installed 3/10 configuration instead.
  fixture.path=log;fixture.path+=".xeen.cfg";
  std::array<char,29> cfg{};cfg[0]=10;cfg[2]=0x20;cfg[3]=2;cfg[4]=3;
  cfg[6]=0x20;cfg[7]=2;cfg[8]=7;cfg[20]=1;cfg[24]=1;
  std::ofstream file(fixture.path,std::ios::binary|std::ios::trunc);
  file.write(cfg.data(),cfg.size());file.close();require(bool(file),"Could not write isolated process audio configuration");
  launchArgs.push_back("--xeen-cfg");launchArgs.push_back(fixture.path.u8string());
 }
 const fs::path ui(MMODERN_TEST_UI_DATA);
 const bool entry=!args.empty() && (args[0]=="--new-game" || args[0]=="--journey-region" ||
  args[0]=="--load-game" || args[0]=="--render-map" || args[0]=="--inspect-map" ||
  args[0]=="--inspect-party" || args[0]=="--inspect-events" ||
  (args[0].rfind("--",0)!=0 && fs::is_directory(fs::u8path(args[0]))));
 if(supplyGameUi && entry && !ui.empty() && std::find(args.begin(),args.end(),"--ui-data")==args.end()) {
  launchArgs.push_back("--ui-data");launchArgs.push_back(ui.u8string());
 }
#ifdef _WIN32
 std::wstring command=L"\""+exe.wstring()+L"\"";
 for(const auto &bytes:launchArgs) {
  const auto arg=fs::u8path(bytes).wstring();
  require(arg.find(L'"')==std::wstring::npos && (arg.empty() || arg.back()!=L'\\'), "unsupported test argument quoting");
  command+=L" \""+arg+L"\"";
 }
 SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
 HANDLE out = CreateFileW(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
 require(out != INVALID_HANDLE_VALUE, "child log exists or cannot be created");
 STARTUPINFOW startup{}; startup.cb = sizeof(startup);
 startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE;
 startup.hStdOutput = out; startup.hStdError = out; startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
 PROCESS_INFORMATION process{};
 // The real CLI has no test-only exit switch. Give only this child the Windows
 // SDL backend, then send its own window WM_CLOSE and require normal exit.
 wchar_t previous[1024]{};
 const auto previousSize = GetEnvironmentVariableW(L"SDL_VIDEODRIVER", previous, 1024);
 require(previousSize < 1024, "SDL environment too long");
 if (closeNativeWindow) SetEnvironmentVariableW(L"SDL_VIDEODRIVER", L"windows");
 const bool ok = CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, TRUE,
  CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process);
 if (closeNativeWindow) SetEnvironmentVariableW(L"SDL_VIDEODRIVER", previousSize ? previous : nullptr);
 CloseHandle(out); require(ok, "child process launch failed");
 std::cout << "PID " << process.dwProcessId << " command " << fs::path(command).u8string() << '\n' << std::flush;
 const auto began = GetTickCount64(); DWORD wait = WAIT_TIMEOUT; bool closed = false;
 unsigned inputStage=0; ULONGLONG lastInput=0;
 while ((wait = WaitForSingleObject(process.hProcess, 50)) == WAIT_TIMEOUT && GetTickCount64() - began < timeoutMs) {
  if (closeNativeWindow && !closed && GetTickCount64() - began > 1000) {
   Window target{process.dwProcessId}; EnumWindows(findWindow, reinterpret_cast<LPARAM>(&target));
   if (target.handle && GetTickCount64()-lastInput>=150) {
    // Normal executable input: open, inspect another owner, close, then quit.
    // Exact live ownership is asserted by the independent instrumented consumer.
    const WPARAM keys[]{'I',VK_F2,'1',VK_ESCAPE};
    if(inputStage<4) {
     const auto key=keys[inputStage++];
     const LPARAM scan=static_cast<LPARAM>(MapVirtualKeyW(static_cast<UINT>(key),MAPVK_VK_TO_VSC))<<16;
     require(PostMessageW(target.handle,WM_KEYDOWN,key,scan|1),"CLI inventory keydown");
     require(PostMessageW(target.handle,WM_KEYUP,key,scan|1|(1ULL<<30)|(1ULL<<31)),"CLI inventory keyup");
     lastInput=GetTickCount64();
    } else closed = PostMessageW(target.handle, WM_CLOSE, 0, 0);
   }
  }
 }
 if (wait != WAIT_OBJECT_0) { TerminateProcess(process.hProcess, 99); WaitForSingleObject(process.hProcess, 2000); }
 DWORD code = 99; const bool gotCode = GetExitCodeProcess(process.hProcess, &code);
 FILETIME created{},exited{},kernel{},user{};
 const bool gotTimes=GetProcessTimes(process.hProcess,&created,&exited,&kernel,&user);
 const std::uint64_t creation=(std::uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime;
 const DWORD pid = process.dwProcessId; CloseHandle(process.hThread); CloseHandle(process.hProcess);
 std::cout << "PID " << pid << " exit " << code << " wait " << wait << " elapsed " << (GetTickCount64() - began) << " ms\n" << std::flush;
 require(wait == WAIT_OBJECT_0 && gotCode && gotTimes && creation, "child failed/timed out; acceptance stopped");
 require(!closeNativeWindow || closed || (escapeExits && inputStage==4 && code==0),
  "CLI did not expose and normally close its gameplay window");
 require(!closeNativeWindow || inputStage==4,"CLI inventory input sequence incomplete");
#else
 require(!closeNativeWindow && !escapeExits,"native-window input is Windows-only");
 const int out=open(log.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0666);
 require(out>=0,"child log exists or cannot be created");
 posix_spawn_file_actions_t actions;
 int error=posix_spawn_file_actions_init(&actions);
 if(error){close(out);throw std::runtime_error("child file actions initialization failed");}
 error=posix_spawn_file_actions_adddup2(&actions,out,STDOUT_FILENO);
 if(!error)error=posix_spawn_file_actions_adddup2(&actions,out,STDERR_FILENO);
 if(!error && out!=STDOUT_FILENO && out!=STDERR_FILENO)error=posix_spawn_file_actions_addclose(&actions,out);
 std::string executable=exe.native();
 std::vector<char *> argv{executable.data()};
 for(auto &arg:launchArgs)argv.push_back(arg.data());
 argv.push_back(nullptr);pid_t child=0;
 if(!error)error=posix_spawn(&child,exe.c_str(),&actions,nullptr,argv.data(),environ);
 posix_spawn_file_actions_destroy(&actions);close(out);
 require(!error,"child process launch failed");
 std::cout<<"PID "<<child<<" command "<<mmodern::xeenPathText(exe);
 for(const auto &arg:launchArgs)std::cout<<" \""<<mmodern::xeenDiagnosticText(arg)<<"\"";
 std::cout<<'\n'<<std::flush;
 const auto began=ticks();bool timedOut=false;siginfo_t info{};
 for(;;) {
  int result;do{result=waitid(P_PID,child,&info,WEXITED|WNOWAIT|WNOHANG);}while(result<0 && errno==EINTR);
  if(result<0){kill(child,SIGKILL);while(waitpid(child,nullptr,0)<0 && errno==EINTR){};throw std::runtime_error("child wait failed");}
  if(info.si_pid)break;
  if(ticks()-began>=timeoutMs) {
   timedOut=true;kill(child,SIGKILL);
   do{result=waitid(P_PID,child,&info,WEXITED|WNOWAIT);}while(result<0 && errno==EINTR);
   break;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
 }
 // A zombie retains /proc identity. Field 2 (comm) may contain spaces and ')'.
 std::ifstream stat("/proc/"+std::to_string(child)+"/stat");std::string record;std::getline(stat,record);
 std::uint64_t creation=0;const auto end=record.rfind(')');
 if(end!=std::string::npos) {
  std::istringstream fields(record.substr(end+2));std::string field;
  for(unsigned n=3;n<=22 && fields>>field;++n)if(n==22){const auto parsed=std::from_chars(field.data(),field.data()+field.size(),creation);if(parsed.ec!=std::errc{} || parsed.ptr!=field.data()+field.size())creation=0;}
 }
 int status=0;pid_t reaped;do{reaped=waitpid(child,&status,0);}while(reaped<0 && errno==EINTR);
 const unsigned pid=child,code=WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);
 std::cout<<"PID "<<pid<<" exit "<<code<<" elapsed "<<ticks()-began<<" ms\n"<<std::flush;
 require(!timedOut && reaped==child && creation,"child failed/timed out; acceptance stopped");
#endif
 std::ifstream input(log); require(bool(input), "child log read failed");
 return {code, std::string(std::istreambuf_iterator<char>(input), {}), pid, creation};
}
}
#endif
