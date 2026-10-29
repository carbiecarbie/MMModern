#ifndef MMODERN_XEEN_OBJECT_STATE_H
#define MMODERN_XEEN_OBJECT_STATE_H
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenMutation.h"
#include <algorithm>
#include <cstdint>
#include <vector>
namespace mmodern {
// Resource-relative cmdMoveObj values; source MOB records remain immutable.
struct XeenObjectOverride {
 XeenObjectIdentity id;
 XeenMutable<int> x=0,y=0;
 XeenMutable<bool> flipped=false;
 // The MOB picture-table index (DOS object byte +2) when the end-of-run chest
 // rule changed it (0x28e04, 0x28e69); absent is the record's own. It belongs to
 // the record: MoveObj and Remove write only x and y (0x27913..0x27937), and
 // the map writer saves it (0x266c8).
 XeenMutableOptional<std::uint8_t> tableIndex;
 friend bool operator==(const XeenObjectOverride &a,const XeenObjectOverride &b) noexcept {
  return a.id==b.id && a.x==b.x && a.y==b.y && a.flipped==b.flipped && a.tableIndex==b.tableIndex;
 }
 friend bool operator!=(const XeenObjectOverride &a,const XeenObjectOverride &b) noexcept {return !(a==b);}
};
// Remove's position (-128,-128) replaces a record's override, which then goes,
// unless the chest rule changed its table index: Remove writes only x and y
// (DOS 0x27913..0x27937), so that override stays there without a flip.
inline void xeenRemoveObjectOverride(std::vector<XeenObjectOverride> &values,XeenObjectIdentity id) {
 const auto at=std::find_if(values.begin(),values.end(),[&](const XeenObjectOverride &v){return v.id==id;});
 if(at==values.end())return;
 if(at->tableIndex) {at->x=-128;at->y=-128;at->flipped=false;}
 else values.erase(at);
}
}
#endif
