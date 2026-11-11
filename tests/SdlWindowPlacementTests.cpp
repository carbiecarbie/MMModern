#include "compat/scummvm/ScummVmPictureFilter.h"
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
    // M115: the game controller option, off by default.
    check(!parseWindowPlacement("").controller && parseWindowPlacement("controller=on\n").controller, "controller option");
    check(!parseWindowPlacement("controller=on\ncontroller=yes\n").controller && !parseWindowPlacement("controller=off\n").controller,
        "anything but on turns the controller off");
    const WindowPlacement controller{false, 800, 500, PictureAspect::FourThree, true};
    check(parseWindowPlacement(formatWindowPlacement(controller)) == controller, "controller format round trip");
    check(formatWindowPlacement({}).find("controller=off\n") != std::string::npos, "the default names the option");
    // M116: the picture filter, off by default; unknown values and older files mean off.
    check(parseWindowPlacement("").filter == PictureFilter::Off && parseWindowPlacement("aspect=square\n").filter == PictureFilter::Off,
        "missing filter is off");
    check(parseWindowPlacement("filter=hq2x\nfilter=xbrz\n").filter == PictureFilter::Off &&
        parseWindowPlacement("filter=HQ2X\n").filter == PictureFilter::Off && parseWindowPlacement("filter=\n").filter == PictureFilter::Off,
        "unknown filter is off");
    check(parseWindowPlacement("filter = supereagle2x \r\n").filter == PictureFilter::SuperEagle2x, "filter with spaces and CRLF");
    check(formatWindowPlacement({}).find("filter=off\n") != std::string::npos, "the default names the filter");
    const char *const filterNames[] = {"off", "advmame2x", "advmame3x", "advmame4x", "hq2x", "hq3x", "edge2x", "edge3x",
        "sai2x", "supersai2x", "supereagle2x", "pm2x"};
    auto walked = PictureFilter::Off;
    for (const char *name : filterNames) {
        check(pictureFilterName(walked) == name, std::string("F11 order at ") + name);
        check(parseWindowPlacement(std::string("filter=") + name + "\n").filter == walked, std::string("filter name ") + name);
        WindowPlacement filtered{true, 800, 600, PictureAspect::Square, true};
        filtered.filter = walked;
        check(parseWindowPlacement(formatWindowPlacement(filtered)) == filtered, std::string("filter round trip ") + name);
        walked = nextPictureFilter(walked);
    }
    check(walked == PictureFilter::Off, "F11 wraps to off");
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
	check(pictureFilterKey(SDLK_F11) && !pictureFilterKey(SDLK_F12) && !pictureFilterKey(SDLK_F10) && !pictureFilterKey(SDLK_RETURN),
		"F11 alone is the picture filter key");
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
	check(writeWindowPlacement(file, {false, 800, 500, PictureAspect::FourThree, true}), "seed settings");
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
	check(readWindowPlacement(file) == WindowPlacement{true, 800, 500, PictureAspect::FourThree, true},
		"toggle saved with the windowed size and the controller option kept");
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

// M116: the fixed frame of the Part 0 probe: eight colours in 7x5 blocks,
// diagonal lines and a speckled square.
constexpr std::uint32_t filterPalette[8] = {0xff000000, 0xffffffff, 0xffc02020, 0xff20c020, 0xff2020c0, 0xffe0e000,
	0xff808080, 0xff40a0e0};
int filterIndex(int x, int y) {
	int i = ((x / 7) ^ (y / 5)) & 7;
	if ((x + y) % 23 == 0) i = 1;
	if (x > 100 && x < 140 && y > 60 && y < 100) i = 2 + ((x * x + y) % 3);
	return i;
}
std::vector<std::uint32_t> filterFrameArgb() {
	std::vector<std::uint32_t> frame(64000);
	for (int y = 0; y < 200; ++y) for (int x = 0; x < 320; ++x) frame[y * 320 + x] = filterPalette[filterIndex(x, y)];
	return frame;
}
IndexedFrame filterFrameIndexed() {
	IndexedFrame frame; frame.width = 320; frame.height = 200; frame.pixels.resize(64000);
	for (int i = 0; i < 8; ++i) for (int c = 0; c < 3; ++c) frame.palette[i * 3 + c] = std::uint8_t(filterPalette[i] >> (16 - 8 * c));
	for (int y = 0; y < 200; ++y) for (int x = 0; x < 320; ++x) frame.pixels[y * 320 + x] = std::uint8_t(filterIndex(x, y));
	return frame;
}
std::uint64_t fnv(const std::vector<std::uint32_t> &pixels) {
	std::uint64_t hash = 1469598103934665603ull;
	for (const auto p : pixels) for (int s = 0; s < 32; s += 8) { hash ^= (p >> s) & 0xff; hash *= 1099511628211ull; }
	return hash;
}
struct FilterCase { PictureFilter filter; int factor; std::uint64_t digest; };
// Part 0: the same digests on Windows (UCRT64) and WSL Linux.
const FilterCase filterCases[] = {
	{PictureFilter::AdvMame2x, 2, 0xcc43aeb2a8a96f9bull}, {PictureFilter::AdvMame3x, 3, 0x7e7a4e9cd4e521cbull},
	{PictureFilter::AdvMame4x, 4, 0xbf2eac2d425b0183ull}, {PictureFilter::Hq2x, 2, 0xee4923c8e0697076ull},
	{PictureFilter::Hq3x, 3, 0x952c679c7b270f93ull}, {PictureFilter::Edge2x, 2, 0xacfba73784d3f43full},
	{PictureFilter::Edge3x, 3, 0x7f703bdfbce57133ull}, {PictureFilter::Sai2x, 2, 0xdea98a89e514c97bull},
	{PictureFilter::SuperSai2x, 2, 0xff09bf2398440756ull}, {PictureFilter::SuperEagle2x, 2, 0xfca082ca3dd551b3ull},
	{PictureFilter::Pm2x, 2, 0xa7f0cfb6821ed07eull}};

