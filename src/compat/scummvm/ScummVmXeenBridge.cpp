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
#include <zlib.h>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <random>
#include <fstream>

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
	// Adapted from the pinned sprites.cpp drawPixel2/3/5 helpers (ScummVM
	// developers, GPL-3.0-or-later). Those upstream modes require g_engine or
	// g_system. Reuse the native decoder/scaler for each component and apply
	// the same pixel operations with the bridge palette and cosmetic sampler.
	void drawEngineFreeEffect(XSurface &dest,int frame,const Common::Point &position,
			uint flags,int scale,const std::array<std::uint8_t,768> &palette,std::uint32_t seed) {
		const unsigned mode=flags&0xf00, index=flags&0x1f;
		if(index>3 || (mode!=0x200&&mode!=0x300&&mode!=0x500))
			throw std::invalid_argument("Invalid engine-free monster effect");
		XSurface black(dest.w,dest.h),white(dest.w,dest.h);
		std::mt19937 random(seed);
		const auto original1=_index[frame]._offset1,original2=_index[frame]._offset2;
		struct Restore {IndexEntry &entry;uint16 first,second;~Restore(){entry._offset1=first;entry._offset2=second;}} restore{_index[frame],original1,original2};
		const bool hasPalette=std::any_of(palette.begin(),palette.end(),[](auto c){return c!=0;});
		const unsigned components=original2&&_index[frame]._override.empty()?2:1;
		for(unsigned component=0;component<components;++component) {
			_index[frame]._offset1=component?original2:original1;_index[frame]._offset2=0;
			for(int y=0;y<dest.h;++y) {
				std::fill_n(static_cast<byte *>(black.getBasePtr(0,y)),dest.w,0);
				std::fill_n(static_cast<byte *>(white.getBasePtr(0,y)),dest.w,255);
			}
			SpriteResource::draw(black,frame,position,flags&~0xfff,scale);
			SpriteResource::draw(white,frame,position,flags&~0xfff,scale);
			for(int y=0;y<dest.h;++y)for(int x=0;x<dest.w;++x) {
				const auto pixel=*static_cast<byte *>(black.getBasePtr(x,y));
				if(pixel!=*static_cast<byte *>(white.getBasePtr(x,y)))continue;
				auto *target=static_cast<byte *>(dest.getBasePtr(x,y));
				if(mode==0x300) {
					if(!hasPalette)continue;
					constexpr unsigned masks[]{1,3,7,15},offsets[]{1,2,4,8};
					const byte level=(pixel&masks[index])-offsets[index]+(*target&15);
					if(level>=0x80)*target&=0xf0;
					else if(level<=15)*target=(*target&0xf0)|level;
					else *target|=15;
					while(*target<255&&!palette[*target*3]&&!palette[*target*3+1]&&!palette[*target*3+2])++*target;
					continue;
				}
				uint16 r1=std::uniform_int_distribution<unsigned>(0,65535)(random),r2=std::uniform_int_distribution<unsigned>(0,65535)(random);
				bool carry=(r1&0x8000)!=0;
				r1=std::uint16_t((std::uint16_t(r1<<1))-r2-(carry?1:0));
				bool next=r2&1;r2=(r2>>1)|(carry?0x8000:0);carry=next;
				r2=(r2>>1)|(carry?0x8000:0);r2^=r1;
				if(mode==0x500) {
					constexpr unsigned thresholds[]{0x3333,0x6666,0x999a,0xcccd};
					if(r2>thresholds[index])*target=pixel;
				} else {
					constexpr int delta[]{-3,3,0,0,0,0,0,0,-5,5,0,0,0,0,0,0,-7,7,0,0,0,0,0,0,-9,9,0,0,0,0,0,0,
						-7,7,0,0,0,0,0,0,-9,9,0,0,0,0,0,0,-11,11,0,0,0,0,0,0,-13,13,0,0,0,0,0,0};
					constexpr unsigned masks1[]{3,0,3,0},masks2[]{0x7e,0x7e,0x7e,0x7e};
					const int shiftedX=x+delta[(r2&masks1[index]&masks2[index])/2];
					const int left=(flags&MM::Shared::Xeen::SPRFLAG_SCENE_CLIPPED)?8:0;
					const int right=(flags&MM::Shared::Xeen::SPRFLAG_SCENE_CLIPPED)?223:dest.w;
					if(shiftedX<left||shiftedX>=right)continue;
					const int shiftedY=y+delta[((r2>>8)&masks1[index]&masks2[index])/2];
					const int bottom=(flags&MM::Shared::Xeen::SPRFLAG_BOTTOM_CLIPPED)?140:dest.h;
					if(shiftedY>=0&&shiftedY<bottom)*static_cast<byte *>(dest.getBasePtr(shiftedX,shiftedY))=pixel;
				}
			}
		}
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
	std::optional<std::vector<std::uint8_t>> readLearnedSpellNamesChecked() const {
		return readMetadataChecked("spells.xen", 937, true);
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

// Presentation-only companion archive. Use the pinned BaseCCArchive index and
// name hashing, and reopen members for every cache reconstruction so the same
// admitted-byte integrity checks apply to all physical archives.
class SceneInstalledArchive final : public MM::Shared::Xeen::BaseCCArchive {
public:
	explicit SceneInstalledArchive(std::filesystem::path path) : _path(std::move(path)) {
		std::ifstream file(_path, std::ios::binary);
		std::vector<std::uint8_t> index(2);
		if (!file.read(reinterpret_cast<char *>(index.data()), 2))
			throw std::runtime_error("Truncated scene archive index: " + _path.string());
		const unsigned count = index[0] | (unsigned(index[1]) << 8);
		index.resize(2 + count * 8);
		if (!file.read(reinterpret_cast<char *>(index.data() + 2), count * 8))
			throw std::runtime_error("Truncated scene archive index: " + _path.string());
		for (std::size_t i = 7; i < count * 8; i += 8) {
			const unsigned b = index[2 + i];
			if ((((b << 2) | (b >> 6)) + 0xac + i * 0x67) % 256 != 0)
				throw std::runtime_error("Invalid scene archive index: " + _path.string());
		}
		Common::MemoryReadStream stream(index.data(), static_cast<uint32>(index.size()));
		loadIndex(stream);
	}

	Common::SeekableReadStream *createReadStreamForMember(const Common::Path &path) const override {
		MM::Shared::Xeen::CCEntry entry;
		if (!getHeaderEntry(path, entry)) return nullptr;
		std::ifstream file(_path, std::ios::binary | std::ios::ate);
		const auto length = file.tellg();
		if (!file || length < 0 || entry._offset < 2 + 8 * static_cast<int64>(_index.size()) ||
			static_cast<int64>(entry._offset) + entry._size > length)
			throw std::runtime_error("Invalid scene archive member extent: " + _path.string());
		file.seekg(entry._offset);
		std::vector<std::uint8_t> bytes(entry._size);
		if (!file.read(reinterpret_cast<char *>(bytes.data()), bytes.size()))
			throw std::runtime_error("Incomplete scene archive member read: " + _path.string());
		for (auto &byte : bytes) byte ^= 0x35;
		Common::MemoryReadStream input(bytes.data(), static_cast<uint32>(bytes.size()));
		return input.readStream(bytes.size());
	}

private:
	std::filesystem::path _path;
};

std::filesystem::path introArchivePath(const GameInstallation &installation) {
	const auto root = installation.root.empty() ? installation.xeenArchive.parent_path() : installation.root;
	std::error_code error;
	for (const auto &entry : std::filesystem::directory_iterator(root, error)) {
		if (error) break;
		auto name = entry.path().filename().string();
		for (auto &c : name) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
		if (name == "intro.cc" && entry.is_regular_file(error)) return entry.path();
	}
	return {};
}

std::unique_ptr<Common::SeekableReadStream> openResource(
		Common::Archive &archive, const std::string &resourceName, const std::string &origin = "xeen.cc") {
	std::unique_ptr<Common::SeekableReadStream> stream(
		archive.createReadStreamForMember(Common::Path(resourceName.c_str(), Common::Path::kNoSeparator)));
	if (!stream)
		throw std::runtime_error("Missing resource in " + origin + ": " + resourceName);
	return stream;
}

} // namespace

struct ScummVmXeenBridge::Impl {
	ScummVmRuntime runtime;
	CCArchive archive;
	bool darkAvailable = false;
	std::unique_ptr<DarkMetadataArchive> darkMetadataArchive;
	std::filesystem::path sceneIntroPath, sceneDarkPath;
	std::unique_ptr<SceneInstalledArchive> sceneIntro, sceneDark;
	XSurface surface;
	std::array<std::uint8_t, IndexedFrame::kPaletteSize> palette{};
	struct CachedSprite {
		std::vector<std::uint8_t> bytes;
		std::unique_ptr<StreamSpriteResource> decoded;
	};
	std::unordered_map<std::string, CachedSprite> sprites;
	std::size_t spriteLoads = 0;
	std::unordered_map<std::string,std::vector<std::uint8_t>> admittedSprites;
	bool spriteIntegrityFailed=false;
	std::unique_ptr<InitialCloudsArchive> initialArchive;

	explicit Impl(const GameInstallation &installation) :
		runtime(installation),
		archive(Common::Path("xeen.cc", Common::Path::kNoSeparator), true) {
		darkAvailable = installation.hasDarkside();
		sceneIntroPath = introArchivePath(installation);
		sceneDarkPath = installation.darkArchive;
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

	struct SceneResource {
		Common::Archive *archive;
		std::string origin;
	};
	SceneResource sceneResource(const std::string &name) {
		const Common::Path path(name.c_str(), Common::Path::kNoSeparator);
		// Pinned File::open/exists: current archive first, then FileManager's
		// registered INTRO fallback. The installed World of Xeen companion DARK
		// is searched last for shared presentation assets, as authorized in M48.
		if (archive.hasFile(path)) return {&archive, "xeen.cc"};
		if (!sceneIntroPath.empty()) {
			if (!sceneIntro) sceneIntro.reset(new SceneInstalledArchive(sceneIntroPath));
			if (sceneIntro->hasFile(path)) return {sceneIntro.get(), "intro.cc"};
		}
		if (!sceneDarkPath.empty()) {
			if (!sceneDark) sceneDark.reset(new SceneInstalledArchive(sceneDarkPath));
			if (sceneDark->hasFile(path)) return {sceneDark.get(), "dark.cc"};
		}
		return {nullptr, {}};
	}

	StreamSpriteResource &sprite(const std::string &resourceName,
			std::optional<std::size_t> checkedFrame = std::nullopt, unsigned monsterFrames = 0,
			bool sceneLookup = false) {
		if(spriteIntegrityFailed) throw std::runtime_error("Sprite resource integrity previously failed");
		const auto source = sceneLookup ? sceneResource(resourceName) : SceneResource{&archive, "xeen.cc"};
		if (!source.archive) throw std::runtime_error("Missing scene resource in installed CC archives: " + resourceName);
		// Clouds retains its existing identity; other archives have separate
		// cache/admission identities even when resource names collide.
		const auto key = source.origin == "xeen.cc" ? resourceName : source.origin + "|" + resourceName;
		const auto validate = [&](const std::vector<std::uint8_t> &bytes) {
			const auto known=admittedSprites.find(key);
			if(known!=admittedSprites.end() && known->second!=bytes) {
				spriteIntegrityFailed=true;throw std::runtime_error("Admitted sprite resource changed: "+resourceName);
			}
			if (monsterFrames) {
				if (bytes.size() < 2 || bytes[0] != monsterFrames || bytes[1] != 0)
					throw std::runtime_error("Unexpected monster sprite frame count");
				for (std::size_t i = 0; i < monsterFrames; ++i) validateXeenObjectSprite(bytes, i);
			} else if (checkedFrame) validateXeenObjectSprite(bytes, *checkedFrame);
		};
		const auto existing = sprites.find(key);
		if (existing != sprites.end()) {
			validate(existing->second.bytes);
			return *existing->second.decoded;
		}

		std::unique_ptr<Common::SeekableReadStream> stream = openResource(*source.archive, resourceName, source.origin);
		auto bytes = readBytes(*stream, resourceName);
		validate(bytes);
		Common::MemoryReadStream input(bytes.data(), static_cast<uint32>(bytes.size()));
		std::unique_ptr<StreamSpriteResource> resource(new StreamSpriteResource());
		const Common::Path path(resourceName.c_str(), Common::Path::kNoSeparator);
		if (!resource->loadFromStream(path, input))
			throw std::runtime_error("Unable to decode sprite: " + resourceName);

		admittedSprites.emplace(key,bytes);
		StreamSpriteResource &result = *resource;
		sprites.emplace(key, CachedSprite{std::move(bytes), std::move(resource)});
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

std::size_t ScummVmXeenBridge::spriteFrameCount(const std::string &name) {
	const auto count=_impl->sprite(name,std::nullopt,0,true).size();
	if (!count) throw std::runtime_error("Empty sprite: "+name);
	for (std::size_t frame=0;frame<count;++frame) _impl->sprite(name,frame,0,true);
	return count;
}

bool ScummVmXeenBridge::hasSceneResource(const std::string &name) {
	return _impl->sceneResource(name).archive != nullptr;
}

void ScummVmXeenBridge::drawSceneSprite(const std::string &name, std::size_t frame,
		int x, int y, const XeenSpriteDrawOptions &options) {
	drawSpriteImpl(name, frame, x, y, options, true);
}

void ScummVmXeenBridge::validateProjectile(const std::string &name) { _impl->sprite(name,std::nullopt,3); }

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

std::optional<std::vector<std::uint8_t>> ScummVmXeenBridge::readLearnedSpellNamesFromDarkArchive() {
	if (!_impl->darkAvailable) return std::nullopt;
	if (!_impl->darkMetadataArchive)
		_impl->darkMetadataArchive.reset(new DarkMetadataArchive());
	return _impl->darkMetadataArchive->readLearnedSpellNamesChecked();
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
	drawSpriteImpl(resourceName, frame, x, y, options, false);
}

void ScummVmXeenBridge::drawSpriteImpl(const std::string &resourceName,
		std::size_t frame, int x, int y, const XeenSpriteDrawOptions &options, bool sceneLookup) {
	StreamSpriteResource &sprite = _impl->sprite(resourceName, std::nullopt, 0, sceneLookup);
	if (frame >= sprite.size())
		throw std::runtime_error("Missing sprite frame in " + resourceName);

	if (options.scaleIndex < 0 || options.scaleIndex > 15)
		throw std::runtime_error("Invalid sprite scale index");
	const unsigned effect=options.monsterEffectFlags, drawer=effect>>8, index=effect&255;
	if (drawer>6 || (drawer==0 && index!=0) ||
		(drawer==1 && index>23) || (drawer>=2 && drawer<=5 && index>3) || (drawer==6 && index>15))
		throw std::runtime_error("Invalid monster effect flags");
	uint flags = options.monsterEffectFlags;
	if (options.horizontalFlip)
		flags |= MM::Shared::Xeen::SPRFLAG_HORIZ_FLIPPED;
	if (options.sceneClipped)
		flags |= MM::Shared::Xeen::SPRFLAG_SCENE_CLIPPED;
	if (options.bottomClipped) {
		SpriteResource::setClippedBottom(140);
		flags |= MM::Shared::Xeen::SPRFLAG_BOTTOM_CLIPPED;
	}
	const int scale = options.enlarge ? MM::Shared::Xeen::SCALE_ENLARGE : options.scaleIndex;
	const auto mode=flags&0xf00;
	if(mode==0x200 || mode==0x300 || mode==0x500) {
		if(options.enlarge || _impl->surface.w!=320 || _impl->surface.h!=200)
			throw std::invalid_argument("Monster effect requires the native scene surface");
		sprite.drawEngineFreeEffect(_impl->surface,static_cast<int>(frame),Common::Point(x,y),flags,scale,_impl->palette,options.monsterEffectSeed);
	} else sprite.draw(_impl->surface, static_cast<int>(frame), Common::Point(x, y), flags, scale);
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

void ScummVmXeenBridge::drawTraining(IndexedFrame &frame) {
	if(!frame.isValid() || frame.width!=320 || frame.height!=200)throw std::invalid_argument("Invalid Training draw context");
	struct Resource {const char *name;std::size_t bytes;std::uint32_t crc;unsigned frames;};
	constexpr Resource resources[]={{"trng1.twn",27998,0xa4e3bbdb,8},{"train.icn",1614,0x76c6ac78,4},{"esc.icn",792,0x096b68b7,2}};
	for(const auto &r:resources) {
		const auto bytes=readArchiveResource(r.name);
		if(bytes.size()!=r.bytes || crc32(0,bytes.data(),static_cast<uInt>(bytes.size()))!=r.crc)
			throw std::invalid_argument("Training original artwork identity changed");
		_impl->sprite(r.name,0,r.frames);
	}
	XSurface surface;surface.create(320,200);
	for(int y=0;y<200;++y)std::copy_n(frame.pixels.data()+y*320,320,static_cast<std::uint8_t *>(surface.getBasePtr(0,y)));
	_impl->sprite("trng1.twn",0,8).draw(surface,0,Common::Point(8,8));
	_impl->sprite("train.icn",0,4).draw(surface,0,Common::Point(8,140));
	_impl->sprite("esc.icn",0,2).draw(surface,0,Common::Point(86,140));
	for(int y=0;y<200;++y)std::copy_n(static_cast<const std::uint8_t *>(surface.getBasePtr(0,y)),320,frame.pixels.data()+y*320);
}
void ScummVmXeenBridge::drawSmith(IndexedFrame &frame) {
	if (!frame.isValid() || frame.width!=320 || frame.height!=200)
		throw std::invalid_argument("Invalid Ironworks draw context");
	const auto bytes=readArchiveResource("blck1.twn");
	if (bytes.size()!=42348 || crc32(0,bytes.data(),static_cast<uInt>(bytes.size()))!=0x70459675)
		throw std::invalid_argument("Ironworks original artwork identity changed");
	auto &sprite=_impl->sprite("blck1.twn",0,8);
	XSurface surface;surface.create(320,200);
	for(int y=0;y<200;++y)std::copy_n(frame.pixels.data()+y*320,320,
		static_cast<std::uint8_t *>(surface.getBasePtr(0,y)));
	sprite.draw(surface,0,Common::Point(8,8));
	for(int y=8;y<140;++y)std::copy_n(static_cast<const std::uint8_t *>(surface.getBasePtr(8,y)),215,
		frame.pixels.data()+y*320+8);
}
void ScummVmXeenBridge::drawTemple(IndexedFrame &frame) {
	if (!frame.isValid() || frame.width!=320 || frame.height!=200)
		throw std::invalid_argument("Invalid Temple draw context");
	const auto bytes=readArchiveResource("tmpl1.twn");
	if (bytes.size()!=21187 || crc32(0,bytes.data(),static_cast<uInt>(bytes.size()))!=0xb9ffe574)
		throw std::invalid_argument("Temple original artwork identity changed");
	const auto escape=readArchiveResource("esc.icn");
	if (escape.size()!=792 || crc32(0,escape.data(),static_cast<uInt>(escape.size()))!=0x096b68b7)
		throw std::invalid_argument("Temple original escape artwork identity changed");
	XSurface surface;surface.create(320,200);
	for(int y=0;y<200;++y)std::copy_n(frame.pixels.data()+y*320,320,
		static_cast<std::uint8_t *>(surface.getBasePtr(0,y)));
	_impl->sprite("tmpl1.twn",0,8).draw(surface,0,Common::Point(8,8));
	_impl->sprite("esc.icn",0,2).draw(surface,0,Common::Point(86,140));
	for(int y=8;y<160;++y)std::copy_n(static_cast<const std::uint8_t *>(surface.getBasePtr(8,y)),215,
		frame.pixels.data()+y*320+8);
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

void ScummVmXeenBridge::drawDialogSprite(IndexedFrame &frame,const char *name,unsigned index,int x,int y) {
	if(!frame.isValid() || frame.width!=320 || frame.height!=200) throw std::invalid_argument("Invalid dialog sprite frame");
	auto &sprite=_impl->sprite(name,index);
	XSurface surface;surface.create(320,200);
	for(int row=0;row<200;++row) std::copy_n(frame.pixels.data()+row*320,320,static_cast<std::uint8_t *>(surface.getBasePtr(0,row)));
	sprite.draw(surface,index,Common::Point(x,y));
	for(int row=0;row<200;++row) std::copy_n(static_cast<const std::uint8_t *>(surface.getBasePtr(0,row)),320,frame.pixels.data()+row*320);
}

} // namespace mmodern
