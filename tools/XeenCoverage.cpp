#include "XeenCoverage.h"
#include "XeenCoverageScreens.h"
#include "games/xeen/XeenEventDiagnostics.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include "games/xeen/XeenItemServiceRules.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenEquipment.h"
#include "games/xeen/XeenItemTransfer.h"
#include "games/xeen/XeenRestRules.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include <algorithm>
#include <array>
#include <sstream>
#include <tuple>

namespace mmodern::coverage {
namespace {
std::string escape(std::string s) {
    for(auto &c:s) if(c=='|' || c=='\r' || c=='\n' || static_cast<unsigned char>(c)<32)c=' ';
    return s;
}
template<class T> std::string join(const T &values) {
    std::ostringstream out;bool first=true;
    for(const auto &v:values) {if(!first)out<<", ";first=false;out<<v;}
    return first?"—":out.str();
}
bool unsupported(XeenEventExecutionErrorKind kind) {
    using K=XeenEventExecutionErrorKind;
    return kind==K::UnsupportedOpcode || kind==K::UnsupportedOperand ||
        kind==K::UnsupportedConditionAction || kind==K::UnsupportedOperationMode;
}
}
const char *statusName(Status s) {
    switch(s) {case Status::Supported:return "supported";case Status::Partial:return "partial";
        case Status::Unsupported:return "unsupported";default:return "unclassified";}
}
void Census::add(std::string dimension,std::string mechanic,Support support,
        unsigned map,std::size_t count,std::string source) {
    // Different engine reasons remain visible, never collapse to the most optimistic one.
    const auto key=dimension+"/"+mechanic+"/"+statusName(support.status)+"/"+support.reason;
    auto &row=rows[key];row.dimension=std::move(dimension);row.mechanic=std::move(mechanic);
    row.support=std::move(support);row.uses+=count;
    if(!source.empty())row.sources.insert(std::move(source));
    if(map) {
        row.maps.insert(map);auto &m=maps[map];
        if(row.support.status==Status::Unsupported)m.unsupported.insert(key);
        if(row.support.status==Status::Partial)m.partial.insert(key);
        if(row.support.status==Status::Unclassified)m.unclassified.insert(key);
    }
}
Support spellSupport(unsigned id) {
    if(!XeenLearnedSpellRules::supported(id))return {Status::Unsupported,"XeenLearnedSpellRules::supported rejects this effect"};
    // CombatForbidden is an original rule, not missing implementation.
    return {Status::Supported,"XeenLearnedSpellRules::supported; original combat restrictions still apply"};
}
Support restSupport(unsigned surface,bool outdoor) {
    // beginRest resets temporary effects before this query, including levitation.
    // Different supported damage durations are original rules, not coverage gaps.
    const bool ordinary=xeenRestTerrain(surface,outdoor,false,false).has_value();
    const bool navigator=xeenRestTerrain(surface,outdoor,true,false).has_value();
    if(ordinary && navigator)return {Status::Supported,"xeenRestTerrain admits after beginRest resets temporary effects"};
    return {ordinary || navigator?Status::Partial:Status::Unsupported,"Rest terrain is not supported yet"};
}
Support itemSupport(unsigned category,const XeenItem &item,const XeenWorld &source,const XeenPartyState &initial) {
    if(category==3)return xeenItemUseSupported(item)?Support{Status::Supported,"xeenItemUseSupported admits; charges and acting-member requirements are gameplay rules"}:
        Support{Status::Unsupported,"xeenItemUseSupported rejects this power"};
    auto world=source.transitionCandidate();XeenPartyState party;world->copyEventParty(initial,party);
    // Test the Equip operation separately from combat/Rest contribution limits.
    // Clear conflicts in a private value copy; preserve real New Game class/rules.
    const auto cat=static_cast<XeenInventoryCategory>(category);
    for(unsigned index=0;index<party.party.size();++index) {
        auto &character=party.roster.at(party.party.activeRosterIds()[index]);
        character.weapons={};character.armor={};character.accessories={};
        auto candidate=item;candidate.frame=0;
        (*xeenInventoryItems(character,cat))[0]=candidate;
        const auto result=xeenSetEquipment(party,index,cat,0,XeenEquipmentOperation::Equip);
        if(result.status==XeenEquipmentStatus::Success || result.status==XeenEquipmentStatus::NoChange)
            return {Status::Supported,"xeenSetEquipment admits; proficiency and slot conflicts are gameplay rules"};
        if(result.status!=XeenEquipmentStatus::NotProficient)
            return {Status::Unsupported,"xeenSetEquipment status="+std::to_string(unsigned(result.status))};
    }
    return {Status::Partial,"xeenSetEquipment: no proficient member in the New Game party"};
}
Support equipmentCombatSupport(unsigned category,const XeenItem &item,const XeenWorld &source,const XeenPartyState &initial) {
    auto world=source.transitionCandidate();XeenPartyState party;world->copyEventParty(initial,party);
    const auto cat=static_cast<XeenInventoryCategory>(category);
    for(unsigned index=0;index<party.party.size();++index) {
        auto &character=party.roster.at(party.party.activeRosterIds()[index]);
        character.weapons={};character.armor={};character.accessories={};
        auto candidate=item;candidate.frame=0;(*xeenInventoryItems(character,cat))[0]=candidate;
        const auto result=xeenSetEquipment(party,index,cat,0,XeenEquipmentOperation::Equip);
        if(result.status==XeenEquipmentStatus::Success || result.status==XeenEquipmentStatus::NoChange) {
            try {xeenValidateJourneyMelee(party);return {Status::Supported,"xeenValidateJourneyMelee accepts equipped candidate"};}
            catch(const std::exception &e) {return {Status::Unsupported,e.what()};}
        }
    }
    return {Status::Unclassified,"No legal equipped candidate; see Equip operation row"};
}
Support eventSupport(const XeenEventRecord &original,const XeenEventFile &originalFile,const XeenWorld &source,
        const XeenPartyState &initial,const XeenEventInterpreter::TextProvider &texts,std::optional<XeenCamera> caller) {
    const auto id=originalFile.mapId;
    const auto decoded=XeenEventDecoder::decode(original,{id,{}, {},true});
    if(const auto *bad=std::get_if<XeenEventDecodeError>(&decoded))
        return {bad->kind==XeenEventDecodeErrorKind::MalformedInstruction?Status::Unclassified:Status::Unsupported,bad->message};
    try {
        auto world=source.transitionCandidate();XeenPartyState party;
        world->copyEventParty(initial,party);
        // Preserve the actual cell, facing, operands, resource-bound actors and RNG.
        // Isolate the operation using inert suffixes; this measures capabilities,
        // not path reachability. A call's continuation is measured by its own row.
        auto record=original;record.line=0;
        const XeenCamera camera=caller.value_or(XeenCamera{id,record.x,record.y,
            record.direction<4?XeenDirection(record.direction):XeenDirection::North});
        record.direction=4;
        XeenEventFile file{id,"census",true,{record}};
        std::set<unsigned> exitLines{1};
        const auto &operation=std::get<XeenDecodedEventInstruction>(decoded).operation;
        if(const auto *condition=std::get_if<XeenEventConditional>(&operation))exitLines.insert(condition->targetLine);
        if(const auto *npc=std::get_if<XeenEventNpc>(&operation))exitLines.insert(npc->targetLine);
        if(const auto *call=std::get_if<XeenEventCallEvent>(&operation))
            if(call->x==record.x && call->y==record.y)exitLines.insert(call->line);
        for(const auto line:exitLines)if(line) {
            XeenEventRecord end=record;end.opcode=0x12;end.lengthField=5;
            end.parameters.clear();end.direction=4;end.line=line;file.records.push_back(end);
        }
        if(const auto *call=std::get_if<XeenEventCallEvent>(&std::get<XeenDecodedEventInstruction>(decoded).operation)) {
            if(call->x!=record.x || call->y!=record.y) {
                XeenEventRecord end;end.opcode=0x12;end.lengthField=5;end.direction=4;
                end.x=call->x;end.y=call->y;end.line=call->line;file.records.push_back(end);
            }
        }
        if(caller) {
            XeenEventRecord call;call.x=camera.x;call.y=camera.y;call.direction=4;call.line=0;
            call.opcode=0x19;call.parameters={record.x,record.y,0};call.lengthField=8;
            file.records.push_back(call);
            call.opcode=0x12;call.parameters.clear();call.lengthField=5;call.line=1;file.records.push_back(call);
        }
        if(const auto *alter=std::get_if<XeenEventAlterEvent>(&operation)) {
            // This operation addresses another original physical record. Check
            // against the complete real file, not the isolated execution suffix.
            world->applyAlterEvent(camera,alter->line,alter->replacement,originalFile);
            return {Status::Supported,"applyAlterEvent accepts the original physical record and detached Journey owner"};
        }
        const XeenEventScript script(file);
        const auto scripts=[&](XeenMapIdentity target) {
            if(target==id)return script;
            return XeenEventScript(XeenEventFile{target,"census",true,{}});
        };
        auto result=XeenEventInterpreter().begin(camera,party,{},*world,scripts,texts);
        for(unsigned step=0;step<256;++step) {
            if(const auto *bad=std::get_if<XeenEventExecutionError>(&result)) {
                if(bad->kind==XeenEventExecutionErrorKind::QuestItemUnderflow || bad->kind==XeenEventExecutionErrorKind::QuestItemOverflow)
                    return {Status::Supported,"Implemented quest-counter refusal: "+bad->message};
                return {unsupported(bad->kind)?Status::Unsupported:Status::Partial,bad->message};
            }
            auto *pending=std::get_if<XeenEventExecutionSuspended>(&result);
            if(!pending)return {Status::Supported,"New Game Journey candidate completes with original world, owners and RNG"};
            if(pending->request.kind==XeenPresentationKind::DeferredAudio)
                return {Status::Partial,pending->request.text};
            // A service request is the engine's terminal Event-to-Service handoff.
            // Service actions are counted separately from admission to the location.
            if(pending->state.pendingPresentation->continuation==XeenEventPendingContinuation::Terminate &&
                    std::holds_alternative<XeenEventTownService>(std::get<XeenDecodedEventInstruction>(decoded).operation))
            {
                using K=XeenPresentationKind;using L=XeenLocationDialog;
                const auto kind=pending->request.kind;
                const auto location=kind==K::BankService?L::Bank:kind==K::TavernService?L::Tavern:
                    kind==K::TempleService?L::Temple:kind==K::TrainingService?L::Training:
                    kind==K::GuildService?L::Guild:L::Smith;
                return {Status::Supported,"Original terminal Event-to-Service handoff admitted with Journey owners",location};
            }
            // A legitimate question/selection is supported input, not absent ownership.
            if(pending->request.response==XeenPresentationResponseRequirement::YesNo ||
                    pending->request.response==XeenPresentationResponseRequirement::CharacterSelection)
                return {Status::Supported,"Original Event input admitted with Journey owners"};
            const auto response=pending->request.response==XeenPresentationResponseRequirement::Presented?
                XeenPresentationResponse::Presented:XeenPresentationResponse::Acknowledged;
            result=XeenEventInterpreter().resume(std::move(pending->state),response,party,*world,scripts,texts);
        }
        return {Status::Unclassified,"Census continuation bound reached"};
    } catch(const std::exception &e) {return {Status::Partial,e.what()};}
}
std::string eventMechanic(const XeenEventRecord &r) {
    std::string key=std::to_string(r.opcode)+" "+XeenEventDiagnostics::opcodeName(r.opcode);
    auto shape=r;
    // The pinned command table routes these variants to cmdTakeOrGive. Decode
    // their shared operand shape for grouping only; eventSupport sees the real opcode.
    if(shape.opcode==0x1c || shape.opcode==0x1d || shape.opcode==0x26)shape.opcode=0x0c;
    const auto decoded=XeenEventDecoder::decode(shape,{{},{},{},true});
    if(const auto *ins=std::get_if<XeenDecodedEventInstruction>(&decoded)) {
        const auto &op=ins->operation;
        if(auto p=std::get_if<XeenEventConditional>(&op)) {
            key+=" mode="+std::to_string(p->action);
            if(p->action==13 || p->action==18 || p->action==19 || p->action==21 || p->action==44 || p->action==99)
                key+=" operand="+std::to_string(p->value);
        }
        const XeenEventTakeOrGive *give=std::get_if<XeenEventTakeOrGive>(&op);
        if(auto p=std::get_if<XeenEventGiveMulti>(&op))give=p;
        if(give) {
            key+=" modes="+std::to_string(give->first.mode)+","+std::to_string(give->second.mode)+","+std::to_string(give->third.mode);
            unsigned slot=0;
            for(const auto &p:{give->first,give->second,give->third}) {
                ++slot;
                if(p.mode==13 || p.mode==15 || p.mode==18 || p.mode==19 || p.mode==21 || p.mode==66 || p.mode==99)
                    key+=" operand"+std::to_string(slot)+"="+std::to_string(p.value);
            }
        }
        if(auto p=std::get_if<XeenEventNpc>(&op))key+=" mode="+std::to_string(p->confirmationMode);
        if(auto p=std::get_if<XeenEventDisplay>(&op))key+=" layout="+(p->layoutValue?std::to_string(*p->layoutValue):"default");
        if(auto p=std::get_if<XeenEventTownService>(&op))key+=" action="+std::to_string(p->action);
        if(auto p=std::get_if<XeenEventSetVar>(&op))key+=" mode="+std::to_string(p->mode);
        if(auto p=std::get_if<XeenEventTeleportAndExit>(&op))key+=" destination="+std::to_string(p->mapId);
        if(auto p=std::get_if<XeenEventTeleportAndContinue>(&op))key+=" destination="+std::to_string(p->mapId);
        if(auto p=std::get_if<XeenEventDamage>(&op))key+=" type="+std::to_string(p->type);
        if(auto p=std::get_if<XeenEventSpawn>(&op))key+=" slot="+std::to_string(p->slot);
        if(auto p=std::get_if<XeenEventAlterEvent>(&op))key+=" replacement="+std::to_string(p->replacement);
        if(auto p=std::get_if<XeenEventGiveEnchanted>(&op))key+=" item="+std::to_string(p->itemCode)+" enchantment="+std::to_string(p->specialId);
    } else if(!r.parameters.empty()) {
        // Diagnostic operand labels only; support is ALWAYS the original decoder's result.
        if(r.opcode==0x11)key+=" action="+std::to_string(r.parameters[0]);
        if(r.opcode==0x07 || r.opcode==0x1f)key+=" destination="+std::to_string(r.parameters[0]);
        if(r.opcode==0x0f)key+=" selector="+std::to_string(r.parameters[0]);
        if(r.opcode==0x1b || r.opcode==0x08 || r.opcode==0x09 || r.opcode==0x0a)
            key+=" mode="+std::to_string(r.parameters[0]);
        if(r.opcode==0x05 && r.parameters.size()>=4)key+=" mode="+std::to_string(r.parameters[3]);
        if(r.opcode==0x2c && r.parameters.size()>=2)key+=" item="+std::to_string(r.parameters[0])+" enchantment="+std::to_string(r.parameters[1]);
    }
    // Named semantic operands from pinned scripts.cpp; never infer support here.
    if(!r.parameters.empty()) {
        if(r.opcode==0x15 || r.opcode==0x37)key+=" inputType="+std::to_string(r.parameters[0]);
        if(r.opcode==0x2a)key+=" monster="+std::to_string(r.parameters[0]);
        if(r.opcode==0x2d)key+=" itemType="+std::to_string(r.parameters[0]);
        if(r.opcode==0x34)key+=" destination="+std::to_string(r.parameters[0]);
        if(r.opcode==0x23 && r.parameters.size()>=3)key+=" surface="+std::to_string(r.parameters[2]);
        if(r.opcode==0x13 && r.parameters.size()>=4)key+=" wall="+std::to_string(r.parameters[3]);
        if(r.opcode==0x21)key+=" damageType="+std::to_string(r.parameters[0]);
    }
    return key;
}
std::string Census::markdown() const {
    std::ostringstream out;
    std::map<std::string,std::array<std::size_t,4>> totals,uses;
    for(const auto &[key,row]:rows) {++totals[row.dimension][unsigned(row.support.status)];uses[row.dimension][unsigned(row.support.status)]+=row.uses;}
    out<<"## "<<side<<" totals\n\n"<<maps.size()<<" map resources; "<<instructions<<" Event instructions; "<<monsters<<" monster catalogue records.\n\n";
    out<<"| Dimension | Supported | Partial | Unsupported | Unclassified | Total mechanic rows | Uses |\n| --- | ---: | ---: | ---: | ---: | ---: | ---: |\n";
    for(const auto &[name,n]:totals) {const auto &u=uses.at(name);out<<"| "<<name<<" | "<<n[0]<<" | "<<n[1]<<" | "<<n[2]<<" | "<<n[3]<<" | "<<n[0]+n[1]+n[2]+n[3]<<" | "<<u[0]+u[1]+u[2]+u[3]<<" |\n";}
    if(uses.count("Events")) {const auto &u=uses.at("Events");out<<"\n"<<u[0]<<" of "<<instructions<<" Event instructions pass the isolated support checks; "<<u[1]<<" partial, "<<u[2]<<" unsupported, "<<u[3]<<" unclassified.\n";}
    out<<"\n### Per-map readiness\n\nCounts are distinct mechanic/reason rows, not occurrences. Zero blockers does not certify a playthrough.\n\n| Map | Name | Unsupported | Partial | Unclassified |\n| ---: | --- | ---: | ---: | ---: |\n";
    for(const auto &[id,m]:maps)out<<"| "<<id<<" | "<<escape(m.name)<<" | "<<m.unsupported.size()<<" | "<<m.partial.size()<<" | "<<m.unclassified.size()<<" |\n";
    for(const auto &[dimension,unused]:totals) {
        out<<"\n### "<<dimension<<"\n\n| Mechanic | Status | Uses | Map count | Maps | Sources / engine reason |\n| --- | --- | ---: | ---: | --- | --- |\n";
        std::vector<const Row *> sorted;
        for(const auto &[key,row]:rows)if(row.dimension==dimension)sorted.push_back(&row);
        std::sort(sorted.begin(),sorted.end(),[](const Row *a,const Row *b) {
            const auto priority=[](Status s){return s==Status::Unsupported?0:s==Status::Partial?1:s==Status::Unclassified?2:3;};
            return std::tuple(priority(a->support.status),-int(a->maps.size()),-int(a->uses),a->mechanic,a->support.reason)<
                std::tuple(priority(b->support.status),-int(b->maps.size()),-int(b->uses),b->mechanic,b->support.reason);
        });
        for(const auto *r:sorted)out<<"| "<<escape(r->mechanic)<<" | "<<statusName(r->support.status)<<" | "<<r->uses<<" | "<<r->maps.size()<<" | "<<join(r->maps)<<" | "<<escape(join(r->sources)+"; "+r->support.reason)<<" |\n";
    }
    return out.str();
}
void reconcileContexts(Census &clouds,Census &darkside) {
    std::set<std::pair<std::string,std::string>> admitted;
    for(const auto *c:{&clouds,&darkside})for(const auto &[key,row]:c->rows)
        if(row.support.status==Status::Supported)admitted.emplace(row.dimension,row.mechanic);
    for(auto *c:{&clouds,&darkside}) {
        for(auto &[id,map]:c->maps) {map.unsupported.clear();map.partial.clear();map.unclassified.clear();}
        for(auto &[key,row]:c->rows) {
            if(row.support.status==Status::Unsupported && admitted.count({row.dimension,row.mechanic})) {
                row.support.status=Status::Partial;
                row.support.reason+="; same mechanic is admitted in another observed context";
            }
            for(const auto id:row.maps) {
                auto &map=c->maps[id];
                if(row.support.status==Status::Unsupported)map.unsupported.insert(key);
                if(row.support.status==Status::Partial)map.partial.insert(key);
                if(row.support.status==Status::Unclassified)map.unclassified.insert(key);
            }
        }
    }
}
std::string largestGaps(const Census &c) {
    struct Impact {std::string dimension,mechanic;std::set<unsigned> maps;std::size_t uses=0;};
    std::map<std::string,Impact> grouped;
    for(const auto &[key,row]:c.rows) {
        if(row.support.status!=Status::Unsupported || row.maps.empty() || row.dimension=="Monster species" ||
                row.mechanic=="Journey map admission" || row.mechanic.rfind("Town service action=",0)==0)continue;
        auto &impact=grouped[row.dimension+"/"+row.mechanic];impact.dimension=row.dimension;impact.mechanic=row.mechanic;
        impact.maps.insert(row.maps.begin(),row.maps.end());impact.uses+=row.uses;
    }
    std::vector<Impact> sorted;for(const auto &[key,value]:grouped)sorted.push_back(value);
    std::sort(sorted.begin(),sorted.end(),[](const auto &a,const auto &b) {
        return std::tuple(-int(a.maps.size()),-int(a.uses),a.dimension,a.mechanic)<
            std::tuple(-int(b.maps.size()),-int(b.uses),b.dimension,b.mechanic);
    });
    std::ostringstream out;out<<"### "<<c.side<<" largest gaps\n\n| Mechanic | Dimension | Maps | Uses |\n| --- | --- | ---: | ---: |\n";
    for(unsigned i=0;i<std::min<std::size_t>(20,sorted.size());++i) {
        const auto &r=sorted[i];out<<"| "<<escape(r.mechanic)<<" | "<<r.dimension<<" | "<<r.maps.size()<<" | "<<r.uses<<" |\n";
    }
    return out.str()+"\n";
}
std::string report(const Census &clouds,const Census &darkside) {
    const auto a=clouds.markdown(),b=darkside.markdown();
    const auto ap=a.find("### Per-map readiness"),bp=b.find("### Per-map readiness");
    const std::string screenText=screens();
    std::string screenTotals="## Manual screen totals\n\n";
    for(const auto status:{Status::Supported,Status::Partial,Status::Unsupported,Status::Unclassified}) {
        const auto token=std::string("| ")+statusName(status)+" |";
        unsigned count=0;std::size_t cursor=0;
        while((cursor=screenText.find(token,cursor))!=std::string::npos) {++count;cursor+=token.size();}
        screenTotals+=std::to_string(count)+" "+statusName(status)+"; ";
    }
    screenTotals+="shared checklist, not data-derived.\n\n";
    return "# Coverage census\n\nGenerated by `mmodern_census <game-dir> --write docs/coverage.md`. Check without writing: `--check docs/coverage.md`.\n\n"+
        a.substr(0,ap)+b.substr(0,bp)+screenTotals+"## Largest gaps\n\nTop 20 unsupported mechanics per side, by distinct map impact across dimensions. Reasons are merged by mechanic. Species rows and duplicate service-admission rows are excluded. Journey map-entry restrictions are reported separately in readiness.\n\n"+largestGaps(clouds)+largestGaps(darkside)+"## Interpretation\n\n"+
        "Read-only World of Xeen CD census. Original resources are never exported. The report retains only counts, identifiers and in-game names.\n\n"
        "Support comes from engine decoders and execution with original New Game owners, geometry, actors and Journey RNG (seed 7), "
        "initialized by XeenEncounterFlow. Each operation uses a fresh detached candidate; mutations never reach the retained source or disk. "
        "Off-map subroutines use physical callers found in the resource. Inert suffixes isolate capabilities, so this is not a reachability or full-playthrough proof. "
        "Terminal Vertigo services are also checked through the live EventFlow handoff. The mainland legacy well uses live Flow publication. "
        "Original quest-counter refusals, proficiency and equipment conflicts are gameplay rules, not missing implementation. "
        "When the same mechanic is admitted elsewhere, its engine refusal is partial in that context instead of a globally missing mechanic. "
        "Audio remains partial because its original timing/animation runs silently. Records rejected as malformed and evidence not classifiable by engine checks remain unclassified.\n\n"
        "Items measure Equip/Use. Equipment combat/Rest is separate: it asks the actual xeenValidateJourneyMelee guard, which since M67 admits every "
        "material, effectiveness counter and broken/cursed state the original accepts and refuses only records outside the generated tables. No predicate from the old diagnostic monster profile is used to judge current scene animation. "
        "The calibration gate accepts only the documented mirror/word-input, audio and six service-action gaps in Vertigo; it never overrides a support result. "
        "Calibration exceptions: ConfirmWord is the unsupported mirror destination input; PlayFX, PlayEventVoc and PlayCD preserve presentation but lack audio; "
        "four Event row types (Return, Display0x01, If2 mode 44 and WhoWill) remain unclassified because their off-map subroutines have no physical caller in the resource. "
        "No missing owner, RNG, publication or service provider is an allowed Vertigo exception. "
        "Generic arrival effects, object/wall behavior, resistance and drop rows still carry explicit unknowns; they do not certify a map as playable.\n\n"
        "Rows split when the same mechanic has different support reasons. Uses mean instruction records, monster placements (including disabled spawn slots), "
        "terrain cells, object/wall placements or explicit item references, as appropriate. Catalogue entries have zero uses unless referenced. "
        "Monster maps include encounter-table membership even when no placed actor uses a slot (zero additional placements). "
        "Map impact is the number of distinct maps containing a row; rows are sorted by unsupported/partial first, then map impact and occurrences. "
        "Map resources include the neighboring 16x16 geometry tiles used by larger indoor locations, not just primary map destinations. "
        "Random treasure/stock does not identify a particular generated item; its generation sites are reported separately. Shared spell/item catalogues appear on both sides. "
        "Darkside counts describe resources; Clouds-only engine admission remains a separate map blocker.\n\n"
        "Monsters have no party-spell ID in the reference format: their magic is represented by attack type, special and range rows. "
        "No monster-to-party-spell crosswalk is inferred. Spell uses count book references and item powers, not casting frequency. "
        "Unclassified rows identify the missing evidence: arrival effects, object/wall behavior, resistances, generic monster drops and rejected resource candidates. "
        "No route simulation or random-drop expansion is performed.\n\n"+
        "## Clouds detail\n\n"+a.substr(ap)+"\n## Darkside detail\n\n"+b.substr(bp)+screenText;
}
}
