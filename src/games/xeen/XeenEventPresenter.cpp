#include "games/xeen/XeenEventPresenter.h"

#include <algorithm>
#include <stdexcept>
#include <variant>
#include <chrono>
#include <limits>

namespace mmodern {
XeenEventPresenter::XeenEventPresenter(const XeenFontFormat &font, NpcDraw npcDraw,
		Clock clock, RandomFrame randomFrame) :
		_npcDraw(std::move(npcDraw)), _clock(std::move(clock)),
		_randomFrame(std::move(randomFrame)), _font(font), _renderer(font) {
	if (!_clock) _clock = [] {
		return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count());
	};
}

XeenTextRenderOptions XeenEventPresenter::optionsFor(
        const XeenPresentationRequest &request) const {
    XeenTextRenderOptions o;
    o.originalControls=true;
    o.stopAtBottom=true;
    switch(request.kind) {
    case XeenPresentationKind::RewardGeneration: break;
    case XeenPresentationKind::RewardWarning:
    case XeenPresentationKind::RewardReceipt:
        o.bounds={234,8,312,138};o.windowBounds={226,0,320,146};
        o.drawWindow=true;break;
    case XeenPresentationKind::DeferredAudio:
        o.windowBounds={52,149,268,198};o.drawWindow=true;
        o.bounds={60,157,260,190};break;
    case XeenPresentationKind::CharacterSelection:
        o.windowBounds={225,74,320,154};o.drawWindow=true;
        o.bounds={233,82,312,146};break;
    case XeenPresentationKind::CenteredMessage:
        o.windowBounds={225,140,320,199};o.drawWindow=true;
        o.bounds={233,148,312,191};break;
    case XeenPresentationKind::BottomWindowMessage:
    case XeenPresentationKind::BottomWindowTwoLines:
        o.windowBounds={0,143,320,199};o.drawWindow=true;
        o.bounds={8,151,312,191};break;
    case XeenPresentationKind::MainWindowMessage:
    case XeenPresentationKind::NpcAcknowledgment:
        o.windowBounds={8,8,224,140};o.drawWindow=true;
        // Window 11 uses the original eight-pixel inner margin.
        o.bounds={16,16,216,132};break;
    default:o.bounds={0,0,230,149};o.stopAtBottom=false;break;
    }
    o.x=o.bounds.left;o.y=o.bounds.top;
    return o;
}

XeenTextRenderResult XeenEventPresenter::drawText(const IndexedFrame &base,
        const XeenPresentationRequest &request) const {
    if(request.kind==XeenPresentationKind::RewardWarning || request.kind==XeenPresentationKind::RewardReceipt) {
        if(!_dosText || !request.treasure)throw std::invalid_argument("Treasure presentation requires original text and delivery display values");
        XeenTextRenderResult result;
        const auto &treasure=*request.treasure;
        if(request.kind==XeenPresentationKind::RewardWarning || treasure.fullPacks)
            result.pages.push_back(drawXeenTreasure(*_dosText,base,_font,treasure,0,true));
        if(request.kind==XeenPresentationKind::RewardReceipt)
            for(std::size_t page=0;page<=treasure.items.size();++page)
                result.pages.push_back(drawXeenTreasure(*_dosText,base,_font,treasure,page));
        return result;
    }
    auto o=optionsFor(request);
    std::string prefix,suffix;
    switch(request.kind) {
    case XeenPresentationKind::DeferredAudio:
        prefix="\3c\v010\t000";break;
    case XeenPresentationKind::SceneLabelReduced:
        prefix="\2\f08\3c\t116\v025";suffix="\3l\fd\1";break;
    case XeenPresentationKind::SceneLabelNormal:
        prefix="\f04\3c\t116\v030";suffix="\3l\fd";break;
    case XeenPresentationKind::SceneLabelSign:
        prefix="\f08\3c\t120\v088";suffix="\3l\fd";break;
    case XeenPresentationKind::SceneLabelSignReduced:
        prefix="\2\f08\3c\t116\v090";suffix="\3l\fd\1";break;
    case XeenPresentationKind::BottomWindowTwoLines:
        prefix="\r\3c\t000\v007\n\n";break;
    case XeenPresentationKind::CenteredMessage:
    case XeenPresentationKind::BottomWindowMessage:
    case XeenPresentationKind::MainWindowMessage:
        prefix="\r\3c";break;
    default:break;
    }
    XeenTextRenderResult result;
    std::size_t offset=0;
    do {
        const auto text=prefix+request.text.substr(offset)+suffix;
        auto page=_renderer.render(base,text,o);
        const auto consumed=page.pageSourceEnds.back();
        auto next=offset+std::min(request.text.size()-offset,
            consumed>prefix.size()?consumed-prefix.size():0);
        result.pages.insert(result.pages.end(),page.pages.begin(),page.pages.end());
        result.diagnostics.insert(result.diagnostics.end(),page.diagnostics.begin(),page.diagnostics.end());
        result.pageSourceEnds.push_back(next);
        if(!o.originalControls || !o.stopAtBottom || next==request.text.size())break;
        if(next==offset)throw std::runtime_error("Event window writer made no progress");
        o.size=page.writeSize;o.colorIndex=page.writeColor;
        offset=next;
    } while(offset<request.text.size());
    return result;
}

