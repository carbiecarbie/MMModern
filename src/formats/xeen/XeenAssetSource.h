#ifndef MMODERN_FORMATS_XEEN_ASSET_SOURCE_H
#define MMODERN_FORMATS_XEEN_ASSET_SOURCE_H

#include "core/GameInstallation.h"
#include "core/IndexedFrame.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"
#include "formats/xeen/XeenMonsterAppearance.h"
#include "formats/xeen/XeenDosText.h"

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
	void drawDialogSprite(IndexedFrame &,const char *,unsigned,int,int);
 IndexedFrame cursorImage();
 IndexedFrame restDreamImage();
	void drawSmith(IndexedFrame &frame);
	void drawTraining(IndexedFrame &frame);
	void drawTemple(IndexedFrame &frame);
	void discardSpriteCache();
	std::size_t cachedSpriteCount() const;
	// Successful resource reads + SpriteResource constructions, not draw calls.
	std::size_t spriteLoadCount() const;
	std::size_t spriteFrameCount(const std::string &resourceName, XeenSceneArchive selection = XeenSceneArchive::Current);
	// Presentation lookup across installed CC archives, independent of gameplay reads.
	bool hasSceneResource(const std::string &resourceName, XeenSceneArchive selection = XeenSceneArchive::Current);
	void drawSceneSprite(const std::string &resourceName, std::size_t frame,
		int x, int y, const XeenSpriteDrawOptions &options = {});
	static std::string normalMonsterResource(std::uint8_t image);
	void validateProjectile(bool enemy);
	void drawProjectile(bool enemy,unsigned row,int x,int y,const XeenSpriteDrawOptions &);
 void drawProjectile(unsigned pow,bool enemy,unsigned row,int x,int y,const XeenSpriteDrawOptions &);
	void validateNormalMonster(std::uint8_t image);
	void validateAttackMonster(std::uint8_t image);
	static std::string attackMonsterResource(std::uint8_t image);
	void drawMonster(std::uint8_t image, XeenMonsterAppearance appearance, int x, int y,
		const XeenSpriteDrawOptions &options);
	void drawNormalMonster(std::uint8_t image, std::size_t frame, int x, int y,
		const XeenSpriteDrawOptions &options);
	// Explicit physical origin; nullopt means archive/member absent, not empty.
	std::optional<std::vector<std::uint8_t>> readCloudsVisualMetadataFromDarkArchive();
	// Optional commercial English material names from exactly DARK.CC/mae.xen.
	std::optional<std::vector<std::uint8_t>> readItemMaterialNamesFromDarkArchive();
	std::optional<std::vector<std::uint8_t>> readLearnedSpellNamesFromDarkArchive();
	// Missing archive/member: nullopt; extent/read failures: exception. Parsing is separate.
	std::optional<std::vector<std::uint8_t>> readCloudsMonsterStatisticsFromDarkArchive();
	void drawObjectVisual(const XeenObjectVisual &visual, int x, int y,
		const XeenSpriteDrawOptions &options = {});
	bool hasArchiveResource(const std::string &resourceName);
	std::vector<std::uint8_t> readArchiveResource(const std::string &resourceName);
	bool hasInitialResource(const std::string &resourceName);
	std::vector<std::uint8_t> readInitialResource(const std::string &resourceName);
 const XeenDosText &uiText();

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

} // namespace mmodern

#endif
