#ifndef MMODERN_XEEN_MERCHANT_TABLES_H
#define MMODERN_XEEN_MERCHANT_TABLES_H
// Numeric reason-0 merchant generation adapted from the ScummVM developers'
// GPL-3.0-or-later Character::makeItem and create_xeen constants.cpp, revision
// 6814ee9ba54582f5b5adcffab49efbbd8f589edd. Corresponding upstream source:
// https://github.com/scummvm/scummvm/tree/6814ee9ba54582f5b5adcffab49efbbd8f589edd
namespace mmodern::merchant {
struct Interval { unsigned lo,hi; };
inline constexpr unsigned counts[2][4][4]={
	{{15,5,5,5},{5,10,5,5},{0,5,10,5},{0,0,0,5}},
	{{10,5,0,5},{10,5,5,5},{0,5,5,10},{0,5,10,0}}};
inline constexpr Interval material[2][5]={
	{{1,4},{3,7},{4,8},{5,9},{8,9}},
	{{1,4},{2,6},{4,7},{6,10},{9,13}}};
inline constexpr Interval elements[6][5]={
	{{1,3},{2,5},{3,6},{4,7},{5,8}},
	{{1,3},{2,5},{3,6},{4,7},{6,7}},
	{{1,2},{1,3},{2,4},{3,5},{4,5}},
	{{1,2},{1,3},{2,4},{3,4},{4,5}},
	{{1,3},{2,5},{3,6},{4,7},{5,8}},
	{{1,1},{1,1},{1,2},{2,2},{2,3}}};
inline constexpr unsigned elementThresholds[6]={25,45,60,75,95,100};
inline constexpr unsigned elementOffsets[6]={0,8,15,20,25,33};
inline constexpr Interval attributes[10][5]={
	{{1,4},{2,5},{3,6},{4,7},{6,10}},
	{{1,3},{2,5},{3,6},{4,7},{5,8}},
	{{1,3},{2,5},{3,6},{4,7},{5,8}},
	{{1,3},{2,5},{3,6},{4,7},{5,8}},
	{{1,2},{1,3},{2,4},{3,5},{4,6}},
	{{1,2},{2,3},{3,4},{4,5},{5,6}},
	{{1,2},{1,3},{2,4},{3,4},{4,5}},
	{{1,2},{1,3},{2,4},{3,5},{4,6}},
	{{1,2},{1,3},{2,4},{3,4},{4,5}},
	{{1,2},{1,4},{3,6},{5,8},{7,10}}};
inline constexpr unsigned attributeThresholds[10]={15,25,35,50,65,80,85,90,95,100};
inline constexpr unsigned attributeOffsets[10]={0,10,18,26,34,40,46,51,57,62};
inline constexpr Interval specials[5]={{1,15},{16,30},{31,40},{41,50},{51,60}};
inline unsigned level(unsigned side,unsigned shop,unsigned band) noexcept { return band+(side==1 && shop>=2?3:1); }
}
#endif
