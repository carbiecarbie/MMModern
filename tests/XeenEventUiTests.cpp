#include "formats/xeen/XeenFontFormat.h"
#include "games/xeen/XeenEventPresenter.h"
#include "games/xeen/XeenTextRenderer.h"

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
	const IndexedFrame base = frame();
	const auto reduced = presenter.present(base, request(
		XeenPresentationKind::SceneLabelReduced,
		XeenPresentationResponseRequirement::Presented));
	check(reduced.response == XeenPresentationResponse::Presented &&
		pixel(reduced.frame, 113, 25) == 0x31,
		"reduced scene label placement failed");
	const auto normal = presenter.present(base, request(
		XeenPresentationKind::SceneLabelNormal,
		XeenPresentationResponseRequirement::Presented));
	check(pixel(normal.frame, 110, 30) == 0x01 &&
		pixel(reduced.frame, 109, 30) == 0x44,
		"normal/reduced scene label distinction failed");
	const auto sign = presenter.present(base, request(
		XeenPresentationKind::SceneLabelSign,
		XeenPresentationResponseRequirement::Presented));
	check(pixel(sign.frame, 114, 88) == 0x31, "sign label placement failed");

	const auto centered = presenter.present(base, request(
		XeenPresentationKind::CenteredMessage,
		XeenPresentationResponseRequirement::Presented));
	check(pixel(centered.frame, 225, 140) == 0xa4,
		"centered message window failed");
	const auto bottom = presenter.present(base, request(
		XeenPresentationKind::BottomWindowMessage,
		XeenPresentationResponseRequirement::Presented));
	check(pixel(bottom.frame, 0, 143) == 0xa4 && pixel(bottom.frame, 225, 140) == 0x44,
		"bottom window composition failed");
	const auto main = presenter.present(base, request(
		XeenPresentationKind::MainWindowMessage,
		XeenPresentationResponseRequirement::Presented));
	check(pixel(main.frame, 8, 8) == 0xa4 && pixel(main.frame, 0, 143) == 0x44,
		"main window semantic distinction failed");
	const auto twoLines = presenter.present(base, request(
		XeenPresentationKind::BottomWindowTwoLines,
		XeenPresentationResponseRequirement::Acknowledgment, "A\nA"));
	check(twoLines.response == std::nullopt && presenter.blocksGameplay(),
		"two-line acknowledgment did not block");
}

void testPagingAndInputProtocol() {
	const XeenFontFormat font(fontBytes());
	XeenEventPresenter presenter(font);
	std::string longText;
	for (int i = 0; i < 40; ++i)
		longText += "AAAA ";
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
