#ifndef MMODERN_COMPAT_SCUMMVM_XEEN_BRIDGE_H
#define MMODERN_COMPAT_SCUMMVM_XEEN_BRIDGE_H

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

class ScummVmXeenBridge {
public:
	explicit ScummVmXeenBridge(const GameInstallation &installation);
	ScummVmXeenBridge(const GameInstallation &installation, int width, int height);
	~ScummVmXeenBridge();

	ScummVmXeenBridge(const ScummVmXeenBridge &) = delete;
	ScummVmXeenBridge &operator=(const ScummVmXeenBridge &) = delete;

	void loadPalette(const std::string &resourceName);
	void loadRawFramebuffer(const std::string &resourceName);
	void drawSprite(const std::string &resourceName, std::size_t frame, int x, int y);
	void drawSprite(const std::string &resourceName, std::size_t frame, int x, int y,
		const XeenSpriteDrawOptions &options);
	IndexedFrame snapshot() const;
	void drawNpc(IndexedFrame &frame, std::uint8_t portraitId, std::size_t portraitFrame);
	void discardSpriteCache();
	std::size_t cachedSpriteCount() const;
	// Successful resource reads + SpriteResource constructions, not draw calls.
	std::size_t spriteLoadCount() const;
	std::optional<std::vector<std::uint8_t>> readCloudsVisualMetadataFromDarkArchive();
	std::optional<std::vector<std::uint8_t>> readItemMaterialNamesFromDarkArchive();
	void drawObjectSprite(const std::string &resourceName, std::size_t frame,
		int x, int y, const XeenSpriteDrawOptions &options);
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
