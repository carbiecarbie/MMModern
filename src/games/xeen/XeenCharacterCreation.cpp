#include "games/xeen/XeenCharacterCreation.h"
#include "games/xeen/XeenCharacterRules.h"
#include <algorithm>
#include <stdexcept>
namespace mmodern {
XeenCreateClasses xeenCreateClasses(const XeenCreateAttributes &a) {
    return {{a[0]>=15,a[0]>=13 && a[2]>=13 && a[3]>=13,a[1]>=13 && a[5]>=13,
        a[2]>=13,a[1]>=13,a[6]>=13,a[4]>=13 && a[5]>=13,a[3]>=15,
        a[1]>=15 && a[2]>=15,a[1]>=12 && a[2]>=12 && a[3]>=12 && a[4]>=12}};
}
std::optional<unsigned> xeenCreateArrow(const XeenCreateClasses &classes) {
    for(unsigned i=0;i<classes.size();++i)if(classes[i])return i;return {};
}
std::optional<unsigned> xeenMoveCreateArrow(const XeenCreateClasses &classes,std::optional<unsigned> arrow,int direction) {
    if(!arrow)return xeenCreateArrow(classes);
    for(int i=int(*arrow)+direction;i>=0 && i<int(classes.size());i+=direction)if(classes[i])return unsigned(i);
    return arrow;
}
std::size_t xeenMoveCreateSlot(std::size_t index,std::size_t count,int direction) {
    if(!count || (direction<0 && !index) || (direction>0 && index+1>=count))return index;
    return direction<0?index-1:index+1;
}
std::vector<std::uint8_t> xeenCreateSlots(const XeenRoster &roster) {
    std::vector<std::uint8_t> slots;for(unsigned i=0;i<24;++i)if(roster.at(i).name.empty())slots.push_back(i);return slots;
}
XeenCreateAttributes xeenCreateRoll(XeenCombatRandom &random) {
    XeenCreateAttributes values;
    do {
        values.fill(0);
        for(unsigned pass=0;pass<3;++pass)for(auto &v:values)v+=int(random.range(10,79))/10;
    } while(!xeenCreateArrow(xeenCreateClasses(values)));
    return values;
}
std::pair<XeenCharacter,XeenCombatInputs> xeenCreatedCharacter(std::uint8_t id,
    const std::string &name,unsigned cls,const XeenCreateAttributes &a,XeenMapIdentity town,const XeenGameplayContext &context) {
    if(id>=24 || cls>=10 || name.empty() || name.size()>10 || name.find('\0')!=std::string::npos)
        throw std::invalid_argument("Invalid created character");
    auto c=xeenDeletedCharacter(id);auto inputs=xeenDeletedCombatInputs();
    c.name=name;c.sex=xeenCreateSex(id);c.race=xeenCreateRace(id);c.characterClass=XeenCharacterClass(cls);
    c.savedMaze=town.number;c.xeenSide=static_cast<unsigned>(town.side);c.permanentLevel=1;c.birthDay=std::uint8_t(context.day);c.birthYear=std::uint16_t(context.year-18);
    inputs.might.permanent=a[0];c.intellect.permanent=a[1];c.personality.permanent=a[2];c.endurance.permanent=a[3];
    inputs.speed.permanent=a[4];inputs.accuracy.permanent=a[5];inputs.luck->permanent=a[6];
    const auto classSkill=kXeenCreateClassSkills[cls].id;
    if(classSkill>=0)c.skills[classSkill]=1;
    const auto race=unsigned(c.race);const auto raceSkill=kXeenCreateRaceSkills[race].id;
    if(raceSkill>=0)c.skills[raceSkill]=1;
    constexpr int spells[10][4]{{-1,-1,-1,-1},{21,-1,-1,-1},{22,-1,-1,-1},{21,1,14,-1},
        {22,0,25,-1},{-1,-1,-1,-1},{-1,-1,-1,-1},{-1,-1,-1,-1},{20,1,11,23},{20,1,-1,-1}};
    c.currentSpell=255;
    for(int slot:spells[cls])if(slot>=0){if(!c.hasSpells)c.currentSpell=slot;c.hasSpells=true;(*c.learnedSpells)[slot]=1;}
    constexpr int resistance[5][6]{{7,7,7,7,7,7},{0,0,0,0,5,5},{2,2,2,2,2,20},{5,5,5,20,5,0},{10,10,10,0,0,0}};
    auto &r=*inputs.resistances;r.firePermanent=resistance[race][0];r.coldPermanent=resistance[race][1];
    r.electricalPermanent=resistance[race][2];inputs.poisonResistance->permanent=resistance[race][3];
    r.energyPermanent=resistance[race][4];r.magicPermanent=resistance[race][5];
    c.currentHp=XeenCharacterRules::maxHp(c,{context.year});c.currentSp=XeenCharacterRules::maxSp(c,{context.year});
    return {std::move(c),std::move(inputs)};
}
void xeenAdvanceCreateDice(XeenCreateDice &dice) {
    constexpr int width[]{27,27,27,31,27,27,27},height[]{26,20,27,30,26,20,27};
    for(auto &d:dice) {
        d.frame=(d.frame+1)%7;d.x+=d.dx;d.y+=d.dy;
        const int right=163-width[d.frame],bottom=93-height[d.frame];
        if(d.x<13 || d.x>right){d.x=std::clamp(d.x,13,right);d.dx=-d.dx;}
        if(d.y<12 || d.y>bottom){d.y=std::clamp(d.y,12,bottom);d.dy=-d.dy;}
    }
}
}
