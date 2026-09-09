#include "XeenSaveTestSupport.h"
#include "XeenRemoveTestSupport.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenMapFormat.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenSaveState.h"

#include <algorithm>
#include <iostream>
#include <map>

using namespace mmodern;
using namespace save_test;
using remove_test::record;

namespace {

Bytes eventBytes(const std::vector<XeenEventRecord> &records) {
	Bytes bytes;
	for (const auto &r : records) {
		bytes.insert(bytes.end(), {static_cast<std::uint8_t>(5 + r.parameters.size()),
			r.x, r.y, r.direction, r.line, r.opcode});
		bytes.insert(bytes.end(), r.parameters.begin(), r.parameters.end());
	}
	return bytes;
}

Bytes partyBytes() {
	Bytes bytes(782, 0);
	bytes[0] = 3; bytes[1] = 2; // Deliberately retain original loading diagnostics.
	std::fill(bytes.begin() + 2, bytes.begin() + 10, 255);
	bytes[2] = 0; bytes[3] = 18;
	return bytes;
}

Bytes rosterBytes() {
	Bytes bytes(30 * XeenCharacter::kSerializedSize, 0);
	for (std::size_t i = 0; i < 30; ++i) {
		const auto base = i * XeenCharacter::kSerializedSize;
		bytes[base] = 'A'; bytes[base + 1] = static_cast<std::uint8_t>('A' + i % 26);
		bytes[base + 35] = 1;
		bytes[base + 342] = 8; bytes[base + 344] = 5;
		bytes[base + 346] = 0x50; bytes[base + 347] = 2; // Birth year 592.
	}
	return bytes;
}

Bytes objectBytes() {
	Bytes bytes(48, 255);
	bytes[0] = 5;
	const Bytes entities{
		1, 1, 0, 0, 2, 2, 0, 0, 128, 2, 0, 0, // Two active and one base-disabled object.
		255, 255, 255, 255, // Object terminator.
		255, 255, 255, 255, 255, 255, 255, 255, // Empty monster list.
		255, 255, 255, 255}; // Empty wall-item list.
	bytes.insert(bytes.end(), entities.begin(), entities.end());
	return bytes;
}

struct Fixture {
	std::map<XeenMapIdentity, Bytes> dat, mob, evt;
	Bytes initialRoster = rosterBytes(), initialParty = partyBytes();
	unsigned mapLoads = 0, objectLoads = 0, eventLoads = 0, initialLoads = 0, preflights = 0;
	XeenSaveResourceSignature signature{{12345, 67890}, XeenArchiveFingerprint{98765, 43210}};
	XeenPartyState party;
	XeenCamera camera{1, 1, 1, XeenDirection::North};
	XeenGameFlags flags;
	XeenWorld world{[&](XeenMapIdentity id) { return loadMap(id); },
		[&](XeenMapIdentity id) { return loadObjects(id); }};

