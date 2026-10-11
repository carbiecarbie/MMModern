#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenSkills.h"
#include <iostream>
using namespace mmodern;
namespace {
void check(bool value,const char *text) {if(!value)throw std::runtime_error(text);}
template<class F>void rejects(F operation) {bool rejected=false;try{operation();}catch(const std::invalid_argument &){rejected=true;}check(rejected,"Invalid exploration accepted");}
XeenMap tile(XeenMapIdentity id) {
 if(id.number<60 || id.number>63)throw std::invalid_argument("Absent physical tile");
 XeenMap m;m.geometry.id=id.number;m.side=id.side;
 if(id.number==60)m.geometry.neighbors=std::array<std::uint16_t,4>{62,61,0,0};
 if(id.number==61)m.geometry.neighbors=std::array<std::uint16_t,4>{63,0,0,60};
 if(id.number==62)m.geometry.neighbors=std::array<std::uint16_t,4>{0,63,60,0};
 if(id.number==63)m.geometry.neighbors=std::array<std::uint16_t,4>{0,0,61,62};
 for(auto &c:m.geometry.cells)c.geometry=XeenIndoorWalls{};
 if(id.number==60)m.geometry.cells[0].stepped=true;
 m.geometry.cells[1].seen=true;
 return m;
}
void ownership() {
 XeenWorld w(tile);XeenPartyState p;XeenCamera c{60,15,15,XeenDirection::North};XeenGameFlags flags;
 p.party=XeenParty::fromRosterIds({0,1});p.roster.at(2).skills[4]=1;
 w.recordExploration(p,c);check(w.sessionState().exploredTiles().empty(),"Inactive Cartographer records nothing");
 check(w.sampleCell(60,0,0)->cell->stepped && !w.sampleCell(60,1,0)->cell->stepped,"seen and stepped must remain distinct");
 p.roster.at(1).skills[4]=1;p.roster.at(1).conditions[13]=1;
 XeenRestoreGuard before(w,p,c,flags);
 w.recordExploration(p,c);check(!before.current(),"Exploration writes invalidate retained preimages");
 const auto first=w.sessionState().exploredTiles();check(first.size()==1 && first[0].mapId==60 && first[0].cells[31]==128,"Physical cell255 bit");
 check(w.sampleCell(60,15,15)->cell->stepped && !w.sampleCell(60,14,15)->cell->stepped,"Occupied cell only");
 XeenRestoreGuard unchanged(w,p,c,flags);w.recordExploration(p,c);
 check(unchanged.current(),"Repeated occupied-cell observation is read-only");
 check(!w.map(60).geometry.cells[255].stepped,"Original MAZ cells remain immutable");
 for(const auto point:std::array<std::pair<int,int>,3>{{{16,15},{15,16},{16,16}}}) {
  c.x=point.first;c.y=point.second;w.recordExploration(p,c);
 }
 const auto values=w.sessionState().exploredTiles();check(values.size()==4,"All quadrant seams retained");
 for(unsigned n=0;n<4;++n)check(values[n].mapId==XeenMapIdentity(60+n),"Explored tiles sorted by physical identity");
 w.discardMapCache();check(w.sampleCell(60,16,16)->cell->stepped,"Cache reload retains explored cells");
}
void restoreAndBarriers() {
 XeenWorld w(tile);XeenPartyState p;p.party=XeenParty::fromRosterIds({0});p.roster.at(0).skills[4]=1;
 XeenCamera c{60,15,15,XeenDirection::East};w.recordExploration(p,c);
 const auto values=w.sessionState().exploredTiles();XeenWorld restored(tile);restored.restoreExploredTiles(values);
 check(restored.sampleCell(60,15,15)->cell->stepped && restored.sampleCell(60,0,0)->cell->stepped,"Restore merges resource defaults without replay");
 auto bad=values;bad.push_back(values[0]);rejects([&]{restored.restoreExploredTiles(bad);});
 bad=values;bad[0].mapId={XeenSide::Darkside,60};rejects([&]{restored.restoreExploredTiles(bad);});
 bad=values;bad[0].mapId=0;rejects([&]{restored.restoreExploredTiles(bad);});
 bad=values;bad[0].mapId=64;rejects([&]{restored.restoreExploredTiles(bad);});
 bad.resize(513);rejects([&]{restored.restoreExploredTiles(bad);});
 check(restored.sessionState().exploredTiles()==values,"Rejected restore publishes nothing");
 w.markEncounterSession(XeenEncounterEntry::Journey);auto candidate=w.transitionCandidate();
 candidate->setBarrier(c,3,true);candidate->recordExploration(p,{60,16,15,XeenDirection::East});
 check(candidate->sampleCell(60,15,15)->cell->stepped && wallAt(*candidate->sampleCell(60,15,15)->cell,XeenDirection::East)==3,
  "Effective exploration and opened barriers share one cell");
 w.publishTransition(*candidate);check(w.sampleCell(60,16,15)->cell->stepped,"Transitions preserve exploration");
}
}
int main() {try {
 ownership();
 restoreAndBarriers();std::cout<<"Exploration ownership and physical tiles passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
