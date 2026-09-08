#include "XeenCheckpointTestSupport.h"
#include "XeenChildProcessTestSupport.h"
#include "XeenVisualRemoveTestSupport.h"
#include "XeenPartySnapshotTestSupport.h"
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
 check(name == "myra", "unknown checkpoint"); return cp::myra;
}
void equalFrame(const IndexedFrame &a, const IndexedFrame &b) {
 check(a.isValid() && a.width == 320 && a.height == 200 && a.pixels == b.pixels && a.palette == b.palette,
  "native scene/palette mismatch");
}
int child(const fs::path &game, const fs::path &dir, const std::string &name, const std::string &role, bool sdl) {
 const bool resume = role == "consumer", produce = role == "producer";
 check(resume || produce || role == "fresh", "unknown child role");
 const auto installation = XeenInstallationDetector().detect(game);
 check(installation && installation->hasXeen(), "original installation unavailable");
 XeenAssetSource assets(*installation, 320, 200);
 const auto defaults = XeenPartyLoader().loadInitialCloudsParty(assets);
 const auto defaultFlags = XeenGameFlagsLoader().loadInitialCloudsFlags(assets);
 check(defaults.questItems.at(17) == 0 && defaults.questItems.at(18) == 0 && !defaults.questFlags.isSet(2), "unexpected original checkpoint prerequisites");
 const auto path = XeenSaveFile::resolve(dir/(name + ".mmsave"), installation->root);
 if (produce) check(!fs::exists(dir/(name + ".mmsave")), "stale producer save refused");
 bool root = resume && (name == "phirna" || name == "cumulative");
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
 std::optional<XeenManualEventResult> terminal; int presentations = 0;
 std::optional<XeenEventExecutionSuspended> pending;
 auto partyCheck = [&](const XeenPartyState &p) {
  check(p.party.activeRosterIds() == defaults.party.activeRosterIds(), "active membership/order changed");
  for (std::size_t i = 0; i < 30; ++i) remove_test::checkSameCharacter(p.roster.characters()[i], defaults.roster.characters()[i]);
  auto counts = defaults.questItems.counts(); counts[17] += root; counts[18] += bone;
  auto quests = defaults.questFlags.values(); if (request) quests[2] = true;
  remove_test::checkPartyQuestState(p, counts, quests);
 };
 auto worldCheck = [&](XeenWorld &w) {
  std::set<XeenObjectIdentity> objects; std::set<XeenEventIdentity> records;
  if (root) { objects.insert({23,13}); for (int i = 125; i <= 135; ++i) records.insert({23,static_cast<std::size_t>(i)}); }
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
 XeenGameplayServices services{
  {signature, [&] { return XeenPartyLoader().loadInitialCloudsParty(assets); }, [&](XeenMapIdentity id) { ++scriptLoads; return scripts.load(id); }},
  [&] { return XeenGameFlagsLoader().loadInitialCloudsFlags(assets); },
  [&](XeenMapIdentity id) { ++mapLoads; return maps.loadGeometryMap(assets, id); },
  [&](XeenMapIdentity id) { ++objectLoads; return maps.loadObjects(assets, id); },
  [&](XeenMapIdentity id) { ++textLoads; return texts.load(id); }, font,
  [&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c) {
   // Includes resume preflight and constructor composition, before show().
   if (!world) { partyCheck(p); worldCheck(w); check(cp::sameCamera(c, expectedCamera), "first composition used default camera"); }
   ++compositions; return composer.compose(assets, w, p, c, {kCloudsInitialYear});
  },
  [&](IndexedFrame &f, std::uint8_t portrait, std::size_t index) { assets.drawNpc(f, portrait, index); },
  [&](XeenEventFlow &f, const XeenCamera &) {
   flow = &f;
   f.reportText = [](const std::string &s) { throw std::runtime_error(s); };
   f.reportAutomatic = [&](const XeenAutomaticEventResult &r) { ++automatic; check(std::holds_alternative<XeenAutomaticEventNoTrigger>(r), "unexpected automatic checkpoint replay"); };
   f.reportManual = [&](const XeenManualEventResult &r) {
    if (const auto *s = std::get_if<XeenEventExecutionSuspended>(&r)) { ++presentations; pending = *s; }
    else terminal = r;
   };
  }, {},
  [&](XeenWorld &w, XeenEventSystem &e, const XeenPartyState &p, XeenCamera &c, const XeenGameFlags &f) {
   world = &w; events = &e; party = &p; camera = &c; flags = &f;
  }
 };
 services.show = [&](const IndexedFrame &first, const auto &handle, const auto &escape, const auto &idle, const auto &status) {
  check(compositions >= (resume ? 2 : 1) && automatic == (resume ? 0 : 1), "startup composition/dispatch count");
  stateCheck(); check(!flow->blocksGameplay() && !flow->presentationGeneration() && !flow->canCancelInteraction() && !flow->presenter().blocksGameplay() && flow->presenter().pageCount() == 0 && flow->presenter().npcTiming().remaining == 0, "retained dialog/WhoWill/NPC startup state");
  equalFrame(first, composer.compose(assets, *world, *party, *camera, {kCloudsInitialYear}));
  visual_remove_test::save(first, dir/(name + "-" + role + "-first.bmp"));
  // SDL uses the actual Application handler, escape and idle callbacks. No
  // alternate startup/save choice. Each bounded input batch owns its window.
  auto drive = [&](const std::vector<cp::Input> &inputs) {
   if (!sdl) { for (const auto &input : inputs) handle(input.action); return; }
   std::size_t index = 0; bool queued = false, quit = false; const auto began = SDL_GetTicks64();
   const bool ok = SdlWindow().showInteractive(flow->frame(), status(), [&](const PlayerAction &a) {
    check(index < inputs.size() && a.index() == inputs[index].action.index(), "SDL action order/repeat");
    if (const auto *m = std::get_if<SelectMemberAction>(&a)) check(m->partyIndex == std::get<SelectMemberAction>(inputs[index].action).partyIndex, "SDL selection index");
    auto frame = handle(a); ++index; queued = false; return frame;
   }, escape, [&]()->std::optional<IndexedFrame> {
    check(SDL_GetTicks64() - began < 5000, "SDL checkpoint input timeout");
    if (!queued && index < inputs.size()) {
     SDL_Event e{}; e.type = SDL_KEYDOWN; e.key.keysym.sym = inputs[index].key; check(SDL_PushEvent(&e) == 1, "SDL push");
     e.key.repeat = 1; check(SDL_PushEvent(&e) == 1, "SDL repeat push"); queued = true;
    } else if (index == inputs.size() && !quit) { SDL_Event e{}; e.type = SDL_QUIT; check(SDL_PushEvent(&e) == 1, "SDL quit push"); quit = true; }
    return idle();
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
  auto interact = [&](XeenCamera c, bool acquire) {
   move(c); terminal.reset(); pending.reset(); presentations = 0;
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
    const auto pixels = flow->frame().pixels; const auto generation = flow->presentationGeneration();
    const auto page = flow->presenter().pageIndex();
    drive({{SDLK_F9, SaveGameAction{}}});
    check(status().find("Cannot save while an interaction is pending") != std::string::npos && flow->frame().pixels == pixels && flow->presentationGeneration() == generation && flow->presenter().pageIndex() == page, "pending F9 advanced original interaction");
    if (produce) check(!fs::exists(dir/(name + ".mmsave")), "pending F9 wrote save");
    if (cp::sameCamera(c, cp::myra) && root) inputs.pop_back();
    if (cp::sameCamera(c, cp::whistle) && acquire) {
     drive({inputs[1]});
     check(pending && pending->state.activeCharacterIndex == selected && pending->request.source.line == 2 && flow->blocksGameplay() && !flow->canCancelInteraction(), "WhoWill selected context/display/acknowledgment");
     drive({inputs[2]});
     std::cout << "ASSERT original WhoWill selected non-first party index " << selected << "; completion leaves no suspended selection\n";
    } else drive(std::vector<cp::Input>(inputs.begin()+1, inputs.end()));
   }
   if (cp::sameCamera(c, cp::phirna)) { completed(acquire ? 18 : 11); if (acquire) root = true; else check(presentations == 0, "removed plant replayed dialogue"); }
   else if (cp::sameCamera(c, cp::whistle)) { completed(acquire ? 10 : 5); if (acquire) bone = true; else check(presentations == 0, "removed bones replayed dialogue"); }
   else if (root) {
    const auto *error = terminal ? std::get_if<XeenEventExecutionError>(&*terminal) : nullptr;
    check(error && error->kind == XeenEventExecutionErrorKind::UnsupportedOperationMode && error->instructionCount == 3 && error->source && error->source->line == 8 && error->source->fileOffset == 255 && !flow->blocksGameplay(), "genuinely acquired Root consumption boundary");
    std::cout << "ASSERT acquired Root: UnsupportedOperationMode line 8 offset 255 instructions 3; no mutation\n";
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
    equalFrame(flow->frame(), first); stateCheck();
   };
   revisit();
   // Preload independent Castle text in this same event owner before eviction.
   auto textControl = [&] {
    move({1,8,8,XeenDirection::West}); drive({{SDLK_SPACE,InteractionAction{}}});
    check(flow->blocksGameplay() && pending && pending->state.activeCharacterIndex == 0, "independent original Castle text/default character context absent");
    drive({{SDLK_n,NoAction{}}}); move(position(name)); drive({{SDLK_RETURN,AcknowledgeAction{}}}); stateCheck();
   };
   textControl();
   for (int cache = 0; cache < 5; ++cache) {
    const auto before = std::array<std::uint64_t,5>{static_cast<unsigned>(mapLoads), static_cast<unsigned>(objectLoads), static_cast<unsigned>(scriptLoads), static_cast<unsigned>(textLoads), assets.spriteLoadCount()};
    if (cache == 0 || cache == 4) world->discardMapCache();
    if (cache == 1 || cache == 4) events->discardScriptCache();
    if (cache == 2 || cache == 4) events->discardTextCache();
    if (cache == 3 || cache == 4) assets.discardSpriteCache();
    flow->refresh(true); equalFrame(flow->frame(), first); revisit();
    if (cache == 2 || cache == 4) textControl();
    const auto after = std::array<std::uint64_t,5>{static_cast<unsigned>(mapLoads), static_cast<unsigned>(objectLoads), static_cast<unsigned>(scriptLoads), static_cast<unsigned>(textLoads), assets.spriteLoadCount()};
    for (int i = 0; i < 5; ++i) {
     const bool required = cache == 4 || (cache == 0 && i < 2) || (cache == 1 && i == 2) || (cache == 2 && i == 3) || (cache == 3 && i == 4);
     if (required) check(after[i] > before[i], "discarded resource was not really reloaded");
    }
    std::cout << "ASSERT cache " << cache << " map/object/script/text/sprite";
    for (int i = 0; i < 5; ++i) std::cout << ' ' << before[i] << "->" << after[i]; std::cout << '\n';
    equalFrame(flow->frame(), first); stateCheck();
   }
   visual_remove_test::save(flow->frame(), dir/(name + "-rebuilt.bmp"));
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
 std::cout << "ACCEPT " << name << ' ' << role << ' ' << (sdl ? "sdl" : "direct") << " PID " << GetCurrentProcessId() << '\n';
 return 0;
}
}
int main(int argc, char **argv) {
 try {
  if (argc == 7 && std::string(argv[1]) == "--child") return child(fs::u8path(argv[2]), fs::u8path(argv[3]), argv[4], argv[5], std::string(argv[6]) == "sdl");
  check(argc == 3 || (argc == 4 && std::string(argv[3]) == "sdl"), "Usage: mmodern_save_resume_smoke <game> <output> [sdl]");
  const auto game = fs::absolute(fs::u8path(argv[1])), output = fs::absolute(fs::u8path(argv[2]));
  fs::create_directories(output);
  const auto dir = output/("run-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()));
  check(fs::create_directory(dir), "acceptance run directory must be new");
  const auto exe = fs::absolute(fs::u8path(argv[0])); const std::wstring mode = argc == 4 ? L"sdl" : L"direct";
  std::ofstream evidence(dir/"processes.log"); check(bool(evidence), "process evidence log");
  for (const std::string name : {"phirna", "whistle", "myra", "cumulative"}) {
   DWORD producer = 0;
   for (const std::string role : {"producer", "consumer", "fresh"}) {
    const auto log = dir/(name + "-" + role + ".log");
    const std::vector<std::wstring> args{L"--child",game.wstring(),dir.wstring(),fs::path(name).wstring(),fs::path(role).wstring(),mode};
    evidence << "COMMAND " << exe.u8string(); for (const auto &a : args) evidence << " \"" << fs::path(a).u8string() << '"'; evidence << '\n' << std::flush;
    const auto r = child_test::launch(exe, args, log);
    evidence << "PID " << r.pid << " exit " << r.exit << " save " << (dir/(name+".mmsave")).u8string() << '\n' << r.output << std::flush;
    if (r.exit != 0) std::cerr << r.output;
    check(r.exit == 0 && r.output.find("ACCEPT " + name + " " + role) != std::string::npos, "child assertions failed; stopped before next process");
    if (role == "producer") producer = r.pid;
    else check(r.pid != producer, "restart must use distinct process");
   }
   auto bytes = [](const fs::path &file) {
    std::ifstream input(file, std::ios::binary); check(bool(input), "native comparison frame missing");
    return std::vector<char>(std::istreambuf_iterator<char>(input), {});
   };
   const bool same = bytes(dir/(name+"-consumer-first.bmp")) == bytes(dir/(name+"-fresh-first.bmp"));
   check(same == (name == "myra"), "fresh/resumed native frame relationship");
   evidence << "ASSERT first frames: " << name << (same ? " unchanged clean Myra scene" : " effective removal differs from fresh original") << '\n';
   const auto cli = exe.parent_path()/"mmodern.exe";
   evidence << "COMMAND \"" << cli.u8string() << "\" --load-game \"" << game.u8string() << "\" \"" << (dir/(name+".mmsave")).u8string() << "\"\n" << std::flush;
   const auto r = child_test::launch(cli, {L"--load-game",game.wstring(),(dir/(name+".mmsave")).wstring()}, dir/(name+"-cli.log"), true);
   evidence << "CLI PID " << r.pid << " exit " << r.exit << '\n' << r.output << std::flush;
   const auto c = position(name);
   const std::string camera = "Map " + std::to_string(c.mapId.number) + " (Clouds): camera X=" + std::to_string(c.x) + " Y=" + std::to_string(c.y) + " direction=" + std::to_string(static_cast<unsigned>(c.direction));
   check(r.exit == 0 && r.output.find("Resumed ") != std::string::npos && r.output.find(camera) != std::string::npos, "actual CLI resume/camera/normal exit failed");
  }
  check(bool(evidence), "process evidence write failed");
  std::cout << "Cross-process original acceptance PASS; evidence " << dir.u8string() << '\n'; return 0;
 } catch (const std::exception &e) { std::cerr << "Save/resume acceptance failed: " << e.what() << '\n'; return 1; }
}
