#include "platform/sdl/SdlWindowPlacement.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>
#include <system_error>

namespace mmodern {
namespace {

constexpr int maximumWindowExtent = 32768;

std::string_view trim(std::string_view text) {
	while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
	while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) text.remove_suffix(1);
	return text;
}

std::optional<int> number(std::string_view text, int minimum, int maximum) {
	int value = 0;
	const auto end = text.data() + text.size();
	const auto result = std::from_chars(text.data(), end, value);
	if (result.ec != std::errc() || result.ptr != end || value < minimum || value > maximum) return std::nullopt;
	return value;
}

} // namespace

WindowPlacement parseWindowPlacement(std::string_view text) {
	WindowPlacement placement;
	std::optional<int> width, height;
	while (!text.empty()) {
		const auto newline = text.find('\n');
		const auto line = trim(text.substr(0, newline));
		text = newline == std::string_view::npos ? std::string_view{} : text.substr(newline + 1);
		const auto equals = line.find('=');
		if (line.empty() || line.front() == '#' || equals == std::string_view::npos) continue;
		const auto key = trim(line.substr(0, equals)), value = trim(line.substr(equals + 1));
		if (key == "fullscreen") {
			if (const auto flag = number(value, 0, 1)) placement.fullscreen = *flag == 1;
		} else if (key == "width") width = number(value, 1, maximumWindowExtent);
		else if (key == "height") height = number(value, 1, maximumWindowExtent);
        else if (key == "aspect") placement.aspect = value == "square" ? PictureAspect::Square : PictureAspect::FourThree;
	}
	// A size is meaningful only as a pair.
	if (width && height) { placement.width = *width; placement.height = *height; }
	return placement;
}

std::string formatWindowPlacement(const WindowPlacement &placement) {
	std::ostringstream text;
	text << "# MMModern window settings. Alt+Enter toggles fullscreen.\n"
		<< "# Picture shape: aspect=4:3 (default) or aspect=square.\n"
        << "aspect=" << (placement.aspect == PictureAspect::Square ? "square" : "4:3") << '\n'
		<< "fullscreen=" << (placement.fullscreen ? 1 : 0) << '\n';
	if (placement.width > 0 && placement.height > 0)
		text << "width=" << placement.width << "\nheight=" << placement.height << '\n';
	return text.str();
}

WindowPlacement readWindowPlacement(const std::filesystem::path &file) {
	std::ifstream input(file, std::ios::binary);
	if (!input) return {};
	std::ostringstream text;
	text << input.rdbuf();
	return parseWindowPlacement(text.str());
}

bool writeWindowPlacement(const std::filesystem::path &file, const WindowPlacement &placement) {
	try {
		std::error_code error;
		std::filesystem::create_directories(file.parent_path(), error);
		if (error) return false;
		// Replace through a sibling so an interrupted write never leaves a partial file.
		auto temporary = file;
		temporary += ".tmp";
		{
			std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
			output << formatWindowPlacement(placement);
			if (!output.flush()) return false;
		}
		std::filesystem::rename(temporary, file, error);
		if (error) std::filesystem::remove(temporary, error);
		return !error;
	} catch (...) {
		return false;
	}
}

std::optional<std::filesystem::path> windowPlacementFile() {
	PWSTR value = nullptr;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &value))) {
		CoTaskMemFree(value);
		return std::nullopt;
	}
	const std::filesystem::path base(value);
	CoTaskMemFree(value);
	if (base.empty() || !base.is_absolute()) return std::nullopt;
	return base / "MMModern" / "window.ini";
}

int initialWindowScale(int usableWidth, int usableHeight, int frameWidth, int frameHeight) {
	if (frameWidth <= 0 || frameHeight <= 0) return 1;
	const int width = static_cast<int>(static_cast<long long>(std::max(usableWidth, 0)) * 9 / 10);
	const int height = static_cast<int>(static_cast<long long>(std::max(usableHeight, 0)) * 9 / 10);
	return std::max(1, std::min(width / frameWidth, height / frameHeight));
}

