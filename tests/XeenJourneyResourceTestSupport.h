#pragma once
#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenSaveState.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <stdexcept>
namespace journey_resources_test {
using namespace mmodern;
inline void require(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
inline void run(const std::function<XeenPartyState()> &initial,const XeenJourneySetup &setup,
 const XeenWorld::MapLoader &maps,const XeenWorld::ObjectLoader &objects,XeenSaveResourceSignature signature) {
 const auto home=xeenJourneyContent(setup.contract).entry.mapId;
 const XeenMapIdentity neighbor{XeenSide::Clouds,static_cast<std::uint16_t>(home.number-1)};
 XeenEventPresenter::Clock clock=[]{return 0;};
 XeenSaveSnapshot source;
 {auto p=initial();auto c=xeenJourneyContent(setup.contract).entry;XeenGameFlags f;XeenWorld w(maps,objects);
  XeenEncounterFlow flow(w,p,c,f,clock,setup);
  require(flow.prepareJourneyFrame(flow.ticket(),[]{}) && flow.presentJourney(flow.ticket()),"Source frame");
  source=XeenSaveState::capture(signature,p,c,f,w);
 }
 for(bool restored:{false,true})for(unsigned operation=0;operation<4;++operation)
 for(unsigned partial=0;partial<3;++partial)for(unsigned change=0;change<13;++change) {
  bool altered=false;
  // 1..4: content; 5..8: internal identity; 9..12: recoverable provider faults.
  // Each group visits home DAT/MOB and neighboring DAT/MOB independently.
  const unsigned kind=change ? (change-1)%4+1 : 0;
  XeenWorld w([&](auto id){auto m=maps(id);if(altered && ((kind==1 && id==home)||(kind==3 && id==neighbor))) {
    if(change>=9){if(kind==1)throw std::runtime_error("Transient DAT I/O");throw std::bad_alloc();}
    if(change>=5){++m.geometry.id;return m;}
    // Change real traversal geometry, not an ignored fixture-only field.
    auto &cell=m.geometry.cells[11*16+8];auto &layers=std::get<XeenOutdoorLayers>(cell.geometry);
    layers.middle=layers.middle==1 ? 0 : 1;cell.rawWord^=0x10;
   }return m;},[&](auto id){auto o=objects(id);if(altered && ((kind==2 && id==home)||(kind==4 && id==neighbor))) {
    if(change>=9){if(kind==4)throw std::runtime_error("Transient MOB I/O");throw std::bad_alloc();}
    if(change>=5){++o.mapId.number;return o;}
    o.entities.monsterTable[0]^=1;
   }return o;});
  XeenPartyState p;auto c=xeenJourneyContent(setup.contract).entry;XeenGameFlags f;
  std::unique_ptr<XeenEncounterFlow> flow;
  if(restored) {
   XeenSaveState::Resources resources{signature,{},[&](auto){return setup.events;},{},{},[&]{return setup.statistics;},setup.regionalManifest};
   XeenSaveState::restoreBeforeGameplay(source,resources,p,c,f,w,[](auto &,const auto &,const auto &,const auto &){});
   flow=std::make_unique<XeenEncounterFlow>(w,p,c,f,clock,XeenJourneyRestoreTag{});
  } else {p=initial();flow=std::make_unique<XeenEncounterFlow>(w,p,c,f,clock,setup);}
  const auto loadAll=[&]{(void)w.map(home);(void)w.objectFile(home);(void)w.map(neighbor);(void)w.objectFile(neighbor);};
  require(flow->prepareJourneyFrame(flow->ticket(),loadAll) && flow->presentJourney(flow->ticket()),"Admit all immutable resources");
  w.discardMapCache();
  require(w.cachedMapCount()==0 && w.cachedObjectFileCount()==0,"Caches remain disposable");
  // Reconstruct just one kind under authority; renewal must also keep missing entries.
  if(partial){flow->holdJourneyFrame();require(flow->prepareJourneyFrame(flow->ticket(),[&]{if(partial==1)(void)w.map(home);else (void)w.objectFile(neighbor);}) && flow->presentJourney(flow->ticket()),"Partial frame");}
  if(operation==0)require(flow->journeyEquipment(flow->ticket(),0,XeenInventoryCategory::Weapons,0,XeenEquipmentOperation::Remove).status==XeenEquipmentStatus::Success,"Authorized equipment publication");
  if(operation==1)require(flow->journeyTransfer(flow->ticket(),5,0,XeenInventoryCategory::Accessories,1).status==XeenTransferStatus::Success,"Authorized transfer publication");
  if(operation==2)require(flow->journeyAction(flow->ticket(),XeenEncounterAction::Right).outcome==XeenEncounterOutcome::Accepted,"Authorized navigation publication");
  if(operation==3)require(flow->journeyPulse(flow->ticket()).outcome==XeenEncounterOutcome::Pulsed,"Authorized actor pulse publication");
  const auto camera=c;const auto context=p.encounterContext;const auto actors=w.sessionState().actors();
  const auto characters=p.roster.characters();const auto random=w.sessionState().journeyRandom();
  // Isolate entries forgotten at renewal even when that publication reloaded home.
  w.discardMapCache();altered=change!=0;flow->holdJourneyFrame();
  const bool prepared=flow->prepareJourneyFrame(flow->ticket(),loadAll);
  if(!change) {
   require(prepared && flow->presentJourney(flow->ticket()) && flow->journeyQuiet() && XeenSaveState::canCapture(p,c,w),"Unchanged reconstruction keeps Quiet/capture");
   (void)XeenSaveFormat::encode(XeenSaveState::capture(signature,p,c,f,w));
  } else {
   require(!prepared && !flow->presentJourney(flow->ticket()) && !flow->journeyQuiet() && !XeenSaveState::canCapture(p,c,w),"Changed reload must reject and close authority");
   require(w.cachedMapCount()==(kind==1 ? 0u : kind==4 ? 2u : 1u) && w.cachedObjectFileCount()==(kind<=2 ? 0u : 1u),"Rejected resource never enters disposable cache");
   altered=false;w.discardMapCache();
   if(change>=9) {
    require(flow->prepareJourneyFrame(flow->ticket(),loadAll) && flow->presentJourney(flow->ticket()) && flow->journeyQuiet() && XeenSaveState::canCapture(p,c,w),"Transient provider failure permits matching retry");
    (void)XeenSaveFormat::encode(XeenSaveState::capture(signature,p,c,f,w));
   } else {
    require(!flow->prepareJourneyFrame(flow->ticket(),loadAll) && !flow->presentJourney(flow->ticket()) && !flow->journeyQuiet() && !XeenSaveState::canCapture(p,c,w),"Matching retry cannot revive integrity failure");
    require(flow->journeyAction(flow->ticket(),XeenEncounterAction::Forward).outcome!=XeenEncounterOutcome::Accepted,"Failed graph cannot traverse changed topology");
   }
  }
  require(xeen_state::sameCamera(c,camera) && p.encounterContext==context,"Reconstruction preserves camera/context");
  require(w.sessionState().journeyRandom()==random,"Reconstruction preserves RNG");
  for(unsigned i=0;i<30;++i)require(xeen_state::sameCharacter(characters[i],p.roster.at(i)),"Authorized item publication survives reconstruction without extra mutation");
  for(unsigned i=0;i<actors.size();++i)require(xeen_state::sameActor(actors[i],w.sessionState().actors()[i]),"Reconstruction preserves actors");
 }
}
}
