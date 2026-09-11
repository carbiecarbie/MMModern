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

std::string XeenAssetSource::normalMonsterResource(std::uint8_t image) {
	const auto number = std::to_string(image);
	return std::string(3 - number.size(), '0') + number + ".mon";
}
void XeenAssetSource::validateNormalMonster(std::uint8_t image) {
	_impl->bridge.validateNormalMonster(normalMonsterResource(image));
}
void XeenAssetSource::drawNormalMonster(std::uint8_t image, std::size_t frame, int x, int y,
		const XeenSpriteDrawOptions &options) {
	if (frame >= 8 || options.horizontalFlip || options.enlarge || !options.sceneClipped)
		throw std::invalid_argument("Unsupported normal monster drawing");
	const auto resource = normalMonsterResource(image);
	_impl->bridge.validateNormalMonster(resource);
	_impl->bridge.drawObjectSprite(resource, frame, x, y, options);
}

void XeenAssetSource::discardSpriteCache() { _impl->bridge.discardSpriteCache(); }
std::size_t XeenAssetSource::cachedSpriteCount() const { return _impl->bridge.cachedSpriteCount(); }
std::size_t XeenAssetSource::spriteLoadCount() const { return _impl->bridge.spriteLoadCount(); }

std::optional<std::vector<std::uint8_t>> XeenAssetSource::readCloudsVisualMetadataFromDarkArchive() {
	return _impl->bridge.readCloudsVisualMetadataFromDarkArchive();
}

std::optional<std::vector<std::uint8_t>> XeenAssetSource::readItemMaterialNamesFromDarkArchive() {
	return _impl->bridge.readItemMaterialNamesFromDarkArchive();
}

std::optional<std::vector<std::uint8_t>> XeenAssetSource::readCloudsMonsterStatisticsFromDarkArchive() {
	return _impl->bridge.readCloudsMonsterStatisticsFromDarkArchive();
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
