#ifndef MMODERN_XEEN_ACTOR_H
#define MMODERN_XEEN_ACTOR_H
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenMap.h"
#include "formats/xeen/XeenMonsterFormat.h"
#include <optional>

namespace mmodern {
enum class XeenActorLifecycle { Present, Disabled, Unresolved };
enum class XeenActorStatus { Physical, Unsupported };
struct XeenActor {
	XeenMonsterIdentity id;
	XeenMapEntity original;
	int x = 0, y = 0;
	std::optional<XeenMonsterRecord> statistics;
	std::int32_t hp = 0;
	bool activated = false;
	XeenActorLifecycle lifecycle = XeenActorLifecycle::Unresolved;
	XeenActorStatus status = XeenActorStatus::Physical;
};
}
#endif
