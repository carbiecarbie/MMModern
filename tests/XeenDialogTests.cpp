#include "XeenTestInstallation.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenEventPresenter.h"
#include "platform/sdl/XeenMainScreenInput.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include <iostream>
#include <set>
#include <stdexcept>
#include <fstream>
using namespace mmodern;
namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
void hits(){
 for(const auto &input:{xeenSheetInput(),xeenItemsInput(false),xeenItemsInput(true),xeenItemsInput(false,true),xeenBuyInput(),xeenBuyInput(true),xeenLocationInput(XeenLocationDialog::Smith),xeenLocationInput(XeenLocationDialog::Training),xeenLocationInput(XeenLocationDialog::Temple),xeenConfirmInput(false),xeenConfirmInput(true)}){
  for(const auto &hit:input.hits){
   for(const int x:{hit.left,hit.right-1})for(const int y:{hit.top,hit.bottom-1}){
    const auto action=input.click(x,y);check(action && std::get<DialogKeyAction>(*action).key==hit.key,"dialog rectangle corner");
   }
   for(const auto point:{std::pair{hit.left-1,hit.top},std::pair{hit.right,hit.top},std::pair{hit.left,hit.top-1},std::pair{hit.left,hit.bottom}}){
    const auto action=input.click(point.first,point.second);check(!action || std::get<DialogKeyAction>(*action).key!=hit.key,"dialog exclusive edge");
   }
   check(input.key(hit.key).has_value(),"hit key unavailable");
   if(hit.button) {
    const auto sprite=input.button(hit.key);
    check(sprite && sprite->x==hit.left && sprite->y==hit.top && sprite->pressedFrame()==(sprite->frame|1),"dialog pressed sprite metadata");
   }
  }
  check(!input.click(-1,100) && !input.click(320,100) && !input.click(0,200),"outside screen accepted");
 }
 check(!xeenSheetInput().click(250,50) && !xeenItemsInput(false).click(270,50),"outside dialog accepted");
 check(xeenConfirmInput().key(InputKey::Escape).has_value(),"Confirm Escape absent");
 check(xeenLocationInput(XeenLocationDialog::Training).button('t')->pressedFrame()==1 &&
  xeenLocationInput(XeenLocationDialog::Training).button(InputKey::Escape)->pressedFrame()==3 &&
  xeenBuyInput().button('b')->frame==9 && xeenBuyInput(true).button('f')->frame==15,
  "original service persistent mode/pressed glyphs");
 check(xeenLocationInput(XeenLocationDialog::Smith).key('r').has_value() && !xeenLocationInput(XeenLocationDialog::Training).key(InputKey::Enter) &&
  !xeenLocationInput(XeenLocationDialog::Temple).key(InputKey::Enter) && !xeenBuyInput().key(InputKey::Right),"project service keys survived");
 check(xeenBuyDisplayCost(XeenInventoryCategory::Weapons,{0,3,0,0})==100 &&
  xeenBuyDisplayCost(XeenInventoryCategory::Armor,{38,3,0,0})==50 &&
  xeenBuyDisplayCost(XeenInventoryCategory::Accessories,{48,1,0,0})==500 &&
  xeenBuyDisplayCost(XeenInventoryCategory::Miscellaneous,{2,16,7,0})==1200,"original stock display arithmetic");
 check(xeenConfirmAnswer('y')==true && xeenConfirmAnswer('n')==false &&
  xeenConfirmAnswer(InputKey::Escape)==false && !xeenConfirmAnswer(InputKey::Enter),"Confirm Y/N/Escape behavior");
 check(xeenItemsInput(true).button('u')->pressedFrame()==19 &&
  xeenSheetInput().button('i')->pressedFrame()==41 &&
  !xeenSheetInput().button(InputKey::F1) && !xeenItemsInput(false).button('1'),"sprite/no-sprite button distinctions");
 DialogInput popup;popup.anyKey=true;popup.anyClick=true;
 check(std::holds_alternative<AcknowledgeAction>(*popup.key('z')) && std::holds_alternative<AcknowledgeAction>(*popup.click(0,0)),"popup any key/click");
}
void rules(){
 std::vector<std::uint8_t> bytes(30*354);for(unsigned a=0;a<7;++a){bytes[20+a*2]=20;bytes[21+a*2]=2;}
 bytes[18]=1;bytes[19]=1;bytes[35]=100;bytes[36]=101;bytes[37]=7;bytes[346]=128;bytes[347]=2;
 bytes[39]=1;bytes[56]=1;bytes[57]=0x21;bytes[66]=0xab;bytes[120]=0x43;
 for(unsigned r=0;r<6;++r){bytes[311+r*2]=r+1;bytes[312+r*2]=2;}
 bytes[348]=42;
 auto roster=XeenCharacterFormat::parseRoster(bytes);auto c=roster.at(0);const auto *details=c.originalDetails();
 check(details && c.skills[17]==1 && details->birthDay==7 && details->awards[0]==1 && details->awards[64]==2 && details->awards[9]==0xab && details->awards[73]==0 && details->awards[63]==3 && details->awards[127]==4,"original immutable fields offsets");
 using R=XeenCharacterRules;const XeenCharacterRulesContext context{650};
 check(R::sheetAge(c,context)==10,"sheet age");
 check(R::skillCount(c)==2 && R::awardCount(c)==5,"skill/award counts");
 check(c.awards[9]==0xab && c.awards[73]==0 && c.awards[127]==4,"Mutable CHR award initialization");
 c.awards[6]=1;check(R::awardCount(c)==6,"Sheet ignored mutable award state");c.awards[6]=0;
 check(R::sheetStat(c,nullptr,0,context,true)==0 && R::sheetStat(c,nullptr,1,context,true)==0,"young-age adjustment");
 c.birthYear=610;c.temporaryAge=0;c.permanentLevel=1;
 check(R::sheetStat(c,nullptr,0,context)==20 && R::sheetStat(c,nullptr,1,context)==24 && R::sheetStat(c,nullptr,6,context)==22,"physical/mental/luck age adjustments");
 c.conditions[3]=3;c.conditions[4]=4;c.conditions[0]=5;c.conditions[1]=1;
 check(R::sheetStat(c,nullptr,0,context)==16 && R::sheetStat(c,nullptr,1,context)==19 && R::sheetStat(c,nullptr,6,context)==16,"sheet condition modifiers");
 c.conditions[13]=1;check(R::sheetStat(c,nullptr,0,context)==20,"dead condition modifiers");
 c.armor[0]={1,2,0,3};c.accessories[0]={34,1,0,1};
 check(R::sheetResistance(c,nullptr,0)==8 && R::sheetResistance(c,nullptr,5)==13,"elemental resistance bonuses");
 c.armor[0].state=128;check(R::sheetResistance(c,nullptr,0)==3,"broken resistance excluded");
 check(R::statColor(0,20)==6 && R::statColor(21,20)==2 && R::statColor(20,20)==15 && R::statColor(5,20)==9 && R::statColor(4,20)==32,"stat colors");
 c.characterClass=XeenCharacterClass::Knight;check(R::currentExperience(c,nullptr)==42 && R::experienceToNextLevel(c,nullptr)==1458,"level-one XP");
 c.permanentLevel=13;check(R::currentExperience(c,nullptr)==1536042 && R::experienceToNextLevel(c,nullptr)==1023958,"high-level XP");
}
void original(const char *path){
 const auto installation=xeenTestInstallationDetector().detect(path);check(bool(installation),"original installation absent");XeenAssetSource assets(*installation);
 auto p=XeenPartyLoader().loadInitialCloudsParty(assets);const auto bytes=assets.readInitialResource("maze.chr"),pty=assets.readInitialResource("maze.pty");
 const XeenFontFormat font(assets.readArchiveResource("fnt"));const auto catalog=loadXeenItemCatalog(assets).catalog;
 IndexedFrame base;base.width=320;base.height=200;base.pixels.assign(64000,77);
 const auto draw=[&](IndexedFrame &frame,const char *resource,unsigned id,int x,int y){assets.drawDialogSprite(frame,resource,id,x,y);};
 // The normal combat pane is the exact DAT stream, including padded colors,
 // with the contact marker independent of text width.
 check(xeenCombatHealthColor(100,100)==15 && xeenCombatHealthColor(99,100)==9 &&
     xeenCombatHealthColor(25,100)==9 && xeenCombatHealthColor(24,100)==32 &&
     xeenCombatHealthColor(2,11)==9 && xeenCombatHealthColor(1,11)==32,"DOS quarter-health thresholds");
 const std::array<XeenCombatPanelRow,3> rows{{{"A",100,100},{"B",25,100},{"C",24,100}}};
 const auto &dos=assets.uiText();
 XeenTreasureDisplay treasure;treasure.gold=123456;treasure.gems=78;
 treasure.items={{"A","item",false},{"B","seal",true}};
 const auto heading=xeenDialogFormat(dos.scalar("PARTY_FOUND"),{"123456","78"});
 const auto quest=xeenDialogFormat(dos.scalar("QUEST_FOUND_PREFIX"),{"seal"});
 check(quest==std::string("\f04 * \fdseal"),"quest star uses original color-04 text and default reset");
 check(xeenTreasureText(dos,treasure,0)==heading+std::string(dos.scalar("GIVE_TREASURE_FORMATTING"))+
     xeenDialogFormat(dos.scalar("X_FOUND_Y"),{"A","item"}),"first item exact DAT stream");
 const auto last=heading+std::string(dos.scalar("GIVE_TREASURE_FORMATTING"))+
     xeenDialogFormat(dos.scalar("X_FOUND_Y"),{"B",quest});
 check(xeenTreasureText(dos,treasure,1)==last &&
     xeenTreasureText(dos,treasure,2)==last+std::string(dos.scalar("HIT_A_KEY")),"final color-37 prompt follows last item");
 XeenTextRenderOptions treasureOptions;treasureOptions.originalControls=treasureOptions.drawWindow=true;
 treasureOptions.windowBounds={226,0,320,146};treasureOptions.bounds={234,8,312,138};treasureOptions.x=234;treasureOptions.y=8;
 for(unsigned page=0;page<3;++page) {
     const auto oracle=XeenTextRenderer(font).render(base,xeenTreasureText(dos,treasure,page),treasureOptions).pages.front();
     check(drawXeenTreasure(dos,base,font,treasure,page).pixels==oracle.pixels,"treasure window 10 bounds/text/colors");
 }
 const auto headingFrame=XeenTextRenderer(font).render(base,heading,treasureOptions).pages.front();
 const auto warning=drawXeenErrorScroll(headingFrame,font,std::string(dos.scalar("BACKPACKS_FULL_PRESS_KEY")));
 check(drawXeenTreasure(dos,base,font,treasure,0,true).pixels==warning.pixels,"modal window 6 over original currency heading");
 for(unsigned inputKind=0;inputKind<3;++inputKind) {
     XeenEventPresenter pages(font);pages.originalAssets(&dos,draw);
     XeenPresentationRequest request;request.kind=XeenPresentationKind::RewardReceipt;
     request.response=XeenPresentationResponseRequirement::Acknowledgment;request.treasure=treasure;request.treasure->fullPacks=true;
     pages.present(base,request);check(pages.pageCount()==4 && pages.frame().pixels==warning.pixels,"separate warning page");
     for(unsigned page=0;page<4;++page) {
         const auto before=pages.frame().pixels;pages.rebase(base);check(pages.frame().pixels==before,"treasure rebase retains exact page");
         const auto input=pages.input();
         const auto action=inputKind==0?input.key('z'):inputKind==1?input.click(0,0):input.rightClick();
         check(action.has_value(),"ordinary key/either mouse treasure acknowledgment");
         const auto update=pages.handle(*action);
         check(bool(update.response)==(page==3),"warning/item acknowledgment cannot finish treasure");
         if(page==0)check(pages.frame().pixels==drawXeenTreasure(dos,base,font,treasure,0).pixels,"warning dismissal restores treasure only");
     }
 }
 {
     XeenEventPresenter pages(font);pages.originalAssets(&dos,draw);
     XeenPresentationRequest audio;audio.kind=XeenPresentationKind::DeferredAudio;audio.text="not supported yet";
     pages.present(base,audio);const auto underlay=pages.frame();
     XeenPresentationRequest request;request.kind=XeenPresentationKind::RewardReceipt;
     request.response=XeenPresentationResponseRequirement::Acknowledgment;request.treasure=treasure;request.treasure->fullPacks=true;
     pages.present(underlay,request);
     const auto modal=drawXeenTreasure(dos,underlay,font,treasure,0,true);
     check(pages.frame().pixels==modal.pixels,"modal full-pack warning takes precedence over passive audio notice");
     pages.rebase(base);check(pages.frame().pixels==modal.pixels,"rebased warning retains modal precedence");
     pages.handle(AcknowledgeAction{});
     for(int y=157;y<190;++y)for(int x=60;x<260;++x)
         check(pages.frame().pixels[y*320+x]==underlay.pixels[y*320+x],"warning dismissal restores passive unsupported notice");
 }
 check(catalog.foundName(XeenInventoryCategory::Miscellaneous,{10,37,7,0})=="potion " &&
     catalog.foundName(XeenInventoryCategory::Weapons,{38,3,0x80,0})==catalog.foundName(XeenInventoryCategory::Weapons,{0,3,0,0}),
     "found names are original base/container names without inventory decorations");
 {
     // Equip refusals format the base names (DAT 0x37a9b..0x37b28, 0x3c564..0x3c5d4):
     // "\b " backs over the trailing space, while NOT_PROFICIENT keeps it before "!".
     const auto axe=catalog.foundName(XeenInventoryCategory::Weapons,{1,29,0,0});
     const auto spear=catalog.foundName(XeenInventoryCategory::Weapons,{0,17,0,0});
     const auto conflict=xeenDialogFormat(xeenDialogText(dos,XeenDialogText::RemoveToEquip),{axe,spear});
     check(conflict=="\x03""c\x0b""007You must remove great axe to equip spear \x08 !","original equip-conflict text");
     const auto proficiency=xeenDialogFormat(xeenDialogText(dos,XeenDialogText::NotProficient),
         {std::string(dos.table("CLASS_NAMES").at(static_cast<unsigned>(XeenCharacterClass::Sorcerer))),catalog.foundName(XeenInventoryCategory::Weapons,{0,1,0,0})});
     check(proficiency=="\t000\x0b""007\x03""cSorcerers are not proficient with a long sword !","original proficiency text");
     check(drawXeenErrorScroll(base,font,conflict).isValid() && drawXeenErrorScroll(base,font,proficiency).isValid(),"original equip refusal layout");
 }
 // Pure composition only: every executed Clouds frame uses actual resources.
 const int deathY[]{196,187,179,169,159,147,138,127,113,101,86,73,60,48,36,23,10};
 for(unsigned even=0;even<=34;even+=2) {
     auto oracle=base;const auto sheet="fire"+std::to_string(even/10+1)+".vga";
     draw(oracle,sheet.c_str(),even%10,0,0);draw(oracle,sheet.c_str(),even%10+1,160,0);
     for(unsigned pair=0;pair<even/2;++pair) {draw(oracle,"death.vga",pair*2,0,deathY[pair]);draw(oracle,"death.vga",pair*2+1,160,deathY[pair]);}
     check(drawXeenDeathFire(base,even,draw).pixels==oracle.pixels,"death fire sheets, accumulated pairs and DOS Y table");
 }
 auto final=drawXeenDeathFire(base,34,draw);auto finalOracle=final;
 draw(finalOracle,"death.vga",34,0,0);draw(finalOracle,"death.vga",35,160,0);
 final=drawXeenDeathFinal(final,draw);check(final.pixels==finalOracle.pixels,"final death pair 34/35");
 check(drawXeenDeathLaugh(final,{},draw).pixels==final.pixels,"laughter starts from bare saved final image");
 for(unsigned index=0;index<=21;++index) {
     auto oracle=final;draw(oracle,"death1.vga",index,0,0);
     check(drawXeenDeathLaugh(final,index,draw).pixels==oracle.pixels,"death1 executed frames 0..21 restore saved image");
 }
 bool rejected=false;try{drawXeenDeathLaugh(final,22,draw);}catch(const std::invalid_argument &){rejected=true;}
 check(rejected && assets.spriteFrameCount("death1.vga")==23,"death1 capacity 23 does not admit frame 22");
 for(unsigned invalid:{1u,36u}) {rejected=false;try{drawXeenDeathFire(base,invalid,draw);}catch(const std::invalid_argument &){rejected=true;}check(rejected,"death fire rejects unexecuted pair");}
 const auto expected=xeenDialogFormat(dos.scalar("COMBAT_DETAILS"),{
     xeenDialogFormat(dos.scalar("COMBAT_ROW_1"),{"15","A"}),
     xeenDialogFormat(dos.scalar("COMBAT_ROW_2"),{"9","B"}),
     xeenDialogFormat(dos.scalar("COMBAT_ROW_3"),{"32","C"})});
 check(xeenCombatPanelText(dos,rows)==expected,"combat DAT text/control stream");
 XeenTextRenderOptions combatOptions;combatOptions.originalControls=true;combatOptions.drawWindow=true;
 combatOptions.bounds={233,9,312,65};combatOptions.windowBounds={225,1,320,73};combatOptions.x=233;combatOptions.y=9;
 auto combatExpected=XeenTextRenderer(font).render(base,expected,combatOptions).pages.front();
 draw(combatExpected,"combat.icn",32,233,37);
 check(drawXeenCombatPanel(dos,base,font,rows,1,draw).pixels==combatExpected.pixels,"combat window/marker/colors");
 for(unsigned row=0;row<3;++row) {
     for(int x:{239,311})for(int y:{27+10*int(row),36+10*int(row)})
         check(std::get<SelectInventorySlotAction>(*xeenMainScreenClick(x,y,MainScreen::Combat)).slot==row,"DOS row hitbox corner");
     check(!xeenMainScreenClick(238,27+10*row,MainScreen::Combat) &&
         !xeenMainScreenClick(312,27+10*row,MainScreen::Combat),"DOS row hitbox exclusive X edge");
 }
 check(!xeenMainScreenClick(239,57,MainScreen::Combat),"DOS row final exclusive Y edge");
 // Original NPC template, window 11 and actual frame.fac/face assets.
 XeenPresentationRequest npc;npc.kind=XeenPresentationKind::NpcAcknowledgment;
 npc.response=XeenPresentationResponseRequirement::Acknowledgment;
 npc.npc=XeenEventNpc{0,1,9,1,0};npc.title="A";npc.text="B";
 XeenEventPresenter presenter(font,[&](IndexedFrame &f,std::uint8_t id,std::size_t phase){assets.drawNpc(f,id,phase);});
 presenter.originalAssets(&dos,draw);
 XeenTextRenderOptions npcOptions;npcOptions.originalControls=true;npcOptions.drawWindow=true;npcOptions.stopAtBottom=true;
 npcOptions.windowBounds={8,8,224,140};npcOptions.bounds={16,16,216,132};npcOptions.x=npcOptions.y=16;
 auto npcExpected=XeenTextRenderer(font).render(base,xeenDialogFormat(dos.scalar("NPC_DIALOG"),{"A","B"}),npcOptions).pages.front();
 assets.drawNpc(npcExpected,9,0);
 check(presenter.present(base,npc).frame.pixels==npcExpected.pixels,"NPC DAT template/window/portrait placement");

 for(unsigned i=0;i<6;++i){auto &c=p.roster.at(p.party.activeRosterIds()[i]);c.permanentLevel=3;
  const auto offset=c.rosterId*354;const auto *d=c.originalDetails();check(d,"original character metadata absent");
  for(unsigned r=0;r<6;++r)check(d->resistances[r][0]==bytes[offset+311+r*2] && d->resistances[r][1]==bytes[offset+312+r*2],"original resistance bytes");
  for(unsigned skill=0;skill<18;++skill)check(c.skills[skill]==bytes[offset+39+skill],"original skill bytes");
  check(XeenCharacterRules::sheetStat(c,nullptr,0,{610})==bytes[offset+20]+bytes[offset+21],"prepared-party Might");
  auto frame=drawXeenSheet(assets.uiText(),base,font,p,i,0,false,draw);check(frame.isValid(),"original sheet layout");
  for(unsigned cell=0;cell<20;++cell)if(cell!=14)check(drawXeenPopup(frame,font,xeenSheetPopup(assets.uiText(),p,i,cell)).isValid(),"original stat popup layout");
  XeenInventorySelection selection;selection.mode=XeenInventoryMode::Browse;selection.source=i;selection.sourceOwner=c.rosterId;
  for(unsigned category=0;category<4;++category){selection.category=static_cast<XeenInventoryCategory>(category);check(drawXeenItems(assets.uiText(),base,font,catalog,p,selection,draw).isValid(),"original item layout");}
  {const auto skills=c.skills;c.skills.fill(0);
   const auto before=xeenSheetPopup(assets.uiText(),p,i,13).text;c.skills[4]=255;
   const auto after=xeenSheetPopup(assets.uiText(),p,i,13).text;
   check(after!=before && after.find(std::string(assets.uiText().table("SKILL_NAMES")[4]))!=std::string::npos,"sheet did not read mutable Cartography");
   c.skills=skills;}
  if(c.characterClass==XeenCharacterClass::Knight || c.characterClass==XeenCharacterClass::Ranger) {
   // Compare the full title band against the same generated template without
   // the separate right-hand charges column. No literal game text is embedded.
   std::vector<std::string> args{"\x03l",std::string(xeenDialogText(assets.uiText(),XeenDialogText::MiscCategory)),c.name,xeenClassName(c.characterClass),""};
   args.resize(14);
   XeenTextRenderOptions options;options.originalControls=true;options.drawWindow=true;
   options.windowBounds={0,0,320,108};options.bounds={8,8,312,100};options.x=8;options.y=8;
   const auto reference=XeenTextRenderer(font).render(base,xeenDialogFormat(xeenDialogText(assets.uiText(),XeenDialogText::ItemsTitle),args),options).pages.front();
   selection.category=XeenInventoryCategory::Miscellaneous;
   const auto items=drawXeenItems(assets.uiText(),base,font,catalog,p,selection,draw);
   auto badArgs=args;badArgs[4]=std::string(xeenDialogText(assets.uiText(),XeenDialogText::Charges));badArgs[4].insert(2,1,'r');
   const auto stray=XeenTextRenderer(font).render(base,xeenDialogFormat(xeenDialogText(assets.uiText(),XeenDialogText::ItemsTitle),badArgs),options).pages.front();
   bool exposesStray=false;
   for(int y=8;y<18;++y)for(int x=8;x<255;++x)
   {
    check(items.pixels[y*320+x]==reference.pixels[y*320+x],"stray glyph after item-title class name");
    exposesStray|=stray.pixels[y*320+x]!=reference.pixels[y*320+x];
   }
   check(exposesStray,"title regression did not detect injected stray glyph");
  }
 }
 XeenTextRenderOptions controls;controls.originalControls=true;controls.bounds={8,8,312,100};controls.x=8;controls.y=8;
 const auto controlFrame=[&](const std::string &text){return XeenTextRenderer(font).render(base,text,controls).pages.front().pixels;};
 check(controlFrame("\t  4\f dZ")==controlFrame("\t004\f00Z"),"fontAtoi space/default-color parameter consumption");
 {
  // The sheet selection blink alternates view.icn 48/49 at the cursor cell (unchanged, not retimed).
  struct Call{std::string name;unsigned frame;int x,y;};
  std::vector<Call> calls;
  const XeenDialogSpriteDraw record=[&](IndexedFrame &f,const char *name,unsigned id,int x,int y){calls.push_back({name,id,x,y});draw(f,name,id,x,y);};
  constexpr int cellX[]{9,60,111,176};
  for(unsigned cursor=0;cursor<=20;++cursor)for(const bool blink:{false,true}) {
   calls.clear();drawXeenSheet(assets.uiText(),base,font,p,0,cursor,blink,record);
   unsigned markers=0;
   for(const auto &call:calls)if(call.name=="view.icn" && (call.frame==48 || call.frame==49)) {
    ++markers;check(cursor<20 && call.frame==(blink?49u:48u) && call.x==cellX[cursor/5] && call.y==23+23*int(cursor%5),"sheet blink marker frame/cell");
   }
   check(markers==(cursor<20?1u:0u),"sheet marker count");
  }
  check(assets.spriteFrameCount("view.icn")==50,"view.icn carries frames 48/49");
 }
 {
  // DOSBox shows "Cursed" in yellow and "Broken" in red in the Items dialog:
  // the pin's \f09 and \f32 prefixes, through the original font palette.
  assets.loadPalette("mm4.pal");const auto palette=assets.snapshot().palette;
  auto &c=p.roster.at(p.party.activeRosterIds()[0]);const auto weapons=c.weapons;
  XeenInventorySelection selection;selection.mode=XeenInventoryMode::Browse;selection.source=0;selection.sourceOwner=c.rosterId;
  selection.category=XeenInventoryCategory::Weapons;
  const auto colors=[&](XeenItem first,XeenItem second) {
   c.weapons={};c.weapons[0]=first;c.weapons[1]=second;
   const auto frame=drawXeenItems(assets.uiText(),base,font,catalog,p,selection,draw);
   std::set<unsigned> used;for(int y=8;y<100;++y)for(int x=8;x<312;++x)used.insert(frame.pixels[y*320+x]);
   return used;
  };
  const auto plain=colors({0,1,0,0},{0,2,0,0}),marked=colors({0,1,0x40,0},{0,2,0x80,0});
  bool yellow=false,red=false;
  for(unsigned index:marked)if(!plain.count(index)) {
   const unsigned r=palette[index*3],g=palette[index*3+1],b=palette[index*3+2];
   yellow|=r>150 && g>150 && b<80;red|=r>150 && g<100 && b<100;
  }
  check(yellow && red,"Items dialog Cursed/Broken prefixes not yellow/red");
  c.weapons=weapons;
  // DOSBox Discard prompt: "Broken" red, the rest of the name white (color 4),
  // and a plain item's whole name white.
  // Only the pixels that differ from the same prompt with the uncolored name.
  const auto prompt=[&](XeenItem item) {
   const auto description=catalog.describe(XeenInventoryCategory::Weapons,item);
   const auto confirm=[&](const std::string &name) {
    return drawXeenConfirm(base,font,xeenDialogFormat(xeenDialogText(assets.uiText(),XeenDialogText::PermanentlyDiscard),{name}),false,draw);
   };
   const auto colored=confirm(description.coloredName(4)),plain=confirm(description.displayName);
   bool white=false,crimson=false;
   for(std::size_t i=0;i<colored.pixels.size();++i)if(colored.pixels[i]!=plain.pixels[i]) {
    const unsigned index=colored.pixels[i];
    const unsigned r=palette[index*3],g=palette[index*3+1],b=palette[index*3+2];
    white|=r>200 && g>200 && b>200;crimson|=r>150 && g<100 && b<100;
   }
   return std::pair{white,crimson};
  };
  check(prompt({0,2,0x80,0})==std::pair{true,true} && prompt({0,2,0,0})==std::pair{true,false},
   "Discard prompt item colors differ from DOS");
 }
 check(p.food==(pty[618]|(pty[619]<<8)),"original food bytes");
 check(p.food==90,"Supplied original initial food is 90");
 for(const auto text:{XeenDialogText::ExchangingInCombat,XeenDialogText::CursedItem,XeenDialogText::Hurry,XeenDialogText::UseInCombat,XeenDialogText::CannotCastEngaged,XeenDialogText::PartyNeedsRest})
  check(drawXeenErrorScroll(base,font,text==XeenDialogText::CannotCastEngaged?xeenDialogFormat(xeenDialogText(assets.uiText(),text),{"X"}):std::string(xeenDialogText(assets.uiText(),text))).isValid(),"original error scroll layout");
 for(unsigned action=0;action<4;++action)check(drawXeenItemSelection(assets.uiText(),base,font,action,draw).isValid(),"original Which item layout");
 check(drawXeenItemTarget(assets.uiText(),base,font).isValid(),"original On Who layout");
 const auto discard=xeenDialogFormat(xeenDialogText(assets.uiText(),XeenDialogText::PermanentlyDiscard),{"Dagger"});
 const auto buy=xeenDialogFormat(xeenDialogText(assets.uiText(),XeenDialogText::BuyForGold),{"Dagger","100"});
 for(const bool large:{false,true}) {
  check(drawXeenConfirm(base,font,discard,large,draw).isValid(),"original Discard Confirm layout");
  check(drawXeenConfirm(base,font,buy,large,draw).isValid(),"original Buy Confirm layout");
 }
}
}
int main(int argc,char **argv){try{hits();rules();if(argc>1)original(argv[1]);std::cout<<"Original dialog hit tables, metadata, formulas and layouts passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
