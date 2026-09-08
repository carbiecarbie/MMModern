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
	case XeenPresentationKind::CharacterSelection:
		options.bounds = {225, 74, 320, 154};
		options.x = 233;
		options.y = 82;
		options.alignment = XeenTextAlignment::Center;
		options.drawWindow = true;
		break;
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

XeenTextRenderResult XeenEventPresenter::drawSelection(const IndexedFrame &base,
		const XeenPresentationRequest &request) const {
	static constexpr const char *verbs[] = {
		"search", "open", "drink", "mine", "touch", "read", "learn", "take",
		"bang", "steal", "bribe", "pay", "sit", "try", "turn", "bathe",
		"destroy", "pull", "descend", "toss a coin", "pray", "join", "act", "play",
		"push", "rub", "pick", "eat", "sign", "close", "look", "try"
	};
	const auto verb = request.verbIndex.value_or(0);
	if (verb >= 32 || request.members.empty() || request.members.size() > 6)
		throw std::invalid_argument("invalid WhoWill presentation metadata");
	// Bound the title independently so a long map string cannot displace the
	// question or the selection keys. Reuse the existing metric wrapping/clipping.
	auto options = optionsFor(request);
	options.bounds.bottom = 100;
	auto rendered = _renderer.render(base, request.text, options);
	auto append = [&](const std::string &text, int top, int bottom) {
		options.drawWindow = false;
		options.bounds.top = options.y = top;
		options.bounds.bottom = bottom;
		auto part = _renderer.render(rendered.pages.front(), text, options);
		rendered.pages = std::move(part.pages);
		rendered.diagnostics.insert(rendered.diagnostics.end(),
			part.diagnostics.begin(), part.diagnostics.end());
	};
	append(std::string("Who will\n") + verbs[verb] + "?", 104, 134);
	append("F1 - F" + std::to_string(request.members.size()), 138, 146);
	if (!request.refusal.empty()) {
		XeenTextRenderOptions feedback;
		feedback.bounds = {8, 112, 216, 140};
		feedback.windowBounds = {0, 104, 224, 148};
		feedback.x = 8;
		feedback.y = 112;
		feedback.drawWindow = true;
		feedback.alignment = XeenTextAlignment::Center;
		auto refusal = _renderer.render(rendered.pages.front(), request.refusal, feedback);
		rendered.pages = std::move(refusal.pages);
		rendered.diagnostics.insert(rendered.diagnostics.end(),
			refusal.diagnostics.begin(), refusal.diagnostics.end());
	}
	return rendered;
}

XeenPresentationUpdate XeenEventPresenter::present(const IndexedFrame &base,
		const XeenPresentationRequest &request) {
	if (!base.isValid())
		throw std::invalid_argument("frame base invalido para apresentacao Xeen");
	_request = request;
	_layers.push_back({request, 0});
	_underlay = base;
	_page = 0;
	_diagnostics.clear();
	if (request.kind == XeenPresentationKind::CharacterSelection) {
		const auto rendered = drawSelection(base, request);
		_pages = rendered.pages;
		_diagnostics = rendered.diagnostics;
	} else if (request.kind == XeenPresentationKind::Confirmation) {
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
	if (_request.response == XeenPresentationResponseRequirement::CharacterSelection) {
		if (std::holds_alternative<CancelInteractionAction>(action))
			update.response = CharacterSelectionCancelled{};
		else if (const auto *selection = std::get_if<SelectMemberAction>(&action)) {
			if (selection->partyIndex < _request.members.size())
				update.response = SelectedCharacter{selection->partyIndex};
		}
		// The interpreter revalidates eligibility against the live party.
		if (update.response) update.frame = finishPresentation();
		return update;
	}
	if (_request.response == XeenPresentationResponseRequirement::Presented) {
		if (!isAcknowledge(action))
			return update;
		if (_page + 1 < _pages.size()) {
			_frame = _pages[++_page];
			_layers.back().page = _page;
			update.frame = _frame;
		}
		if (_page + 1 == _pages.size()) {
			update.response = XeenPresentationResponse::Presented;
		}
	} else if (_request.response ==
			XeenPresentationResponseRequirement::Acknowledgment) {
		if (isAcknowledge(action)) {
			update.response = XeenPresentationResponse::Acknowledged;
		}
	} else if (std::holds_alternative<YesAction>(action)) {
		update.response = XeenPresentationResponse::Yes;
	} else if (std::holds_alternative<NoAction>(action)) {
		update.response = XeenPresentationResponse::No;
	}
	if (update.response) update.frame = finishPresentation();
	return update;
}

IndexedFrame XeenEventPresenter::finishPresentation() {
	if (!_active) return _frame; // handle() and respond() may finish the same request.
	_active = false;
	if (_request.kind == XeenPresentationKind::CharacterSelection ||
			_request.kind == XeenPresentationKind::Confirmation ||
			_request.response == XeenPresentationResponseRequirement::YesNo) {
		_layers.pop_back();
		_frame = _underlay;
	}
	return _frame;
}

IndexedFrame XeenEventPresenter::dismissSelection() {
	if (!_layers.empty() && _layers.back().request.kind == XeenPresentationKind::CharacterSelection) {
		return finishPresentation();
	}
	return _frame;
}

void XeenEventPresenter::clear() {
	_layers.clear();
	_pages.clear();
	_active = false;
	_page = 0;
	_diagnostics.clear();
}

IndexedFrame XeenEventPresenter::rebase(const IndexedFrame &base) {
	if (!base.isValid())
		throw std::invalid_argument("invalid Xeen presentation base");
	IndexedFrame current = base;
	for (const auto &layer : _layers) {
		_underlay = current;
		if (layer.request.kind == XeenPresentationKind::CharacterSelection)
			_pages = drawSelection(current, layer.request).pages;
		else if (layer.request.kind == XeenPresentationKind::Confirmation)
			_pages = {drawConfirmation(current)};
		else
			_pages = _renderer.render(current, layer.request.text,
				optionsFor(layer.request)).pages;
		current = _pages.at(layer.page);
	}
	_frame = current;
	return _frame;
}

} // namespace mmodern
