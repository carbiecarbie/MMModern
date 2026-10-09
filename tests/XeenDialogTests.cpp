#include "XeenTestInstallation.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include <iostream>
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
 check(details && details->skills[17]==1 && details->birthDay==7 && details->awards[0]==1 && details->awards[64]==2 && details->awards[9]==0xab && details->awards[73]==0 && details->awards[63]==3 && details->awards[127]==4,"original immutable fields offsets");
 using R=XeenCharacterRules;const XeenCharacterRulesContext context{650};
 check(R::sheetAge(c,context)==10,"sheet age");
 check(R::skillCount(c)==2 && R::awardCount(c)==5,"skill/award counts");
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
 for(unsigned i=0;i<6;++i){auto &c=p.roster.at(p.party.activeRosterIds()[i]);c.permanentLevel=3;
  const auto offset=c.rosterId*354;const auto *d=c.originalDetails();check(d,"original character metadata absent");
  for(unsigned r=0;r<6;++r)check(d->resistances[r][0]==bytes[offset+311+r*2] && d->resistances[r][1]==bytes[offset+312+r*2],"original resistance bytes");
  for(unsigned skill=0;skill<18;++skill)check(d->skills[skill]==bytes[offset+39+skill],"original skill bytes");
  check(XeenCharacterRules::sheetStat(c,nullptr,0,{610})==bytes[offset+20]+bytes[offset+21],"prepared-party Might");
  auto frame=drawXeenSheet(assets.uiText(),base,font,p,i,0,false,draw);check(frame.isValid(),"original sheet layout");
  for(unsigned cell=0;cell<20;++cell)if(cell!=14)check(drawXeenPopup(frame,font,xeenSheetPopup(assets.uiText(),p,i,cell)).isValid(),"original stat popup layout");
  XeenInventorySelection selection;selection.mode=XeenInventoryMode::Browse;selection.source=i;selection.sourceOwner=c.rosterId;
  for(unsigned category=0;category<4;++category){selection.category=static_cast<XeenInventoryCategory>(category);check(drawXeenItems(assets.uiText(),base,font,catalog,p,selection,draw).isValid(),"original item layout");}
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
