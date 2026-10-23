#include "formats/xeen/XeenFontFormat.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenTextRenderer.h"
#include <tuple>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

using namespace mmodern;

namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

std::vector<std::uint8_t> fontBytes() {
	std::vector<std::uint8_t> bytes(XeenFontFormat::kMinimumSize, 0);
	for (std::size_t character = 0; character < 128; ++character) {
		bytes[0x1000 + character] = 6;
		bytes[0x1080 + character] = 3;
	}
	bytes[0x1000 + ' '] = 4;
	bytes[0x1080 + ' '] = 3;
	for (int y = 0; y < 8; ++y) {
		bytes[static_cast<std::size_t>('A') * 16 + y * 2] = 0x01;
		bytes[0x0800 + static_cast<std::size_t>('A') * 16 + y * 2] = 0x02;
		bytes[static_cast<std::size_t>('Y') * 16 + y * 2] = 0x01;
		bytes[static_cast<std::size_t>('N') * 16 + y * 2] = 0x01;
	}
	return bytes;
}

IndexedFrame frame(int width = 320, int height = 200) {
	IndexedFrame result;
	result.width = width;
	result.height = height;
	result.pixels.assign(static_cast<std::size_t>(width) * height, 0x44);
	return result;
}

std::uint8_t pixel(const IndexedFrame &value, int x, int y) {
	return value.pixels[static_cast<std::size_t>(y) * value.width + x];
}

XeenPresentationRequest request(XeenPresentationKind kind,
		XeenPresentationResponseRequirement response, std::string text = "AA") {
	XeenPresentationRequest result;
	result.kind = kind;
	result.response = response;
	result.text = std::move(text);
	return result;
}

void testFontFormatAndRasterization() {
	bool rejected = false;
	try {
		XeenFontFormat invalid(std::vector<std::uint8_t>(100));
	} catch (const std::invalid_argument &) {
		rejected = true;
	}
	check(rejected, "truncated font accepted");
	const XeenFontFormat font(fontBytes());
	const auto normal = font.glyph('A', XeenFontSize::Normal);
	const auto reduced = font.glyph('A', XeenFontSize::Reduced);
	check(normal.advance == 6 && normal.pixels[0] == 1 && normal.pixels[8] == 1,
		"normal glyph decoding failed");
	check(reduced.advance == 3 && reduced.pixels[0] == 2,
		"reduced glyph bank failed");
	check(font.advance(' ', XeenFontSize::Normal) == 4 &&
		font.advance(' ', XeenFontSize::Reduced) == 3, "space metrics failed");

	XeenTextRenderer renderer(font);
	XeenTextRenderOptions options;
	options.bounds = {0, 0, 12, 10};
	options.x = 0;
	options.y = 0;
	const auto rendered = renderer.render(frame(12, 10), "A", options);
	check(pixel(rendered.pages.front(), 0, 0) == 0x19,
		"normal indexed glyph rendering failed");
	options.size = XeenFontSize::Reduced;
	const auto small = renderer.render(frame(12, 10), "A", options);
	check(pixel(small.pages.front(), 0, 0) == 0x19,
		"reduced indexed glyph rendering failed");
	check(renderer.textWidth("AA A", XeenFontSize::Normal) == 22 &&
		renderer.textWidth("AA A", XeenFontSize::Reduced) == 12,
		"glyph advance measurement failed");
}

