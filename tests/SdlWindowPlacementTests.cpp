#include "platform/sdl/SdlWindow.h"
#include "platform/sdl/SdlWindowPlacement.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace mmodern;

namespace {

void check(bool value, const std::string &message) {
	if (!value) throw std::runtime_error(message);
}

// Unique per run: a scanner may briefly hold a just-written file, so cleanup
// is best effort and never collides with a later run.
std::filesystem::path scratchDirectory(const char *name) {
	return std::filesystem::temp_directory_path() / ("mmodern-window-placement-" + std::string(name) + '-' +
		std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
}
void removeScratch(const std::filesystem::path &directory) {
	std::error_code ignored;
	std::filesystem::remove_all(directory, ignored);
}

void placementText() {
	check(parseWindowPlacement("") == WindowPlacement{}, "empty text keeps defaults");
	const WindowPlacement stored{true, 1280, 800};
	check(parseWindowPlacement(formatWindowPlacement(stored)) == stored, "format/parse round trip");
	check(parseWindowPlacement(formatWindowPlacement({})) == WindowPlacement{}, "default round trip");
	check(parseWindowPlacement("fullscreen = 1\r\nwidth=960\r\nheight = 600\r\n") == WindowPlacement{true, 960, 600},
		"spaces and CRLF");
	check(parseWindowPlacement("# fullscreen=1\nunknown=4\nfullscreen=0\n") == WindowPlacement{}, "comments and unknown keys");
	// Malformed values keep the defaults; a size needs both extents.
	check(parseWindowPlacement("fullscreen=yes\nwidth=12x\nheight=600\n") == WindowPlacement{}, "malformed values");
	check(parseWindowPlacement("fullscreen=2\nwidth=-5\nheight=0\n") == WindowPlacement{}, "out-of-range values");
	check(parseWindowPlacement("width=99999\nheight=600\n") == WindowPlacement{}, "oversized extent");
	check(parseWindowPlacement("width=960\n") == WindowPlacement{}, "lone width");
	constexpr char noise[] = "garbage\n\0\xff=\nfullscreen=1";
	check(parseWindowPlacement(std::string_view(noise, sizeof noise - 1)) == WindowPlacement{true, 0, 0}, "binary noise");
	std::cout << "Window placement text passed\n";
}

void placementFile() {
	const auto directory = scratchDirectory("test");
	const auto file = directory / "nested" / "window.ini";
	check(readWindowPlacement(file) == WindowPlacement{}, "missing file keeps defaults");
	check(writeWindowPlacement(file, {true, 1600, 1000}), "write creates the directory");
	check(readWindowPlacement(file) == WindowPlacement{true, 1600, 1000}, "file round trip");
	check(writeWindowPlacement(file, {false, 640, 400}), "file replacement");
	check(readWindowPlacement(file) == WindowPlacement{false, 640, 400}, "replaced file");
	check(!std::filesystem::exists(directory / "nested" / "window.ini.tmp"), "temporary file left behind");
	{ std::ofstream corrupt(file, std::ios::binary | std::ios::trunc); corrupt << "\x01\x02 not settings"; }
	check(readWindowPlacement(file) == WindowPlacement{}, "corrupt file keeps defaults");
	// A file in the parent directory's place: failure is reported, not thrown.
	{ std::ofstream blocker(directory / "blocker", std::ios::binary); }
	check(!writeWindowPlacement(directory / "blocker" / "window.ini", {true, 640, 400}), "unwritable target reports failure");
	removeScratch(directory);
	const auto settings = windowPlacementFile();
	check(settings && settings->filename() == "window.ini" && settings->parent_path().filename() == "MMModern" &&
		settings->is_absolute(), "settings live in Local AppData\\MMModern");
	std::cout << "Window placement file passed\n";
}

void windowSizes() {
	// 90% of the usable area: 1080p with a taskbar takes 4x, 1440p 6x, 4K 9x.
	check(initialWindowScale(1920, 1040, 320, 200) == 4, "1080p scale");
	check(initialWindowScale(2560, 1400, 320, 200) == 6, "1440p scale");
	check(initialWindowScale(3840, 2120, 320, 200) == 9, "4K scale");
	check(initialWindowScale(1366, 728, 320, 200) == 3, "small laptop scale");
	check(initialWindowScale(1024, 768, 320, 200) == 2, "XGA scale");
	check(initialWindowScale(320, 200, 320, 200) == 1 && initialWindowScale(0, 0, 320, 200) == 1, "minimum scale");
	const auto size = [](const WindowPlacement &placement, int width, int height) {
		return windowedSize(placement, width, height, 320, 200);
	};
	check(size({}, 1920, 1040) == std::pair<int, int>{1280, 800}, "first run size");
	check(size({false, 1000, 700}, 1920, 1040) == std::pair<int, int>{1000, 700}, "remembered size");
	check(size({true, 1000, 700}, 1920, 1040) == std::pair<int, int>{1000, 700}, "remembered size under fullscreen");
	check(size({false, 2000, 700}, 1920, 1040) == std::pair<int, int>{1280, 800}, "remembered size wider than display");
	check(size({false, 1000, 1100}, 1920, 1040) == std::pair<int, int>{1280, 800}, "remembered size taller than display");
	check(size({false, 300, 700}, 1920, 1040) == std::pair<int, int>{1280, 800}, "remembered size below frame");
	std::cout << "Window sizes passed\n";
}

void toggleKeys() {
	check(fullscreenToggleKey(SDLK_RETURN, KMOD_LALT) && fullscreenToggleKey(SDLK_RETURN, KMOD_RALT) &&
		fullscreenToggleKey(SDLK_KP_ENTER, KMOD_LALT) && fullscreenToggleKey(SDLK_RETURN, KMOD_LALT | KMOD_NUM),
		"Alt+Enter toggles");
	check(!fullscreenToggleKey(SDLK_RETURN, KMOD_NONE) && !fullscreenToggleKey(SDLK_KP_ENTER, KMOD_NUM) &&
		!fullscreenToggleKey(SDLK_RETURN, KMOD_LCTRL) && !fullscreenToggleKey(SDLK_RETURN, KMOD_LSHIFT),
		"Enter without Alt reaches the game");
	check(!fullscreenToggleKey(SDLK_LALT, KMOD_LALT) && !fullscreenToggleKey(SDLK_SPACE, KMOD_LALT) &&
		!fullscreenToggleKey(SDLK_F4, KMOD_LALT), "other Alt combinations are not the toggle");
	std::cout << "Fullscreen toggle keys passed\n";
}

// Real production loop on the dummy video driver (display 1024x768).
struct WindowRun {
	unsigned stage = 0, acknowledgments = 0, actions = 0;
	SDL_Window *window = nullptr;
	static void key(SDL_Keycode code, Uint16 modifiers = 0, Uint32 type = SDL_KEYDOWN, Uint8 repeat = 0) {
		SDL_Event event{};
		event.type = type;
		event.key.state = type == SDL_KEYUP ? SDL_RELEASED : SDL_PRESSED;
		event.key.keysym.sym = code;
		event.key.keysym.scancode = SDL_GetScancodeFromKey(code);
		event.key.keysym.mod = modifiers;
		event.key.repeat = repeat;
		event.key.timestamp = SDL_GetTicks() + 1000;
		check(SDL_PeepEvents(&event, 1, SDL_ADDEVENT, 0, 0) == 1, "queue key");
	}
	static void quit() { SDL_Event event{}; event.type = SDL_QUIT; check(SDL_PushEvent(&event) == 1, "queue quit"); }
	bool fullscreen() const { return SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP; }
	std::pair<int, int> size() const { int w = 0, h = 0; SDL_GetWindowSize(window, &w, &h); return {w, h}; }
	void run(const char *title, const std::function<void(WindowRun &)> &script) {
		IndexedFrame frame;
		frame.width = 320; frame.height = 200; frame.pixels.resize(64000);
		SdlWindow::FrameUpdateHandler handler = [&](const PlayerAction &action) -> std::optional<IndexedFrame> {
			++actions;
			if (std::holds_alternative<AcknowledgeAction>(action)) ++acknowledgments;
			return std::nullopt;
		};
		const bool ok = SdlWindow().showInteractive(frame, title, handler, {}, [&]() -> std::optional<IndexedFrame> {
			if (!window) {
				for (Uint32 id = 1; id < 256 && !window; ++id)
					if (auto *candidate = SDL_GetWindowFromID(id); candidate && std::string(SDL_GetWindowTitle(candidate)) == title)
						window = candidate;
				check(window, "production window not found");
			}
			check(stage < 20, std::string(title) + " timed out");
			script(*this);
			++stage;
			return std::nullopt;
		});
		check(ok, std::string(title) + " failed");
	}
};

void fullscreenToggle() {
	SdlWindow::rememberPlacement(std::nullopt);
	WindowRun first;
	first.run("toggle", [](WindowRun &run) {
		switch (run.stage) {
		case 0:
			check(!run.fullscreen() && run.size() == std::pair<int, int>{640, 400}, "first window: 2x of 1024x768, windowed");
			WindowRun::key(SDLK_RETURN, KMOD_LALT);
			WindowRun::key(SDLK_RETURN, KMOD_LALT, SDL_KEYDOWN, 1);
			WindowRun::key(SDLK_RETURN, KMOD_LALT, SDL_KEYUP);
			break;
		case 1:
			check(run.fullscreen(), "Alt+Enter enters fullscreen");
			check(run.actions == 0, "Alt+Enter or its repeat reached the game");
			WindowRun::key(SDLK_RETURN);
			WindowRun::key(SDLK_RETURN, 0, SDL_KEYUP);
			break;
		case 2:
			check(run.acknowledgments == 1 && run.fullscreen(), "plain Enter reaches the game and keeps the mode");
			WindowRun::key(SDLK_KP_ENTER, KMOD_RALT);
			break;
		case 3:
			check(!run.fullscreen() && run.size() == std::pair<int, int>{640, 400}, "Alt+keypad Enter restores the window");
			check(run.actions == 1, "keypad toggle reached the game");
			SDL_SetWindowSize(run.window, 700, 450);
			break;
		case 4:
			WindowRun::key(SDLK_RETURN, KMOD_LALT);
			break;
		case 5:
			check(run.fullscreen() && run.actions == 1, "fullscreen again before closing");
			WindowRun::quit();
			break;
		}
	});
	// The title-to-game handoff opens a new window: mode and size carry over.
	WindowRun second;
	second.run("handoff", [](WindowRun &run) {
		if (run.stage == 0) {
			check(run.fullscreen(), "fullscreen carried to the next window");
			WindowRun::key(SDLK_RETURN, KMOD_LALT);
		} else if (run.stage == 1) {
			check(!run.fullscreen() && run.size() == std::pair<int, int>{700, 450}, "resized window size carried over");
			WindowRun::quit();
		}
	});
	std::cout << "Fullscreen toggle and handoff passed\n";
}

void persistedPlacement() {
	const auto directory = scratchDirectory("run");
	const auto file = directory / "window.ini";
	check(writeWindowPlacement(file, {false, 800, 500}), "seed settings");
	SdlWindow::rememberPlacement(file);
	WindowRun run;
	run.run("persisted", [](WindowRun &run) {
		if (run.stage == 0) {
			check(!run.fullscreen() && run.size() == std::pair<int, int>{800, 500}, "settings file size used");
			WindowRun::key(SDLK_RETURN, KMOD_LALT);
		} else if (run.stage == 1) {
			check(run.fullscreen(), "persisted toggle");
			WindowRun::quit();
		}
	});
	check(readWindowPlacement(file) == WindowPlacement{true, 800, 500}, "toggle saved with the windowed size");
	// A placement larger than the display falls back to the integer default.
	check(writeWindowPlacement(file, {false, 3000, 2000}), "seed oversized settings");
	SdlWindow::rememberPlacement(file);
	WindowRun oversized;
	oversized.run("oversized", [](WindowRun &run) {
		check(!run.fullscreen() && run.size() == std::pair<int, int>{640, 400}, "oversized settings fall back");
		WindowRun::quit();
	});
	SdlWindow::rememberPlacement(std::nullopt);
	removeScratch(directory);
	std::cout << "Persisted window placement passed\n";
}

} // namespace

int main() {
	try {
		placementText();
		placementFile();
		windowSizes();
		toggleKeys();
		fullscreenToggle();
		persistedPlacement();
	} catch (const std::exception &error) {
		std::cerr << "Window placement test failed: " << error.what() << '\n';
		return 1;
	}
	return 0;
}
