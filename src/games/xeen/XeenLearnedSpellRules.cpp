#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenJourneyContent.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace mmodern {
XeenMapIdentity XeenLearnedSpellRules::townPortalMap(XeenSide side,unsigned town) {
    if(town<1 || town>5)throw std::invalid_argument("Invalid Town Portal town");
    return {side,static_cast<std::uint16_t>(side==XeenSide::Darkside?27+2*town:27+town)};
}
XeenSpellTravel XeenLearnedSpellRules::travel(XeenWorld &world,const XeenCamera &camera,
        XeenLearnedSpell spell,unsigned distance,const XeenCharacter *caster,bool cloudsCompleted) {
    const auto &map=world.map(camera.mapId);
    const unsigned flag=spell==XeenLearnedSpell::LloydsBeacon?0x800:spell==XeenLearnedSpell::TownPortal?0x100:spell==XeenLearnedSpell::SuperShelter?0x200:
        spell==XeenLearnedSpell::Etherealize?0x40:spell==XeenLearnedSpell::TimeDistortion?0x400:0x1000;
    XeenSpellTravel result;
    if(map.geometry.flags&flag) {result.failed=true;return result;}
    if(spell==XeenLearnedSpell::LloydsBeacon || spell==XeenLearnedSpell::TownPortal) {
        auto to=camera;
        if(spell==XeenLearnedSpell::LloydsBeacon) {
            if(!caster)throw std::invalid_argument("Beacon requires a caster");
            std::array<std::uint8_t,4> beacon{caster->lloydMap,caster->lloydX,caster->lloydY,caster->lloydSide};
            if(!beacon[0])beacon=camera.mapId.side==XeenSide::Darkside?
                std::array<std::uint8_t,4>{29,25,21,1}:std::array<std::uint8_t,4>{28,18,4,0};
            result.beacon=beacon;
            if(distance==1) {result.beacon=std::array<std::uint8_t,4>{std::uint8_t(camera.mapId.number),std::uint8_t(int(camera.x)),std::uint8_t(int(camera.y)),std::uint8_t(camera.mapId.side)};return result;}
            if(distance!=2)return result;
            to.mapId=XeenMapIdentity(static_cast<XeenSide>(beacon[3]),beacon[0]);to.x=beacon[1];to.y=beacon[2];
            // DAT 0x2feb3..0x2fee2: saved completion, never flag 75 or title won.
            if(camera.mapId.side==XeenSide::Clouds && cloudsCompleted && to.mapId.number>=75 && to.mapId.number<=78) {
                result.failed=true;return result;
            }
            result.reload=to.mapId!=camera.mapId;
        } else {
            if(!distance)return result;
            if(distance>5) {result.unsupported=true;return result;}
            to.mapId=townPortalMap(camera.mapId.side,distance);result.reload=true;
            if(camera.mapId.side==XeenSide::Darkside) {
                // CD DAT 0x30779..0x30798 doubles the side's table stride.
                // The dialog names towns, but travel reads the following five
                // bytes as map IDs. Castleview -> Area A4 is confirmed in DOS.
                constexpr std::uint16_t maps[]{4,19,22,6,98};
                to.mapId={XeenSide::Darkside,maps[distance-1]};
                constexpr int x[]{14,5,2,8,2},y[]{11,12,15,11,6};
                constexpr XeenDirection facing[]{XeenDirection::South,XeenDirection::West,
                    XeenDirection::East,XeenDirection::North,XeenDirection::North};
                to.x=x[distance-1];to.y=y[distance-1];to.direction=facing[distance-1];
            }
        }
        const auto admitted=world.prepareEventTransition(to,spell==XeenLearnedSpell::TownPortal && camera.mapId.side==XeenSide::Clouds);
        result.unsupported=!admitted;to=admitted.destination;
        if(!result.unsupported)result.destination=to;
        return result;
    }
    if(spell==XeenLearnedSpell::SuperShelter) {result.shelter=true;return result;}
    if(spell==XeenLearnedSpell::Teleport && !distance)return result;
    auto to=camera;
    if(spell==XeenLearnedSpell::TimeDistortion) {to.x=map.geometry.runX;to.y=map.geometry.runY;}
    else {
        constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
        const auto d=unsigned(XeenDirection(camera.direction));
        if(d>=4 || distance>9)throw std::invalid_argument("Invalid travel spell input");
        const unsigned count=spell==XeenLearnedSpell::Etherealize?1:distance;
        to.x+=int(count)*dx[d];to.y+=int(count)*dy[d];
    }
    const auto cell=world.sampleCell(camera.mapId,to.x,to.y);
    // DOS 0x30996 and 0x303e3: destination lookup only, no traversed walls.
    const unsigned word=cell?unsigned(cell->cell->rawWord):0x8888;
    const unsigned surface=cell?unsigned(cell->cell->surfaceIndex):15;
    if((spell==XeenLearnedSpell::Etherealize && word==0x8888) ||
        (spell==XeenLearnedSpell::Teleport && (map.geometry.isOutdoors()?
            surface==0 || surface==8:word==0x8888))) {result.failed=true;return result;}
    result.unsupported=!cell;
    if(map.geometry.isOutdoors()) {
        if(cell) {to.mapId=cell->mapId;to.x=cell->x;to.y=cell->y;}
    } else if(!xeenIndoorCoordinate(to.x,to.y))result.unsupported=true;
    if(!result.unsupported)result.unsupported=!world.prepareEventTransition(to);
    result.reload=to.mapId!=camera.mapId;
    if(!result.unsupported)result.destination=to;
    return result;
}
XeenItemSpellPreparation::XeenItemSpellPreparation(XeenLearnedSpell value,unsigned casterLevel,
        XeenInventoryCategory kind,XeenItem selected):item(selected),spell(value),level(casterLevel),category(kind) {}
