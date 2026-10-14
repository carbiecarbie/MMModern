#include "XeenCoverage.h"
#include "games/xeen/XeenJourneyRules.h"
#include <iostream>
#include <stdexcept>
using namespace mmodern;
using namespace mmodern::coverage;
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
XeenEventRecord record(unsigned opcode,std::vector<std::uint8_t> parameters={}) {
    XeenEventRecord r;r.opcode=opcode;r.direction=4;r.parameters=std::move(parameters);r.lengthField=5+r.parameters.size();return r;
}
int main() {
    try {
        check(eventMechanic(record(0x09,{20,1,1})).find("mode=20")!=std::string::npos,"condition dimension");
        check(eventMechanic(record(0x1c,{19,10,0,0})).find("operand1=10")!=std::string::npos,"unsupported variant still groups shared operand shape");
        check(spellSupport(26).status==Status::Supported && spellSupport(0).status==Status::Unsupported,"spell checks");
        check(restSupport(5,true).status==Status::Unsupported,"rest refusal is missing support");
        check(restSupport(13,true).status==Status::Unsupported,"rest clears levitation before checking terrain");
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
        std::cout<<"Coverage synthetic checks passed\n";return 0;
    }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
