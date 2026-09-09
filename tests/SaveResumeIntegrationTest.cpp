#include "XeenCheckpointTestSupport.h"
#include "XeenChildProcessTestSupport.h"
#include "XeenVisualRemoveTestSupport.h"
#include "XeenPartySnapshotTestSupport.h"
#include "XeenSaveTestSupport.h"
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "platform/XeenSaveFile.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include <algorithm>
#include <set>

using namespace mmodern;
using remove_test::check;
namespace fs = std::filesystem;
namespace cp = checkpoint_test;
namespace {
XeenCamera position(const std::string &name) {
 if (name == "phirna") return cp::phirna;
 if (name == "whistle" || name == "cumulative") return cp::whistle;
 check(name == "myra" || name == "myra-exchange", "unknown checkpoint"); return cp::myra;
}
std::vector<std::uint8_t> diskBytes(const fs::path &path) {
 std::ifstream input(path, std::ios::binary); check(bool(input), "evidence file missing");
 return {std::istreambuf_iterator<char>(input), {}};
}
bool sameItem(XeenItem a, XeenItem b) {
 return a.material == b.material && a.id == b.id && a.state == b.state && a.frame == b.frame;
}
void recipientPrerequisites(const XeenPartyState &p) {
 check(p.party.activeRosterIds() == std::vector<std::uint8_t>({0,18,14,11,1,6}), "original ordered membership prerequisite");
 for (const auto id : p.party.activeRosterIds()) {
  const auto &c = p.roster.at(id);
  check(c.canAct() && c.miscellaneous.back().id == 0, "live eligibility/miscellaneous tail prerequisite");
 }
}
void originalItems(const XeenPartyState &p) {
 recipientPrerequisites(p);
 unsigned occupied = 0;
 for (const auto &c : p.roster.characters()) {
  for (const auto *category : {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous})
   for (const auto item : *category) occupied += item.id != 0;
  for (const auto item : c.miscellaneous)
   check(sameItem(item, {}), "original miscellaneous empty-slot bytes prerequisite");
 }
 check(occupied == 35, "original occupied record prerequisite");
}
void cleanPresentation(const XeenEventFlow &flow, bool startup = false) {
 const auto &t = flow.presenter().npcTiming();
 // blocksGameplay includes the owned continuation, not a test-side report copy.
 check(!flow.blocksGameplay() && !flow.presentationGeneration() && !flow.canCancelInteraction() &&
  !flow.presenter().blocksGameplay(),
  "live NPC/reward continuation, generation or timing survived");
 if (startup) check(flow.presenter().pageCount() == 0 && !t.displayedFrame && !t.nextFrame &&
  !t.phase && !t.remaining && !t.deadline, "startup inherited pages or portrait timing");
}
void exactReceipt(const XeenEventExecutionSuspended &s) {
 const auto &r = s.state.rewardReceipt;
 check(s.request.kind == XeenPresentationKind::RewardReceipt && s.state.instructionCount == 9 &&
  s.state.rewardPhase == XeenRewardPhase::Receipt && !s.state.pendingRewards.hasWork() &&
  r.count == 5 && r.delivered == 5 && !r.lost && !r.overflow && !r.invalid && !r.discarded &&
  r.discardReason == XeenRewardDiscard::None, "exchange finalization/accounting differs");
 for (unsigned i = 0; i < 5; ++i)
  check(sameItem(r.entries[i].item, {10,37,1,0}) && r.entries[i].owner == 0 &&
   r.entries[i].loss == XeenRewardLoss::None, "exchange receipt record/owner differs");
}
void equalFrame(const IndexedFrame &a, const IndexedFrame &b) {
 check(a.isValid() && a.width == 320 && a.height == 200 && a.pixels == b.pixels && a.palette == b.palette,
  "native scene/palette mismatch");
}
int child(const fs::path &game, const fs::path &dir, const std::string &name, const std::string &role, bool sdl,
 const std::optional<fs::path> &manualTarget = {}) {
 const bool manual = manualTarget.has_value(), exchange = name == "myra-exchange";
 const bool resume = role == "consumer", produce = role == "producer";
 check(!manual || (produce && exchange), "manual role/checkpoint mismatch");
 check(resume || produce || role == "fresh", "unknown child role");
 const auto installation = XeenInstallationDetector().detect(game);
 check(installation && installation->hasXeen(), "original installation unavailable");
 XeenAssetSource assets(*installation, 320, 200);
 const auto defaults = XeenPartyLoader().loadInitialCloudsParty(assets);
 if (exchange) originalItems(defaults);
 auto expectedCharacters = defaults.roster.characters();
 if (exchange && resume) for (unsigned i = 0; i < 5; ++i) expectedCharacters[0].miscellaneous[i] = {10,37,1,0};
 const auto defaultFlags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
 check(defaults.questItems.at(17) == 0 && defaults.questItems.at(18) == 0 && !defaults.questFlags.isSet(2), "unexpected original checkpoint prerequisites");
 const auto path = XeenSaveFile::resolve(manualTarget.value_or(dir/(name + ".mmsave")), installation->root);
 if (produce) check(!fs::exists(path), "stale producer save refused");
 bool root = resume && (name == "phirna" || name == "cumulative");
 bool phirnaRemoved = resume && (name == "phirna" || name == "cumulative" || exchange);
 bool bone = resume && (name == "whistle" || name == "cumulative");
 bool request = resume && (name == "myra" || name == "cumulative");
 XeenCamera expectedCamera = produce && name == "cumulative" ? cp::myra : position(name);
 const XeenMapLoader maps;
 const XeenEventLoader scripts([&](const std::string &resource)->std::optional<std::vector<std::uint8_t>> {
  if (!assets.hasInitialResource(resource)) return {}; return assets.readInitialResource(resource);
 });
 const XeenEventTextLoader texts([&](const std::string &resource)->std::optional<std::vector<std::uint8_t>> {
  if (!assets.hasArchiveResource(resource)) return {}; return assets.readArchiveResource(resource);
 });
 const auto base23 = scripts.load(23), base20 = scripts.load(20);
 const auto geometry23 = remove_test::geometrySnapshot(maps.loadGeometryMap(assets, 23).geometry);
 const auto geometry20 = remove_test::geometrySnapshot(maps.loadGeometryMap(assets, 20).geometry);
 const auto objects23 = maps.loadObjects(assets, 23), objects20 = maps.loadObjects(assets, 20);
 const auto resolver = XeenObjectVisualResolver::load(assets);
 const XeenFontFormat font(assets.readArchiveResource("fnt"));
 const CloudsMapComposer composer;
 int mapLoads = 0, objectLoads = 0, scriptLoads = 0, textLoads = 0, automatic = 0, compositions = 0;
 XeenWorld *world = nullptr; XeenEventSystem *events = nullptr; const XeenPartyState *party = nullptr;
 XeenCamera *camera = nullptr; const XeenGameFlags *flags = nullptr; XeenEventFlow *flow = nullptr;
 std::optional<XeenManualEventResult> terminal; int presentations = 0, receiptReports = 0;
 std::optional<XeenEventExecutionSuspended> pending;
 auto partyCheck = [&](const XeenPartyState &p) {
  check(p.party.activeRosterIds() == defaults.party.activeRosterIds(), "active membership/order changed");
  for (std::size_t i = 0; i < 30; ++i) remove_test::checkSameCharacter(p.roster.characters()[i], expectedCharacters[i]);
  auto counts = defaults.questItems.counts(); counts[17] += root; counts[18] += bone;
  auto quests = defaults.questFlags.values(); quests[2] = request;
  remove_test::checkPartyQuestState(p, counts, quests);
 };
 auto worldCheck = [&](XeenWorld &w) {
  std::set<XeenObjectIdentity> objects; std::set<XeenEventIdentity> records;
  if (phirnaRemoved) { objects.insert({23,13}); for (int i = 125; i <= 135; ++i) records.insert({23,static_cast<std::size_t>(i)}); }
  if (bone) { objects.insert({20,1}); for (int i = 1; i <= 5; ++i) records.insert({20,static_cast<std::size_t>(i)}); }
  check(w.sessionState().disabledObjects() == objects && w.sessionState().disabledEvents() == records, "exact independent multi-map removal identities differ");
  for (const auto &entry : std::vector<std::pair<XeenMapIdentity, const XeenEventFile *>>{{23,&base23},{20,&base20}}) {
   for (std::size_t i = 0; i < entry.second->records.size(); ++i) {
    const auto &original = entry.second->records[i]; const auto effective = w.effectiveEvent({entry.first,i}, original);
    if (records.count({entry.first,i})) check(effective.opcode == 0 && remove_test::sameRecord(original, effective, false), "removed record not effective None with original metadata");
    else check(remove_test::sameRecord(original, effective), "unrelated effective event changed");
   }
  }
  for (const auto &entry : std::vector<std::pair<XeenCamera, XeenObjectIdentity>>{{cp::phirna,{23,13}},{cp::whistle,{20,1}}}) {
   const bool removed = objects.count(entry.second);
   const auto selected = w.selectObject(entry.first);
   check(removed ? !(selected == entry.second) : selected == entry.second, "selection identity differs");
   const auto commands = XeenOutdoorScene().build(w, entry.first, &resolver);
   const bool drawn = std::any_of(commands.begin(), commands.end(), [&](const auto &c) { return c.object() && c.object()->visual.identity == entry.second; });
   check(drawn != removed, "object draw identity differs");
  }
  check(remove_test::geometrySnapshot(w.map(23).geometry) == geometry23 && remove_test::geometrySnapshot(w.map(20).geometry) == geometry20, "original geometry changed");
  remove_test::sameEntities(objects23.entities, w.objectFile(23).entities);
  remove_test::sameEntities(objects20.entities, w.objectFile(20).entities);
 };
 auto stateCheck = [&] {
  check(party && flags && camera && world, "live observer missing"); partyCheck(*party); worldCheck(*world);
  check(flags->values() == defaultFlags.values(), "unrelated game flags changed");
  check(cp::sameCamera(*camera, expectedCamera), "saved/controlled camera differs");
 };
 const auto signature = XeenSaveFile::fingerprint(*installation);
 // Independent expected values: original defaults plus explicitly enumerated
 // event effects. Never capture tested owners or invoke delivery for this oracle.
 auto expectedSnapshot = [&] {
  XeenSaveSnapshot s; s.resources = signature; s.camera = expectedCamera;
  s.activeRosterIds = defaults.party.activeRosterIds(); s.characters = expectedCharacters;
  s.questItems = defaults.questItems.counts(); s.questItems[17] += root; s.questItems[18] += bone;
  s.questFlags = defaults.questFlags.values(); s.questFlags[2] = request; s.gameFlags = defaultFlags.values();
  if (bone) { s.disabledObjects.push_back({20,1}); for (unsigned i=1;i<=5;++i) s.disabledEvents.push_back({20,i}); }
  if (phirnaRemoved) { s.disabledObjects.push_back({23,13}); for (unsigned i=125;i<=135;++i) s.disabledEvents.push_back({23,i}); }
  return s;
 };
 std::uint64_t observedPhase=0, ordinaryNow=0;
 const auto initialDisk = resume ? diskBytes(path) : std::vector<std::uint8_t>{};
 XeenGameplayServices services{
  {signature, [&] { return XeenPartyLoader().loadInitialCloudsParty(assets); }, [&](XeenMapIdentity id) { ++scriptLoads; return scripts.load(id); }},
  [&] { return XeenGameFlagsLoader().loadInitialCloudsFlags(assets); },
  [&](XeenMapIdentity id) { ++mapLoads; return maps.loadGeometryMap(assets, id); },
  [&](XeenMapIdentity id) { ++objectLoads; return maps.loadObjects(assets, id); },
  [&](XeenMapIdentity id) { ++textLoads; return texts.load(id); }, font,
  [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, std::uint64_t phase) {
   // Includes resume preflight and constructor composition, before show().
   if (!world) { partyCheck(p); worldCheck(w); check(cp::sameCamera(c, expectedCamera), "first composition used default camera"); }
   if(world==&w)observedPhase=phase;
   ++compositions; XeenEventFlow::Composition result;
   result.frame = composer.compose(assets, w, p, c, {kCloudsInitialYear}, nullptr, phase, &result.containsOrdinaryAnimation);
   return result;
  },
  [&](IndexedFrame &f, std::uint8_t portrait, std::size_t index) { assets.drawNpc(f, portrait, index); },
  [&](XeenEventFlow &f, const XeenCamera &) {
   flow = &f;
   f.reportText = [](const std::string &s) { throw std::runtime_error(s); };
   f.reportAutomatic = [&](const XeenAutomaticEventResult &r) { ++automatic; check(std::holds_alternative<XeenAutomaticEventNoTrigger>(r), "unexpected automatic checkpoint replay"); };
   f.reportManual = [&](const XeenManualEventResult &r) {
    check(f.blocksGameplay(), "reporting boundary exposed an idle/save-safe gap");
    if (const auto *s = std::get_if<XeenEventExecutionSuspended>(&r)) {
     ++presentations; pending = *s;
     if (s->request.kind == XeenPresentationKind::RewardReceipt) ++receiptReports;
    }
    else terminal = r;
   };
  }, {},
  [&](XeenWorld &w, XeenEventSystem &e, const XeenPartyState &p, XeenCamera &c, const XeenGameFlags &f) {
   world = &w; events = &e; party = &p; camera = &c; flags = &f;
   stateCheck();
   save_test::sameSnapshot(expectedSnapshot(), XeenSaveState::capture(signature, p, c, f, w));
   cleanPresentation(*flow, true);
  }
 };
 if(!manual)services.clock=[&]{return ordinaryNow;};
 services.show = [&](const IndexedFrame &first, const auto &handle, const auto &escape, const auto &idle, const auto &status) {
  check(compositions >= (resume ? 2 : 1) && automatic == (resume ? 0 : 1), "startup composition/dispatch count");
  stateCheck(); cleanPresentation(*flow, true);
  equalFrame(first, composer.compose(assets, *world, *party, *camera, {kCloudsInitialYear}, nullptr, observedPhase));
  visual_remove_test::save(first, dir/(name + "-" + role + "-first.bmp"));
  auto cleanBase=[&]{return composer.compose(assets,*world,*party,*camera,{kCloudsInitialYear},nullptr,observedPhase);};
  auto tick=[&]{if(!manual)ordinaryNow+=25;return idle();};
  if(!manual && cp::sameCamera(*camera,cp::myra)) {
   check(observedPhase==0,"original fresh/restored first phase");std::vector<IndexedFrame> cycle{first};
   auto sample=[&]{
    const auto commands=XeenOutdoorScene().build(*world,*camera,&resolver,nullptr,observedPhase);
    const auto target=std::find_if(commands.begin(),commands.end(),[](const auto &c){return c.object()&&c.object()->visual.identity==XeenObjectIdentity{23,1};});
    check(target!=commands.end()&&target->object()->visual.frame==observedPhase%3,"original startup selected-frame cycle");
    cycle.push_back(flow->frame());
   };
   if(sdl){const auto began=SDL_GetTicks64();bool done=false;
    check(SdlWindow().showInteractive(first,"Original startup animation",{}, {},[&]()->std::optional<IndexedFrame>{
     check(SDL_GetTicks64()-began<5000,"original startup idle watchdog");const auto before=observedPhase;auto frame=tick();
     if(observedPhase!=before)sample();if(observedPhase==3&&!done){done=true;SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);}return frame;
    })&&done,"original startup SDL animation");
   }else for(int i=0;i<3;++i){ordinaryNow+=100;idle();sample();}
   check(cycle.size()==4 && std::any_of(cycle.begin()+1,cycle.end(),[&](const auto &frame){return frame.pixels!=cycle.front().pixels;}),"original fresh/restored animation pixels");
   stateCheck();cleanPresentation(*flow);std::cout<<"ASSERT original "<<role<<" Myra startup phase 0, idle 1/2/3, selected 0/1/2/0; no initial replay\n";
  }
  // Compare immediately around Application F9, before any later SDL idle tick
  // can legitimately animate an NPC. Actual before/after values are mutation
  // checks only; the independent durable oracle remains expectedSnapshot().
  auto checkedHandle = [&](const PlayerAction &a) -> std::optional<IndexedFrame> {
   if (!std::holds_alternative<SaveGameAction>(a)) return handle(a);
   const auto before = XeenSaveState::capture(signature, *party, *camera, *flags, *world);
   const auto frame = flow->frame(); const auto generation = flow->presentationGeneration();
   const auto page = flow->presenter().pageIndex(), pages = flow->presenter().pageCount();
   const auto timing = flow->presenter().npcTiming(); const auto reports = presentations;
   const auto blocked = flow->blocksGameplay();
   const auto result = handle(a);
   save_test::sameSnapshot(before, XeenSaveState::capture(signature, *party, *camera, *flags, *world));
   equalFrame(frame, flow->frame());
   const auto after = flow->presenter().npcTiming();
   check(flow->presentationGeneration() == generation && flow->presenter().pageIndex() == page &&
    flow->presenter().pageCount() == pages && presentations == reports && flow->blocksGameplay() == blocked &&
    timing.displayedFrame == after.displayedFrame && timing.nextFrame == after.nextFrame &&
    timing.phase == after.phase && timing.remaining == after.remaining && timing.deadline == after.deadline,
    "F9 changed page/continuation/generation/timing");
   if (blocked) {
    check(status().find("Cannot save while an interaction is pending") != std::string::npos, "production refusal feedback absent");
    if (produce) check(!fs::exists(path), "refused F9 wrote a save");
    std::cout << "ASSERT immediate F9 refusal: state/page/generation/timing unchanged\n" << std::flush;
   }
   return result;
  };
  std::function<std::optional<IndexedFrame>(const PlayerAction &)> inputHandler = checkedHandle;
  // SDL uses the actual Application handler, escape and idle callbacks. No
  // alternate startup/save choice. Each bounded input batch owns its window.
  auto drive = [&](const std::vector<cp::Input> &inputs) {
   if (!sdl) { for (const auto &input : inputs) inputHandler(input.action); return; }
   std::size_t index = 0; bool queued = false, quit = false; const auto began = SDL_GetTicks64();
   const bool ok = SdlWindow().showInteractive(flow->frame(), status(), [&](const PlayerAction &a) {
    check(index < inputs.size() && a.index() == inputs[index].action.index(), "SDL action order/repeat");
    if (const auto *m = std::get_if<SelectMemberAction>(&a)) check(m->partyIndex == std::get<SelectMemberAction>(inputs[index].action).partyIndex, "SDL selection index");
    auto frame = inputHandler(a); ++index; queued = false; return frame;
   }, escape, [&]()->std::optional<IndexedFrame> {
    check(SDL_GetTicks64() - began < 5000, "SDL checkpoint input timeout");
    if (!queued && index < inputs.size()) {
     SDL_Event e{}; e.type = SDL_KEYDOWN; e.key.keysym.sym = inputs[index].key; check(SDL_PushEvent(&e) == 1, "SDL push");
     e.key.repeat = 1; check(SDL_PushEvent(&e) == 1, "SDL repeat push"); queued = true;
    } else if (index == inputs.size() && !quit) { SDL_Event e{}; e.type = SDL_QUIT; check(SDL_PushEvent(&e) == 1, "SDL quit push"); quit = true; }
    return tick();
   }, status);
   check(ok && index == inputs.size(), "SDL checkpoint batch failed");
  };
  drive({});
  auto move = [&](XeenCamera c) { check(!flow->blocksGameplay(), "harness positioning during interaction"); *camera = c; expectedCamera = c; flow->refresh(); };
  auto completed = [&](std::size_t count) {
   const auto *done = terminal ? std::get_if<XeenManualEventCompleted>(&*terminal) : nullptr;
   check(done && done->instructionCount == count && !flow->blocksGameplay(), "original completed instruction count");
   std::cout << "ASSERT completed " << count << " instructions\n";
  };
  if (produce && exchange) {
   enum class Phase { Request, Phirna, Return, Receipt, Save, Done };
   Phase phase = Phase::Request;
   bool npcF9 = false, receiptF9 = false, idleAfterReceipt = false;
   unsigned receipts = 0;
   auto announce = [&](const char *message) { std::cout << "PHASE " << message << '\n' << std::flush; };
   announce("Myra request: press Space, then acknowledge both original pages with Space/Enter/Escape.");
   auto observedIdle = [&]() -> std::optional<IndexedFrame> {
    auto frame = tick();
    if (phase == Phase::Save) {
     cleanPresentation(*flow); stateCheck();
     check(!fs::exists(path), "refused save was deferred until idle");
     idleAfterReceipt = true;
    }
    return frame;
   };
   // Both automated input and the continuous physical loop use these exact
   // transitions. Camera changes occur only AFTER Application's callback exits.
   inputHandler = [&](const PlayerAction &a) -> std::optional<IndexedFrame> {
    if (phase == Phase::Done) {
     auto result = checkedHandle(a); stateCheck();
     save_test::sameSnapshot(expectedSnapshot(), XeenSaveFile::read(path));
     return result;
    }
    stateCheck();
    const bool save = std::holds_alternative<SaveGameAction>(a);
    const bool blocked = flow->blocksGameplay();
    const auto priorPhase = phase;
    const auto generation = flow->presentationGeneration();
    const auto page = flow->presenter().pageIndex(), pages = flow->presenter().pageCount();
    const bool ack = std::holds_alternative<AcknowledgeAction>(a) ||
     std::holds_alternative<InteractionAction>(a) || std::holds_alternative<CancelInteractionAction>(a);
    if (phase == Phase::Return && !blocked) recipientPrerequisites(*party);
    if (phase == Phase::Save && save) {
     observedIdle();
     check(npcF9 && receiptF9 && idleAfterReceipt, "required pending F9 observations missing");
    }
    if (!blocked) { terminal.reset(); pending.reset(); }
    auto result = checkedHandle(a);
    if (save && blocked) {
     if (phase == Phase::Return) npcF9 = true;
     if (phase == Phase::Receipt) receiptF9 = true;
    }
    if (phase == Phase::Request && terminal) {
     completed(5); request = true; stateCheck(); cleanPresentation(*flow);
     phase = Phase::Phirna; move(cp::phirna); terminal.reset(); pending.reset();
     announce("Phirna positioned (camera only): Space, Y, then acknowledge the harvest.");
     result = flow->frame();
    } else if (phase == Phase::Phirna) {
     if (terminal) {
      completed(18); root = true; phirnaRemoved = true;
      check(request, "Phirna changed the request"); stateCheck();
      phase = Phase::Return; move(cp::myra); terminal.reset(); pending.reset();
      recipientPrerequisites(*party);
      announce("Myra return positioned: Space, then F9 DURING the original return text; observe refusal before acknowledgment.");
      result = flow->frame();
     }
    } else if (phase == Phase::Return && pending && pending->request.kind == XeenPresentationKind::RewardReceipt) {
     check(npcF9 && blocked && ack && page + 1 == pages, "return advanced without required pending F9/final acknowledgment");
     exactReceipt(*pending); check(++receipts == 1 && receiptReports == 1 && !terminal, "duplicate delivery or premature completion");
     root = false; request = false;
     for (unsigned i=0;i<5;++i) expectedCharacters[0].miscellaneous[i] = {10,37,1,0};
     phase = Phase::Receipt;
     visual_remove_test::save(flow->frame(), dir/(name + "-receipt.bmp"));
     announce("Reward receipt: five delivered, zero loss/overflow. Press F9 while pending, then acknowledge every page.");
    } else if (phase == Phase::Receipt) {
     check(receipts == 1 && receiptReports == 1, "receipt exactly-once report count");
     if (flow->blocksGameplay()) {
      check(pending && !terminal, "receipt completed before final page"); exactReceipt(*pending);
      if (ack) check(page + 1 < pages && flow->presenter().pageIndex() == page + 1 &&
       flow->presentationGeneration() == generation, "nonfinal receipt acknowledgment advanced execution");
     } else {
      check(receiptF9 && ack && page + 1 == pages, "receipt skipped required F9 or final acknowledgment");
      completed(9); cleanPresentation(*flow); pending.reset();
      phase = Phase::Save;
      announce("Receipt complete. Press a NEW F9 to save; earlier refused saves must not run later.");
     }
    }
    if (flow->blocksGameplay() && (phase == Phase::Request || phase == Phase::Return)) {
     check(pending && !terminal && pending->request.kind == XeenPresentationKind::NpcAcknowledgment &&
      pending->request.source.line == (phase == Phase::Return ? 7 : 4) &&
      pending->request.source.fileOffset == (phase == Phase::Return ? 244 : 217) &&
      pending->state.instructionCount == (phase == Phase::Return ? 2U : 3U) &&
      flow->presenter().pageCount() == (phase == Phase::Return ? 1U : 2U), "original Myra branch/source/pages differ");
    }
    stateCheck();
    if (save && priorPhase == Phase::Save) {
     check(status().find("MMModern - Saved [") == 0 && fs::exists(path), "production eligible save did not succeed");
     const auto bytes = diskBytes(path); check(bytes.size() > 20 && bytes[8] == 2 && bytes[9] == 0, "production write is not v2");
     save_test::sameSnapshot(expectedSnapshot(), XeenSaveFile::read(path));
     phase = Phase::Done;
     std::cout << "ASSERT production F9 disk oracle PASS " << path.u8string() << '\n';
     announce("Producer checks complete. Exit this window normally before starting the separate load command.");
    } else check(!fs::exists(path), "unexpected or deferred save before eligible final F9");
    return result;
   };
   if (manual) {
    const bool ok = SdlWindow().showInteractive(first, status(), inputHandler, escape, observedIdle, status);
    check(ok && phase == Phase::Done && npcF9 && receiptF9, "manual producer incomplete: early exit or skipped required observation");
    std::cout << "MANUAL producer assertions complete; Gabriel must separately observe the consumer. Save: " << path.u8string() << '\n' << std::flush;
   } else {
    for (unsigned step = 0; phase != Phase::Done; ++step) {
     check(step < 40, "bounded exchange phase driver exhausted");
     if (phase == Phase::Save) { observedIdle(); drive({{SDLK_F9,SaveGameAction{}}}); }
     else if (!flow->blocksGameplay()) drive({{SDLK_SPACE,InteractionAction{}}});
     else if ((phase == Phase::Return && !npcF9) || phase == Phase::Receipt) drive({{SDLK_F9,SaveGameAction{}}});
     if (phase == Phase::Receipt || (flow->blocksGameplay() && phase != Phase::Save && phase != Phase::Done)) {
      if (phase == Phase::Return && !npcF9) continue;
      if (phase == Phase::Phirna && pending->request.response == XeenPresentationResponseRequirement::YesNo)
       drive({{SDLK_y,YesAction{}}});
      else drive({{SDLK_RETURN,AcknowledgeAction{}}});
     }
    }
   }
   cleanPresentation(*flow); stateCheck();
   save_test::sameSnapshot(expectedSnapshot(), XeenSaveFile::read(path));
   return true;
  }
  auto interact = [&](XeenCamera c, bool acquire) {
   move(c); terminal.reset(); pending.reset(); presentations = 0;
   const bool returning = cp::sameCamera(c, cp::myra) && root;
   auto inputs = cp::collection(c);
   std::size_t selected = 0;
   if (cp::sameCamera(c, cp::whistle) && acquire) {
    selected = party->party.size();
    while (selected && !party->party.member(party->roster, selected-1).canAct()) --selected;
    check(selected > 1, "need eligible non-first member for restart selection control"); --selected;
    inputs[1] = {static_cast<SDL_Keycode>(SDLK_F1 + selected), SelectMemberAction{selected}};
   }
   drive({inputs.front()});
   if (flow->blocksGameplay()) {
    check(pending.has_value(), "missing original suspension");
    if (cp::sameCamera(c, cp::whistle)) {
     check(pending->state.activeCharacterIndex == 0, "new WhoWill inherited temporary selection");
    }
    if (cp::sameCamera(c, cp::myra)) {
     check(flow->presenter().pageCount() == (root ? 1U : 2U), "original Myra page count");
     check(pending->request.kind == XeenPresentationKind::NpcAcknowledgment && pending->request.source.line == (root ? 7 : 4) && pending->request.source.fileOffset == (root ? 244 : 217), "original Myra suspension source");
    }
    const auto generation = flow->presentationGeneration();
    const auto page = flow->presenter().pageIndex();
    drive({{SDLK_F9, SaveGameAction{}}});
    check(status().find("Cannot save while an interaction is pending") != std::string::npos && flow->presentationGeneration() == generation && flow->presenter().pageIndex() == page, "pending F9 advanced original interaction");
    if (produce) check(!fs::exists(dir/(name + ".mmsave")), "pending F9 wrote save");
    if (cp::sameCamera(c, cp::myra)) stateCheck(); // Still restored/pre-exchange at the NPC.
    if (cp::sameCamera(c, cp::whistle) && acquire) {
     drive({inputs[1]});
     check(pending && pending->state.activeCharacterIndex == selected && pending->request.source.line == 2 && flow->blocksGameplay() && !flow->canCancelInteraction(), "WhoWill selected context/display/acknowledgment");
     drive({inputs[2]});
     std::cout << "ASSERT original WhoWill selected non-first party index " << selected << "; completion leaves no suspended selection\n";
    } else if (cp::sameCamera(c, cp::myra)) {
     unsigned acks=0;bool expectedExchange=false;
     while(flow->blocksGameplay()) {
      check(++acks<100 && pending,"bounded Myra phases");
      const auto kind=pending->request.kind;
      check(kind==XeenPresentationKind::NpcAcknowledgment || kind==XeenPresentationKind::RewardReceipt ||
       kind==XeenPresentationKind::RewardWarning,"unexpected Myra phase");
      drive({{SDLK_RETURN,AcknowledgeAction{}}});
      if(returning && !expectedExchange && pending->request.kind==XeenPresentationKind::RewardReceipt) {
       check(pending->state.instructionCount==9 && pending->state.rewardReceipt.delivered==5 &&
        pending->state.rewardReceipt.lost==0 && pending->state.rewardReceipt.overflow==0,"restored Root exchange receipt");
       root=false;request=false;expectedExchange=true;
       // This restored pre-exchange fixture has empty misc packs and an eligible first owner.
       const auto owner=defaults.party.activeRosterIds().front();
       check(defaults.roster.at(owner).canAct(),"restored fixture eligibility");
       for(const auto &item:defaults.roster.at(owner).miscellaneous)check(item.id==0,"restored fixture capacity");
       for(unsigned i=0;i<5;++i)expectedCharacters[owner].miscellaneous[i]={10,37,1,0};
      }
      if(!returning && !flow->blocksGameplay())request=true;
      stateCheck();
     }
    } else drive(std::vector<cp::Input>(inputs.begin()+1, inputs.end()));
   }
   if (cp::sameCamera(c, cp::phirna)) { completed(acquire ? 18 : 11); if (acquire) { root = true; phirnaRemoved = true; } else check(presentations == 0, "removed plant replayed dialogue"); }
   else if (cp::sameCamera(c, cp::whistle)) { completed(acquire ? 10 : 5); if (acquire) bone = true; else check(presentations == 0, "removed bones replayed dialogue"); }
   else if (returning) {
    completed(9);check(!root && !request,"restored Root was not consumed/Q2 cleared");
    std::cout << "ASSERT restored pre-exchange Root consumed in-process; five deterministic items; no post-exchange save\n";
   } else { completed(5); request = true; }
   stateCheck();
  };
  if (produce) {
   if (name == "cumulative") { interact(cp::myra, true); interact(cp::phirna, true); interact(cp::whistle, true); }
   else interact(position(name), true);
   check(!flow->blocksGameplay(), "save boundary not eligible");
   drive({{SDLK_F9, SaveGameAction{}}});
   check(status().find("MMModern - Saved [") == 0 && fs::exists(dir/(name + ".mmsave")), "production eligible save did not succeed");
   stateCheck(); std::cout << "ASSERT production save success " << path.u8string() << '\n';
  } else if (resume) {
   auto revisit = [&] {
    if (name == "cumulative") { interact(cp::phirna, false); interact(cp::whistle, false); interact(cp::myra, false); }
    else interact(position(name), false);
    move(position(name)); drive({{SDLK_RETURN, AcknowledgeAction{}}}); // Clear deliberately transient passive text.
    equalFrame(flow->frame(), cleanBase()); stateCheck();
   };
   if (!exchange) revisit();
   // Preload independent Castle text in this same event owner before eviction.
   auto textControl = [&] {
    move({1,8,8,XeenDirection::West}); drive({{SDLK_SPACE,InteractionAction{}}});
    check(flow->blocksGameplay() && pending && pending->state.activeCharacterIndex == 0, "independent original Castle text/default character context absent");
    drive({{SDLK_n,NoAction{}}}); move(position(name)); drive({{SDLK_RETURN,AcknowledgeAction{}}}); stateCheck();
   };
   textControl();
   for (int cache = exchange ? 4 : 0; cache < 5; ++cache) {
    const auto before = std::array<std::uint64_t,5>{static_cast<unsigned>(mapLoads), static_cast<unsigned>(objectLoads), static_cast<unsigned>(scriptLoads), static_cast<unsigned>(textLoads), assets.spriteLoadCount()};
    if (cache == 0 || cache == 4) world->discardMapCache();
    if (cache == 1 || cache == 4) events->discardScriptCache();
    if (cache == 2 || cache == 4) events->discardTextCache();
    if (cache == 3 || cache == 4) assets.discardSpriteCache();
    flow->refresh(true); equalFrame(flow->frame(), cleanBase());
    if (!exchange) revisit();
    if (cache == 2 || cache == 4) textControl();
    const auto after = std::array<std::uint64_t,5>{static_cast<unsigned>(mapLoads), static_cast<unsigned>(objectLoads), static_cast<unsigned>(scriptLoads), static_cast<unsigned>(textLoads), assets.spriteLoadCount()};
    for (int i = 0; i < 5; ++i) {
     const bool required = cache == 4 || (cache == 0 && i < 2) || (cache == 1 && i == 2) || (cache == 2 && i == 3) || (cache == 3 && i == 4);
     if (required) check(after[i] > before[i], "discarded resource was not really reloaded");
    }
    std::cout << "ASSERT cache " << cache << " map/object/script/text/sprite";
    for (int i = 0; i < 5; ++i) std::cout << ' ' << before[i] << "->" << after[i]; std::cout << '\n';
    equalFrame(flow->frame(), cleanBase()); stateCheck();
   }
   visual_remove_test::save(flow->frame(), dir/(name + "-rebuilt.bmp"));
   if (exchange) {
    check(!root && !request && phirnaRemoved, "pre-revisit reconstructed exchange differs");
    save_test::sameSnapshot(expectedSnapshot(), XeenSaveFile::read(path));
    cleanPresentation(*flow);
    std::cout << "ASSERT combined reconstruction before first Myra revisit; Root=0 Q2=0 five rewards and Phirna removed\n";
    revisit();
    check(!root && request && phirnaRemoved, "resumed no-Root request did not set Q2");
    cleanPresentation(*flow);
    std::cout << "ASSERT first resumed Myra request: five instructions, Q2=1, no new item or removal\n";
   }
  } else {
   // A distinct no-load process checks original defaults, records and presence.
   // Rendering alone cannot demonstrate a fresh temporary WhoWill context.
   if (name == "whistle") {
    drive({{SDLK_SPACE,InteractionAction{}}});
    check(pending && pending->state.activeCharacterIndex == 0 && flow->canCancelInteraction(), "fresh WhoWill context");
    drive({{SDLK_ESCAPE,CancelInteractionAction{}}}); stateCheck();
   }
  }
  std::cout << "ASSERT item99/index17=" << party->questItems.at(17) << " item100/index18=" << party->questItems.at(18)
   << " Q2=" << party->questFlags.isSet(2) << " disabled objects=" << world->sessionState().disabledObjectCount()
   << " events=" << world->sessionState().disabledEventCount() << '\n';
  std::cout << "ASSERT live camera, ordered membership, all 30 characters/all fields, all 35 counters, all 30 quest flags, 256 game flags, exact object/event sets, base records/geometry, selection/draw, clean first scene PASS\n";
  return true;
 };
 const auto result = Application().playGameplay(services, resume ? XeenCamera{} : expectedCamera, produce || resume ? std::optional<fs::path>(path) : std::nullopt, resume);
 check(result == 0, "Application acceptance failed");
 if (resume) check(diskBytes(path) == initialDisk, "consumer changed original save bytes");
 std::cout << "ACCEPT " << name << ' ' << role << ' ' << (manual ? "physical-keyboard" : sdl ? "sdl" : "direct") << " PID " << GetCurrentProcessId() << '\n';
 return 0;
}
}
int main(int argc, char **argv) {
 try {
  if (argc == 4 && std::string(argv[1]) == "--manual-myra-exchange") {
   const auto game = fs::absolute(fs::u8path(argv[2])), save = fs::absolute(fs::u8path(argv[3]));
   check(fs::is_directory(save.parent_path()) && !fs::exists(save), "manual target requires existing directory and absent file; nothing is deleted");
   std::cout << "Physical keyboard mode; camera-only checkpoint positioning, one continuous SDL loop.\n"
    << "Save: " << save.u8string() << "\nAfter this producer exits completely, run:\n& \""
    << (fs::absolute(fs::u8path(argv[0])).parent_path()/"mmodern.exe").u8string()
    << "\" --load-game \"" << game.u8string() << "\" \"" << save.u8string() << "\"\n" << std::flush;
   return child(game, save.parent_path(), "myra-exchange", "producer", false, save);
  }
  if (argc == 7 && std::string(argv[1]) == "--child") return child(fs::u8path(argv[2]), fs::u8path(argv[3]), argv[4], argv[5], std::string(argv[6]) == "sdl");
  check(argc == 3 || (argc == 4 && std::string(argv[3]) == "sdl"), "Usage: mmodern_save_resume_smoke <game> <output> [sdl]");
  const auto game = fs::absolute(fs::u8path(argv[1])), output = fs::absolute(fs::u8path(argv[2]));
  fs::create_directories(output);
  const auto dir = output/("run-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
  check(fs::create_directory(dir), "acceptance run directory must be new");
  const auto exe = fs::absolute(fs::u8path(argv[0])); const std::wstring mode = argc == 4 ? L"sdl" : L"direct";
  std::ofstream evidence(dir/"processes.log"); check(bool(evidence), "process evidence log");
  for (const std::string name : {"phirna", "whistle", "myra", "cumulative", "myra-exchange"}) {
   DWORD producer = 0;
   std::vector<std::uint8_t> savedBytes;
   for (const std::string role : {"producer", "consumer", "fresh"}) {
    const auto log = dir/(name + "-" + role + ".log");
    const std::vector<std::wstring> args{L"--child",game.wstring(),dir.wstring(),fs::path(name).wstring(),fs::path(role).wstring(),mode};
    evidence << "COMMAND " << exe.u8string(); for (const auto &a : args) evidence << " \"" << fs::path(a).u8string() << '"'; evidence << '\n' << std::flush;
    const auto r = child_test::launch(exe, args, log);
    evidence << "PID " << r.pid << " exit " << r.exit << " save " << (dir/(name+".mmsave")).u8string() << '\n' << r.output << std::flush;
    if (r.exit != 0) std::cerr << r.output;
    check(r.exit == 0 && r.output.find("ACCEPT " + name + " " + role) != std::string::npos, "child assertions failed; stopped before next process");
    if (role == "producer") { producer = r.pid; savedBytes = diskBytes(dir/(name+".mmsave")); }
    else {
     check(r.pid != producer, "restart must use distinct process");
     check(diskBytes(dir/(name+".mmsave")) == savedBytes, "consumer/fresh altered producer file");
    }
   }
   auto bytes = [](const fs::path &file) {
    std::ifstream input(file, std::ios::binary); check(bool(input), "native comparison frame missing");
    return std::vector<char>(std::istreambuf_iterator<char>(input), {});
   };
   const bool same = bytes(dir/(name+"-consumer-first.bmp")) == bytes(dir/(name+"-fresh-first.bmp"));
   check(same == (name == "myra" || name == "myra-exchange"), "fresh/resumed native frame relationship");
   evidence << "ASSERT first frames: " << name << (same ? " unchanged clean Myra scene" : " effective removal differs from fresh original") << '\n';
   const auto cli = exe.parent_path()/"mmodern.exe";
   evidence << "COMMAND \"" << cli.u8string() << "\" --load-game \"" << game.u8string() << "\" \"" << (dir/(name+".mmsave")).u8string() << "\"\n" << std::flush;
   const auto r = child_test::launch(cli, {L"--load-game",game.wstring(),(dir/(name+".mmsave")).wstring()}, dir/(name+"-cli.log"), true);
   evidence << "CLI PID " << r.pid << " exit " << r.exit << '\n' << r.output << std::flush;
   const auto c = position(name);
   const std::string camera = "Map " + std::to_string(c.mapId.number) + " (Clouds): camera X=" + std::to_string(c.x) + " Y=" + std::to_string(c.y) + " direction=" + std::to_string(static_cast<unsigned>(c.direction));
   check(r.exit == 0 && r.output.find("Resumed ") != std::string::npos && r.output.find(camera) != std::string::npos, "actual CLI resume/camera/normal exit failed");
   if (name == "myra-exchange") {
    check(r.output.find("Setup Inventory: 6 active references, 30 owners; Root=0 Q2=0") != std::string::npos &&
     r.output.find("Active order: [0->0] [1->18] [2->14] [3->11] [4->1] [5->6]") != std::string::npos,
     "CLI restored inventory/Root/Q2 diagnostics absent");
    const auto firstOwner = r.output.find("Owner 0 "), nextOwner = r.output.find("Owner 1 ");
    check(firstOwner != std::string::npos && nextOwner > firstOwner, "CLI roster diagnostics missing");
    const auto owner = r.output.substr(firstOwner, nextOwner-firstOwner);
    for (unsigned i=0;i<5;++i) check(owner.find(" " + std::to_string(i) + ": M=10 ID=37 S=1 F=0") != std::string::npos,
     "CLI exact reward slot diagnostic absent");
    evidence << "ASSERT actual CLI setup: five roster-0 rewards, Root=0 Q2=0; Windows SDL normal close\n";
   }
   check(diskBytes(dir/(name+".mmsave")) == savedBytes, "CLI altered producer file");
   evidence << "ASSERT producer file unchanged after consumer/fresh/CLI\n";
  }
  check(bool(evidence), "process evidence write failed");
  std::cout << "Cross-process original acceptance PASS; evidence " << dir.u8string() << '\n'; return 0;
 } catch (const std::exception &e) { std::cerr << "Save/resume acceptance failed: " << e.what() << '\n'; return 1; }
}
