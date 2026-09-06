#include "formats/xeen/XeenMapFormat.h"

#include <iostream>
#include <stdexcept>

using namespace mmodern;
using Bytes = std::vector<std::uint8_t>;

namespace {
void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

template<class Function> void rejects(Function function) {
	bool rejected = false;
	try { function(); } catch (const std::runtime_error &) { rejected = true; }
	check(rejected, "malformed input was accepted");
}

void testGeometry() {
	Bytes dat(892, 0);
	// Independent byte fixture: asymmetric wall nibbles catch reversed directions.
	dat[0] = 0x34; dat[1] = 0x12;
	dat[512] = 0xfd;
	dat[768] = 1;
	dat[770] = 0x23; dat[771] = 0x01;
	dat[772] = 5; dat[774] = 2;
	dat[821] = 0xff; dat[822] = 0x80; dat[823] = 0x7f;
	dat[828] = 0x81; dat[829] = 0x01;
	dat[860] = 0x02;
	auto map = XeenMapFormat::parseDat(dat);
	check(!map.isOutdoors() && map.id == 1 && map.cells.size() == 256, "DAT header");
	check(map.neighbors == std::array<std::uint16_t, 4>{291, 5, 2, 0}, "neighbors / endian");
	check(std::get<XeenIndoorWalls>(map.cells[0].geometry).walls ==
		std::array<std::uint8_t, 4>{1, 2, 3, 4}, "indoor directions");
	check(wallAt(map.cells[0], XeenDirection::North) == 1 &&
		wallAt(map.cells[0], XeenDirection::East) == 2 &&
		wallAt(map.cells[0], XeenDirection::South) == 3 &&
		wallAt(map.cells[0], XeenDirection::West) == 4, "wallAt directions");
	check(map.cells[0].surfaceIndex == 5 && map.cells[0].flags == 0xf8, "indoor attributes");
	check(map.difficulties[5] == -1 && map.difficulties[6] == -128 &&
		map.difficulties[7] == 127, "signed metadata");
	check(map.cells[0].seen && !map.cells[1].seen && map.cells[7].seen &&
		map.cells[8].seen && map.cells[1].stepped, "bitmaps / bit order");
	dat[781] = 0x80;
	dat[480] = 0x0e; // (0,15), independently different from (0,0).
	map = XeenMapFormat::parseDat(dat);
	const auto layers = std::get<XeenOutdoorLayers>(map.cells[0].geometry);
	check(map.isOutdoors() && layers.surface == 4 && layers.middle == 3 &&
		layers.top == 2 && layers.overlay == 1, "outdoor layers");
	check(map.cells[0].surfaceIndex == 4 && map.cells[240].surfaceIndex == 14, "outdoor Y ordering");
	for (std::size_t size = 0; size < dat.size(); ++size)
		rejects([&] { XeenMapFormat::parseDat(Bytes(dat.begin(), dat.begin() + size)); });
	dat.push_back(0);
	rejects([&] { XeenMapFormat::parseDat(dat); });
}

Bytes mobFixture() {
	Bytes mob(48, 0xff);
	mob[0] = 5; mob[2] = 9; // Preserve object hole at slot 1.
	mob[16] = 56; mob[18] = 19; // Monster slot 1 addresses compacted ID 19.
	const Bytes lists = {
		0x80, 2, 2, 3, // Disabled object; signed X must stay -128.
		3, 4, 0, 1,    // Active object.
		0xff, 0xff, 0xff, 0xff,
		7, 0x80, 1, 0, // Disabled monster; signed Y must stay -128.
		0xff, 0xff, 0xff, 0xff,
		99, 99, 0, 0, // Unresolved wall placeholder, as in the real Area A1.
		0xff, 0xff, 0xff, 0xff
	};
	mob.insert(mob.end(), lists.begin(), lists.end());
	return mob;
}

void testEntities() {
	auto mob = mobFixture();
	const auto entities = XeenMapFormat::parseMob(mob);
	check(entities.objects.size() == 2 && entities.monsters.size() == 1 &&
		entities.wallItems.size() == 1, "MOB lists");
	check(entities.objects[0].x == -128 && entities.objects[0].isDisabled() &&
		entities.objects[0].resourceId == 9 && entities.objects[1].isActive(), "object slots / disabled");
	check(entities.monsters[0].resourceId == 19 && !entities.monsters[0].isActive(), "compact monster slots");
	check(!entities.wallItems[0].hasResource() && !entities.wallItems[0].isActive() &&
		!entities.wallItems[0].isDisabled(), "wall placeholder must not become an active sprite");
	for (std::size_t size = 0; size < mob.size(); ++size)
		rejects([&] { XeenMapFormat::parseMob(Bytes(mob.begin(), mob.begin() + size)); });
	mob.push_back(0);
	rejects([&] { XeenMapFormat::parseMob(mob); });
	mob = mobFixture(); mob[50] = 16;
	rejects([&] { XeenMapFormat::parseMob(mob); });
	// Two empty lists (two markers each) followed by an empty wall list (one).
	const auto empty = XeenMapFormat::parseMob(Bytes(48 + 5 * 4, 0xff));
	check(empty.objects.empty() && empty.monsters.empty() && empty.wallItems.empty(), "double markers");
}

void testInstructions() {
	const Bytes evt = {7, 1, 2, 4, 0, 0xfa, 0x35, 0x80, 5, 1, 2, 4, 1, 0xfb};
	const auto instructions = XeenMapFormat::parseEvt(evt);
	check(instructions.size() == 2 && instructions[0].parameters == Bytes{0x35, 0x80} &&
		instructions[1].parameters.empty(), "variable records / opaque parameters");
	check(instructions[0].opcode == 0xfa && instructions[1].line == 1, "instruction fields");
	check(XeenMapFormat::parseEvt({}).empty(), "empty EVT");
	rejects([] { XeenMapFormat::parseEvt({4, 0, 0, 0, 0}); });
	rejects([] { XeenMapFormat::parseEvt({255, 0, 0, 0, 0, 0}); });
	for (std::size_t size = 1; size < evt.size(); ++size) {
		if (size != 8) // A complete first record is a valid one-instruction EVT.
			rejects([&] { XeenMapFormat::parseEvt(Bytes(evt.begin(), evt.begin() + size)); });
	}
}
} // namespace

int main() {
	try {
		testGeometry();
		testEntities();
		testInstructions();
		std::cout << "DAT/MOB/EVT: geometry, sentinels, disabled entities, placeholders and bounds OK\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
