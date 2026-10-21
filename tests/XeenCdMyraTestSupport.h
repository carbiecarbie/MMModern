#ifndef MMODERN_CD_MYRA_TEST_SUPPORT_H
#define MMODERN_CD_MYRA_TEST_SUPPORT_H
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenTextRenderer.h"
#include "games/xeen/XeenDialogView.h"
#include <stdexcept>

namespace cd_myra_test {
using namespace mmodern;
// CD maze0023.evt (9,11,West): PlayCD line 5/offset 261 precedes
// request NPC line 6/272; PlayCD line 9/299 precedes return NPC line 10/310.
// Each branch executes one extra instruction: request 6, return/receipt 10.
inline constexpr unsigned requestCount=6, returnCount=10;
inline void checkAudio(const XeenEventExecutionSuspended &s, bool returning) {
 const auto &q=s.request;
 if(q.kind!=XeenPresentationKind::DeferredAudio ||
    q.response!=XeenPresentationResponseRequirement::Presented ||
    q.mapId!=XeenMapIdentity{23} || q.source.opcode!=0x3c ||
    q.source.line!=(returning?9:5) || q.source.fileOffset!=(returning?299:261) ||
    q.text!="CD speech not supported yet" || q.npc ||
    s.state.instructionCount!=(returning?2U:3U) ||
    s.state.logicalAddress.line!=(returning?10:6) ||
    s.state.treasureChanged() || s.state.reward.receipt.count)
  throw std::runtime_error("CD Myra PlayCD presentation/continuation differs");
}
// The explicit unsupported-audio notice is passive in original error window 6.
inline IndexedFrame withNotice(const XeenFontFormat &font,const IndexedFrame &base) {
 return drawXeenErrorScroll(base,font,"CD speech not supported yet");
}

}
#endif