	Fixture() {
		for (std::uint16_t id = 1; id <= 3; ++id) {
			Bytes bytes(892, 0); bytes[768] = static_cast<std::uint8_t>(id);
			bytes[781] = 128; bytes[798] = 1; // Outdoor grass.
			dat.emplace(id, std::move(bytes)); mob.emplace(id, objectBytes());
			evt.emplace(id, eventBytes({record(1, 1, 0, 0x12), record(1, 1, 1, 0xff)}));
		}
		party = loadInitial();
	}
	XeenPartyState loadInitial() {
		++initialLoads;
		return XeenPartyLoader().loadFromResources(initialRoster, initialParty);
	}
	XeenMap loadMap(XeenMapIdentity id) {
		++mapLoads;
		requireCloudsMap(id);
		XeenMap map; map.side = id.side;
		map.geometry = XeenMapFormat::parseDat(dat.at(id));
		return map;
	}
	XeenObjectFile loadObjects(XeenMapIdentity id) {
		++objectLoads;
		return XeenMapLoader().loadObjects([&](const std::string &) -> std::optional<Bytes> {
			const auto found = mob.find(id);
			return found == mob.end() ? std::nullopt : std::optional<Bytes>(found->second);
		}, id);
	}
	XeenEventFile loadEvents(XeenMapIdentity id) {
		++eventLoads;
		return XeenEventLoader([&](const std::string &) -> std::optional<Bytes> {
			const auto found = evt.find(id);
			return found == evt.end() ? std::nullopt : std::optional<Bytes>(found->second);
		}).load(id);
	}
	XeenSaveState::Resources resources() {
		return {signature, [&] { return loadInitial(); }, [&](XeenMapIdentity id) { return loadEvents(id); }};
	}
	static IndexedFrame compose(XeenWorld &world, const XeenPartyState &party,
			const XeenCamera &camera, const XeenGameFlags &flags) {
		static_cast<void>(world.map(camera.mapId));
		IndexedFrame frame; frame.width = 320; frame.height = 200; frame.pixels.resize(64000);
		frame.pixels[0] = static_cast<std::uint8_t>(camera.mapId.number);
		frame.pixels[1] = static_cast<std::uint8_t>(camera.x);
		frame.pixels[2] = static_cast<std::uint8_t>(camera.y);
		frame.pixels[3] = static_cast<std::uint8_t>(camera.direction);
		frame.pixels[4] = static_cast<std::uint8_t>(party.questItems.at(17));
		frame.pixels[5] = flags.isSet(7); frame.pixels[6] = party.questFlags.isSet(2);
		const auto &objects = world.objectFile(camera.mapId).entities.objects;
		for (std::size_t i = 0; i < objects.size(); ++i)
			frame.pixels[10 + i] = world.isObjectDisabled({camera.mapId, i}) ? 0 : 255;
		const auto hp = CloudsUiComposer::buildHpPlacements(party, {kCloudsInitialYear});
		const auto faces = CloudsUiComposer::buildPortraitPlacements(party);
		for (std::size_t i = 0; i < hp.size(); ++i) {
			frame.pixels[20 + i] = static_cast<std::uint8_t>(hp[i].frame);
			frame.pixels[30 + i] = hp[i].rosterId;
			frame.pixels[40 + i] = static_cast<std::uint8_t>(faces[i].frame);
		}
		return frame;
	}
	void restore(const XeenSaveSnapshot &snapshot) {
		XeenSaveState::restoreBeforeGameplay(snapshot, resources(), party, camera, flags, world,
			[&](XeenWorld &w, const XeenPartyState &p, const XeenCamera &c, const XeenGameFlags &f) {
				++preflights; static_cast<void>(compose(w, p, c, f));
			});
	}
	XeenSaveSnapshot capture() const { return XeenSaveState::capture(signature, party, camera, flags, world); }
	XeenSaveSnapshot incoming() const {
		auto s = sample(); s.resources = signature; s.camera = {1, 4, 5, XeenDirection::West};
		s.disabledObjects = {{2, 0}, {2, 2}}; s.disabledEvents = {{3, 1}};
		return s;
	}
	void copyResources(const Fixture &source) {
		dat = source.dat; mob = source.mob; evt = source.evt;
		initialRoster = source.initialRoster; initialParty = source.initialParty; signature = source.signature;
	}
};

XeenFontFormat font() {
	Bytes bytes(XeenFontFormat::kMinimumSize);
	for (int i = 0; i < 128; ++i) bytes[0x1000 + i] = 6;
	return XeenFontFormat(bytes);
}

void fullRestoration() {
	Fixture f;
	f.world.disableObject({1, 1});
	f.flags.set(1); f.party.questFlags.set(0); f.party.questItems.increment(0);
	f.party.roster.at(29).currentSp = -50;
	const auto snapshot = f.incoming();
	const auto before = snapshot;
	const auto originalMetadata = f.loadInitial();
	f.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(snapshot)));
	sameSnapshot(f.capture(), before);
	check(f.party.firstSerializedCount == originalMetadata.firstSerializedCount &&
		f.party.effectiveSerializedCount == originalMetadata.effectiveSerializedCount &&
		f.party.diagnostics == originalMetadata.diagnostics, "initial metadata was not reconstructed");
	check(&f.party.party.member(f.party.roster, 0) == &f.party.roster.at(18) &&
		&f.party.party.member(f.party.roster, 2) == &f.party.roster.at(18), "duplicate members lost roster ownership");
	check(!f.world.isObjectDisabled({1, 1}) && f.world.isObjectDisabled({2, 0}) &&
		f.world.isObjectDisabled({2, 2}), "wrong restored object set");
	const auto events = f.loadEvents(3);
	check(f.world.effectiveEvent({3, 0}, events.records[0]).opcode == 0x12 &&
		f.world.effectiveEvent({3, 1}, events.records[1]).opcode == 0, "event identity expanded by cell");
	check(f.world.sessionState().disabledObjectCount() == 2 &&
		f.world.sessionState().disabledEventCount() == 1, "sets inferred from each other");
	const auto maps = f.mapLoads, objects = f.objectLoads, scripts = f.eventLoads;
	f.world.discardMapCache();
	for (int id : {1, 2, 3}) { f.world.map(id); f.world.objectFile(id); f.loadEvents(id); }
	check(f.mapLoads == maps + 3 && f.objectLoads == objects + 3 && f.eventLoads == scripts + 3,
		"restored resource providers did not reload");
	sameSnapshot(f.capture(), before);
	auto copy = f.capture(); copy.characters[18].currentHp = 123; copy.disabledEvents.clear();
	sameSnapshot(f.capture(), before);

	for (const auto &members : std::vector<std::vector<std::uint8_t>>{{}, {0}, {18, 0, 18, 23, 1, 6}}) {
		Fixture next; auto s = next.incoming(); s.activeRosterIds = members;
		next.restore(s); sameSnapshot(next.capture(), s);
	}
	// Independent zero/nonzero categories and valid event-only/object-only states.
	for (int category = 0; category < 6; ++category) {
		Fixture next; auto s = next.capture();
		if (category == 0) s.characters[29].currentHp = -32768;
		if (category == 1) s.questItems[34] = 0xffffffffU;
		if (category == 2) s.questFlags[29] = true;
		if (category == 3) s.gameFlags[255] = true;
		if (category == 4) s.disabledObjects = {{2, 1}};
		if (category == 5) s.disabledEvents = {{3, 1}};
		next.restore(s); sameSnapshot(next.capture(), s);
	}
}