void filterDigests() {
	const auto frame = filterFrameArgb();
	ScummVmPictureFilter scaler;
	std::vector<std::uint32_t> output{1, 2, 3};
	check(ScummVmPictureFilter::factor("off") == 0 && !scaler.apply("off", frame.data(), 320, 200, output) &&
		!scaler.apply("hq2x", nullptr, 320, 200, output) && output.size() == 3, "off and invalid frames are not filtered");
	for (const auto &c : filterCases) {
		const auto name = pictureFilterName(c.filter);
		check(ScummVmPictureFilter::factor(name) == c.factor, std::string(name) + " factor");
		check(scaler.apply(name, frame.data(), 320, 200, output) && output.size() == std::size_t(64000 * c.factor * c.factor),
			std::string(name) + " output size");
		check(fnv(output) == c.digest, std::string(name) + " digest");
	}
	// A fresh scaler gives the same picture as one that switched filters.
	ScummVmPictureFilter fresh;
	check(fresh.apply("hq2x", frame.data(), 320, 200, output) && fnv(output) == filterCases[3].digest, "fresh scaler digest");
	std::cout << "Picture filter digests passed\n";
}

// Runs the production loop on the software renderer; `inspect` reads the
// picture every second cycle (after a full redraw) and returns false to quit.
void readbackLoop(const std::string &title, const IndexedFrame &frame,
		const std::function<bool(SDL_Window *, SDL_Renderer *, int)> &inspect) {
	int cycles = 0, inspections = 0;
	bool quitting = false;
	const bool ok = SdlWindow().showInteractive(frame, title, {}, {}, [&]() -> std::optional<IndexedFrame> {
		if (quitting || cycles++ % 2) return {};
		SDL_Window *window = nullptr;
		for (Uint32 id = 1; id < 65536 && !window; ++id)
			if (auto *candidate = SDL_GetWindowFromID(id); candidate && title == SDL_GetWindowTitle(candidate)) window = candidate;
		check(window, title + " window");
		auto *renderer = SDL_GetRenderer(window);
		check(renderer && inspections < 20, title + " renderer");
		if (!inspect(window, renderer, inspections++)) {
			quitting = true;
			SDL_Event event{}; event.type = SDL_QUIT;
			check(SDL_PushEvent(&event) == 1, title + " quit");
		}
		return {};
	});
	check(ok, title + " failed");
}
// The renderer's viewport: the whole output for 4:3, the integer picture for square.
std::vector<Uint32> readViewport(SDL_Renderer *renderer, int width, int height) {
	std::vector<Uint32> output(std::size_t(width) * height);
	check(SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, output.data(), width * 4) == 0,
		"software renderer readback unavailable");
	return output;
}
void pushEvent(Uint32 type) { SDL_Event event{}; event.type = type; check(SDL_PushEvent(&event) == 1, "push event"); }
// Frame pixels whose 5x5 neighbourhood is one colour keep it in every filter.
std::vector<std::pair<int, int>> uniformPixels() {
	std::vector<std::pair<int, int>> points;
	for (int y = 2; y < 198; ++y) for (int x = 2; x < 318; ++x) {
		bool uniform = true;
		for (int dy = -2; dy <= 2 && uniform; ++dy) for (int dx = -2; dx <= 2; ++dx)
			if (filterIndex(x + dx, y + dy) != filterIndex(x, y)) { uniform = false; break; }
		if (uniform) points.push_back({x, y});
	}
	check(points.size() > 500, "uniform sample pixels");
	return points;
}
bool near(Uint32 a, Uint32 b, int tolerance) {
	for (int s = 0; s < 24; s += 8) if (std::abs(int((a >> s) & 255) - int((b >> s) & 255)) > tolerance) return false;
	return true;
}