bool XeenItemSpellPreparation::service(XeenConsequenceDraw &draw) {
    if(complete)return true;
    if(spell==XeenLearnedSpell::EnchantItem) {
        if(!generated) {
            const auto roll=draw.draw(1,level/5+1);if(!roll)return false;
            // DOS 0x31c91..0x31da3: caster-level draw precedes every refusal.
            failed=!item.id || item.material || item.state ||
                (category!=XeenInventoryCategory::Weapons && category!=XeenInventoryCategory::Armor) ||
                (category==XeenInventoryCategory::Weapons && item.id==34);
            if(failed){draw.effect(21);return complete=true;}
            generated.emplace(std::min(*roll,5u),category==XeenInventoryCategory::Armor?
                XeenItemCandidate::Enchant::Armor:XeenItemCandidate::Enchant::Weapon);
        }
        if(!generated->service(draw))return false;
        item.material=generated->item().material;item.state=generated->item().state;
        draw.effect(19);
    } else if(spell==XeenLearnedSpell::RechargeItem) {
        failed=category!=XeenInventoryCategory::Miscellaneous || !item.material || item.material>9 || item.id==53;
        if(!failed) {
            const auto roll=draw.draw(1,6);if(!roll)return false;
            draw.effect(20);
            // DOS 0x31c0d: drop curse (0x40), retain broken (0x80) for the cap.
            item.state=static_cast<std::uint8_t>((item.state&0xc0)|std::min(63u,unsigned(item.state&0xbf)+*roll));
        } else draw.effect(21);
    } else if(spell==XeenLearnedSpell::ItemToGold) {
        failed=category==XeenInventoryCategory::Weapons && item.id==34;
        if(failed)draw.effect(21);
        if(!failed) {
            if(category==XeenInventoryCategory::Miscellaneous?item.material!=0:item.id!=0)draw.effect(20);
            const auto price=xeenItemPrice(category,item,XeenItemService::Buy);
            failed=!price;if(price) {gold=*price;item={};}
        }
    } else throw std::invalid_argument("Not an item spell");
    return complete=true;
}

