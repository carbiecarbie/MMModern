#ifndef MMODERN_TESTS_XEEN_DECISION_NINE_H
#define MMODERN_TESTS_XEEN_DECISION_NINE_H
// M69 decision 9, shared by every witness that compares an uninterrupted run
// with runs resumed from its checkpoints. A game load rebuilds each live
// monster at its MON HP and asleep (decision 8), so a resumed run equals the
// uninterrupted one only when, at the checkpoint, every live monster of the
// camera map already is. Each witness checks its checkpoints on live session
// state before saving them: a checkpoint some run resumes from must hold;
// the others are only logged. Intentional wounded or woken saves are explicit
// decision 8 cases with their own healing checks instead.
#include "games/xeen/XeenWorld.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace xeen_decision9 {
// The camera map's live monsters that are woken or below their MON HP.
inline std::string violations(const mmodern::XeenWorld &world, const mmodern::XeenCamera &camera) {
	std::string found;
	for (const auto &a : world.sessionState().monsters(camera.mapId))
		if (a.lifecycle == mmodern::XeenActorLifecycle::Present && a.statistics &&
				(a.activated || a.hp != std::int32_t(a.statistics->baseHp())))
			found += " #" + std::to_string(std::size_t(a.id.recordIndex)) + " hp " + std::to_string(a.hp) + "/" +
				std::to_string(a.statistics->baseHp()) + (a.activated ? " woken" : "");
	return found;
}
// Logs the checkpoint; a resumed checkpoint that does not hold fails the witness.
inline void check(const mmodern::XeenWorld &world, const mmodern::XeenCamera &camera, const std::string &label, bool resumed) {
	const auto found = violations(world, camera);
	std::cerr << "M69 DECISION9 " << label << (resumed ? " (resumed)" : "") << " map " << camera.mapId.number
		<< (found.empty() ? " holds" : " violated:" + found) << '\n';
	if (resumed && !found.empty()) throw std::runtime_error("M69 decision 9 violated at resumed checkpoint " + label);
}
}
#endif
