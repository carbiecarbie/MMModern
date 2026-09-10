#ifndef MMODERN_CHILD_PROCESS_TEST_SUPPORT_H
#define MMODERN_CHILD_PROCESS_TEST_SUPPORT_H
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace child_test {
namespace fs = std::filesystem;
struct Result { DWORD exit; std::string output; DWORD pid; };
inline void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
struct Window { DWORD pid; HWND handle = nullptr; };
inline BOOL CALLBACK findWindow(HWND window, LPARAM data) {
 auto &target = *reinterpret_cast<Window *>(data); DWORD pid = 0;
 GetWindowThreadProcessId(window, &pid);
 wchar_t title[512]{}; GetWindowTextW(window, title, 512);
 if (pid == target.pid && std::wstring(title).find(L"MMModern - Map") == 0) target.handle = window;
 return TRUE;
}
// Bounded Win32 launch extracted from XeenSaveCliTests. Used only by tests;
// no process framework, game-state transport or application-specific startup.
inline Result launch(const fs::path &exe, const std::vector<std::wstring> &args,
 const fs::path &log, bool closeNativeWindow = false) {
 std::wstring command = L"\"" + exe.wstring() + L"\"";
 for (const auto &arg : args) {
  require(arg.find(L'"') == std::wstring::npos && (arg.empty() || arg.back() != L'\\'), "unsupported test argument quoting");
  command += L" \"" + arg + L"\"";
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
 while ((wait = WaitForSingleObject(process.hProcess, 50)) == WAIT_TIMEOUT && GetTickCount64() - began < 30000) {
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
 const DWORD pid = process.dwProcessId; CloseHandle(process.hThread); CloseHandle(process.hProcess);
 std::cout << "PID " << pid << " exit " << code << " wait " << wait << '\n' << std::flush;
 require(wait == WAIT_OBJECT_0 && gotCode, "child failed/timed out; acceptance stopped");
 require(!closeNativeWindow || closed, "CLI did not expose its gameplay window");
 require(!closeNativeWindow || inputStage==4,"CLI inventory input sequence incomplete");
 std::ifstream input(log); require(bool(input), "child log read failed");
 return {code, std::string(std::istreambuf_iterator<char>(input), {}), pid};
}
}
#endif
