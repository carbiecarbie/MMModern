#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "compat/scummvm/ScummVmXeenBridge.h"

#include "compat/scummvm/ScummVmRuntime.h"
#include "formats/xeen/XeenObjectSpriteSafety.h"

#include "common/file.h"
#include "common/path.h"
#include "common/memstream.h"
#include "common/stream.h"
#include "mm/shared/xeen/cc_archive.h"
#include "mm/shared/xeen/sprites.h"
#include "mm/shared/xeen/xsurface.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace mmodern {
namespace {

using MM::Shared::Xeen::CCArchive;
using MM::Shared::Xeen::SpriteResource;
using MM::Shared::Xeen::XSurface;

// Read-only initial state, reconstructed as SaveArchive::reset does. No saves,
// Party, or engine are instantiated. BaseCCArchive remains the real index reader.
class InitialCloudsArchive final : public MM::Shared::Xeen::BaseCCArchive {
public:
	explicit InitialCloudsArchive(std::vector<std::uint8_t> data) : _data(std::move(data)) {
		if (_data.size() < 2)
			throw std::runtime_error("conteiner inicial de Clouds truncado");
		const std::size_t count = _data[0] | (static_cast<unsigned>(_data[1]) << 8);
		const std::size_t headerSize = 2 + count * 8;
		if (!count || headerSize > _data.size())
			throw std::runtime_error("indice inicial de Clouds invalido");
		// Validate the reserved byte before the upstream reader's assertion.
		for (std::size_t i = 7; i < count * 8; i += 8) {
			const unsigned b = _data[2 + i];
			if ((((b << 2) | (b >> 6)) + 0xac + i * 0x67) % 256 != 0)
				throw std::runtime_error("byte reservado invalido no indice inicial");
		}
		Common::MemoryReadStream stream(_data.data(), static_cast<uint32>(_data.size()));
		loadIndex(stream);
		for (const auto &entry : _index) {
			const auto offset = static_cast<std::size_t>(entry._offset);
			if (offset < headerSize || offset > _data.size() || entry._size > _data.size() - offset)
				throw std::runtime_error("recurso fora dos limites do conteiner inicial");
		}
	}

	Common::SeekableReadStream *createReadStreamForMember(const Common::Path &path) const override {
		MM::Shared::Xeen::CCEntry entry;
		if (!getHeaderEntry(path, entry))
			return nullptr;
		// The outer CCArchive already removed XOR 0x35. Inner payload is plaintext.
		return new Common::MemoryReadStream(_data.data() + entry._offset, entry._size);
	}

private:
	std::vector<std::uint8_t> _data;
};

std::vector<std::uint8_t> readBytes(Common::SeekableReadStream &stream,
		const std::string &name) {
	const auto length = stream.size();
	if (length < 0 || length > 65535)
		throw std::runtime_error("tamanho de recurso CC invalido: " + name);
	std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
	if (stream.read(bytes.data(), static_cast<uint32>(bytes.size())) != bytes.size() || stream.err())
		throw std::runtime_error("leitura incompleta: " + name);
	return bytes;
}

class StreamSpriteResource final : public SpriteResource {
public:
	bool loadFromStream(const Common::Path &filename, Common::SeekableReadStream &stream) {
		_filename = filename;
		SpriteResource::load(stream);
		return !empty() && !stream.err();
	}
};

// CCArchive's ordinary member reader terminates the process when an indexed
// payload is short. Keep its index/archive ownership, but make the one optional
// catalog member recoverable by checking the indexed extent before reading it.
class DarkMetadataArchive final : public CCArchive {
public:
	DarkMetadataArchive() :
		CCArchive(Common::Path("dark.cc", Common::Path::kNoSeparator), true) {
	}

