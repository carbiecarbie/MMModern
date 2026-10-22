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
	}
	// A size is meaningful only as a pair.
	if (width && height) { placement.width = *width; placement.height = *height; }
	return placement;
}

std::string formatWindowPlacement(const WindowPlacement &placement) {
	std::ostringstream text;
	text << "# MMModern window settings. Alt+Enter toggles fullscreen.\n"
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
	if (placement.width >= frameWidth && placement.height >= frameHeight &&
			placement.width <= usableWidth && placement.height <= usableHeight)
		return {placement.width, placement.height};
	const int scale = initialWindowScale(usableWidth, usableHeight, frameWidth, frameHeight);
	return {frameWidth * scale, frameHeight * scale};
}

bool fullscreenToggleKey(std::int32_t keycode, std::uint16_t modifiers) {
	return (modifiers & KMOD_ALT) && (keycode == SDLK_RETURN || keycode == SDLK_KP_ENTER);
}

} // namespace mmodern
