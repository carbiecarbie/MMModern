#include "games/xeen/XeenEventPresenter.h"

#include <algorithm>
#include <stdexcept>
#include <variant>
#include <chrono>
#include <limits>

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

XeenEventPresenter::XeenEventPresenter(const XeenFontFormat &font, NpcDraw npcDraw,
		Clock clock, RandomFrame randomFrame) :
		_npcDraw(std::move(npcDraw)), _clock(std::move(clock)),
		_randomFrame(std::move(randomFrame)), _renderer(font) {
	if (!_clock) _clock = [] {
		return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
	};
}

XeenTextRenderOptions XeenEventPresenter::optionsFor(
		const XeenPresentationRequest &request) const {
	XeenTextRenderOptions options;
	switch (request.kind) {
	case XeenPresentationKind::RewardWarning:
	case XeenPresentationKind::RewardReceipt:
		options.bounds = {8, 8, 312, 152};
		options.x = 16;
		options.y = 16;
		options.drawWindow = true;
		options.paginate = true;
		break;
	case XeenPresentationKind::NpcAcknowledgment:
		break; // Its heading/body have independent bounded layout below.
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
	_underlay = base;
	_request = request;
	_layers.push_back({request, 0});
	_page = 0;
	_diagnostics.clear();
	if (request.kind == XeenPresentationKind::NpcAcknowledgment) {
		_npcTiming = {};
		layoutNpc(base, request);
		startNpcPage();
	} else if (request.kind == XeenPresentationKind::CharacterSelection) {
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
	_frame = request.kind == XeenPresentationKind::NpcAcknowledgment ? drawNpcPage() : _pages.front();
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

XeenPresentationUpdate XeenEventPresenter::handle(const PlayerAction &action, bool finishResponse) {
	XeenPresentationUpdate update{_frame, std::nullopt, _active};
	if (!_active)
		return update;
	if (_request.kind == XeenPresentationKind::RewardWarning ||
			_request.kind == XeenPresentationKind::RewardReceipt) {
		if (!isAcknowledge(action) && !std::holds_alternative<CancelInteractionAction>(action)) return update;
		if (_page + 1 < _pages.size()) {
			_frame = _pages[++_page];
			_layers.back().page = _page;
			update.frame = _frame;
		} else update.response = XeenPresentationResponse::Acknowledged;
		// Flow validates the generation/kind before removing this layer.
		return update;
	}
	if (_request.kind == XeenPresentationKind::NpcAcknowledgment) {
		if (!isAcknowledge(action) && !std::holds_alternative<CancelInteractionAction>(action))
			return update;
		if (_page + 1 < _pages.size()) {
			++_page;
			_layers.back().page = _page;
			startNpcPage();
			_frame = drawNpcPage();
			update.frame = _frame;
		} else {
			update.response = XeenPresentationResponse::Acknowledged;
			if (finishResponse) update.frame = finishPresentation();
		}
		return update;
	}
	if (_request.response == XeenPresentationResponseRequirement::CharacterSelection) {
		if (std::holds_alternative<CancelInteractionAction>(action))
			update.response = CharacterSelectionCancelled{};
		else if (const auto *selection = std::get_if<SelectMemberAction>(&action)) {
			if (selection->partyIndex < _request.members.size())
				update.response = SelectedCharacter{selection->partyIndex};
		}
		// The interpreter revalidates eligibility against the live party.
		if (update.response && finishResponse) update.frame = finishPresentation();
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
	if (update.response && finishResponse) update.frame = finishPresentation();
	return update;
}

IndexedFrame XeenEventPresenter::finishPresentation() {
	if (!_active) return _frame; // handle() and respond() may finish the same request.
	_active = false;
	if (_request.kind == XeenPresentationKind::NpcAcknowledgment ||
			_request.kind == XeenPresentationKind::RewardWarning ||
			_request.kind == XeenPresentationKind::RewardReceipt ||
			_request.kind == XeenPresentationKind::CharacterSelection ||
			_request.kind == XeenPresentationKind::Confirmation ||
			_request.response == XeenPresentationResponseRequirement::YesNo) {
		_layers.pop_back();
		_frame = _underlay;
	}
	return _frame;
}

IndexedFrame XeenEventPresenter::dismissSelection() {
	if (!_layers.empty() && _layers.back().request.kind == XeenPresentationKind::NpcAcknowledgment)
		return discardNpc();
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
		if (layer.request.kind == XeenPresentationKind::NpcAcknowledgment) {
			layoutNpc(current, layer.request);
			current = drawNpcPage();
			continue;
		} else if (layer.request.kind == XeenPresentationKind::CharacterSelection)
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

void XeenEventPresenter::layoutNpc(const IndexedFrame &base,
		const XeenPresentationRequest &request) {
	if (!request.npc || request.npc->confirmationMode != 1 || !_npcDraw)
		throw std::invalid_argument("NPC presentation requires mode 1 metadata and an asset provider");
	XeenTextRenderOptions panel;
	panel.bounds = {16, 16, 216, 132};
	panel.windowBounds = {8, 8, 224, 140};
	panel.x = panel.y = 16;
	panel.drawWindow = true;
	IndexedFrame heading = _renderer.render(base, "", panel).pages.front();
	int y = 30;
	std::size_t begin = 0;
	std::string styledLine;
	_diagnostics.clear();
	while (begin <= request.title.size()) {
		const auto newline = request.title.find('\n', begin);
		const auto end = newline == std::string::npos ? request.title.size() : newline;
		std::string text;
		int anchor = 141;
		for (auto i = begin; i < end;) {
			if (request.title[i] != '\t') { text += request.title[i++]; continue; }
			++i;
			int offset = 0;
			bool valid = end - i >= 3;
			for (int digit = 0; digit < 3 && i < end; ++digit, ++i) {
				if (request.title[i] < '0' || request.title[i] > '9') valid = false;
				else offset = offset * 10 + request.title[i] - '0';
			}
			if (!valid) _diagnostics.push_back("invalid NPC title horizontal-position control");
			else anchor = std::min(216, 16 + offset);
		}
		XeenTextRenderOptions title;
		title.bounds = {64, 16, 216, 62};
		title.x = 64; title.y = y;
		title.alignment = XeenTextAlignment::Center;
		title.alignmentAnchor = anchor;
		if (y < 62) {
			// The existing clear-text control drops prior glyphs while preserving
			// font/color/alignment state. Reuse it instead of parsing styles twice.
			if (begin) styledLine += '\r';
			styledLine += text;
			const int width = _renderer.textWidth(styledLine, XeenFontSize::Normal);
			if (y + 10 > 62 || anchor - width / 2 < 64 || anchor + (width + 1) / 2 > 216)
				_diagnostics.push_back("NPC title exceeds heading bounds");
			auto rendered = _renderer.render(heading, styledLine, title);
			heading = std::move(rendered.pages.front());
			_diagnostics.insert(_diagnostics.end(), rendered.diagnostics.begin(), rendered.diagnostics.end());
		} else _diagnostics.push_back("NPC title exceeds heading bounds");
		if (newline == std::string::npos) break;
		begin = end + 1;
		// Bounded even for a resource containing thousands of empty title lines.
		y = std::min(62, y + 10);
	}
	XeenTextRenderOptions body;
	body.bounds = {16, 70, 216, 132};
	body.x = 16; body.y = 70;
	body.alignment = XeenTextAlignment::Center;
	body.paginate = true;
	auto rendered = _renderer.render(heading, request.text, body);
	_pages = std::move(rendered.pages);
	_npcPageSourceEnds = std::move(rendered.pageSourceEnds);
	_diagnostics.insert(_diagnostics.end(), rendered.diagnostics.begin(), rendered.diagnostics.end());
}

IndexedFrame XeenEventPresenter::drawNpcPage() const {
	auto frame = _pages.at(_page);
	_npcDraw(frame, _request.npc->portraitId, _npcTiming.displayedFrame);
	return frame;
}

void XeenEventPresenter::startNpcPage() {
	const auto begin = _page ? _npcPageSourceEnds.at(_page - 1) : 0;
	const auto end = _npcPageSourceEnds.at(_page);
	const auto spaces = static_cast<std::uint64_t>(std::count(_request.title.begin(), _request.title.end(), ' ')) +
		static_cast<std::uint64_t>(std::count(_request.text.begin() + begin, _request.text.begin() + end, ' '));
	if (spaces > std::numeric_limits<std::uint64_t>::max() / 2)
		throw std::overflow_error("NPC speech duration overflow");
	_npcTiming.remaining = spaces * 2;
	_npcTiming.deadline = _clock() + 150;
}

std::optional<IndexedFrame> XeenEventPresenter::updateNpc() {
	if (!_active || _request.kind != XeenPresentationKind::NpcAcknowledgment ||
			(!_npcTiming.remaining && !_npcTiming.nextFrame && !_npcTiming.displayedFrame))
		return std::nullopt;
	const auto now = _clock();
	if (now < _npcTiming.deadline) return std::nullopt;
	_npcTiming.deadline = now + 150; // No catch-up loop after a stalled host.
	_npcTiming.displayedFrame = _npcTiming.nextFrame;
	auto frame = drawNpcPage();
	_npcTiming.phase ^= 1;
	unsigned next = _randomFrame ? _randomFrame() : std::uniform_int_distribution<unsigned>(0, 3)(_random);
	if (next >= 4) throw std::invalid_argument("NPC random frame outside 0..3");
	if (!_npcTiming.phase || !_npcTiming.remaining) {
		if (_npcTiming.remaining) --_npcTiming.remaining;
		if (!_npcTiming.remaining) next = 0;
	}
	_npcTiming.nextFrame = next;
	if (frame.pixels == _frame.pixels) return std::nullopt;
	_frame = std::move(frame);
	return _frame;
}

IndexedFrame XeenEventPresenter::discardNpc() {
	if (!_layers.empty() && _layers.back().request.kind == XeenPresentationKind::NpcAcknowledgment) {
		_layers.pop_back();
		_active = false;
		_frame = _underlay;
		_npcTiming = {};
	}
	return _frame;
}

void XeenEventPresenter::discardTransient() noexcept {
	if (_layers.empty()) return;
	const auto &request = _layers.back().request;
	if (request.kind == XeenPresentationKind::RewardWarning || request.kind == XeenPresentationKind::RewardReceipt ||
		request.kind == XeenPresentationKind::NpcAcknowledgment || request.kind == XeenPresentationKind::CharacterSelection ||
		request.kind == XeenPresentationKind::Confirmation || request.response == XeenPresentationResponseRequirement::YesNo) {
		_layers.pop_back();
		_frame = std::move(_underlay);
	}
	_active = false;
	_pages.clear();
	_page = 0;
	_npcTiming = {};
}

} // namespace mmodern