	std::optional<std::vector<std::uint8_t>> readItemMaterialNamesChecked() const {
		return readMetadataChecked("mae.xen", 8192);
	}
	std::optional<std::vector<std::uint8_t>> readMonsterStatisticsChecked() const {
		return readMetadataChecked("xeen.mon", 65535, true);
	}

private:
	// Only the two explicit metadata reads above use this checked path.
	std::optional<std::vector<std::uint8_t>> readMetadataChecked(const char *name,
			std::size_t limit, bool requirePayloadOffset = false) const {
		MM::Shared::Xeen::CCEntry entry;
		const Common::Path member(name, Common::Path::kNoSeparator);
		const std::string origin = std::string("DARK.CC/") + name;
		if (!getHeaderEntry(member, entry))
			return std::nullopt;
		if (entry._offset < 0 || entry._size > limit)
			throw std::runtime_error("invalid " + origin + " index bounds");
		if (requirePayloadOffset && static_cast<std::uint64_t>(entry._offset) <
				2 + 8 * static_cast<std::uint64_t>(_index.size()))
			throw std::runtime_error("invalid " + origin + " payload overlaps archive index");

		Common::File file;
		const Common::Path archive("dark.cc", Common::Path::kNoSeparator);
		if (!file.open(archive))
			throw std::runtime_error("cannot reopen " + origin);
		const auto archiveSize = file.size();
		const auto offset = static_cast<std::uint64_t>(entry._offset);
		const auto size = static_cast<std::uint64_t>(entry._size);
		if (archiveSize < 0 || offset > static_cast<std::uint64_t>(archiveSize) ||
				size > static_cast<std::uint64_t>(archiveSize) - offset)
			throw std::runtime_error("truncated " + origin + " payload");
		if (!file.seek(entry._offset))
			throw std::runtime_error("cannot seek to " + origin + " payload");

		std::vector<std::uint8_t> bytes(entry._size);
		if (!bytes.empty() &&
				(file.read(bytes.data(), static_cast<uint32>(bytes.size())) != bytes.size() ||
				 file.err()))
			throw std::runtime_error("incomplete " + origin + " payload read");
		for (auto &byte : bytes)
			byte ^= 0x35;
		return bytes;
	}
};

std::unique_ptr<Common::SeekableReadStream> openResource(
		CCArchive &archive, const std::string &resourceName) {
	std::unique_ptr<Common::SeekableReadStream> stream(
		archive.createReadStreamForMember(Common::Path(resourceName.c_str(), Common::Path::kNoSeparator)));
	if (!stream)
		throw std::runtime_error("recurso ausente em xeen.cc: " + resourceName);
	return stream;
}

} // namespace

struct ScummVmXeenBridge::Impl {
	ScummVmRuntime runtime;
	CCArchive archive;
	bool darkAvailable = false;
	std::unique_ptr<DarkMetadataArchive> darkMetadataArchive;
	XSurface surface;
	std::array<std::uint8_t, IndexedFrame::kPaletteSize> palette{};
	struct CachedSprite {
		std::vector<std::uint8_t> bytes;
		std::unique_ptr<StreamSpriteResource> decoded;
	};
	std::unordered_map<std::string, CachedSprite> sprites;
	std::size_t spriteLoads = 0;
	std::unique_ptr<InitialCloudsArchive> initialArchive;

	explicit Impl(const GameInstallation &installation) :
		runtime(installation),
		archive(Common::Path("xeen.cc", Common::Path::kNoSeparator), true) {
		darkAvailable = installation.hasDarkside();
	}

	Impl(const GameInstallation &installation, int width, int height) : Impl(installation) {
		if (width <= 0 || height <= 0)
			throw std::runtime_error("dimensoes invalidas para o framebuffer");
		surface.create(width, height);
		surface.clear(0);
	}

	InitialCloudsArchive &initial() {
		if (!initialArchive) {
			// engines/mm/xeen/files.cpp: SaveArchive::reset, original order.
			const char *const blocks[] = {"2a0c", "2a1c", "2a2c", "2a3c", "284c", "2a5c"};
			std::vector<std::uint8_t> data;
			for (const char *name : blocks) {
				const Common::Path path(name, Common::Path::kNoSeparator);
				if (!archive.hasFile(path))
					continue;
				auto stream = openResource(archive, name);
				const auto block = readBytes(*stream, name);
				data.insert(data.end(), block.begin(), block.end());
			}
			initialArchive.reset(new InitialCloudsArchive(std::move(data)));
		}
		return *initialArchive;
	}

	StreamSpriteResource &sprite(const std::string &resourceName,
			std::optional<std::size_t> checkedFrame = std::nullopt, unsigned monsterFrames = 0) {
		const auto validate = [&](const std::vector<std::uint8_t> &bytes) {
			if (monsterFrames) {
				if (bytes.size() < 2 || bytes[0] != monsterFrames || bytes[1] != 0)
					throw std::runtime_error("Unexpected monster sprite frame count");
				for (std::size_t i = 0; i < monsterFrames; ++i) validateXeenObjectSprite(bytes, i);
			} else if (checkedFrame) validateXeenObjectSprite(bytes, *checkedFrame);
		};
		const auto existing = sprites.find(resourceName);
		if (existing != sprites.end()) {
			validate(existing->second.bytes);
			return *existing->second.decoded;
		}

		std::unique_ptr<Common::SeekableReadStream> stream = openResource(archive, resourceName);
		auto bytes = readBytes(*stream, resourceName);
		validate(bytes);
		Common::MemoryReadStream input(bytes.data(), static_cast<uint32>(bytes.size()));
		std::unique_ptr<StreamSpriteResource> resource(new StreamSpriteResource());
		const Common::Path path(resourceName.c_str(), Common::Path::kNoSeparator);
		if (!resource->loadFromStream(path, input))
			throw std::runtime_error("nao foi possivel decodificar: " + resourceName);

		StreamSpriteResource &result = *resource;
		sprites.emplace(resourceName, CachedSprite{std::move(bytes), std::move(resource)});
		++spriteLoads;
		return result;
	}
};