std::optional<XeenSpellInformation> XeenLearnedSpellRules::information(XeenLearnedSpell spell,
        const XeenCamera &camera,const std::vector<XeenActor> &actors,
        const std::array<std::optional<XeenMonsterIdentity>,3> &slots) {
    if(spell!=XeenLearnedSpell::DetectMonster && spell!=XeenLearnedSpell::IdentifyMonster)return {};
    XeenSpellInformation result;result.facing=camera.direction;result.side=camera.mapId.side;
    result.detection=spell==XeenLearnedSpell::DetectMonster;
    if(result.detection) {
        // DOS checks positions alone, without walls, species or activation tests.
        for(const auto &actor:actors) {
            const int x=actor.x-camera.x+3,y=camera.y+3-actor.y;
            if(x>=0 && x<7 && y>=0 && y<7) {
                auto &count=result.grid[y*7+x];count=std::min(count+1,3u);
            }
        }
    } else {
        result.failed=true;
        for(unsigned n=0;n<3;++n)if(slots[n]) {
            const auto it=std::find_if(actors.begin(),actors.end(),[&](const auto &a){return a.id==*slots[n];});
            if(it==actors.end() || !it->statistics)throw std::invalid_argument("Missing Identify Monster attack slot");
            const auto &m=*it->statistics;
            result.monsters[n]=XeenSpellInformation::Monster{m.name(),it->hp,m.armorClass(),m.attacks(),m.specialAttack()};
            result.failed=false;
        }
    }
    return result;
}

void XeenSpellEffect::apply(XeenCharacter &character) const {
    character.currentHp=hp;character.conditions=conditions;
    if(permanentEndurance)character.endurance.permanent=*permanentEndurance;
    if(temporaryAge)character.temporaryAge=*temporaryAge;
}
namespace {
// Adapted from ScummVM 6814ee9ba54582f5b5adcffab49efbbd8f589edd,
// devtools/create_mm/create_xeen/constants.cpp, SPELLS_ALLOWED. ScummVM
// contributors are credited in upstream COPYRIGHT; GPL-3.0-or-later.
constexpr std::array<std::array<std::uint8_t, 39>, 3> kAllowed{{
	{{0,1,2,3,5,6,7,8,9,10,12,14,16,23,26,27,28,30,31,32,33,42,46,48,49,50,52,55,56,58,59,62,64,65,67,68,71,73,74}},
	{{1,4,11,13,15,17,18,19,20,21,22,24,25,29,34,35,36,37,38,39,40,41,42,43,44,45,47,51,53,54,57,60,61,63,66,69,70,72,75}},
	{{0,1,2,3,4,5,7,9,10,20,25,26,27,28,30,31,34,38,40,41,42,43,44,45,49,50,52,53,55,59,60,61,62,67,68,72,73,74,75}}
}};

std::size_t ownerFor(const XeenPartyState &party, std::size_t activeIndex) {
	if (activeIndex >= party.party.size()) throw std::invalid_argument("Invalid spell target index");
	const auto owner = party.party.activeRosterIds()[activeIndex];
	if (owner >= XeenRoster::kCharacterCount) throw std::invalid_argument("Invalid spell target owner");
	return owner;
}
}

XeenLearnedSpellNames XeenLearnedSpellNames::parse(std::vector<std::uint8_t> bytes) {
	if (bytes.empty() || bytes.size()>XeenLearnedSpellNames{}.names.size()*64u)
		throw std::invalid_argument("Incompatible DARK.CC/spells.xen resource");
	XeenLearnedSpellNames result;
	std::size_t offset=0;
	for (auto &name:result.names) {
		const auto begin=offset;
		while (offset<bytes.size() && bytes[offset]) ++offset;
		if (offset==bytes.size() || offset==begin || offset-begin>63)
			throw std::invalid_argument("Malformed DARK.CC/spells.xen name");
		name.assign(reinterpret_cast<const char *>(bytes.data()+begin),offset-begin);
		++offset;
	}
	if (offset!=bytes.size()) throw std::invalid_argument("Trailing DARK.CC/spells.xen data");
	result.raw=std::move(bytes);
	return result;
}

std::optional<XeenSpellCategory> XeenLearnedSpellRules::categoryForClass(XeenCharacterClass value) noexcept {
	switch (value) {
	case XeenCharacterClass::Paladin: case XeenCharacterClass::Cleric: return XeenSpellCategory::Clerical;
	case XeenCharacterClass::Archer: case XeenCharacterClass::Sorcerer: return XeenSpellCategory::Wizardry;
	case XeenCharacterClass::Druid: case XeenCharacterClass::Ranger: return XeenSpellCategory::Druidic;
	default: return std::nullopt;
	}
}

std::optional<std::uint8_t> XeenLearnedSpellRules::spellForSlot(XeenSpellCategory category,
		std::size_t slot) noexcept {
	const auto index = static_cast<std::size_t>(category);
	if (index >= kAllowed.size() || slot >= kAllowed[index].size()) return std::nullopt;
	return kAllowed[index][slot];
}