void squareOffReadback() {
	const auto directory = scratchDirectory("square-off");
	const auto file = directory / "window.ini";
	check(writeWindowPlacement(file, {false, 1000, 700, PictureAspect::Square}), "square off settings");
	SdlWindow::rememberPlacement(file);
	const auto frame = filterFrameIndexed();
	const auto argb = filterFrameArgb();
	readbackLoop("Square off readback", frame, [&](SDL_Window *, SDL_Renderer *renderer, int) {
		// Today's square picture: the frame enlarged 3x without smoothing, centred.
		const auto output = readViewport(renderer, 960, 600);
		for (int y = 0; y < 600; ++y) for (int x = 0; x < 960; ++x)
			check((output[y * 960 + x] & 0xffffffu) == (argb[(y / 3) * 320 + x / 3] & 0xffffffu), "square off is nearest 3x");
		return false;
	});
	SdlWindow::rememberPlacement(std::nullopt); removeScratch(directory);
	std::cout << "Square picture without a filter passed\n";
}

void filteredReadback() {
	const auto directory = scratchDirectory("filters");
	const auto file = directory / "window.ini";
	const auto frame = filterFrameIndexed();
	const auto argb = filterFrameArgb();
	const auto uniform = uniformPixels();
	ScummVmPictureFilter scaler;
	for (const auto &c : filterCases) {
		const auto name = std::string(pictureFilterName(c.filter));
		std::vector<std::uint32_t> expected;
		check(scaler.apply(name, argb.data(), 320, 200, expected), name + " expected");
		// A square window of the filter's own size shows its output exactly; 4x is halved.
		const int width = c.factor == 3 ? 960 : 640, height = c.factor == 3 ? 600 : 400, fw = 320 * c.factor;
		WindowPlacement settings{false, width, height, PictureAspect::Square};
		settings.filter = c.filter;
		check(writeWindowPlacement(file, settings), name + " settings");
		SdlWindow::rememberPlacement(file);
		readbackLoop("Square " + name, frame, [&](SDL_Window *window, SDL_Renderer *renderer, int step) {
			int ow = 0, oh = 0;
			check(SDL_GetRendererOutputSize(renderer, &ow, &oh) == 0, name + " output size");
			if (step == 3) {
				// Output size change: a 2x filter in a 3x window is enlarged
				// evenly, then fitted, so uniform areas keep their colour.
				check(ow == 960 && oh == 600, name + " resized output");
				const auto output = readViewport(renderer, 960, 600);
				for (const auto &p : uniform)
					check(near(output[(p.second * 3 + 1) * 960 + p.first * 3 + 1], argb[p.second * 320 + p.first], 2),
						name + " resized uniform colour");
				return false;
			}
			check(ow == width && oh == height, name + " output");
			const auto output = readViewport(renderer, width, height);
			for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
				const auto actual = output[y * width + x] & 0xffffffu;
				if (c.factor < 4) check(actual == (expected[y * fw + x] & 0xffffffu), name + " square picture is the filter output");
				else {
					int sum[3] = {};
					for (int dy = 0; dy < 2; ++dy) for (int dx = 0; dx < 2; ++dx)
						for (int s = 0; s < 3; ++s) sum[s] += (expected[(2 * y + dy) * fw + 2 * x + dx] >> (8 * s)) & 255;
					const Uint32 average = Uint32((sum[2] + 2) / 4) << 16 | Uint32((sum[1] + 2) / 4) << 8 | Uint32((sum[0] + 2) / 4);
					check(near(actual, average, 2), name + " halved square picture");
				}
			}
			// Each renderer reset is followed by the same picture.
			if (step == 0) pushEvent(SDL_RENDER_TARGETS_RESET);
			else if (step == 1) pushEvent(SDL_RENDER_DEVICE_RESET);
			else if (c.factor != 2) return false;
			else SDL_SetWindowSize(window, 960, 600);
			return true;
		});
	}
	// 4:3: the same borders and picture rectangle as without a filter.
	std::vector<Uint32> unfiltered;
	for (const auto filter : {PictureFilter::Off, PictureFilter::Hq2x, PictureFilter::AdvMame3x}) {
		const auto name = std::string(pictureFilterName(filter));
		WindowPlacement settings{false, 1000, 700};
		settings.filter = filter;
		check(writeWindowPlacement(file, settings), name + " 4:3 settings");
		SdlWindow::rememberPlacement(file);
		readbackLoop("4:3 " + name, frame, [&](SDL_Window *, SDL_Renderer *renderer, int step) {
			const auto output = readViewport(renderer, 1000, 700);
			const auto picture = pictureRect(1000, 700, 320, 200, PictureAspect::FourThree);
			for (int y = 0; y < 700; ++y) for (int x = 0; x < 1000; ++x)
				if (x < picture.x || x >= picture.x + picture.width)
					check((output[y * 1000 + x] & 0xffffffu) == 0, name + " black 4:3 borders");
			for (const auto &p : uniform) {
				const int x = picture.x + int((p.first + 0.5) * picture.width / 320), y = int((p.second + 0.5) * picture.height / 200);
				check(near(output[y * 1000 + x], argb[p.second * 320 + p.first], 2), name + " 4:3 uniform colour");
			}
			if (filter == PictureFilter::Off) unfiltered = output;
			else check(output != unfiltered, name + " 4:3 picture is filtered");
			if (step == 0) pushEvent(SDL_RENDER_TARGETS_RESET);
			else if (step == 1) pushEvent(SDL_RENDER_DEVICE_RESET);
			return step < 2;
		});
	}
	SdlWindow::rememberPlacement(std::nullopt); removeScratch(directory);
	std::cout << "Filtered pictures, renderer resets and output size changes passed\n";
}

