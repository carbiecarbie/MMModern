#ifndef MMODERN_FORMATS_XEEN_SAVE_FORMAT_H
#define MMODERN_FORMATS_XEEN_SAVE_FORMAT_H

#include "games/xeen/XeenSaveSnapshot.h"

#include <istream>
#include <stdexcept>

namespace mmodern {

class XeenUnsupportedSave : public std::runtime_error {
public:
	// Older: a recognizable earlier format that a current save may replace.
	// Newer: a newer or unrecognized envelope/pair; the file stays protected.
	enum class Kind { Older, Newer };
	explicit XeenUnsupportedSave(Kind kind) : std::runtime_error(kind == Kind::Older ?
		"This save was created by an older MMModern build and is no longer supported." :
		"This save was created by a newer or unsupported MMModern build."),
		recognizableOlder(kind == Kind::Older) {}
	const bool recognizableOlder;
};

class XeenSaveFormat {
public:
	static constexpr std::uint16_t kJourneyVersion = 4;
	static constexpr std::size_t kHeaderSize = 20;
	static constexpr std::size_t kMaximumSize = 4 * 1024 * 1024;
	static constexpr std::size_t kMaximumObjects = 65536;
	static constexpr std::size_t kMaximumEvents = 262144;

	// Throws before returning a value on malformed input. No live state access.
	static XeenSaveSnapshot decode(const std::vector<std::uint8_t> &bytes);
	static std::vector<std::uint8_t> encode(const XeenSaveSnapshot &snapshot);
	// Resource identity, active-character safety and composition are separate.
	static void validate(const XeenSaveSnapshot &snapshot);

	// Reads from the caller's current position to EOF in bounded chunks. The
	// caller supplies a newly opened binary archive stream; no file is opened here.
	static XeenArchiveFingerprint fingerprint(std::istream &stream);
};

} // namespace mmodern
#endif
