#include "app/XeenEventFlow.h"
#include "app/XeenAudio.h"
#include "games/xeen/XeenEndingScore.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenScriptedRules.h"
#include <iostream>
namespace mmodern {
void XeenEventFlow::startEnding(XeenEndingRequest kind) {
 if(_endingTailApplied)return;
 if(!journey() || !_encounter->journeyEvent() || _transition || _pending)
  throw std::logic_error("Clouds ending requires a published terminal Event");
 auto &guard=_encounter->journeySavePreimage();guard.check();
 if(kind==XeenEndingRequest::Clouds) {
  // DAT 0x28b98..0x28c0d. The marker precedes flag 75 and relocation.
  guard.cloudsCompleted=true;_party.cloudsCompleted=true;guard.adoptMutationBoundary();
  bool won=false;
  try {won=writeWon && writeWon();}catch(const std::exception &) {won=false;}
  guard.check();_endingWonNotice=!won;
  auto camera=_camera;camera.mapId=28;camera.x=18;camera.y=4;
  auto flags=_flags;flags.set(75);
  guard.cameraValue=camera;guard.flagValues=flags.values();
  guard.mapRuleView=xeenMapRuleView(flags,true);
  _flags=flags;_camera=camera;
  // Derived load-rule view only: no map-28 resource, actor or visit is loaded.
  _world.applyMainMapRules(_camera.mapId,_flags,true);
  guard.adoptMutationBoundary();guard.check();
 }
 _endingTailApplied=true;_encounterFrame=_encounter->ticket();
}
IndexedFrame XeenEventFlow::finishEnding() {
 auto &guard=_encounter->journeySavePreimage();guard.check();
 if(_endingWonNotice || !_endingError.empty()) {
  const auto message=_endingWonNotice?"Win settings failed.\nEnter: continue.":_endingError;
  _frame=drawXeenErrorScroll(_endingBase,_inventoryFont,message);
  auto returned=_frame;sealFrame(returned);_encounterFrame=_encounter->ticket();return returned;
 }
 if(_endingScore)return frameCopy();
 try {forcedJourneySave(true);}
 catch(const std::exception &e) {
  guard.check();
  // One frozen first-attempt value, independent of time spent acknowledging I/O errors.
  if(_endingSnapshot && !_endingFrozenScore)
   _endingFrozenScore=xeenCloudsEndingScore(_party,_program.playTime(_party.playTimeSeconds));
  std::cerr<<"Ending save failed: "<<e.what()<<'\n';
  _endingError="Ending save failed.\nEnter: retry.";
  return finishEnding();
 }
 guard.check();
 if(_program.audio)_program.audio->fadeSong(207,presentationNow());
 guard.soundOptions.efx=guard.soundOptions.music=true;
 _party.soundOptions=guard.soundOptions;guard.adoptMutationBoundary();
 if(_program.audio)_program.audio->observeOptions(_party.soundOptions);
 _endingScore=_endingFrozenScore ? *_endingFrozenScore : xeenCloudsEndingScore(_party,_program.playTime(_party.playTimeSeconds));
 guard.check();_encounterFrame=_encounter->ticket();return frameCopy();
}
}
