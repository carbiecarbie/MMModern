#ifndef MMODERN_PLATFORM_SDL_WINDOW_H
#define MMODERN_PLATFORM_SDL_WINDOW_H

#include "core/IndexedFrame.h"
#include "core/PlayerAction.h"

#include <functional>
#include <optional>
#include <string>

namespace mmodern {

class SdlWindow {
public:
	using FrameUpdateHandler = std::function<std::optional<IndexedFrame>(const PlayerAction &)>;
	using IdleFrameHandler = std::function<std::optional<IndexedFrame>()>;

	bool show(const IndexedFrame &frame, const std::string &title) const;
	bool showInteractive(const IndexedFrame &frame, const std::string &title,
		const FrameUpdateHandler &handler,
		const std::function<bool()> &handlesEscape = {},
		const IdleFrameHandler &idle = {},
		const std::function<std::string()> &status = {}) const;
};

} // namespace mmodern

#endif
