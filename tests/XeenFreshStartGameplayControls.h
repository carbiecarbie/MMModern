// Extends the existing purchase CLI witness with one original new-game route.
// Only the seed sampling seam is controlled. Every owner change uses gameplay
// input; no camera, actor, character or economy is rewritten by the witness.
#include "games/xeen/XeenEquipmentPurchase.h"
#include "games/xeen/XeenMovement.h"
#include <queue>

int freshStartGameplay(const Application *app, const XeenGameplayServices &original,
        XeenCamera camera, const std::optional<fs::path> &target, bool resume,
        XeenEncounterEntry entry, std::optional<std::uint32_t> seed, const std::string &stage) {
    check(!seed && (resume || original.originalStart), "M52 public entry configuration");
    auto services = original;
    XeenEventFlow *flow = nullptr; XeenWorld *world = nullptr;
    const XeenPartyState *party = nullptr; const XeenCamera *position = nullptr;
    const XeenGameFlags *flags = nullptr;
    std::uint64_t now = 0, cycle = 0;
    unsigned saves = 0, eastEvents = 0, attacks = 0;
    std::string refusal;
    services.sampleJourneySeed = [] { return 1u; };
    services.clock = [&] { return now; };
    services.observeSaveStage = [&](auto) { ++saves; };
    services.observeGameplay = [&](auto &w, auto &, const auto &p, const auto &c, const auto &f) {
        world = &w; party = &p; position = &c; flags = &f;
    };
    services.configureFlow = [&](auto &f, const auto &c) {
        original.configureFlow(f,c); flow = &f;
        f.reportText = [&](const std::string &message) { refusal = message; };
        const auto report = f.reportManual;
        f.reportManual = [&,report](const auto &result) {
            if (position && position->mapId == XeenMapIdentity(28) && position->x == 18 &&
                    position->y == 4 && position->direction == XeenDirection::East &&
                    std::holds_alternative<XeenManualEventCompleted>(result)) ++eastEvents;
            if (report) report(result);
        };
    };
    services.show = [&](const auto &, const auto &handler, const auto &escape, const auto &idle, const auto &status) {
        check(flow && world && party && position && flags, "M52 production owners");
        const auto snapshot = [&] {
            return XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);
        };
        const auto present = [&] {
            check(handler.frameCurrent(), "M52 current frame");
            handler.framePresented(flow->frame().presentation());
            handler.completeInputHandoff(flow->frame().presentation());
        };
        const auto act = [&](PlayerAction action) {
            handler.beginCycle(++cycle);
            check(handler.displayedInput().has_value(), "M52 acquired input");
            handler.withPresentedInput(action,*handler.displayedInput(),flow->frame().presentation());
            present();
        };
        const auto pulse = [&] { now += 100; handler.beginCycle(++cycle); idle(); present(); };
        const auto settle = [&](bool answerYes = false) {
            for (unsigned n = 0; n < 5000; ++n) {
                if (flow->canSave()) return;
                if (const auto combat = flow->encounter()->combat()) {
                    check(combat->phase() != XeenCombatPhase::Failed && combat->phase() != XeenCombatPhase::Defeat &&
                        combat->phase() != XeenCombatPhase::SupportStopped, "M52 normal party cannot continue combat");
                    if (combat->phase() == XeenCombatPhase::PlayerReady) { ++attacks; act(AttackAction{}); }
                    else pulse();
                } else {
                    const auto context = handler.inputContext(flow->frame().presentation());
                    if (context.dialog && context.dialog->anyKey) act(AcknowledgeAction{});
                    else if (context.dialog && context.dialog->key('y') && context.dialog->key('n'))
                        act(answerYes ? PlayerAction{YesAction{}} : PlayerAction{NoAction{}});
                    else if (world->sessionState().journeyActivity() == XeenJourneyActivity::Event ||
                            world->sessionState().journeyActivity() == XeenJourneyActivity::Reward) {
                        act(AcknowledgeAction{});
                        if (world->sessionState().journeyActivity() == XeenJourneyActivity::Event)
                            act(answerYes ? PlayerAction{YesAction{}} : PlayerAction{NoAction{}});
                    } else pulse();
                }
            }
            throw std::runtime_error("M52 settlement bound: " + flow->encounter()->notice());
        };
        const auto checkpoint = [&](const std::string &label) {
            check(flow->canSave(), "M52 quiet checkpoint");
            const auto before = XeenSaveFormat::encode(snapshot()); const auto count = saves;
            act(SaveGameAction{});
            check(target && saves == count + 3 && XeenSaveFormat::encode(XeenSaveFile::read(*target)) == before &&
                XeenSaveFormat::encode(snapshot()) == before, "M52 F9 exact bytes and no owner/RNG/stock replay");
            fs::copy_file(*target,target->parent_path()/(target->stem().string()+"-"+label+".mmsave"));
            std::cout << "M52 CHECKPOINT " << label << '\n';
        };
        const auto face = [&](XeenDirection direction) {
            for (unsigned n = 0; position->direction != direction && n < 4; ++n) {
                act(NavigationAction::TurnRight); settle();
            }
            check(position->direction == direction, "M52 ordinary turn");
        };
        const auto walk = [&](int x, int y) {
            check(position->mapId == XeenMapIdentity(28), "M52 city route");
            // Find a path through original wall/surface geometry, independent of
            // actor movement and Events. Execute every step through production.
            std::array<int,1024> parent; parent.fill(-1);
            std::array<XeenDirection,1024> direction{};
            const int start = position->y*32+position->x, end = y*32+x;
            std::queue<int> cells; cells.push(start); parent[start] = start;
            while (!cells.empty() && parent[end] < 0) {
                const int cell = cells.front(); cells.pop();
                for (unsigned d = 0; d < 4; ++d) {
                    XeenCamera next{28,cell%32,cell/32,XeenDirection(d)};
                    if (XeenMovement().apply(*world,next,NavigationAction::MoveForward) != XeenMovementResult::Moved) continue;
                    const int index = next.y*32+next.x;
                    if (parent[index] >= 0) continue;
                    parent[index] = cell; direction[index] = XeenDirection(d); cells.push(index);
                }
            }
            check(parent[end] >= 0, "M52 original reachable route");
            std::vector<XeenDirection> route;
            for (int cell = end; cell != start; cell = parent[cell]) route.push_back(direction[cell]);
            for (auto it = route.rbegin(); it != route.rend(); ++it) {
                face(*it); act(NavigationAction::MoveForward); settle();
            }
            check(position->x == x && position->y == y && position->mapId == XeenMapIdentity(28), "M52 walked destination");
        };
        present();
        if (resume) {
            check(target && XeenSaveFormat::encode(snapshot()) == XeenSaveFormat::encode(XeenSaveFile::read(*target)),
                "M52 restore exact before first input");
            check(!replay_test::draws && !replay_test::journeyInitializations && !replay_test::actions &&
                !replay_test::pulses && !replay_test::commands && !replay_test::timePreparations,
                "M52 restore replays no gameplay or stock draws");
        } else {
            check(xeen_state::sameCamera(*position,{28,18,4,XeenDirection::West}) &&
                party->encounterContext->difficulty == *original.originalStart && party->encounterContext->day == 1 &&
                party->encounterContext->minutes == 480 && party->food == 90 && party->monsterTreasure->gold == 800,
                "M52 public original first frame/default or explicit difficulty");
        }
        if (!target) {
            const auto random = world->sessionState().journeyRandom();
            act(SaveGameAction{});
            check(!saves && world->sessionState().journeyRandom() == random && status().find("target") != std::string::npos,
                "M52 no implicit F9 target");
        } else if (stage != "m52-after") {
            checkpoint("initial");
            if (stage == "m52-play") {
                face(XeenDirection::East); act(InteractionAction{}); settle();
                check(eastEvents, "M52 East start-cell DoorTextSml executed");
                face(XeenDirection::West); walk(15,4);
                check(attacks && party->roster.combatInputs(0)->experience > 0,
                    "M52 new-game real fight and earned XP");
                const auto food = party->food;
                act(RestAction{}); settle();
                check(party->food == food-6 && party->encounterContext->rested,
                    "M52 completed original Rest from earned combat");
                walk(8,4); face(XeenDirection::West); act(InteractionAction{});
                for (unsigned n = 0; n < 1000 && world->sessionState().journeyActivity() != XeenJourneyActivity::Service; ++n) pulse();
                check(world->sessionState().journeyActivity() == XeenJourneyActivity::Service, "M52 original Smith service");
                act(SelectMemberAction{0});
                act(DialogKeyAction{'b'});
                std::optional<XeenEquipmentPurchaseResult> offer;
                for (unsigned slot = 0; slot < 8; ++slot) {
                    const auto quote = xeenQuoteEquipmentPurchase(*party,0,XeenInventoryCategory::Weapons,slot);
                    if (quote.outcome == XeenEquipmentPurchaseOutcome::Quoted && !quote.shortfall) { offer = quote; break; }
                }
                check(offer.has_value(), "M52 affordable original offer");
                const auto gold = party->monsterTreasure->gold;
                unsigned recipient = 0;
                for (const auto &item : party->roster.at(0).weapons) if (item.id) ++recipient;
                auto delivered = offer->offer; delivered.frame = 0;
                act(SelectInventorySlotAction{offer->offerSlot}); act(YesAction{});
                check(party->monsterTreasure->gold == gold-offer->price &&
                    xeenSameItem(party->roster.at(0).weapons[recipient],delivered), "M52 paid service delivery");
                act(CancelInteractionAction{}); act(CancelInteractionAction{}); settle();
                const auto before = XeenSaveFormat::encode(snapshot());
                act(UnsupportedMainScreenAction{"Map"});
                check(refusal == "Map: not supported yet" && XeenSaveFormat::encode(snapshot()) == before,
                    "M52 visible unsupported refusal preserves gameplay");
                walk(15,0); face(XeenDirection::South); act(InteractionAction{}); settle(true);
                check(xeen_state::sameCamera(*position,{23,10,12,XeenDirection::South}), "M52 original mainland exit");
                face(XeenDirection::North); act(NavigationAction::MoveForward); settle();
                act(InteractionAction{}); settle(true);
                check(position->mapId == XeenMapIdentity(28), "M52 original city re-entry");
                checkpoint("played");
            }
        }
        if (target && (stage == "m52-play" || stage == "m52-after")) {
            act(WaitAction{}); settle(); checkpoint("continued");
        }
        std::cout << "M52 PUBLIC GAMEPLAY PASSED difficulty " << unsigned(party->encounterContext->difficulty) << '\n';
        // Finally exercise the real native upload/presentation/close path on the
        // current acquired frame. The same owners remain live throughout.
        bool quit = false;
        auto native = handler;
        native.beginCycle = [&](std::uint64_t) { handler.beginCycle(++cycle); };
        return original.show(flow->frame(),native,escape,[&]() -> std::optional<IndexedFrame> {
            if (!quit) { SDL_Event e{}; e.type = SDL_QUIT; check(SDL_PushEvent(&e) == 1,"M52 native quit"); quit = true; }
            return {};
        },status);
    };
    return realPlay(app,services,camera,target,resume,entry,seed);
}
