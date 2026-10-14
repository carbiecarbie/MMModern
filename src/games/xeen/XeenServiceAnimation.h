#ifndef MMODERN_XEEN_SERVICE_ANIMATION_H
#define MMODERN_XEEN_SERVICE_ANIMATION_H

#include "core/PresentationClock.h"
#include <optional>
#include <string>
#include <vector>

namespace mmodern {

// The six Clouds service backgrounds drawn at (8,8) by the original location
// routine (DAT 0x1c038..0x1c51f). Frame selection only: no clock, no sound state
// and no random draw live here.
enum class XeenServiceArt { Bank, Smith, Guild, Tavern, Temple, Training };

struct XeenServiceSprite {
	std::string resource;
	unsigned frame = 0;
	bool operator==(const XeenServiceSprite &other) const {
		return resource == other.resource && frame == other.frame;
	}
};

// Sample-driver visual busy interval: replacement starts a new span; polling
// never extends it. Playback remains outside the presentation owner.
class XeenSilentSampleInterval {
public:
	void start(std::uint64_t now,std::uint64_t duration) { end=PresentationClock::checkedAdd(now,duration); }
	void stop() noexcept { end=0; }
	bool playing(std::uint64_t now) const noexcept { return now<end; }
	std::uint64_t deadline() const noexcept { return end; }
private:
	std::uint64_t end=0;
};

class XeenServiceAnimation {
public:
	static constexpr int kX = 8, kY = 8;
	// Frame modulus per service (DAT 0x508b6): Bank 23, Smith 13, Guild 32, Tavern 16,
	// Temple 26, Training 16. The first cycle starts at 0; later cycles restart at
	// firstRepeat() (Guild 17, Temple 20, the others 0), DAT 0x1c45e..0x1c499.
	static unsigned modulus(XeenServiceArt);
	static const char *greeting(XeenServiceArt);
	static unsigned firstRepeat(XeenServiceArt);
	// The logical index after the one just drawn.
	static unsigned next(XeenServiceArt, unsigned index);
	// Base frame: file prefix + (index / 8 + 1) + ".twn", local frame index % 8.
	static XeenServiceSprite base(XeenServiceArt, unsigned index);
	// Inclusive local frame range of the DOS speaking overlay in its resource, or none
	// for Guild and Tavern. Bank bnkr3 7..11, Temple tmpl4 2..4 and Training trng2 8..12
	// are drawn over the base; Smith blck2 5..7 replaces it (0x1c03b..0x1c54f).
	struct Speaking {
		std::string resource;
		unsigned first = 0, last = 0;
		bool replacesBase = false;
	};
	static std::optional<Speaking> speaking(XeenServiceArt);
	// Draw list in order. A speaking frame, when supplied, must lie in speaking()'s range;
	// the caller owns whether and which frame (M63 owns the DOS gate and random choice).
	static std::vector<XeenServiceSprite> compose(XeenServiceArt, unsigned index,
		std::optional<unsigned> speakingFrame = std::nullopt);
};

} // namespace mmodern
#endif
