#include "platform/sdl/SdlWindow.h"
#include "platform/sdl/SdlWindowPlacement.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
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
    check(parseWindowPlacement("aspect=4:3\n").aspect == PictureAspect::FourThree, "explicit correction");
    check(parseWindowPlacement("aspect=square\n").aspect == PictureAspect::Square, "square option");
    check(parseWindowPlacement("aspect=square\naspect=invalid\n").aspect == PictureAspect::FourThree, "unknown resets default");
    const WindowPlacement square{false, 800, 500, PictureAspect::Square};
    check(parseWindowPlacement(formatWindowPlacement(square)) == square, "square format round trip");
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
	check(size({}, 1920, 1040) == std::pair<int, int>{1248, 936}, "first run size");
	check(size({false, 1000, 700}, 1920, 1040) == std::pair<int, int>{1000, 700}, "remembered size");
	check(size({true, 1000, 700}, 1920, 1040) == std::pair<int, int>{1000, 700}, "remembered size under fullscreen");
	check(size({false, 2000, 700}, 1920, 1040) == std::pair<int, int>{1248, 936}, "remembered size wider than display");
	check(size({false, 1000, 1100}, 1920, 1040) == std::pair<int, int>{1248, 936}, "remembered size taller than display");
	check(size({false, 300, 700}, 1920, 1040) == std::pair<int, int>{1248, 936}, "remembered size below frame");
	std::cout << "Window sizes passed\n";
}

