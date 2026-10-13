#include "games/xeen/XeenServiceAnimation.h"
#include <stdexcept>

namespace mmodern {
namespace {
struct Entry {
	const char *prefix;
	unsigned modulus, repeat;
};
// Index by XeenServiceArt. Prefix pointers DAT 0x5088c..0x508a7.
constexpr Entry kEntries[] = {
	{"bnkr", 23, 0}, {"blck", 13, 0}, {"gild", 32, 17},
	{"tvrn", 16, 0}, {"tmpl", 26, 20}, {"trng", 16, 0},
};
const Entry &entry(XeenServiceArt art) {
	const auto index = static_cast<unsigned>(art);
	if (index >= sizeof(kEntries) / sizeof(kEntries[0])) throw std::invalid_argument("unknown service art");
	return kEntries[index];
}
std::string resource(XeenServiceArt art, unsigned file) {
	return std::string(entry(art).prefix) + std::to_string(file) + ".twn";
}
}

unsigned XeenServiceAnimation::modulus(XeenServiceArt art) { return entry(art).modulus; }
unsigned XeenServiceAnimation::firstRepeat(XeenServiceArt art) { return entry(art).repeat; }

unsigned XeenServiceAnimation::next(XeenServiceArt art, unsigned index) {
	const auto &e = entry(art);
	if (index >= e.modulus) throw std::invalid_argument("service art index out of range");
	return index + 1 >= e.modulus ? e.repeat : index + 1;
}

XeenServiceSprite XeenServiceAnimation::base(XeenServiceArt art, unsigned index) {
	if (index >= entry(art).modulus) throw std::invalid_argument("service art index out of range");
	return {resource(art, index / 8 + 1), index % 8};
}

std::optional<XeenServiceAnimation::Speaking> XeenServiceAnimation::speaking(XeenServiceArt art) {
	switch (art) {
	case XeenServiceArt::Bank: return Speaking{resource(art, 3), 7, 11, false};
	case XeenServiceArt::Smith: return Speaking{resource(art, 2), 5, 7, true};
	case XeenServiceArt::Temple: return Speaking{resource(art, 4), 2, 4, false};
	case XeenServiceArt::Training: return Speaking{resource(art, 2), 8, 12, false};
	case XeenServiceArt::Guild: case XeenServiceArt::Tavern: break;
	}
	return std::nullopt;
}

std::vector<XeenServiceSprite> XeenServiceAnimation::compose(XeenServiceArt art, unsigned index,
		std::optional<unsigned> speakingFrame) {
	auto sprite = base(art, index);
	if (!speakingFrame) return {std::move(sprite)};
	const auto overlay = speaking(art);
	if (!overlay || *speakingFrame < overlay->first || *speakingFrame > overlay->last)
		throw std::invalid_argument("service speaking frame is outside the original range");
	XeenServiceSprite speech{overlay->resource, *speakingFrame};
	if (overlay->replacesBase) return {std::move(speech)};
	return {std::move(sprite), std::move(speech)};
}

} // namespace mmodern
