#include "formats/xeen/XeenDosText.h"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace mmodern {
namespace {
constexpr std::size_t kLoadBase=0x56c0, kDataBase=0x4d890;
constexpr unsigned kDataSegment=0x481d;
constexpr std::size_t kTokenLimit=512, kOutputLimit=65536;
[[noreturn]] void malformed(const std::string &detail) {
 throw std::invalid_argument("Unsupported or malformed English DOS UI module: "+detail);
}
unsigned word(const std::vector<std::uint8_t> &b,std::size_t at) {
 if(at>b.size() || b.size()-at<2)malformed("truncated word");
 return b[at]|(unsigned(b[at+1])<<8);
}
std::string arguments(std::string_view text) {
 std::string result;
 for(std::size_t i=0;i<text.size();++i)if(text[i]=='%') {
  if(++i==text.size())malformed("truncated format argument");
  if(text[i]=='%')continue;
  const auto begin=i;
  unsigned width=0;
  while(i<text.size() && text[i]>='0' && text[i]<='9') {
   width=width*10+unsigned(text[i++]-'0');if(width>32)malformed("format width");
  }
  if(i<text.size() && text[i]=='l')++i;
  if(i==text.size() || std::string_view("sduic").find(text[i])==std::string_view::npos)malformed("format conversion");
  if(!result.empty())result+=',';
  result+=text.substr(begin,i-begin+1);
 }
 return result;
}
}
const std::vector<XeenDosText::Field> &XeenDosText::layout() {
 // Bounds and far-pointer relationships were checked against the uncompressed
 // DOS module; no original strings or whole-file digest are compiled here.
 static const std::vector<Field> fields{
  {"ON_WHO",0x54108,0x54117,1,false,""},
  {"IN_NO_CONDITION",0x50aae,0x50ae1,1,false,"s"},
  {"THE_PARTY_NEEDS_REST",0x53b2b,0x53b45,1,false,""},
  {"REST_COMPLETE",0x53193,0x531ce,1,false,"s,d"},
  {"PARTY_IS_STARVING",0x531ce,0x531ea,1,false,""},
  {"HIT_SPELL_POINTS_RESTORED",0x531ea,0x5320a,1,false,""},
  {"TOO_DANGEROUS_TO_REST",0x5312c,0x53148,1,false,""},
  {"SOME_CHARS_MAY_DIE",0x53148,0x53169,1,false,""},
  {"GOOD",0x51a13,0x51a18,1,false,""},
  {"BLACKSMITH_TEXT",0x52c3c,0x52c9e,1,false,"s,s"},
  {"TEMPLE_TEXT",0x52c9e,0x52d37,1,false,"s,lu,lu,s,s"},
  {"EXPERIENCE_FOR_LEVEL",0x52b79,0x52b9f,1,false,"s,lu,u"},
  {"TRAINING_LEARNED_ALL",0x52b9f,0x52bc0,1,false,"s"},
  {"ELIGIBLE_FOR_LEVEL",0x52bc0,0x52bf1,1,false,"s,d,lu"},
  {"TRAINING_TEXT",0x52bf1,0x52c3c,1,false,"s,s"},
  {"NOT_ENOUGH_X_IN_THE_Y",0x5320a,0x5322b,1,false,"s,s"},
  {"CHARACTER_DETAILS",0x55a1a,0x55b71,1,false,"s,s,s,s,s,02u,u,02u,u,02u,d,lu,02u,u,02u,u,02u,u,lu,02u,u,02u,u,u,lu,02u,u,02u,u,u,u,c,02u,u,02u,u,u,02u,s,s,s,s,s"},
  {"PARTY_GOLD",0x55b71,0x55b7c,1,false,""},
  {"CHARACTER_TEMPLATE",0x5559d,0x556a7,1,false,"s"},
  {"EXCHANGING_IN_COMBAT",0x556a7,0x556d6,1,false,""},
  {"CURRENT_MAXIMUM_RATING_TEXT",0x5500e,0x55049,1,false,"s,lu,lu,s"},
  {"CURRENT_MAXIMUM_TEXT",0x550f1,0x5511e,1,false,"s,lu,lu"},
  {"CURRENT_MAXIMUM_SIGNED_TEXT",0x550c5,0x550f1,1,false,"s,d,lu"},
  {"AGE_TEXT",0x55049,0x55085,1,false,"s,u,u,u,u"},
  {"LEVEL_TEXT",0x55085,0x550c5,1,false,"s,u,u,u,s"},
  {"RESISTENCES_TEXT",0x5511e,0x5518c,1,false,"s,u,u,u,u,u,u"},
  {"NONE",0x5519e,0x551a8,1,false,""},
  {"EXPERIENCE_TEXT",0x551e2,0x55215,1,false,"s,lu,s"},
  {"ELIGIBLE",0x551d4,0x551e2,1,false,""},
  {"IN_PARTY_IN_BANK",0x55215,0x5523c,1,false,"s,lu,lu"},
  {"FOOD_TEXT",0x5523c,0x5526a,1,false,"s,u,u,s"},
  {"DAY_PLURAL",0x5500c,0x5500e,1,false,""},
  {"DAY_SINGULAR",0x5500d,0x5500e,1,false,""},
  {"ITEMS_DIALOG_TEXT1",0x55701,0x55760,1,false,"s,s,s,s"},
  {"ITEMS_DIALOG_LINE1",0x557d3,0x557ee,1,false,"02u,2d,s"},
  {"ITEMS_DIALOG_LINE2",0x557ee,0x55812,1,false,"02u,2d,s,lu"},
  {"BTN_USE",0x55760,0x55769,1,false,""},
  {"NO_ITEMS_AVAILABLE",0x55812,0x5582d,1,false,""},
  {"X_FOR_THE_Y",0x5582d,0x55868,1,false,"s,s,s,s,s,s,s,s,s,s,s,s,s,s"},
  {"X_FOR_Y",0x558be,0x558fa,1,false,"s,s,s,s,s,s,s,s,s,s,s,s"},
  {"FMT_CHARGES",0x556d6,0x556e6,1,false,""},
  {"AVAILABLE_GOLD_COST",0x5586d,0x558be,1,false,"s,s,lu,s,s,s,s,s,s,s,s,s"},
  {"COST",0x55902,0x55907,1,false,""},
  {"WHICH_ITEM",0x543da,0x543f1,1,false,"s"},
  {"WHATS_YOUR_HURRY",0x543f1,0x54427,1,false,""},
  {"USE_ITEM_IN_COMBAT",0x54427,0x5446a,1,false,""},
  {"NO_SPECIAL_ABILITIES",0x5446a,0x5448e,1,false,"s"},
  {"CANT_CAST_WHILE_ENGAGED",0x54117,0x5413a,1,false,"s"},
  {"EQUIPPED_ALL_YOU_CAN",0x54fad,0x54fda,1,false,"s"},
  {"REMOVE_X_TO_EQUIP_Y",0x54fe5,0x5500c,1,false,"s,s"},
  {"RING",0x54fda,0x54fdf,1,false,""},
  {"MEDAL",0x54fdf,0x54fe5,1,false,""},
  {"CANNOT_REMOVE_CURSED_ITEM",0x5594b,0x5596e,1,false,""},
  {"PERMANENTLY_DISCARD",0x5435f,0x54383,1,false,"s"},
  {"BACKPACK_IS_FULL",0x5451d,0x5453c,1,false,"s"},
  {"BUY_X_FOR_Y_GOLD",0x5453c,0x5455f,1,false,"s,lu"},
  {"ITEM_NOT_BROKEN",0x544d8,0x544f3,1,false,""},
  {"FIX_IDENTIFY_GOLD",0x55cbf,0x55cdf,1,false,"s,s,lu"},
  {"NOT_PROFICIENT",0x55c93,0x55cbf,1,false,"s,s"},
  {"CATEGORY_BACKPACK_IS_FULL",0x5596e,0x55a1a,4,false,"s"},
  {"FIX_IDENTIFY",0x55cdf,0x55cec,2,false,""},
  {"RACE_NAMES",0x50724,0x50738,5,true,""},
  {"SEX_NAMES",0x50744,0x5074c,2,true,""},
  {"CONDITION_NAMES",0x50750,0x50794,17,true,""},
  {"STAT_NAMES",0x50794,0x507d4,16,true,""},
  {"CONSUMABLE_NAMES",0x507d4,0x507e4,4,true,""},
  {"SKILL_NAMES",0x507e4,0x5082c,18,true,""},
  {"RATING_TEXT",0x5082c,0x5088c,24,true,""},
  {"CLASS_NAMES",0x500fa,0x50126,11,true,""},
  {"CATEGORY_NAMES",0x500ea,0x500fa,4,true,""},
  {"ITEM_ACTIONS",0x500ce,0x500ea,7,true,""},
  {"WHERE_NAMES",0x50234,0x5023c,2,true,""},
  {"BTN_EQUIP",0x500ae,0x500b2,1,true,""},
  {"BTN_REMOVE",0x500b2,0x500b6,1,true,""},
  {"BTN_DISCARD",0x500b6,0x500ba,1,true,""},
  {"BTN_QUEST",0x500ba,0x500be,1,true,""},
  {"BTN_BUY",0x500be,0x500c2,1,true,""},
  {"BTN_SELL",0x500c2,0x500c6,1,true,""},
  {"BTN_IDENTIFY",0x500c6,0x500ca,1,true,""},
  {"BTN_FIX",0x500ca,0x500ce,1,true,""},
  {"WORLD_MENU",0x52023,0x52086,1,false,""},
  {"WORLD_MENU_TITLE",0x52041,0x52086,1,false,""},
  {"CREDITS_RESOURCE",0x5208f,0x5209b,1,false,""},
  {"MENU_SPRITE",0x520a7,0x520b2,1,false,""},
  {"LOAD_LABEL",0x537ab,0x537b0,1,false,""},
  {"SAVE_LABEL",0x537b6,0x537bb,1,false,""},
  {"OVERWRITE_CONFIRM",0x537c1,0x537d5,1,false,"s"},
  {"NAME_PROMPT",0x52002,0x52023,1,false,""},
  {"CREDITS_BACKGROUND",0x52086,0x5208f,1,false,""},
  {"OTHER_BUTTONS",0x5209b,0x520a7,1,false,""},
  {"TITLE_ANIMATIONS",0x520a7,0x520c8,3,false,""},
  {"TITLE_BUTTONS",0x520c8,0x520d2,1,false,""},
  {"TITLE_PALETTE",0x520f6,0x520ff,1,false,""},
  {"TITLE_BACKGROUND",0x520ff,0x52109,1,false,""},
  {"NO_SAVES",0x52135,0x52159,1,false,""},
  {"OPTIONS_TEMPLATE",0x52159,0x521ba,1,false,"s,03d"},
  {"WORLD_LABEL",0x521ba,0x521c0,1,false,""},
  {"SAVED_NOTICE",0x532cf,0x53305,1,false,"s"},
  {"SAVE_AS_SPACE",0x5346a,0x534b2,1,false,""},
  {"DOS_SLOT_PATTERN",0x536a2,0x536af,1,false,"02d"},
  {"SLOT_DETAILS",0x536af,0x536c7,1,false,"c,u"},
  {"DOS_FILE_PATTERN",0x536c7,0x536d4,1,false,"02d"},
  {"CHOOSER",0x536d4,0x537ab,1,false,"s,2u,s,s,2u,s,s,2u,s,s,2u,s,s,2u,s,s,2u,s,s,2u,s,s,2u,s,s,2u,s,s,2u,s,s,s"},
  {"START_LABEL",0x537b0,0x537b6,1,false,""},
  {"EMPTY_SLOT",0x537bb,0x537c1,1,false,""},
  {"NEW_SPACE",0x537eb,0x53833,1,false,""},
  {"DIFFICULTY_BUTTONS",0x53833,0x5383e,1,false,""},
  {"DIFFICULTY_TEXT",0x5383e,0x5385f,1,false,""},
  {"CHOOSER_SPRITES",0x53697,0x536a2,1,false,""},
  {"SAVE_RESTRICTED",0x53250,0x532b1,1,false,""},
  {"PANEL_ON",0x53305,0x5330e,1,false,""},
  {"PANEL_OFF",0x5330e,0x53318,1,false,""},
  {"PANEL_BUTTON_TEXT",0x53318,0x5336e,1,false,"s,s,s,s"},
  {"PANEL_SPRITES",0x5336e,0x53379,1,false,""},
  {"PANEL_TEXT",0x53379,0x5344c,1,false,"s,s"},
  {"NO_LOADING_IN_COMBAT",0x534bd,0x534db,1,false,""},
  {"NO_SAVING_IN_COMBAT",0x534e6,0x53503,1,false,""},
  {"CONFIRM_QUIT",0x53503,0x53522,1,false,""},
  {"MR_WIZARD",0x53522,0x5354b,1,false,""}
 };
 return fields;
}
const std::vector<XeenDosText::ButtonLayout> &XeenDosText::buttonLayouts() {
 // Verified against the installed DOS call sites: title 0xbad7..0xbb22,
 // chooser 0x20459..0x2048f, difficulty 0x20a5e..0x20a94 (file offsets).
 static const std::vector<ButtonLayout> layouts{
  {"TITLE",0x4f162,0x4f176,0x4f17e,0x4f186,0x4f18e,0x4f192,4},
  {"CHOOSER",0x4f19a,0x4f1bc,0x4f1cc,0x4f1dc,0x4f1ec,0x4f1fc,16},
  {"DIFFICULTY",0x4f690,0x4f696,0x4f698,0x4f69a,0x4f69c,0x4f69e,2},
  // DOS panel setup at 0x1eecb..0x1ef05, including Save As/Text/Speech.
  {"PANEL",0x4f45e,0x4f472,0x4f47b,0x4f484,0x4f48d,0x4f496,9}
 };return layouts;
}
void XeenDosText::validateControls(std::string_view text) {
 for(std::size_t i=0;i<text.size();) {
  const unsigned c=static_cast<unsigned char>(text[i++])&127;
  if(c>=32 || c==1 || c==2 || c==5 || c==6 || c==10 || c==13)continue;
  if(c==3 || c==8) {
   if(i==text.size())malformed("truncated alignment/outline");
   if(c==3 && text[i]!='c' && text[i]!='l' && text[i]!='r')malformed("alignment");
   ++i;continue;
  }
  if(c!=4 && c!=7 && c!=9 && c!=11 && c!=12)malformed("unsupported control");
  const unsigned digits=c==12?2:3;
  if(i<text.size() && text[i]=='%') {
   const auto begin=++i;
   if(i<text.size() && text[i]=='0')++i;
   if(i+1>=text.size() || text[i]!=char('0'+digits) || std::string_view("dui").find(text[i+1])==std::string_view::npos)
    malformed("control format argument");
   i+=2;(void)begin;continue;
  }
  if(c==12 && i<text.size() && text[i]=='d') {++i;continue;}
  unsigned number=0;
  for(unsigned d=0;d<digits;++d) {
   if(i==text.size() || (text[i]!=' ' && (text[i]<'0' || text[i]>'9')))malformed("control parameter");
   number=number*10+(text[i]==' '?0:unsigned(text[i]-'0'));++i;
  }
  if(c==12 && number>=40)malformed("palette index");
 }
}
XeenDosText::XeenDosText(const std::vector<std::uint8_t> &bytes) {
 if(bytes.size()<28 || bytes.size()>1024*1024 || word(bytes,0)!=0x5a4d)malformed("MZ header");
 const auto last=word(bytes,2),pages=word(bytes,4),relocations=word(bytes,6);
 const std::size_t load=std::size_t(word(bytes,8))*16,relocationTable=word(bytes,24);
 if(!pages || last>511)malformed("MZ image length");
 const std::size_t image=std::size_t(pages)*512-(last?512-last:0);
 if(image!=bytes.size() || load!=kLoadBase || load>=image ||
    relocationTable<28 || relocationTable>load || relocations>(load-relocationTable)/4)
  malformed("packed or incompatible MZ load image/relocations");
 const std::size_t loaded=image-load;
 const unsigned minimum=word(bytes,10),maximum=word(bytes,12);
 const std::size_t entry=std::size_t(word(bytes,22))*16+word(bytes,20);
 // SS may refer to allocated memory beyond the load image; no module is run.
 const std::size_t allocation=(loaded+15)/16*16+std::size_t(maximum)*16;
 const std::size_t stack=std::size_t(word(bytes,14))*16+word(bytes,16);
 if(minimum>maximum || entry>=loaded || stack>allocation)malformed("MZ entry/allocated stack bounds");
 std::set<std::size_t> relocated;
 for(unsigned i=0;i<relocations;++i) {
  const auto at=relocationTable+i*4;
  const std::size_t target=load+std::size_t(word(bytes,at+2))*16+word(bytes,at);
  if(target<load || target>image || image-target<2 || !relocated.insert(target).second)malformed("relocation extent/duplicate");
 }
 std::size_t total=0;
 const auto token=[&](std::size_t at,std::size_t end,const Field &field,unsigned index) {
  if(at<kDataBase || at>=end || end>image)malformed(std::string(field.name)+" field extent");
  const auto begin=at;
  while(at<end && bytes[at]) {if(at-begin==kTokenLimit)malformed("token limit");++at;}
  if(at==end)malformed(std::string(field.name)+" terminator");
  std::string value(reinterpret_cast<const char *>(bytes.data()+begin),at-begin);
  const bool emptyAllowed=std::string_view(field.name)=="DAY_SINGULAR" || (std::string_view(field.name)=="CLASS_NAMES" && index==10);
  if(value.empty() && !emptyAllowed)malformed(std::string(field.name)+" empty field");
  if(arguments(value)!=field.arguments)malformed(std::string(field.name)+" format arguments");
  validateControls(value);total+=value.size();if(total>kOutputLimit)malformed("aggregate output bound");
  return std::pair<std::string,std::size_t>{std::move(value),at+1};
 };
 for(const auto &field:layout()) {
  std::vector<std::string> values;
  if(field.begin>=field.end || field.end>image || !field.count || field.count>24)malformed("named field extent/count");
  auto at=field.begin;
  for(unsigned i=0;i<field.count;++i) {
   if(field.pointers) {
    if(field.end-field.begin!=field.count*4 || !relocated.count(at+2) || word(bytes,at+2)!=kDataSegment)malformed(std::string(field.name)+" table relocation");
    const auto target=kDataBase+word(bytes,at);
    const auto bounds=targets(field.name).at(i);
    if(target!=bounds.first)malformed(std::string(field.name)+" pointer relationship");
    auto value=token(target,bounds.second,field,i);
    if(value.second!=bounds.second)malformed(std::string(field.name)+" ambiguous table string");
    values.push_back(std::move(value.first));at+=4;
   } else {
    auto value=token(at,field.end,field,i);values.push_back(std::move(value.first));at=value.second;
   }
  }
  if(at!=field.end)malformed(std::string(field.name)+" ambiguous/trailing field bytes");
  if(!_fields.emplace(field.name,std::move(values)).second)malformed("duplicate named field");
 }
 for(const auto &layout:buttonLayouts()) {
  std::vector<Button> buttons;
  for(unsigned i=0;i<layout.count;++i) {
   for(const auto at:{layout.y,layout.width,layout.height,layout.key,layout.painted})
    if(at+i>=image)malformed("button table extent");
   Button button{word(bytes,layout.x+i*2),bytes[layout.y+i],bytes[layout.width+i],bytes[layout.height+i],bytes[layout.key+i],bytes[layout.painted+i]!=0};
   if(button.x>=320 || button.y>=200 || !button.width || !button.height || button.x+button.width>320 || button.y+button.height>200 || !button.key || bytes[layout.painted+i]>1)
    malformed(std::string(layout.name)+" button bounds/key/paint");
   buttons.push_back(button);
  }
  // Title has a second non-World table following its terminator; the other
  // admitted tables end exactly at their sentinel.
  if(word(bytes,layout.x+layout.count*2)!=65535)malformed("button table sentinel");
  _buttons.emplace(layout.name,std::move(buttons));
 }
 // DOS Other Options reuses the title's first two positions, but constructs
 // its hotkey array on the stack at 0xc2ce/0xc2e7 rather than in a data table.
 auto other=_buttons.at("TITLE");other.resize(2);
 for(unsigned i=0;i<other.size();++i) {
  const auto at=kOtherButtonKeyOffsets[i];
  if(at>=image || bytes[at-3]!=0xc6 || bytes[at-2]!=0x46 || bytes[at-1]!=0xf0+i || !bytes[at])
   malformed("Other Options button key script");
  other[i].key=bytes[at];
 }
 _buttons.emplace("OTHER",std::move(other));
}
const std::vector<XeenDosText::Button> &XeenDosText::buttons(std::string_view name) const {
 const auto it=_buttons.find(std::string(name));if(it==_buttons.end())throw std::out_of_range("Unknown DOS button table");return it->second;
}
const std::vector<std::string> &XeenDosText::table(std::string_view name) const {
 const auto it=_fields.find(std::string(name));if(it==_fields.end())throw std::out_of_range("Unknown DOS UI field");
 return it->second;
}
std::string_view XeenDosText::scalar(std::string_view name) const {
 const auto &values=table(name);if(values.size()!=1)throw std::invalid_argument("DOS UI field is a table");
 return values.front();
}
const std::vector<std::pair<std::size_t,std::size_t>> &XeenDosText::targets(std::string_view name) {
 static const std::unordered_map<std::string,std::vector<std::pair<std::size_t,std::size_t>>> bounds{
 {"RACE_NAMES",{{0x51bbb,0x51bc1},{0x51bc1,0x51bc5},{0x51bc5,0x51bcb},{0x51bcb,0x51bd1},{0x51bd1,0x51bd7}}},
 {"SEX_NAMES",{{0x51bd7,0x51bdc},{0x51bdc,0x51be3}}},
 {"CONDITION_NAMES",{{0x51be3,0x51bea},{0x51bea,0x51bf7},{0x5184c,0x51851},{0x51bf7,0x51c00},{0x51c00,0x51c09},{0x51802,0x51809},{0x51c09,0x51c11},{0x51c11,0x51c17},{0x51809,0x51810},{0x51c17,0x51c21},{0x51c21,0x51c2a},{0x51c2a,0x51c34},{0x51c34,0x51c40},{0x5185c,0x51861},{0x51861,0x51867},{0x51c40,0x51c4b},{0x51a13,0x51a18}}},
 {"STAT_NAMES",{{0x51975,0x5197b},{0x5197b,0x51985},{0x51985,0x51991},{0x51c4b,0x51c55},{0x51991,0x51997},{0x51997,0x519a0},{0x519a0,0x519a5},{0x51c55,0x51c59},{0x51c59,0x51c5f},{0x519b0,0x519bc},{0x519a5,0x519b0},{0x517c6,0x517d3},{0x51c5f,0x51c6b},{0x51c6b,0x51c72},{0x51c72,0x51c79},{0x51c79,0x51c84}}},
 {"CONSUMABLE_NAMES",{{0x518f2,0x518f7},{0x517d3,0x517d8},{0x51c84,0x51c89},{0x51c89,0x51c93}}},
 {"SKILL_NAMES",{{0x51c93,0x51ca0},{0x51ca0,0x51cac},{0x51cac,0x51cb7},{0x51cb7,0x51cc4},{0x51cc4,0x51cd1},{0x51cd1,0x51cda},{0x51cda,0x51cea},{0x51cea,0x51cf3},{0x51cf3,0x51cfc},{0x51cfc,0x51d08},{0x51d08,0x51d12},{0x51d12,0x51d1e},{0x51d1e,0x51d2c},{0x51d2c,0x51d3c},{0x51d3c,0x51d44},{0x51d44,0x51d4c},{0x51d4c,0x51d5e},{0x51d5e,0x51d6b}}},
 {"RATING_TEXT",{{0x51d6b,0x51d77},{0x51d77,0x51d81},{0x51d7c,0x51d81},{0x51d81,0x51d8a},{0x51d86,0x51d8a},{0x51d8a,0x51d92},{0x51a13,0x51a18},{0x51d92,0x51d9c},{0x51d9c,0x51da1},{0x51da1,0x51dab},{0x51dab,0x51db1},{0x51db1,0x51db7},{0x51db7,0x51dbf},{0x51dbf,0x51dca},{0x51dca,0x51dd3},{0x51dd3,0x51ddd},{0x51ddd,0x51de8},{0x51de8,0x51df4},{0x51df4,0x51dff},{0x51dff,0x51e0a},{0x51e0a,0x51e13},{0x51e13,0x51e1b},{0x51e1b,0x51e29},{0x51e29,0x51e32}}},
 {"CLASS_NAMES",{{0x5191f,0x51926},{0x51926,0x5192e},{0x5192e,0x51935},{0x51935,0x5193c},{0x5193c,0x51945},{0x51945,0x5194c},{0x5194c,0x51952},{0x51952,0x5195c},{0x5195c,0x51962},{0x51962,0x51969},{0x5093a,0x5093b}}},
 {"CATEGORY_NAMES",{{0x518f7,0x518ff},{0x518ff,0x51905},{0x51905,0x51911},{0x51911,0x5191f}}},
 {"ITEM_ACTIONS",{{0x518c8,0x518ce},{0x518ce,0x518d5},{0x518d5,0x518d9},{0x518d9,0x518e1},{0x518e1,0x518e9},{0x518e9,0x518f2},{0x518f2,0x518f7}}},
 {"WHERE_NAMES",{{0x51a54,0x51a5a},{0x51a5a,0x51a5f}}},
 {"BTN_EQUIP",{{0x51875,0x51880}}},{"BTN_REMOVE",{{0x51880,0x51889}}},
 {"BTN_DISCARD",{{0x51889,0x51893}}},{"BTN_QUEST",{{0x51893,0x5189e}}},
 {"BTN_BUY",{{0x5189e,0x518a7}}},{"BTN_SELL",{{0x518a7,0x518b1}}},
 {"BTN_IDENTIFY",{{0x518b1,0x518bf}}},{"BTN_FIX",{{0x518bf,0x518c8}}}
 };
 const auto it=bounds.find(std::string(name));if(it==bounds.end())throw std::out_of_range("Unknown DOS UI pointer table");return it->second;
}
}
