#include "XeenVisualRemoveTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

using namespace mmodern;
using namespace remove_test;
namespace {
void checkpoint(XeenAssetSource &assets, const std::filesystem::path &output, bool cancel, bool sdl) {
	auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
	const auto counts=party.questItems.counts();
	const XeenMapLoader mapLoader;
	XeenWorld world([&](XeenMapIdentity id){return mapLoader.loadGeometryMap(assets,id);},
		[&](XeenMapIdentity id){return mapLoader.loadObjects(assets,id);});
	XeenEventLoader loader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
		if(!assets.hasInitialResource(name))return {};return assets.readInitialResource(name);
	});
	XeenEventTextLoader texts([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
		if(!assets.hasArchiveResource(name))return {};return assets.readArchiveResource(name);
	});
	XeenEventSystem events([&](XeenMapIdentity id){return XeenEventScript(loader.load(id));},
		[&](XeenMapIdentity id){return texts.load(id);});
	XeenCamera camera{20,5,14,XeenDirection::North};
	auto flags=XeenGameFlagsLoader().loadInitialCloudsFlags(assets);const auto flagsBefore=flags.values();
	check(world.selectObject(camera)==XeenObjectIdentity{20,1},"original WhoWill selected object");
	const auto script=loader.load(20);
	check(script.records.size()==16,"original map-20 record count");
	const XeenFontFormat font(assets.readArchiveResource("fnt"));
	const CloudsMapComposer composer;const XeenCharacterRulesContext rules{kCloudsInitialYear};
	XeenEventFlow flow(world,events,party,camera,flags,font,[&]{return composer.compose(assets,world,party,camera,rules);});
	const auto base=flow.frame();
	std::vector<int> lines;
	std::optional<XeenPresentationRequest> choice;
	bool completed=false;
	flow.reportText=[](const std::string &message){throw std::runtime_error(message);};
	flow.reportManual=[&](const auto &r) {
		check(!std::holds_alternative<XeenEventExecutionError>(r),"original WhoWill execution error");
		if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&r)) {
			lines.push_back(s->request.source.line);
			if(s->request.source.line==0) {
				choice=s->request;
				check(s->request.kind==XeenPresentationKind::CharacterSelection && s->request.text=="Bones" &&
					s->request.verbIndex==0 && s->request.textIndex==3 && s->request.source.fileOffset==7,"original WhoWill prompt/source");
			} else if(s->request.source.line==1) {
				check(s->request.kind==XeenPresentationKind::BottomWindowMessage && s->request.textIndex==0 &&
					s->request.text==texts.load(20).strings.at(0),"original next display");
			}
		} else completed=std::holds_alternative<XeenManualEventCompleted>(r);
	};
	std::size_t selected=0;
	while(selected<party.party.size() && !party.party.member(party.roster,selected).canAct())++selected;
	check(selected<party.party.size(),"original party has no eligible member");
	std::size_t step=0;
	auto advance=[&](const PlayerAction &action)->std::optional<IndexedFrame> {
		const auto frame=flow.handle(action);
		if(step==0) {
			check(choice.has_value() && flow.canCancelInteraction() && lines==std::vector<int>{0},"original line-0 dispatch");
			visual_remove_test::save(frame,output/(cancel?"cancel-choice.bmp":"choice.bmp"));
			if(!cancel)for(int verb=0;verb<32;++verb) {
				auto request=*choice;request.verbIndex=static_cast<std::uint8_t>(verb);
				XeenEventPresenter presenter(font);auto rendered=presenter.present(base,request);
				check(!rendered.response && presenter.diagnostics().empty(),"verb rendering diagnostic");
				visual_remove_test::save(rendered.frame,output/("verb-"+std::to_string(verb)+".bmp"));
			}
			if(!cancel) {
				auto request=*choice;
				request.refusal=party.party.member(party.roster,selected).name+" is in no condition to act.";
				XeenEventPresenter presenter(font);
				visual_remove_test::save(presenter.present(base,request).frame,output/"refusal-layout.bmp");
			}
		} else if(cancel) {
			check(completed && !flow.blocksGameplay() && lines==std::vector<int>{0} && frame.pixels==base.pixels,"original cancel ran later presentation");
			visual_remove_test::save(frame,output/"cancel-result.bmp");
		} else {
			check(!completed && flow.blocksGameplay() && !flow.canCancelInteraction() && lines==std::vector<int>({0,1,2}),"valid choice did not reach original display/ack");
			visual_remove_test::save(frame,output/"next-display.bmp");
		}
		check(party.questItems.counts()==counts && world.sessionState().disabledObjectCount()==0 &&
			world.sessionState().disabledEventCount()==0 && flags.values()==flagsBefore &&
			camera.mapId==20 && camera.x==5 && camera.y==14 && camera.direction==XeenDirection::North,"checkpoint changed state before acknowledgment");
		for(std::size_t i=0;i<script.records.size();++i)
			check(sameRecord(script.records[i],world.effectiveEvent({20,i},script.records[i])),"checkpoint removed event");
		++step;return frame;
	};
	if(sdl) {
		std::atomic<bool> finished{false};std::exception_ptr senderError;
		std::thread sender([&] {
			try {
				auto push=[&](SDL_Event event) {
					for(int i=0;i<200 && !finished;++i) {
						std::this_thread::sleep_for(std::chrono::milliseconds(10));
						if(SDL_PushEvent(&event)==1)return;
					}
					if(!finished)throw std::runtime_error("SDL checkpoint queue unavailable");
				};
				for(auto key:std::vector<SDL_Keycode>{SDLK_SPACE,cancel?SDLK_ESCAPE:static_cast<SDL_Keycode>(SDLK_F1+selected)}) {
					SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=key;push(event);
					event.key.repeat=1;push(event);event.type=SDL_KEYUP;event.key.repeat=0;push(event);
				}
				SDL_Event quit{};quit.type=SDL_QUIT;push(quit);
			}catch(...){senderError=std::current_exception();SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);}
		});
		const bool ok=SdlWindow().showInteractive(base,"MMModern 18A original checkpoint",advance,[&]{return flow.canCancelInteraction();});
		finished=true;sender.join();if(senderError)std::rethrow_exception(senderError);
		check(ok,"original SDL checkpoint failed");
	} else {
		advance(InteractionAction{});
		advance(cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{selected}});
	}
	check(step==2,"original SDL repeated/lost input");
	std::cout<<(cancel?"Cancellation":"Selection")<<" passed: map 20 (5,14) North, selected active index "<<selected
		<<", roster ID "<<+party.party.activeRosterIds()[selected]<<", "<<(sdl?"SDL":"direct")<<"; no grant/Remove certification (18B).\n";
}
}
int main(int argc,char **argv) {
	try {
		check(argc==3 || (argc==4 && std::string(argv[3])=="sdl"),"usage: mmodern_who_will_smoke <game-directory> <output-directory> [sdl]");
		std::filesystem::create_directories(argv[2]);
		const auto installation=XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(),"Clouds installation unavailable");
		XeenAssetSource assets(*installation,320,200);
		checkpoint(assets,argv[2],false,argc==4);checkpoint(assets,argv[2],true,argc==4);
		return 0;
	}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