IndexedFrame XeenEventPresenter::overlayNotices(IndexedFrame frame) const {
    // A modal full-pack warning owns window 6 until its acknowledgment.
    // Restore any passive unsupported-audio notice on the next treasure page.
    if(_request.kind==XeenPresentationKind::RewardWarning ||
        (_request.kind==XeenPresentationKind::RewardReceipt && _request.treasure &&
            _request.treasure->fullPacks && _page==0))return frame;
    // The explicit audio deferral stays visible in window 6 above Event pages,
    // without inserting a new wait or changing the Event continuation.
    for(const auto &layer:_layers)if(layer.request.kind==XeenPresentationKind::DeferredAudio)
        frame=drawText(frame,layer.request).pages.front();
    return frame;
}

IndexedFrame XeenEventPresenter::drawConfirmation(const IndexedFrame &base) const {
    auto result=base;
    if (_request.response==XeenPresentationResponseRequirement::YesNo && _spriteDraw) {
        _spriteDraw(result,"global.icn",7,232,74);
        _spriteDraw(result,"confirm.icn",0,235,75);
        _spriteDraw(result,"confirm.icn",2,260,75);
    }
    return result;
}

DialogInput XeenEventPresenter::input() const {
    DialogInput input;
    if(!_active)return input;
    if(_page+1<_pages.size() ||
        _request.response==XeenPresentationResponseRequirement::Acknowledgment ||
        _request.response==XeenPresentationResponseRequirement::Presented) {
        input.anyKey=input.anyClick=input.keyMouseWait=true;
    } else if(_request.response==XeenPresentationResponseRequirement::YesNo) {
        input.keys={'y','n',InputKey::Escape};
        input.hits={{235,75,259,95,'y',InputButton{"confirm.icn",0,235,75}},
            {260,75,284,95,'n',InputButton{"confirm.icn",2,260,75}}};
    }
    return input;
}

bool XeenEventPresenter::hasNpcOffer() const {
	return std::any_of(_layers.begin(),_layers.end(),[](const auto &layer){
		return layer.request.npc && layer.request.npc->confirmationMode==2;
	});
}

XeenTextRenderResult XeenEventPresenter::drawSelection(const IndexedFrame &base,
		const XeenPresentationRequest &request) const {
	XeenTextRenderOptions options;
	options.originalControls=true;options.drawWindow=true;options.stopAtBottom=true;
	options.bounds={233,82,312,146};options.windowBounds={225,74,320,154};
	options.x=233;options.y=82;
	return _renderer.render(base,request.text,options);
}

XeenPresentationUpdate XeenEventPresenter::present(const IndexedFrame &base,
		const XeenPresentationRequest &request) {
	if (!base.isValid())
		throw std::invalid_argument("invalid frame base for Xeen presentation");
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
		const auto rendered = drawText(base, request);
		_pages = rendered.pages;
		_diagnostics = rendered.diagnostics;
	}
	_frame = request.kind == XeenPresentationKind::NpcAcknowledgment ? drawNpcPage() : _pages.front();
	_frame = overlayNotices(std::move(_frame));
	const bool waitsBetweenPages = !(request.npc && request.npc->confirmationMode==2) && _pages.size() > 1 &&
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
		std::holds_alternative<AcknowledgeAction>(action) ||
		std::holds_alternative<CancelInteractionAction>(action);
}