ScummVmXeenBridge::ScummVmXeenBridge(const GameInstallation &installation) :
	_impl(new Impl(installation)) {
}

ScummVmXeenBridge::ScummVmXeenBridge(const GameInstallation &installation,
		int width, int height) : _impl(new Impl(installation, width, height)) {
}

ScummVmXeenBridge::~ScummVmXeenBridge() = default;

void ScummVmXeenBridge::discardSpriteCache() { _impl->sprites.clear(); }
std::size_t ScummVmXeenBridge::cachedSpriteCount() const { return _impl->sprites.size(); }
std::size_t ScummVmXeenBridge::spriteLoadCount() const { return _impl->spriteLoads; }

void ScummVmXeenBridge::validateNormalMonster(const std::string &resourceName) {
	_impl->sprite(resourceName, std::nullopt, 8);
}
void ScummVmXeenBridge::validateAttackMonster(const std::string &resourceName) {
	_impl->sprite(resourceName, std::nullopt, 4);
}

std::optional<std::vector<std::uint8_t>> ScummVmXeenBridge::readCloudsVisualMetadataFromDarkArchive() {
	if (!_impl->darkAvailable) return std::nullopt;
	if (!_impl->darkMetadataArchive)
		_impl->darkMetadataArchive.reset(new DarkMetadataArchive());
	const Common::Path path("clouds.dat", Common::Path::kNoSeparator);
	std::unique_ptr<Common::SeekableReadStream> stream(_impl->darkMetadataArchive->createReadStreamForMember(path));
	if (!stream) return std::nullopt;
	return readBytes(*stream, "DARK.CC/clouds.dat");
}

std::optional<std::vector<std::uint8_t>> ScummVmXeenBridge::readItemMaterialNamesFromDarkArchive() {
	if (!_impl->darkAvailable)
		return std::nullopt;
	if (!_impl->darkMetadataArchive)
		_impl->darkMetadataArchive.reset(new DarkMetadataArchive());
	return _impl->darkMetadataArchive->readItemMaterialNamesChecked();
}

std::optional<std::vector<std::uint8_t>> ScummVmXeenBridge::readCloudsMonsterStatisticsFromDarkArchive() {
	if (!_impl->darkAvailable) return std::nullopt;
	if (!_impl->darkMetadataArchive)
		_impl->darkMetadataArchive.reset(new DarkMetadataArchive());
	return _impl->darkMetadataArchive->readMonsterStatisticsChecked();
}

void ScummVmXeenBridge::drawObjectSprite(const std::string &resourceName,
		std::size_t frame, int x, int y, const XeenSpriteDrawOptions &options) {
	// M16 uses the native framebuffer and normal drawer. In particular, the
	// upstream enlarge path writes a second pixel/row without edge checks.
	if (_impl->surface.w != 320 || _impl->surface.h != 200 || options.enlarge ||
		x < -320 || x > 320 || y < -200 || y > 200 ||
		options.scaleIndex < 0 || options.scaleIndex > 15)
		throw std::runtime_error("M16 sprite: unsupported framebuffer, anchor, or scale/enlargement");
	try {
		_impl->sprite(resourceName, frame);
	} catch (const std::runtime_error &error) {
		throw std::runtime_error(resourceName + ": " + error.what());
	}
	drawSprite(resourceName, frame, x, y, options);
}

bool ScummVmXeenBridge::hasArchiveResource(const std::string &resourceName) {
	return _impl->archive.hasFile(
		Common::Path(resourceName.c_str(), Common::Path::kNoSeparator));
}

std::vector<std::uint8_t> ScummVmXeenBridge::readArchiveResource(const std::string &resourceName) {
	auto stream = openResource(_impl->archive, resourceName);
	return readBytes(*stream, resourceName);
}

bool ScummVmXeenBridge::hasInitialResource(const std::string &resourceName) {
	return _impl->initial().hasFile(
		Common::Path(resourceName.c_str(), Common::Path::kNoSeparator));
}

std::vector<std::uint8_t> ScummVmXeenBridge::readInitialResource(const std::string &resourceName) {
	std::unique_ptr<Common::SeekableReadStream> stream(_impl->initial().createReadStreamForMember(
		Common::Path(resourceName.c_str(), Common::Path::kNoSeparator)));
	if (!stream)
		throw std::runtime_error("recurso ausente no conteiner inicial: " + resourceName);
	return readBytes(*stream, resourceName);
}

