#include "platform/XeenFileOperations.h"
#include <string>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif
namespace mmodern {
bool xeenIsOutOfSpace(const std::exception &error) {
 const std::string message=error.what();
#ifdef _WIN32
 return message.find("Windows error 112")!=std::string::npos || message.find("Windows error 39")!=std::string::npos;
#else
 for(const auto code:{ENOSPC,EDQUOT})if(message.find("(POSIX error "+std::to_string(code)+")")!=std::string::npos)return true;
 return false;
#endif
}
#ifndef _WIN32
void xeenSyncDirectory(int fd) {
 int result;
 do {result=::fsync(fd);}while(result<0 && errno==EINTR);
 const int code=errno;
 if(result<0 && code!=EINVAL && code!=ENOTSUP && code!=ENOSYS)
  throw std::runtime_error("Publish save file failed (POSIX error "+std::to_string(code)+")");
}
void xeenSyncDirectory(const std::filesystem::path &path) {
 int fd;
 do {fd=::open(path.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);}while(fd<0 && errno==EINTR);
 if(fd<0) {
  const int code=errno;
  throw std::runtime_error("Open save directory failed (POSIX error "+std::to_string(code)+")");
 }
 try{xeenSyncDirectory(fd);}catch(...){::close(fd);throw;}
 ::close(fd);
}
#endif
bool xeenReplaceSmallFile(const std::filesystem::path &temporary,const std::filesystem::path &target) {
#ifdef _WIN32
 return MoveFileExW(temporary.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
 if(::rename(temporary.c_str(),target.c_str())<0)return false;
 try{xeenSyncDirectory(target.parent_path());return true;}catch(...){return false;}
#endif
}
}