void legacyAndExplicitItems() {
	Fixture f;
	const auto legacy = XeenSaveFormat::decode(nonzeroLegacy());
	f.signature = legacy.resources;
	// Populate the actual initial resource bytes, including inactive roster 29.
	XeenRoster initial;
	distinctiveInitialItems(initial);
	for (unsigned i = 0; i < 30; ++i) {
		const auto &c = initial.at(i);
		const XeenItemCategory *categories[]{&c.weapons, &c.armor, &c.accessories, &c.miscellaneous};
		for (unsigned category = 0; category < 4; ++category)
			for (unsigned slot = 0; slot < 9; ++slot) {
				const auto offset = i * 354 + 166 + category * 36 + slot * 4;
				const auto item = (*categories[category])[slot];
				f.initialRoster[offset] = item.material; f.initialRoster[offset + 1] = item.id;
				f.initialRoster[offset + 2] = item.state; f.initialRoster[offset + 3] = item.frame;
			}
	}
	f.party = f.loadInitial();
	f.world.disableObject({1, 1}); f.flags.set(255); f.party.questFlags.set(7);
	const auto before = f.capture();
	const auto metadata = remove_test::partySnapshot(f.party);
	const auto *map = &f.world.map(1);
	const auto expected = expectedLegacy(legacy, f.party.roster);
	rejects([&] { XeenSaveState::restoreBeforeGameplay(legacy, f.resources(), f.party, f.camera, f.flags, f.world,
		[&](XeenWorld &, const XeenPartyState &candidate, const XeenCamera &, const XeenGameFlags &) {
			for (unsigned i = 0; i < 30; ++i)
				remove_test::checkSameCharacter(candidate.roster.at(i), expected.characters[i]);
			throw std::runtime_error("late legacy preflight failure");
		}); }, "late legacy");
	sameSnapshot(before, f.capture());
	check(metadata == remove_test::partySnapshot(f.party) && &f.world.map(1) == map,
		"failed legacy preflight published owners or cache");
	f.restore(legacy);
	sameSnapshot(expected, f.capture());
	check(&f.party.party.member(f.party.roster, 0) == &f.party.party.member(f.party.roster, 2),
		"legacy duplicate membership copied inventory");
	sameSnapshot(legacy, XeenSaveFormat::decode(nonzeroLegacy())); // Caller input remained unresolved and unchanged.
	const auto upgraded = XeenSaveFormat::decode(XeenSaveFormat::encode(f.capture()));
	check(upgraded.itemState == XeenSaveItemState::Complete, "recapture retained legacy state");
	f.restore(upgraded); sameSnapshot(expected, f.capture());
	// Complete v2 empty records are authoritative against nonzero initial defaults.
	auto empty = upgraded;
	for (auto &c : empty.characters) {
		c.weapons = {}; c.armor = {}; c.accessories = {}; c.miscellaneous = {};
	}
	f.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(empty)));
	sameSnapshot(empty, f.capture());
}

