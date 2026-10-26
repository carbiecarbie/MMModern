#ifndef MMODERN_XEEN_ENDING_SCORE_H
#define MMODERN_XEEN_ENDING_SCORE_H
#include "games/xeen/XeenParty.h"
#include <cstdint>
#include <string>
namespace mmodern {
std::uint32_t xeenCloudsEndingScore(const XeenPartyState &, std::uint32_t seconds);
std::string xeenFormatEndingScore(std::uint32_t);
}
#endif
