#ifndef MMODERN_FORMATS_XEEN_SAVE_FORMAT_H
#define MMODERN_FORMATS_XEEN_SAVE_FORMAT_H

#include "games/xeen/XeenSaveSnapshot.h"

#include <istream>

namespace mmodern {

class XeenSaveFormat {
public:
	static constexpr std::uint16_t kVersion = 2;
	static constexpr std::size_t kHeaderSize = 20;
	static constexpr std::size_t kMaximumSize = 4 * 1024 * 1024;
	static constexpr std::size_t kMaximumObjects = 65536;
	static constexpr std::size_t kMaximumEvents = 262144;

	// Throws before returning a value on malformed input. No live state access.
	static XeenSaveSnapshot decode(const std::vector<std::uint8_t> &bytes);
	static std::vector<std::uint8_t> encode(const XeenSaveSnapshot &snapshot);
	// Structural checks accept unresolved v1 input; only encoding requires complete items.
	// Resource identity, active-character safety and composition are separate.
	static void validate(const XeenSaveSnapshot &snapshot);

	// Reads from the caller's current position to EOF in bounded chunks. The
	// caller supplies a newly opened binary archive stream; no file is opened here.
	static XeenArchiveFingerprint fingerprint(std::istream &stream);
};

} // namespace mmodern
#endif
