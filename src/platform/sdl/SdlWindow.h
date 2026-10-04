#ifndef MMODERN_PLATFORM_SDL_WINDOW_H
#define MMODERN_PLATFORM_SDL_WINDOW_H

#include "core/IndexedFrame.h"
#include "core/PlayerAction.h"
#include "core/InputContext.h"

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
		// Checks the identity carried by the supplied immutable frame, never relabels it.
		std::function<bool(const IndexedFrame::Presentation &)> acceptsFrame;
		// A cosmetic upload may coexist with the still actionable acquired origin.
		std::function<bool(const IndexedFrame::Presentation &)> acceptsInputFrame;
		// Follow framePresented with this after the final bounded old-origin batch.
		// Alternate Show implementations complete both steps when acquiring a frame.
		std::function<void(const IndexedFrame::Presentation &)> completeInputHandoff;
		// Called only after successful upload and normal current-frame presentation.
		std::function<void(const IndexedFrame::Presentation &)> framePresented;
		std::function<void()> failed;
		std::function<void()> closed;
		// Readiness of the supplied presented origin, never of an unacquired upload.
		std::function<InputContext(const IndexedFrame::Presentation &)> inputContext;
		// Draws on a temporary native-display copy; never acquires a gameplay frame.
		std::function<void(IndexedFrame &,const InputButton &)> drawButton;
		bool protectAllKeys = false;
		std::function<std::optional<std::uint64_t>()> displayedInput;
		std::function<std::optional<IndexedFrame>(const PlayerAction &,std::uint64_t)> withDisplayedInput;
		// Native responses retain the concrete frame sampled with their semantic input.
		std::function<std::optional<IndexedFrame>(const PlayerAction &,std::uint64_t,
			const IndexedFrame::Presentation &)> withPresentedInput;
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
