#ifndef MMODERN_CD_MYRA_TEST_SUPPORT_H
#define MMODERN_CD_MYRA_TEST_SUPPORT_H
#include "games/xeen/XeenEventInterpreter.h"
#include "games/xeen/XeenTextRenderer.h"
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
    s.state.pendingRewards.hasWork() || s.state.rewardReceipt.count)
  throw std::runtime_error("CD Myra PlayCD presentation/continuation differs");
}
// Those PlayCD records leave the explicit deferral in a passive top strip.
// Independent renderer bounds: the notice must survive NPC dismissal/rebase,
// with no pixel or palette change outside x=[0,224), y=[0,8).
inline IndexedFrame withNotice(const XeenFontFormat &font,const IndexedFrame &base) {
 XeenTextRenderOptions options;
 options.bounds={0,0,224,8};options.x=0;options.y=0;options.alignmentAnchor=112;
 options.size=XeenFontSize::Reduced;options.alignment=XeenTextAlignment::Center;
 const auto rendered=XeenTextRenderer(font).render(base,"CD speech not supported yet",options);
 if(!rendered.diagnostics.empty() || rendered.pages.size()!=1)
  throw std::runtime_error("CD speech strip oracle failed to render");
 const auto &frame=rendered.pages.front();bool changed=false;
 for(int y=0;y<200;++y)for(int x=0;x<320;++x)if(frame.pixels[y*320+x]!=base.pixels[y*320+x]) {
  changed=true;if(x>=224 || y>=8)throw std::runtime_error("CD speech strip escaped bounds");
 }
 if(!changed || frame.palette!=base.palette)throw std::runtime_error("CD speech strip invisible/palette changed");
 return frame;
}
}
#endif
