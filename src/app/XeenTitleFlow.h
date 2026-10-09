#ifndef MMODERN_APP_XEEN_TITLE_FLOW_H
#define MMODERN_APP_XEEN_TITLE_FLOW_H
#include "app/XeenSession.h"
#include "core/InputContext.h"
#include "games/xeen/XeenDialogView.h"
#include "platform/XeenSaveFile.h"
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
 };
 enum class Screen { Menu, Background, Other, Credits, NewSlots, LoadSlots, Overwrite, Name, Difficulty, Notice, PublicationFailure };
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
 const std::optional<XeenSessionEntry> &entry() const {return _entry;}
 void close() {_closed=true;_inputFrame.reset();}
 bool current() const {return !_closed;}
 void drawButton(IndexedFrame &frame,const InputButton &button) const {
  _services.draw(frame,button.resource,button.pressedFrame(),button.x,button.y);
 }
 void newSlots();
 void cancelPublication();
 void publicationFailure(std::string message);
 void startupFailure(std::string message);
private:
 Services _services;
 Screen _screen=Screen::Background,_noticeReturn=Screen::Menu;
 std::optional<XeenSessionEntry> _entry;
 std::array<XeenSaveFile::Slot,10> _slots;
 std::optional<unsigned> _selected;
 std::string _name,_notice;
 unsigned _phase=1,_page=0;
 bool _firstTitle=true;
 std::uint64_t _deadline=0,_context=1;
 std::uint64_t _fadeDeadline=0;
 unsigned _fadeLevel=0;
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
