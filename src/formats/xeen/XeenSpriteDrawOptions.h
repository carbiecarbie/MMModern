#ifndef MMODERN_FORMATS_XEEN_SPRITE_DRAW_OPTIONS_H
#define MMODERN_FORMATS_XEEN_SPRITE_DRAW_OPTIONS_H

namespace mmodern {

// Platform-neutral options. ScummVM flags are deliberately confined to the bridge.
struct XeenSpriteDrawOptions {
	int scaleIndex = 0;
	bool horizontalFlip = false;
	bool sceneClipped = false;
	bool bottomClipped = false;
	bool enlarge = false;
};

} // namespace mmodern

#endif
