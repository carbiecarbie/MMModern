#include "SyntheticXeenArchive.h"
#include "XeenIndoorObjectTestOracle.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenFontFormat.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenIndoorSceneTables.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenSaveState.h"
#include "games/xeen/XeenWorld.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>

using namespace mmodern;
using namespace sprite_test;

namespace {

void check(bool condition, const char *message) {
	if (!condition) throw std::runtime_error(message);
}

Bytes repeatedFrames(const Bytes &stream, unsigned count) {
	Bytes bytes;
	word(bytes, count);
	for (unsigned i = 0; i < count; ++i) {
		word(bytes, 2 + count * 4);
		word(bytes, 0);
	}
	bytes.insert(bytes.end(), stream.begin(), stream.end());
	return bytes;
}

Bytes solid(unsigned x, unsigned width, unsigned y, unsigned height,
		std::uint8_t color) {
	Bytes rows;
	for (unsigned row = 0; row < height; ++row) {
		Bytes payload{0};
		for (unsigned column = 0; column < width;) {
			const unsigned count = std::min(32u, width - column);
			payload.push_back(static_cast<std::uint8_t>(count - 1));
			payload.insert(payload.end(), count, color);
			column += count;
		}
		rows.push_back(static_cast<std::uint8_t>(payload.size()));
		rows.insert(rows.end(), payload.begin(), payload.end());
	}
	return cell(x, width, y, height, rows);
}

Bytes transparentBands(unsigned width, unsigned height, std::uint8_t color) {
	check(width == 160, "transparent wall fixture width");
	Bytes rows;
	for (unsigned row = 0; row < height; ++row) {
		Bytes payload{0, 31};
		payload.insert(payload.end(), 32, color);
		payload.push_back(0xbf); // Transparent span of 32 pixels.
		payload.push_back(31);
		payload.insert(payload.end(), 32, color);
		payload.push_back(0xbf);
		payload.push_back(0xbf);
		rows.push_back(static_cast<std::uint8_t>(payload.size()));
		rows.insert(rows.end(), payload.begin(), payload.end());
	}
	return cell(0, width, 0, height, rows);
}

Bytes selectedWallFrames() {
	std::vector<std::pair<Bytes, Bytes>> frames(34,
		{cell(0, 0, 0, 0, {}), {}});
	frames[17].first = solid(0, 160, 0, 80, 42);
	return multiFrameSprite(frames);
}

XeenMap indoorMap(XeenMapIdentity id = 33) {
	XeenMap map;
	map.side = id.side;
	map.geometry.id = id.number;
	map.geometry.wallKind = 0;
	for (auto &cellValue : map.geometry.cells)
		cellValue.geometry = XeenIndoorWalls{};
	return map;
}

void setQueryWall(XeenMap &map, const XeenCamera &camera,
		std::size_t query, std::uint8_t value) {
	const auto direction = static_cast<std::size_t>(camera.direction);
	const int x = camera.x + xeen_indoor_scene_tables::kScreenPositioningX[direction][query];
	const int y = camera.y + xeen_indoor_scene_tables::kScreenPositioningY[direction][query];
	check(x >= 0 && x < 16 && y >= 0 && y < 16, "wall fixture query outside map");
	const auto shift = xeen_indoor_scene_tables::kWallShifts[direction][query];
	const std::size_t face = shift == 12 ? 0 : shift == 8 ? 1 : shift == 4 ? 2 : 3;
	std::get<XeenIndoorWalls>(map.geometry.cells[static_cast<std::size_t>(y) * 16 + x].geometry)
		.walls[face] = value;
}

XeenObjectFile objectFile(XeenMapIdentity id, std::vector<XeenMapEntity> records) {
	XeenObjectFile file;
	file.mapId = id;
	file.resourceName = "synthetic.mob";
	file.resourcePresent = true;
	file.entities.objects = std::move(records);
	return file;
}

XeenWorld worldWith(XeenMap map, XeenObjectFile objects) {
	return XeenWorld(
		[map = std::move(map)](XeenMapIdentity id) {
			check(id == map.identity(), "unexpected indoor map request");
			return map;
		},
		[objects = std::move(objects)](XeenMapIdentity id) {
			check(id == objects.mapId, "unexpected indoor MOB request");
			return objects;
		});
}

const XeenIndoorDrawCommand &targetCommand(
		const std::vector<XeenIndoorDrawCommand> &commands, XeenObjectIdentity id) {
	const auto found = std::find_if(commands.begin(), commands.end(), [&](const auto &command) {
		return command.object() && command.object()->visual.identity == id;
	});
	if (found == commands.end()) throw std::runtime_error("indoor target command missing");
	return *found;
}

std::map<std::string, Bytes> baseFiles() {
	const auto empty = repeatedFrames(cell(0, 0, 0, 0, {}), 50);
	std::map<std::string, Bytes> files;
	for (const char *name : {"town.sky", "town.gnd", "ftown1.fwl", "ftown2.fwl",
			"ftown4.fwl", "global.icn", "border.icn", "fecp.brd", "bless.icn",
			"restorex.icn", "main.icn"})
		files[name] = empty;
	files["ftown3.fwl"] = selectedWallFrames();
	files["stown.swl"] = repeatedFrames(transparentBands(160, 80, 43), 48);
	files["back.raw"] = Bytes(320 * 200, 99);
	files["mm4.pal"] = Bytes(768);
	files["global.icn"] = sprite(solid(0, 1, 0, 1, 55));
	files["111.0bj"] = sprite(solid(0, 240, 0, 160, 7));
	files["113.0bj"] = sprite(solid(0, 240, 0, 200, 8),
		solid(180, 40, 20, 60, 9));
	files["115.0bj"] = sprite(solid(30, 16, 10, 16, 61));
	files["110.0bj"] = multiFrameSprite({
		{solid(0, 40, 0, 40, 17), {}},
		{solid(0, 40, 0, 40, 18), {}},
		{solid(0, 40, 0, 40, 19), {}}
	});
	files["112.0bj"] = multiFrameSprite({
		{solid(0, 40, 0, 40, 27), {}},
		{cell(0, 8, 0, 1, {2, 0, 1}), {}}
	});
	return files;
}

Bytes metadata() {
	Bytes bytes(1452);
	for (const int resource : {111, 113, 115})
		for (std::size_t relative = 0; relative < 4; ++relative) {
			bytes[static_cast<std::size_t>(resource) * 12 + relative] = 0;
			bytes[static_cast<std::size_t>(resource) * 12 + 4 + relative] =
				static_cast<std::uint8_t>(relative % 2);
			bytes[static_cast<std::size_t>(resource) * 12 + 8 + relative] = 1;
		}
	for (std::size_t relative = 0; relative < 4; ++relative)
		bytes[110 * 12 + 8 + relative] = 3;
	bytes[112 * 12 + 1] = 1;
	bytes[112 * 12 + 9] = 2;
	bytes[112 * 12 + 8] = 1;
	return bytes;
}

GameInstallation installationAt(const std::filesystem::path &directory,
		const std::map<std::string, Bytes> &files, const Bytes &visualMetadata) {
	archive(directory / "xeen.cc", files);
	archive(directory / "dark.cc", {{"clouds.dat", visualMetadata}});
	GameInstallation installation;
	installation.xeenArchive = directory / "xeen.cc";
	installation.darkArchive = directory / "dark.cc";
	return installation;
}

Bytes fontBytes() {
	Bytes bytes(XeenFontFormat::kMinimumSize);
	for (int character = 0; character < 128; ++character) {
		bytes[0x1000 + character] = 6;
		bytes[0x1080 + character] = 3;
		for (int y = 0; y < 8; ++y) {
			bytes[character * 16 + y * 2] = 0x55;
			bytes[0x800 + character * 16 + y * 2] = 0x55;
		}
	}
	return bytes;
}

XeenPartyState validEmptyParty() {
	XeenPartyState party;
	for (std::size_t i = 0; i < XeenRoster::kCharacterCount; ++i)
		party.roster.at(i).rosterId = static_cast<std::uint8_t>(i);
	return party;
}

XeenEventRecord indoorEventRecord(std::uint8_t line, std::uint8_t opcode,
		std::vector<std::uint8_t> parameters, std::size_t offset) {
	return {offset, static_cast<std::uint8_t>(5 + parameters.size()),
		8, 8, kXeenEventDirectionAll, line, opcode, std::move(parameters)};
}

struct IndoorRemoveSources {
	int maps = 0;
	int objects = 0;
	int scripts = 0;
	int texts = 0;
	XeenMapIdentity mapId = 33;