std::optional<XeenLearnedSpell> XeenLearnedSpellRules::supported(std::uint8_t id) noexcept {
    switch (id) {
    case 44: case 71: case 21: case 66: case 69: case 70:
    case 19: case 39: case 57:
    case 4: case 15: case 34: case 41: case 42: case 74:
    case 3: case 12: case 13: case 30: case 31: case 53: case 55:
    case 16: case 56: case 58:
    case 6: case 7: case 8: case 9: case 49: case 52: case 64: case 67: case 68:
    case 24: case 29: case 32: case 46: case 51:
    case 0: case 5: case 11: case 14: case 18: case 22: case 23: case 25: case 28:
    case 35: case 36: case 37: case 43: case 47: case 48: case 54: case 63: case 65:
    case 1: case 10: case 20: case 26: case 27: case 40: case 45:
    case 2: case 17: case 33: case 38: case 61: case 62: case 72: case 73:
    case 50: case 59: case 60: case 75: return static_cast<XeenLearnedSpell>(id);
    default: return std::nullopt;
    }
}

std::optional<XeenLearnedSpell> XeenLearnedSpellRules::supportedIn(std::uint8_t id, bool combat) noexcept {
    if (combat && (id==44 || id==71 || id==19 || id==21 || id==40 || id==66 || id==69 || id==75)) return std::nullopt;
    return supported(id);
}
unsigned XeenLearnedSpellRules::cost(XeenLearnedSpell spell) noexcept {
    return cost(static_cast<std::uint8_t>(spell),1).sp;
}

