#include "XeenEncounterTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "SyntheticXeenArchive.h"
#include "XeenChildProcessTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <fstream>
#include <iostream>

using namespace encounter_test;
namespace fs=std::filesystem;
namespace {
void ppm(const fs::path &path,const IndexedFrame &f) {
	std::ofstream out(path,std::ios::binary);out<<"P6\n"<<f.width<<' '<<f.height<<"\n255\n";
	for(auto p:f.pixels)out.write(reinterpret_cast<const char*>(f.palette.data()+3*p),3);
	check(bool(out),"image output failed");
}
Bytes bytes(const fs::path &path) {std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
struct Harness {
	Fixture domain;
	XeenFontFormat font{gameplay_test::fontBytes()};
	std::unique_ptr<XeenAssetSource> assets;
	XeenMapLoader maps;
	std::unique_ptr<XeenEventLoader> events;
	std::unique_ptr<XeenEventTextLoader> texts;
	XeenEventFlow *flow=nullptr;
	XeenWorld *world=nullptr;
	const XeenPartyState *party=nullptr;
	XeenCamera *camera=nullptr;
	std::uint64_t now=0;
	unsigned initialized=0,validated=0,composed=0,ordinary=0,saveStages=0;
	bool original=false;
	Harness(const std::optional<fs::path> &game) {
		if(!game)return;
		original=true;const auto installation=XeenInstallationDetector().detect(*game);
		check(installation&&installation->hasDarkside(),"World of Xeen installation required");
		assets=std::make_unique<XeenAssetSource>(*installation,320,200);
		font=XeenFontFormat(assets->readArchiveResource("fnt"));
		events=std::make_unique<XeenEventLoader>([&](const std::string &name)->std::optional<Bytes>{
			if(!assets->hasInitialResource(name))return {};return assets->readInitialResource(name);});
		texts=std::make_unique<XeenEventTextLoader>([&](const std::string &name)->std::optional<Bytes>{
			if(!assets->hasArchiveResource(name))return {};return assets->readArchiveResource(name);});
	}
	XeenGameplayServices services() {
		XeenGameplayServices s{
			{{{1,2},{}},[&]{return original?XeenPartyLoader().loadInitialCloudsParty(*assets):domain.p;},
				[&](XeenMapIdentity id){return original?events->load(id):domain.evt;}},
			[&]{return original?XeenGameFlagsLoader().loadInitialCloudsFlags(*assets):XeenGameFlags{};},
			[&](XeenMapIdentity id){if(original)return maps.loadGeometryMap(*assets,id);auto m=domain.terrain;m.geometry.id=id.number;return m;},
			[&](XeenMapIdentity id){if(original)return maps.loadObjects(*assets,id);auto m=domain.objects;m.mapId=id;return m;},
			[&](XeenMapIdentity id){return original?texts->load(id):XeenEventTextFile{};},font,
			[&](XeenWorld &,const XeenPartyState &,const XeenCamera &,std::uint64_t)->XeenEventFlow::Composition{
				++ordinary;throw std::runtime_error("ordinary composition in encounter");}, {},
			[&](XeenEventFlow &f,const XeenCamera &){flow=&f;f.rebuildEncounterPresentation=[&]{if(assets)assets->discardSpriteCache();};}, {},
			[&](XeenWorld &w,XeenEventSystem &,const XeenPartyState &p,XeenCamera &c,const XeenGameFlags &){world=&w;party=&p;camera=&c;}
		};
		s.clock=[&]{return now;};
		s.initializeEncounter=[&](XeenWorld &w,XeenPartyState &p,XeenCamera &c,XeenEncounterState &state){
			++initialized;check(w.sessionState().encounterMarked(),"initialization before save marker");
			return original?XeenActorApproach::initializeFromResources(*assets,w,p,c,state):
				XeenActorApproach::initialize(w,p,c,state,domain.statistics,domain.context,domain.evt);
		};
		s.validateEncounterSprite=[&](std::uint8_t image){++validated;if(assets)assets->validateNormalMonster(image);};
		s.composeEncounter=[&](XeenWorld &w,const XeenPartyState &p,const XeenCamera &c,std::uint64_t ordinaryPhase,std::uint8_t actorFrame){
			++composed;check(w.sessionState().encounterInitialized()&&p.encounterContext,"composition before initialization");
			XeenEventFlow::Composition result;
			if(assets) result.frame=CloudsMapComposer().compose(*assets,w,p,c,{610},nullptr,ordinaryPhase,&result.containsOrdinaryAnimation,actorFrame);
			else {result.frame.width=320;result.frame.height=200;result.frame.pixels.resize(64000);result.frame.pixels[0]=actorFrame;}
			return result;
		};
		s.observeSaveStage=[&](auto){++saveStages;};return s;
	}
};

void traces(const std::optional<fs::path> &game,const fs::path &output) {
	const auto target=output/"unchanged.mmsave";
	{std::ofstream out(target,std::ios::binary);out<<"existing target must not be inspected or replaced";}
	const auto targetBytes=bytes(target);
	for(int trace=0;trace<4;++trace) {
		Harness h(game);auto services=h.services();
		services.show=[&](const IndexedFrame &first,const auto &handle,const auto &escape,const auto &idle,const auto &status){
			check(h.initialized==1&&h.validated==1&&h.composed==1&&h.ordinary==0,"production entry counts");
			const auto initial=*h.party;const auto actors=h.world->sessionState().actors();
			check(h.camera->x==13&&h.camera->y==1&&h.party->encounterContext->minutes==480,"fixed startup");
			if(game){const int hp[]{12,16,12,10,7,5},sp[]{2,0,2,0,7,9};check(actors.size()==27,"original 27 actors");
				for(unsigned i=0;i<6;++i){const auto &c=h.party->party.member(h.party->roster,i);check(c.currentHp==hp[i]&&c.currentSp==sp[i],"original HP/SP");}
				check(actors[5].statistics->image()==8&&actors[5].hp==20,"original image/statistics");
				ppm(output/("initial-"+std::to_string(trace)+".ppm"),first);}
			if(game && trace==0) {
				// A command alone is not evidence of visible production pixels.
				const auto resolver=XeenObjectVisualResolver::load(*h.assets);
				const auto commands=XeenOutdoorScene().build(*h.world,*h.camera,&resolver,nullptr,0,0);
				unsigned prefix=0;
				const unsigned expectedPrefix[]{1757,1757,1757,92,92,92,92,92,8,8};
				for(int end:{94,103,104,105,106,107,108,109,110,131}) {
					std::vector<XeenOutdoorDrawCommand> with,without;
					for(const auto &command:commands)if(command.originalOrder<=end){with.push_back(command);if(!command.actor())without.push_back(command);}
					// Same original background and original ordered stream; only remove the actor command in the control.
					CloudsMapComposer().compose(*h.assets,*h.world,*h.party,*h.camera,{610});
					CloudsMapComposer().drawOutdoorCommands(*h.assets,with);const auto visible=h.assets->snapshot();
					CloudsMapComposer().compose(*h.assets,*h.world,*h.party,*h.camera,{610});
					CloudsMapComposer().drawOutdoorCommands(*h.assets,without);const auto control=h.assets->snapshot();
					unsigned changed=0;for(unsigned i=0;i<visible.pixels.size();++i)changed+=visible.pixels[i]!=control.pixels[i];
					std::cout<<"initial actor pixel contribution through order "<<end<<": "<<changed<<'\n';
					check(changed==expectedPrefix[prefix++],"original ordered occlusion changed");
					if(end==94||end==131)ppm(output/("actor-order-"+std::to_string(end)+".ppm"),visible);
				}
				for(unsigned frame=0;frame<8;++frame) {
					const auto with=CloudsMapComposer().compose(*h.assets,*h.world,*h.party,*h.camera,{610},nullptr,0,nullptr,frame);
					const auto without=CloudsMapComposer().compose(*h.assets,*h.world,*h.party,*h.camera,{610},nullptr,0);
					unsigned changed=0;for(unsigned i=0;i<with.pixels.size();++i)changed+=with.pixels[i]!=without.pixels[i];
					std::cout<<"initial original normal frame "<<frame<<" visible pixel contribution: "<<changed<<'\n';
					const unsigned expected[]{8,6,6,6,8,9,9,9};
					check(changed==expected[frame],"source-faithful initial normal raster changed");
				}
			}
			const auto commands=XeenOutdoorScene::actorCommands(actors,*h.camera,0);
			check(commands.size()==1&&commands[0].originalOrder==94&&commands[0].actor()->identity.recordIndex==5,"original initial visible command");
			const auto refuse=[&]{const auto state=h.flow->encounter()->state();const auto frame=h.flow->frame().pixels;
				handle(SaveGameAction{});check(status().find("unsaveable")!=std::string::npos&&h.saveStages==0&&
					bytes(target)==targetBytes&&h.flow->encounter()->state().revision()==state.revision()&&h.flow->frame().pixels==frame,"F9 bypass");};
			refuse();for(PlayerAction a:std::vector<PlayerAction>{InspectInventoryAction{},InteractionAction{},EquipmentInventoryAction{},
				TransferInventoryAction{},SelectMemberAction{0},SelectInventorySlotAction{0},YesAction{},NoAction{},AcknowledgeAction{}})handle(a);
			check(h.party->encounterContext->minutes==480&&h.flow->encounter()->state().pending()==0,"refused input charged/moved");
			const auto beforeBlocked=h.party->encounterContext;
			handle(NavigationAction::MoveBackward);
			check(h.flow->encounter()->notice().find("Movement blocked by terrain.")!=std::string::npos&&
				h.camera->x==13&&h.camera->y==1&&h.party->encounterContext==beforeBlocked&&
				h.flow->encounter()->state().pending()==0,"production collision feedback/invariants");
			if(game)ppm(output/("collision-"+std::to_string(trace)+".ppm"),h.flow->frame());
			for(unsigned i=1;i<=8;++i){h.now=i*100;idle();check(h.flow->encounter()->frame()==i%8,"frames0..7 wrap");}
			sameActors(actors,h.world->sessionState().actors());
			if(trace==0)handle(WaitAction{});
			if(trace==1)handle(NavigationAction::MoveForward);
			if(trace>=2){handle(NavigationAction::TurnRight);handle(NavigationAction::MoveForward);refuse();
				check(h.flow->encounter()->actionPending()==3&&h.flow->encounter()->state().pending()==2,"real East3/2");
				h.now+=100;idle();check(h.flow->encounter()->state().pending()==1,"real due1");h.now+=100;idle();
				check(h.flow->encounter()->state().pending()==0&&h.world->sessionState().actors()[5].y==1&&h.party->encounterContext->minutes==490,"real due0 approach");
				if(trace==2){handle(NavigationAction::TurnLeft);handle(NavigationAction::TurnLeft);handle(WaitAction{});}
				else handle(NavigationAction::MoveForward);
			}
			const auto phase=h.flow->encounter()->state().phase();
			check(phase==(trace==3?XeenEncounterPhase::SupportStopped:XeenEncounterPhase::Engaged),"real terminal state");
			check(h.party->encounterContext->minutes==(trace==2?500:490),"real final time");
			const auto before=h.flow->frame();const auto live=h.world->sessionState().actors();
			h.world->discardMapCache();if(h.assets)h.assets->discardSpriteCache();h.flow->refresh(true);
			check(h.flow->frame().pixels==before.pixels,"real cache frame mismatch");sameActors(live,h.world->sessionState().actors());
			for(unsigned i=0;i<actors.size();++i)if(i!=5) sameActors({actors[i]},{live[i]});
			sameParty(initial,*h.party);check(!escape(),"terminal intercepted Escape");refuse();
			if(game)ppm(output/("terminal-"+std::to_string(trace)+".ppm"),h.flow->frame());
			std::cout<<"production trace="<<trace<<" P="<<h.camera->x<<','<<h.camera->y<<" A="<<live[5].x<<','<<live[5].y<<" T="<<h.party->encounterContext->minutes<<'\n';
			return true;
		};
		check(Application().playGameplay(services,XeenActorApproach::kEntry,target,false,XeenEncounterEntry::Diagnostic26)==0,"production trace failed");
		check(h.saveStages==0&&bytes(target)==targetBytes,"save changed after exit");
	}
}

void sdl(const std::optional<fs::path> &game) {
	Harness h(game);auto s=h.services();unsigned inputs=0;std::uint64_t cycle=0;
	s.show=[&](const IndexedFrame &first,const SdlWindow::FrameUpdateHandler &handle,const auto &escape,const auto &idle,const auto &status){
		SdlWindow::FrameUpdateHandler wrapped=[&](const PlayerAction &a){++inputs;auto result=handle(a);h.now+=200;return result;};
		wrapped.frameCurrent=handle.frameCurrent;wrapped.closed=handle.closed;wrapped.failed=handle.failed;
		wrapped.beginCycle=[&](std::uint64_t c){cycle=c;handle.beginCycle(c);
			const auto key=[](SDL_Keycode key,Uint8 repeat=0){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=key;e.key.repeat=repeat;check(SDL_PushEvent(&e)==1,"push failed");};
			if(c==1){key(SDLK_RIGHT);key(SDLK_UP,1);key(SDLK_UP);}
			if(c==2||c==3)h.now+=100;
			if(c==4){key(SDLK_LEFT);key(SDLK_LEFT);key(SDLK_PERIOD);}
			if(c==5){key(SDLK_F9);key(SDLK_i);key(SDLK_SPACE);key(SDLK_ESCAPE);key(SDLK_PERIOD);}
			check(c<10,"SDL did not exit");
		};
		return SdlWindow().showInteractive(first,"M26 SDL integration",wrapped,escape,[&]()->std::optional<IndexedFrame>{
			auto f=idle();const auto pending=h.flow->encounter()->state().pending();
			if(cycle==1)check(pending==2,"SDL batch input+idle double pulse");
			if(cycle==2)check(pending==1,"SDL first due");
			if(cycle==3)check(pending==0,"SDL second due");
			if(cycle==4)check(h.flow->encounter()->state().phase()==XeenEncounterPhase::Engaged,"SDL period engagement");
			return f;},status);
	};
	check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic26)==0,"SDL encounter failed");
	check(inputs==8&&cycle==5&&h.saveStages==0,"SDL repeat/late-input/exit count");
}

