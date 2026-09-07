#ifndef MMODERN_GAMES_XEEN_MAP_IDENTITY_H
#define MMODERN_GAMES_XEEN_MAP_IDENTITY_H

#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <tuple>

namespace mmodern {

enum class XeenSide : std::uint8_t { Clouds, Darkside };

// Runtime/loading context, never part of the original binary map format.
struct XeenMapIdentity {
	XeenSide side = XeenSide::Clouds;
	std::uint16_t number = 0;

	constexpr XeenMapIdentity() = default;
	// Compatibility for existing Clouds entry points/fixtures. No conversion
	// back to a numeric ID: providers cannot silently drop the side.
	constexpr XeenMapIdentity(std::uint16_t cloudsNumber) : number(cloudsNumber) {}
	constexpr XeenMapIdentity(XeenSide sideValue, std::uint16_t mapNumber) :
		side(sideValue), number(mapNumber) {}

	explicit constexpr operator bool() const {
		return number != 0 && (side == XeenSide::Clouds || side == XeenSide::Darkside);
	}
	friend bool operator==(XeenMapIdentity a, XeenMapIdentity b) {
		return a.side == b.side && a.number == b.number;
	}
	friend bool operator!=(XeenMapIdentity a, XeenMapIdentity b) { return !(a == b); }
	friend bool operator<(XeenMapIdentity a, XeenMapIdentity b) {
		return std::tie(a.side, a.number) < std::tie(b.side, b.number);
	}
};

inline std::ostream &operator<<(std::ostream &out, XeenMapIdentity id) {
	return out << id.number << (id.side == XeenSide::Clouds ? " (Clouds)" :
		id.side == XeenSide::Darkside ? " (Darkside)" : " (invalid side)");
}

inline void requireCloudsMap(XeenMapIdentity id) {
	if (id.side != XeenSide::Clouds)
		throw std::invalid_argument("resource adapter supports Clouds maps only");
}

} // namespace mmodern

#endif
