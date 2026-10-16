#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_PRESENTER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_PRESENTER_H

#include "core/PlayerAction.h"
#include "games/xeen/XeenTextInput.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenTextRenderer.h"
#include "games/xeen/XeenDialogView.h"

#include <optional>
#include <string>
#include <vector>
#include <random>

namespace mmodern {

struct XeenPresentationUpdate {
	IndexedFrame frame;
	std::optional<XeenPresentationResponse> response;
	bool consumed = false;
};

class XeenEventPresenter {
public:
	struct NpcTiming;
	using NpcDraw = std::function<void(IndexedFrame &, std::uint8_t, std::size_t)>;
	using Clock = std::function<std::uint64_t()>;
	using RandomFrame = std::function<unsigned()>;
	explicit XeenEventPresenter(const XeenFontFormat &font, NpcDraw npcDraw = {},
		Clock clock = {}, RandomFrame randomFrame = {});

	// Consecutive presents layer over frame(); clear before starting a new scene.
	XeenPresentationUpdate present(const IndexedFrame &base,
		const XeenPresentationRequest &request, std::size_t retainedNpcPage=0);
	XeenPresentationUpdate handle(const PlayerAction &action, bool finishResponse = true);
	// Rebuild displayed layers and all pending pages without producing a response.
	IndexedFrame rebase(const IndexedFrame &base);
	IndexedFrame dismissSelection();
	// Finish the consumed request, retaining passive text and removing transient UI.
	IndexedFrame finishPresentation();
	// Removes incomplete/transient NPC even if composition failed before activation.
	IndexedFrame discardNpc();
	// Removes transient UI even when present/layout failed before activation.
	void discardTransient() noexcept;
	// Compose the Flow's retained choice; neither clock nor RNG belongs to a view.
	std::optional<IndexedFrame> updateNpc(const NpcTiming &);
	void clear();

	bool hasNpcOffer() const;
	void closeNpcOffers();
    bool window11Enabled() const;
    void closeWindow12();
	bool blocksGameplay() const { return _active; }
	const IndexedFrame &frame() const { return _frame; }
	const std::vector<std::string> &diagnostics() const { return _diagnostics; }
	std::size_t pageIndex() const { return _page; }
	std::size_t pageCount() const { return _pages.size(); }
	struct NpcTiming {
		unsigned displayedFrame = 0, nextFrame = 0, phase = 0;
		std::uint64_t remaining = 0, deadline = 0;
	};
	const NpcTiming &npcTiming() const { return _npcTiming; }
    void retainNpcTiming(const NpcTiming &timing) { _npcTiming=timing; }
	void originalAssets(const XeenDosText *text, XeenDialogSpriteDraw draw) {
		_dosText=text; _spriteDraw=std::move(draw);
	}
	DialogInput input() const;
    bool wordInputActive() const {return _active && _request.kind==XeenPresentationKind::WordInput;}
    std::optional<std::uint64_t> wordDeadline() const {return _wordInput.deadline();}
    bool animateWord(std::uint64_t now) {return wordInputActive() && _wordInput.animate(now);}


private:
	XeenTextRenderOptions optionsFor(const XeenPresentationRequest &request) const;
	XeenTextRenderResult drawText(const IndexedFrame &, const XeenPresentationRequest &) const;
	IndexedFrame overlayNotices(IndexedFrame) const;
	IndexedFrame drawWord(const IndexedFrame &) const;
	XeenTextInput _wordInput;
	IndexedFrame drawConfirmation(const IndexedFrame &base) const;
	XeenTextRenderResult drawSelection(const IndexedFrame &base,
		const XeenPresentationRequest &request) const;
	bool isAcknowledge(const PlayerAction &action) const;
	void layoutNpc(const IndexedFrame &base, const XeenPresentationRequest &request);
	IndexedFrame drawNpcPage() const;
	void startNpcPage();
	NpcDraw _npcDraw;
	const XeenDosText *_dosText=nullptr;
	XeenDialogSpriteDraw _spriteDraw;
	NpcTiming _npcTiming;
	std::vector<std::size_t> _npcPageSourceEnds;

	const XeenFontFormat &_font;
	XeenTextRenderer _renderer;
	XeenPresentationRequest _request;
	std::vector<IndexedFrame> _pages;
	std::size_t _page = 0;
	IndexedFrame _frame;
	IndexedFrame _underlay;
	std::vector<std::string> _diagnostics;
	bool _active = false;
	struct Layer {
		XeenPresentationRequest request;
		std::size_t page;
	};
	std::vector<Layer> _layers;
};

} // namespace mmodern

#endif