// Pinned Spells::calcSpellPoints/calcSpellCost and constants.cpp cost tables.
// ScummVM developers (upstream COPYRIGHT), GPL-3.0-or-later, 6814ee9b.
namespace {
constexpr int kSp[]{8,1,5,-2,5,-2,20,10,12,8,3,-3,75,40,12,6,200,10,100,30,-1,30,15,25,10,-2,1,2,7,20,-2,-2,100,15,5,100,35,75,5,20,4,5,1,-2,6,2,75,40,60,6,4,25,-2,-2,60,-1,50,15,125,2,-1,3,-1,200,35,150,15,5,4,10,8,30,4,5,7,5,0};
constexpr unsigned kGems[]{0,0,2,1,2,4,5,0,0,0,0,10,10,10,0,0,20,4,10,20,1,10,5,5,4,2,0,0,0,10,3,1,20,4,0,20,10,10,1,10,0,0,0,2,2,0,10,10,10,0,0,10,3,2,10,1,10,10,20,0,0,1,1,20,5,20,5,0,0,0,0,5,1,2,0,2,0};
}
XeenSpellCost XeenLearnedSpellRules::cost(std::uint8_t id,unsigned level) noexcept {
    if(id>=77)return {};
    return {kSp[id]>=0?unsigned(kSp[id]):level*unsigned(-kSp[id]),kGems[id]};
}
std::optional<std::size_t> XeenLearnedSpellRules::slotForSpell(XeenSpellCategory category,std::uint8_t id) noexcept {
    for(std::size_t slot=0;slot<39;++slot)if(spellForSlot(category,slot)==id)return slot;
    return {};
}
std::uint32_t XeenLearnedSpellRules::purchasePrice(std::uint8_t id,XeenCharacterClass c) noexcept {
    if(id>=76)return 0;
    const unsigned base=kSp[id]<0?500*unsigned(-kSp[id]):100*unsigned(kSp[id]);
    return base*((c==XeenCharacterClass::Paladin || c==XeenCharacterClass::Archer || c==XeenCharacterClass::Ranger)?2:1);
}
std::vector<std::size_t> XeenLearnedSpellRules::rows(const XeenCharacter &c,bool guild,bool info,XeenMapIdentity map) {
    std::vector<std::size_t> result;const auto category=categoryForClass(c.characterClass);
    if(!category || !c.learnedSpells)return result;
    if(guild) {
        // DOS-verified CLOUDS_GUILD_SPELLS rows, including their stored order.
        constexpr int catalogue[5][20]{
            {1,10,20,26,27,38,40,42,45,50,55,59,60,61,62,68,72,75,77,77},
            {3,4,5,14,15,25,30,31,34,41,49,51,53,67,73,75,-1,-1,-1,-1},
            {4,8,9,12,13,22,23,24,28,34,41,44,52,70,73,74,-1,-1,-1,-1},
            {6,7,9,11,12,13,17,21,22,24,29,36,56,58,64,71,-1,-1,-1,-1},
            {6,7,9,11,12,13,18,21,29,32,36,37,46,51,56,58,69,-1,-1,-1}};
        const auto append=[&](unsigned id) {
            if(const auto slot=slotForSpell(*category,id);slot && (info || !known(c,*slot)))result.push_back(*slot);
        };
        if(map.side==XeenSide::Clouds) {
            if(map.number==49)for(unsigned id=0;id<76;++id)append(id);
            else if(map.number>=28 && map.number<=32)
                for(int id:catalogue[map.number-28])if(id>=0 && id<76)append(id);
        } else if(map.number==37)for(unsigned id=0;id<76;++id)append(id);
        else if(map.number>=29 && map.number<=35 && map.number%2) {
            // Part 0 compared all entries with DOS tables 0x4f839 / 0x546c2.
            constexpr unsigned order[3][39]{
                {42,1,26,59,27,10,50,68,55,62,67,73,2,5,3,31,30,52,49,28,74,0,9,7,14,8,33,6,23,71,64,56,48,46,12,32,58,65,16},
                {42,1,45,61,72,40,20,60,38,41,75,34,4,43,25,53,44,15,70,17,24,69,22,66,57,11,29,39,51,21,19,36,47,13,54,37,18,35,63},
                {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38}};
            constexpr unsigned ranges[3][4][2]{
                {{0,20},{16,35},{27,37},{29,39}},
                {{0,17},{14,34},{26,37},{29,39}},
                {{0,20},{16,35},{27,37},{29,39}}};
            const unsigned cat=static_cast<unsigned>(*category),group=(map.number-29)/2;
            for(unsigned i=ranges[cat][group][0];i<ranges[cat][group][1];++i)append(order[cat][i]);
        }
    } else for(std::size_t slot=0;slot<39;++slot)if(known(c,slot))result.push_back(slot);
    return result;
}
XeenSpellRange XeenLearnedSpellRules::range(std::uint8_t id) noexcept {
    switch(id) {
    case 51: case 0: case 5: case 18: case 48: case 54: case 63: case 65: return XeenSpellRange::All;
    case 24: case 32: case 46:
    case 11: case 14: case 22: case 25: case 37: case 43: case 47:
    case 2: case 33: case 38: case 50: case 60: case 61: case 62: case 72: case 73: return XeenSpellRange::Group;
    default: return XeenSpellRange::Single;
    }
}
XeenSpellTarget XeenLearnedSpellRules::target(std::uint8_t id) noexcept {
    switch (id) {
    case 19: case 39: case 57:return XeenSpellTarget::Item;
    case 4: case 15: case 34: case 41: case 42: case 74: return XeenSpellTarget::Party;
    case 24: case 29: case 32: case 46: case 51:
    case 0: case 5: case 11: case 14: case 18: case 22: case 23: case 25: case 28:
    case 35: case 36: case 37: case 43: case 47: case 48: case 54: case 63: case 65: return XeenSpellTarget::Contact;
    case 3: case 12: case 13: case 30: case 31: case 53:
    case 1: case 6: case 16: case 75: return XeenSpellTarget::Party;
    case 55: return XeenSpellTarget::Element;
    case 56: case 58:
    case 7: case 8: case 9: case 49: case 52: case 64: case 67: case 68:
    case 10: case 26: case 59: return XeenSpellTarget::Member;
    case 2: case 17: case 33: case 38: case 61: case 62: case 72: case 73:
    case 20: case 27: case 45: case 50: case 60: return XeenSpellTarget::Contact;
    case 44: case 71: case 21: case 69: case 70: case 40: return XeenSpellTarget::Relocation;
    case 66: return XeenSpellTarget::Party;
    default: return XeenSpellTarget::Unsupported;
    }
}
XeenQuickSpellCheck XeenLearnedSpellRules::check(const XeenCharacter &c,std::size_t slot,std::uint32_t gems,bool combat) noexcept {
    using Q=XeenQuickSpellCheck;if(slot==255)return Q::NoSpell;
    const auto category=categoryForClass(c.characterClass);
    const auto id=category?spellForSlot(*category,slot):std::nullopt;
    // Explicit incomplete-engine policy: no paid failure/turn for any missing effect.
    if(!id || !supported(*id) || !c.hasSpells || !known(c,slot))return Q::Unsupported;
    const auto required=cost(*id,c.currentLevel());
    if(c.currentSp<int(required.sp))return Q::InsufficientSp;
    if(gems<required.gems)return Q::InsufficientGems;
    if(combat)for(auto forbidden:{19,21,40,44,66,69,71,75})if(*id==forbidden)return Q::CombatForbidden;
    return Q::Supported;
}
XeenQuickSpellCheck XeenLearnedSpellRules::quickSpellCheck(const XeenCharacter &c,std::uint32_t gems) noexcept {
    return check(c,c.currentSpell,gems,true);
}

