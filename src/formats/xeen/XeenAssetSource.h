#ifndef MMODERN_FORMATS_XEEN_ASSET_SOURCE_H
#define MMODERN_FORMATS_XEEN_ASSET_SOURCE_H

#include "core/GameInstallation.h"
#include "core/IndexedFrame.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

struct XeenObjectVisual;

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
	// Transient NPC composition uses the existing cache, never the world surface.
	void drawNpc(IndexedFrame &frame, std::uint8_t portraitId, std::size_t portraitFrame);
	void discardSpriteCache();
	std::size_t cachedSpriteCount() const;
	// Successful resource reads + SpriteResource constructions, not draw calls.
	std::size_t spriteLoadCount() const;
	// Explicit physical origin; nullopt means archive/member absent, not empty.
	std::optional<std::vector<std::uint8_t>> readCloudsVisualMetadataFromDarkArchive();
	void drawObjectVisual(const XeenObjectVisual &visual, int x, int y,
		const XeenSpriteDrawOptions &options = {});
	bool hasArchiveResource(const std::string &resourceName);
	std::vector<std::uint8_t> readArchiveResource(const std::string &resourceName);
	bool hasInitialResource(const std::string &resourceName);
	std::vector<std::uint8_t> readInitialResource(const std::string &resourceName);

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

} // namespace mmodern

#endif
