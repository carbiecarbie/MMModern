#ifndef MMODERN_XEEN_CUTSCENE_H
#define MMODERN_XEEN_CUTSCENE_H
#include "formats/xeen/XeenCutsceneData.h"
#include "games/xeen/XeenPresentationRandom.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"
#include <functional>
#include <memory>
#include <optional>
#include <variant>
namespace mmodern {
// A frame is an ordered composition, independent of SDL, resources and audio
// devices. Ending operations can extend Operation without changing segment holds.
struct XeenCutscene {
 struct Raw {std::string resource;};
 struct Compressed {std::string resource;};
 struct PrepareCompressed {std::string resource;};
 struct Palette {std::string resource;};
 struct Restore {std::string slot;};
 struct Store {std::string slot;};
 struct Sprite {std::string resource;unsigned frame=0;int x=0,y=0,scale=0;unsigned flags=0;};
 struct Merge {std::string first,second;unsigned offset=0;}; // Vertical, 320x200 pages.
 struct HorizontalMerge {std::string first,second;unsigned offset=0;};
 struct Rotate {std::string slot;unsigned columns=0;};
 struct Scroll {unsigned index;}; // Existing DAT geometry; EXE input is separate.
 struct Text {std::string value;int x=0,y=0,left=0,top=0,right=320,bottom=200;};
 struct Level {unsigned value;};
 struct Prepare {std::string resource;};
 struct Load {std::string resource;};
 struct Archive {XeenSceneArchive selection;};
 struct Audio {
  enum Kind {Song,FadeSong,Fm,RetainDigital,Digital,StopDigital,Mixer} kind;
  std::string resource;unsigned value=0;
 };
 using Operation=std::variant<Raw,Compressed,PrepareCompressed,Palette,Restore,Store,Sprite,Merge,HorizontalMerge,Rotate,Scroll,Text,Level,Prepare,Load,Archive,Audio>;
 using Frame=std::vector<Operation>;
 struct Context {unsigned iteration;std::uint64_t tick,elapsed;bool digitalBusy;};
 struct Segment {
  std::string name;
  Frame enter;
  std::function<Frame(const Context &)> compose;
  std::function<Frame(const Context &)> duringHold;
  unsigned holdInterval=0;
  enum class Reset {BeforeCompose,AfterFlip,Entry,Previous} reset=Reset::BeforeCompose;
  enum class Input {Ignore,Unpolled,Escape,Scroll,Keyboard,BoundaryHold} input=Input::Escape;
  unsigned frames=1,ticks=0;
  unsigned fadeAfterFirst=0;
  bool holdBeforeFlip=false,resetBeforeEnter=false,untilDigitalIdle=false,pollAfterHold=false;
  bool audioBeforeFlip=false;
  unsigned totalTicks=0;
  std::function<unsigned(const Context &)> threshold;
  std::function<bool(const Context &)> finished;
  std::function<void(const Context &)> published;
  std::function<void(const Context &)> entering;
  Frame terminal; // Scroll endpoint, also used when its key poll shortens it.
 };
 struct Observations {
  unsigned panStride=0;std::vector<unsigned> starsOffsets,lakeOffsets,captionWidths;
  std::vector<std::uint64_t> captionAdvances;
  std::vector<unsigned> throneOffsets,kingMouths,captionRecords;
  std::vector<unsigned> paradeSelectors,skyColumns,settledDigits;
  std::vector<std::array<unsigned,10>> scoreFrames;
  bool darksideSeenPoint=false;
  std::vector<unsigned> darksideQueenFrames,darksideLordFrames,darksideWizardFrames;
 };
 std::vector<Segment> segments;
 std::size_t closing=0;
 Frame completion; // On natural arrival at closing only; Escape bypasses it.
 std::shared_ptr<Observations> observations=std::make_shared<Observations>();
 static XeenCutscene clouds(const XeenCutsceneData &,std::string caption);
 static XeenCutscene cloudsEnding(const XeenCutsceneData &,const XeenCutsceneData::Captions &,XeenWallClock,std::uint32_t score);
 enum class DarksidePath {Full,Short};
 static XeenCutscene darkside(const XeenCutsceneData &,const XeenCutsceneData::Captions &,XeenWallClock,DarksidePath);
 static bool elapsed(std::uint64_t captured,std::uint64_t current,unsigned count) {return current>=captured && current-captured>=count;}
 static unsigned stride(std::uint64_t measured);
};
}
#endif