void startupFailures(const fs::path &out) {
	for(unsigned kind=0;kind<6;++kind) {
		Harness h({});unsigned windows=0;
		if(kind==1||kind==5) {
			const auto root=out/"startup-sprite";fs::create_directories(root);
			std::vector<std::pair<Bytes,Bytes>> frames(8,{sprite_test::cell(20,1,20,1,{3,0,0,1}),{}});
			frames[7].first.pop_back();
			sprite_test::archive(root/"xeen.cc",{{kind==1?"042.mon":"unrelated",sprite_test::multiFrameSprite(frames)}});
			GameInstallation installation{root,root/"xeen.cc",{},GameEdition::CloudsOfXeen};
			h.assets=std::make_unique<XeenAssetSource>(installation,320,200);
		}
		auto s=h.services();
		if(kind==0) h.domain.context.minutes=960;
		if(kind==2) s.composeEncounter=[](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{};};
		if(kind==3) s.configureFlow=[](auto &,const auto &){throw std::runtime_error("startup configuration");};
		if(kind==4) {
			const auto observe=s.observeGameplay;
			s.observeGameplay=[&,observe](auto &w,auto &e,const auto &p,auto &c,const auto &f){
				observe(w,e,p,c,f);auto state=h.flow->encounter()->state();
				XeenActorApproach::stop(w,state,XeenEncounterStop::Domain);
			};
		}
		s.show=[&](const auto &,const auto &,const auto &,const auto &,const auto &){++windows;return true;};
		check(Application().playGameplay(s,XeenActorApproach::kEntry,{},false,XeenEncounterEntry::Diagnostic26)==3&&
			windows==0&&h.ordinary==0,"startup failure exposed window/fallback");
		if(kind==1||kind==5)check(h.validated==1&&h.composed==0,"bad later/missing sprite passed production admission");
	}
}