template<class Mutation>
void rejectWithoutPublication(Mutation mutation) {
	Fixture f;
	f.world.disableObject({1, 1});
	f.world.disableEventsAtCell(f.camera, f.loadEvents(1));
	f.party.questItems.increment(5); f.party.questFlags.set(7); f.flags.set(255);
	const auto before = f.capture();
	const auto metadata = remove_test::partySnapshot(f.party);
	const auto *map = &f.world.map(1);
	const auto mapCount = f.world.cachedMapCount(), objectCount = f.world.cachedObjectFileCount();
	auto s = f.incoming();
	mutation(f, s);
	rejects([&] { f.restore(s); });
	sameSnapshot(f.capture(), before);
	check(remove_test::partySnapshot(f.party) == metadata, "failed restore changed metadata/characters");
	check(&f.world.map(1) == map && f.world.cachedMapCount() == mapCount &&
		f.world.cachedObjectFileCount() == objectCount, "failed preparation touched destination caches");
}

void invalidResourceAndState() {
	for (int mode = 0; mode < 23; ++mode) rejectWithoutPublication([&](Fixture &f, XeenSaveSnapshot &s) {
		switch (mode) {
		case 0: s.resources.clouds.size++; break;
		case 1: s.resources.clouds.crc32 ^= 1; break;
		case 2: s.resources.darkside.reset(); break;
		case 3: s.resources.darkside->size++; break;
		case 4: s.resources.darkside->crc32 ^= 1; break;
		case 5: f.initialRoster.pop_back(); break;
		case 6: f.initialParty.resize(781); break;
		case 7: f.dat.erase(1); break;
		case 8: f.dat[2][768] = 3; break;
		case 9: f.dat[3].pop_back(); break;
		case 10: f.mob.erase(2); break;
		case 11: f.mob[2].pop_back(); break;
		case 12: f.evt.erase(3); break;
		case 13: f.evt[3].pop_back(); break;
		case 14: s.disabledObjects = {{2, 3}}; break;
		case 15: s.disabledEvents = {{3, 2}}; break;
		case 16: s.activeRosterIds = {30}; break;
		case 17: s.activeRosterIds = {24}; break; // Existing unsupported portrait policy.
		case 18: s.characters[18].race = static_cast<XeenRace>(5); break;
		case 19: s.characters[18].characterClass = static_cast<XeenCharacterClass>(10); break;
		case 20: s.characters[29].rosterId = 0; break;
		case 21: s.camera.mapId.side = XeenSide::Darkside; break;
		case 22: s.camera.x = -1; break;
		}
	});
	Fixture f; const auto before = f.capture(); auto s = f.incoming();
	auto resources = f.resources();
	const auto loads = f.initialLoads;
	s.resources.clouds.crc32++;
	rejects([&] { f.restore(s); }, "incompatible");
	check(f.initialLoads == loads && f.preflights == 0, "incompatible save invoked providers");
	s = f.incoming();
	resources.loadEvents = [&](XeenMapIdentity id) { auto file = f.loadEvents(id); file.mapId = 2; return file; };
	auto preflight = [&](XeenWorld &, const XeenPartyState &, const XeenCamera &, const XeenGameFlags &) {};
	rejects([&] { XeenSaveState::restoreBeforeGameplay(s, resources, f.party, f.camera, f.flags, f.world, preflight); });
	sameSnapshot(f.capture(), before);
	resources = f.resources(); resources.loadInitialParty = {};
	rejects([&] { XeenSaveState::restoreBeforeGameplay(s, resources, f.party, f.camera, f.flags, f.world, preflight); });
	resources = f.resources();
	rejects([&] { XeenSaveState::restoreBeforeGameplay(s, resources, f.party, f.camera, f.flags, f.world, {}); });
	rejects([&] { XeenSaveState::restoreBeforeGameplay(s, resources, f.party, f.camera, f.flags, f.world,
		[](XeenWorld &, const XeenPartyState &, const XeenCamera &, const XeenGameFlags &) {
			throw std::runtime_error("missing synthetic presentation resource");
		}); }, "presentation resource");
	sameSnapshot(f.capture(), before);
	// Missing optional data is legal if no identity or current composition needs it.
	s.disabledObjects.clear(); s.disabledEvents.clear(); f.mob.erase(2); f.evt.erase(3);
	f.restore(s); sameSnapshot(f.capture(), s);
	// The direct owner API is also atomic across object and event validation.
	const auto old = f.capture();
	rejects([&] { f.world.restoreSessionState({{1, 0}}, {{1, 2}}, [&](XeenMapIdentity id) { return f.loadEvents(id); }); });
	sameSnapshot(f.capture(), old);
	rejects([&] { f.world.restoreSessionState({{1, 0}, {1, 0}}, {}, {}); });
	sameSnapshot(f.capture(), old);
}