bool XeenLearnedSpellRules::known(const XeenCharacter &character, std::size_t slot) noexcept {
	return slot < 39 && character.learnedSpells && (*character.learnedSpells)[slot] != 0;
}

bool XeenLearnedSpellRules::eligible(const XeenPartyState &party, std::size_t activeIndex,
		std::size_t slot, bool combat) noexcept {
	if (activeIndex >= party.party.size()) return false;
	const auto owner = party.party.activeRosterIds()[activeIndex];
	if (owner >= XeenRoster::kCharacterCount) return false;
	const auto &character = party.roster.at(owner);
	const auto category = categoryForClass(character.characterClass);
	const auto spell = category ? spellForSlot(*category, slot) : std::nullopt;
	const auto effect=spell ? supportedIn(*spell,combat) : std::nullopt;
	return character.hasSpells && character.canAct() && effect &&
		character.currentSp >= int(cost(*spell,character.currentLevel()).sp) && known(character, slot);
}

namespace {
XeenSpellPreparation cure(const XeenPartyState &,std::size_t,std::uint32_t,unsigned,bool suppress=false);
XeenSpellPreparation revive(const XeenPartyState &,std::size_t,std::uint32_t,bool);
XeenSpellPreparation divineIntervention(const XeenPartyState &,std::uint8_t,std::uint32_t);
}