void sdlFailures(const fs::path &out) {
	const auto target=out/"must-not-exist.mmsave";
	check(!fs::exists(target),"unexpected failure-test save target");
	for(unsigned kind=0;kind<7;++kind) {
		Harness h({});auto s=h.services();
		s.show=[&](const IndexedFrame &,const SdlWindow::FrameUpdateHandler &handle,const auto &escape,const auto &idle,const auto &status){
			handle(NavigationAction::TurnRight);handle(NavigationAction::MoveForward);
			const auto beforeActors=h.world->sessionState().actors();
			const auto beforeContext=h.party->encounterContext;
			std::optional<XeenEncounterState> newer;
			unsigned failures=0,closes=0,cycles=0;
			SdlWindow::FrameUpdateHandler wrapped=[&](const PlayerAction &a){
				auto frame=handle(a);if(kind==0&&frame)frame->width=319;return frame;
			};
			wrapped.frameCurrent=handle.frameCurrent;
			wrapped.failed=[&]{++failures;handle.failed();};
			wrapped.closed=[&]{++closes;handle.closed();};
			wrapped.beginCycle=[&](std::uint64_t cycle){
				++cycles;check(cycle==1,"failed/closed SDL kept running");handle.beginCycle(cycle);
				SDL_Event event{};
				if(kind==4)event.type=SDL_QUIT;
				else {event.type=SDL_KEYDOWN;event.key.keysym.sym=kind==5?SDLK_ESCAPE:kind==0?SDLK_LEFT:SDLK_F9;}
				check(SDL_PushEvent(&event)==1,"SDL failure event push");
			};
			auto first=h.flow->frame();if(kind==6)first.pixels.clear();
			const bool ok=SdlWindow().showInteractive(first,"M26 failure boundary",wrapped,escape,idle,[&]()->std::string {
				if(kind==1)throw std::runtime_error("status failure");
				if(kind==2||kind==3){
					newer=h.flow->encounter()->state();
					// Directly publish through another authorized state value during status.
					auto &party=const_cast<XeenPartyState &>(*h.party);
					XeenActorApproach::action(*h.world,party,*h.camera,*newer,XeenEncounterAction::Left,h.domain.evt);
					if(kind==3)throw std::runtime_error("obsolete status failure");
				}
				return status();
			});
			check(ok==(kind==4||kind==5)&&closes==1&&failures==(ok?0U:1U),"SDL failure/close notification");
			if(newer)check(XeenActorApproach::authoritative(*h.world,*h.party,*h.camera,*newer)&&newer->pending()==2&&
				newer->phase()==XeenEncounterPhase::Exploring,"stale SDL failure stopped newer pending work");
			else if(ok){check(h.flow->encounter()->state().pending()==2,"quit supplied final pulse");
				sameActors(beforeActors,h.world->sessionState().actors());check(h.party->encounterContext==beforeContext,"quit charged time");}
			else check(h.flow->encounter()->state().phase()==XeenEncounterPhase::SupportStopped&&
				h.flow->encounter()->state().pending()==0,"current SDL failure did not stop");
			const auto actors=h.world->sessionState().actors();const auto context=h.party->encounterContext;
			h.now=10000;handle(WaitAction{});idle();handle(SaveGameAction{});
			check(status().find("unsaveable")!=std::string::npos&&context==h.party->encounterContext&&h.saveStages==0,
				"inactive callback advanced/saved");sameActors(actors,h.world->sessionState().actors());
			return ok;
		};
		const auto result=Application().playGameplay(s,XeenActorApproach::kEntry,target,false,XeenEncounterEntry::Diagnostic26);
		check(result==((kind==4||kind==5)?0:4)&&!fs::exists(target),"SDL failure result/target");
	}
	for(const auto &file:fs::directory_iterator(out))check(file.path().filename().u8string().find("must-not-exist")==std::string::npos,
		"encounter created save temporary");
}

