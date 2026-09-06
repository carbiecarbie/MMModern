#include "formats/xeen/XeenAssetSource.h"

#include "compat/scummvm/ScummVmXeenBridge.h"

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

} // namespace mmodern
