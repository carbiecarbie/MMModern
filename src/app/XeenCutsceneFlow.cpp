#include "app/XeenCutsceneFlow.h"
#include "app/XeenAudio.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenTextRenderer.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
XeenCutsceneFlow::Services XeenCutsceneFlow::original(XeenAssetSource &assets,const XeenFontFormat &font,XeenAudio &audio,PresentationClock::Source clock,bool digital) {
 auto retained=std::make_shared<std::map<std::string,XeenAudio::RetainedDigital>>();
 Services s;s.clock=std::move(clock);s.font=&font;
 s.read=[&assets](const auto &name){return assets.readArchiveResource(name,XeenSceneArchive::CloudsProgram);};
 s.prepare=[&assets](const auto &name){assets.spriteFrameCount(name,XeenSceneArchive::CloudsProgram);};
 s.draw=[&assets](auto &frame,const auto &sprite){assets.drawDialogSprite(frame,sprite.resource.c_str(),sprite.frame,sprite.x,sprite.y,XeenSceneArchive::CloudsProgram,sprite.scale,sprite.flags);};
 s.audio=[&audio,retained,digital](const auto &a,auto now) {
  using A=XeenCutscene::Audio;constexpr auto archive=XeenSceneArchive::CloudsProgram;
  switch(a.kind) {
   case A::Song:audio.requestSong(a.resource,archive,0,now);audio.advance(now);break;
   case A::FadeSong:audio.fadeSong(a.value,now);break;
   case A::Fm:audio.effect(a.value,now);break;
   case A::RetainDigital:if(digital)(*retained)[a.resource]=audio.retainDigital(a.resource,archive);break;
   case A::Digital:if(digital)audio.startDigital(retained->at(a.resource),now);break;
   case A::StopDigital:audio.stopDigital(now);retained->clear();break;
  }
 };
 s.digitalBusy=[&audio,digital](auto now){return digital && audio.digitalBusy(now);};
 s.digitalDeadline=[&audio,digital](auto now){return digital?audio.digitalDeadline(now):std::nullopt;};return s;
}
XeenCutsceneFlow::XeenCutsceneFlow(XeenCutscene scene,Services services):_scene(std::move(scene)),_services(std::move(services)) {
 if(_scene.segments.empty() || !_services.clock || !_services.read || !_services.draw || !_services.audio || !_services.digitalBusy)
  throw std::invalid_argument("Missing cutscene presentation providers");
 if(_services.initial){_canvas=*_services.initial;_palette=_canvas.palette;}
 begin();publish();
}
XeenCutscene::Context XeenCutsceneFlow::context(std::uint64_t now) const {
 const auto tick=PresentationClock::tick(now);return {_iteration,tick,tick-_reset,_services.digitalBusy(now)};
}
void XeenCutsceneFlow::trace(const char *event) {
 if(_services.trace){const auto now=_services.clock();_services.trace({event,_segment<_scene.segments.size()?segmentName():"",_iteration,now,PresentationClock::tick(now),_reset});}
}
void XeenCutsceneFlow::execute(const XeenCutscene::Frame &operations,bool deferAudio) {
 using C=XeenCutscene;
 for(const auto &op:operations)std::visit([&](const auto &value) {
  using T=std::decay_t<decltype(value)>;
  if constexpr(std::is_same_v<T,C::Raw>) {
   auto b=_services.read(value.resource);if(b.size()!=64000)throw std::runtime_error("Invalid cutscene RAW: "+value.resource);_canvas.pixels=std::move(b);
  }else if constexpr(std::is_same_v<T,C::Palette>) {
   auto b=_services.read(value.resource);if(b.size()!=768 || std::any_of(b.begin(),b.end(),[](auto c){return c>63;}))throw std::runtime_error("Invalid cutscene palette: "+value.resource);
   for(unsigned i=0;i<768;++i)_palette[i]=b[i]<<2;
  }else if constexpr(std::is_same_v<T,C::Restore>)_canvas.pixels=_slots.at(value.slot).pixels;
  else if constexpr(std::is_same_v<T,C::Store>)_slots[value.slot]=_canvas;
  else if constexpr(std::is_same_v<T,C::Sprite>)_services.draw(_canvas,value);
  else if constexpr(std::is_same_v<T,C::Prepare>) {if(_services.prepare)_services.prepare(value.resource);}
  else if constexpr(std::is_same_v<T,C::Merge>) {
   if(value.offset>200)throw std::invalid_argument("Cutscene page merge offset");
   const auto &first=_slots.at(value.first).pixels,&second=_slots.at(value.second).pixels;
   const auto tail=(200-value.offset)*320;
   std::copy_n(first.begin()+value.offset*320,tail,_canvas.pixels.begin());
   std::copy_n(second.begin(),value.offset*320,_canvas.pixels.begin()+tail);
  }else if constexpr(std::is_same_v<T,C::Scroll>) {
   _canvas=drawXeenScroll(_slots.at("scroll-base"),value.index,[&](auto &f,const char *name,unsigned frame,int x,int y){
    _services.draw(f,{name,frame,x,y,0,value.index>14 && name[0]=='h'?0x800u:0u});
   });
  }else if constexpr(std::is_same_v<T,C::Text>) {
   if(!_services.font)throw std::logic_error("Cutscene text needs a font");
   XeenTextRenderOptions o;o.bounds={0,0,320,200};o.originalControls=true;o.startupColors=true;o.x=value.x;o.y=value.y;
   auto rendered=XeenTextRenderer(*_services.font).render(_canvas,value.value,o);
   if(rendered.pages.size()!=1 || !rendered.diagnostics.empty())throw std::runtime_error("Cutscene text layout: "+(rendered.diagnostics.empty()?std::string("multiple pages"):rendered.diagnostics.front()));
   _canvas=std::move(rendered.pages.front());
  }else if constexpr(std::is_same_v<T,C::Level>)_level=value.value;
  else if constexpr(std::is_same_v<T,C::Audio>) {
   if(deferAudio)_after.push_back(value);else _services.audio(value,_services.clock());
  }
 },op);
}
void XeenCutsceneFlow::publish() {
 _frame=_canvas;_frame.palette=_palette;_frame.scaleVgaPalette(_level);
 _frame._presentation.reset();_frame._presentation=std::make_shared<const IndexedFrame>(_frame);
 _acquired=false;_publicationReady=true;
}
void XeenCutsceneFlow::begin() {
 // Empty composers are deliberately omitted content (no-digital later slots).
 while(_segment<_scene.segments.size() && !_scene.segments[_segment].compose)++_segment;
 if(_segment==_scene.segments.size()){_done=true;return;}
 const auto &s=_scene.segments[_segment];_iteration=0;_terminal=false;_fade=false;_fadePending=false;_afterSent=false;_after.clear();
 _input.reset();
 _reset=PresentationClock::tick(_services.clock());
 if(s.entering)s.entering(context(_services.clock()));
 execute(s.enter);
 if(!s.resetBeforeEnter)_reset=PresentationClock::tick(_services.clock());
 trace("entry reset");compose();
}
void XeenCutsceneFlow::compose() {
 const auto &s=_scene.segments[_segment];const auto now=_services.clock();
 if(s.reset==XeenCutscene::Segment::Reset::BeforeCompose)_reset=PresentationClock::tick(now);
 trace("compose");auto c=context(now);_after.clear();_afterSent=false;
 execute(s.compose(c),true);
 _holdTicks=s.threshold?s.threshold(c):s.ticks;
 if(_iteration==0 && s.fadeAfterFirst)_level=0;
 _deadline=PresentationClock::retraceAfter(_services.clock());
 if(s.holdBeforeFlip)_deadline=PresentationClock::retraceAfter(std::max(_services.clock(),PresentationClock::boundary(_reset+_holdTicks)));
 _acquired=false;_polled=false;_publicationReady=false;
}
void XeenCutsceneFlow::presented(const IndexedFrame::Presentation &p) {
 if(!acceptsFrame(p))throw std::logic_error("Stale cutscene presentation");
 if(!_publicationReady)return; // A redraw of the previous upload cannot acquire prepared content.
 if(_acquired)return;_acquired=true;trace("flip");
 const auto &s=_scene.segments[_segment];const auto now=_services.clock();
 if(_fade) {
  if(_fadeLevel==128)_fade=false;
  _deadline=PresentationClock::retraceAfter(now);return;
 }
 if(s.reset==XeenCutscene::Segment::Reset::AfterFlip){_reset=PresentationClock::tick(now);trace("flip reset");}
 if(!_terminal && s.published)s.published(context(now));
 if(!_terminal && _iteration==0 && s.fadeAfterFirst) {
  _fade=s.input==XeenCutscene::Segment::Input::Scroll;_fadePending=!_fade;
  _fadeLevel=0;_deadline=PresentationClock::retraceAfter(now);
 }
}
InputContext XeenCutsceneFlow::inputContext(const IndexedFrame::Presentation &p) const {
 InputContext c;c.contextId=1;c.acceptsQueuedInput=true;c.readyForAction=acceptsInput(p) && !_done;c.hideCursor=true;
 auto d=std::make_shared<DialogInput>();d->anyKey=true;d->retainKey=true;c.dialog=std::move(d);return c;
}
std::optional<IndexedFrame> XeenCutsceneFlow::handle(const PlayerAction &action,const IndexedFrame::Presentation &p) {
 if(!acceptsInput(p) || _done)return {};
 if(auto k=std::get_if<DialogKeyAction>(&action))_keys.push_back(k->key);
 else if(std::holds_alternative<CancelInteractionAction>(action))_keys.push_back(27);
 return {};
}
bool XeenCutsceneFlow::poll() {
 using I=XeenCutscene::Segment::Input;const auto &s=_scene.segments[_segment];
 if(_keys.empty() || s.input==I::Ignore)return false;
 const auto key=_keys.front();_keys.pop_front();
 if(s.input==I::Scroll && !_terminal) {
  execute(s.terminal);_terminal=true;_after.clear();_acquired=false;_publicationReady=false;_input.reset();_deadline=PresentationClock::retraceAfter(_services.clock());return true;
 }
 if(s.input==I::Escape && key==27 && _segment<_scene.closing) {
  trace("escape");_segment=_scene.closing;begin();return true;
 }return false;
}
void XeenCutsceneFlow::next() {
 if(++_segment==_scene.segments.size()){_done=true;_keys.clear();_input.reset();trace("done");return;}begin();
}
std::optional<IndexedFrame> XeenCutsceneFlow::update() {
 if(_done || _closed)return {};
 const auto now=_services.clock();
 if(_fade) {
  if(!_acquired || now<_deadline)return {};
  _fadeLevel=std::min(128u,_fadeLevel+_scene.segments[_segment].fadeAfterFirst);_level=_fadeLevel;
  publish();return _frame;
 }
 if(!_acquired) {
  if(_publicationReady)return {}; // An upload must be acquired before further work.
  if(now<_deadline)return {};
  publish();return _frame;
 }
 const auto &s=_scene.segments[_segment];
 auto c=context(now);
 const bool waiting=!s.holdBeforeFlip && c.elapsed<_holdTicks && (!s.untilDigitalIdle || c.digitalBusy);
 if(!_polled && (!s.pollAfterHold || !waiting)) {
  _polled=true;if(poll())return {}; // Scroll endpoint/closing publishes on its next retrace.
 }
 if(_fadePending){_fadePending=false;_fade=true;_deadline=PresentationClock::retraceAfter(now);return {};}
 if(!_afterSent){for(const auto &a:_after)_services.audio(a,now);_afterSent=true;}
 c=context(now);
 if(waiting)return {};
 const bool ended=_terminal || (s.frames && _iteration+1>=s.frames) || (s.totalTicks && c.elapsed>=s.totalTicks) ||
  (s.finished && s.finished(c)) || (s.untilDigitalIdle && !s.finished && !c.digitalBusy);
 if(ended) {
  if(!_terminal && !s.terminal.empty()){execute(s.terminal);_terminal=true;_acquired=false;_publicationReady=false;_input.reset();_deadline=PresentationClock::retraceAfter(now);return {};}
  next();return {};
 }
 ++_iteration;compose();return {};
}
std::optional<std::uint64_t> XeenCutsceneFlow::nextPresentationDeadline() const {
 if(_done || _closed)return {};
 const auto now=_services.clock();if(!_acquired || _fade)return _deadline;
 const auto &s=_scene.segments[_segment];const auto c=context(now);
 if(!s.holdBeforeFlip && c.elapsed<_holdTicks && (!s.untilDigitalIdle || c.digitalBusy)) {
  auto end=PresentationClock::boundary(_reset+_holdTicks);
  if(s.untilDigitalIdle && _services.digitalDeadline)if(auto voice=_services.digitalDeadline(now))end=std::min(end,*voice);
  return end;
 }return now;
}
}
