#pragma once
#include "formats/xeen/XeenAudioConfig.h"
#include <array>
#include <optional>

namespace mmodern {
// Visible installer keys, in display order (1..9,0). PAS Alt follows key 4.
struct XeenSoundRow {
 const char *caption;
 unsigned id, musicPort;
 std::vector<unsigned> ports, irqs, dmas;
 bool volume=false;
};
const std::array<XeenSoundRow,10> &xeenSpeechMenu();
const std::array<XeenSoundRow,10> &xeenMusicMenu();
struct XeenSoundAnswers {
 unsigned speech=0,music=0; // display-order indices, not CFG ids
 std::optional<unsigned> digitalPort,musicPort,irq,dma,volume;
};
void xeenValidateSetupBase(const std::vector<std::uint8_t> &);
std::vector<std::uint8_t> xeenApplySoundSetup(const std::vector<std::uint8_t> &,const XeenSoundAnswers &);
std::string xeenSoundCaption(unsigned id,bool music);
std::string xeenSoundStatus(const XeenAudioConfig &,bool music);
}
