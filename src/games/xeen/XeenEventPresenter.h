#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_PRESENTER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_PRESENTER_H

#include "core/PlayerAction.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenTextRenderer.h"

#include <optional>
#include <string>
#include <vector>

namespace mmodern {

struct XeenPresentationUpdate {
	IndexedFrame frame;
	std::optional<XeenPresentationResponse> response;
	bool consumed = false;
};

class XeenEventPresenter {
public:
	explicit XeenEventPresenter(const XeenFontFormat &font);

	XeenPresentationUpdate present(const IndexedFrame &base,
		const XeenPresentationRequest &request);
	XeenPresentationUpdate handle(const PlayerAction &action);

	bool blocksGameplay() const { return _active; }
	const IndexedFrame &frame() const { return _frame; }
	const std::vector<std::string> &diagnostics() const { return _diagnostics; }

private:
	XeenTextRenderOptions optionsFor(const XeenPresentationRequest &request) const;
	IndexedFrame drawConfirmation(const IndexedFrame &base) const;
	bool isAcknowledge(const PlayerAction &action) const;

	XeenTextRenderer _renderer;
	XeenPresentationRequest _request;
	std::vector<IndexedFrame> _pages;
	std::size_t _page = 0;
	IndexedFrame _frame;
	IndexedFrame _underlay;
	std::vector<std::string> _diagnostics;
	bool _active = false;
};

} // namespace mmodern

#endif
