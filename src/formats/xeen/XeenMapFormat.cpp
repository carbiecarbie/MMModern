#include "formats/xeen/XeenMapFormat.h"

#include "formats/xeen/XeenEventFormat.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace mmodern {
namespace {

class Reader {
public:
	Reader(const std::vector<std::uint8_t> &bytes, const char *format) :
		_bytes(bytes), _format(format) {}

	std::size_t remaining() const { return _bytes.size() - _offset; }
	[[noreturn]] void fail(const char *message) const {
		throw std::runtime_error(std::string(_format) + " no offset " +
			std::to_string(_offset) + ": " + message);
	}
	std::uint8_t byte() {
		if (!remaining())
			fail("arquivo truncado");
		return _bytes[_offset++];
	}
	int signedByte() {
		const int value = byte();
		return value < 128 ? value : value - 256;
	}
	std::uint16_t word() {
		const auto low = byte();
		return static_cast<std::uint16_t>(low | (static_cast<unsigned>(byte()) << 8));
	}
	void requireEnd() const {
		if (remaining())
			fail("bytes excedentes");
	}

private:
	const std::vector<std::uint8_t> &_bytes;
	const char *_format;
	std::size_t _offset = 0;
};

XeenMapEntity readEntity(Reader &reader) {
	XeenMapEntity entity;
	entity.x = reader.signedByte();
	entity.y = reader.signedByte();
	entity.tableIndex = reader.byte();
	entity.direction = reader.byte();
	return entity;
}

bool isTerminator(const XeenMapEntity &entity) {
	// Original list loaders test X and ID, not the unused Y/direction bytes.
	return entity.x == -1 && entity.tableIndex == 0xff;
}

std::vector<XeenMapEntity> readEntities(Reader &reader,
		const std::vector<int> &table, bool emptyHasDummy) {
	std::vector<XeenMapEntity> result;
	XeenMapEntity entity = readEntity(reader);
	if (isTerminator(entity)) {
		// Objects and monsters encode an empty list as dummy + terminator.
		if (emptyHasDummy && !isTerminator(readEntity(reader)))
			reader.fail("lista vazia sem segundo terminador");
		return result;
	}
	while (!isTerminator(entity)) {
		if (entity.tableIndex >= 16)
			reader.fail("indice de entidade fora dos 16 slots");
		// Area A1 has a wall-item placeholder at (99,99) with an empty table.
		// The engine skips unresolved wall/monster entries. Preserve raw records
		// for diagnostics, but do not manufacture a sprite/type or mark them active.
		entity.resourceId = entity.tableIndex < table.size() ? table[entity.tableIndex] : -1;
		result.push_back(entity); // Keep order and disabled records for script indices.
		entity = readEntity(reader);
	}
	return result;
}

std::vector<int> resolveTable(const std::array<std::uint8_t, 16> &raw, bool compact) {
	std::vector<int> table;
	for (const auto id : raw) {
		if (id != 0xff || !compact)
			table.push_back(id == 0xff ? -1 : id);
	}
	return table;
}

} // namespace

XeenMapGeometry XeenMapFormat::parseDat(const std::vector<std::uint8_t> &bytes) {
	// engines/mm/xeen/map.cpp: MazeData::synchronize (892 bytes).
	Reader reader(bytes, "maze.dat");
	if (bytes.size() != 892)
		reader.fail("tamanho esperado: 892 bytes");
	XeenMapGeometry map;
	for (auto &cell : map.cells)
		cell.rawWord = reader.word();
	for (auto &cell : map.cells) {
		cell.rawAttributes = reader.byte();
		cell.flags = cell.rawAttributes & 0xf8;
	}
	map.id = reader.word();
	for (auto &neighbor : map.neighbors)
		neighbor = reader.word();
	map.flags = reader.word();
	map.flags2 = reader.word();
	for (auto &type : map.wallTypes)
		type = reader.byte();
	for (auto &type : map.surfaceTypes)
		type = reader.byte();
	map.floorType = reader.byte();
	map.runX = reader.byte();
	for (std::size_t i = 0; i < map.difficulties.size(); ++i)
		map.difficulties[i] = i < 5 ? reader.byte() : reader.signedByte();
	map.runY = reader.byte();
	map.trapDamage = reader.byte();
	map.wallKind = reader.byte();
	map.tavernTips = reader.byte();
	for (int set = 0; set < 2; ++set) {
		for (std::size_t i = 0; i < map.cells.size(); i += 8) {
			const auto bits = reader.byte(); // File::syncBitFlags: least significant bit first.
			for (unsigned bit = 0; bit < 8; ++bit) {
				const bool value = (bits & (1U << bit)) != 0;
				if (set == 0)
					map.cells[i + bit].seen = value;
				else
					map.cells[i + bit].stepped = value;
			}
		}
	}
	reader.requireEnd();
	for (auto &cell : map.cells) {
		const auto nibble = [&cell](unsigned shift) {
			return static_cast<std::uint8_t>((cell.rawWord >> shift) & 0xf);
		};
		if (map.isOutdoors()) {
			cell.geometry = XeenOutdoorLayers{nibble(0), nibble(4), nibble(8), nibble(12)};
			cell.surfaceIndex = nibble(0);
		} else {
			// Runtime WALL_SHIFTS[direction][2] and Map::setWall, not C++ bitfield order.
			cell.geometry = XeenIndoorWalls{{nibble(12), nibble(8), nibble(4), nibble(0)}};
			cell.surfaceIndex = cell.rawAttributes & 7;
		}
	}
	return map;
}

XeenMapEntities XeenMapFormat::parseMob(const std::vector<std::uint8_t> &bytes) {
	// engines/mm/xeen/map.cpp: MonsterObjectData::synchronize / MobStruct.
	Reader reader(bytes, "maze.mob");
	XeenMapEntities entities;
	for (auto &id : entities.objectTable)
		id = reader.byte();
	for (auto &id : entities.monsterTable)
		id = reader.byte();
	for (auto &id : entities.wallItemTable)
		id = reader.byte();
	entities.objects = readEntities(reader, resolveTable(entities.objectTable, false), true);
	entities.monsters = readEntities(reader, resolveTable(entities.monsterTable, true), true);
	entities.wallItems = readEntities(reader, resolveTable(entities.wallItemTable, true), false);
	reader.requireEnd();
	return entities;
}

std::vector<XeenEventInstruction> XeenMapFormat::parseEvt(const std::vector<std::uint8_t> &bytes) {
	std::vector<XeenEventInstruction> instructions;
	for (auto &record : XeenEventFormat::parse(bytes)) {
		XeenEventInstruction instruction;
		instruction.x = record.x;
		instruction.y = record.y;
		instruction.direction = record.direction;
		instruction.line = record.line;
		instruction.opcode = record.opcode;
		instruction.parameters = std::move(record.parameters);
		instructions.push_back(std::move(instruction));
	}
	return instructions;
}

} // namespace mmodern
