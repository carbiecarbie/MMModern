#ifndef MMODERN_XEEN_CUTSCENE_DATA_H
#define MMODERN_XEEN_CUTSCENE_DATA_H
#include "core/GameInstallation.h"
#include <array>
#include <string>
#include <vector>
namespace mmodern {
// Runtime data from the calibrated CD EXE. No original text/tables are embedded.
struct XeenCutsceneData {
 enum Resource {Scratch,LogoPalette,Song,LogoBack,Logo,LogoTail,Fire,Growl,Jvc,JvcPalette,
  Stars,Title,Lake,Xeen,Tower,Wizard,LakeFlight,LakeDescent,Burst,ScenePalette,Blank,
  GroupBack,Group,Crodo,Caption,Box,ResourceCount};
 std::array<std::string,ResourceCount> resources;
 std::array<std::string,14> voices;
 std::array<std::uint8_t,248> mouths;
 std::array<std::uint16_t,248> waits;
 std::array<std::uint8_t,14> rows;
 std::array<std::uint8_t,16> towerLeft,towerRight,towerY;
 std::string labels,captionFormat,handPattern,marblePattern;
 unsigned digitalDevice=0;
 static XeenCutsceneData installed(const GameInstallation &);
 static XeenCutsceneData parse(const std::vector<std::uint8_t> &exe,const std::vector<std::uint8_t> &cfg);
 static std::string firstCaption(const std::vector<std::uint8_t> &);
 std::string hand(unsigned) const;
 std::string marble(unsigned) const;
};
}
#endif