XeenPresentationUpdate XeenEventPresenter::handle(const PlayerAction &action, bool finishResponse) {
    if(const auto *key=std::get_if<DialogKeyAction>(&action)) {
        if(key->key==InputKey::Escape)return handle(CancelInteractionAction{},finishResponse);
        if(_request.response==XeenPresentationResponseRequirement::YesNo) {
            if(key->key=='y')return handle(YesAction{},finishResponse);
            if(key->key=='n')return handle(NoAction{},finishResponse);
        }
    }
	XeenPresentationUpdate update{_frame, std::nullopt, _active};
	if (!_active)
		return update;
	if (_request.kind == XeenPresentationKind::RewardWarning ||
			_request.kind == XeenPresentationKind::RewardReceipt) {
		if (!isAcknowledge(action) && !std::holds_alternative<CancelInteractionAction>(action)) return update;
		if (_page + 1 < _pages.size()) {
			_frame = overlayNotices(_pages[++_page]);
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
			_frame = overlayNotices(_pages[++_page]);
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
	} else if (std::holds_alternative<NoAction>(action) || std::holds_alternative<CancelInteractionAction>(action)) {
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
			current = _pages.at(layer.page);
			_npcDraw(current,layer.request.npc->portraitId,_npcTiming.displayedFrame);
			continue;
		} else if (layer.request.kind == XeenPresentationKind::CharacterSelection)
			_pages = drawSelection(current, layer.request).pages;
		else if (layer.request.kind == XeenPresentationKind::Confirmation)
			_pages = {drawConfirmation(current)};
		else
			_pages = drawText(current, layer.request).pages;
		current = _pages.at(layer.page);
	}
	_frame = overlayNotices(std::move(current));
	return _frame;
}

void XeenEventPresenter::layoutNpc(const IndexedFrame &base,
        const XeenPresentationRequest &request) {
    if(!request.npc || (request.npc->confirmationMode!=1 && request.npc->confirmationMode!=2) || !_npcDraw || !_dosText)
        throw std::invalid_argument("NPC presentation requires supported metadata and original assets");
    _pages.clear();_npcPageSourceEnds.clear();_diagnostics.clear();
    auto o=optionsFor(request);
    std::size_t offset=0;
    do {
        const auto remaining=request.text.substr(offset);
        const auto text=xeenDialogFormat(_dosText->scalar("NPC_DIALOG"),{request.title,remaining});
        // The body is the last substitution in the original DAT template.
        const auto header=text.size()-remaining.size();
        auto rendered=_renderer.render(base,text,o);
        const auto consumed=rendered.pageSourceEnds.back();
        const auto next=offset+std::min(remaining.size(),consumed>header?consumed-header:0);
        _pages.push_back(std::move(rendered.pages.front()));
        _npcPageSourceEnds.push_back(next);
        _diagnostics.insert(_diagnostics.end(),rendered.diagnostics.begin(),rendered.diagnostics.end());
        if(next==request.text.size())break;
        if(next==offset)throw std::runtime_error("NPC window writer made no progress");
        o.size=rendered.writeSize;o.colorIndex=rendered.writeColor;
        offset=next;
    } while(offset<request.text.size());
}

IndexedFrame XeenEventPresenter::drawNpcPage() const {
	auto frame = _pages.at(_page);
	_npcDraw(frame, _request.npc->portraitId, _npcTiming.displayedFrame);
	return overlayNotices(std::move(frame));
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
	if (!_active || (_request.kind != XeenPresentationKind::NpcAcknowledgment && !hasNpcOffer()) ||
			(!_npcTiming.remaining && !_npcTiming.nextFrame && !_npcTiming.displayedFrame))
		return std::nullopt;
	const auto now = _clock();
	if (now < _npcTiming.deadline) return std::nullopt;
	_npcTiming.deadline = now + 150; // No catch-up loop after a stalled host.
	_npcTiming.displayedFrame = _npcTiming.nextFrame;
	auto frame = _frame;
	if(_request.kind==XeenPresentationKind::NpcAcknowledgment)frame=drawNpcPage();
	else for(const auto &layer:_layers)if(layer.request.npc && layer.request.npc->confirmationMode==2)
		_npcDraw(frame,layer.request.npc->portraitId,_npcTiming.displayedFrame);
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
