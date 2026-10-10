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
#include <random>
#include <fstream>

namespace mmodern {
namespace {


using MM::Shared::Xeen::SpriteResource;
using MM::Shared::Xeen::XSurface;

// Read-only initial state, reconstructed as SaveArchive::reset does. No saves,
// Party, or engine are instantiated. BaseCCArchive remains the real index reader.
class InitialCloudsArchive final : public MM::Shared::Xeen::BaseCCArchive {
public:
	explicit InitialCloudsArchive(std::vector<std::uint8_t> data) : _data(std::move(data)) {
		if (_data.size() < 2)
			throw std::runtime_error("Truncated Clouds initial container");
		const std::size_t count = _data[0] | (static_cast<unsigned>(_data[1]) << 8);
		const std::size_t headerSize = 2 + count * 8;
		if (!count || headerSize > _data.size())
			throw std::runtime_error("Invalid Clouds initial index");
		// Validate the reserved byte before the upstream reader's assertion.
		for (std::size_t i = 7; i < count * 8; i += 8) {
			const unsigned b = _data[2 + i];
			if ((((b << 2) | (b >> 6)) + 0xac + i * 0x67) % 256 != 0)
				throw std::runtime_error("Invalid reserved Clouds initial index byte");
		}
		Common::MemoryReadStream stream(_data.data(), static_cast<uint32>(_data.size()));
		loadIndex(stream);
		for (const auto &entry : _index) {
			const auto offset = static_cast<std::size_t>(entry._offset);
			if (offset < headerSize || offset > _data.size() || entry._size > _data.size() - offset)
				throw std::runtime_error("Resource outside Clouds initial container");
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
		throw std::runtime_error("Invalid CC resource size: " + name);
	std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
	if ((!bytes.empty() && stream.read(bytes.data(), static_cast<uint32>(bytes.size())) != bytes.size()) || stream.err())
		throw std::runtime_error("Incomplete resource read: " + name);
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

// All installed archive roles use the pinned BaseCCArchive name/index decoder.
// Validate its preconditions first, so malformed resources never reach error().
class CheckedInstalledArchive : public MM::Shared::Xeen::BaseCCArchive {
public:
 explicit CheckedInstalledArchive(ReadOnlyDataFile file):_file(std::move(file)) {
  auto source=_file.open();_indexBytes.resize(2);
  if(source->read(_indexBytes.data(),2)!=2)throw std::runtime_error("Truncated CC count: "+_file.identity());
  const unsigned count=_indexBytes[0]|unsigned(_indexBytes[1])<<8;
  const std::size_t size=2+count*8;
  if(!count||size>source->size())throw std::runtime_error("Invalid CC index extent: "+_file.identity());
  _indexBytes.resize(size);
  if(source->read(_indexBytes.data()+2,size-2)!=size-2)throw std::runtime_error("Truncated CC index: "+_file.identity());
  for(std::size_t i=7;i<count*8;i+=8){const unsigned b=_indexBytes[2+i];
   if((((b<<2)|(b>>6))+0xac+i*0x67)%256!=0)throw std::runtime_error("Invalid CC reserved index byte: "+_file.identity());}
  Common::MemoryReadStream stream(_indexBytes.data(),static_cast<uint32>(_indexBytes.size()));loadIndex(stream);
  for(std::size_t i=0;i<_index.size();++i) {
   const auto &entry=_index[i];
   // Requested extents are checked below, so an unrelated truncated optional
   // member cannot prevent reading another structurally valid resource.
   for(std::size_t j=0;j<i;++j)if(_index[j]._id==entry._id)throw std::runtime_error("Ambiguous CC member ID: "+_file.identity());
  }
 }
 std::string identity() const{return _file.identity();}
 Common::SeekableReadStream *createReadStreamForMember(const Common::Path &path) const override {
  MM::Shared::Xeen::CCEntry entry;if(!getHeaderEntry(path,entry))return nullptr;
  auto source=_file.open();std::vector<std::uint8_t> index(_indexBytes.size());
  if(source->read(index.data(),index.size())!=index.size()||index!=_indexBytes)throw std::runtime_error("Admitted CC index changed: "+_file.identity());
  if(entry._offset<static_cast<int64>(_indexBytes.size()))throw std::runtime_error("CC member overlaps archive index: "+_file.identity());
  if(static_cast<std::uint64_t>(entry._offset)>source->size() || entry._size>source->size()-entry._offset)throw std::runtime_error("CC member payload is truncated: "+_file.identity());
  if(!source->seek(entry._offset))throw std::runtime_error("Cannot seek CC member: "+_file.identity());
  std::vector<std::uint8_t> bytes(entry._size);
  if(source->read(bytes.data(),bytes.size())!=bytes.size())throw std::runtime_error("Short CC member read: "+_file.identity());
  for(auto &byte:bytes)byte^=0x35;
  if(bytes.empty())return new Common::MemoryReadStream(nullptr,0);
  Common::MemoryReadStream input(bytes.data(),static_cast<uint32>(bytes.size()));return input.readStream(bytes.size());
 }
 std::optional<std::vector<std::uint8_t>> readMetadataChecked(const char *name,std::size_t limit) const {
  const Common::Path member(name,Common::Path::kNoSeparator);MM::Shared::Xeen::CCEntry entry;
  if(!getHeaderEntry(member,entry))return std::nullopt;
  if(entry._size>limit)throw std::runtime_error(std::string("Oversized DARK.CC/")+name);
  std::unique_ptr<Common::SeekableReadStream> stream(createReadStreamForMember(member));return readBytes(*stream,name);
 }
private:ReadOnlyDataFile _file;std::vector<std::uint8_t> _indexBytes;
};
class DarkMetadataArchive final:public CheckedInstalledArchive {
public:
 explicit DarkMetadataArchive(ReadOnlyDataFile file):CheckedInstalledArchive(std::move(file)){}
 std::optional<std::vector<std::uint8_t>> readItemMaterialNamesChecked() const{return readMetadataChecked("mae.xen",8192);}
 std::optional<std::vector<std::uint8_t>> readLearnedSpellNamesChecked() const{return readMetadataChecked("spells.xen",77*64);}
 std::optional<std::vector<std::uint8_t>> readMonsterStatisticsChecked() const{return readMetadataChecked("xeen.mon",65535);}
};
using SceneInstalledArchive=CheckedInstalledArchive;

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
	CheckedInstalledArchive archive;
	bool darkAvailable = false;
	std::unique_ptr<DarkMetadataArchive> darkMetadataArchive;
	std::optional<ReadOnlyDataFile> sceneIntroPath, sceneDarkPath;
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
		archive(archiveDataFile(installation,XeenArchiveRole::Clouds)) {
		darkAvailable = installation.hasDarkside();
		sceneIntroPath = installation.introData;
		if(installation.hasDarkside()) sceneDarkPath=archiveDataFile(installation,XeenArchiveRole::Darkside);
	}

	Impl(const GameInstallation &installation, int width, int height) : Impl(installation) {
		if (width <= 0 || height <= 0)
			throw std::runtime_error("Invalid framebuffer dimensions");
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
	SceneResource sceneResource(const std::string &name, XeenSceneArchive selection = XeenSceneArchive::Current) {
		const Common::Path path(name.c_str(), Common::Path::kNoSeparator);
		// Pinned File::open: selected/current archive, then registered search
		// sources (INTRO). DARK is an archive selection, never a search source.
		if (selection == XeenSceneArchive::Darkside || selection == XeenSceneArchive::DarksideOnly) {
			if (sceneDarkPath.has_value()) {
				if (!sceneDark) sceneDark.reset(new SceneInstalledArchive(*sceneDarkPath));
				if (sceneDark->hasFile(path)) return {sceneDark.get(), "dark.cc"};
			}
		} else if (archive.hasFile(path)) return {&archive, "xeen.cc"};
		if (selection == XeenSceneArchive::DarksideOnly) return {nullptr, {}};
		if (sceneIntroPath.has_value()) {
			if (!sceneIntro) sceneIntro.reset(new SceneInstalledArchive(*sceneIntroPath));
			if (sceneIntro->hasFile(path)) return {sceneIntro.get(), "intro.cc"};
		}
		return {nullptr, {}};
	}

	StreamSpriteResource &sprite(const std::string &resourceName,
			std::optional<std::size_t> checkedFrame = std::nullopt, unsigned monsterFrames = 0,
			bool sceneLookup = false, XeenSceneArchive selection = XeenSceneArchive::Current) {
		if(spriteIntegrityFailed) throw std::runtime_error("Sprite resource integrity previously failed");
		const auto source = sceneLookup ? sceneResource(resourceName,selection) : SceneResource{&archive, "xeen.cc"};
		if (!source.archive) throw std::runtime_error("Missing scene resource in installed CC archives: " + resourceName);
		// Clouds retains its existing identity; other archives have separate
		// cache/admission identities even when resource names collide.
		const auto key = static_cast<CheckedInstalledArchive *>(source.archive)->identity()+"|"+source.origin+"|"+std::to_string(MM::Shared::Xeen::BaseCCArchive::convertNameToId(Common::Path(resourceName.c_str(),Common::Path::kNoSeparator)));
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

std::size_t ScummVmXeenBridge::spriteFrameCount(const std::string &name, XeenSceneArchive selection) {
	const auto count=_impl->sprite(name,std::nullopt,0,true,selection).size();
	if (!count) throw std::runtime_error("Empty sprite: "+name);
	for (std::size_t frame=0;frame<count;++frame) _impl->sprite(name,frame,0,true,selection);
	return count;
}

bool ScummVmXeenBridge::hasSceneResource(const std::string &name, XeenSceneArchive selection) {
	return _impl->sceneResource(name,selection).archive != nullptr;
}
IndexedFrame ScummVmXeenBridge::restDreamImage() {
 const auto source=_impl->sceneResource("scene1.raw",XeenSceneArchive::Current);
 if(!source.archive)throw std::runtime_error("Original Rest dream image is absent");
 const auto stream=openResource(*source.archive,"scene1.raw",source.origin);
 auto bytes=readBytes(*stream,"scene1.raw");
 if(bytes.size()!=64000)throw std::runtime_error("Invalid original Rest dream image size");
 IndexedFrame frame{320,200,std::move(bytes)};frame.palette=_impl->palette;return frame;
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
		_impl->darkMetadataArchive.reset(new DarkMetadataArchive(*_impl->sceneDarkPath));
	const Common::Path path("clouds.dat", Common::Path::kNoSeparator);
	std::unique_ptr<Common::SeekableReadStream> stream(_impl->darkMetadataArchive->createReadStreamForMember(path));
	if (!stream) return std::nullopt;
	return readBytes(*stream, "DARK.CC/clouds.dat");
}

std::optional<std::vector<std::uint8_t>> ScummVmXeenBridge::readItemMaterialNamesFromDarkArchive() {
	if (!_impl->darkAvailable)
		return std::nullopt;
	if (!_impl->darkMetadataArchive)
		_impl->darkMetadataArchive.reset(new DarkMetadataArchive(*_impl->sceneDarkPath));
	return _impl->darkMetadataArchive->readItemMaterialNamesChecked();
}

std::optional<std::vector<std::uint8_t>> ScummVmXeenBridge::readLearnedSpellNamesFromDarkArchive() {
	if (!_impl->darkAvailable) return std::nullopt;
	if (!_impl->darkMetadataArchive)
		_impl->darkMetadataArchive.reset(new DarkMetadataArchive(*_impl->sceneDarkPath));
	return _impl->darkMetadataArchive->readLearnedSpellNamesChecked();
}

std::optional<std::vector<std::uint8_t>> ScummVmXeenBridge::readCloudsMonsterStatisticsFromDarkArchive() {
	if (!_impl->darkAvailable) return std::nullopt;
	if (!_impl->darkMetadataArchive)
		_impl->darkMetadataArchive.reset(new DarkMetadataArchive(*_impl->sceneDarkPath));
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

std::vector<std::uint8_t> ScummVmXeenBridge::readArchiveResource(const std::string &resourceName, XeenSceneArchive selection) {
	const auto source=(selection==XeenSceneArchive::Current || selection==XeenSceneArchive::Clouds) ? Impl::SceneResource{&_impl->archive,"xeen.cc"} : _impl->sceneResource(resourceName,selection);
	if(!source.archive)throw std::runtime_error("Missing resource in selected archive: "+resourceName);
	auto stream = openResource(*source.archive, resourceName, source.origin);
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

void ScummVmXeenBridge::loadPalette(const std::string &resourceName, XeenSceneArchive selection) {
	const auto source=(selection==XeenSceneArchive::Current || selection==XeenSceneArchive::Clouds) ? Impl::SceneResource{&_impl->archive,"xeen.cc"} : _impl->sceneResource(resourceName,selection);
	if(!source.archive)throw std::runtime_error("Missing palette in selected archive: "+resourceName);
	std::unique_ptr<Common::SeekableReadStream> stream = openResource(*source.archive, resourceName, source.origin);
	if (stream->size() != static_cast<int64>(IndexedFrame::kPaletteSize))
		throw std::runtime_error("Invalid palette size: " + resourceName);

	if (stream->read(_impl->palette.data(), static_cast<uint32>(_impl->palette.size())) !=
			_impl->palette.size())
		throw std::runtime_error("Incomplete palette read: " + resourceName);

	for (std::uint8_t &component : _impl->palette)
		component = static_cast<std::uint8_t>(component << 2);
}

void ScummVmXeenBridge::loadRawFramebuffer(const std::string &resourceName, XeenSceneArchive selection) {
	const auto source=(selection==XeenSceneArchive::Current || selection==XeenSceneArchive::Clouds) ? Impl::SceneResource{&_impl->archive,"xeen.cc"} : _impl->sceneResource(resourceName,selection);
	if(!source.archive)throw std::runtime_error("Missing RAW in selected archive: "+resourceName);
	std::unique_ptr<Common::SeekableReadStream> stream = openResource(*source.archive, resourceName, source.origin);
	const uint32 expectedSize = static_cast<uint32>(_impl->surface.w * _impl->surface.h);
	if (stream->size() != expectedSize)
		throw std::runtime_error("Invalid RAW framebuffer size: " + resourceName);

	for (int y = 0; y < _impl->surface.h; ++y) {
		if (stream->read(_impl->surface.getBasePtr(0, y), _impl->surface.w) !=
				static_cast<uint32>(_impl->surface.w))
			throw std::runtime_error("Incomplete framebuffer read: " + resourceName);
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
	StreamSpriteResource &sprite = _impl->sprite(resourceName, std::nullopt, 0, sceneLookup, options.archive);
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
	struct Resource {const char *name;unsigned frames;};
	constexpr Resource resources[]={{"trng1.twn",8},{"train.icn",4},{"esc.icn",2}};
	for(const auto &r:resources) {
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

void ScummVmXeenBridge::drawDialogSprite(IndexedFrame &frame,const char *name,unsigned index,int x,int y,XeenSceneArchive selection) {
	if(!frame.isValid() || frame.width!=320 || frame.height!=200) throw std::invalid_argument("Invalid dialog sprite frame");
	auto &sprite=_impl->sprite(name,index,0,selection!=XeenSceneArchive::Current,selection);
	XSurface surface;surface.create(320,200);
	for(int row=0;row<200;++row) std::copy_n(frame.pixels.data()+row*320,320,static_cast<std::uint8_t *>(surface.getBasePtr(0,row)));
	sprite.draw(surface,index,Common::Point(x,y));
	for(int row=0;row<200;++row) std::copy_n(static_cast<const std::uint8_t *>(surface.getBasePtr(0,row)),320,frame.pixels.data()+row*320);
}

IndexedFrame ScummVmXeenBridge::cursorImage() {
 return cursorImage(XeenSceneArchive::Current);
}
IndexedFrame ScummVmXeenBridge::cursorImage(XeenSceneArchive selection) {
 // EventsManager::setCursor(0): resize, transparent palette index 0, hotspot 0,0.
 auto &sprite=_impl->sprite("mouse.icn",0,0,selection!=XeenSceneArchive::Current,selection);
 XSurface surface;sprite.draw(surface,0,Common::Point(0,0),MM::Shared::Xeen::SPRFLAG_RESIZE);
 auto frame=snapshot();frame.width=surface.w;frame.height=surface.h;
 frame.pixels.resize(std::size_t(surface.w)*surface.h);
 for(int row=0;row<surface.h;++row)std::copy_n(static_cast<const std::uint8_t *>(surface.getBasePtr(0,row)),surface.w,frame.pixels.data()+row*surface.w);
 return frame;
}

} // namespace mmodern