std::pair<int, int> windowedSize(const WindowPlacement &placement, int usableWidth, int usableHeight,
		int frameWidth, int frameHeight) {
	const int minHeight = placement.aspect == PictureAspect::FourThree ? frameWidth * 3 / 4 : frameHeight;
	if (placement.width >= frameWidth && placement.height >= minHeight &&
			placement.width <= usableWidth && placement.height <= usableHeight)
		return {placement.width, placement.height};
	if (placement.aspect == PictureAspect::FourThree) {
        const int w = int(static_cast<long long>(std::max(usableWidth, 0)) * 9 / 10);
        const int h = int(static_cast<long long>(std::max(usableHeight, 0)) * 9 / 10);
        const int units = std::max(frameWidth / 4, std::min(w / 4, h / 3));
        return {units * 4, units * 3};
    }
	const int scale = initialWindowScale(usableWidth, usableHeight, frameWidth, frameHeight);
	return {frameWidth * scale, frameHeight * scale};
}

PictureRect pictureRect(int ow, int oh, int fw, int fh, PictureAspect aspect) {
    if (ow <= 0 || oh <= 0 || fw <= 0 || fh <= 0) return {0, 0, 0, 0};
    int w, h;
    if (aspect == PictureAspect::Square) {
        const int scale = std::max(1, std::min(ow / fw, oh / fh));
        w = fw * scale; h = fh * scale;
    } else if (static_cast<long long>(ow) * 3 <= static_cast<long long>(oh) * 4) {
        w = ow; h = int(static_cast<long long>(ow) * 3 / 4);
    } else {
        h = oh; w = int(static_cast<long long>(oh) * 4 / 3);
    }
    return {(ow - w) / 2, (oh - h) / 2, w, h};
}

int picturePrescale(PictureRect picture, int fw, int fh, int maxWidth, int maxHeight) {
    if (fw <= 0 || fh <= 0 || picture.width <= 0 || picture.height <= 0) return 0;
    const int wanted = std::max({1, (picture.width + fw - 1) / fw, (picture.height + fh - 1) / fh});
    const int limit = std::min(maxWidth > 0 ? maxWidth / fw : wanted, maxHeight > 0 ? maxHeight / fh : wanted);
    return std::min(wanted, limit);
}

std::optional<std::pair<int, int>> windowToFrame(int x, int y, int ww, int wh, int ow, int oh,
        int fw, int fh, PictureAspect aspect) {
    if (x < 0 || y < 0 || x >= ww || y >= wh || ww <= 0 || wh <= 0) return std::nullopt;
    const auto r = pictureRect(ow, oh, fw, fh, aspect);
    if (r.width <= 0 || r.height <= 0) return std::nullopt;
    const auto px = static_cast<long long>(x) * ow;
    const auto py = static_cast<long long>(y) * oh;
    if (px < static_cast<long long>(r.x) * ww || px >= static_cast<long long>(r.x + r.width) * ww ||
        py < static_cast<long long>(r.y) * wh || py >= static_cast<long long>(r.y + r.height) * wh) return std::nullopt;
    return std::pair<int, int>{int((px - static_cast<long long>(r.x) * ww) * fw / (static_cast<long long>(r.width) * ww)),
        int((py - static_cast<long long>(r.y) * wh) * fh / (static_cast<long long>(r.height) * wh))};
}

std::optional<std::pair<int, int>> frameToWindow(int x, int y, int ww, int wh, int ow, int oh,
        int fw, int fh, PictureAspect aspect) {
    if (ww <= 0 || wh <= 0 || ow <= 0 || oh <= 0 || fw <= 0 || fh <= 0 ||
        x < 0 || y < 0 || x >= fw || y >= fh) return std::nullopt;
    const auto r = pictureRect(ow, oh, fw, fh, aspect);
    if (r.width <= 0 || r.height <= 0) return std::nullopt;
    const auto cx = (static_cast<long long>(2) * r.x * fw + (2LL * x + 1) * r.width) * ww / (2LL * fw * ow);
    const auto cy = (static_cast<long long>(2) * r.y * fh + (2LL * y + 1) * r.height) * wh / (2LL * fh * oh);
    // Rounding the centre to an integer window coordinate may cross an edge.
    for (int dy = 0; dy <= 1; ++dy) for (int dx = 0; dx <= 1; ++dx) {
        const int wx = int(cx + dx), wy = int(cy + dy);
        if (windowToFrame(wx, wy, ww, wh, ow, oh, fw, fh, aspect) == std::pair<int, int>{x, y})
            return std::pair<int, int>{wx, wy};
    }
    return std::nullopt;
}

bool fullscreenToggleKey(std::int32_t keycode, std::uint16_t modifiers) {
	return (modifiers & KMOD_ALT) && (keycode == SDLK_RETURN || keycode == SDLK_KP_ENTER);
}

} // namespace mmodern
