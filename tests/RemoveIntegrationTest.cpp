#include "XeenRemoveTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include <array>
#include <iostream>

using namespace mmodern;
using namespace remove_test;

int main(int argc, char **argv) {
	try {
		if (argc != 2) throw std::runtime_error("usage: mmodern_remove_smoke <game-directory>");
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(), "Clouds installation unavailable");
		XeenAssetSource assets(*installation);
		const XeenMapIdentity mapId{XeenSide::Clouds, 23};
		const XeenMapLoader mapLoader;
		int objectLoads = 0;
		XeenWorld world([&](XeenMapIdentity id) { return mapLoader.loadGeometryMap(assets, id); },
			[&](XeenMapIdentity id) { ++objectLoads; return mapLoader.loadObjects(assets, id); });
		const XeenEventLoader loader([&](const std::string &name) -> std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasInitialResource(name)) return std::nullopt;
			return assets.readInitialResource(name);
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
		const XeenCamera camera{mapId,8,2,XeenDirection::North};
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
		const auto party=XeenPartyLoader().loadInitialCloudsParty(assets);
		const auto flags=XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
		const auto beforeFlags=flags.values();
		// The production interpreter resolves selection itself. The only checkpoint
		// override is initial line 7; normal gameplay continues to start at line 0.
		const auto result=XeenEventInterpreter().begin(camera,party,flags,world,
			[&](XeenMapIdentity id) { check(id==mapId,"unexpected script map"); return original; }, {}, 7);
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
		std::cout << "EVT records=170; cell indices=125..135; Remove index=132 line=7 offset=1113 operands=0\n"
			<< "Production selection=13; disabled objects=1 events=11; dispatched=" << done.instructionCount
			<< " (Remove + 11 None from logical line 0); base/unrelated state unchanged\n";
		return 0;
	} catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
