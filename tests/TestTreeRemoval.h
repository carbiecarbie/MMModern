#pragma once
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// Removes a test's working tree; true when nothing remains. Tests use this
// instead of std::filesystem::remove_all: on Windows, libstdc++'s remove_all
// loops forever on an entry that vanishes or is delete-pending between listing
// and removal (a virus scanner or the search indexer briefly holding a file a
// test has just written), which hung xeen_save_file to its CTest timeout.
// Each pass lists a directory completely before removing its entries, removes
// links (junctions) without following them, and the passes are bounded.
#ifdef _WIN32
namespace test_tree_detail {
inline void removeOnce(const std::wstring &path) {
 const DWORD attributes = GetFileAttributesW(path.c_str());
 if (attributes == INVALID_FILE_ATTRIBUTES) return;
 if ((attributes & FILE_ATTRIBUTE_DIRECTORY) && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
  std::vector<std::wstring> children;
  WIN32_FIND_DATAW data{};
  const HANDLE search = FindFirstFileW((path + L"\\*").c_str(), &data);
  if (search != INVALID_HANDLE_VALUE) {
   do {
    const std::wstring name(data.cFileName);
    if (name != L"." && name != L"..") children.push_back(name);
   } while (FindNextFileW(search, &data));
   FindClose(search);
  }
  for (const auto &child : children) removeOnce(path + L"\\" + child);
  RemoveDirectoryW(path.c_str());
 } else if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
  RemoveDirectoryW(path.c_str()); // A directory link: remove the link only.
 } else {
  if (attributes & FILE_ATTRIBUTE_READONLY) SetFileAttributesW(path.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
  DeleteFileW(path.c_str());
 }
}
}
inline bool removeTestTree(const std::filesystem::path &root) noexcept {
 try {
  if (root.empty()) return true;
  // The extended prefix keeps literal names such as trailing-dot fixtures.
  std::wstring path = std::filesystem::absolute(root).lexically_normal().make_preferred().wstring();
  while (path.size() > 3 && path.back() == L'\\') path.pop_back();
  if (path.rfind(L"\\\\", 0) != 0) path = L"\\\\?\\" + path;
  for (int pass = 0; pass < 50; ++pass) {
   test_tree_detail::removeOnce(path);
   if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES &&
     (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND))
    return true;
   Sleep(100);
  }
  return false;
 } catch (...) {
  return false;
 }
}
#else
inline bool removeTestTree(const std::filesystem::path &root) noexcept {
 std::error_code error;
 std::filesystem::remove_all(root, error);
 return !error && !std::filesystem::exists(root, error);
}
#endif
