#include "XeenCoverage.h"
#include "XeenCoverageContext.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenJourneyRules.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
using namespace mmodern::coverage;
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
XeenEventRecord record(unsigned opcode,std::vector<std::uint8_t> parameters={}) {
    XeenEventRecord r;r.opcode=opcode;r.direction=4;r.parameters=std::move(parameters);r.lengthField=5+r.parameters.size();return r;
}
void fixtureContexts(const char *directory) {
    const auto installation=xeenTestInstallationDetector().detect(directory);
    check(installation.has_value(),"fixture original installation");
    const XeenCamera start{27,10,10,XeenDirection::North};
    OriginalContext context(*installation,start);
    check(arrivalSupport(5,true,start,context.world).status==Status::Supported,
        "lava arrival has a production rule query");
    check(arrivalSupport(10,true,{25,0,0,XeenDirection::North},context.world).status==Status::Partial,
        "implemented sky fall names its unadmitted destination");
    check(arrivalSupport(13,true,{23,0,0,XeenDirection::North},context.world).status==Status::Supported,
        "cloud fall can reach its open mainland water destination");
    check(context.world.developerEventMap()==start.mapId && context.camera.x==10 && context.camera.y==10,
        "documented developer fixture entry");
    const auto at=[](XeenCamera camera,unsigned line,unsigned opcode,std::vector<std::uint8_t> parameters={}) {
        auto r=record(opcode,std::move(parameters));r.x=camera.x;r.y=camera.y;r.line=line;return r;
    };
    const auto texts=[&](auto id){return context.text(id);};
    const auto probe=[&](XeenEventRecord r,std::optional<XeenCamera> caller={}) {
        return eventSupport(r,XeenEventFile{start.mapId,"fixture",true,{r}},
            context.world,context.party,texts,caller);
    };
    for(const auto &r:{at(start,7,0x12),at(start,7,0x20,{0,0}),at(start,7,0x01,{0}),
            at(start,7,0x19,{100,100,9})})
        check(probe(r).status==Status::Supported,"valid fixture operation context");
    check(probe(at({27,100,100,XeenDirection::North},9,0x1a),start).status==Status::Supported,
        "Return has a physical caller and an active call frame");
    // Return's target remains a subroutine even when its coordinates are
    // physical. An actual CallEvent can also share that same physical cell.
    const auto physicalReturn=at(start,1,0x1a);
    const XeenEventFile physicalSubroutine{start.mapId,"fixture",true,
        {at(start,0,0x19,{10,10,1}),physicalReturn}};
    const auto physicalCaller=physicalEventCaller(physicalReturn,physicalSubroutine,false);
    check(physicalCaller.has_value(),"physical Return resolves an original CallEvent caller");
    check(eventSupport(physicalReturn,physicalSubroutine,context.world,context.party,texts,physicalCaller).status==Status::Supported,
        "same-cell physical Return uses an active call frame without wrapper-line collisions");
    const auto limit=probe(at(start,0,0x08,{9,0,0}));
    check(limit.status==Status::Unclassified && limit.reason.find("Census limit:")==0,
        "real original-line self-target remains an explicit census safety limit");
    std::optional<XeenObjectIdentity> selected;
    XeenCamera destination;
    const auto &objects=context.world.objectFile(start.mapId).entities.objects;
    for(unsigned i=0;i<objects.size();++i) {
        const auto &object=objects[i];
        if(!object.isActive() || (object.x<16 && object.y<16) || object.x<0 || object.y<0 ||
                object.x>=32 || object.y>=32 || !context.world.sampleCell(start.mapId,object.x,object.y))continue;
        destination={start.mapId,object.x,object.y,XeenDirection::North};
        selected=context.world.selectObject(destination);
        if(selected)break;
    }
    check(selected.has_value(),"original indoor object beyond the primary 16x16 DAT");
    const auto destinationX=static_cast<std::uint8_t>(destination.x),destinationY=static_cast<std::uint8_t>(destination.y);
    check(probe(at(start,7,0x0b,{static_cast<std::uint8_t>(selected->recordIndex),destinationX,destinationY})).status==Status::Supported,
        "MoveObj retains the original object owner in the fixture");
    auto world=context.world.transitionCandidate();XeenPartyState party;
    world->copyEventParty(context.party,party);
    XeenEventFile continued{start.mapId,"fixture",true,{
        at(start,0,0x1f,{27,destinationX,destinationY}),at(destination,0,0x01,{0})}};
    const auto scripts=[&](auto){return XeenEventScript(continued);};
    auto result=XeenEventInterpreter().begin(start,party,{},*world,scripts,texts);
    const auto *pending=std::get_if<XeenEventExecutionSuspended>(&result);
    check(pending && pending->state.selectedObject==selected && pending->state.workingCamera.x==destination.x &&
            pending->state.workingCamera.y==destination.y,
        "TeleportAndContinue reselects the original destination object in valid indoor context");
    auto called=context.world.transitionCandidate();XeenPartyState calledParty;
    called->copyEventParty(context.party,calledParty);
    continued.records={at(start,0,0x19,{100,100,0}),
        at({27,100,100,XeenDirection::North},0,0x1f,{27,destinationX,destinationY})};
    result=XeenEventInterpreter().begin(start,calledParty,{},*called,scripts,texts);
    check(std::holds_alternative<XeenEventExecutionCompleted>(result),"Continue with active call discards caller and naturally closes at destination");

}
int main(int argc,char **argv) {
    try {
        check(eventMechanic(record(0x09,{20,1,1})).find("mode=20")!=std::string::npos,"condition dimension");
        check(eventMechanic(record(0x1c,{19,10,0,0})).find("operand1=10")!=std::string::npos,"unsupported variant still groups shared operand shape");
        check(spellSupport(26).status==Status::Supported && spellSupport(0).status==Status::Supported &&
            spellSupport(24).status==Status::Supported && spellSupport(29).status==Status::Supported &&
            spellSupport(32).status==Status::Supported && spellSupport(46).status==Status::Supported &&
            spellSupport(51).status==Status::Supported,"spell checks");
        check(restSupport(5,true).status==Status::Supported,"lava Rest applies shared arrival after recovery");
        check(restSupport(13,true).status==Status::Supported,"cloud Rest applies shared arrival after losing levitation");
        check(restSupport(6,true).status==Status::Supported,"supported navigator damage variation is not a gap");
        Census c;c.side="Synthetic";c.maps[1].name="A|B";
        c.add("Events","mode 1",{Status::Unsupported,"engine reason"},1,3);
        c.add("Events","mode 1",{Status::Unsupported,"engine reason"},1,2);
        c.add("Events","mode 1",{Status::Partial,"context reason"},1,1);
        c.add("Events","mode 1",{Status::Unsupported,"engine reason"},2,1);
        check(c.rows.size()==2 && c.maps[1].unsupported.size()==1 && c.maps[1].partial.size()==1,"unique mechanics not occurrences");
        const auto rendered=c.markdown();
        check(rendered==c.markdown() && rendered.find("A B")!=std::string::npos,"deterministic escaped output");
        check(rendered.find("| unsupported | 6 | 2 | 1, 2 |")!=std::string::npos,"counts and map impact");
        Census other;other.side="Other";
        other.add("Events","mode 1",{Status::Supported,"real owner admits"},3,1);
        reconcileContexts(c,other);
        check(c.maps[1].unsupported.empty() && c.maps[1].partial.size()==2,"cross-context refusal must not rank as a missing mechanic");
        c.add("Monsters","missing attack",{Status::Unsupported,"engine rejects"},1,2);
        const auto combined=report(c,other);
        check(combined.find("## Largest gaps")!=std::string::npos &&
            combined.find("| missing attack | Monsters | 1 | 2 |")!=std::string::npos,"cross-dimension impact summary");
        const auto malformed=XeenEventDecoder::decode(record(0x09,{20}),{{},{},{},true});
        check(std::holds_alternative<XeenEventDecodeError>(malformed),"bounded decoder preserves malformed input evidence");
        auto branch=record(0x08,{9,1,0});branch.x=2;branch.y=3;branch.line=7;
        const auto isolated=isolatedEventProbe(branch,29);
        const XeenEventScript branchScript(isolated);
        check(branchScript.findInstruction(2,3,XeenDirection::North,7)->opcode==0x08,
            "isolated probe retains the original line");
        check(branchScript.findInstruction(2,3,XeenDirection::North,0)->opcode==0x12 &&
            branchScript.findInstruction(2,3,XeenDirection::North,8)->opcode==0x12,
            "branch to original line zero and sequential continuation are inert exits");
        branch.line=0;
        const auto self=isolatedEventProbe(branch,29);
        check(self.records.size()==2 && self.records[0].opcode==0x08,
            "true original self-target retains the instruction-limit witness");
        auto subroutine=record(0x1a);subroutine.x=100;subroutine.y=100;subroutine.line=9;
        const auto called=isolatedEventProbe(subroutine,29,XeenCamera{29,2,3,XeenDirection::North});
        const XeenEventScript callScript(called);
        const auto *entry=callScript.findInstruction(2,3,XeenDirection::North,0);
        check(entry && entry->parameters==std::vector<std::uint8_t>({100,100,9}),
            "physical caller enters the original subroutine line");
        if(argc==2)fixtureContexts(argv[1]);
        const XeenEventFile noCaller{29,"fixture",true,{record(0x1a)}};
        check(!physicalEventCaller(noCaller.records[0],noCaller,false),"unbound physical Return remains a census context limit");
        std::cout<<"Coverage synthetic checks passed\n";return 0;
    }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
