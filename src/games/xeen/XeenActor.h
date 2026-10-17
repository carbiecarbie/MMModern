#ifndef MMODERN_XEEN_ACTOR_H
#define MMODERN_XEEN_ACTOR_H
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenMap.h"
#include "formats/xeen/XeenMonsterFormat.h"
#include <optional>

#include "games/xeen/XeenMutation.h"
namespace mmodern {
enum class XeenActorLifecycle { Present, Disabled, Unresolved, Defeated };
enum class XeenActorStatus { Physical, Unsupported, Sleep, BeastMaster, Hypnotize, DragonSleep };
inline bool xeenActorStatusSupported(XeenActorStatus status) noexcept {
	switch(status) {
	case XeenActorStatus::Physical: case XeenActorStatus::Sleep:
	case XeenActorStatus::BeastMaster: case XeenActorStatus::Hypnotize:
	case XeenActorStatus::DragonSleep: return true;
	default: return false;
	}
}
inline XeenActorStatus xeenActorStatusAfterDamage(XeenActorStatus status, bool nonzeroEntry) noexcept {
	return nonzeroEntry && (status==XeenActorStatus::Sleep || status==XeenActorStatus::DragonSleep) ?
		XeenActorStatus::Physical : status;
}
struct XeenActor {
	XeenMonsterIdentity id;
	XeenMapEntity original;
	XeenMutable<int> x = 0, y = 0;
	XeenMutableOptional<XeenMonsterRecord> statistics;
	XeenMutable<std::int32_t> hp = 0;
	XeenMutable<bool> activated = false;
	XeenMutable<XeenActorLifecycle> lifecycle = XeenActorLifecycle::Unresolved;
	XeenMutable<XeenActorStatus> status = XeenActorStatus::Physical;
};
}
#endif
