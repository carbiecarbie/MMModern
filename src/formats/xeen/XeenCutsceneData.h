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
 unsigned digitalDevice=0,musicDevice=0;
 struct Darkside {
  enum Resource {Palette,Caption,Box,Nwc1,Nwc2,Nwc3,Nwc4,Dragon1,Dragon2,Dragon3,
   NwcRaw1,NwcRaw2,Jvc,PyramidRaw2,PyramidRaw3,Title,PyramidTop,Pyramid,
   Pharaoh1a,Pharaoh1b,Room2,Room3,Room1,Dragon,Pharaoh2a,Pharaoh2b,Pharaoh2c,Pharaoh2d,Pharaoh2e,
   BallRaw,Ball,Pharaoh,DarkLord,Goon,Queen,Wizard,Fizzle,Claw,WizardMouth,ScratchVoice,
   Pharaoh3,Queen1,Queen2,QueenHiss,Pharaoh4,Dark3,Pharaoh5a,Pharaoh5b,DarkArchive,Laugh,IntroArchive,
   Pharaoh6w,Alamar1,Pharaoh7t,Pharaoh8,Ellinger,Pharaoh9a,Pharaoh9b,Pharaoh11,
   FlyPalette,FlyRaw,Fly,BirdRaw,BirdPattern,DropPattern,Comet,Screech,Disintegrate,
   DropRaw,Branch,Vine,Thud,EndRaw,End1,End2,End3,EndTitle,Config,FirePalette,FireRaw,Explosion,Rumble,FirePattern,Count};
  std::array<std::string,Count> resources;
  std::string song,alternateSong,startupScratch,pyramidScratch;
  std::array<std::array<std::int16_t,12>,3> title;
  std::array<std::array<std::int16_t,15>,3> pyramid;
  std::array<std::array<std::int16_t,10>,2> room;
  std::array<std::uint16_t,36> roomFrames;
  std::array<std::array<std::int16_t,32>,8> ball;
  struct Cue {std::uint16_t key,target;};
  std::array<Cue,7> birdCues;
  std::array<Cue,6> nwcTailCues;
  std::array<Cue,4> nwcCues;
  std::string bird(unsigned) const;
  std::string drop(unsigned) const;
  std::string fire(unsigned) const;
  const std::string &theme(unsigned device) const {return device==5 || device==9?alternateSong:song;}
 } darkside;
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
  std::array<std::int8_t,8> flyingX,flyingY;
  unsigned initialFlying=0;
  std::array<std::string,2> scorePages;
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
