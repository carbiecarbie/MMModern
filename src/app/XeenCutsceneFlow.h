#ifndef MMODERN_XEEN_CUTSCENE_FLOW_H
#define MMODERN_XEEN_CUTSCENE_FLOW_H
#include "games/xeen/XeenCutscene.h"
#include "core/IndexedFrame.h"
#include "core/InputContext.h"
#include "core/PresentationClock.h"
#include "formats/xeen/XeenFontFormat.h"
#include <map>
#include <deque>
namespace mmodern {
class XeenAssetSource;class XeenAudio;
// XEEN.EXE presentation lifetime, independent of title and gameplay owners.
class XeenCutsceneFlow {
public:
 struct Trace {std::string event,segment;unsigned iteration;std::uint64_t nanoseconds,tick,reset;};
 struct Services {
  PresentationClock::Source clock;
  std::function<std::vector<std::uint8_t>(const std::string &)> read;
  std::function<void(const std::string &)> prepare;
  std::function<void(IndexedFrame &,const XeenCutscene::Sprite &)> draw;
  std::function<void(const XeenCutscene::Audio &,std::uint64_t)> audio;
  std::function<bool(std::uint64_t)> digitalBusy;
  std::function<std::optional<std::uint64_t>(std::uint64_t)> digitalDeadline;
  const XeenFontFormat *font=nullptr;
  std::optional<IndexedFrame> initial;
  std::function<void(const Trace &)> trace;
 };
 XeenCutsceneFlow(XeenCutscene,Services);
 static Services original(XeenAssetSource &,const XeenFontFormat &,XeenAudio &,PresentationClock::Source,bool digital=true);
 const IndexedFrame &frame() const {return _frame;}
 bool done() const {return _done;}
 bool current() const {return !_closed;}
 void close() {_closed=true;}
 bool acceptsFrame(const IndexedFrame::Presentation &p) const {return !_closed && p && p==_frame.presentation();}
 bool acceptsInput(const IndexedFrame::Presentation &p) const {return !_closed && p && p==_input;}
 InputContext inputContext(const IndexedFrame::Presentation &) const;
 void presented(const IndexedFrame::Presentation &);
 void completeInput(const IndexedFrame::Presentation &p) {if(acceptsFrame(p))_input=p;}
 std::optional<IndexedFrame> handle(const PlayerAction &,const IndexedFrame::Presentation &);
 std::optional<IndexedFrame> update();
 std::optional<std::uint64_t> nextPresentationDeadline() const;
 const XeenCutscene &cutscene() const {return _scene;}
 const std::string &segmentName() const {return _scene.segments.at(_segment).name;}
private:
 XeenCutscene _scene;Services _services;
 std::size_t _segment=0;unsigned _iteration=0,_level=128,_holdTicks=0;
 std::uint64_t _reset=0,_deadline=0;
 bool _done=false,_closed=false,_acquired=false,_terminal=false,_fade=false,_fadePending=false,_afterSent=false,_polled=false,_publicationReady=false;
 unsigned _fadeLevel=0;
 std::deque<unsigned> _keys;
 IndexedFrame _canvas{320,200,std::vector<std::uint8_t>(64000)},_frame;
 std::array<std::uint8_t,768> _palette{};
 std::map<std::string,IndexedFrame> _slots;
 std::vector<XeenCutscene::Audio> _after;
 IndexedFrame::Presentation _input;
 XeenCutscene::Context context(std::uint64_t) const;
 void begin();void compose();void execute(const XeenCutscene::Frame &,bool deferAudio=false);
 void publish();void next();void trace(const char *);
 bool poll();
};
}
#endif
