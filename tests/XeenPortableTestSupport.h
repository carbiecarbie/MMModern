#pragma once
#include <filesystem>
#include <cstdlib>
#include <optional>
#include "XeenChildProcessTestSupport.h"
namespace portable_test {
inline bool environment(const char *name,const std::optional<std::filesystem::path> &value) {
#ifdef _WIN32
 return SetEnvironmentVariableW(std::filesystem::u8path(name).c_str(),value?value->c_str():nullptr)!=0;
#else
 return value?setenv(name,value->c_str(),1)==0:unsetenv(name)==0;
#endif
}
inline std::optional<std::filesystem::path> environment(const char *name) {
#ifdef _WIN32
 const auto key=std::filesystem::u8path(name);const auto size=GetEnvironmentVariableW(key.c_str(),nullptr,0);
 if(!size)return {};std::wstring text(size,L'\0');GetEnvironmentVariableW(key.c_str(),text.data(),size);text.resize(size-1);return text;
#else
 const auto *text=std::getenv(name);return text?std::optional<std::filesystem::path>{text}:std::nullopt;
#endif
}
struct Environment {
 const char *name;std::optional<std::filesystem::path> old;
 explicit Environment(const char *key):name(key),old(environment(key)){}
 ~Environment(){environment(name,old);}
};
}

namespace portable_test {
template<class Char> inline bool setEnvironment(const char *name,const Char *value) {
 return environment(name,value?std::optional<std::filesystem::path>{std::filesystem::path(value)}:std::nullopt);
}
inline bool setEnvironment(const char *name,std::nullptr_t){return environment(name,std::nullopt);}
}
