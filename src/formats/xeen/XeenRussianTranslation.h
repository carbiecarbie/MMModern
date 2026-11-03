#ifndef MMODERN_FORMATS_XEEN_RUSSIAN_TRANSLATION_H
#define MMODERN_FORMATS_XEEN_RUSSIAN_TRANSLATION_H

#include "core/GameInstallation.h"
#include "formats/xeen/XeenLanguage.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace mmodern {

// How a substituted archive entry is combined with the CD's (M110 decision 3).
// Text and pictures come from the translation once their structure matches the
// CD; formats that also carry gameplay data keep the CD's bytes and take only
// the named text fields. KeptCd entries are never substituted.
enum class XeenLayerFormat {
	EventText,        // aaze*.txt: same number of NUL-separated strings
	MapName,          // xeen/dark map names: same number of strings
	StringTable,      // *.bin, spells.xen: same number of strings
	Picture,          // sprite: at least the CD's frames
	RawPicture,       // 320x200 raw frame: same size
	Font,             // fnt: English bank equal to the CD font, second bank added
	NewGameContainer, // 2a0c: CD block, roster names from the translation
	MonsterTable,     // xeen.mon/dark.mon: CD records, names from the translation
	MirrorList,       // xeenmirr/darkmirr.txt: CD records, words from the translation
	KeptCd            // stays CD: no reader names it, or its reader needs Part B/C
};

struct XeenLayerEntry {
	std::uint16_t id;
	XeenLayerFormat format;
	const char *name; // Diagnostic only; empty when no reader names it.
};

// The fixed per-archive list of substituted entry ids (metadata only).
const std::vector<XeenLayerEntry> &xeenRussianEntries(XeenArchiveRole role);
const char *xeenLayerFormatName(XeenLayerFormat format);

// Combines one CD entry with its translated counterpart. Throws
// std::runtime_error when the translated entry does not keep the CD's
// structure; KeptCd returns the CD bytes.
std::vector<std::uint8_t> xeenLayerMerge(XeenLayerFormat format,
	const std::vector<std::uint8_t> &cd, const std::vector<std::uint8_t> &translated);

class XeenTranslationError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

// The Russian World of Xeen translation (Clouds of Xeen RUS and Darkside of
// Xeen RUS 1.2), identified by the size and CRC-32 of each of its five files.
// Any other or missing file is refused with a message naming it.
class XeenRussianTranslation {
public:
	static std::shared_ptr<const XeenRussianTranslation> open(const std::filesystem::path &folder);

	const std::filesystem::path &folder() const { return _folder; }
	// The identified Russian programs (Part B reads their text).
	const std::filesystem::path &program(bool executable) const { return executable ? _exe : _dat; }
	// The translated entry of a listed id, as stored in the translation.
	const std::vector<std::uint8_t> &translatedEntry(XeenArchiveRole role, std::uint16_t id) const;
	// nullopt: the CD entry is read unchanged.
	std::optional<std::vector<std::uint8_t>> substitute(XeenArchiveRole role, std::uint16_t id,
		const std::vector<std::uint8_t> &cd) const;

private:
	XeenRussianTranslation() = default;
	std::filesystem::path _folder, _dat, _exe;
	std::map<XeenArchiveRole, std::map<std::uint16_t, std::vector<std::uint8_t>>> _entries;
};

inline XeenLanguage xeenLanguage(const GameInstallation &installation) {
	return installation.translation ? XeenLanguage::Russian : XeenLanguage::English;
}

} // namespace mmodern

#endif
