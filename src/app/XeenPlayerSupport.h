#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace mmodern {
std::filesystem::path xeenPlayerDirectory();
std::filesystem::path xeenLogFile();
std::string xeenPrivatePaths(std::string);
std::string xeenPrivatePaths(std::string,const std::filesystem::path &localAppData,const std::filesystem::path &profile);
std::optional<std::filesystem::path> xeenPlayerCfg(const std::optional<std::filesystem::path> &explicitCfg={});
void xeenWriteOwnedCfg(const std::filesystem::path &,const std::vector<std::uint8_t> &);
void xeenResetOwnedCfg(const std::filesystem::path &);
std::string xeenDiagnostics(const std::filesystem::path &,const std::optional<std::filesystem::path> &cfg={},const std::filesystem::path &ui={});
int xeenSoundSetupCommand(const std::vector<std::string> &);
class XeenPlayerLog {
public:
 XeenPlayerLog();
 ~XeenPlayerLog();
private:
 struct Impl;std::unique_ptr<Impl> _impl;
};
}
