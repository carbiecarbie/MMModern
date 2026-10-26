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
 struct Ending {
  enum Resource {Prec,Blank,StormPalette,AlamarPalette,XeenLaugh,Dark1,Dark2,Dark3,DarkLaugh,
   Song,Palette,Later,EndText,King,Room,Mirror,Sky,People,Crodo,KingCrodo,MirrorBack,King4,
   Throne1,Throne2,Box,MirrorPalette,MirrorRaw,Count};
  std::array<std::string,Count> resources;
  std::array<std::string,41> prec;
  std::array<std::string,20> vortex;
  std::array<std::string,6> castleTop,castleBottom,powers;
  std::array<std::string,3> darkLord,kingVoices;
  std::array<std::uint8_t,6> castleFrames;
  std::array<std::int16_t,13> kingLeft,kingRight,kingY;
  std::array<std::int16_t,9> mirrorX,mirrorY;
  std::array<std::uint8_t,42> stormFm{};
  // Tables indexed exactly as their EXE callers: destruction uses the next
  // vortex index, the other three use that incremented/wrapped index minus one.
  std::array<std::array<std::uint8_t,20>,4> vortexFm{};
  std::array<std::uint16_t,3> randomWords;
  struct Actor {
   std::uint8_t selector=0;
   std::array<std::uint8_t,40> metadata;
   std::string idle,attack,voice;
  };
  std::array<Actor,73> parade;
 } ending;
 struct Captions {
  std::vector<std::uint16_t> offsets;
  std::vector<std::string> records;
  const std::string &record(unsigned index,unsigned copyBound) const;
 };
 static XeenCutsceneData installed(const GameInstallation &);
 static XeenCutsceneData parse(const std::vector<std::uint8_t> &exe,const std::vector<std::uint8_t> &cfg);
 static std::string firstCaption(const std::vector<std::uint8_t> &);
 static Captions captions(const std::vector<std::uint8_t> &);
 std::string hand(unsigned) const;
 std::string marble(unsigned) const;
};
}
#endif