void testFormattingWrappingAndClipping() {
	const XeenFontFormat font(fontBytes());
	XeenTextRenderer renderer(font);
	XeenTextRenderOptions options;
	options.bounds = {2, 2, 12, 12};
	options.x = -4;
	options.y = -2;
	const auto clipped = renderer.render(frame(14, 14), "AAAA", options);
	check(clipped.pages.size() == 1 && pixel(clipped.pages[0], 0, 0) == 0x44,
		"text escaped clipping bounds");

	options.bounds = {0, 0, 14, 30};
	options.x = 0;
	options.y = 0;
	options.paginate = true;
	const auto wrapped = renderer.render(frame(14, 30), "AA AA", options);
	check(pixel(wrapped.pages[0], 0, 0) == 0x19 &&
		pixel(wrapped.pages[0], 0, 10) == 0x19,
		"metric word wrapping failed");

	std::string formatted;
	formatted.push_back(2);
	formatted += "A";
	formatted.push_back(1);
	formatted.push_back(12);
	formatted += "08A";
	formatted.push_back(3);
	formatted += "cA";
	options.bounds.bottom = 20;
	const auto supported = renderer.render(frame(30, 20), formatted, options);
	check(supported.diagnostics.empty(), "supported controls reported diagnostic");
	std::string unsupported(1, static_cast<char>(0x1f));
	unsupported += "garbage";
	const auto diagnosed = renderer.render(frame(30, 20), unsupported, options);
	check(diagnosed.diagnostics.size() == 1 &&
		std::all_of(diagnosed.pages[0].pixels.begin(), diagnosed.pages[0].pixels.end(),
			[](std::uint8_t value) { return value == 0x44; }),
		"unsupported control was not explicit and non-rendering");
}

void testPresentationComposition() {
    const XeenFontFormat font(fontBytes());
    XeenEventPresenter presenter(font);
    const auto base=frame();
    const auto oracle=[&](const std::string &text,XeenTextRect inner,XeenTextRect outer,bool window) {
        XeenTextRenderOptions options;options.originalControls=true;
        options.bounds=inner;options.windowBounds=outer;options.drawWindow=window;
        options.x=inner.left;options.y=inner.top;options.stopAtBottom=window;
        return XeenTextRenderer(font).render(base,text,options).pages.front();
    };
    for(const auto &spec:std::vector<std::pair<XeenPresentationKind,std::string>>{
        {XeenPresentationKind::SceneLabelReduced,"\2\f08\3c\t116\v025AA\3l\fd\1"},
        {XeenPresentationKind::SceneLabelNormal,"\f04\3c\t116\v030AA\3l\fd"},
        {XeenPresentationKind::SceneLabelSign,"\f08\3c\t120\v088AA\3l\fd"},
        {XeenPresentationKind::SceneLabelSignReduced,"\2\f08\3c\t116\v090AA\3l\fd\1"}}) {
        const auto shown=presenter.present(base,request(spec.first,XeenPresentationResponseRequirement::Presented));
        check(shown.response==XeenPresentationResponse::Presented &&
            shown.frame.pixels==oracle(spec.second,{0,0,230,149},{},false).pixels,"original scene-label stream");
    }
    for(const auto &spec:std::vector<std::tuple<XeenPresentationKind,XeenTextRect,XeenTextRect>>{
        {XeenPresentationKind::CenteredMessage,{233,148,312,191},{225,140,320,199}},
        {XeenPresentationKind::BottomWindowMessage,{8,151,312,191},{0,143,320,199}},
        {XeenPresentationKind::MainWindowMessage,{16,16,216,132},{8,8,224,140}}}) {
        const auto shown=presenter.present(base,request(std::get<0>(spec),XeenPresentationResponseRequirement::Presented));
        check(shown.frame.pixels==oracle("\r\3cAA",std::get<1>(spec),std::get<2>(spec),true).pixels,"original Event window/control stream");
    }
    std::vector<std::tuple<std::string,unsigned,int,int>> sprites;
    presenter.originalAssets(nullptr,[&](auto &,const char *r,unsigned f,int x,int y){sprites.emplace_back(r,f,x,y);});
    presenter.present(base,request(XeenPresentationKind::Confirmation,XeenPresentationResponseRequirement::Acknowledgment,""));
    check(presenter.frame().pixels==base.pixels && sprites.empty(),"invented acknowledgment panel");
    auto generic=presenter.input();
    check(generic.anyKey && generic.anyClick && generic.keyMouseWait &&
        std::holds_alternative<AcknowledgeAction>(*generic.key('z')) &&
        std::holds_alternative<AcknowledgeAction>(*generic.click(0,0)),"generic ordinary-key/either-mouse wait");
    presenter.present(base,request(XeenPresentationKind::Confirmation,XeenPresentationResponseRequirement::YesNo,""));
    check(sprites==std::vector<std::tuple<std::string,unsigned,int,int>>{
        {"global.icn",7,232,74},{"confirm.icn",0,235,75},{"confirm.icn",2,260,75}},"original YesNo sprites");
    const auto input=presenter.input();
    for(const auto &hit:input.hits)for(int x:{hit.left,hit.right-1})for(int y:{hit.top,hit.bottom-1})
        check(std::get<DialogKeyAction>(*input.click(x,y)).key==hit.key,"YesNo rectangle corner");
    check(!input.click(259,75) && !input.click(284,75) && !input.click(235,95) &&
        !input.key('z') && !input.key(InputKey::Enter),"YesNo exclusive edges/unrelated keys");
    check(!input.click(0,0),"Part 0: either mouse button outside YesNo does nothing");
    check(presenter.handle(*input.click(260,75)).response==XeenPresentationResponse::No,"Part 0: coordinate click selects No");
}