void filterKey() {
	const auto directory = scratchDirectory("filter-key");
	const auto file = directory / "window.ini";
	check(writeWindowPlacement(file, {false, 800, 600, PictureAspect::FourThree, true}), "filter key settings");
	SdlWindow::rememberPlacement(file);
	WindowRun run;
	run.run("filter key", [&](WindowRun &r) {
		const auto stored = readWindowPlacement(file);
		switch (r.stage) {
		case 0:
			check(SdlWindow::pictureFilterSetting() == PictureFilter::Off, "filter starts off");
			WindowRun::key(SDLK_F11);
			WindowRun::key(SDLK_F11, 0, SDL_KEYDOWN, 1);
			WindowRun::key(SDLK_F11, 0, SDL_KEYUP);
			break;
		case 1:
			check(stored.filter == PictureFilter::AdvMame2x && SdlWindow::pictureFilterSetting() == PictureFilter::AdvMame2x,
				"F11 selects and saves the next filter; its repeat does nothing");
			check(stored.controller && stored.width == 800 && !stored.fullscreen, "F11 keeps the other settings");
			check(r.actions == 0, "F11 reached the game");
			WindowRun::key(SDLK_F11, KMOD_LSHIFT);
			WindowRun::key(SDLK_F11, KMOD_LSHIFT, SDL_KEYUP);
			break;
		case 2:
			check(stored.filter == PictureFilter::AdvMame3x && r.actions == 0, "Shift+F11 selects the next filter");
			for (int i = 0; i < 10; ++i) { WindowRun::key(SDLK_F11); WindowRun::key(SDLK_F11, 0, SDL_KEYUP); }
			break;
		case 3:
			check(stored.filter == PictureFilter::Off && SdlWindow::pictureFilterSetting() == PictureFilter::Off,
				"F11 wraps around to off");
			check(r.actions == 0, "F11 presses reached the game");
			WindowRun::key(SDLK_RETURN);
			WindowRun::key(SDLK_RETURN, 0, SDL_KEYUP);
			break;
		case 4:
			check(r.acknowledgments == 1 && r.actions == 1, "other keys still reach the game");
			WindowRun::quit();
			break;
		}
	});
	// Developer entries read the filter but never save it.
	WindowPlacement developer{false, 800, 600, PictureAspect::Square};
	developer.filter = PictureFilter::Hq2x;
	check(writeWindowPlacement(file, developer), "developer filter settings");
	SdlWindow::rememberPlacement(std::nullopt); SdlWindow::useAspectSetting(file);
	WindowRun developerRun;
	developerRun.run("developer filter", [](WindowRun &r) {
		if (r.stage == 0) {
			check(SdlWindow::pictureFilterSetting() == PictureFilter::Hq2x, "developer reads the filter");
			WindowRun::key(SDLK_F11);
		} else if (r.stage == 1) {
			check(SdlWindow::pictureFilterSetting() == PictureFilter::Hq3x && r.actions == 0, "developer F11 switches");
			WindowRun::quit();
		}
	});
	check(readWindowPlacement(file) == developer, "developer F11 does not write settings");
	SdlWindow::useAspectSetting(std::nullopt);
	check(SdlWindow::pictureFilterSetting() == PictureFilter::Off, "no settings file means no filter");
	removeScratch(directory);
	std::cout << "Picture filter key passed\n";
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
        filterDigests();
        squareOffReadback();
        filteredReadback();
        filterKey();
	} catch (const std::exception &error) {
		std::cerr << "Window placement test failed: " << error.what() << '\n';
		return 1;
	}
	return 0;
}
