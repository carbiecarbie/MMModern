// Test-only SDL input adapter around the real CLI and production construction.
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "platform/XeenSaveFile.h"
#include "XeenRestoreReplayProbe.h"
#include "games/xeen/XeenStateEquality.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <deque>
using namespace mmodern;
namespace fs=std::filesystem;
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjES5_ItE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__wrap_" PLAY_SYMBOL);
namespace {
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
void press(SDL_Keycode code) {
	for (auto type:{SDL_KEYDOWN,SDL_KEYUP}) { SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.keysym.scancode=SDL_GetScancodeFromKey(code);e.key.timestamp=SDL_GetTicks()+1;check(SDL_PushEvent(&e)==1,"SDL input enqueue"); }
}
std::vector<std::uint8_t> disk(const fs::path &path) { std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}}; }
void frameEvidence(const IndexedFrame &frame) {
 const char *path=std::getenv("MMODERN_REGION_FRAME");if(!path)return;
 std::vector<std::uint32_t> pixels;pixels.reserve(frame.pixels.size());
 for(auto index:frame.pixels){const auto p=index*3;pixels.push_back(0xff000000u|(frame.palette[p]<<16)|(frame.palette[p+1]<<8)|frame.palette[p+2]);}
 SDL_Surface *surface=SDL_CreateRGBSurfaceFrom(pixels.data(),frame.width,frame.height,32,frame.width*4,0xff0000,0xff00,0xff,0xff000000);
 check(surface,"Evidence surface");const auto result=SDL_SaveBMP(surface,path);SDL_FreeSurface(surface);check(result==0,"Evidence bitmap");
}
}
extern "C" int wrappedPlay(const Application *app,const XeenGameplayServices &original,XeenCamera camera,
	const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,std::optional<std::uint32_t> seed,std::optional<std::uint16_t> contract) {
	try {
		const char *routeValue=std::getenv("MMODERN_REGION_ROUTE");check(routeValue,"Missing regional witness route");
		std::string route=routeValue;
		if(route=="-")route.clear();
		XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;const XeenCamera *position=nullptr;const XeenGameFlags *flags=nullptr;
		auto services=original;std::uint64_t now=0;unsigned automatic=0,saves=0;
		services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto appearance) {
			try {return original.composeEncounter(w,p,c,phase,appearance);}
			catch (const std::exception &e) {std::cerr << "Regional composition: " << e.what() << '\n';throw;}
		};
		const auto configure=services.configureFlow;
		services.configureFlow=[&](auto &f,const auto &c) {
			if (configure) configure(f,c);flow=&f;
			const auto report=f.reportAutomatic;
			f.reportAutomatic=[&,report](const auto &result){if (std::holds_alternative<XeenAutomaticEventCompleted>(result)) ++automatic;if(report)report(result);};
		};
		services.observeGameplay=[&](auto &w,auto &,const auto &p,auto &c,const auto &f){world=&w;party=&p;position=&c;flags=&f;};
		services.clock=[&]{return now;};
		services.observeSaveStage=[&](auto){++saves;};
		const auto before=resume ? disk(*target) : std::vector<std::uint8_t>{};
		if (resume) ++replay_test::depth;
		services.show=[&](const auto &first,const auto &handler,const auto &escape,const auto &idle,const auto &status) {
			unsigned stage=0,loops=0,stable=0;std::size_t index=0;bool done=false;
			std::vector<std::uint8_t> expected;
			std::deque<SDL_Keycode> modal;
			std::optional<XeenItem> equipmentBefore;
			const auto capture=[&]{return XeenSaveFormat::encode(XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world));};
			const auto drive=[&] {
				if (done) return;
				if (stage==0) {
					check(flow->encounter()->journeyQuiet(),"Startup quiet boundary");
					if (resume) {--replay_test::depth;check(!replay_test::unexpected,"Startup replay");check(capture()==before,"Complete startup state differs from saved bytes");}
					std::cout << "STARTUP\n" << flow->encounter()->journeyInspection();stage=1;
				}
				if (stage==1) {
					if (!modal.empty()) {press(modal.front());modal.pop_front();return;}
					if (flow->encounter()->state().phase()==XeenEncounterPhase::SupportStopped) {
						check(!flow->canSave(),"Stopped capture admission");expected=target && fs::exists(*target) ? disk(*target) : std::vector<std::uint8_t>{};
						press(SDLK_F9);stage=4;return;
					}
					if (!flow->encounter()->journeyQuiet()) return;
					if (index<route.size()) {
						const char c=route[index++];
						if (c=='M') {equipmentBefore=party->roster.at(0).weapons[0];modal={SDLK_i,SDLK_F1,SDLK_1,SDLK_e,SDLK_i};return;}
						press(c=='F' ? SDLK_UP : c=='L' ? SDLK_LEFT : c=='R' ? SDLK_RIGHT : c=='W' ? SDLK_PERIOD : c=='I' ? SDLK_i : SDLK_SPACE);return;
					}
					if (equipmentBefore) {
						const auto &after=party->roster.at(0).weapons[0];
						check(after.frame!=equipmentBefore->frame && after.id==equipmentBefore->id && after.material==equipmentBefore->material && after.state==equipmentBefore->state,"Restored owner equipment mutation");
					}
					check(!flow->encounter()->combat(),"No regional combat owner");
					std::cout << "FINAL\n" << flow->encounter()->journeyInspection();
					if (route=="LFFFRFFFFRF") check(automatic==1 && position->x==5 && position->y==9 && party->encounterContext->minutes==560,"Actual automatic sign witness");
					if (target) { expected=capture();press(SDLK_F9);stage=2;return; }
					stage=3;
				}
				if (stage==2) { check(saves==3 && disk(*target)==expected,"Exact real F9 bytes");stage=3; }
				if (stage==4) {
					check(saves==0,"Stopped F9 called save providers");
					if (!expected.empty()) check(disk(*target)==expected,"Stop changed disk save");
					const auto context=*party->encounterContext;const std::vector<XeenActor> actors=world->sessionState().actors();
					handler.withDisplayedInput(WaitAction{},*handler.displayedInput());
					check(*party->encounterContext==context && world->sessionState().actors().size()==actors.size(),"Stopped input advanced context");
					for(unsigned i=0;i<actors.size();++i)check(xeen_state::sameActor(actors[i],world->sessionState().actors()[i]),"Stopped input changed actor");stage=3;
				}
				if (stage==3) {frameEvidence(flow->frame());done=true;SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Quit enqueue");}
			};
			const auto timedIdle=[&] {
				check(++loops<1000,"Bounded regional SDL witness");
				if (flow->encounter()->state().pending()) now+=100;
				auto frame=idle();if(frame)stable=0;else if(++stable==2){stable=0;drive();}return frame;
			};
			const bool result=original.show(first,handler,escape,timedIdle,status);
			check(result && done,"SDL witness did not finish");
			std::cout << "REGIONAL SDL PASS route=" << route << " resume=" << resume << " automatic=" << automatic << " saveStages=" << saves << '\n';return result;
		};
		return realPlay(app,services,camera,target,resume,entry,seed,contract);
	} catch (const std::exception &e) {std::cerr << "Regional witness: " << e.what() << '\n';return 8;}
}
