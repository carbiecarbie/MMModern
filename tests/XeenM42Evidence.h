#ifndef MMODERN_TEST_M42_EVIDENCE_H
#define MMODERN_TEST_M42_EVIDENCE_H
#include "XeenM40Evidence.h"
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenCombat.h"
#include <array>
#include <functional>
#include <limits>
// Independent synchronous oracle for DOS reason-0 merchant generation.
// Draws and comparisons: WORLD/XEEN.DAT 0x25ca8..0x26382. Numeric tables:
// ScummVM developers, GPL-3.0-or-later, revision 6814ee9. It shares no
// production generator/validator data.
namespace m42_test {
using namespace mmodern;
using m40_test::check;
struct StockOracle {
    std::uint32_t state;std::uint64_t count;
    std::string trace;
    // Scripted legal draws for exhaustive mapping; records exact requests independently.
    std::function<unsigned(unsigned,unsigned)> scripted;
    std::vector<XeenCombatRandom::Draw> requests;
    unsigned draw(unsigned lo,unsigned hi) {
        if(scripted) {const auto n=scripted(lo,hi);check(n>=lo && n<=hi,"M71 illegal oracle draw");requests.push_back({lo,hi,n});return n;}
        const std::uint32_t span=hi-lo+1,threshold=(0u-span)%span;
        for(;;) {
            check(state && count<std::numeric_limits<std::uint64_t>::max(),"M42 oracle cursor exhausted");
            state^=state<<13;state^=state>>17;state^=state<<5;++count;
            const bool accepted=state>=threshold;
            trace+="DRAW "+std::to_string(lo)+":"+std::to_string(hi)+":"+
                (accepted?std::to_string(lo+state%span):"rejected")+":"+std::to_string(state)+":"+std::to_string(count)+"\n";
            if(accepted)return lo+state%span;
        }
    }
    std::pair<unsigned,XeenItem> item(unsigned level,std::optional<int> type=std::nullopt) {
        const auto c=draw(1,100),s=draw(1,level>=6?80:100);
        unsigned category=0,id=0;
        if(type) {category=*type<35?0:*type<49?1:*type<60?2:3;id=unsigned(*type-(category==0?0:category==1?35:category==2?49:60));}
        else if(c<=(level==1?40u:35u)) {
            const unsigned lo=s<=30?1:s<=60?7:s<=85?18:30,hi=s<=30?6:s<=60?17:s<=85?29:33;
            id=draw(lo,hi);
        } else if(level==1) {category=c<=85?1:3;id=draw(1,category==1?7:9);}
        else if(c<=60) {category=1;id=s>70?8:draw(1,7);}
        else if(s<=10) {category=1;id=9;}
        else if(s<=20) {category=1;id=13;}
        else if(s<=35) {category=2;id=1;}
        else if(s<=45) {category=1;id=10;}
        else if(s<=55) {category=1;id=draw(11,12);}
        else if(s<=65) {category=2;id=2;}
        else if(s<=75) {category=2;id=draw(3,7);}
        else if(s<=80) {category=2;id=draw(8,10);}
        else {category=3;id=draw(1,9);}
        XeenItem result{0,static_cast<std::uint8_t>(id),0,0};
        const auto enchant=draw(1,100);
        if(category==3) {
            constexpr unsigned ranges[7][2]={{1,15},{16,30},{31,40},{41,50},{51,60},{61,73},{61,73}};
            result.material=std::uint8_t(id);result.id=std::uint8_t(draw(ranges[level-1][0],ranges[level-1][1]));
            result.state=std::uint8_t(draw(1,8)+2);return {category,result};
        }
        if(level==1)return {category,result};
        constexpr unsigned metal[2][6][2]={{{1,4},{3,7},{4,8},{5,9},{8,9},{9,9}},{{1,4},{2,6},{4,7},{6,10},{9,13},{13,13}}};
        constexpr unsigned elemental[6][6][2]={{{1,3},{2,5},{3,6},{4,7},{5,8},{8,8}},{{1,3},{2,5},{3,6},{4,7},{6,7},{7,7}},
            {{1,2},{1,3},{2,4},{3,5},{4,5},{5,5}},{{1,2},{1,3},{2,4},{3,4},{4,5},{5,5}},{{1,3},{2,5},{3,6},{4,7},{5,8},{8,8}},{{1,1},{1,1},{1,2},{2,2},{2,3},{3,3}}};
        constexpr unsigned attribute[10][6][2]={{{1,4},{2,5},{3,6},{4,7},{6,10},{10,10}},{{1,3},{2,5},{3,6},{4,7},{5,8},{8,8}},
            {{1,3},{2,5},{3,6},{4,7},{5,8},{8,8}},{{1,3},{2,5},{3,6},{4,7},{5,8},{8,8}},{{1,2},{1,3},{2,4},{3,5},{4,6},{6,6}},
            {{1,2},{2,3},{3,4},{4,5},{5,6},{6,6}},{{1,2},{1,3},{2,4},{3,4},{4,5},{5,5}},{{1,2},{1,3},{2,4},{3,5},{4,6},{6,6}},
            {{1,2},{1,3},{2,4},{3,4},{4,5},{5,5}},{{1,2},{1,4},{3,6},{5,8},{7,10},{10,10}}};
        constexpr unsigned elementEnd[]={25,45,60,75,95,100},elementOffset[]={0,8,15,20,25,33};
        constexpr unsigned attributeEnd[]={15,25,35,50,65,80,85,90,95,100},attributeOffset[]={0,10,18,26,34,40,46,51,57,62};
        const auto select=draw(1,100);unsigned lo=0,hi=0,offset=0;
        if(enchant<=(category==2?20u:70u)) {const unsigned row=level==7?1:(select<=70?0:1);lo=metal[row][level-2][0];hi=metal[row][level-2][1];offset=36+9*row;}
        else if(enchant<=(category==2?60u:98u)) {unsigned row=0;while(select>elementEnd[row])++row;lo=elemental[row][level-2][0];hi=elemental[row][level-2][1];offset=elementOffset[row];}
        else {unsigned row=0;while(select>attributeEnd[row])++row;lo=attribute[row][level-2][0];hi=attribute[row][level-2][1];offset=58+attributeOffset[row];}
        result.material=std::uint8_t(offset+draw(lo,hi));
        if(category==0 && draw(1,20)==10)result.state=std::uint8_t(draw(1,6));
        return {category,result};
    }
    XeenServiceEconomy generate() {
        constexpr unsigned schedule[2][4][4]={{{15,5,5,5},{5,10,5,5},{0,5,10,5},{0,0,0,5}},{{10,5,0,5},{10,5,5,5},{0,5,5,10},{0,5,10,0}}};
        XeenServiceEconomy result;
        for(unsigned side=0;side<2;++side)for(unsigned shop=0;shop<4;++shop) {
            std::array<unsigned,4> inserted{};
            for(unsigned band=0;band<4;++band)for(unsigned call=0;call<schedule[side][band][shop];++call) {
                const auto generated=item(band+(side==1 && shop>=2?3:1));const auto c=generated.first;
                if(inserted[c]<8)result.wares[side][shop][c][inserted[c]++]=generated.second;
            }
        }
        return result;
    }
};
inline void sameCategory(const XeenItemCategory &actual,const XeenItemCategory &expected) {
    for(unsigned slot=0;slot<9;++slot)check(xeenSameItem(actual[slot],expected[slot]),"M42 literal physical category differs");
}
inline XeenItemCategory armorBefore() {return {{{0,6,0,0},{0,3,0,0},{0,5,0,0},{0,7,0,0},{0,1,0,0},{0,1,0,0},{0,5,0,0},{9,5,0,0},{}}};}
inline XeenItemCategory armorAfter() {return {{{0,6,0,0},{0,5,0,0},{0,7,0,0},{0,1,0,0},{0,1,0,0},{0,5,0,0},{9,5,0,0},{},{}}};}
inline XeenItemCategory weaponsBefore() {return {{{0,17,0,0},{0,10,0,0},{0,9,0,0},{0,6,0,0},{0,3,0,0},{0,29,0,0},{0,31,0,0},{},{}}};}
inline XeenItemCategory weaponsAfterTwo() {return {{{0,17,0,0},{0,10,0,0},{0,9,0,0},{0,29,0,0},{0,31,0,0},{},{},{},{}}};}
inline XeenItemCategory restockedWeaponsAfterMissile() {return {{{0,4,0,0},{0,32,0,0},{0,29,0,0},{0,25,0,0},{27,7,0,0},{40,33,0,0},{},{},{}}};}
}
#endif
