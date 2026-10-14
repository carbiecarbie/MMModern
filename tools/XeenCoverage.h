#ifndef MMODERN_TOOLS_XEEN_COVERAGE_H
#define MMODERN_TOOLS_XEEN_COVERAGE_H
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenMap.h"
#include "games/xeen/XeenDialogView.h"
#include <map>
#include <set>
#include <string>

namespace mmodern::coverage {
enum class Status { Supported, Partial, Unsupported, Unclassified };
struct Support {
    Status status=Status::Unclassified;
    std::string reason;
    std::optional<XeenLocationDialog> service;
};
struct Row {
    std::string dimension, mechanic;
    Support support;
    std::size_t uses=0;
    std::set<unsigned> maps;
    std::set<std::string> sources;
};
struct MapReadiness { std::string name; std::set<std::string> unsupported, partial, unclassified; };
class Census {
public:
    std::string side;
    std::map<std::string,Row> rows;
    std::map<unsigned,MapReadiness> maps;
    std::size_t instructions=0, monsters=0;
    void add(std::string dimension,std::string mechanic,Support support,
        unsigned map=0,std::size_t count=1,std::string source={});
    std::string markdown() const;
};
Support eventSupport(const XeenEventRecord &, const XeenEventFile &, const XeenWorld &,
    const XeenPartyState &,const XeenEventInterpreter::TextProvider &,
    std::optional<XeenCamera> caller={});
std::string eventMechanic(const XeenEventRecord &);
Support spellSupport(unsigned id);
Support restSupport(unsigned surface,bool outdoor);
Support itemSupport(unsigned category,const XeenItem &,const XeenWorld &,const XeenPartyState &);
Support equipmentCombatSupport(unsigned,const XeenItem &,const XeenWorld &,const XeenPartyState &);
void reconcileContexts(Census &,Census &);
std::string report(const Census &,const Census &);
const char *statusName(Status);
}
#endif