	XeenMap loadMap(XeenMapIdentity id) {
		check(id == mapId, "unexpected indoor Remove map request");
		++maps;
		return indoorMap(id);
	}
	XeenObjectFile loadObjects(XeenMapIdentity id) {
		check(id == mapId, "unexpected indoor Remove object request");
		++objects;
		// Record 0 is the same-cell interaction target. Record 1 is a visible
		// one-forward-right sibling with the same resolved sprite resource.
		return objectFile(id, {{8,8,0,0,115},{9,9,0,0,115}});
	}
	XeenEventFile loadEvents(XeenMapIdentity id) {
		check(id == mapId, "unexpected indoor Remove event request");
		++scripts;
		return {id, "synthetic.evt", true, {
			indoorEventRecord(0, 0x29, {0}, 100),
			indoorEventRecord(1, 0x09, {44,1,2}, 106),
			indoorEventRecord(2, 0x0e, {}, 114)
		}};
	}
	XeenEventTextFile loadText(XeenMapIdentity id) {
		check(id == mapId, "unexpected indoor Remove text request");
		++texts;
		return {id, "synthetic.txt", true, {"Indoor target removal"}};
	}
};

std::vector<std::size_t> changedPixels(
		const IndexedFrame &withIdentity, const IndexedFrame &withoutIdentity) {
	check(withIdentity.pixels.size() == withoutIdentity.pixels.size(),
		"identity attribution frame size mismatch");
	std::vector<std::size_t> result;
	for (std::size_t pixel = 0; pixel < withIdentity.pixels.size(); ++pixel)
		if (withIdentity.pixels[pixel] != withoutIdentity.pixels[pixel])
			result.push_back(pixel);
	return result;
}

IndexedFrame replay(XeenAssetSource &assets, const CloudsMapComposer &composer,
		const XeenPartyState &party, const XeenCharacterRulesContext &context,
		const std::vector<XeenIndoorDrawCommand> &commands,
		std::optional<XeenObjectIdentity> omitted = std::nullopt) {
	CloudsUiComposer().loadBackground(assets);
	for (const auto &command : commands) {
		if (omitted && command.object() && command.object()->visual.identity == *omitted)
			continue;
		composer.drawIndoorCommands(assets, {command});
	}
	composer.drawInterfaceLayers(assets, party, context);
	return assets.snapshot();
}

void testAllPlacementPixels(const GameInstallation &installation) {
	struct Bounds { std::size_t minX, minY, maxX, maxY; };
	// Literal expected output bounds for the synthetic solid sprites at the
	// anchors/scales in XeenIndoorObjectTestOracle.h. These values neither query
	// nor derive from the production placement table.
	static constexpr std::array<Bounds, 12> normalBounds = {{
		{ 8, 8,222,139}, {45,25,179,114}, { 8,25, 74,114}, {150,25,222,114},
		{82,50,141, 89}, {25,50, 84, 89}, {139,50,198, 89}, { 96,58,125,77},
		{71,58,100, 77}, {121,58,150,77}, { 47,58, 76,77}, {145,58,174,77}
	}};
	static constexpr std::array<Bounds, 12> resource113Bounds = {{
		{ 8, 8,204,134}, {17, 8,151,106}, { 8, 8, 44,106}, {120, 8,222,106},
		{25,36,114, 85}, { 8,36, 54, 85}, {79,36,168, 85}, { 8,54, 99,78},
		{43,54, 72, 78}, {26,54,120, 78}, { 8,54, 36,78}, {56,54,150,78}
	}};
	const CloudsMapComposer composer;
	for (const int resource : {111, 113}) {
		for (const auto direction : {XeenDirection::North, XeenDirection::East,
				XeenDirection::South, XeenDirection::West}) {
			const XeenCamera camera{33, 8, 8, direction};
			const auto directionIndex = static_cast<std::size_t>(direction);
			std::vector<XeenMapEntity> records;
			for (const auto &placement : indoor_object_test::kPlacementSpecs) {
				const auto source = indoor_object_test::expectedSource(camera, placement);
				records.push_back({source.x, source.y, 0, 0, resource});
			}
			XeenAssetSource assets(installation, 320, 200);
			auto world = worldWith(indoorMap(), objectFile(33, records));
			const auto resolver = XeenObjectVisualResolver::load(assets);
			const auto commands = XeenIndoorScene().build(world, camera, &resolver);
			for (std::size_t record = 0; record < records.size(); ++record) {
				const auto &specification = indoor_object_test::kPlacementSpecs[record];
				assets.loadRawFramebuffer("back.raw");
				const auto before = assets.snapshot();
				composer.drawIndoorCommands(assets,
					{targetCommand(commands, {33, record})});
				const auto after = assets.snapshot();
				std::size_t changed = 0, minX = 320, minY = 200, maxX = 0, maxY = 0;
				for (std::size_t pixel = 0; pixel < after.pixels.size(); ++pixel)
					if (before.pixels[pixel] != after.pixels[pixel]) {
						++changed;
						const auto x = pixel % 320, y = pixel / 320;
						minX = std::min(minX, x); maxX = std::max(maxX, x);
						minY = std::min(minY, y); maxY = std::max(maxY, y);
						check(x >= 8 && x < 223 && y >= 8 && y < 141,
							"indoor placement escaped scene clipping");
						if (record == 0) check(y < 140, "query-2 object escaped bottom clipping");
					}
				if (!changed)
					throw std::runtime_error("indoor placement produced no checked object pixels: resource " +
						std::to_string(resource) + ", direction " +
						std::to_string(directionIndex) + ", record " + std::to_string(record));
				const auto &expected = resource == 113 ?
					resource113Bounds[record] : normalBounds[record];
				if (minX != expected.minX || minY != expected.minY ||
						maxX != expected.maxX || maxY != expected.maxY)
					throw std::runtime_error("indoor placement raster bounds mismatch at query " +
						std::to_string(specification.query) + ", resource " +
						std::to_string(resource) + ", direction " +
						std::to_string(directionIndex));
				if (resource == 113 && record == 0)
					check(std::count(after.pixels.begin(), after.pixels.end(), 8) > 0 &&
						std::count(after.pixels.begin(), after.pixels.end(), 9) > 0,
						"both cells of indoor resource 113 did not contribute");
			}
		}
	}
}

void testAdmittedObjectFullyCovered(const GameInstallation &installation) {
	const XeenCamera camera{33, 8, 8, XeenDirection::North};
	auto map = indoorMap();
	setQueryWall(map, camera, 13, 8);
	auto world = worldWith(std::move(map), objectFile(33, {{8,10,0,0,111}}));
	XeenAssetSource assets(installation, 320, 200);
	const auto resolver = XeenObjectVisualResolver::load(assets);
	const auto commands = XeenIndoorScene().build(world, camera, &resolver);
	const auto &target = targetCommand(commands, {33,0});
	assets.loadRawFramebuffer("back.raw");
	CloudsMapComposer().drawIndoorCommands(assets, {target});
	const auto throughTarget = assets.snapshot();
	check(std::count(throughTarget.pixels.begin(), throughTarget.pixels.end(), 7) > 0,
		"fully covered fixture object did not draw before the wall");
	const auto full = CloudsMapComposer().compose(assets, world, {}, camera, {});
	check(std::count(full.pixels.begin(), full.pixels.end(), 7) == 0,
		"admitted indoor object survived an opaque later wall");
}

void testOrderedWallCoverage(const GameInstallation &installation) {
	const XeenCamera camera{33, 8, 8, XeenDirection::North};
	auto map = indoorMap();
	setQueryWall(map, camera, 14, 8); // Back wall, order 89.
	setQueryWall(map, camera, 13, 8); // Nearer side wall, order 108.
	auto world = worldWith(std::move(map), objectFile(33, {{8,10,0,0,111}}));
	XeenAssetSource assets(installation, 320, 200);
	const auto resolver = XeenObjectVisualResolver::load(assets);
	const auto commands = XeenIndoorScene().build(world, camera, &resolver);
	const auto &target = targetCommand(commands, {33,0});
	check(target.queryIndex == 14 && target.originalOrder == 97,
		"coverage fixture selected wrong object placement");
	const CloudsMapComposer composer;
	assets.loadRawFramebuffer("back.raw");
	IndexedFrame beforeObject, afterObject;
	for (const auto &command : commands) {
		if (command.object()) beforeObject = assets.snapshot();
		composer.drawIndoorCommands(assets, {command});
		if (command.object()) afterObject = assets.snapshot();
	}
	const auto actual = assets.snapshot();
	std::size_t coveredBack = 0, overwrittenLater = 0, survived = 0;
	std::size_t minObjectX = 320, maxObjectX = 0, minObjectY = 200, maxObjectY = 0;
	for (std::size_t pixel = 0; pixel < actual.pixels.size(); ++pixel) {
		if (beforeObject.pixels[pixel] == 42 && afterObject.pixels[pixel] == 7)
			++coveredBack;
		if (afterObject.pixels[pixel] == 7 && actual.pixels[pixel] == 43)
			++overwrittenLater;
		if (afterObject.pixels[pixel] == 7) {
			minObjectX = std::min(minObjectX, pixel % 320);
			maxObjectX = std::max(maxObjectX, pixel % 320);
			minObjectY = std::min(minObjectY, pixel / 320);
			maxObjectY = std::max(maxObjectY, pixel / 320);
		}
		if (afterObject.pixels[pixel] == 7 && actual.pixels[pixel] == 7)
			++survived;
	}
	if (!coveredBack || !overwrittenLater || !survived)
		throw std::runtime_error("indoor object/wall ordering or transparent-span coverage failed: back=" +
			std::to_string(coveredBack) + ", later=" + std::to_string(overwrittenLater) +
			", survive=" + std::to_string(survived) + ", bounds=" +
			std::to_string(minObjectX) + "," + std::to_string(minObjectY) + "-" +
			std::to_string(maxObjectX) + "," + std::to_string(maxObjectY));
	assets.loadRawFramebuffer("back.raw");
	for (const auto &command : commands)
		if (!command.object()) composer.drawIndoorCommands(assets, {command});
	composer.drawIndoorCommands(assets, {target});
	const auto targetLast = assets.snapshot();
	check(targetLast.pixels != actual.pixels,
		"object-last replay failed to distinguish ordered indoor rasterization");
	std::cout << "Indoor ordered pixels: back=" << coveredBack
		<< " later=" << overwrittenLater << " survive=" << survived << '\n';
}

void testCompositionLifecycleAndStaticPhase(const GameInstallation &installation) {
	const XeenCamera north{33, 8, 8, XeenDirection::North};
	XeenAssetSource assets(installation, 320, 200);
	auto world = worldWith(indoorMap(), objectFile(33, {{8,8,0,0,111}}));
	const CloudsMapComposer composer;
	bool animation = true;
	std::vector<XeenObjectVisual> diagnostics;
	const auto original = composer.compose(assets, world, {}, north, {}, &diagnostics,
		std::nullopt, &animation);
	check(!animation && diagnostics.empty() && original.pixels[8 * 320 + 8] == 55,
		"indoor composition animation flag, diagnostics, or interface overlay");
	for (const std::uint64_t phase : {0, 1, 2, 999}) {
		animation = true;
		check(composer.compose(assets, world, {}, north, {}, nullptr, phase, &animation).pixels ==
			original.pixels && !animation,
			"outdoor phase changed static indoor output");
	}
	const XeenCamera east{33, 8, 8, XeenDirection::East};
	const auto turned = composer.compose(assets, world, {}, east, {});
	check(turned.isValid(), "indoor turn failed to compose");
	check(composer.compose(assets, world, {}, north, {}).pixels == original.pixels,
		"indoor turn/rebuild left ghost pixels");
	world.disableObject({33,0});
	const auto removed = composer.compose(assets, world, {}, north, {});
	check(removed.pixels != original.pixels &&
		std::count(removed.pixels.begin(), removed.pixels.end(), 7) == 0,
		"indoor object disable did not rebuild a clean frame");
	world.discardMapCache();
	assets.discardSpriteCache();
	check(composer.compose(assets, world, {}, north, {}).pixels == removed.pixels,
		"indoor cache reconstruction revived disabled pixels");

	XeenAssetSource animatedAssets(installation, 320, 200);
	auto animatedWorld = worldWith(indoorMap(), objectFile(33, {{8,8,0,0,110}}));
	diagnostics.clear();
	animation = true;
	const auto animated = composer.compose(animatedAssets, animatedWorld, {}, north, {},
		&diagnostics, 2, &animation);
	check(!animation && diagnostics.size() == 1 &&
		diagnostics[0].status == XeenObjectVisualStatus::UnsupportedAnimation &&
		std::count(animated.pixels.begin(), animated.pixels.end(), 17) == 0 &&
		std::count(animated.pixels.begin(), animated.pixels.end(), 18) == 0 &&
		std::count(animated.pixels.begin(), animated.pixels.end(), 19) == 0,
		"indoor animated appearance froze or activated");
}

void testSelectedFrameFailuresAndRecovery(const GameInstallation &installation) {
	const CloudsMapComposer composer;
	const XeenCamera north{33,8,8,XeenDirection::North};
	const XeenCamera east{33,8,8,XeenDirection::East};
	XeenAssetSource assets(installation, 320, 200);
	auto world = worldWith(indoorMap(), objectFile(33, {{8,8,0,0,112}}));
	const auto valid = composer.compose(assets, world, {}, north, {});
	check(std::count(valid.pixels.begin(), valid.pixels.end(), 27) > 0 &&
		assets.cachedSpriteCount() > 0, "valid indoor frame did not warm sprite cache");
	bool animation = true, rejected = false;
	try {
		(void)composer.compose(assets, world, {}, east, {}, nullptr, 99, &animation);
	} catch (const std::runtime_error &) {
		rejected = true;
	}
	check(rejected && !animation, "malformed selected indoor frame or output flag was accepted");
	check(composer.compose(assets, world, {}, north, {}).pixels == valid.pixels,
		"valid composition after warm-cache failure retained partial pixels");

	XeenAssetSource missingAssets(installation, 320, 200);
	auto missingWorld = worldWith(indoorMap(), objectFile(33, {{8,8,0,0,114}}));
	rejected = false;
	try {
		(void)composer.compose(missingAssets, missingWorld, {}, north, {});
	} catch (const std::runtime_error &) {
		rejected = true;
	}
	check(rejected, "missing selected indoor sprite was replaced or ignored");
}

void testCombinedIndoorRemoveLifecycle(const GameInstallation &installation) {
	const XeenObjectIdentity target{33,0};
	const XeenObjectIdentity sibling{33,1};
	const XeenCamera initialCamera{33,8,8,XeenDirection::North};
	const CloudsMapComposer composer;
	const XeenCharacterRulesContext context{kCloudsInitialYear};
	IndoorRemoveSources sources;
	XeenAssetSource assets(installation, 320, 200);
	XeenWorld world(
		[&](XeenMapIdentity id) { return sources.loadMap(id); },
		[&](XeenMapIdentity id) { return sources.loadObjects(id); });
	XeenEventSystem events(
		[&](XeenMapIdentity id) { return XeenEventScript(sources.loadEvents(id)); },
		[&](XeenMapIdentity id) { return sources.loadText(id); });
	auto party = validEmptyParty();
	XeenCamera camera = initialCamera;
	XeenGameFlags flags;
	const XeenFontFormat font(fontBytes());

	const auto resolver = XeenObjectVisualResolver::load(assets);
	const auto initialCommands = XeenIndoorScene().build(world, camera, &resolver);
	const auto &targetDraw = targetCommand(initialCommands, target);
	const auto &siblingDraw = targetCommand(initialCommands, sibling);
	check(targetDraw.queryIndex == 2 && targetDraw.sourceX == 8 && targetDraw.sourceY == 8 &&
		targetDraw.object()->visual.spriteName == "115.0bj" &&
		siblingDraw.queryIndex == 9 && siblingDraw.sourceX == 9 && siblingDraw.sourceY == 9 &&
		siblingDraw.object()->visual.spriteName == targetDraw.object()->visual.spriteName,
		"indoor Remove fixture did not create distinct shared-resource identities");
	const auto initial = composer.compose(assets, world, party, camera, context);
	const auto targetPixels = changedPixels(initial,
		replay(assets, composer, party, context, initialCommands, target));
	const auto siblingPixels = changedPixels(initial,
		replay(assets, composer, party, context, initialCommands, sibling));
	check(!targetPixels.empty() && !siblingPixels.empty(),
		"initial indoor Remove identities lacked independent visible contributions");
	check(std::none_of(targetPixels.begin(), targetPixels.end(), [&](std::size_t pixel) {
		return std::find(siblingPixels.begin(), siblingPixels.end(), pixel) != siblingPixels.end();
	}), "shared-resource indoor identities were not spatially distinguishable");

	std::vector<XeenManualEventResult> reports;
	XeenEventFlow flow(world, events, party, camera, flags, font,
		[&](std::uint64_t phase) {
			XeenEventFlow::Composition result;
			result.frame = composer.compose(assets, world, party, camera, context,
				nullptr, phase, &result.containsOrdinaryAnimation);
			return result;
		});
	flow.reportManual = [&](const auto &result) { reports.push_back(result); };
	check(flow.frame().pixels == initial.pixels,
		"production Flow initial indoor composition changed the fixture");
	const auto pending = flow.handle(InteractionAction{});
	const auto generation = flow.presentationGeneration();
	check(flow.blocksGameplay() && generation && reports.size() == 2,
		"indoor Remove fixture did not reach the acknowledgment presentation");
	const auto *pendingState = std::get_if<XeenEventExecutionSuspended>(&reports.back());
	check(pendingState && pendingState->state.selectedObject == target &&
		pendingState->request.kind == XeenPresentationKind::Confirmation &&
		pendingState->request.response == XeenPresentationResponseRequirement::Acknowledgment,
		"pending indoor Remove continuation lost the selected target identity");
	const auto pendingPage = flow.presenter().pageIndex();
	const auto pendingPages = flow.presenter().pageCount();
	check(std::any_of(targetPixels.begin(), targetPixels.end(), [&](std::size_t pixel) {
		return pending.pixels[pixel] == initial.pixels[pixel];
	}) && std::any_of(siblingPixels.begin(), siblingPixels.end(), [&](std::size_t pixel) {
		return pending.pixels[pixel] == initial.pixels[pixel];
	}), "pending presentation obscured or conflated the indoor object underlay");
	flow.handle(NavigationAction::TurnRight);
	check(camera.direction == XeenDirection::North &&
		flow.presentationGeneration() == generation &&
		flow.presenter().pageIndex() == pendingPage && flow.presenter().pageCount() == pendingPages,
		"pending indoor Remove presentation accepted gameplay input");

	const auto postRemove = flow.handle(AcknowledgeAction{});
	check(!flow.blocksGameplay() && !flow.presentationGeneration() && reports.size() == 3 &&
		std::holds_alternative<XeenManualEventCompleted>(reports.back()) &&
		world.isObjectDisabled(target) && !world.isObjectDisabled(sibling),
		"Flow continuation did not complete the selected indoor Remove");
	for (std::size_t record = 0; record < 3; ++record)
		check(world.isEventDisabled({33,record}),
			"indoor Remove did not disable a physical-cell event identity");
	const auto postResolver = XeenObjectVisualResolver::load(assets);
	const auto postCommands = XeenIndoorScene().build(world, camera, &postResolver);
	check(std::none_of(postCommands.begin(), postCommands.end(), [&](const auto &command) {
		return command.object() && command.object()->visual.identity == target;
	}) && targetCommand(postCommands, sibling).object()->visual.identity == sibling,
		"indoor Remove hid the sibling or retained the target command");
	const auto postBase = composer.compose(assets, world, party, camera, context);
	check(!changedPixels(postBase,
		replay(assets, composer, party, context, postCommands, sibling)).empty(),
		"shared-resource sibling lost its post-Remove contribution");
	for (int y = 8; y < 140; ++y)
		for (int x = 8; x < 223; ++x)
			check(postRemove.pixels[static_cast<std::size_t>(y) * 320 + x] ==
				postBase.pixels[static_cast<std::size_t>(y) * 320 + x],
				"removed indoor object left ghost pixels beneath retained presentation");
	check(postRemove.pixels != postBase.pixels,
		"indoor Remove completion discarded the retained bottom presentation");

	const int oldMaps = sources.maps, oldObjects = sources.objects,
		oldScripts = sources.scripts;
	const auto oldSpriteLoads = assets.spriteLoadCount();
	const auto retainedPage = flow.presenter().pageIndex();
	const auto retainedPages = flow.presenter().pageCount();
	world.discardMapCache();
	events.discardScriptCache();
	events.discardTextCache();
	assets.discardSpriteCache();
	const auto reconstructed = flow.refresh(true);
	check(reconstructed.pixels == postRemove.pixels && !flow.blocksGameplay() &&
		flow.presenter().pageIndex() == retainedPage &&
		flow.presenter().pageCount() == retainedPages &&
		sources.maps > oldMaps && sources.objects > oldObjects &&
		assets.spriteLoadCount() > oldSpriteLoads &&
		world.isObjectDisabled(target) && !world.isObjectDisabled(sibling),
		"combined indoor cache reconstruction lost presentation or identity state");

	const XeenSaveResourceSignature signature{};
	const auto saved = XeenSaveFormat::decode(XeenSaveFormat::encode(
		XeenSaveState::capture(signature, party, camera, flags, world)));
	check(saved.disabledObjects == std::vector<XeenObjectIdentity>{target} &&
		saved.disabledEvents == std::vector<XeenEventIdentity>{
			{33,0},{33,1},{33,2}},
		"indoor Remove save contained the wrong authoritative identities");
	const auto suppressedRetry = flow.handle(InteractionAction{});
	const auto *suppressed = std::get_if<XeenManualEventCompleted>(&reports.back());
	check(suppressed && suppressed->instructionCount == 3 &&
		sources.scripts > oldScripts && suppressedRetry.pixels == postBase.pixels &&
		world.isObjectDisabled(target) && !world.isObjectDisabled(sibling),
		"cache-reconstructed Flow replayed removed behavior or lost retained-message lifetime");

	IndoorRemoveSources freshSources;
	XeenAssetSource freshAssets(installation, 320, 200);
	XeenWorld freshWorld(
		[&](XeenMapIdentity id) { return freshSources.loadMap(id); },
		[&](XeenMapIdentity id) { return freshSources.loadObjects(id); });
	auto freshParty = validEmptyParty();
	XeenCamera freshCamera{33,1,1,XeenDirection::West};
	XeenGameFlags freshFlags;
	XeenSaveState::Resources resources{
		signature, [] { return validEmptyParty(); },
		[&](XeenMapIdentity id) { return freshSources.loadEvents(id); }};
	XeenSaveState::restoreBeforeGameplay(saved, resources, freshParty, freshCamera,
		freshFlags, freshWorld,
		[&](XeenWorld &candidateWorld, const XeenPartyState &candidateParty,
				const XeenCamera &candidateCamera, const XeenGameFlags &) {
			check(composer.compose(freshAssets, candidateWorld, candidateParty,
				candidateCamera, context).isValid(),
				"fresh-owner indoor Remove restore failed visual preflight");
		});
	check(freshCamera.mapId == initialCamera.mapId && freshCamera.x == initialCamera.x &&
		freshCamera.y == initialCamera.y && freshCamera.direction == initialCamera.direction &&
		freshWorld.isObjectDisabled(target) && !freshWorld.isObjectDisabled(sibling),
		"fresh-owner restore lost indoor camera or object identity state");
	for (std::size_t record = 0; record < 3; ++record)
		check(freshWorld.isEventDisabled({33,record}),
			"fresh-owner restore resurrected an indoor physical-cell event");
	const auto freshResolver = XeenObjectVisualResolver::load(freshAssets);
	const auto freshCommands = XeenIndoorScene().build(freshWorld, freshCamera, &freshResolver);
	check(std::none_of(freshCommands.begin(), freshCommands.end(), [&](const auto &command) {
		return command.object() && command.object()->visual.identity == target;
	}) && targetCommand(freshCommands, sibling).object()->visual.identity == sibling,
		"derived indoor visuals resurrected the target after fresh-owner restore");
	const auto freshBase = composer.compose(
		freshAssets, freshWorld, freshParty, freshCamera, context);
	check(!changedPixels(freshBase,
		replay(freshAssets, composer, freshParty, context, freshCommands, sibling)).empty(),
		"fresh-owner restore lost the shared-resource sibling pixels");

	XeenEventSystem freshEvents(
		[&](XeenMapIdentity id) { return XeenEventScript(freshSources.loadEvents(id)); },
		[&](XeenMapIdentity id) { return freshSources.loadText(id); });
	std::vector<XeenManualEventResult> freshReports;
	XeenEventFlow freshFlow(freshWorld, freshEvents, freshParty, freshCamera, freshFlags, font,
		[&](std::uint64_t phase) {
			XeenEventFlow::Composition result;
			result.frame = composer.compose(freshAssets, freshWorld, freshParty,
				freshCamera, context, nullptr, phase, &result.containsOrdinaryAnimation);
			return result;
		});
	freshFlow.reportManual = [&](const auto &result) { freshReports.push_back(result); };
	check(freshFlow.frame().pixels == freshBase.pixels && !freshFlow.presentationGeneration(),
		"fresh Flow restored transient rendering or presentation state");
	const auto afterRetry = freshFlow.handle(InteractionAction{});
	const auto *retry = freshReports.empty() ? nullptr :
		std::get_if<XeenManualEventCompleted>(&freshReports.back());
	check(retry && retry->instructionCount == 3 && !freshFlow.blocksGameplay() &&
		afterRetry.pixels == freshBase.pixels && freshWorld.isObjectDisabled(target) &&
		!freshWorld.isObjectDisabled(sibling),
		"fresh interaction replayed removed indoor behavior or changed its sibling");
}

} // namespace

