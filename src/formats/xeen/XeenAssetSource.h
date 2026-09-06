#ifndef MMODERN_FORMATS_XEEN_ASSET_SOURCE_H
#define MMODERN_FORMATS_XEEN_ASSET_SOURCE_H

#include "core/GameInstallation.h"
#include "core/IndexedFrame.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mmodern {

class XeenAssetSource {
public:
	explicit XeenAssetSource(const GameInstallation &installation);
	XeenAssetSource(const GameInstallation &installation, int width, int height);
	~XeenAssetSource();

	XeenAssetSource(const XeenAssetSource &) = delete;
	XeenAssetSource &operator=(const XeenAssetSource &) = delete;

	void loadPalette(const std::string &resourceName);
	void loadRawFramebuffer(const std::string &resourceName);
	void drawSprite(const std::string &resourceName, std::size_t frame, int x, int y);
	void drawSprite(const std::string &resourceName, std::size_t frame, int x, int y,
		const XeenSpriteDrawOptions &options);
	IndexedFrame snapshot() const;
	bool hasInitialResource(const std::string &resourceName);
	std::vector<std::uint8_t> readInitialResource(const std::string &resourceName);

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

} // namespace mmodern

#endif