void sprites(const fs::path &out) {
	GameInstallation i;i.root=out;i.xeenArchive=out/"xeen.cc";i.edition=GameEdition::CloudsOfXeen;
	std::vector<std::pair<Bytes,Bytes>> frames;
	for(unsigned n=0;n<8;++n)frames.push_back({sprite_test::cell(20,1,20,1,{3,0,0,static_cast<std::uint8_t>(n+1)}),{}});
	const auto valid=sprite_test::multiFrameSprite(frames);
	const auto occluder=sprite_test::sprite(sprite_test::cell(20,1,20,1,{3,0,0,99}));
	sprite_test::archive(i.xeenArchive,{{"042.mon",valid},{"terrain",occluder},{"object",occluder},{"blank",Bytes(64000)}});
	{XeenAssetSource a(i,320,200);a.validateNormalMonster(42);check(a.spriteLoadCount()==1,"normal preflight cache");
		XeenSpriteDrawOptions options;options.sceneClipped=true;
		for(unsigned frame=0;frame<8;++frame){a.drawNormalMonster(42,frame,0,0,options);check(a.snapshot().pixels[20*320+20]==frame+1,"normal frame pixels");}
		a.discardSpriteCache();a.validateNormalMonster(42);check(a.spriteLoadCount()==2,"normal cache reconstruction");}
	{XeenAssetSource a(i,320,200);
		XeenOutdoorDrawCommand actor;actor.originalOrder=94;actor.content=XeenOutdoorActorDraw{{20,5},42,0,3,0,false};
		XeenOutdoorDrawCommand terrain;terrain.originalOrder=105;terrain.content=XeenOutdoorTerrainDraw{"terrain",0,{}};
		XeenObjectVisual visual;visual.identity={20,0};visual.spriteName="object";visual.status=XeenObjectVisualStatus::SupportedStatic;
		XeenOutdoorDrawCommand object;object.originalOrder=110;object.content=XeenOutdoorObjectDraw{visual,0,false};
		CloudsMapComposer composer;
		for(const auto &occluderCommand:{terrain,object}) {
			a.loadRawFramebuffer("blank");composer.drawOutdoorCommands(a,{actor,occluderCommand});
			check(a.snapshot().pixels[20*320+20]==99,"later original command did not obscure actor");
			a.loadRawFramebuffer("blank");composer.drawOutdoorCommands(a,{occluderCommand,actor});
			check(a.snapshot().pixels[20*320+20]==1,"ordered raster control did not reveal actor");
		}
		// One-pixel cell offsets travel through the real decoder and scene/bottom clips.
		for(const auto &[x,y,visible]:std::vector<std::tuple<int,int,bool>>{{7,20,false},{8,20,true},{222,20,true},
			{223,20,false},{20,7,false},{20,8,true},{20,140,true},{20,141,false}}) {
			a.loadRawFramebuffer("blank");actor.x=x-20;actor.y=y-20;composer.drawOutdoorCommands(a,{actor});
			check((a.snapshot().pixels[y*320+x]==1)==visible,"normal scene clip edge");
		}
		actor.x=0;actor.y=120;std::get<XeenOutdoorActorDraw>(actor.content).bottomClipped=true;
		a.loadRawFramebuffer("blank");composer.drawOutdoorCommands(a,{actor});
		check(a.snapshot().pixels[140*320+20]==0,"near bottom clip edge");
	}
	frames[7].first.pop_back();sprite_test::archive(i.xeenArchive,{{"042.mon",sprite_test::multiFrameSprite(frames)}});
	{XeenAssetSource a(i,320,200);rejects([&]{a.validateNormalMonster(42);});}
	{XeenAssetSource a(i,320,200);a.drawSprite("042.mon",0,0,0);rejects([&]{a.validateNormalMonster(42);});}
	fs::remove(i.xeenArchive);
}
}
int main(int argc,char **argv){try{
	check(argc==1||argc==3,"usage: encounter gameplay tests [original-installation output-directory]");
	const std::optional<fs::path> game=argc==3?std::optional<fs::path>{fs::u8path(argv[1])}:std::nullopt;
	const auto output=argc==3?fs::u8path(argv[2]):fs::current_path()/"encounter-gameplay-tests";fs::create_directories(output);
	traces(game,output);sdl(game);if(!game){sprites(output);startupFailures(output);sdlFailures(output);}
	if(game) {
		const auto exe=fs::absolute(fs::u8path(argv[0])).parent_path()/"mmodern.exe";
		const auto result=child_test::launch(exe,{L"--encounter-26",game->wstring()},
			output/("encounter-cli-"+std::to_string(GetCurrentProcessId())+".log"),true,true);
		check(result.exit==0&&result.output.find("Map 20 (Clouds): camera X=13 Y=1 direction=0")!=std::string::npos&&
			result.output.find("\nInventory:")==std::string::npos,"actual encounter CLI startup/modal exclusion/exit");
	}
	std::cout<<(game?"Original-data":"Synthetic")<<" Application/Flow/composer and SDL encounter checks passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
