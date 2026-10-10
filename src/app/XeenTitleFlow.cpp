#include "app/XeenTitleFlow.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "formats/xeen/XeenAssetSource.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
namespace {
std::string str(std::string_view value){return std::string(value);}
unsigned lower(unsigned key){return key>='A' && key<='Z'?key+('a'-'A'):key;}
IndexedFrame text(const IndexedFrame &base,const XeenFontFormat &font,const std::string &value,
 XeenTextRect outer,XeenTextRect inner,bool border,bool startup=true,bool stopAtBottom=false) {
 XeenTextRenderOptions options;options.originalControls=true;options.startupColors=startup;options.bounds=inner;
 options.x=inner.left;options.y=inner.top;options.drawWindow=border;options.windowBounds=outer;
 options.stopAtBottom=stopAtBottom;
 auto result=XeenTextRenderer(font).render(base,value,options);
 if(result.pages.size()!=1 || !result.diagnostics.empty())throw std::runtime_error("Original dialog layout overflow at "+
  std::to_string(inner.left)+","+std::to_string(inner.top)+": "+
  (result.diagnostics.empty()?"unexpected pages":result.diagnostics.front())+"; write position "+
  std::to_string(result.writeX)+","+std::to_string(result.writeY));
 return std::move(result.pages.front());
}
}
XeenTitleFlow::Services XeenTitleFlow::original(XeenAssetSource &assets,const XeenFontFormat &font,
 std::function<std::array<XeenSaveFile::Slot,10>()> slots,std::function<std::filesystem::path(unsigned)> path) {
 const auto &dos=assets.uiText();constexpr auto dark=XeenSceneArchive::DarksideOnly;
 for(unsigned c=0x20;c<=0x7e;++c) {
  const auto glyph=font.glyph(c,XeenFontSize::Normal);
  if(!glyph.advance || (c!=' ' && std::none_of(glyph.pixels.begin(),glyph.pixels.end(),[](auto value){return value!=0;})))
   throw std::runtime_error("CD title font cannot display accepted save-name code "+std::to_string(c));
 }
 assets.loadPalette(str(dos.scalar("TITLE_PALETTE")),dark);
 if(!assets.cursorImage(dark).isValid())throw std::runtime_error("Invalid CD title cursor");
 assets.loadRawFramebuffer(str(dos.scalar("TITLE_BACKGROUND")),dark);
 const auto base=assets.snapshot();std::vector<IndexedFrame> animation;
 unsigned animationResource=0;
 for(const auto &resource:dos.table("TITLE_ANIMATIONS")) {
  const auto count=assets.spriteFrameCount(resource,dark);
  constexpr unsigned expected[]{5,5,3};
  if(animationResource==3 || count!=expected[animationResource++])throw std::runtime_error("Unexpected CD title animation frames");
  for(unsigned index=0;index<count;++index) {
   auto frame=base;assets.drawDialogSprite(frame,resource.c_str(),index,0,0,dark);animation.push_back(std::move(frame));
  }
 }
 for(const auto &role:std::vector<std::pair<const char *,unsigned>>{{"TITLE_BUTTONS",8},{"OTHER_BUTTONS",10},{"DIFFICULTY_BUTTONS",4},{"CHOOSER_SPRITES",6}}) {
  const auto resource=str(dos.scalar(role.first));
  if(assets.spriteFrameCount(resource,dark)!=role.second)throw std::runtime_error("Unexpected CD menu sprite frames");
  for(unsigned i=0;i<role.second;++i){auto scratch=base;assets.drawDialogSprite(scratch,resource.c_str(),i,0,0,dark);}
 }
 if(assets.spriteFrameCount("confirm.icn",dark)!=4)throw std::runtime_error("Unexpected CD confirmation frames");
 assets.loadRawFramebuffer(str(dos.scalar("CREDITS_BACKGROUND")),dark);
 const auto background=assets.snapshot();
 const auto bytes=assets.readArchiveResource(str(dos.scalar("CREDITS_RESOURCE")),dark);
 if(bytes.empty() || bytes.size()>65535)throw std::runtime_error("Malformed CD credits extent");
 std::array<IndexedFrame,4> credits;unsigned page=0;std::size_t begin=0;
 for(unsigned i=0;i<bytes.size();++i)if(!bytes[i]) {
  if(page==4 || begin==i)throw std::runtime_error("Malformed CD credits page boundaries");
  std::string value(reinterpret_cast<const char *>(bytes.data()+begin),i-begin);
  XeenDosText::validateControls(value);
  credits[page++]=text(background,font,value,{0,0,320,200},{0,0,320,200},false);begin=i+1;
 }
 if(page!=4 || begin!=bytes.size())throw std::runtime_error("CD credits require four terminated pages");
 return Services{dos,font,base,std::move(animation),std::move(credits),
  [&assets](IndexedFrame &frame,const char *resource,unsigned index,int x,int y){assets.drawDialogSprite(frame,resource,index,x,y,dark);},
  std::move(slots),std::move(path)};
}
XeenTitleFlow::XeenTitleFlow(Services services):_services(std::move(services)) {
 if((!_services.panel && _services.animation.empty()) || !_services.draw || !_services.slots || !_services.path)
  throw std::invalid_argument("Missing title presentation providers");
 if(!_services.background.isValid())throw std::invalid_argument("Invalid title background");
 if(_services.panel){_screen=Screen::Panel;_fadeLevel=128;_firstTitle=false;}
 else _phase%=_services.animation.size();
 for(const auto &frame:_services.animation)if(!frame.isValid())throw std::invalid_argument("Invalid title animation frame");
 if(!_services.panel)for(const auto &frame:_services.credits)if(!frame.isValid())throw std::invalid_argument("Invalid credits page");
 render();
}
void XeenTitleFlow::setScreen(Screen screen) {
 if(_screen==Screen::Credits && screen==Screen::Menu){_fadeLevel=0;_fadeDeadline=0;_deadline=0;}
 if(screen==Screen::Name)_name.begin();
 _screen=screen;render();
}
void XeenTitleFlow::notice(std::string message,Screen back) {
 _notice=std::move(message);_noticeReturn=back;setScreen(Screen::Notice);
}
void XeenTitleFlow::newSlots() {
 _closed=false;_entry.reset();_selected.reset();_name.clear();_slots=_services.slots();setScreen(Screen::NewSlots);
}
void XeenTitleFlow::publicationFailure(std::string message) {
 _closed=false;_entry.reset();_notice=std::move(message);setScreen(Screen::PublicationFailure);
}
void XeenTitleFlow::cancelPublication() {
 _closed=false;_entry.reset();_slots=_services.slots();setScreen(Screen::NewSlots);
}
void XeenTitleFlow::startupFailure(std::string message) {
 _closed=false;_entry.reset();notice(std::move(message),Screen::LoadSlots);
}
void XeenTitleFlow::loadFailure(std::string message) {
 _entry.reset();notice(std::move(message),Screen::LoadSlots);
}
void XeenTitleFlow::panelCurrent(unsigned slot,std::string name) {
 _services.currentSlot=slot;_services.currentName=std::move(name);
}
void XeenTitleFlow::panelResult(std::string message,bool success) {
 _entry.reset();
 if(_saveAs)_slots=_services.slots();
 _notice=std::move(message);_noticeReturn=_saveAs?Screen::SaveSlots:Screen::Panel;
 setScreen(success?Screen::SavedNotice:Screen::Notice);
}
DialogInput XeenTitleFlow::dialog() const {
 DialogInput input;input.keys.push_back(InputKey::Escape);
 const auto add=[&](const XeenDosText::Button &button,unsigned key,const char *resource,unsigned frame) {
  input.keys.push_back(key);input.hits.push_back({int(button.x),int(button.y),int(button.x+button.width),int(button.y+button.height),key,
   resource?std::optional<InputButton>{{resource,frame,int(button.x),int(button.y)}}:std::nullopt});
 };
 if(_screen==Screen::Menu || _screen==Screen::Other) {
  const bool other=_screen==Screen::Other;const auto &buttons=_services.text.buttons("TITLE");
  const char *resource=_services.text.scalar(other?"OTHER_BUTTONS":"TITLE_BUTTONS").data();
  for(unsigned i=0;i<(other?2u:4u);++i)add(buttons[i],other?(i?'c':'d'):lower(buttons[i].key),resource,i*2);
 } else if(_screen==Screen::Panel) {
  for(const auto &button:_services.text.buttons("PANEL"))
   add(button,lower(button.key),_services.text.scalar("PANEL_SPRITES").data(),0);
 } else if(_screen==Screen::NewSlots || _screen==Screen::LoadSlots || _screen==Screen::SaveSlots) {
  const auto &buttons=_services.text.buttons("CHOOSER");
  const char *resource=_services.text.scalar("CHOOSER_SPRITES").data();
  for(unsigned i=0;i<buttons.size();++i) {
   const auto key=lower(buttons[i].key);
   add(buttons[i],key,buttons[i].painted?resource:nullptr,i*2);
  }
  input.keys.push_back(InputKey::Enter);
 } else if(_screen==Screen::Difficulty) {
  const auto &buttons=_services.text.buttons("DIFFICULTY");
  for(unsigned i=0;i<buttons.size();++i)add(buttons[i],lower(buttons[i].key),_services.text.scalar("DIFFICULTY_BUTTONS").data(),i*2);
 } else if(_screen==Screen::Overwrite || _screen==Screen::Quit || _screen==Screen::Wizard)input=xeenConfirmInput();
 else if(_screen==Screen::Name) {input.textEntry=true;input.keys.insert(input.keys.end(),{8,InputKey::Enter});}
 else {input.anyKey=true;input.anyClick=true;}
 return input;
}
void XeenTitleFlow::render(bool semantic) {
 if(semantic){++_context;_inputFrame.reset();}
 auto frame=_services.panel?_services.background:_screen==Screen::Credits?_services.credits.at(_page):_firstTitle?_services.background:_services.animation.at(_phase);
 const auto &dos=_services.text;const auto &font=_services.font;
 if(_screen==Screen::Menu || _screen==Screen::Other) {
  const bool other=_screen==Screen::Other;
  const auto label=other?xeenDialogFormat(dos.scalar("OPTIONS_TEMPLATE"),{str(dos.scalar("WORLD_LABEL")),"67"}):str(dos.scalar("WORLD_MENU"));
  frame=text(frame,font,label,{72,25,248,other?125:175},{80,33,240,other?117:167},true);
  for(const auto &hit:dialog().hits)if(hit.button){const auto &b=*hit.button;_services.draw(frame,b.resource,b.frame,b.x,b.y);}
 } else if(_screen==Screen::Panel) {
  // DOS window 23: code-segment arrays at 0x3dc2c/8c/ec, 0x3dd4c (80,28,176,140).
  const auto state=str(dos.scalar("PANEL_OFF"));
  const auto buttons=xeenDialogFormat(dos.scalar("PANEL_BUTTON_TEXT"),{state,state,state,state});
  frame=text(frame,font,xeenDialogFormat(dos.scalar("PANEL_TEXT"),{buttons,_services.currentName}),
   {80,28,256,168},{88,36,248,160},true,false,true);
  for(const auto &hit:dialog().hits)if(hit.button){const auto &b=*hit.button;_services.draw(frame,b.resource,b.frame,b.x,b.y);}
  // DOS draws button backgrounds before its labels.
  frame=text(frame,font,xeenDialogFormat(dos.scalar("PANEL_TEXT"),{buttons,_services.currentName}),
   {80,28,256,168},{88,36,248,160},false,false,true);
 } else if(_screen==Screen::NewSlots || _screen==Screen::LoadSlots || _screen==Screen::SaveSlots) {
  std::vector<std::string> args{str(dos.scalar(_screen==Screen::NewSlots?"START_LABEL":_screen==Screen::SaveSlots?"SAVE_LABEL":"LOAD_LABEL"))};
  for(unsigned i=0;i<10;++i) {
   const auto &slot=_slots[i];args.push_back(_selected==i?"15":"0");
   if(slot.state==XeenSaveFile::Slot::State::Available && slot.snapshot && slot.snapshot->name) {
    args.push_back(*slot.snapshot->name);
    std::uint64_t level=0;
    // DOS slot metadata calls the current-level getter (file 0xf16f/0x21fc9).
    // Use a wide sum while inspecting untrusted snapshots, before restore.
    for(auto id:slot.snapshot->activeRosterIds) {
     const auto &character=slot.snapshot->characters[id];
     const auto current=std::max<std::int64_t>(0,std::int64_t(character.permanentLevel)+character.temporaryLevel);
     level=std::max(level,static_cast<std::uint64_t>(current));
    }
    args.push_back(xeenDialogFormat(dos.scalar("SLOT_DETAILS"),{slot.snapshot->journey->context->difficulty==XeenDifficulty::Adventurer?"A":"W",std::to_string(level)}));
   } else {args.push_back(slot.state==XeenSaveFile::Slot::State::Empty?str(dos.scalar("EMPTY_SLOT")):"Unavailable");args.emplace_back();}
  }
  args.push_back(_selected?xeenDialogFormat(dos.scalar("DOS_FILE_PATTERN"),{std::to_string(*_selected+1)}):"");
  // The chooser uses the same native writer's checked newline termination.
  frame=text(frame,font,xeenDialogFormat(dos.scalar("CHOOSER"),args),{27,6,207,142},{35,14,187,134},true,!_services.panel,true);
  const auto *resource=dos.scalar("CHOOSER_SPRITES").data();
  // Original DOS chooser drawing calls at file offsets 0x20749..0x207a9.
  for(const auto &b:std::array<InputButton,4>{{{resource,4,39,26},{resource,0,187,26},{resource,2,187,111},{resource,5,132,123}}})
   _services.draw(frame,b.resource,b.frame,b.x,b.y);
 } else if(_screen==Screen::Overwrite) {
  const auto &slot=_slots.at(*_selected);
  const auto name=slot.snapshot && slot.snapshot->name?*slot.snapshot->name:"older MMModern save";
  // The native window 21 writer stops before a newline exceeds its bottom.
  frame=drawXeenConfirm(frame,font,xeenDialogFormat(dos.scalar("OVERWRITE_CONFIRM"),{name}),false,_services.draw,!_services.panel,true);
 } else if(_screen==Screen::Name) {
  XeenTextRenderOptions options;options.originalControls=true;options.startupColors=!_services.panel;
  options.bounds={60,157,260,190};options.x=60;options.y=157;
  options.drawWindow=true;options.windowBounds={52,149,268,198};
  frame=_name.render(frame,font,str(dos.scalar("NAME_PROMPT")),options);
 } else if(_screen==Screen::Quit || _screen==Screen::Wizard) {
  frame=drawXeenConfirm(frame,font,str(dos.scalar(_screen==Screen::Quit?"CONFIRM_QUIT":"MR_WIZARD")),false,_services.draw);
 } else if(_screen==Screen::Difficulty) {
  frame=text(frame,font,str(dos.scalar("DIFFICULTY_TEXT")),{52,149,268,198},{60,157,260,190},true);
  for(const auto &hit:dialog().hits)if(hit.button){const auto &b=*hit.button;_services.draw(frame,b.resource,b.frame,b.x,b.y);}
 } else if(_screen==Screen::SavedNotice) {
  // DOS save routine at 0x1ed5e writes SAVED_NOTICE directly to window 21.
  // Window arrays at 0x3dc28/0x3dc88/0x3dce8/0x3dd48: 99,59,138,82.
  // Its CR/alignment/v010 controls belong to this window, without an error prefix.
  // DOS newline at 0x43dfc returns carry before a line exceeds the inner bottom;
  // writeString then returns. Keep glyph/control overflow diagnostics active.
  frame=text(frame,font,_notice,{99,59,237,141},{107,67,229,133},true,false,true);
 } else if(_screen==Screen::Notice || _screen==Screen::PublicationFailure)frame=drawXeenErrorScroll(frame,font,_notice,!_services.panel);
 if(_fadeLevel<128)for(auto &component:frame.palette)component=static_cast<std::uint8_t>(((unsigned(component)>>2)*_fadeLevel/128)<<2);
 frame._presentation.reset();auto retained=std::make_shared<IndexedFrame>(frame);frame._presentation=std::move(retained);_frame=std::move(frame);
}
InputContext XeenTitleFlow::inputContext(const IndexedFrame::Presentation &origin) const {
 InputContext context;context.contextId=_context;context.readyForAction=acceptsInput(origin) && !_entry && _fadeLevel==128;
 context.dialog=std::make_shared<DialogInput>(dialog());return context;
}
void XeenTitleFlow::presented(const IndexedFrame::Presentation &p) {
 if(!acceptsFrame(p))throw std::logic_error("Stale title presentation");
}
void XeenTitleFlow::completeInput(const IndexedFrame::Presentation &p) {
 if(!acceptsFrame(p))throw std::logic_error("Stale title input handoff");_inputFrame=p;
}
void XeenTitleFlow::choose() {
 if(!_selected)return;
 const auto &slot=_slots[*_selected];
 if(slot.state==XeenSaveFile::Slot::State::Protected) {notice(slot.reason,_screen);return;}
 if(_screen==Screen::LoadSlots) {
  if(slot.state!=XeenSaveFile::Slot::State::Available || !slot.snapshot) {
   if(slot.state==XeenSaveFile::Slot::State::Older)notice(slot.reason,Screen::LoadSlots);return;
  }
  XeenSessionEntry entry;entry.kind=XeenSessionEntry::Kind::Load;entry.slot=*_selected;
  entry.path=_services.path(*_selected);entry.snapshot=std::make_shared<const XeenSaveSnapshot>(*slot.snapshot);entry.name=*slot.snapshot->name;
  _entry=std::move(entry);return;
 }
 if(_services.panel)_name.clear();
 setScreen(slot.state==XeenSaveFile::Slot::State::Empty?Screen::Name:Screen::Overwrite);
}
std::optional<IndexedFrame> XeenTitleFlow::handle(const PlayerAction &action,const IndexedFrame::Presentation &origin) {
 if(!acceptsInput(origin) || _entry || _fadeLevel<128)return {};
 if(const auto *typed=std::get_if<TextInputAction>(&action)) {
  if(_screen!=Screen::Name || typed->text.empty())return {};
  // Reject the whole input packet on unsupported codes; preserve accepted case.
  if(!_name.type(typed->text,_services.font))return {};
  render();return _frame;
 }
 const auto *keyAction=std::get_if<DialogKeyAction>(&action);
 const unsigned key=keyAction?lower(keyAction->key):std::holds_alternative<CancelInteractionAction>(action)?27:0;
 const bool escape=key==27;
 switch(_screen) {
 case Screen::Menu:
  if(escape)setScreen(Screen::Background);
  else if(key=='s')newSlots();
  else if(key=='l') {
   _slots=_services.slots();_selected.reset();
   const bool any=std::any_of(_slots.begin(),_slots.end(),[](const auto &s){return s.state!=XeenSaveFile::Slot::State::Empty;});
   if(!any)notice(str(_services.text.scalar("NO_SAVES")),Screen::Menu);else setScreen(Screen::LoadSlots);
  } else if(key=='c'){_page=0;setScreen(Screen::Credits);}
  else if(key=='o')setScreen(Screen::Other);
  else return {};
  break;
 case Screen::Background:
  if(_firstTitle){_firstTitle=false;_phase=(_phase+1)%_services.animation.size();_deadline=0;}
  setScreen(Screen::Menu);break;
 case Screen::Other:
  if(escape)setScreen(Screen::Menu);
  else if(key=='c' || key=='d')notice("not supported yet",Screen::Other);else return {};
  break;
 case Screen::Credits:
  if(escape || _page==3)setScreen(Screen::Menu);else {++_page;render();}break;
 case Screen::Panel:
  if(escape){_closed=true;_inputFrame.reset();return {};}
  if(key=='q'){setScreen(Screen::Quit);break;}
  if(key=='w'){setScreen(Screen::Wizard);break;}
  if(key=='e' || key=='m' || key=='p' || key=='t'){notice("not supported yet",Screen::Panel);break;}
  if(key!='s' && key!='a' && key!='l')return {};
  if(_services.combat){notice(str(_services.text.scalar(key=='l'?"NO_LOADING_IN_COMBAT":"NO_SAVING_IN_COMBAT")),Screen::Panel);break;}
  if(!_services.saveable){notice("not supported yet",Screen::Panel);break;}
  if(key!='l' && _services.saveRestricted){notice(str(_services.text.scalar("SAVE_RESTRICTED")),Screen::Panel);break;}
  _saveAs=key!='l' && (key=='a' || !_services.currentSlot);
  if(key=='s' && !_saveAs) {
   XeenSessionEntry entry;entry.kind=XeenSessionEntry::Kind::Save;entry.slot=*_services.currentSlot;
   entry.name=_services.currentName;_entry=std::move(entry);break;
  }
  _slots=_services.slots();_selected.reset();_name.clear();
  if(key=='l' && std::none_of(_slots.begin(),_slots.end(),[](const auto &s){return s.state!=XeenSaveFile::Slot::State::Empty;}))
   notice(str(_services.text.scalar("NO_SAVES")),Screen::Panel);
  else setScreen(key=='l'?Screen::LoadSlots:Screen::SaveSlots);
  break;
 case Screen::Quit:case Screen::Wizard:
  if(escape){setScreen(Screen::Panel);break;}
  if(const auto answer=xeenConfirmAnswer(key)) {
   if(!*answer)setScreen(Screen::Panel);
   else if(_screen==Screen::Wizard)notice("not supported yet",Screen::Panel);
   else {XeenSessionEntry entry;entry.kind=XeenSessionEntry::Kind::Exit;_entry=std::move(entry);}
  } else return {};
  break;
 case Screen::NewSlots:case Screen::LoadSlots:case Screen::SaveSlots:
  if(escape)setScreen(_services.panel?Screen::Panel:Screen::Menu);
  else if(key>='0' && key<='9') {
   _selected=key=='0'?9:key-'1';
   if(_slots[*_selected].state==XeenSaveFile::Slot::State::Protected)notice(_slots[*_selected].reason,_screen);else render();
  }
  else if(key==InputKey::Enter || key=='s')choose();else return {};
  break;
 case Screen::Overwrite:
  if(escape){setScreen(_services.panel?Screen::SaveSlots:Screen::NewSlots);break;}
  if(const auto answer=xeenConfirmAnswer(key)) {if(*answer)setScreen(Screen::Name);else setScreen(_services.panel?Screen::SaveSlots:Screen::NewSlots);}else return {};
  break;
 case Screen::Name:
  if(escape)setScreen(_services.panel?Screen::SaveSlots:Screen::NewSlots);
  else if(key==8 && _name.backspace())render();
  else if(key==InputKey::Enter && !_name.value().empty()) {
   if(_services.panel){XeenSessionEntry entry;entry.kind=XeenSessionEntry::Kind::Save;entry.slot=*_selected;entry.name=_name.value();_entry=std::move(entry);}
   else setScreen(Screen::Difficulty);
  }else return {};
  break;
 case Screen::Difficulty:
  if(escape){setScreen(Screen::NewSlots);break;}
  if(key=='a' || key=='w') {
   XeenSessionEntry entry;entry.kind=XeenSessionEntry::Kind::New;entry.slot=*_selected;entry.path=_services.path(*_selected);
   entry.name=_name.value();entry.difficulty=key=='a'?XeenDifficulty::Adventurer:XeenDifficulty::Warrior;_entry=std::move(entry);
  }else return {};
  break;
 case Screen::Notice:case Screen::SavedNotice:setScreen(_noticeReturn);break;
 case Screen::PublicationFailure: {
  XeenSessionEntry entry;entry.kind=escape?XeenSessionEntry::Kind::CancelNew:XeenSessionEntry::Kind::Retry;_entry=std::move(entry);break;
 }
 }
 return _frame;
}
std::optional<IndexedFrame> XeenTitleFlow::animate(std::uint64_t milliseconds) {
 if(!_closed && !_entry && _screen==Screen::Name) {
  if(!_name.animate(milliseconds))return {};
  render(false);return _frame;
 }
 if(_services.panel)return {};
 if(!_closed && !_entry && _fadeLevel<128) {
  // Pinned screen.cpp fadeIn(2): palette levels 0..128, polling at 10 ms.
  if(!_fadeDeadline){_fadeDeadline=milliseconds+10;return {};}
  if(milliseconds<_fadeDeadline)return {};
  const auto ticks=1+(milliseconds-_fadeDeadline)/10;_fadeDeadline+=ticks*10;
  _fadeLevel=static_cast<unsigned>(std::min<std::uint64_t>(128,_fadeLevel+ticks*2));
  render(false);if(_fadeLevel==128)_deadline=milliseconds+200;return _frame;
 }
 if(_closed || _entry || _screen==Screen::Credits || _screen==Screen::NewSlots || _screen==Screen::LoadSlots ||
  _screen==Screen::Name || _screen==Screen::Difficulty || _screen==Screen::Overwrite || _screen==Screen::PublicationFailure)return {};
 if(!_deadline){_deadline=milliseconds+200;return {};}
 if(milliseconds<_deadline)return {};
 const auto steps=1+(milliseconds-_deadline)/200;_deadline+=steps*200;
 _phase=(_phase+steps)%_services.animation.size();_firstTitle=false;render(false);return _frame;
}
}