void characterPreflight() {
	for (int mode = 0; mode < 9; ++mode) rejectWithoutPublication([&](Fixture &, XeenSaveSnapshot &s) {
		s.activeRosterIds = {0};
		XeenCharacter c; c.birthYear = 592; c.permanentLevel = 1;
		const auto maximum = std::numeric_limits<int>::max(), minimum = std::numeric_limits<int>::min();
		switch (mode) {
		case 0: c.permanentLevel = maximum; c.temporaryLevel = 1; break;
		case 1: c.permanentLevel = minimum; c.temporaryLevel = -1; break;
		case 2: c.temporaryAge = maximum; break;
		case 3: c.intellect = {maximum, 1}; break;
		case 4: c.intellect = {minimum, 0}; c.birthYear = 610; break;
		case 5: c.personality = {minimum + 1, 0}; c.conditions[4] = 255; break;
		case 6: c.permanentLevel = maximum; break; // HP product.
		case 7: c.characterClass = XeenCharacterClass::Cleric; c.hasSpells = true;
			c.personality.permanent = 70000; c.permanentLevel = 100000000; break; // SP product.
		case 8: c.characterClass = XeenCharacterClass::Druid; c.hasSpells = true;
			c.personality.permanent = c.intellect.permanent = 70000;
			c.permanentLevel = 60000000; break; // Sum of two individually safe SP passes.
		}
		s.characters[0] = c;
	});
	Fixture f; auto s = f.capture(); s.activeRosterIds = {0};
	auto &c = s.characters[0];
	c.intellect = {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
	c.permanentLevel = std::numeric_limits<int>::min(); c.temporaryLevel = std::numeric_limits<int>::max();
	c.temporaryAge = std::numeric_limits<int>::min();
	c.currentHp = -32768; c.currentSp = 32767; c.conditions[15] = 255;
	f.restore(s); sameSnapshot(f.capture(), s);
	check(f.party.roster.at(0).currentLevel() == 0, "safe negative level was normalized in storage");
	// Every defined class/race, relevant skill and byte-valued modifier category.
	for (unsigned race = 0; race < 5; ++race) for (unsigned type = 0; type < 10; ++type) {
		XeenCharacter value; value.race = static_cast<XeenRace>(race);
		value.characterClass = static_cast<XeenCharacterClass>(type);
		value.birthYear = 592; value.permanentLevel = 3; value.temporaryLevel = -2;
		value.intellect.permanent = value.personality.permanent = value.endurance.permanent = 70000;
		value.maxStatSkills = {true, true, true, true}; value.hasSpells = true;
		for (unsigned material = 0; material < 256; ++material) {
			value.weapons[0] = {static_cast<std::uint8_t>(material), 0, 0, 1};
			XeenCharacterRules::validateForUse(value, {610});
			check(XeenCharacterRules::maxHp(value, {610}) >= 0 &&
				XeenCharacterRules::maxSp(value, {610}) >= 0, "safe preflight changed defined rules");
		}
	}
}

void restoreEffects(const Fixture &source) {
	Fixture restored; restored.copyResources(source);
	const auto saved = source.capture();
	restored.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(saved)));
	sameSnapshot(restored.capture(), saved);
}

