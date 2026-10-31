#include "XeenCoverage.h"
#include "games/xeen/XeenEntranceScene.h"
#include "XeenCoverageContext.h"
#include "XeenCoverageReference.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenMapFormat.h"
#include "formats/xeen/XeenMonsterFormat.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenRestRules.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenMonsterTreasure.h"
#include "games/xeen/XeenObjectVisual.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace mmodern;
using namespace mmodern::coverage;
namespace {
std::string resource(unsigned id,const char *extension) {
    char name[32];std::snprintf(name,sizeof(name),id>=100?"mazex%03u.%s":"maze%04u.%s",id,extension);return name;
}
Support rejected(const std::exception &e) {return {Status::Unclassified,e.what()};}
std::string spellKey(unsigned id,const XeenLearnedSpellNames &names) {
    return std::to_string(id)+" "+(id<names.names.size()?names.names[id]:"invalid spell ID");
}
void spell(Census &c,unsigned id,const XeenLearnedSpellNames &names,unsigned map,std::size_t count,const std::string &source) {
    if(id>=76) {c.add("Unclassified","Spell reference "+std::to_string(id),{Status::Unclassified,"No party spell at this ID"},map,count,source);return;}
    c.add("Spells",spellKey(id,names),spellSupport(id),map,count,source);
}
void item(Census &c,const XeenItemCatalog &catalog,unsigned category,XeenItem value,
        unsigned map,std::size_t count,const std::string &source,const XeenLearnedSpellNames &names,OriginalContext &context) {
    if(!value.id)return;
    XeenItem named=value;if(category==3) {named.material=1;named.state=1;}
    const auto description=catalog.describe(static_cast<XeenInventoryCategory>(category),named);
    const std::string key=category==3?"Misc power "+std::to_string(value.id)+" "+description.displayName:
        "Category "+std::to_string(category)+" material "+std::to_string(value.material);
    // Equipment rows measure enchantment contribution, using a canonical legal
    // base item. Acquisition IDs are reported separately by their Event rows.
    if(category!=3) {value.id=1;value.frame=category==0?1:category==1?3:8;value.state=0;}
    c.add("Items",key,itemSupport(category,value,context.world,context.party),map,count,source);
    if(category<3)c.add("Equipment combat/Rest",key,equipmentCombatSupport(category,value,context.world,context.party),map,count,source);
    // A name is a display token, not an exported item record.
    if(category==3 && value.id<std::size(itemSpellIds) && itemSpellIds[value.id]<76)
        spell(c,itemSpellIds[value.id],names,map,count,"item power "+std::to_string(value.id));
    if(category==3 && value.id==33)spell(c,28,names,map,count,"item power 33: separate frostbite2 variant");
}
// The runtime reads a monster's six elemental resistances in exactly one place:
// XeenPhysicalPlayerCandidate (M67) reduces an equipped elemental weapon's
// contribution by the resistance of the weapon's category (attack2's
// getMonsterResistance). Probe that path with a level-1 Knight whose equipped
// weapon has the first material of the category and a tape that must hit:
// twelve natural 20s add 240 to the Accuracy-250 Adventurer base of 26, so the
// total 285 beats AC + 10 for every byte AC. Physical resistance is read by the
// same candidate for every melee hit.
Support elementalResistanceProbe(const XeenMonsterRecord &m,unsigned index,unsigned category) {
    constexpr unsigned materials[]{1,9,16,21,26,34};
    try {
        XeenCharacter c;c.rosterId=0;c.characterClass=XeenCharacterClass::Knight;c.permanentLevel=1;c.birthYear=592;c.currentHp=10;
        c.intellect=c.personality=c.endurance={15,0};
        if(category<6)c.weapons[0]={static_cast<std::uint8_t>(materials[category]),1,0,1};else c.weapons[0]={0,1,0,1};
        XeenCombatInputs i;i.might=i.speed={15,0};i.accuracy={250,0};i.luck=XeenAttributeValue{15,0};
        i.resistances=XeenCombatResistances{};i.poisonResistance=XeenAttributeValue{0,0};
        XeenPhysicalPlayerCandidate candidate(c,i,m,index,610,false,XeenDifficulty::Adventurer);
        std::vector<XeenCombatRandom::Draw> tape{{1,3,3},{1,3,3},{1,3,3}};
        for(unsigned n=0;n<12;++n)tape.push_back({1,20,20});
        tape.push_back({1,20,19});
        XeenCombatRandom rng(tape);
        bool done=false;
        for(unsigned n=0;n<40 && !done;++n) {XeenConsequenceDraw draw{rng,1,{}};done=candidate.service(draw);}
        if(!done || !candidate.hit)return {Status::Unclassified,"census attack probe did not hit this record"};
        return {Status::Supported,category<6?"XeenPhysicalPlayerCandidate applies the M67 elemental weapon contribution against this resistance":
            "XeenPhysicalPlayerCandidate applies physical resistance to melee damage"};
    }catch(const std::exception &e) {return {Status::Unsupported,e.what()};}
}
void monster(Census &c,const XeenMonsterRecord &m,unsigned index,unsigned map,std::size_t count,const std::string &source) {
    Support attack{Status::Supported,"validateAttackCapabilities accepts"};
    try {m.validateAttackCapabilities();}catch(const std::exception &e) {attack={Status::Unsupported,e.what()};}
    auto admission=attack;
    // Movement is the runtime's capability (every live record); the terrain rule
    // separates walkers, flyers and species 59 (Water Dragon, Vulture Roc).
    const char *movementGap="xeenMonsterMovementSupported rejects: no HP, not a live actor";
    if(admission.status==Status::Supported && !xeenMonsterMovementSupported(m,int(index)))admission={Status::Unsupported,movementGap};
    if(admission.status==Status::Supported && !m.supportsRendering())
        admission={Status::Unsupported,"supportsRendering rejects"};
    if(admission.status==Status::Supported)admission.reason="Attack, movement and mechanic-rendering checks accept; see resistance/drop gaps separately";
    c.add("Monster species",source,admission,map,count);
    c.add("Monsters","Attack type="+std::to_string(m.damageType())+" special="+std::to_string(m.raw[30])+" ranged="+std::to_string(m.raw[32]),attack,map,count,source);
    c.add("Monsters","Movement flying="+std::to_string(m.raw[46])+(index==59?" species=59":""),
        xeenMonsterMovementSupported(m,int(index))?Support{Status::Supported,"xeenMonsterMovementSupported accepts"}:Support{Status::Unsupported,movementGap},map,count,source);
    c.add("Monsters","Animation loop="+std::to_string(m.raw[48])+" effect="+std::to_string(m.animationEffect()),
        m.supportsRendering()?Support{Status::Supported,"supportsRendering accepts (current scene presentation)"}:
        Support{Status::Unsupported,"supportsRendering rejects"},map,count,source);
    Support target{Status::Supported,"XeenMonsterTargetCandidate accepts hatred"};
    try {(void)XeenMonsterTargetCandidate({},m.hatred(),0x3f);}catch(const std::exception &e) {target={Status::Unsupported,e.what()};}
    c.add("Monsters","Target hatred="+std::to_string(m.hatred()),target,map,count,source);
    for(unsigned resistance=34;resistance<=40;++resistance)if(m.raw[resistance]) {
        // Monster resistances are never read by the monster's own attack (the M68
        // damage sequence uses the character's resistances); the engine reads them
        // in the player's M67 weapon attack and, for magic and physical, in the
        // ranged spell damage. Probe both paths.
        Support support=elementalResistanceProbe(m,index,resistance-34);
        if(support.status==Status::Supported && (resistance==39 || resistance==40)) {
            try {
                // Current ranged-spell damage accepts both original resistance
                // values; physical spells also apply physical damage reduction.
                (void)XeenSpellDamageCandidate(1,0,m.magicResistance(),index,
                    resistance==39?XeenLearnedSpell::MagicArrow:XeenLearnedSpell::Shrapmetal,m.physicalResistance());
                support.reason+="; XeenSpellDamageCandidate admits it for spell damage";
            }catch(const std::exception &e) {support={Status::Unsupported,e.what()};}
        }
        c.add("Monsters","Resistance field="+std::to_string(resistance-34),support,map,count,source);
    }
    if(m.raw[45]) {
        // The generic lethal drop (M81) reads the record's drop field; item
        // identity still depends on runtime RNG.
        Support drop{Status::Supported,m.raw[45]==7 ?
            "XeenMonsterDropCandidate makes one chance draw; field 7 has zero item chance" :
            "XeenMonsterDropCandidate stages this level through XeenItemCandidate; item identity depends on runtime RNG"};
        try {(void)XeenMonsterDropCandidate(XeenTreasureStaging{},m,XeenMapIdentity(static_cast<std::uint16_t>(map)));}catch(const std::exception &e) {drop={Status::Unsupported,e.what()};}
        c.add("Items","Monster random treasure level="+std::to_string(m.raw[45]),drop,map,count,source);
    }
}
void events(Census &c,const std::vector<XeenEventRecord> &records,XeenMapIdentity id,
        const XeenMapEntities &entities,const XeenItemCatalog &catalog,const XeenLearnedSpellNames &names,OriginalContext &context,bool outdoors) {
    const XeenEventFile original{id,resource(id.number,"evt"),true,records};
    // Use the actual side's admitted session and MON bank. Fixture failures
    // remain visible evidence rather than relabeling the Clouds installation.
    std::unique_ptr<OriginalContext> fixture;
    std::optional<std::string> fixtureFailure;
    if(!(id.side==XeenSide::Darkside && id.number>=113 && id.number<=128)) {
        // Session entry forbids immediate monster contact. Choose a repeatable
        // empty physical cell from the primary DAT, without removing any actor.
        XeenCamera start{id,0,0,XeenDirection::North};
        bool empty=false;
        for(int cell=0;cell<256 && !empty;++cell) {
            start.x=cell%16;start.y=cell/16;empty=true;
            for(const auto &actor:entities.monsters)
                if(actor.isActive() && actor.x==start.x && actor.y==start.y) {empty=false;break;}
        }
        try {fixture=std::make_unique<OriginalContext>(context.installation,start);}
        catch(const std::exception &e) {fixtureFailure=e.what();}
    }
    auto &eventContext=fixture?*fixture:context;
    for(const auto &r:records) {
        ++c.instructions;
        const auto physical=[&](unsigned x,unsigned y){return x<(outdoors?16u:32u) && y<(outdoors?16u:32u);};
        const bool needsCaller=!physical(r.x,r.y) || r.opcode==0x1a;
        const auto caller=needsCaller?physicalEventCaller(r,original,outdoors):std::optional<XeenCamera>{};
        Support unavailable{Status::Unclassified,fixtureFailure?"Developer Event fixture: "+*fixtureFailure:""};
        if(fixtureFailure) {
            // A failed session does not erase an independent decoder finding.
            const auto decoded=XeenEventDecoder::decode(r,{id,{}, {},true});
            if(const auto *bad=std::get_if<XeenEventDecodeError>(&decoded))
                unavailable={bad->kind==XeenEventDecodeErrorKind::MalformedInstruction?Status::Unclassified:Status::Unsupported,bad->message};
        }
        auto support=(needsCaller && !caller)?
            Support{Status::Unclassified,!physical(r.x,r.y)?"Off-map subroutine has no physical caller in this resource":
                "Physical Return has no CallEvent caller in this resource"}:
            fixtureFailure?unavailable:
            eventSupport(r,original,eventContext.world,eventContext.party,[&](auto map){return eventContext.text(map);},caller);
        if(support.service && id==XeenMapIdentity(28)) {
            auto entry=caller.value_or(XeenCamera{id,r.x,r.y,r.direction<4?XeenDirection(r.direction):XeenDirection::North});
            auto live=context.replayRoot(entry);
            if(r.direction==4)for(unsigned facing=1;facing<4 && live.status!=Status::Supported;++facing) {
                entry.direction=XeenDirection(facing);live=context.replayRoot(entry);
            }
            if(live.status!=Status::Supported)support=live;
        }
        // An all-directions instruction can address a direction-specific
        // physical line. North alone is not a complete AlterEvent probe.
        if(!caller && r.direction==4 && physical(r.x,r.y) &&
                support.reason=="AlterEvent physical line was not found") {
            for(unsigned direction=1;direction<4 && support.status!=Status::Supported;++direction) {
                auto facing=r;facing.direction=direction;
                const auto alternate=eventSupport(facing,original,eventContext.world,eventContext.party,
                    [&](auto map){return eventContext.text(map);});
                if(alternate.status==Status::Supported)support=alternate;
            }
        }
        c.add("Events",eventMechanic(r),support,id.number);
        if(r.opcode==0x11 && !r.parameters.empty())c.add("Map features","Town service action="+std::to_string(r.parameters[0]),support,id.number);
        if(r.opcode==0x11 && !r.parameters.empty()) {
            const auto site=xeenEntranceSite(r.parameters[0],{id,r.x,r.y,XeenDirection::North});
            if(site && site->quest>=35)
                c.add("Map features","Entrance action="+std::to_string(site->action)+" keyed quest item="+std::to_string(site->quest),
                    {Status::Partial,"Event quest item is outside the Clouds items"},id.number);
        }
        if(support.service)for(const auto key:xeenLocationInput(*support.service).keys) {
            if(const auto refusal=xeenLocationUnsupportedAction(*support.service,key))
                c.add("Services",refusal->substr(0,refusal->find(':')),{Status::Unsupported,*refusal},id.number);
            else {
                const char *name=nullptr;
                if(*support.service==XeenLocationDialog::Tavern)
                    name=key=='s'?"Sign In":key=='d'?"Drink":key=='t'?"Tip":key=='r'?"Rumors":nullptr;
                if(*support.service==XeenLocationDialog::Temple)
                    name=key=='d'?"Donation":key=='u'?"Uncurse":nullptr;
                if(name)c.add("Services",name,{Status::Supported,"Original dialog action admitted by the existing service owner"},id.number);
            }
        }
        auto shape=r;
        if(shape.opcode==0x1c || shape.opcode==0x1d || shape.opcode==0x26)shape.opcode=0x0c;
        const auto decoded=XeenEventDecoder::decode(shape,{id,{}, {},true});
        if(const auto *ins=std::get_if<XeenDecodedEventInstruction>(&decoded)) {
            const auto &op=ins->operation;
            const XeenEventTakeOrGive *give=std::get_if<XeenEventTakeOrGive>(&op);
            if(auto p=std::get_if<XeenEventGiveMulti>(&op))give=p;
            if(give)for(const auto &p:{give->first,give->second,give->third}) {
                if(p.mode==19)spell(c,p.value,names,id.number,1,"Event spell-book change");
                if(p.mode==66)c.add("Items","Event random item level="+std::to_string(p.value),support,id.number,1,"Event");
                if(p.mode==21)c.add("Items","Event item ID="+std::to_string(p.value),support,id.number,1,"Event TakeOrGive; base item/quest ID, not a Misc power");
            }
        }
        // GiveEnchanted's format relation is useful even when the engine rejects it.
        // This reads named identifiers only; support still comes from the decoder.
        if(r.opcode==0x2c && r.parameters.size()>=2) {
            const unsigned code=r.parameters[0],enchantment=r.parameters[1];
            if(code>=60 && code<=81) {
                XeenItem value{};value.material=code-60;value.id=enchantment;value.state=1;
                item(c,catalog,3,value,id.number,1,"Event GiveEnchanted",names,context);
            } else if(code<60) {
                const unsigned category=code<35?0:code<49?1:2;
                XeenItem value{};value.id=1;value.material=enchantment;
                item(c,catalog,category,value,id.number,1,"Event GiveEnchanted",names,context);
            } else c.add("Items","Event quest item="+std::to_string(code),support,id.number,1,"Event GiveEnchanted");
        }
    }
}
Census census(const GameInstallation &original,XeenSide side,OriginalContext &context) {
    Census c;c.side=side==XeenSide::Clouds?"Clouds":"Darkside";
    // The bounded initial-container decoder reads the selected physical archive.
    const auto &installation=original;
    XeenAssetSource assets(installation);
    const auto nameBytes=assets.readLearnedSpellNamesFromDarkArchive();
    if(!nameBytes)throw std::runtime_error("Missing spells.xen catalogue");
    const auto names=XeenLearnedSpellNames::parse(*nameBytes);
    const auto loadedCatalog=loadXeenItemCatalog(assets);
    const auto &catalog=loadedCatalog.catalog;
    if(catalog.materialAvailability()!=XeenMaterialAvailability::Ready)
        c.add("Unclassified","Material names",{Status::Unclassified,loadedCatalog.diagnostic});
    const auto objectVisuals=XeenObjectVisualResolver::load(assets);
    const auto cloudsMonsters=XeenMonsterFormat::parse(assets.readArchiveResource("xeen.mon",XeenSceneArchive::DarksideOnly));
    const auto darkMonsters=XeenMonsterFormat::parse(assets.readArchiveResource("dark.mon",XeenSceneArchive::DarksideOnly));
    const auto &ownMonsters=side==XeenSide::Clouds?cloudsMonsters:darkMonsters;
    c.monsters=ownMonsters.size();
    for(unsigned i=0;i<ownMonsters.size();++i)monster(c,ownMonsters[i],i,0,0,
        std::string(side==XeenSide::Clouds?"xeen.mon ":"dark.mon ")+std::to_string(i)+" "+ownMonsters[i].name());
    for(unsigned i=0;i<76;++i)spell(c,i,names,0,0,"party spell catalogue");
    const char *books[]{"Clerical book (Cleric/Paladin)","Wizardry book (Sorcerer/Archer)","Druidic book (Druid/Ranger)"};
    for(unsigned book=0;book<3;++book)for(unsigned slot=0;slot<39;++slot)
        if(const auto id=XeenLearnedSpellRules::spellForSlot(static_cast<XeenSpellCategory>(book),slot))
            spell(c,*id,names,0,0,books[book]);
    for(unsigned power=1;power<=73;++power) {XeenItem value{};value.material=1;value.id=power;value.state=1;item(c,catalog,3,value,0,0,"Misc catalogue",names,context);}
    for(unsigned category=0;category<3;++category)for(unsigned material=0;material<=130;++material) {
        XeenItem value{};value.id=1;value.material=material;item(c,catalog,category,value,0,0,"equipment material catalogue",names,context);
    }
    const auto rosterBytes=assets.readInitialResource("maze.chr");
    const auto roster=XeenCharacterFormat::parseRoster(rosterBytes);
    for(unsigned i=0;i<XeenRoster::kCharacterCount;++i) {
        const auto &character=roster.at(i);
        for(unsigned category=0;category<4;++category)
            for(const auto &value:*xeenInventoryItems(character,static_cast<XeenInventoryCategory>(category)))
                item(c,catalog,category,value,0,1,"initial roster",names,context);
        const auto category=XeenLearnedSpellRules::categoryForClass(character.characterClass);
        const auto learned=XeenCharacterFormat::parseLearnedSpells(rosterBytes,i);
        if(category)for(unsigned slot=0;slot<learned.size();++slot)if(learned[slot])
            if(const auto id=XeenLearnedSpellRules::spellForSlot(*category,slot))spell(c,*id,names,0,1,"initial roster");
    }
    const auto read=[&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
        if(assets.hasInitialResource(name,side))return assets.readInitialResource(name,side);
        return {};
    };
    // Reference map IDs are bytes. A CC hash match is not enough: DAT identity must agree.
    const Support lighting=XeenScenePresentation::blackView(0x4000,0) &&
        !XeenScenePresentation::blackView(0x4000,1) && !XeenScenePresentation::blackView(0,0) &&
        XeenScenePresentation::checkedLight(1,0x20,true)==0 &&
        XeenScenePresentation::checkedLight(1,0x20,false)==1 &&
        XeenScenePresentation::checkedLight(0,0x20,true)==0 ?
        Support{Status::Supported,"XeenScenePresentation black/clear and stepped drain probes accept"}:
        Support{Status::Unsupported,"Lighting rule probe failed"};
    c.add("Map features","Light-drain cell",lighting,0,0);
    std::map<unsigned,Support> skillTraversal;
    for(unsigned wall:{1u,7u,9u,10u,12u,3u,6u}) {
        XeenMapGeometry probe;probe.flags2=0x8000;probe.surfaceTypes[0]=2;
        XeenMapCell cell;cell.geometry=XeenOutdoorLayers{0,std::uint8_t(wall),0,0};
        XeenMovement::Capabilities enabled;enabled.mountaineer=wall!=3 && wall!=6;enabled.pathfinder=!enabled.mountaineer;
        const bool accepts=XeenMovement::outdoorDestination(probe,cell,{})==XeenMovementResult::BlockedByTerrain &&
            XeenMovement::outdoorDestination(probe,cell,enabled)==XeenMovementResult::Moved;
        const Support support{accepts?Status::Supported:Status::Unsupported,"XeenMovement skill capability refusal/admission probe"};
        skillTraversal.emplace(wall,support);
        c.add("Map features",std::string(enabled.pathfinder?"Pathfinder":"Mountaineer")+" wall="+std::to_string(wall),support,0,0);
    }
    for(unsigned number=1;number<=255;++number) {
        const auto dat=read(resource(number,"dat"));if(!dat)continue;
        const XeenMapIdentity id{side,static_cast<std::uint16_t>(number)};
        XeenMapGeometry geometry;
        try {geometry=XeenMapFormat::parseDat(*dat);if(geometry.id!=number)continue;}
        catch(const std::exception &e) {c.add("Unclassified","DAT candidate "+std::to_string(number),rejected(e));continue;}
        if(geometry.flags2&0x4000) {
            c.add("Map features","Dark physical DAT tile",lighting,number);
            // Original naming separates primary mazeNNNN from mazexNNN tiles.
            if(number<100)c.add("Map features","Dark primary map",lighting,number);
        }
        auto &entry=c.maps[number];
        try {entry.name=assets.mapName(id);}catch(const std::exception &) {entry.name="(name unavailable)";}
        try {
            requireXeenMap(id);
            if(!assets.hasInitialResource(resource(number,"mob"),side)) {
                c.add("Map features","Physical indoor geometry",{Status::Supported,"Neighbor DAT tile; gameplay retains the primary logical map identity"},number);
            } else {
                const auto admitted=context.world.prepareEventTransition({id,0,0,XeenDirection::North});
                c.add("Map features","Journey map admission",{admitted?Status::Supported:Status::Unsupported,
                    admitted?"XeenWorld admits the original primary DAT and coordinate":admitted.reason},number);
            }
        } catch(const std::exception &e) {c.add("Map features","Journey map admission",{Status::Unsupported,e.what()},number);}
        XeenMapEntities entities;bool mobDecoded=false;
        if(const auto mob=read(resource(number,"mob"))) {
            try {entities=XeenMapFormat::parseMob(*mob);mobDecoded=true;}
            catch(const std::exception &e) {c.add("Unclassified","MOB decode",rejected(e),number);}
        }
        // World CD's Darkside 117..120 and 124 intentionally use xeen.mon.
        const bool usesClouds=side==XeenSide::Clouds || (number>=117 && number<=120) || number==124;
        const auto &statistics=usesClouds?cloudsMonsters:darkMonsters;
        if(mobDecoded)for(const auto type:entities.monsterTable)if(type!=255 && type<statistics.size())
            monster(c,statistics[type],type,number,0,std::string(usesClouds?"xeen.mon ":"dark.mon ")+std::to_string(type)+" "+statistics[type].name());
        for(const auto &m:entities.monsters)if(m.hasResource()) {
            if(unsigned(m.resourceId)>=statistics.size()) {c.add("Unclassified","Monster ID "+std::to_string(m.resourceId),{Status::Unclassified,"MOB monster outside statistics catalogue"},number);continue;}
            const auto &record=statistics[m.resourceId];
            monster(c,record,unsigned(m.resourceId),number,1,std::string(usesClouds?"xeen.mon ":"dark.mon ")+std::to_string(m.resourceId)+" "+record.name());
        }
        std::map<unsigned,Support> arrivalSupportBySurface;
        for(const auto &cell:geometry.cells) {
            if(cell.rawAttributes&0x20)c.add("Map features","Light-drain cell",lighting,number);
            const unsigned surface=geometry.surfaceTypes[cell.surfaceIndex];
            if(geometry.isOutdoors()) {
                const auto &layers=xeenGet<XeenOutdoorLayers>(cell.geometry);
                Support traversal{Status::Supported,"XeenMovement::outdoorDestination accepts ordinary traversal"};
                try {
                    const auto ordinary=XeenMovement::outdoorDestination(geometry,cell,{});
                    if(ordinary==XeenMovementResult::BlockedByTerrain || ordinary==XeenMovementResult::BlockedBySurface) {
                        XeenMovement::Capabilities capabilities;
                        capabilities.mountaineer=true;capabilities.pathfinder=true;
                        capabilities.swimming=true;capabilities.walkOnWater=true;
                        try {
                            const auto enabled=XeenMovement::outdoorDestination(geometry,cell,capabilities);
                            if(enabled!=XeenMovementResult::Moved && !(enabled==XeenMovementResult::BlockedBySurface && surface==15))
                                traversal={Status::Partial,"Traversal capability probe refused"};
                        }
                        catch(const std::exception &e) {traversal={Status::Partial,e.what()};}
                    }
                } catch(const std::exception &e) {traversal=rejected(e);}
                c.add("Map features","Traversal middle="+std::to_string(layers.middle)+" surface="+std::to_string(surface),traversal,number);
                const auto wall=unsigned(layers.middle);
                if(wall==1 || wall==7 || wall==9 || wall==10 || wall==12 || wall==3 || wall==6)
                    c.add("Map features",std::string(wall==3 || wall==6?"Pathfinder":"Mountaineer")+" wall="+std::to_string(wall),skillTraversal.at(wall),number);
            }
            c.add("Map features","Rest surface="+std::to_string(surface),
                restSupport(surface,geometry.isOutdoors()),number);
            auto arrival=arrivalSupportBySurface.find(surface);
            if(arrival==arrivalSupportBySurface.end())arrival=arrivalSupportBySurface.emplace(surface,
                arrivalSupport(surface,geometry.isOutdoors(),{id,0,0,XeenDirection::North},context.world)).first;
            c.add("Map features","Arrival surface="+std::to_string(surface),arrival->second,number);
        }
        for(const auto &o:entities.objects)if(o.hasResource())c.add("Map features","Object sprite="+std::to_string(o.resourceId),
            {Status::Unclassified,"Object behavior depends on selected Event and state; see Event rows"},number);
        const XeenObjectFile objectFile{id,resource(number,"mob"),mobDecoded,entities};
        for(unsigned i=0;i<entities.objects.size();++i)if(entities.objects[i].hasResource()) {
            const auto visual=objectVisuals.resolve(objectFile,i,XeenDirection::North,0);
            using V=XeenObjectVisualStatus;
            const auto status=visual.status==V::SupportedStatic || visual.status==V::SupportedAnimated?Status::Supported:
                visual.status==V::UnsupportedAnimation || visual.status==V::UnsupportedSide?Status::Unsupported:Status::Unclassified;
            c.add("Map features","Object visual sprite="+std::to_string(entities.objects[i].resourceId),
                {status,visual.diagnostic.empty()?"XeenObjectVisualResolver accepts":visual.diagnostic},number);
        }
        for(const auto &w:entities.wallItems)if(w.hasResource())c.add("Map features","Wall item="+std::to_string(w.resourceId),
            {Status::Unclassified,"No engine wall-item behavior support query"},number);
        if(const auto evt=read(resource(number,"evt"))) {
            try {events(c,XeenEventFormat::parse(*evt),id,entities,catalog,names,context,geometry.isOutdoors());}
            catch(const std::exception &e) {throw std::runtime_error(c.side+" map "+std::to_string(number)+" Event census failed: "+e.what());}
        }
    }
    return c;
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=4 || (std::string(argv[2])!="--write" && std::string(argv[2])!="--check")) {
            std::cerr<<"Usage: mmodern_census <game-dir> --write|--check <coverage.md>\n";return 2;
        }
        const auto installation=XeenInstallationDetector().detect(argv[1]);
        if(!installation || !installation->hasXeen() || !installation->hasDarkside())throw std::runtime_error("World of Xeen CD installation required");
        OriginalContext context(*installation);
        auto clouds=census(*installation,XeenSide::Clouds,context);
        auto darkside=census(*installation,XeenSide::Darkside,context);
        reconcileContexts(clouds,darkside);
        const auto text=report(clouds,darkside);
        if(std::string(argv[2])=="--check") {
            std::ifstream in(argv[3],std::ios::binary);const std::string old{std::istreambuf_iterator<char>(in),{}};
            if(old!=text) {std::cerr<<"Coverage report is stale. Regenerate with mmodern_census <game-dir> --write "<<argv[3]<<"\n";return 1;}
            std::cout<<"Coverage report is current.\n";
        } else {
            // Never write into any detected commercial-data directory.
            const auto output=std::filesystem::weakly_canonical(argv[3]);
            for(const auto &dir:installation->protectedDirectories) {
                for(auto parent=output.parent_path();!parent.empty();) {
                    if(std::filesystem::exists(parent) && std::filesystem::equivalent(parent,dir))
                        throw std::runtime_error("Report destination is inside original data");
                    const auto next=parent.parent_path();if(next==parent)break;parent=next;
                }
            }
            std::ofstream out(output,std::ios::binary|std::ios::trunc);out<<text;
            if(!out)throw std::runtime_error("Cannot write coverage report");
            std::cout<<"Wrote "<<output<<"\n";
        }
        std::cout<<"Clouds maps="<<clouds.maps.size()<<" Events="<<clouds.instructions<<"; Darkside maps="<<darkside.maps.size()<<" Events="<<darkside.instructions<<"\n";
        return 0;
    }catch(const std::exception &e) {std::cerr<<"Census: "<<e.what()<<"\n";return 1;}
}