XeenSpellPreparation XeenLearnedSpellRules::prepareEffect(XeenLearnedSpell spell,const XeenPartyState &party,
        std::optional<std::size_t> target,std::uint32_t year,std::optional<std::uint8_t> casterOwner,bool viewBlack) {
    if(spell==XeenLearnedSpell::Light)return {false,{}, {},!viewBlack};
    if(spell==XeenLearnedSpell::Awaken)return prepareAwaken(party);
    if(spell==XeenLearnedSpell::CreateFood)return {false,{},static_cast<std::uint16_t>(party.food+party.party.size())};
    if(spell==XeenLearnedSpell::DivineIntervention) {
        if(!casterOwner)throw std::invalid_argument("Divine Intervention requires its caster owner");
        return divineIntervention(party,*casterOwner,year);
    }
    if(!target)return {};
    switch(spell) {
    case XeenLearnedSpell::RaiseDead:return revive(party,*target,year,false);
    case XeenLearnedSpell::Resurrection:return revive(party,*target,year,true);
    case XeenLearnedSpell::FirstAid:return prepareFirstAid(party,*target,year);
    case XeenLearnedSpell::CureWounds:return prepareFirstAid(party,*target,year,15);
    case XeenLearnedSpell::Revitalize:return prepareRevitalize(party,*target,year);
    case XeenLearnedSpell::NaturesCure:return prepareFirstAid(party,*target,year,25);
    case XeenLearnedSpell::CurePoison:return cure(party,*target,year,3);
    case XeenLearnedSpell::CureDisease:return cure(party,*target,year,4);
    case XeenLearnedSpell::CureParalysis:return cure(party,*target,year,11);
    case XeenLearnedSpell::StoneToFlesh:return cure(party,*target,year,14);
    case XeenLearnedSpell::SuppressPoison:return cure(party,*target,year,3,true);
    case XeenLearnedSpell::SuppressDisease:return cure(party,*target,year,4,true);
    default:throw std::invalid_argument("Unsupported member spell effect");
    }
}
void XeenLearnedSpellRules::applyPartyEffect(XeenLearnedSpell spell,XeenGameplayContext &context,
        unsigned level,std::optional<unsigned> element) {
    // DOS byte writes, including Day spells' flag/count assignments (M76 Part 0).
    const auto byte=static_cast<std::uint8_t>(level);
    switch(spell) {
    case XeenLearnedSpell::Clairvoyance: context.effects[XeenPartyEffect::Clairvoyance]=1;break;
    case XeenLearnedSpell::Levitate: context.effects[XeenPartyEffect::Levitate]=1;break;
    case XeenLearnedSpell::WalkOnWater: context.effects[XeenPartyEffect::WalkOnWater]=1;break;
    case XeenLearnedSpell::Light:
        context.lightAndResistances[0]=static_cast<std::uint16_t>(context.lightAndResistances[0]+1);
        break;
    case XeenLearnedSpell::Bless: context.effects[XeenPartyEffect::Bless]=byte;break;
    case XeenLearnedSpell::Heroism: context.effects[XeenPartyEffect::Heroism]=byte;break;
    case XeenLearnedSpell::HolyBonus: context.effects[XeenPartyEffect::HolyBonus]=byte;break;
    case XeenLearnedSpell::PowerShield: context.effects[XeenPartyEffect::PowerShield]=byte;break;
    case XeenLearnedSpell::ProtectionFromElements:
        if(element) {
            if(*element>=4)throw std::invalid_argument("Invalid protection element");
            context.lightAndResistances[*element+2]=std::min(2u*level+5u,200u);
        }
        break;
    case XeenLearnedSpell::DayOfProtection:
        for(auto i:{XeenPartyEffect::WalkOnWater,XeenPartyEffect::Heroism,XeenPartyEffect::HolyBonus,XeenPartyEffect::Bless})context.effects[i]=byte;
        for(unsigned i:{0u,2u,3u,4u,5u})context.lightAndResistances[i]=byte;
        break;
    case XeenLearnedSpell::DayOfSorcery:
        for(auto i:{XeenPartyEffect::Clairvoyance,XeenPartyEffect::WizardEye,XeenPartyEffect::Levitate,XeenPartyEffect::PowerShield})context.effects[i]=byte;
        context.effects[XeenPartyEffect::AutomapOn]=0;context.lightAndResistances[0]=byte;
        break;
    default: break;
    }
    if(spell==XeenLearnedSpell::WizardEye) {context.effects[XeenPartyEffect::WizardEye]=1;context.effects[XeenPartyEffect::AutomapOn]=0;}
}

