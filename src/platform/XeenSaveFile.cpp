#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <stdexcept>
#include <cwctype>
#include <array>
#include <sstream>
#include <iomanip>
#include <zlib.h>
namespace mmodern {
namespace {
using Path = std::filesystem::path;
std::runtime_error error(const char *operation) {
 return std::runtime_error(std::string(operation) + " failed (Windows error " + std::to_string(GetLastError()) + ")");
}
struct Handle {
 HANDLE value = INVALID_HANDLE_VALUE;
 ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
 void close() {
  if (!CloseHandle(value)) throw error("Close save file");
  value = INVALID_HANDLE_VALUE;
 }
};
bool ordinary(const Path &path, bool absent) {
 const DWORD attributes = GetFileAttributesW(path.c_str());
 if (attributes == INVALID_FILE_ATTRIBUTES) {
  if (absent && GetLastError() == ERROR_FILE_NOT_FOUND) return false;
  throw error("Inspect save file");
 }
 if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE | FILE_ATTRIBUTE_REPARSE_POINT))
  throw std::runtime_error("Save target must be an ordinary local file");
 return true;
}
std::wstring lower(std::wstring text) {
 std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) { return std::towlower(c); });
 return text;
}
bool localExtendedPath(const std::wstring &path) {
 return path.size() >= 7 && path.rfind(L"\\\\?\\", 0) == 0 &&
  ((path[4] >= L'A' && path[4] <= L'Z') || (path[4] >= L'a' && path[4] <= L'z')) &&
  path[5] == L':' && path[6] == L'\\';
}
Path resolvedDirectory(const Path &path) {
 Handle directory{CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
  nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr)};
 if (directory.value == INVALID_HANDLE_VALUE) throw error("Open save directory");
 BY_HANDLE_FILE_INFORMATION information{};
 if (!GetFileInformationByHandle(directory.value, &information)) throw error("Inspect save directory");
 if (!(information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
  throw std::runtime_error("Save parent directory must already exist");
 // Follow reparse points and let Windows normalize directory aliases. No file is created.
 std::wstring finalPath(32768, L'\0');
 const DWORD length = GetFinalPathNameByHandleW(directory.value, finalPath.data(),
  static_cast<DWORD>(finalPath.size()), FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
 if (!length) throw error("Resolve save directory");
 if (length >= finalPath.size()) throw std::runtime_error("Resolved save directory path is too long");
 finalPath.resize(length);
 directory.close();
 // Retain the extended prefix: removing it can change the directory accessed by later I/O.
 if (!localExtendedPath(finalPath))
  throw std::runtime_error("Save path must be on a local Windows filesystem");
 while (finalPath.size() > 7 && finalPath.back() == L'\\') finalPath.pop_back();
 const Path resolved(finalPath);
 if (GetDriveTypeW(finalPath.substr(4, 3).c_str()) == DRIVE_REMOTE)
  throw std::runtime_error("Save path must be on a local Windows filesystem");
 return resolved;
}
bool sameWindowsPath(const std::wstring &left, const std::wstring &right) {
 const int comparison = CompareStringOrdinal(left.data(), static_cast<int>(left.size()),
  right.data(), static_cast<int>(right.size()), TRUE);
 if (!comparison) throw error("Compare save directory paths");
 return comparison == CSTR_EQUAL;
}
}
std::filesystem::path XeenSaveFile::resolve(const Path &path, const Path &installation) {
 if (path.empty()) throw std::runtime_error("Save path is empty");
 const bool extended = localExtendedPath(path.native());
 // A previously resolved path (including one copied from save feedback) retains its semantics.
 const Path absolute = extended ? path : std::filesystem::absolute(path).lexically_normal();
 if (extended) {
  if (path.native().find(L'/') != std::wstring::npos)
   throw std::runtime_error("Extended save path requires native separators");
  for (const auto &component : path)
   if (component == L"." || component == L"..")
    throw std::runtime_error("Extended save path cannot contain relative components");
 }
 const auto leaf = lower(absolute.filename().wstring());
 const auto device = leaf.substr(0, leaf.find(L'.'));
 if (device == L"con" || device == L"prn" || device == L"aux" || device == L"nul" ||
   (device.size() == 4 && (device.substr(0, 3) == L"com" || device.substr(0, 3) == L"lpt") &&
    device[3] >= L'1' && device[3] <= L'9'))
  throw std::runtime_error("Save target cannot be a Windows device name");
 if (lower(absolute.extension().wstring()) != L".mmsave" ||
   (extended ? absolute.native().substr(7) : absolute.relative_path().wstring()).find(L':') != std::wstring::npos)
  throw std::runtime_error("Save path requires a .mmsave extension and no alternate stream");
 if ((!extended && absolute.native().rfind(L"\\\\", 0) == 0) ||
   GetDriveTypeW((extended ? absolute.native().substr(4, 3) : absolute.root_path().wstring()).c_str()) == DRIVE_REMOTE)
  throw std::runtime_error("Save path must be on a local Windows filesystem");
 const auto root = resolvedDirectory(installation).wstring();
 const Path parent = resolvedDirectory(absolute.parent_path());
 const auto target = parent.wstring();
 const auto prefix = root.back() == L'\\' ? root : root + L"\\";
 if (sameWindowsPath(target, root) ||
   (target.size() >= prefix.size() && sameWindowsPath(target.substr(0, prefix.size()), prefix)))
  throw std::runtime_error("Cannot save inside the commercial game installation");
 const Path resolved = parent / absolute.filename();
 ordinary(resolved, true);
 return resolved;
}
XeenSaveSnapshot XeenSaveFile::read(const Path &path) {
 ordinary(path, false);
 Handle file{CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
 if (file.value == INVALID_HANDLE_VALUE) throw error("Open save file");
 LARGE_INTEGER size{};
 if (!GetFileSizeEx(file.value, &size)) throw error("Read save size");
 if (size.QuadPart < 0 || size.QuadPart > XeenSaveFormat::kMaximumSize)
  throw std::runtime_error("Save file is oversized");
 std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size.QuadPart));
 std::size_t offset = 0;
 while (offset < bytes.size()) {
  DWORD count = 0;
  if (!ReadFile(file.value, bytes.data() + offset, static_cast<DWORD>(bytes.size() - offset), &count, nullptr))
   throw error("Read save file");
  if (!count) throw std::runtime_error("Save file was truncated while reading");
  offset += count;
 }
 file.close();
 return XeenSaveFormat::decode(bytes);
}
void XeenSaveFile::write(const Path &path, const XeenSaveSnapshot &snapshot, const Fault &fault, bool managed) {
 const auto bytes = XeenSaveFormat::encode(snapshot); // Before any destination I/O.
 if(managed && !snapshot.name)throw std::runtime_error("Managed save requires a name");
 // Serialize our publications across threads and MMModern processes. External
 // replacements after the final check and before rename remain outside scope.
 Handle mutex{CreateMutexW(nullptr,FALSE,L"Local\\MMModern-XeenSaveWriter")};
 if(mutex.value==nullptr) {mutex.value=INVALID_HANDLE_VALUE;throw error("Create save writer mutex");}
 const auto waited=WaitForSingleObject(mutex.value,INFINITE);
 if(waited!=WAIT_OBJECT_0 && waited!=WAIT_ABANDONED)throw error("Acquire save writer mutex");
 struct Unlock {HANDLE value;~Unlock(){ReleaseMutex(value);}} unlock{mutex.value};
 const auto validateTarget=[&] {
  if(ordinary(path,true)) {
   try {
    const auto previous=read(path);
    if(!(previous.resources==snapshot.resources))throw std::runtime_error("Save target belongs to different game data");
    if(managed && !previous.name)throw std::runtime_error("Managed target has no validated name");
   } catch(const XeenUnsupportedSave &e) {if(!e.recognizableOlder)throw;}
  }
 };
 validateTarget();
 const auto fails = [&](Operation op) { return fault && fault(op); };
 const auto require = [&](Operation op, const char *message) { if (fails(op)) throw std::runtime_error(message); };
 static std::atomic<unsigned long long> sequence{0};
 Path temporary;
 Handle file;
 bool created = false;
 try {
  require(Operation::Open, "Injected temporary open failure");
  for (unsigned attempt = 0; attempt < 100; ++attempt) {
   temporary = path;
   temporary += L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(++sequence);
   file.value = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
   if (file.value != INVALID_HANDLE_VALUE) { created = true; break; }
   if (GetLastError() != ERROR_FILE_EXISTS) throw error("Create temporary save");
  }
  if (!created) throw std::runtime_error("Cannot create a unique temporary save");
  std::size_t offset = 0;
  while (offset < bytes.size()) {
   require(Operation::Write, "Injected write failure");
   const DWORD requested = static_cast<DWORD>(std::min<std::size_t>(65536, bytes.size() - offset));
   DWORD count = 0;
   const DWORD actual = fails(Operation::ShortWrite) ? requested / 2 : requested;
   if (!WriteFile(file.value, bytes.data() + offset, actual, &count, nullptr)) throw error("Write save file");
   if (count != requested) throw std::runtime_error("Short save write");
   offset += count;
  }
  require(Operation::Flush, "Injected flush failure");
  if (!FlushFileBuffers(file.value)) throw error("Flush save file");
  file.close();
  require(Operation::Close, "Injected close failure");
  require(Operation::Replace, "Injected replacement failure");
  require(Operation::Revalidate, "Injected re-validation failure");
  validateTarget();
  if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
   throw error("Publish save file");
 } catch (const std::exception &failure) {
  std::string message = failure.what();
  if (file.value != INVALID_HANDLE_VALUE) {
   try { file.close(); } catch (const std::exception &e) { message += std::string("; ") + e.what(); }
  }
  if (created && (fails(Operation::Cleanup) || !DeleteFileW(temporary.c_str())))
   message += "; temporary cleanup failed: " + temporary.u8string();
  throw std::runtime_error(message);
 }
}
std::filesystem::path XeenSaveFile::resolve(const Path &path,const GameInstallation &installation) {
 auto result=resolve(path,installation.root);
 for(const auto &directory:installation.protectedDirectories)result=resolve(result,directory);
 // Manually assembled fixtures still protect all explicit archive paths.
 for(const auto &file:{installation.xeenArchive,installation.darkArchive})if(!file.empty())result=resolve(result,file.parent_path());
 if(installation.uiModule)result=resolve(result,installation.uiModule->physicalPath.parent_path());
 for(const auto &file:{installation.cloudsData,installation.darksideData,installation.introData})
  if(file)result=resolve(result,file->physicalPath.parent_path());
 return result;
}
XeenSaveResourceSignature XeenSaveFile::fingerprint(const GameInstallation &installation) {
 const auto hash=[](const ReadOnlyDataFile &file) {
  auto stream=file.open();std::array<std::uint8_t,65536> bytes{};XeenArchiveFingerprint result{};
  result.size=stream->size();uLong crc=crc32(0,nullptr,0);
  while(stream->pos()<stream->size()) { const auto count=stream->read(bytes.data(),bytes.size());crc=crc32(crc,bytes.data(),static_cast<uInt>(count)); }
  result.crc32=static_cast<std::uint32_t>(crc);return result;
 };
 XeenSaveResourceSignature result;result.clouds=hash(archiveDataFile(installation,XeenArchiveRole::Clouds));
 if(installation.hasDarkside())result.darkside=hash(archiveDataFile(installation,XeenArchiveRole::Darkside));return result;
}
std::filesystem::path XeenSaveFile::slotPath(const Path &directory,unsigned slot) {
 if(slot>=10)throw std::out_of_range("Save slot must be in 0..9");
 return directory/("slot-"+std::to_string(slot)+".mmsave");
}
XeenSaveFile::Slot XeenSaveFile::inspectSlot(const Path &path,const XeenSaveResourceSignature &signature) {
 Slot result;
 try {
  if(!ordinary(path,true))return result;
  auto snapshot=read(path);
  if(!(snapshot.resources==signature))throw std::runtime_error("Save belongs to different game data");
  if(!snapshot.name)throw std::runtime_error("Managed save has no validated name");
  result.snapshot=std::move(snapshot);result.state=Slot::State::Available;
 } catch(const XeenUnsupportedSave &e) {
  result.state=e.recognizableOlder ? Slot::State::Older : Slot::State::Protected;result.reason=e.what();
 } catch(const std::exception &e) {result.state=Slot::State::Protected;result.reason=e.what();}
 return result;
}
void XeenSaveFile::writeSlot(const Path &directory,unsigned slot,const XeenSaveSnapshot &snapshot,
 const GameInstallation &installation,const Path &repository,const Fault &fault,const std::function<void()> &check) {
 if(check)check();
 const auto target=resolve(resolve(slotPath(directory,slot),installation),repository);
 if(check)check();
 write(target,snapshot,[&](Operation op) {
  if(check)check();const bool fail=fault && fault(op);if(check)check();
  if(op==Operation::Revalidate && resolve(resolve(target,installation),repository)!=target)
   throw std::runtime_error("Managed save directory identity changed");
  return fail;
 },true);
 if(check)check();
}
std::filesystem::path XeenSaveFile::createSlotDirectory(const GameInstallation &installation,
 const Path &repository,const std::optional<Path> &override) {
 Path base;
 if(override)base=*override;
 else {
  PWSTR value=nullptr;
  const auto status=SHGetKnownFolderPath(FOLDERID_LocalAppData,0,nullptr,&value);
  if(FAILED(status))throw std::runtime_error("Resolve Local AppData failed (HRESULT "+std::to_string(status)+")");
  struct Free {PWSTR value;~Free(){CoTaskMemFree(value);}} free{value};
  base=value;
 }
 if(base.empty() || !base.is_absolute())throw std::runtime_error("Local AppData must be an absolute existing directory");
 // Reuse the path owner for every source, alias and link check. A synthetic
 // leaf performs no I/O and also checks the resolved repository boundary.
 const auto check=[&](const Path &directory) {
  auto probe=resolve(directory/"slot-directory-check.mmsave",installation);
  probe=resolve(probe,repository);
  return probe.parent_path();
 };
 base=check(base);
 const auto signature=fingerprint(installation);
 std::ostringstream key;key<<std::hex<<std::setfill('0')<<std::setw(16)<<signature.clouds.size
  <<'-'<<std::setw(8)<<signature.clouds.crc32;
 if(signature.darkside)key<<'-'<<std::setw(16)<<signature.darkside->size<<'-'<<std::setw(8)<<signature.darkside->crc32;
 else key<<"-no-darkside";
 for(const auto &component:std::vector<Path>{"MMModern","Saves",key.str()}) {
  const auto child=base/component;
  // The parent is resolved and protected before creation. Resolve an existing
  // child first so a junction into source data can never receive a new child.
  const auto attributes=GetFileAttributesW(child.c_str());
  if(attributes!=INVALID_FILE_ATTRIBUTES)base=check(child);
  else {
   if(GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND)throw error("Inspect managed save directory");
   if(!CreateDirectoryW(child.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS)throw error("Create managed save directory");
   base=check(child);
  }
 }
 return base;
}
}