void ScummVmXeenBridge::loadPalette(const std::string &resourceName) {
	std::unique_ptr<Common::SeekableReadStream> stream = openResource(_impl->archive, resourceName);
	if (stream->size() != static_cast<int64>(IndexedFrame::kPaletteSize))
		throw std::runtime_error("paleta com tamanho invalido: " + resourceName);

	if (stream->read(_impl->palette.data(), static_cast<uint32>(_impl->palette.size())) !=
			_impl->palette.size())
		throw std::runtime_error("leitura incompleta da paleta: " + resourceName);

	for (std::uint8_t &component : _impl->palette)
		component = static_cast<std::uint8_t>(component << 2);
}

void ScummVmXeenBridge::loadRawFramebuffer(const std::string &resourceName) {
	std::unique_ptr<Common::SeekableReadStream> stream = openResource(_impl->archive, resourceName);
	const uint32 expectedSize = static_cast<uint32>(_impl->surface.w * _impl->surface.h);
	if (stream->size() != expectedSize)
		throw std::runtime_error("framebuffer RAW com tamanho invalido: " + resourceName);

	for (int y = 0; y < _impl->surface.h; ++y) {
		if (stream->read(_impl->surface.getBasePtr(0, y), _impl->surface.w) !=
				static_cast<uint32>(_impl->surface.w))
			throw std::runtime_error("leitura incompleta do framebuffer: " + resourceName);
	}
}

void ScummVmXeenBridge::drawSprite(const std::string &resourceName,
		std::size_t frame, int x, int y) {
	drawSprite(resourceName, frame, x, y, XeenSpriteDrawOptions{});
}

void ScummVmXeenBridge::drawSprite(const std::string &resourceName,
		std::size_t frame, int x, int y, const XeenSpriteDrawOptions &options) {
	StreamSpriteResource &sprite = _impl->sprite(resourceName);
	if (frame >= sprite.size())
		throw std::runtime_error("quadro inexistente em " + resourceName);

	if (options.scaleIndex < 0 || options.scaleIndex > 15)
		throw std::runtime_error("indice de reducao de sprite invalido");
	uint flags = 0;
	if (options.horizontalFlip)
		flags |= MM::Shared::Xeen::SPRFLAG_HORIZ_FLIPPED;
	if (options.sceneClipped)
		flags |= MM::Shared::Xeen::SPRFLAG_SCENE_CLIPPED;
	if (options.bottomClipped) {
		SpriteResource::setClippedBottom(140);
		flags |= MM::Shared::Xeen::SPRFLAG_BOTTOM_CLIPPED;
	}
	const int scale = options.enlarge ? MM::Shared::Xeen::SCALE_ENLARGE : options.scaleIndex;
	sprite.draw(_impl->surface, static_cast<int>(frame), Common::Point(x, y), flags, scale);
}

IndexedFrame ScummVmXeenBridge::snapshot() const {
	IndexedFrame frame;
	frame.width = _impl->surface.w;
	frame.height = _impl->surface.h;
	frame.palette = _impl->palette;
	frame.pixels.resize(static_cast<std::size_t>(frame.width) * frame.height);

	for (int y = 0; y < frame.height; ++y) {
		const std::uint8_t *source = static_cast<const std::uint8_t *>(
			_impl->surface.getBasePtr(0, y));
		std::copy(source, source + frame.width,
			frame.pixels.begin() + static_cast<std::size_t>(y) * frame.width);
	}

	return frame;
}

void ScummVmXeenBridge::drawNpc(IndexedFrame &frame, std::uint8_t portraitId,
		std::size_t portraitFrame) {
	if (!frame.isValid() || frame.width != 320 || frame.height != 200 || portraitFrame >= 4)
		throw std::invalid_argument("invalid NPC draw context");
	const std::string face = "face" + (portraitId < 10 ? std::string("0") : std::string()) +
		std::to_string(portraitId) + ".fac";
	// Validate every usable frame before any pixels are applied. Shared cells and
	// optional second cells remain decoded/drawn by the pinned SpriteResource.
	for (std::size_t i = 0; i < 4; ++i) _impl->sprite(face, i);
	auto &border = _impl->sprite("frame.fac", 0);
	auto &portrait = _impl->sprite(face, portraitFrame);
	XSurface surface;
	surface.create(320, 200);
	for (int y = 0; y < 200; ++y)
		std::copy_n(frame.pixels.data() + y * 320, 320,
			static_cast<std::uint8_t *>(surface.getBasePtr(0, y)));
	border.draw(surface, 0, Common::Point(16, 16));
	portrait.draw(surface, static_cast<int>(portraitFrame), Common::Point(23, 22));
	for (int y = 0; y < 200; ++y)
		std::copy_n(static_cast<const std::uint8_t *>(surface.getBasePtr(0, y)), 320,
			frame.pixels.data() + y * 320);
}

} // namespace mmodern
