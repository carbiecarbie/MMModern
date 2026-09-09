#include "XeenRemoveTestSupport.h"
#include "XeenVisualRemoveTestSupport.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include <array>
#include <algorithm>
#include <iostream>

using namespace mmodern;
using namespace remove_test;

int main(int argc, char **argv) {
	try {
		if (argc < 2 || argc > 4) throw std::runtime_error("usage: mmodern_remove_smoke <game-directory> [output-directory] [sdl]");
		const std::filesystem::path output=argc>2?argv[2]:"build/16c/real-remove";
		std::filesystem::create_directories(output);
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(), "Clouds installation unavailable");
		XeenAssetSource assets(*installation,320,200);
		const XeenMapIdentity mapId{XeenSide::Clouds, 23};
		const XeenMapLoader mapLoader;
		int mapLoads = 0, objectLoads = 0;
		XeenWorld world([&](XeenMapIdentity id) {
			++mapLoads; return mapLoader.loadGeometryMap(assets, id);
		},
			[&](XeenMapIdentity id) { ++objectLoads; return mapLoader.loadObjects(assets, id); });
		const XeenEventLoader loader([&](const std::string &name) -> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasInitialResource(name)) return std::nullopt;
			return assets.readInitialResource(name);
		});
		const XeenEventTextLoader textLoader([&](const std::string &name)
				-> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasArchiveResource(name)) return std::nullopt;
			return assets.readArchiveResource(name);
		});
		const auto geometry = geometrySnapshot(world.map(mapId).geometry);
		check(objectLoads == 0, "geometry load eagerly read MOB");
		const auto baseObjects = world.objectFile(mapId);
		check(baseObjects.resourcePresent && baseObjects.resourceName == "maze0023.mob" &&
			baseObjects.entities.objects.size() > 13, "map 23 MOB unavailable or unexpected count");
		const auto &plant = baseObjects.entities.objects[13];
		std::cout << "MOB objects=" << baseObjects.entities.objects.size() << "; index=13 resource="
			<< plant.resourceId << " position=(" << plant.x << ',' << plant.y << ") active="
			<< plant.isActive() << '\n';
		check(plant.resourceId == 111 && plant.x == 8 && plant.y == 2 && plant.isActive(),
			"MOB checkpoint disagrees with approved object 13/resource 111");
		XeenCamera camera{mapId,8,2,XeenDirection::North};
		const auto selected = world.selectObject(camera);
		check(selected == XeenObjectIdentity{mapId,13}, "production resolver did not select original object 13");
		const XeenEventScript original(loader.load(mapId));
		check(original.file().resourcePresent && original.file().resourceName == "maze0023.evt" &&
			original.records().size() == 170, "EVT checkpoint record count/resource mismatch");
		const std::array<std::size_t,11> offsets{1056,1063,1072,1078,1087,1094,1103,1113,1119,1125,1132};
		std::size_t cellRecords=0;
		for (std::size_t i=0;i<original.records().size();++i) {
			const auto &r=original.records()[i];
			if(r.x!=8 || r.y!=2) continue;
			check(i==125+cellRecords && cellRecords<offsets.size(), "unexpected physical-cell event index");
			check(r.fileOffset==offsets[cellRecords] && r.line==cellRecords &&
				r.direction==kXeenEventDirectionAll, "EVT offset/line/direction mismatch");
			++cellRecords;
		}
		check(cellRecords==11 && original.records()[132].opcode==0x0e &&
			original.records()[132].parameters.empty() && original.records()[131].opcode==0x0c,
			"Remove boundary or preceding TakeOrGive mismatch");
		const auto beforeMob=assets.readInitialResource("maze0023.mob");
		const auto beforeEvt=assets.readInitialResource("maze0023.evt");
		auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
		auto flags=XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		const auto beforeFlags=flags.values();
		int scriptLoads = 0, textLoads = 0;
		XeenEventSystem events([&](XeenMapIdentity id) {
			++scriptLoads; return XeenEventScript(loader.load(id));
		}, [&](XeenMapIdentity id) {
			++textLoads; return textLoader.load(id);
		});
		const XeenFontFormat font(assets.readArchiveResource("fnt"));
		const CloudsMapComposer composer;
		const XeenCharacterRulesContext context{kCloudsInitialYear};
		std::uint64_t observedPhase=0;
		XeenEventFlow flow(world,events,party,camera,flags,font,[&](std::uint64_t phase){observedPhase=phase;XeenEventFlow::Composition result;result.frame=composer.compose(assets,world,party,camera,context,nullptr,phase,&result.containsOrdinaryAnimation);return result;});
		const auto beforeFrame=flow.frame();
		visual_remove_test::save(beforeFrame,output/"phirna-before.bmp");
		const auto resolver=XeenObjectVisualResolver::load(assets);
		const auto beforeCommands=XeenOutdoorScene().build(world,camera,&resolver);
		check(std::any_of(beforeCommands.begin(),beforeCommands.end(),[](const auto &c){return c.object() && c.object()->visual.identity==XeenObjectIdentity{23,13};}),"Phirna initial draw command missing");
		// The production interpreter resolves selection itself. The only checkpoint
		// override is initial line 7; normal gameplay continues to start at line 0.
		XeenEventExecutionStepResult result;
		auto removeCheckpoint=[&]{
			result=XeenEventInterpreter().begin(camera,party,flags,world,
				[&](XeenMapIdentity id) { check(id==mapId,"unexpected script map"); return original; }, {}, 7);
			return visual_remove_test::checkpoint(flow,result,camera,flags);
		};
		if(argc==4){
			std::atomic<bool> dispatched{false};
			std::thread input([&]{
				for(int i=0;i<100 && !dispatched;++i){std::this_thread::sleep_for(std::chrono::milliseconds(50));if(SDL_WasInit(SDL_INIT_VIDEO)){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_SPACE;SDL_PushEvent(&e);}}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);
			});
			const bool ok=SdlWindow().showInteractive(beforeFrame,"M16C original Remove checkpoint",[&](const PlayerAction &action)->std::optional<IndexedFrame>{
				if(!dispatched){auto frame=removeCheckpoint();dispatched=true;return frame;}return flow.handle(action);
			});input.join();check(ok && dispatched,"SDL runtime checkpoint failed");
		}else removeCheckpoint();
		const auto afterFrame=flow.frame(); // Already refreshed by production result handling.
		const auto reference=composer.compose(assets,world,party,camera,context,nullptr,observedPhase);
		check(afterFrame.pixels==reference.pixels,"runtime result differs from effective reference");
		std::size_t changedPixels=0;
		for(int y=0;y<200;++y)for(int x=0;x<320;++x)if(beforeFrame.pixels[y*320+x]!=afterFrame.pixels[y*320+x]){
			++changedPixels;check(x>=8 && x<223 && y>=8 && y<141,"Remove changed UI/outside scene");
		}
		check(changedPixels>0,"Phirna had no visible contribution");
		const auto afterCommands=XeenOutdoorScene().build(world,camera,&resolver);
		check(afterCommands.size()+1==beforeCommands.size(),"Remove changed unrelated draw commands");
		for(const auto &c:afterCommands)if(c.object())check(!(c.object()->visual.identity==XeenObjectIdentity{23,13}),"Phirna command survived");
		visual_remove_test::save(afterFrame,output/"phirna-after.bmp");
		std::cout<<"Runtime no-motion pixel removal: "<<changedPixels<<" changed scene pixels; full reference equality; UI unchanged\n";
		if(const auto *e=std::get_if<XeenEventExecutionError>(&result))
			throw std::runtime_error("real Remove execution failed: "+e->message);
		const auto &done=std::get<XeenEventExecutionCompleted>(result);
		check(done.instructionCount==12, "expected Remove then eleven None dispatches from line zero");
		check(done.finalCamera.mapId==mapId && done.finalCamera.x==8 && done.finalCamera.y==2 &&
			done.finalCamera.direction==camera.direction && done.finalGameFlags.values()==beforeFlags &&
			flags.values()==beforeFlags, "checkpoint changed camera/flags");
		check(world.sessionState().disabledObjectCount()==1 && world.isObjectDisabled({mapId,13}) &&
			world.sessionState().disabledEventCount()==11, "unexpected mutation set size");
		for(std::size_t i=0;i<baseObjects.entities.objects.size();++i)
			check(world.sessionState().isObjectDisabled({mapId,i})==(i==13), "unrelated object changed");
		sameEntities(baseObjects.entities,world.objectFile(mapId).entities);
		const auto reloaded=loader.load(mapId);
		check(reloaded.records.size()==original.records().size(), "base event count changed");
		for(std::size_t i=0;i<original.records().size();++i) {
			const auto &base=original.records()[i];
			check(sameRecord(base,reloaded.records[i]),"original event data changed");
			const auto effective=world.effectiveEvent({mapId,i},base);
			check(sameRecord(base,effective,false) &&
				effective.opcode==((i>=125 && i<=135)?0:base.opcode),"unexpected effective event change");
		}
		check(geometry==geometrySnapshot(world.map(mapId).geometry),"map geometry/flags changed");
		check(beforeMob==assets.readInitialResource("maze0023.mob") &&
			beforeEvt==assets.readInitialResource("maze0023.evt"),"commercial base bytes changed");

		// A normal interaction after mutation starts at line zero. The immutable
		// script remains cached while effective state suppresses the original branch.
		XeenCamera repeatedCamera = camera;
		XeenGameFlags repeatedFlags = flags;
		const auto repeated = events.runManualEvent(world, party, repeatedCamera, repeatedFlags);
		flow.acceptManual(repeated);
		check(flow.frame().pixels==afterFrame.pixels,"normal post-Remove interaction restored pixels");
		check(std::get<XeenManualEventCompleted>(repeated).instructionCount == 11 &&
			scriptLoads == 1 && world.selectObject(camera) == std::nullopt,
			"normal post-Remove interaction replayed the Phirna branch");

		// Leave and return through production geometry loads using the same owner.
		camera={1,1,14,XeenDirection::West};flow.refresh();
		visual_remove_test::save(flow.frame(),output/"control-air-corner.bmp");
		camera={23,8,2,XeenDirection::North};
		check(flow.refresh().pixels==afterFrame.pixels,"leave/return restored plant pixels");
		visual_remove_test::save(flow.frame(),output/"phirna-return.bmp");
		const auto *sessionOwner = &world.sessionState();
		check(world.map({XeenSide::Clouds, 1}).identity() == XeenMapIdentity{XeenSide::Clouds, 1},
			"failed to load another real Clouds map");
		check(world.map(mapId).identity() == mapId && sessionOwner == &world.sessionState() &&
			world.isObjectDisabled({mapId,13}), "leave/return lost session mutation");
		for (std::size_t i=125;i<=135;++i)
			check(world.effectiveEvent({mapId,i},original.records()[i]).opcode==0,
				"leave/return restored a cell event");

		// Prove actual map/object reconstruction without replacing the session owner.
		const int mapsBeforeDiscard = mapLoads;
		const int objectsBeforeDiscard = objectLoads;
		world.discardMapCache();
		check(world.cachedMapCount()==0 && world.cachedObjectFileCount()==0,
			"map/object caches were not discarded");
		const auto &rebuiltMap = world.map(mapId);
		const auto &rebuiltObjects = world.objectFile(mapId);
		check(mapLoads==mapsBeforeDiscard+1 && objectLoads==objectsBeforeDiscard+1 &&
			rebuiltMap.identity()==mapId && rebuiltObjects.entities.objects[13].resourceId==111 &&
			world.isObjectDisabled({mapId,13}), "map/object cache was not rebuilt with mutation intact");
		check(flow.refresh(true).pixels==afterFrame.pixels,"map reconstruction restored pixels");

		// Independently rebuild the script cache and run normal lookup again.
		events.discardScriptCache();
		check(events.cachedScriptCount()==0, "script cache was not discarded");
		const int scriptsBeforeReload = scriptLoads;
		repeatedCamera = camera;
		const auto scriptReload = events.runManualEvent(world, party, repeatedCamera, repeatedFlags);
		check(std::get<XeenManualEventCompleted>(scriptReload).instructionCount==11 &&
			scriptLoads==scriptsBeforeReload+1, "script provider was not reloaded");
		const auto reloadedAgain = loader.load(mapId);
		check(reloadedAgain.records[132].opcode==0x0e &&
			reloadedAgain.records[132].fileOffset==1113,
			"script reconstruction changed immutable Remove metadata");
		flow.acceptManual(scriptReload);
		check(flow.frame().pixels==afterFrame.pixels,"script reconstruction restored pixels");

		// Exercise real text caching on the existing Castle Basenji interaction.
		XeenCamera castle{{XeenSide::Clouds,1},8,8,XeenDirection::West};
		XeenGameFlags castleFlags=flags;
		auto castlePending=std::get<XeenEventExecutionSuspended>(
			events.runManualEvent(world,party,castle,castleFlags));
		check(castlePending.request.textIndex==19 && textLoads==1,
			"real event text did not load before discard");
		events.discardTextCache();
		check(events.cachedTextCount()==0,"text cache was not discarded");
		castle={{XeenSide::Clouds,1},8,8,XeenDirection::West};
		castlePending=std::get<XeenEventExecutionSuspended>(
			events.runManualEvent(world,party,castle,castleFlags));
		check(castlePending.request.textIndex==19 && textLoads==2 &&
			world.isObjectDisabled({mapId,13}), "text reload changed behavior or world state");

		// Discard all caches together, then prove every provider is invoked again.
		const int combinedMaps=mapLoads, combinedObjects=objectLoads;
		const int combinedScripts=scriptLoads, combinedTexts=textLoads;
		world.discardMapCache(); events.discardScriptCache(); events.discardTextCache();
		world.map(mapId); world.objectFile(mapId);
		repeatedCamera=camera;
		check(std::holds_alternative<XeenManualEventCompleted>(
			events.runManualEvent(world,party,repeatedCamera,repeatedFlags)),
			"combined rebuild broke normal Phirna interaction");
		castle={{XeenSide::Clouds,1},8,8,XeenDirection::West};
		check(std::holds_alternative<XeenEventExecutionSuspended>(
			events.runManualEvent(world,party,castle,castleFlags)),
			"combined rebuild broke real text interaction");
		check(mapLoads>=combinedMaps+2 && objectLoads>=combinedObjects+2 &&
			scriptLoads>=combinedScripts+2 && textLoads==combinedTexts+1,
			"combined cache reconstruction did not call every provider");
		check(world.isObjectDisabled({mapId,13}) &&
			!world.isEventDisabled({{XeenSide::Clouds,1},0}),
			"combined reconstruction changed mutation scope");
		const auto spriteLoads=assets.spriteLoadCount();
		assets.discardSpriteCache();check(assets.cachedSpriteCount()==0,"sprite cache not discarded");
		check(flow.refresh(true).pixels==afterFrame.pixels,"resource reconstruction restored plant");
		check(assets.spriteLoadCount()>spriteLoads,"sprite objects not reconstructed");
		visual_remove_test::save(flow.frame(),output/"phirna-rebuilt.bmp");
		// A surviving object proves visual sprite reconstruction independently of
		// terrain reads. Never reactivate the plant to exercise this cache.
		camera={23,12,2,XeenDirection::North};const auto control=flow.refresh();
		assets.discardSpriteCache();const auto controlLoads=assets.spriteLoadCount();
		const auto controlVisual=resolver.resolve(world.objectFile(23),11,XeenDirection::North);
		assets.drawObjectVisual(controlVisual,-5,2);
		check(assets.spriteLoadCount()==controlLoads+1 && assets.cachedSpriteCount()==1,"surviving object sprite was not reread/reconstructed");
		assets.drawObjectVisual(controlVisual,-5,2);
		check(assets.spriteLoadCount()==controlLoads+1,"cached object draw reconstructed again");
		check(flow.refresh(true).pixels==control.pixels && assets.spriteLoadCount()>controlLoads,"control object reconstruction");
		visual_remove_test::save(flow.frame(),output/"control-117.bmp");
		camera={23,8,2,XeenDirection::North};check(flow.refresh().pixels==afterFrame.pixels,"control view altered plant");
		// Real presentation suspended across every cache discard, same runtime flow.
		camera={1,8,8,XeenDirection::West};flow.refresh();flow.handle(InteractionAction{});
		check(flow.blocksGameplay(),"real Castle choice missing");const auto question=flow.frame();
		const int textsBefore=textLoads;
		world.discardMapCache();events.discardScriptCache();events.discardTextCache();assets.discardSpriteCache();
		check(flow.refresh(true).pixels==question.pixels && flow.blocksGameplay(),"real suspended rebase changed question");
		flow.handle(NoAction{});check(!flow.blocksGameplay(),"real No did not resume");
		flow.handle(InteractionAction{});check(textLoads>textsBefore,"real text not reloaded");flow.handle(NoAction{});
		camera={23,8,2,XeenDirection::North};check(flow.refresh().pixels==afterFrame.pixels,"combined suspended rebuild restored plant");
		// Presentation over the modified real scene; semantic request only, with
		// no commercial script replacement or additional interpreter execution.
		XeenEventPresenter overlay(font);XeenPresentationRequest label;
		label.kind=XeenPresentationKind::SceneLabelSign;label.text="Removed";
		label.response=XeenPresentationResponseRequirement::Presented;
		visual_remove_test::save(overlay.present(flow.frame(),label).frame,output/"phirna-presentation.bmp");

		// A genuinely new world/event-system graph restores resource-effective state.
		int freshObjectLoads=0;
		XeenWorld freshWorld([&](XeenMapIdentity id) {
			return mapLoader.loadGeometryMap(assets,id);
		}, [&](XeenMapIdentity id) {
			++freshObjectLoads; return mapLoader.loadObjects(assets,id);
		});
		XeenEventSystem freshEvents([&](XeenMapIdentity id) {
			return XeenEventScript(loader.load(id));
		}, [&](XeenMapIdentity id) { return textLoader.load(id); });
		check(&freshWorld.sessionState()!=sessionOwner &&
			freshWorld.sessionState().disabledObjectCount()==0 &&
			freshWorld.sessionState().disabledEventCount()==0 &&
			freshWorld.selectObject(camera)==XeenObjectIdentity{mapId,13} &&
			freshObjectLoads==1 && freshEvents.cachedScriptCount()==0,
			"genuine new session did not restore original object state");
		for(std::size_t i=125;i<=135;++i)
			check(freshWorld.effectiveEvent({mapId,i},original.records()[i]).opcode==
				original.records()[i].opcode, "new session inherited an event mutation");
		XeenCamera freshCamera=camera;XeenGameFlags freshFlags=flags;
		XeenEventFlow freshFlow(freshWorld,freshEvents,party,freshCamera,freshFlags,font,[&](std::uint64_t phase){XeenEventFlow::Composition result;result.frame=composer.compose(assets,freshWorld,party,freshCamera,context,nullptr,phase,&result.containsOrdinaryAnimation);return result;});
		check(freshFlow.frame().pixels==beforeFrame.pixels && !freshFlow.blocksGameplay(),"fresh graph did not restore initial pixels");
		visual_remove_test::save(freshFlow.frame(),output/"phirna-new-session.bmp");
		std::cout<<"Sprite constructions after actual cache reads="<<assets.spriteLoadCount()<<'\n';
		check(beforeMob==assets.readInitialResource("maze0023.mob") &&
			beforeEvt==assets.readInitialResource("maze0023.evt"),
			"lifecycle validation changed commercial resource bytes");
		std::cout << "EVT records=170; cell indices=125..135; Remove index=132 line=7 offset=1113 operands=0\n"
			<< "Production selection=13; disabled objects=1 events=11; dispatched=" << done.instructionCount
			<< " (Remove + 11 None from logical line 0); base/unrelated state unchanged\n"
			<< "Same-session leave/return, map/object/script/text/combined rebuild and fresh-session restoration OK\n"
			<< "Provider loads: maps=" << mapLoads << " objects=" << objectLoads
			<< " scripts=" << scriptLoads << " texts=" << textLoads << '\n';
		return 0;
	} catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
