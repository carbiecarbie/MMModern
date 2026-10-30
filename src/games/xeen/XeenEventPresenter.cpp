#include "games/xeen/XeenEventPresenter.h"
#include "core/PresentationClock.h"

#include <algorithm>
#include <stdexcept>
#include <variant>
#include <chrono>
#include <limits>

namespace mmodern {
XeenEventPresenter::XeenEventPresenter(const XeenFontFormat &font, NpcDraw npcDraw,
		Clock, RandomFrame) : _npcDraw(std::move(npcDraw)), _font(font), _renderer(font) {}

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
    case XeenPresentationKind::NothingHere:
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
    const bool label=request.kind==XeenPresentationKind::SceneLabelReduced || request.kind==XeenPresentationKind::SceneLabelNormal ||
        request.kind==XeenPresentationKind::SceneLabelSign || request.kind==XeenPresentationKind::SceneLabelSignReduced;
    if(label && sceneLabelsVisible && !sceneLabelsVisible()) {
        XeenTextRenderResult result;result.pages.push_back(base);return result;
    }
    if(request.kind==XeenPresentationKind::RewardWarning || request.kind==XeenPresentationKind::RewardReceipt) {
        if(!_dosText || !request.treasure)throw std::invalid_argument("Treasure presentation requires original text and delivery display values");
        XeenTextRenderResult result;
        const auto &treasure=*request.treasure;
        if(request.kind==XeenPresentationKind::RewardWarning || treasure.fullPacks)
            result.pages.push_back(drawXeenTreasure(*_dosText,base,_font,treasure,0,true));
        if(request.kind==XeenPresentationKind::RewardReceipt)
            for(std::size_t page=0;page<=(treasure.stage==XeenTreasureDisplay::Stage::Pages?treasure.items.size():0);++page)
                result.pages.push_back(drawXeenTreasure(*_dosText,base,_font,treasure,page));
        return result;
    }
    if(request.kind==XeenPresentationKind::NothingHere) {
        if(!_dosText)throw std::invalid_argument("Event tail requires original DOS text");
        return _renderer.render(base,std::string(_dosText->scalar("NOTHING_HERE")),optionsFor(request));
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

IndexedFrame XeenEventPresenter::drawConfirmation(const IndexedFrame &base, const XeenPresentationRequest &request) const {
    auto result=base;
    if (request.response==XeenPresentationResponseRequirement::YesNo && _spriteDraw) {
        _spriteDraw(result,"global.icn",7,232,74);
        if(request.npcBodyHidden) {
            // DAT 0x259bb..0x25a31 redraws window 40 over the button-panel
            // backing. Preserve its title/portrait above the YesNo buttons.
            for(int y=0;y<100;++y)
                std::copy_n(base.pixels.begin()+y*base.width+226,93,
                    result.pixels.begin()+y*result.width+226);
        }
        const auto y=request.npcBodyHidden?101:75;
        _spriteDraw(result,"confirm.icn",0,235,y);
        _spriteDraw(result,"confirm.icn",2,260,y);
    }
    return result;
}

IndexedFrame XeenEventPresenter::drawWord(const IndexedFrame &base) const {
    if(!_dosText)throw std::invalid_argument("Word input requires original DOS UI text");
    if(_request.kind==XeenPresentationKind::ArenaNumber) {
        // LocationMessage mode 2 leaves window 11 open. NumericInput uses its
        // saved writer position and a centered 200-pixel field (argument 200).
        auto o=optionsFor(XeenPresentationRequest{ });
        o.originalControls=true;o.drawWindow=true;o.stopAtBottom=true;
        o.windowBounds={8,8,224,140};o.bounds={16,16,216,132};o.x=16;o.y=16;
        const auto text=xeenDialogFormat(_dosText->scalar("NPC_DIALOG"),{_request.title,_request.text});
        auto prompt=_renderer.render(base,text,o);
        if(prompt.pages.size()!=1 || !prompt.diagnostics.empty())throw std::runtime_error("Arena numeric prompt overflow");
        auto frame=prompt.pages.front();_npcDraw(frame,27,0);
        o.drawWindow=false;o.x=prompt.writeX;o.y=prompt.writeY;o.size=prompt.writeSize;o.colorIndex=prompt.writeColor;
        return _wordInput.render(frame,_font,"\3l\t000\4" "200\3c",o);
    }
    XeenTextRenderOptions o;o.originalControls=true;o.drawWindow=true;
    // Original window 6: a 200-pixel input field within the dialog's inner margins.
    o.windowBounds={52,149,268,198};o.bounds={60,157,260,190};o.x=60;o.y=157;
    const auto title=_request.word->titleText?_request.title:std::string(_dosText->scalar("WHATS_THE_PASSWORD"));
    const auto prompt=xeenDialogFormat(_dosText->scalar("WORD_INPUT_WINDOW"),{title})+"\3l\t000\4" "200\3c";
    return _wordInput.render(base,_font,prompt,o);
}

DialogInput XeenEventPresenter::input() const {
    DialogInput input;
    if(!_active)return input;
    if(wordInputActive()) {
        input.textEntry=true;input.keys={8,127,InputKey::Escape,InputKey::Enter};
        // Both mouse buttons are swallowed during text entry (DOS coordinate-only input).
        return input;
    }
    if(_page+1<_pages.size() ||
        _request.response==XeenPresentationResponseRequirement::Acknowledgment ||
        _request.response==XeenPresentationResponseRequirement::Presented) {
        input.anyKey=input.anyClick=input.keyMouseWait=true;
    } else if(_request.response==XeenPresentationResponseRequirement::YesNo) {
        input.keys={'y','n',InputKey::Escape};
        const auto y=_request.npcBodyHidden?101:75;
        input.hits={{235,y,259,y+20,'y',InputButton{"confirm.icn",0,235,y}},
            {260,y,284,y+20,'n',InputButton{"confirm.icn",2,260,y}}};
    }
    return input;
}

bool XeenEventPresenter::window11Enabled() const {
    return std::any_of(_layers.begin(),_layers.end(),[](const auto &layer){
        return layer.request.kind==XeenPresentationKind::MainWindowMessage ||
            (layer.request.kind==XeenPresentationKind::NpcAcknowledgment && !layer.request.npcBodyHidden);
    });
}

void XeenEventPresenter::restoreNpcText(const std::string &notice) {
    const auto restore=[&](auto &request) {
        request.npcBodyHidden=false;
        if(request.kind==XeenPresentationKind::NpcAcknowledgment)request.speechNotice=notice;
    };
    restore(_request);
    for(auto &layer:_layers)restore(layer.request);
}

void XeenEventPresenter::closeWindow12() {
    _layers.erase(std::remove_if(_layers.begin(),_layers.end(),[](const auto &layer){
        return layer.request.kind==XeenPresentationKind::BottomWindowMessage || layer.request.kind==XeenPresentationKind::BottomWindowTwoLines;
    }),_layers.end());
}

bool XeenEventPresenter::hasNpcOffer() const {
	return std::any_of(_layers.begin(),_layers.end(),[](const auto &layer){
		return layer.request.npc && layer.request.npc->confirmationMode==2;
	});
}

void XeenEventPresenter::closeNpcOffers() {
    _layers.erase(std::remove_if(_layers.begin(),_layers.end(),[](const auto &layer) {
        return layer.request.npc && layer.request.npc->confirmationMode==2;
    }),_layers.end());
    _npcTiming={};
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
		const XeenPresentationRequest &request, std::size_t retainedNpcPage) {
	if (!base.isValid())
		throw std::invalid_argument("invalid frame base for Xeen presentation");
	_underlay = base;
	_request = request;
	_layers.push_back({request, retainedNpcPage});
	_page = retainedNpcPage;
	_diagnostics.clear();
	if(request.kind==XeenPresentationKind::WordInput || request.kind==XeenPresentationKind::ArenaNumber) {
        _wordInput.clear();_wordInput.begin();_pages={drawWord(base)};
    } else if (request.kind == XeenPresentationKind::NpcAcknowledgment) {
		_npcTiming = {};
		layoutNpc(base, request);
		startNpcPage();
	} else if (request.kind == XeenPresentationKind::CharacterSelection) {
		const auto rendered = drawSelection(base, request);
		_pages = rendered.pages;
		_diagnostics = rendered.diagnostics;
	} else if (request.kind == XeenPresentationKind::Confirmation) {
		_pages = {drawConfirmation(base, request)};
	} else {
		const auto rendered = drawText(base, request);
		_pages = rendered.pages;
		_diagnostics = rendered.diagnostics;
	}
	_frame = request.kind == XeenPresentationKind::NpcAcknowledgment ? drawNpcPage(request, _page) : _pages.front();
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
	// Raw dialog keys include arrows; an already translated outer intent is not a response.
	return std::holds_alternative<DialogKeyAction>(action) || std::holds_alternative<WaitAction>(action) ||
        std::holds_alternative<InteractionAction>(action) ||
		std::holds_alternative<AcknowledgeAction>(action) ||
		std::holds_alternative<CancelInteractionAction>(action);
}

XeenPresentationUpdate XeenEventPresenter::handle(const PlayerAction &action, bool finishResponse) {
    if(wordInputActive()) {
        std::optional<XeenPresentationResponse> response;
        if(std::holds_alternative<CancelInteractionAction>(action))response=XeenWordInput{};
        else if(const auto *typed=std::get_if<TextInputAction>(&action)) {
            if(_request.kind!=XeenPresentationKind::ArenaNumber || std::all_of(typed->text.begin(),typed->text.end(),[](unsigned char c){return c>='0' && c<='9';}))
                _wordInput.type(typed->text,_font,_request.kind==XeenPresentationKind::ArenaNumber?2:30,true);
        }
        else if(const auto *key=std::get_if<DialogKeyAction>(&action)) {
            if(key->key==InputKey::Enter)response=XeenWordInput{_wordInput.value()};
            else if(key->key==InputKey::Escape)response=XeenWordInput{};
            else if(key->key==8)_wordInput.backspace();
            else if(_request.kind==XeenPresentationKind::ArenaNumber && key->key>='0' && key->key<='9')_wordInput.type(std::string(1,char(key->key)),_font,2);
        }
        if(response) {
            if(finishResponse)finishPresentation();
        } else {_wordInput.keyRedraw();_frame=drawWord(_underlay);_pages={_frame};}
        return {_frame,response,true};
    }
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
		const bool choice=_request.response == XeenPresentationResponseRequirement::YesNo;
		if (choice && _page + 1 == _pages.size()) {
			// Only Y, N or Escape answer the last page (LocationMessage::execute).
			if (std::holds_alternative<YesAction>(action)) update.response = XeenPresentationResponse::Yes;
			else if (std::holds_alternative<NoAction>(action) || std::holds_alternative<CancelInteractionAction>(action))
				update.response = XeenPresentationResponse::No;
			if (update.response && finishResponse) update.frame = finishPresentation();
			return update;
		}
		if (!isAcknowledge(action) && !std::holds_alternative<CancelInteractionAction>(action) &&
			!(choice && (std::holds_alternative<YesAction>(action) || std::holds_alternative<NoAction>(action))))
			return update;
		if (_page + 1 < _pages.size()) {
			const auto beforePage=_page;const auto beforeTiming=_npcTiming;
			try {
				++_page;startNpcPage();
				auto frame=drawNpcPage(_request, _page);
				_layers.back().page=_page;_frame=std::move(frame);
			} catch(...) {_page=beforePage;_npcTiming=beforeTiming;throw;}
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
	if (_request.kind == XeenPresentationKind::ArenaNumber || _request.kind == XeenPresentationKind::WordInput || _request.kind == XeenPresentationKind::NpcAcknowledgment ||
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
	// Mode 2 returns with its art alive until Event cleanup (DOS 0x25aa3,
	// 0x291a9). Every following prompt shares that retained location layer.
	if (!_layers.empty() && _layers.back().request.npc &&
			_layers.back().request.npc->confirmationMode == 2)
		return _frame;
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
			current = drawNpcPage(layer.request, layer.page);
			continue;
		} else if(layer.request.kind==XeenPresentationKind::WordInput || layer.request.kind==XeenPresentationKind::ArenaNumber)
            _pages={drawWord(current)};
        else if (layer.request.kind == XeenPresentationKind::CharacterSelection)
			_pages = drawSelection(current, layer.request).pages;
		else if (layer.request.kind == XeenPresentationKind::Confirmation)
			_pages = {drawConfirmation(current, layer.request)};
		else
			_pages = drawText(current, layer.request).pages;
		current = _pages.at(layer.page);
	}
	_frame = overlayNotices(std::move(current));
	return _frame;
}

void XeenEventPresenter::layoutNpc(const IndexedFrame &base,
        const XeenPresentationRequest &request) {
    if(!request.npc || request.npc->confirmationMode>2 || !_npcDraw || !_dosText)
        throw std::invalid_argument("NPC presentation requires supported metadata and original assets");
    _pages.clear();_npcPageSourceEnds.clear();_diagnostics.clear();
    auto o=optionsFor(request);
    if(request.npcBodyHidden) {
        // DAT 0x257b4 selects window 40: table 0x3dc4e/0x3dcae/0x3dd0e/0x3dd6e,
        // 93x100 at (226,0), with an eight-pixel margin (0x3de2e).
        // The title format at 0x53b20 places its first line at inner y+56.
        o.windowBounds={226,0,319,100};o.bounds={234,8,311,92};
        o.x=234;o.y=8;o.drawWindow=true;
        auto text=xeenDialogFormat(_dosText->scalar("NPC_TITLE_ONLY"),{request.title});
        if(text.size()>1)text[1]=1; // 0x25820: normal font first pass.
        for(std::size_t i=0;i+3<text.size();++i)if(text[i]=='\t' && text[i+1]!='0') {
            text[i+1]=text[i+2]=text[i+3]='0';i+=3;
        }
        auto rendered=_renderer.render(base,text,o);
        if(rendered.pageSourceEnds.back()<text.size()) {
            text[1]=2;rendered=_renderer.render(base,text,o);
        }
        _pages.push_back(std::move(rendered.pages.front()));
        _npcPageSourceEnds.push_back(request.text.size());
        _diagnostics=std::move(rendered.diagnostics);return;
    }
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

void XeenEventPresenter::drawNpcPortrait(IndexedFrame &frame,const XeenPresentationRequest &request) const {
	if(request.npcBodyHidden) {
        if(!_spriteDraw)throw std::logic_error("Text-off NPC sprite provider absent");
        const auto id=request.npc->portraitId;
        const auto name="face"+std::string(id<10?"0":"")+std::to_string(id)+".fac";
        _spriteDraw(frame,"frame.fac",0,248,12);
        _spriteDraw(frame,name.c_str(),_npcTiming.displayedFrame,255,18);
    } else _npcDraw(frame, request.npc->portraitId, _npcTiming.displayedFrame);
}
IndexedFrame XeenEventPresenter::drawNpcPage(const XeenPresentationRequest &request, std::size_t page) const {
	auto frame = _pages.at(page);
    drawNpcPortrait(frame,request);
	// Confirmation mode 0 adds the Yes/No buttons after the last page's text.
	if (request.npc->confirmationMode == 0 && page + 1 == _pages.size())
		frame = drawConfirmation(frame, request);
	if(!request.speechNotice.empty()) {
        XeenPresentationRequest notice;notice.kind=XeenPresentationKind::DeferredAudio;notice.text=request.speechNotice;
        frame=drawText(frame,notice).pages.front();
    }
	return overlayNotices(std::move(frame));
}

void XeenEventPresenter::startNpcPage() {
	_npcTiming.displayedFrame=0;
}

std::optional<IndexedFrame> XeenEventPresenter::updateNpc(const NpcTiming &timing) {
	if (!_active || (_request.kind != XeenPresentationKind::NpcAcknowledgment && !hasNpcOffer()))
		return std::nullopt;
	const auto previous=_npcTiming;_npcTiming=timing;
	try {
		auto frame = _frame;
		if(_request.kind==XeenPresentationKind::NpcAcknowledgment)frame=drawNpcPage(_request, _page);
		else for(const auto &layer:_layers)if(layer.request.npc && layer.request.npc->confirmationMode==2)
			drawNpcPortrait(frame,layer.request);
		_frame=std::move(frame);
	} catch(...) {_npcTiming=previous;throw;}
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
	if (request.kind == XeenPresentationKind::ArenaNumber || request.kind == XeenPresentationKind::WordInput || request.kind == XeenPresentationKind::RewardWarning || request.kind == XeenPresentationKind::RewardReceipt ||
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