int main() {
	try {
		const auto directory = std::filesystem::temp_directory_path() /
			("mmodern-indoor-composer-" + std::to_string(
				std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::create_directories(directory);
		struct Cleanup {
			std::filesystem::path path;
			~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
		} cleanup{directory};
		const auto files = baseFiles();
		const auto installation = installationAt(directory, files, metadata());
		testAllPlacementPixels(installation);
		std::cout << "Indoor placement pixels passed\n";
		testOrderedWallCoverage(installation);
		auto opaqueFiles = files;
		opaqueFiles["stown.swl"] = repeatedFrames(solid(0, 160, 0, 80, 43), 48);
		const auto opaqueDirectory = directory / "opaque";
		std::filesystem::create_directories(opaqueDirectory);
		testAdmittedObjectFullyCovered(
			installationAt(opaqueDirectory, opaqueFiles, metadata()));
		testCompositionLifecycleAndStaticPhase(installation);
		std::cout << "Indoor composition lifecycle passed\n";
		testSelectedFrameFailuresAndRecovery(installation);
		std::cout << "Indoor selected-frame recovery passed\n";
		testCombinedIndoorRemoveLifecycle(installation);
		std::cout << "Indoor combined Remove lifecycle passed\n";

		GameInstallation noMetadata = installation;
		noMetadata.darkArchive.clear();
		{
			XeenAssetSource assets(noMetadata, 320, 200);
			auto world = worldWith(indoorMap(), objectFile(33, {{8,8,0,0,111}}));
			std::vector<XeenObjectVisual> diagnostics;
			bool animation = true;
			const auto geometryOnly = CloudsMapComposer().compose(
				assets, world, {}, {33,8,8,XeenDirection::North}, {},
				&diagnostics, 7, &animation);
			check(geometryOnly.isValid() && !animation && diagnostics.size() == 1 &&
				diagnostics[0].status == XeenObjectVisualStatus::MetadataUnavailable &&
				std::count(geometryOnly.pixels.begin(), geometryOnly.pixels.end(), 7) == 0,
				"missing indoor metadata changed geometry fallback semantics");
		}
		std::cout << "Indoor missing-metadata fallback passed\n";

		const auto malformedDirectory = directory / "malformed";
		std::filesystem::create_directories(malformedDirectory);
		const auto malformedInstallation = installationAt(malformedDirectory, files, Bytes(12));
		XeenAssetSource malformedAssets(malformedInstallation, 320, 200);
		auto malformedWorld = worldWith(indoorMap(), objectFile(33, {{8,8,0,0,111}}));
		bool animation = true;
		bool rejected = false;
		try {
			(void)CloudsMapComposer().compose(malformedAssets, malformedWorld, {},
				{33,8,8,XeenDirection::North}, {}, nullptr, 7, &animation);
		} catch (const std::runtime_error &) {
			rejected = true;
		}
		check(rejected && !animation, "malformed indoor metadata was hidden");
		std::cout << "Indoor composition, ordered occlusion, clipping and failure tests passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