void mutationPolicies() {
	for (int outcome = 0; outcome < 3; ++outcome) {
		Fixture f;
		f.evt[1] = eventBytes({record(1, 1, 0, 0x1f, {2, 1, 1})});
		if (outcome == 0) {
			f.evt[2] = eventBytes({record(1, 1, 0, 0x19, {7, 8, 0}), record(1, 1, 1, 0x12),
				record(7, 8, 0, 9, {21, 99, 6}), record(7, 8, 1, 12, {0, 0, 20, 7}),
				record(7, 8, 2, 12, {0, 0, 104, 2}), record(7, 8, 3, 12, {0, 0, 21, 99}),
				record(7, 8, 4, 0x0e), record(7, 8, 5, 0xff), record(7, 8, 6, 0xff)});
		} else {
			f.evt[2] = eventBytes({record(1, 1, 0, 12, {0, 0, 20, 7}), record(1, 1, 1, 12, {0, 0, 104, 2}),
				record(1, 1, 2, 12, {0, 0, 21, 99}),
				record(1, 1, 3, outcome == 1 ? 9 : 0x20, outcome == 1 ? Bytes{44, 1, 4} : Bytes{0, 0}),
				record(1, 1, 4, 12, {0, 0, 104, 29}), record(1, 1, 5, 0x12)});
		}
		XeenEventSystem events([&](XeenMapIdentity id) { return XeenEventScript(f.loadEvents(id)); },
			[](XeenMapIdentity id) { return XeenEventTextFile{id, "synthetic.txt", true, {"Choose"}}; });
		auto textFont = font();
		XeenEventFlow flow(f.world, events, f.party, f.camera, f.flags, textFont,
			[&](std::uint64_t) { return XeenEventFlow::Composition{Fixture::compose(f.world, f.party, f.camera, f.flags), false}; });
		bool failed = false;
		flow.reportManual = [&](const auto &r) { failed = std::holds_alternative<XeenEventExecutionError>(r); };
		flow.handle(InteractionAction{});
		if (outcome == 1) { check(flow.blocksGameplay(), "missing acknowledgment"); flow.abandonPresentation(); }
		if (outcome == 2) { check(flow.canCancelInteraction(), "missing WhoWill"); flow.handle(CancelInteractionAction{}); }
		check(!flow.blocksGameplay() && f.party.questItems.at(17) == 1 && f.party.questFlags.isSet(2) &&
			!f.party.questFlags.isSet(29), "immediate effects or later instruction policy changed");
		check(f.flags.isSet(7) == (outcome == 2) && f.camera.mapId == XeenMapIdentity(outcome == 2 ? 2 : 1),
			"WhoWill completion versus error/abandon camera/game-flag policy");
		check(failed == (outcome == 0) && f.world.sessionState().disabledObjectCount() == (outcome == 0 ? 1U : 0U) &&
			f.world.sessionState().disabledEventCount() == (outcome == 0 ? 2U : 0U), "Remove before error policy");
		restoreEffects(f);
	}
	Fixture moved;
	moved.dat[1][512 + 33] = 0x10;
	moved.evt[1] = eventBytes({record(1, 2, 0, 12, {0, 0, 20, 7}), record(1, 2, 1, 12, {0, 0, 21, 99}),
		record(1, 2, 2, 12, {0, 0, 104, 2}), record(1, 2, 3, 0xff)});
	XeenEventSystem events([&](XeenMapIdentity id) { return XeenEventScript(moved.loadEvents(id)); });
	auto textFont = font();
	XeenEventFlow flow(moved.world, events, moved.party, moved.camera, moved.flags, textFont,
		[&](std::uint64_t) { return XeenEventFlow::Composition{Fixture::compose(moved.world, moved.party, moved.camera, moved.flags), false}; });
	bool failed = false;
	flow.reportAutomatic = [&](const auto &r) { failed = std::holds_alternative<XeenEventExecutionError>(r); };
	flow.handle(NavigationAction::MoveForward);
	check(failed && moved.camera.y == 2 && !moved.flags.isSet(7) && moved.party.questItems.at(17) == 1 &&
		moved.party.questFlags.isSet(2), "event failure rolled back earlier movement or immediate party effects");
	restoreEffects(moved);
}