namespace {
XeenSpellPreparation heal(const XeenCharacter &target,std::size_t owner,std::uint32_t year,unsigned healing) {
	XeenSpellPreparation result;
	if (target.conditions[13] || target.conditions[14] || target.conditions[15]) {
		result.failed = true;
		return result;
	}
	XeenCharacterRules::validateForUse(target, {year});
	const int maximum = XeenCharacterRules::maxHp(target, {year});
	const auto hp = target.currentHp;
	const auto after = hp <= maximum ? std::min(static_cast<std::int64_t>(hp) + healing,
		static_cast<std::int64_t>(maximum)) : static_cast<std::int64_t>(hp);
	if (after < std::numeric_limits<std::int16_t>::min() ||
			after > std::numeric_limits<std::int16_t>::max())
		throw std::invalid_argument("First Aid HP exceeds storage domain");
	XeenSpellEffect effect;
	effect.owner = static_cast<std::uint8_t>(owner);
	effect.hp = static_cast<std::int16_t>(after);
	effect.conditions = target.conditions;
	effect.healing = hp <= maximum;
	if (after > 0) effect.conditions[12] = 0;
	result.effects.push_back(effect);
	return result;
}

XeenSpellPreparation revive(const XeenPartyState &party,std::size_t index,std::uint32_t year,bool resurrection) {
    const auto owner=ownerFor(party,index);
    auto target=party.roster.at(owner);
    if(!target.conditions[resurrection?15:13])return {true,{},{}};
    XeenSpellEffect effect{static_cast<std::uint8_t>(owner),target.currentHp,target.conditions};
    if(resurrection) {
        // DOS calls healing(0) while Eradicated still refuses it, then clears only that flag.
        effect.conditions[15]=0;
        effect.temporaryAge=std::min(int(target.temporaryAge)+5,250);
    } else {
        target.conditions[13]=target.conditions[12]=0;target.currentHp=0;
        const auto healed=heal(target,owner,year,1);
        effect.hp=0;effect.conditions=target.conditions;
        if(!healed.failed)effect=healed.effects.front();
    }
    // DOS byte subtraction precedes the minimum; input zero wraps to 255.
    effect.permanentEndurance=std::max(1,int(static_cast<std::uint8_t>(int(target.endurance.permanent)-1)));
    return {false,{effect},{}};
}

XeenSpellPreparation divineIntervention(const XeenPartyState &party,std::uint8_t caster,std::uint32_t year) {
    const auto &ids=party.party.activeRosterIds();
    if(std::find(ids.begin(),ids.end(),caster)==ids.end())throw std::invalid_argument("Invalid Divine Intervention caster");
    XeenSpellPreparation result;
    for(std::size_t index=0;index<party.party.size();++index) {
        const auto owner=ownerFor(party,index);
        auto target=party.roster.at(owner);
        XeenSpellEffect effect{static_cast<std::uint8_t>(owner),target.currentHp,target.conditions};
        if(owner==caster) {
            target.temporaryAge=std::min(int(target.temporaryAge)+5,250);
            effect.temporaryAge=target.temporaryAge;
        }
        std::fill(effect.conditions.begin(),effect.conditions.begin()+15,0);
        target.conditions=effect.conditions;
        if(!target.conditions[15]) {
            XeenCharacterRules::validateForUse(target,{year});
            effect.hp=XeenCharacterRules::maxHp(target,{year});
            // Direct HP assignment; unlike addHitPoints this has no portrait sparkle.
        }
        result.effects.push_back(effect);
    }
    return result;
}

XeenSpellPreparation cure(const XeenPartyState &party,std::size_t index,std::uint32_t year,
        unsigned condition,bool suppress) {
    const auto owner=ownerFor(party,index);
    auto target=party.roster.at(owner);
    if(suppress) {
        auto &count=target.conditions[condition];
        if(count)count=count<4?1:count-3;
    }
    // DOS: suppression changes its count first; cures clear after healing(0).
    // Healing refuses terminal flags, but the explicit condition operation does not.
    auto result=heal(target,owner,year,0);
    if(result.failed) {
        result.failed=false;
        result.effects.push_back({static_cast<std::uint8_t>(owner),target.currentHp,target.conditions});
    }
    if(!suppress)result.effects.front().conditions[condition]=0;
    return result;
}
}
XeenSpellPreparation XeenLearnedSpellRules::prepareFirstAid(const XeenPartyState &party,
        std::size_t index,std::uint32_t year,unsigned healing) {
    const auto owner=ownerFor(party,index);
    return heal(party.roster.at(owner),owner,year,healing);
}
XeenSpellPreparation XeenLearnedSpellRules::prepareRevitalize(const XeenPartyState &party,
        std::size_t index,std::uint32_t year) {
    return cure(party,index,year,2);
}
std::optional<XeenSpellPreparation> XeenLearnedSpellRules::preparePowerCure(const XeenPartyState &party,
        std::size_t index,std::uint32_t year,unsigned level,XeenConsequenceDraw &draw,bool *fmRequested) {
    const auto guard=prepareFirstAid(party,index,year,0);
    if(guard.failed)return guard;
    if(fmRequested && !*fmRequested){*fmRequested=true;draw.effect(30);}
    while(draw.remaining)if(const auto value=draw.draw(2,12))
        return prepareFirstAid(party,index,year,*value*level);
    return {};
}

XeenSpellPreparation XeenLearnedSpellRules::prepareAwaken(const XeenPartyState &party) {
	XeenSpellPreparation result;
	std::array<bool, XeenRoster::kCharacterCount> seen{};
	result.effects.reserve(party.party.size());
	for (std::size_t index = 0; index < party.party.size(); ++index) {
		const auto owner = ownerFor(party, index);
		if (seen[owner]) continue;
		seen[owner] = true;
		const auto &target = party.roster.at(owner);
		XeenSpellEffect effect;
		effect.owner = static_cast<std::uint8_t>(owner);
		effect.hp = target.currentHp;
		effect.conditions = target.conditions;
		effect.conditions[8] = 0;
		if (effect.hp > 0) effect.conditions[12] = 0;
		result.effects.push_back(effect);
	}
	return result;
}

} // namespace mmodern
