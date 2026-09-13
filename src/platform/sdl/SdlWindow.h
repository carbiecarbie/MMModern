#ifndef MMODERN_PLATFORM_SDL_WINDOW_H
#define MMODERN_PLATFORM_SDL_WINDOW_H

#include "core/IndexedFrame.h"
#include "core/PlayerAction.h"

#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace mmodern {

class SdlWindow {
public:
	struct FrameUpdateHandler : std::function<std::optional<IndexedFrame>(const PlayerAction &)> {
		using Function = std::function<std::optional<IndexedFrame>(const PlayerAction &)>;
		using Function::Function;
		FrameUpdateHandler() = default;
		FrameUpdateHandler(Function function) : Function(std::move(function)) {}
		// One identity per event batch/idle iteration, independent of elapsed time.
		std::function<void(std::uint64_t)> beginCycle;
		std::function<bool()> frameCurrent;
		// Called only after successful upload and normal current-frame presentation.
		std::function<void()> framePresented;
		std::function<void()> failed;
		std::function<void()> closed;
		bool protectAllKeys = false;
		std::function<std::optional<std::uint64_t>()> displayedInput;
		std::function<std::optional<IndexedFrame>(const PlayerAction &,std::uint64_t)> withDisplayedInput;
	};
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
