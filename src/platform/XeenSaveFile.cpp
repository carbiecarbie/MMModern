#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenSaveFormat.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <fstream>
#include <stdexcept>
#include <cwctype>
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
void XeenSaveFile::write(const Path &path, const XeenSaveSnapshot &snapshot, const Fault &fault) {
 const auto bytes = XeenSaveFormat::encode(snapshot); // Before any destination I/O.
 if (ordinary(path, true)) static_cast<void>(read(path));
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
XeenSaveResourceSignature XeenSaveFile::fingerprint(const GameInstallation &installation) {
 const auto hash = [](const Path &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("Cannot open original archive: " + path.u8string());
  return XeenSaveFormat::fingerprint(stream);
 };
 XeenSaveResourceSignature result;
 result.clouds = hash(installation.xeenArchive);
 if (installation.hasDarkside()) result.darkside = hash(installation.darkArchive);
 return result;
}
}