void initialDispatchAndReconstruction() {
	Fixture f;
	f.dat[1][512 + 17] = 0x10;
	f.evt[1] = eventBytes({record(1, 1, 0, 12, {0, 0, 21, 99}), record(1, 1, 1, 12, {0, 0, 20, 7}),
		record(1, 1, 2, 0x1f, {2, 1, 1})});
	auto saved = f.capture(); saved.questItems[17] = 3;
	f.restore(saved);
	unsigned scripts = 0, reports = 0;
	XeenEventSystem events([&](XeenMapIdentity id) { ++scripts; return XeenEventScript(f.loadEvents(id)); });
	auto textFont = font();
	XeenEventFlow flow(f.world, events, f.party, f.camera, f.flags, textFont,
		[&](std::uint64_t) { return XeenEventFlow::Composition{Fixture::compose(f.world, f.party, f.camera, f.flags), false}; });
	flow.reportAutomatic = [&](const auto &) { ++reports; };
	check(scripts == 0 && reports == 0 && !flow.blocksGameplay() && !flow.presentationGeneration() &&
		!flow.updatePresentation() && flow.frame().pixels[0] == 1 && flow.frame().pixels[4] == 3,
		"fresh restored flow dispatched an initial event or retained presentation");
	sameSnapshot(f.capture(), saved);
	flow.handle(NavigationAction::TurnRight);
	check(reports == 1 && scripts == 2 && f.camera.mapId == XeenMapIdentity(2) &&
		f.party.questItems.at(17) == 4 && f.flags.isSet(7), "later automatic event behavior changed");
	Fixture fresh; fresh.copyResources(f);
	XeenEventSystem freshEvents([&](XeenMapIdentity id) { return XeenEventScript(fresh.loadEvents(id)); });
	XeenEventFlow newSession(fresh.world, freshEvents, fresh.party, fresh.camera, fresh.flags, textFont,
		[&](std::uint64_t) { return XeenEventFlow::Composition{Fixture::compose(fresh.world, fresh.party, fresh.camera, fresh.flags), false}; });
	newSession.initial();
	check(fresh.party.questItems.at(17) == 1 && fresh.flags.isSet(7) && fresh.camera.mapId == XeenMapIdentity(2),
		"explicit new-session initial event did not dispatch");

	Fixture a, b;
	auto sa = a.capture(); sa.disabledObjects = {{1, 0}};
	auto sb = b.capture(); sb.disabledObjects = {{1, 1}};
	a.restore(sa); b.restore(sb);
	const auto fa = Fixture::compose(a.world, a.party, a.camera, a.flags);
	const auto fb = Fixture::compose(b.world, b.party, b.camera, b.flags);
	check(fa.pixels != fb.pixels && fa.pixels[10] == 0 && fb.pixels[10] == 255,
		"same-count restoration reused the wrong identities/frame");

	Fixture reload;
	auto s = reload.incoming();
	reload.evt[1] = eventBytes({record(4, 5, 0, 0x29, {0}), record(4, 5, 1, 9, {44, 1, 2}), record(4, 5, 2, 0x12)});
	reload.restore(s);
	unsigned scriptReads = 0, textReads = 0;
	XeenEventSystem reloadedEvents([&](XeenMapIdentity id) { ++scriptReads; return XeenEventScript(reload.loadEvents(id)); },
		[&](XeenMapIdentity id) { ++textReads; return XeenEventTextFile{id, "synthetic.txt", true, {"Message"}}; });
	XeenEventFlow reloadedFlow(reload.world, reloadedEvents, reload.party, reload.camera, reload.flags, textFont,
		[&](std::uint64_t) { return XeenEventFlow::Composition{Fixture::compose(reload.world, reload.party, reload.camera, reload.flags), false}; });
	const auto first = reloadedFlow.frame();
	for (int pass = 0; pass < 2; ++pass) {
		const auto mapCount = reload.mapLoads, objectCount = reload.objectLoads;
		reload.world.discardMapCache(); reloadedEvents.discardScriptCache(); reloadedEvents.discardTextCache();
		check(reloadedFlow.refresh(true).pixels == first.pixels && reload.mapLoads > mapCount &&
			reload.objectLoads > objectCount, "cache rebuild lost restored first frame");
		reloadedFlow.handle(InteractionAction{});
		check(reloadedFlow.blocksGameplay(), "text reload did not reach acknowledgment");
		reloadedFlow.handle(AcknowledgeAction{});
		reloadedFlow.handle(AcknowledgeAction{}); // Clear the completed nonblocking label normally.
		check(scriptReads == static_cast<unsigned>(pass + 1) && textReads == static_cast<unsigned>(pass + 1),
			"script/text cache providers were not genuinely invoked");
		sameSnapshot(reload.capture(), s);
	}
}

} // namespace

int main() {
	try {
		fullRestoration(); legacyAndExplicitItems(); invalidResourceAndState(); characterPreflight(); mutationPolicies(); initialDispatchAndReconstruction();
		std::cout << "M20A save state: atomic preparation, resource identities, rules and owner/flow lifetimes passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n'; return 1;
	}
}
