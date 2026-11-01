#ifndef MMODERN_GAMES_XEEN_MAP_IDENTITY_H
#define MMODERN_GAMES_XEEN_MAP_IDENTITY_H

#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string_view>
#include <tuple>

#include "games/xeen/XeenMutation.h"
#include "formats/xeen/XeenSpriteDrawOptions.h"
namespace mmodern {

enum class XeenSide : std::uint8_t { Clouds, Darkside };

// Runtime/loading context, never part of the original binary map format.
struct XeenMapIdentity {
	XeenMutable<XeenSide> side = XeenSide::Clouds;
	XeenMutable<std::uint16_t> number = 0;

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

inline void requireXeenMap(XeenMapIdentity id) {
	if (!id)
		throw std::invalid_argument("Map resources: not supported yet");
}

enum class XeenMapMusic { Clouds, DarksideRandom };

// Derived from the logical ROOT identity, never from a neighboring DAT tile.
// DOS evidence: docs/rules/evidence.md, M104 Part 0 (0xf62a..0x1077d).
// Table identities name files in DARK.CC, independently of the art archives.
struct XeenMapResources {
	XeenSide mapArchive, monsterTable, animationTable;
	// The pin loads _wallPicSprites but has no subsequent consumer. Scene
	// animation uses .pic frame headers; export still preflights this file.
	XeenSide pictureMetadata;
	XeenSceneArchive monsterArt, objectArt, pictures;
	XeenSceneArchive walls, sky, ground, tiles, surfaces;
	XeenSceneArchive voices, ambient, musicArchive;
	XeenSide townPortal;
	XeenMapMusic music;
	unsigned missingCellSlot;
	bool hideHorizon, alwaysDay;
};

inline XeenMapResources xeenMapResources(XeenMapIdentity id) {
	const bool dark=id.side==XeenSide::Darkside;
	const unsigned n=id.number;
	const auto logical=dark?XeenSceneArchive::Darkside:XeenSceneArchive::Clouds;
	const bool mixed=dark && n>=113 && n<=128;
	const bool sphinx=mixed && n>=125 && n<=127;
	const bool cloudsMonsters=!dark || (mixed && ((n>=117 && n<=120) || n==124));
	const bool cloudsObjects=!dark || (mixed && (n<=120 || n>=124));
	const auto scenery=sphinx?XeenSceneArchive::Clouds:logical;
	const bool cloudsMusic=!dark || sphinx;
	const unsigned missing=!dark?(n>=25 && n<=27?7u:0u):
		((n>=53 && n<=88 && n!=73) || (n>=113 && n<=120) ||
		 n==125 || n==126 || n==128 || n==129?6u:0u);
	return {id.side,cloudsMonsters?XeenSide::Clouds:XeenSide::Darkside,
		cloudsObjects?XeenSide::Clouds:XeenSide::Darkside,
		sphinx || !dark?XeenSide::Clouds:XeenSide::Darkside,
		cloudsMonsters?XeenSceneArchive::Clouds:XeenSceneArchive::Darkside,
		cloudsObjects?XeenSceneArchive::Clouds:XeenSceneArchive::Darkside,
		sphinx?XeenSceneArchive::Clouds:logical,
		scenery,scenery,scenery,scenery,scenery,logical,logical,
		cloudsMusic?XeenSceneArchive::Clouds:XeenSceneArchive::DarksideOnly,
		mixed && n<=127?XeenSide::Clouds:static_cast<XeenSide>(id.side),
		cloudsMusic?XeenMapMusic::Clouds:XeenMapMusic::DarksideRandom,missing,
		(sphinx || !dark)?n>=25 && n<=27:(n>=89 && n<=112) || n==128 || n==129,
		(n>=89 && n<=112) || n==128 || n==129};
}

inline XeenSceneArchive xeenMonsterArt(XeenMapIdentity root,unsigned species) {
	return species==91?XeenSceneArchive::Clouds:xeenMapResources(root).monsterArt;
}

// Map/minimap tiles pass through the dialog sprite provider alongside UI art.
inline XeenSceneArchive xeenDialogSpriteArchive(XeenMapIdentity root,std::string_view name) {
	const auto resources=xeenMapResources(root);
	return name.size()>=4 && name.substr(name.size()-4)==".til"?resources.tiles:resources.ambient;
}

} // namespace mmodern

#endif
