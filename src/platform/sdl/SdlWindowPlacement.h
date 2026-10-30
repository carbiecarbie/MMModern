#ifndef MMODERN_PLATFORM_SDL_WINDOW_PLACEMENT_H
#define MMODERN_PLATFORM_SDL_WINDOW_PLACEMENT_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace mmodern {

// Host window presentation only (never game state): borderless desktop
// fullscreen and the windowed client size, remembered between runs.
enum class PictureAspect { FourThree, Square };

struct WindowPlacement {
	bool fullscreen = false;
	int width = 0, height = 0; // Windowed client size; 0 = choose from the display.
	PictureAspect aspect = PictureAspect::FourThree;
	bool operator==(const WindowPlacement &other) const {
		return fullscreen == other.fullscreen && width == other.width && height == other.height && aspect == other.aspect;
	}
	bool operator!=(const WindowPlacement &other) const { return !(*this == other); }
};

// Unknown keys, malformed lines and out-of-range values keep their defaults.
WindowPlacement parseWindowPlacement(std::string_view text);
std::string formatWindowPlacement(const WindowPlacement &placement);
// A missing or unreadable file yields the defaults.
WindowPlacement readWindowPlacement(const std::filesystem::path &file);
// Creates the parent directory; returns false (never throws) on failure.
bool writeWindowPlacement(const std::filesystem::path &file, const WindowPlacement &placement);
// %LOCALAPPDATA%\MMModern\window.ini, beside the launcher's game-folder.txt.
std::optional<std::filesystem::path> windowPlacementFile();

// Largest integer multiple of the frame within 90% of the usable display area, at least 1.
int initialWindowScale(int usableWidth, int usableHeight, int frameWidth, int frameHeight);
// The remembered size when it still holds the frame and fits the usable display area,
// otherwise the frame at initialWindowScale.
std::pair<int, int> windowedSize(const WindowPlacement &placement, int usableWidth, int usableHeight,
	int frameWidth, int frameHeight);

struct PictureRect { int x, y, width, height; };
PictureRect pictureRect(int outputWidth, int outputHeight, int frameWidth, int frameHeight, PictureAspect aspect);
int picturePrescale(PictureRect picture, int frameWidth, int frameHeight, int maxWidth, int maxHeight);
std::optional<std::pair<int, int>> windowToFrame(int x, int y, int windowWidth, int windowHeight,
    int outputWidth, int outputHeight, int frameWidth, int frameHeight, PictureAspect aspect);

// A window-coordinate sample inside the frame pixel, if one exists.
std::optional<std::pair<int, int>> frameToWindow(int x, int y, int windowWidth, int windowHeight,
    int outputWidth, int outputHeight, int frameWidth, int frameHeight, PictureAspect aspect);

// Alt+Enter (main or keypad Enter) toggles fullscreen. The game never receives it.
bool fullscreenToggleKey(std::int32_t keycode, std::uint16_t modifiers);

} // namespace mmodern

#endif
