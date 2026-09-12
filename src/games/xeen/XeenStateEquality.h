#ifndef MMODERN_XEEN_STATE_EQUALITY_H
#define MMODERN_XEEN_STATE_EQUALITY_H
#include "games/xeen/XeenWorld.h"
namespace mmodern::xeen_state {
inline bool sameAttribute(XeenAttributeValue a, XeenAttributeValue b) {
	return a.permanent == b.permanent && a.temporary == b.temporary;
}
inline bool sameInputs(const XeenCombatInputs &a, const XeenCombatInputs &b) {
	return sameAttribute(a.might, b.might) && sameAttribute(a.speed, b.speed) &&
		sameAttribute(a.accuracy, b.accuracy) && a.temporaryAc == b.temporaryAc &&
		a.experience == b.experience;
}
inline bool sameItemCategory(const XeenItemCategory &a, const XeenItemCategory &b) {
	for (std::size_t i = 0; i < a.size(); ++i)
		if (a[i].material != b[i].material || a[i].id != b[i].id ||
			a[i].state != b[i].state || a[i].frame != b[i].frame) return false;
	return true;
}
inline bool sameCharacter(const XeenCharacter &a, const XeenCharacter &b) {
	return a.rosterId == b.rosterId && a.name == b.name && a.sex == b.sex &&
		a.race == b.race && a.characterClass == b.characterClass &&
		sameAttribute(a.intellect, b.intellect) && sameAttribute(a.personality, b.personality) &&
		sameAttribute(a.endurance, b.endurance) && a.permanentLevel == b.permanentLevel &&
		a.temporaryLevel == b.temporaryLevel && a.temporaryAge == b.temporaryAge &&
		a.maxStatSkills.astrologer == b.maxStatSkills.astrologer &&
		a.maxStatSkills.bodybuilder == b.maxStatSkills.bodybuilder &&
		a.maxStatSkills.prayerMaster == b.maxStatSkills.prayerMaster &&
		a.maxStatSkills.prestidigitation == b.maxStatSkills.prestidigitation &&
		a.hasSpells == b.hasSpells && sameItemCategory(a.weapons, b.weapons) &&
		sameItemCategory(a.armor, b.armor) && sameItemCategory(a.accessories, b.accessories) &&
		sameItemCategory(a.miscellaneous, b.miscellaneous) && a.currentHp == b.currentHp &&
		a.currentSp == b.currentSp && a.conditions == b.conditions && a.birthYear == b.birthYear;
}
inline bool sameCamera(const XeenCamera &a, const XeenCamera &b) {
	return a.mapId == b.mapId && a.x == b.x && a.y == b.y && a.direction == b.direction;
}
inline bool sameActor(const XeenActor &a, const XeenActor &b) {
	return a.id == b.id && a.original.x == b.original.x && a.original.y == b.original.y &&
		a.original.direction == b.original.direction && a.original.tableIndex == b.original.tableIndex &&
		a.original.resourceId == b.original.resourceId && a.x == b.x && a.y == b.y && a.hp == b.hp &&
		a.activated == b.activated && a.lifecycle == b.lifecycle && a.status == b.status &&
		bool(a.statistics) == bool(b.statistics) && (!a.statistics || a.statistics->raw == b.statistics->raw);
}

inline bool sameEntities(const XeenMapEntities &a, const XeenMapEntities &b) {
	if (a.objectTable != b.objectTable || a.monsterTable != b.monsterTable || a.wallItemTable != b.wallItemTable) return false;
	const auto records = [](const auto &x, const auto &y) {
		if (x.size() != y.size()) return false;
		for (std::size_t i = 0; i < x.size(); ++i)
			if (x[i].x != y[i].x || x[i].y != y[i].y || x[i].tableIndex != y[i].tableIndex ||
				x[i].direction != y[i].direction || x[i].resourceId != y[i].resourceId) return false;
		return true;
	};
	return records(a.objects, b.objects) && records(a.monsters, b.monsters) && records(a.wallItems, b.wallItems);
}
inline bool sameMap(const XeenMap &a, const XeenMap &b) {
	const auto &x = a.geometry, &y = b.geometry;
	if (a.side != b.side || x.id != y.id || x.flags != y.flags || x.flags2 != y.flags2 ||
		x.neighbors != y.neighbors || x.wallTypes != y.wallTypes || x.surfaceTypes != y.surfaceTypes ||
		x.floorType != y.floorType || x.runX != y.runX || x.runY != y.runY || x.difficulties != y.difficulties ||
		x.trapDamage != y.trapDamage || x.wallKind != y.wallKind || x.tavernTips != y.tavernTips ||
		!sameEntities(a.entities, b.entities) || a.instructions.size() != b.instructions.size()) return false;
	for (std::size_t i = 0; i < x.cells.size(); ++i) {
		const auto &c = x.cells[i], &d = y.cells[i];
		if (c.rawWord != d.rawWord || c.rawAttributes != d.rawAttributes || c.surfaceIndex != d.surfaceIndex ||
			c.flags != d.flags || c.seen != d.seen || c.stepped != d.stepped || c.geometry.index() != d.geometry.index()) return false;
		if (const auto *layers = std::get_if<XeenOutdoorLayers>(&c.geometry)) {
			const auto &other = std::get<XeenOutdoorLayers>(d.geometry);
			if (layers->surface != other.surface || layers->middle != other.middle || layers->top != other.top || layers->overlay != other.overlay) return false;
		} else if (std::get<XeenIndoorWalls>(c.geometry).walls != std::get<XeenIndoorWalls>(d.geometry).walls) return false;
	}
	for (std::size_t i = 0; i < a.instructions.size(); ++i) {
		const auto &c = a.instructions[i], &d = b.instructions[i];
		if (c.x != d.x || c.y != d.y || c.direction != d.direction || c.line != d.line || c.opcode != d.opcode || c.parameters != d.parameters) return false;
	}
	return true;
}
inline bool sameObjectFile(const XeenObjectFile &a, const XeenObjectFile &b) {
	return a.mapId == b.mapId && a.resourceName == b.resourceName && a.resourcePresent == b.resourcePresent && sameEntities(a.entities, b.entities);
}

inline bool sameAuthority(const XeenCompletedEncounterAuthority &a, const XeenCompletedEncounterAuthority &b) {
	if (a.party != b.party || a.roster != b.roster || a.camera != b.camera ||
		a.activeRosterIds != b.activeRosterIds || a.questItems != b.questItems || a.questFlags != b.questFlags ||
		!(a.context == b.context) || a.firstSerializedCount != b.firstSerializedCount ||
		a.effectiveSerializedCount != b.effectiveSerializedCount || a.diagnostics != b.diagnostics ||
		!sameCamera(a.cameraValue, b.cameraValue) || a.actors.size() != b.actors.size() ||
		a.objects != b.objects || a.events != b.events || !(a.monster == b.monster)) return false;
	for (std::size_t i = 0; i < a.characters.size(); ++i) {
		if (!sameCharacter(a.characters[i], b.characters[i]) || bool(a.combatInputs[i]) != bool(b.combatInputs[i])) return false;
		if (a.combatInputs[i] && !sameInputs(*a.combatInputs[i], *b.combatInputs[i])) return false;
	}
	for (std::size_t i = 0; i < a.actors.size(); ++i) if (!sameActor(a.actors[i], b.actors[i])) return false;
	return true;
}
}
#endif
