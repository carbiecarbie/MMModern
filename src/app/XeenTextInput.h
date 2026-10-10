#ifndef MMODERN_APP_XEEN_TEXT_INPUT_H
#define MMODERN_APP_XEEN_TEXT_INPUT_H
#include "games/xeen/XeenTextRenderer.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <algorithm>
#include <optional>
namespace mmodern {
// Input::waitForKey/animateCursor: UI ticks only, never a gameplay clock/RNG.
class XeenTextInput {
 std::string _value;
 unsigned _phase=0;
 std::optional<std::uint64_t> _deadline;
public:
 const std::string &value() const {return _value;}
 void clear() {_value.clear();}
 void begin() {_phase=0;keyRedraw();}
 void keyRedraw() {_phase=_phase?_phase-1:5;_deadline.reset();}
 unsigned cursor() const {static constexpr unsigned glyphs[]{32,124,126,127,126,124};return glyphs[_phase];}
 bool animate(std::uint64_t now) {
  if(!_deadline){_deadline=now+50;return false;}
  if(now<*_deadline)return false;
  const auto ticks=1+(now-*_deadline)/50;*_deadline+=ticks*50;
  _phase=(_phase+6-ticks%6)%6;return true;
 }
 bool type(const std::string &packet,const XeenFontFormat &font) {
  if(packet.empty() || packet.size()>20 || !std::all_of(packet.begin(),packet.end(),[](unsigned char c){return c>=0x20 && c<=0x7e;}))return false;
  for(unsigned char c:packet)if(!font.advance(c,XeenFontSize::Normal))return false;
  // getString ignores spaces while the line is empty, including batched keys.
  const auto first=_value.empty()?packet.find_first_not_of(' '):0;
  if(first==std::string::npos)return false;
  _value+=packet.substr(first,20-_value.size());return true;
 }
 bool backspace() {if(_value.empty())return false;_value.pop_back();return true;}
 IndexedFrame render(const IndexedFrame &base,const XeenFontFormat &font,
   const std::string &prompt,XeenTextRenderOptions options) const {
  auto result=XeenTextRenderer(font).render(base,prompt+_value,options);
  if(result.pages.size()!=1 || !result.diagnostics.empty())throw std::runtime_error("Original text input overflow");
  // Draw at the saved write position, without appending to the input or moving it.
  options.drawWindow=false;options.alignment=XeenTextAlignment::Left;
  options.x=result.writeX;options.y=result.writeY;options.size=result.writeSize;options.colorIndex=result.writeColor;
  auto cursorFrame=XeenTextRenderer(font).render(result.pages.front(),std::string(1,char(cursor())),options);
  if(cursorFrame.pages.size()!=1 || !cursorFrame.diagnostics.empty())throw std::runtime_error("Original text cursor overflow");
  return std::move(cursorFrame.pages.front());
 }
};
}
#endif
