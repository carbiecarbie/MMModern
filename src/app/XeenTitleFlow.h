#ifndef MMODERN_APP_XEEN_TITLE_FLOW_H
#define MMODERN_APP_XEEN_TITLE_FLOW_H
#include "app/XeenSession.h"
#include "core/InputContext.h"
#include "games/xeen/XeenDialogView.h"
#include "platform/XeenSaveFile.h"
#include "games/xeen/XeenTextInput.h"
#include "core/PresentationClock.h"
namespace mmodern {
class XeenAssetSource;
// Title/modal presentation only. Application keeps session and resource lifetime.
class XeenTitleFlow {
public:
 struct Services {
  const XeenDosText &text;
  const XeenFontFormat &font;
  IndexedFrame background;
  std::vector<IndexedFrame> animation;
  std::array<IndexedFrame,4> credits;
  XeenDialogSpriteDraw draw;
  std::function<std::array<XeenSaveFile::Slot,10>()> slots;
  std::function<std::filesystem::path(unsigned)> path;
  bool panel=false,combat=false,saveRestricted=false,saveable=true;
  std::string saveUnavailable="not supported yet"; // Panel notice while !saveable
  std::optional<unsigned> currentSlot;
  std::string currentName;
  XeenSoundOptions soundOptions;
  bool cloudsContext=true; // DAT DS:0095: readable XEEN.CC in installed CD World.
  bool cloudsWon=false;
  std::function<std::string()> cloudsIntroCheck;
  std::function<std::pair<std::uint32_t,std::uint32_t>()> clocks;
 };
 enum class Screen { Menu, Background, Other, Credits, NewSlots, LoadSlots, Overwrite, Name, Difficulty, Notice, PublicationFailure, Panel, SaveSlots, Quit, Wizard, SavedNotice };
 explicit XeenTitleFlow(Services services);
 static Services original(XeenAssetSource &,const XeenFontFormat &,
  std::function<std::array<XeenSaveFile::Slot,10>()>,std::function<std::filesystem::path(unsigned)>);
 const IndexedFrame &frame() const {return _frame;}
 Screen screen() const {return _screen;}
 InputContext inputContext(const IndexedFrame::Presentation &) const;
 bool acceptsFrame(const IndexedFrame::Presentation &p) const {return !_closed && p==_frame.presentation();}
 bool acceptsInput(const IndexedFrame::Presentation &p) const {return !_closed && p && p==_inputFrame;}
 void presented(const IndexedFrame::Presentation &p);
 void completeInput(const IndexedFrame::Presentation &p);
 std::optional<IndexedFrame> handle(const PlayerAction &,const IndexedFrame::Presentation &);
 std::optional<IndexedFrame> animate(std::uint64_t milliseconds);
 std::optional<IndexedFrame> animateAt(std::uint64_t nanoseconds);
 std::optional<std::uint64_t> nextPresentationDeadline() const {
  if(clocksVisible())return _panelClockDeadline;
  if(_screen==Screen::Name && _name.deadline())return PresentationClock::fromMilliseconds(*_name.deadline());
  return _fadeLevel<128 ? std::optional<std::uint64_t>{_fadeDeadline} :
   _deadline ? std::optional<std::uint64_t>{_deadline} : std::nullopt;
 }
 const std::optional<XeenSessionEntry> &entry() const {return _entry;}
 void close() {_closed=true;_clockGate=0;_showClocks=false;_inputFrame.reset();}
 void clocks(std::function<std::pair<std::uint32_t,std::uint32_t>()> source) {_services.clocks=std::move(source);}
 bool clocksVisible() const {return !_closed && _screen==Screen::Panel && _showClocks;}
 bool current() const {return !_closed;}
 void drawButton(IndexedFrame &frame,const InputButton &button) const {
  _services.draw(frame,button.resource,button.pressedFrame(),button.x,button.y);
 }
 void soundOptions(XeenSoundOptions);
 XeenSoundOptions soundOptions() const {return _services.soundOptions;}
 std::optional<unsigned> takeSoundToggle() {auto value=_soundToggle;_soundToggle.reset();return value;}
 void newSlots();
 void returnToTitle();
 IndexedFrame cutsceneExitFrame() const {return _services.animation.at(7);}
 void cloudsEndingFailure(std::string message) {_entry.reset();notice(std::move(message),Screen::Background);}
 void cloudsIntroFailure(std::string message) {_entry.reset();notice(std::move(message),Screen::Other);}
 void cancelPublication();
 void publicationFailure(std::string message);
 void startupFailure(std::string message);
 void panelResult(std::string message,bool success=false);
 void loadFailure(std::string message);
 void panelFailure(std::string message) {_entry.reset();notice(std::move(message),Screen::Panel);}
 void panelCurrent(unsigned slot,std::string name);
private:
 Services _services;
 std::optional<unsigned> _soundToggle;
 Screen _screen=Screen::Background,_noticeReturn=Screen::Menu;
 std::optional<XeenSessionEntry> _entry;
 std::array<XeenSaveFile::Slot,10> _slots;
 std::optional<unsigned> _selected;
 XeenTextInput _name;
 std::string _notice;
 bool _saveAs=false;
 unsigned _clockGate=0;
 bool _showClocks=false;
 std::uint64_t _panelClockDeadline=0;
 unsigned _phase=1,_page=0;
 bool _firstTitle=true;
 std::uint64_t _deadline=0,_context=1;
 std::uint64_t _fadeDeadline=0;
 unsigned _fadeLevel=0,_fadeStep=4;
 bool _fadeAcquired=false,_animationAcquired=false;
 bool _closed=false;
 IndexedFrame _frame;
 IndexedFrame::Presentation _inputFrame;
 DialogInput dialog() const;
 void render(bool semantic=true);
 void notice(std::string message,Screen back);
 void choose();
 void setScreen(Screen screen);
};
}
#endif