void testPagingAndInputProtocol() {
	const XeenFontFormat font(fontBytes());
	XeenEventPresenter presenter(font);
	std::string longText;
	for (int i = 0; i < 40; ++i)
		longText += "AAAA ";
    // The original writer exposes the actual unconsumed source, including
    // whitespace and controls, instead of returning the full source length.
    XeenTextRenderOptions cursor;cursor.originalControls=true;cursor.stopAtBottom=true;
    cursor.bounds={0,0,24,20};cursor.x=cursor.y=0;
    const std::string stream="\f09AA AA AA AA AA AA AA AA";
    const auto written=XeenTextRenderer(font).render(frame(),stream,cursor);
    check(written.pageSourceEnds.size()==1 && written.pageSourceEnds[0]>3 &&
        written.pageSourceEnds[0]<stream.size() && written.writeColor==9,"original source continuation/color state");
    auto continuation=cursor;continuation.colorIndex=written.writeColor;continuation.size=written.writeSize;
    const auto next=XeenTextRenderer(font).render(frame(),stream.substr(written.pageSourceEnds[0]),continuation);
    check(next.pageSourceEnds[0]>0 && next.writeColor==9,"original continuation did not retain writer state");

	const auto first = presenter.present(frame(), request(
		XeenPresentationKind::CenteredMessage,
		XeenPresentationResponseRequirement::Presented, longText));
	check(!first.response && presenter.blocksGameplay(), "multi-page message did not suspend");
	const auto blocked = presenter.handle(NavigationAction::MoveForward);
	check(blocked.consumed && !blocked.response && presenter.blocksGameplay(),
		"navigation was not blocked while paging");
	int acknowledgments = 0;
	while (presenter.blocksGameplay()) {
		const auto update = presenter.handle(InteractionAction{});
		if (update.response) {
			check(*update.response == XeenPresentationResponse::Presented,
				"paging returned wrong 14C response");
			++acknowledgments;
		}
	}
	check(acknowledgments == 1, "paged presentation resumed more than once");
	check(!presenter.handle(NavigationAction::MoveForward).consumed,
		"navigation did not resume after presentation");

	presenter.present(frame(), request(XeenPresentationKind::Confirmation,
		XeenPresentationResponseRequirement::Acknowledgment, ""));
	check(!presenter.handle(NavigationAction::TurnLeft).response,
		"navigation acknowledged a confirmation");
	check(presenter.handle(AcknowledgeAction{}).response ==
		XeenPresentationResponse::Acknowledged &&
		!presenter.handle(AcknowledgeAction{}).response,
		"acknowledgment did not resume exactly once");

	presenter.present(frame(), request(XeenPresentationKind::Confirmation,
		XeenPresentationResponseRequirement::YesNo, ""));
	check(!presenter.handle(InteractionAction{}).response,
		"triggering Space incorrectly answered Yes/No");
	check(presenter.handle(YesAction{}).response == XeenPresentationResponse::Yes,
		"Yes mapping failed");
	presenter.present(frame(), request(XeenPresentationKind::Confirmation,
		XeenPresentationResponseRequirement::YesNo, ""));
	check(presenter.handle(NoAction{}).response == XeenPresentationResponse::No,
		"No mapping failed");
}

} // namespace

int main() {
	try {
		testFontFormatAndRasterization();
		testFormattingWrappingAndClipping();
		testPresentationComposition();
		testPagingAndInputProtocol();
		std::cout << "Xeen event UI presentation, layout, and input protocol OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