void geometry() {
    struct Case { int w,h,x,y,pw,ph,scale,sx,sy,sw,sh; };
    const Case cases[] = {
        {1920,1080,240,0,1440,1080,6,160,40,1600,1000},
        {2560,1440,320,0,1920,1440,8,160,20,2240,1400},
        {3840,2160,480,0,2880,2160,11,320,80,3200,2000},
        {1366,768,171,0,1024,768,4,203,84,960,600},
        {400,800,0,250,400,300,2,40,300,320,200},
        {800,400,133,0,533,400,2,80,0,640,400},
        {320,240,0,0,320,240,2,0,20,320,200}
    };
    for (const auto &c : cases) {
        const auto r = pictureRect(c.w,c.h,320,200,PictureAspect::FourThree);
        check(r.x==c.x && r.y==c.y && r.width==c.pw && r.height==c.ph, "corrected rectangle");
        check(picturePrescale(r,320,200,0,0)==c.scale, "corrected prescale");
        const auto q = pictureRect(c.w,c.h,320,200,PictureAspect::Square);
        check(q.x==c.sx && q.y==c.sy && q.width==c.sw && q.height==c.sh, "square rectangle");
        check(picturePrescale(q,320,200,0,0)==q.width/320, "square prescale");
        const auto map = [&](int x,int y) { return windowToFrame(x,y,c.w,c.h,c.w,c.h,320,200,PictureAspect::FourThree); };
        check(map(r.x,r.y)==std::pair<int,int>{0,0}, "top left");
        check(map(r.x+r.width-1,r.y+r.height-1)==std::pair<int,int>{319,199}, "bottom right");
        if(r.x>0)check(!map(r.x-1,r.y), "left border");
        if(r.y>0)check(!map(r.x,r.y-1), "top border");
        check(!map(r.x+r.width,r.y), "right edge");
        check(!map(r.x,r.y+r.height), "bottom edge");
    }
    for (const auto aspect : {PictureAspect::FourThree, PictureAspect::Square}) {
        for (const auto sizes : {std::array<int,4>{1920,1080,1920,1080}, {1000,700,1000,700},
                {800,600,1600,1200}, {960,540,1920,1080}, {320,240,640,480}}) {
            for (int y = 0; y < 200; ++y) for (int x = 0; x < 320; ++x) {
                const auto window = frameToWindow(x,y,sizes[0],sizes[1],sizes[2],sizes[3],320,200,aspect);
                check(bool(window), "inverse frame pixel exists");
                check(windowToFrame(window->first,window->second,sizes[0],sizes[1],sizes[2],sizes[3],320,200,aspect) ==
                    std::pair<int,int>{x,y}, "frame/window round trip");
            }
        }
    }
    check(!frameToWindow(-1,0,800,600,800,600,320,200,PictureAspect::FourThree), "invalid frame point");
    check(picturePrescale(pictureRect(3840,2160,320,200,PictureAspect::FourThree),320,200,1920,1200)==6, "texture limit");
    check(windowToFrame(120,0,960,540,1920,1080,320,200,PictureAspect::FourThree)==std::pair<int,int>{0,0}, "high DPI mapping");
    check(!windowToFrame(119,0,960,540,1920,1080,320,200,PictureAspect::FourThree), "high DPI border");
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

// Read back the production presentation on SDL's software/dummy renderer.
void developerAspect() {
    const auto directory=scratchDirectory("developer-aspect");const auto file=directory/"window.ini";
    check(writeWindowPlacement(file,{true,800,600,PictureAspect::Square}),"developer aspect settings");
    SdlWindow::rememberPlacement(std::nullopt);SdlWindow::useAspectSetting(file);
    WindowRun run;
    run.run("developer square",[](WindowRun &r) {
        check(!r.fullscreen() && r.size()==std::pair<int,int>{920,690},"developer uses aspect without stored size or fullscreen");
        WindowRun::quit();
    });
    check(readWindowPlacement(file)==WindowPlacement{true,800,600,PictureAspect::Square},"developer does not write placement");
    SdlWindow::useAspectSetting(std::nullopt);removeScratch(directory);
}

void softwarePresentation() {
    const auto directory = scratchDirectory("pixels");
    const auto file = directory / "window.ini";
    check(writeWindowPlacement(file, {false,1000,700}), "seed pixel test settings");
    SdlWindow::rememberPlacement(file);
    IndexedFrame frame; frame.width=320; frame.height=200; frame.pixels.resize(64000);
    frame.palette[3]=frame.palette[4]=frame.palette[5]=255;
    for(int y=0;y<200;++y) for(int x=0;x<320;++x) frame.pixels[y*320+x]=y%2;
    int inspections=0, cycles=0;
    const bool ok=SdlWindow().showInteractive(frame,"Software picture readback",{}, {}, [&]()->std::optional<IndexedFrame> {
        if (cycles++ % 2) return {}; // Let the reset event be followed by a full redraw.
        SDL_Window *window=nullptr;
        for(Uint32 id=1;id<65536 && !window;++id)
            if(auto *candidate=SDL_GetWindowFromID(id); candidate && std::string(SDL_GetWindowTitle(candidate))=="Software picture readback") window=candidate;
        check(window,"pixel test window");
        auto *renderer=SDL_GetRenderer(window);check(renderer,"pixel test renderer");
        int w=0,h=0;check(SDL_GetRendererOutputSize(renderer,&w,&h)==0 && w==1000 && h==700,"pixel output size");
        std::vector<Uint32> output(w*h);
        check(SDL_RenderReadPixels(renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,output.data(),w*4)==0,"software renderer readback unavailable");
        const auto picture=pictureRect(w,h,320,200,PictureAspect::FourThree);
        const int prescale=picturePrescale(picture,320,200,0,0);
        check(picture.x==33 && picture.width==933 && picture.height==700 && prescale==4,"readback geometry");
        for(int y=0;y<h;++y) {
            check((output[y*w]&0x00ffffffu)==0 && (output[y*w+w-1]&0x00ffffffu)==0,"black picture borders");
            // Linear sample of uniformly four-pixel-tall prescaled source rows.
            const double sample=(y+0.5)*(200.0*prescale)/picture.height-0.5;
            const int i=int(std::floor(sample));const double f=sample-i;
            const auto value=[&](int row){return (std::clamp(row,0,200*prescale-1)/prescale)%2 ? 255.0 : 0.0;};
            const int expected=int(std::round(value(i)*(1-f)+value(i+1)*f));
            const int actual=int((output[y*w+w/2]>>16)&255);
            check(std::abs(actual-expected)<=18,"uniform sharp-bilinear pixel rows");
        }
        SDL_Event event{};
        ++inspections;
        event.type=inspections==1 ? SDL_RENDER_TARGETS_RESET : inspections==2 ? SDL_RENDER_DEVICE_RESET : SDL_QUIT;
        check(SDL_PushEvent(&event)==1,"pixel test reset or quit");return {};
    });
    check(ok && inspections==3,"software picture readback and resets");
    SdlWindow::rememberPlacement(std::nullopt);removeScratch(directory);
    std::cout<<"Software picture borders and uniform pixel rows passed\n";
}

void fullscreenToggle() {
	SdlWindow::rememberPlacement(std::nullopt);
	WindowRun first;
	first.run("toggle", [](WindowRun &run) {
		switch (run.stage) {
		case 0:
			check(!run.fullscreen() && run.size() == std::pair<int, int>{920, 690}, "first corrected window");
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
			check(!run.fullscreen() && run.size() == std::pair<int, int>{920, 690}, "Alt+keypad Enter restores the window");
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
		check(!run.fullscreen() && run.size() == std::pair<int, int>{920, 690}, "oversized settings fall back");
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
        geometry();
		toggleKeys();
		fullscreenToggle();
		persistedPlacement();
        developerAspect();
        softwarePresentation();
	} catch (const std::exception &error) {
		std::cerr << "Window placement test failed: " << error.what() << '\n';
		return 1;
	}
	return 0;
}
