#include "formats/xeen/XeenAssetSource.h"

#include "compat/scummvm/ScummVmXeenBridge.h"
#include "games/xeen/XeenObjectVisual.h"
#include <stdexcept>

namespace mmodern {

struct XeenAssetSource::Impl {
	ScummVmXeenBridge bridge;

	explicit Impl(const GameInstallation &installation) : bridge(installation) {}

	Impl(const GameInstallation &installation, int width, int height) :
		bridge(installation, width, height) {
	}
};

XeenAssetSource::XeenAssetSource(const GameInstallation &installation) :
	_impl(new Impl(installation)) {
}

XeenAssetSource::XeenAssetSource(const GameInstallation &installation,
		int width, int height) :
	_impl(new Impl(installation, width, height)) {
}

XeenAssetSource::~XeenAssetSource() = default;

void XeenAssetSource::discardSpriteCache() { _impl->bridge.discardSpriteCache(); }
std::size_t XeenAssetSource::cachedSpriteCount() const { return _impl->bridge.cachedSpriteCount(); }
std::size_t XeenAssetSource::spriteLoadCount() const { return _impl->bridge.spriteLoadCount(); }

std::optional<std::vector<std::uint8_t>> XeenAssetSource::readCloudsVisualMetadataFromDarkArchive() {
	return _impl->bridge.readCloudsVisualMetadataFromDarkArchive();
}

void XeenAssetSource::drawObjectVisual(const XeenObjectVisual &visual, int x, int y,
		const XeenSpriteDrawOptions &options) {
	if ((visual.status != XeenObjectVisualStatus::SupportedStatic &&
		visual.status != XeenObjectVisualStatus::SupportedAnimated) ||
		!visual.identity.mapId || visual.identity.mapId.side != XeenSide::Clouds)
		throw std::runtime_error("cannot draw unsupported object visual: " + visual.diagnostic);
	auto resolvedOptions = options;
	resolvedOptions.horizontalFlip = visual.horizontalFlip;
	_impl->bridge.drawObjectSprite(visual.spriteName, visual.frame, x, y, resolvedOptions);
}

bool XeenAssetSource::hasArchiveResource(const std::string &resourceName) {
	return _impl->bridge.hasArchiveResource(resourceName);
}

std::vector<std::uint8_t> XeenAssetSource::readArchiveResource(const std::string &resourceName) {
	return _impl->bridge.readArchiveResource(resourceName);
}

bool XeenAssetSource::hasInitialResource(const std::string &resourceName) {
	return _impl->bridge.hasInitialResource(resourceName);
}

std::vector<std::uint8_t> XeenAssetSource::readInitialResource(const std::string &resourceName) {
	return _impl->bridge.readInitialResource(resourceName);
}

void XeenAssetSource::loadPalette(const std::string &resourceName) {
	_impl->bridge.loadPalette(resourceName);
}

void XeenAssetSource::loadRawFramebuffer(const std::string &resourceName) {
	_impl->bridge.loadRawFramebuffer(resourceName);
}

void XeenAssetSource::drawSprite(const std::string &resourceName,
		std::size_t frame, int x, int y) {
	_impl->bridge.drawSprite(resourceName, frame, x, y);
}

void XeenAssetSource::drawSprite(const std::string &resourceName,
		std::size_t frame, int x, int y, const XeenSpriteDrawOptions &options) {
	_impl->bridge.drawSprite(resourceName, frame, x, y, options);
}

IndexedFrame XeenAssetSource::snapshot() const {
	return _impl->bridge.snapshot();
}

void XeenAssetSource::drawNpc(IndexedFrame &frame, std::uint8_t portraitId,
		std::size_t portraitFrame) {
	_impl->bridge.drawNpc(frame, portraitId, portraitFrame);
}

} // namespace mmodern
