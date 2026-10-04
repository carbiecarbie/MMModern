#ifndef MMODERN_FORMATS_XEEN_SPRITE_DRAW_OPTIONS_H
#define MMODERN_FORMATS_XEEN_SPRITE_DRAW_OPTIONS_H
#include <cstdint>

namespace mmodern {

// File::open's explicit CC selection, scoped to a presentation command.
enum class XeenSceneArchive { Current, Clouds, Darkside };

// Platform-neutral options. ScummVM flags are deliberately confined to the bridge.
struct XeenSpriteDrawOptions {
	XeenSceneArchive archive = XeenSceneArchive::Current;
	int scaleIndex = 0;
	bool horizontalFlip = false;
	bool sceneClipped = false;
	bool bottomClipped = false;
	bool enlarge = false;
	// Original low twelve-bit monster effect flags, selected from metadata.
	unsigned monsterEffectFlags = 0;
	std::uint32_t monsterEffectSeed = 0;
};

} // namespace mmodern

#endif
