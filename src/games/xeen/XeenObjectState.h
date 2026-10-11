#ifndef MMODERN_XEEN_OBJECT_STATE_H
#define MMODERN_XEEN_OBJECT_STATE_H
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenMutation.h"
namespace mmodern {
// Resource-relative cmdMoveObj values; source MOB records remain immutable.
struct XeenObjectOverride {
 XeenObjectIdentity id;
 XeenMutable<int> x=0,y=0;
 XeenMutable<bool> flipped=false;
 friend bool operator==(const XeenObjectOverride &a,const XeenObjectOverride &b) noexcept {
  return a.id==b.id && a.x==b.x && a.y==b.y && a.flipped==b.flipped;
 }
 friend bool operator!=(const XeenObjectOverride &a,const XeenObjectOverride &b) noexcept {return !(a==b);}
};
}
#endif
