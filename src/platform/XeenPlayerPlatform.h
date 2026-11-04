#ifndef MMODERN_PLATFORM_XEEN_PLAYER_PLATFORM_H
#define MMODERN_PLATFORM_XEEN_PLAYER_PLATFORM_H
#include <filesystem>
#include <string>
#include <ctime>
#include <fstream>
namespace mmodern {
std::string windowsVersion();
void replacePath(std::string &,const std::filesystem::path &,const char *);
void xeenOpenLog(std::ofstream &);
void xeenLocalTime(std::tm &,const std::time_t &);
}
#endif
