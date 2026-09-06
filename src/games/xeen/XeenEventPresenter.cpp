#include "games/xeen/XeenEventPresenter.h"

#include <algorithm>
#include <stdexcept>
#include <variant>

namespace mmodern {
namespace {

void fill(IndexedFrame &frame, int left, int top, int right, int bottom,
		std::uint8_t color) {
	left = std::max(0, left);
	top = std::max(0, top);
	right = std::min(frame.width, right);
	bottom = std::min(frame.height, bottom);
	for (int y = top; y < bottom; ++y) {
		std::fill(frame.pixels.begin() + static_cast<std::size_t>(y) * frame.width + left,
			frame.pixels.begin() + static_cast<std::size_t>(y) * frame.width + right, color);
	}
}

} // namespace

XeenEventPresenter::XeenEventPresenter(const XeenFontFormat &font) :
		_renderer(font) {
}

XeenTextRenderOptions XeenEventPresenter::optionsFor(
		const XeenPresentationRequest &request) const {
	XeenTextRenderOptions options;
	switch (request.kind) {
	case XeenPresentationKind::CenteredMessage:
		options.bounds = {225, 140, 320, 199};
		options.x = 233;
		options.y = 148;
		options.alignment = XeenTextAlignment::Center;
		options.drawWindow = true;
		options.paginate = true;
		break;
	case XeenPresentationKind::SceneLabelReduced:
		options.bounds = {0, 0, 230, 149};
		options.x = 0;
		options.y = 25;
		options.alignmentAnchor = 116;
		options.size = XeenFontSize::Reduced;
		options.colorIndex = 8;
		options.alignment = XeenTextAlignment::Center;
		break;
	case XeenPresentationKind::SceneLabelNormal:
		options.bounds = {0, 0, 230, 149};
		options.x = 0;
		options.y = 30;
		options.alignmentAnchor = 116;
		options.colorIndex = 4;
		options.alignment = XeenTextAlignment::Center;
		break;
	case XeenPresentationKind::SceneLabelSign:
		options.bounds = {0, 0, 230, 149};
		options.x = 0;
		options.y = 88;
		options.alignmentAnchor = 120;
		options.colorIndex = 8;
		options.alignment = XeenTextAlignment::Center;
		break;
	case XeenPresentationKind::BottomWindowMessage:
	case XeenPresentationKind::BottomWindowTwoLines:
		options.bounds = {0, 143, 320, 199};
		options.x = 8;
		options.y = request.kind == XeenPresentationKind::BottomWindowTwoLines ? 178 : 151;
		options.alignment = XeenTextAlignment::Center;
		options.drawWindow = true;
		options.paginate = request.kind == XeenPresentationKind::BottomWindowMessage;
		break;
	case XeenPresentationKind::MainWindowMessage:
		options.bounds = {8, 8, 224, 140};
		options.x = 16;
		options.y = 16;
		options.alignment = XeenTextAlignment::Center;
		options.drawWindow = true;
		options.paginate = true;
		break;
	case XeenPresentationKind::Confirmation:
		break;
	}
	if (options.drawWindow) {
		options.windowBounds = options.bounds;
		options.bounds.left += 8;
		options.bounds.top += 8;
		options.bounds.right -= 8;
		options.bounds.bottom -= 8;
	}
	return options;
}

IndexedFrame XeenEventPresenter::drawConfirmation(const IndexedFrame &base) const {
	IndexedFrame result = base;
	fill(result, 232, 74, 285, 96, 0x99);
	fill(result, 234, 76, 258, 94, 0xa4);
	fill(result, 236, 78, 256, 92, 0x99);
	fill(result, 260, 76, 284, 94, 0x97);
	fill(result, 262, 78, 282, 92, 0x99);
	XeenTextRenderOptions yes;
	yes.bounds = {236, 78, 256, 92};
	yes.x = 236;
	yes.y = 81;
	yes.alignment = XeenTextAlignment::Center;
	XeenTextRenderOptions no = yes;
	no.bounds = {262, 78, 282, 92};
	no.x = 262;
	result = _renderer.render(result, "Y", yes).pages.front();
	result = _renderer.render(result, "N", no).pages.front();
	return result;
}

XeenPresentationUpdate XeenEventPresenter::present(const IndexedFrame &base,
		const XeenPresentationRequest &request) {
	if (!base.isValid())
		throw std::invalid_argument("frame base invalido para apresentacao Xeen");
	_request = request;
	_underlay = base;
	_page = 0;
	_diagnostics.clear();
	if (request.kind == XeenPresentationKind::Confirmation) {
		_pages = {drawConfirmation(base)};
	} else {
		const XeenTextRenderResult rendered = _renderer.render(base, request.text,
			optionsFor(request));
		_pages = rendered.pages;
		_diagnostics = rendered.diagnostics;
	}
	_frame = _pages.front();
	const bool waitsBetweenPages = _pages.size() > 1 &&
		request.response == XeenPresentationResponseRequirement::Presented;
	_active = waitsBetweenPages ||
		request.response != XeenPresentationResponseRequirement::Presented;
	XeenPresentationUpdate update{_frame, std::nullopt, true};
	if (!_active)
		update.response = XeenPresentationResponse::Presented;
	return update;
}

bool XeenEventPresenter::isAcknowledge(const PlayerAction &action) const {
	return std::holds_alternative<InteractionAction>(action) ||
		std::holds_alternative<AcknowledgeAction>(action);
}

XeenPresentationUpdate XeenEventPresenter::handle(const PlayerAction &action) {
	XeenPresentationUpdate update{_frame, std::nullopt, _active};
	if (!_active)
		return update;
	if (_request.response == XeenPresentationResponseRequirement::Presented) {
		if (!isAcknowledge(action))
			return update;
		if (_page + 1 < _pages.size()) {
			_frame = _pages[++_page];
			update.frame = _frame;
		}
		if (_page + 1 == _pages.size()) {
			_active = false;
			update.response = XeenPresentationResponse::Presented;
		}
	} else if (_request.response ==
			XeenPresentationResponseRequirement::Acknowledgment) {
		if (isAcknowledge(action)) {
			_active = false;
			update.response = XeenPresentationResponse::Acknowledged;
			if (_request.kind == XeenPresentationKind::Confirmation)
				update.frame = _frame = _underlay;
		}
	} else if (std::holds_alternative<YesAction>(action)) {
		_active = false;
		update.response = XeenPresentationResponse::Yes;
		update.frame = _frame = _underlay;
	} else if (std::holds_alternative<NoAction>(action)) {
		_active = false;
		update.response = XeenPresentationResponse::No;
		update.frame = _frame = _underlay;
	}
	return update;
}

} // namespace mmodern
