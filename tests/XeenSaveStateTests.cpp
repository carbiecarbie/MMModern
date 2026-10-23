#include "XeenRegionalTestSupport.h"
#include "XeenSaveTestSupport.h"
#include "XeenDosTextTestSupport.h"
#include "XeenRemoveTestSupport.h"
#include "XeenMonsterTestAccess.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenMapFormat.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenSaveState.h"

#include <algorithm>
#include <chrono>
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

	XeenSaveSnapshot capture() const {
        XeenSaveSnapshot s;s.resources=signature;s.camera=camera;s.activeRosterIds=party.party.activeRosterIds();
        s.characters=party.roster.characters();s.questItems=party.questItems.counts();s.questFlags=party.questFlags.values();s.gameFlags=flags.values();
        const auto &state=world.sessionState();{const auto projected=state.disabledObjects();save_test::setRemovedObjects(s,{projected.begin(),projected.end()});};
        {const auto projected=state.disabledEvents();save_test::setDisabledEvents(s,{projected.begin(),projected.end()});};return s;
    }


};

XeenFontFormat font() {
	Bytes bytes(XeenFontFormat::kMinimumSize);
	for (int i = 0; i < 128; ++i) bytes[0x1000 + i] = 6;
	return XeenFontFormat(bytes);
}









void characterPreflight() {
	for (int mode = 0; mode < 9; ++mode) {
		XeenCharacter c; c.birthYear = 592; c.permanentLevel = 1;
		const auto maximum = std::numeric_limits<int>::max(), minimum = std::numeric_limits<int>::min();
		switch (mode) {
		case 0: c.permanentLevel = maximum; c.temporaryLevel = 1; break;
		case 1: c.permanentLevel = minimum; c.temporaryLevel = -1; break;
		case 2: c.temporaryAge = maximum; break;
		case 3: c.intellect = {maximum, 1}; break;
		case 4: c.intellect = {minimum, 0}; c.birthYear = 610; break;
		case 5: c.personality = {minimum + 1, 0}; c.conditions[4] = 255; break; // Unsigned Disease underflows, including FF.
		case 6: c.permanentLevel = maximum; break; // HP product.
		case 7: c.characterClass = XeenCharacterClass::Cleric; c.hasSpells = true;
			c.personality.permanent = 70000; c.permanentLevel = 100000000; break; // SP product.
		case 8: c.characterClass = XeenCharacterClass::Druid; c.hasSpells = true;
			c.personality.permanent = c.intellect.permanent = 70000;
			c.permanentLevel = 60000000; break; // Sum of two individually safe SP passes.
		}
		rejects([&]{XeenCharacterRules::validateForUse(c,{610});});
	}
	// Every defined class/race, relevant skill and byte-valued modifier category.
	for (unsigned race = 0; race < 5; ++race) for (unsigned type = 0; type < 10; ++type) {
		XeenCharacter value; value.race = static_cast<XeenRace>(race);
		value.characterClass = static_cast<XeenCharacterClass>(type);
		value.birthYear = 592; value.permanentLevel = 3; value.temporaryLevel = -2;
		value.intellect.permanent = value.personality.permanent = value.endurance.permanent = 70000;
		value.skills.fill(1); value.hasSpells = true;
		for (unsigned material = 0; material < 256; ++material) {
			value.weapons[0] = {static_cast<std::uint8_t>(material), 0, 0, 1};
			XeenCharacterRules::validateForUse(value, {610});
			check(XeenCharacterRules::maxHp(value, {610}) >= 0 &&
				XeenCharacterRules::maxSp(value, {610}) >= 0, "safe preflight changed defined rules");
		}
	}
}



void mutationPolicies() {
    mmodern::XeenProgramState programState;
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
		XeenEventFlow flow(programState,f.world, events, f.party, f.camera, f.flags, textFont,
			[&](std::uint64_t) { return XeenEventFlow::Composition{Fixture::compose(f.world, f.party, f.camera, f.flags), false}; });
		flow.dialogText=&dos_test::text();
		bool failed = false;std::optional<XeenEventExecutionState> suspended;
		flow.reportManual = [&](const auto &r) { failed = std::holds_alternative<XeenEventExecutionError>(r);
			if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&r))suspended=s->state; };
		flow.handle(InteractionAction{});
		if (outcome == 1) { check(flow.blocksGameplay(), "missing acknowledgment"); flow.abandonPresentation(); }
		if (outcome == 2) {
			check(flow.canCancelInteraction(), "missing WhoWill"); flow.handle(CancelInteractionAction{});
			for(const auto phase:{XeenRewardPhase::Blank,XeenRewardPhase::Found,XeenRewardPhase::Receipt}) {
				check(flow.blocksGameplay() && suspended && suspended->reward.phase==phase &&
					suspended->pendingPresentation->request.rewardWaitTicks==(phase==XeenRewardPhase::Blank?1u:phase==XeenRewardPhase::Found?10u:0u) &&
					f.party.questItems.at(17)==1 && !f.flags.isSet(7) && f.camera.mapId==XeenMapIdentity(1),
					"cancelled WhoWill skipped treasure, re-granted or committed camera/flags early");
				flow.handle(AcknowledgeAction{});
			}
		}
		check(!flow.blocksGameplay() && f.party.questItems.at(17) == 1 && f.party.questFlags.isSet(2) &&
			!f.party.questFlags.isSet(29), "immediate effects or later instruction policy changed");
		check(f.flags.isSet(7) == (outcome == 2) && f.camera.mapId == XeenMapIdentity(outcome == 2 ? 2 : 1),
			"WhoWill completion versus error/abandon camera/game-flag policy");
		check(failed == (outcome == 0) && f.world.sessionState().disabledObjectCount() == (outcome == 0 ? 1U : 0U) &&
			f.world.sessionState().disabledEventCount() == (outcome == 0 ? 2U : 0U), "Remove before error policy");
	}
	Fixture moved;
	moved.dat[1][512 + 33] = 0x10;
	moved.evt[1] = eventBytes({record(1, 2, 0, 12, {0, 0, 20, 7}), record(1, 2, 1, 12, {0, 0, 21, 99}),
		record(1, 2, 2, 12, {0, 0, 104, 2}), record(1, 2, 3, 0xff)});
	XeenEventSystem events([&](XeenMapIdentity id) { return XeenEventScript(moved.loadEvents(id)); });
	auto textFont = font();
	XeenEventFlow flow(programState,moved.world, events, moved.party, moved.camera, moved.flags, textFont,
		[&](std::uint64_t) { return XeenEventFlow::Composition{Fixture::compose(moved.world, moved.party, moved.camera, moved.flags), false}; });
	flow.dialogText=&dos_test::text();
	bool failed = false;
	flow.reportAutomatic = [&](const auto &r) { failed = std::holds_alternative<XeenEventExecutionError>(r); };
	flow.handle(NavigationAction::MoveForward);
	check(failed && moved.camera.y == 2 && !moved.flags.isSet(7) && moved.party.questItems.at(17) == 1 &&
		moved.party.questFlags.isSet(2), "event failure rolled back earlier movement or immediate party effects");
}



void journeyBorrowRefusal() {
    mmodern::XeenProgramState programState;
	Fixture f;
	XeenEventSystem events([&](XeenMapIdentity id) { return XeenEventScript(f.loadEvents(id)); },
		[](XeenMapIdentity id) { return XeenEventTextFile{id, "synthetic.txt", true, {}}; });
	auto textFont = font();
	XeenEventFlow flow(programState,f.world, events, f.party, f.camera, f.flags, textFont,
		[&](std::uint64_t) { return XeenEventFlow::Composition{Fixture::compose(f.world, f.party, f.camera, f.flags), false}; });
	flow.dialogText=&dos_test::text();
	auto snapshot = regional_test::snapshot(); snapshot.resources = f.signature;
	auto resources = f.resources();
	unsigned encounterReads = 0;
	resources.loadInitialCharacters = [&] { ++encounterReads; return Bytes{}; };
	resources.loadInitialContext = [&] { ++encounterReads; return XeenGameplayContext{}; };
	resources.loadMonsterStatistics = [&] { ++encounterReads; return std::vector<XeenMonsterRecord>{}; };
	const auto before = f.capture(); const auto initialReads = f.initialLoads;
	rejects([&] { XeenSaveState::restoreBeforeGameplay(snapshot, resources, f.party, f.camera, f.flags, f.world,
		[](XeenWorld &, const XeenPartyState &, const XeenCamera &, const XeenGameFlags &) {}); }, "unborrowed");
	check(encounterReads == 0 && initialReads == f.initialLoads, "borrowed restore invoked resource providers");
	sameSnapshot(before, f.capture());
}


struct CurrentDestination {
    mmodern::XeenProgramState programState;
 XeenPartyState party;XeenCamera camera{23,9,11,XeenDirection::West};XeenGameFlags flags;
 unsigned maps=0,objects=0,events=0,monsters=0,preflights=0;
 std::function<XeenMap(XeenMapIdentity)> mapProvider=regional_test::map;
 std::function<XeenObjectFile(XeenMapIdentity)> objectProvider=regional_test::objects;
 XeenWorld world{[&](auto id){++maps;return mapProvider(id);},[&](auto id){++objects;return objectProvider(id);}};
 XeenSaveState::Resources resources=regional_test::resources();
 std::unique_ptr<XeenEncounterFlow> flow;
 CurrentDestination(){resources.loadEvents=[&](auto id){++events;return regional_test::events(id);};
  resources.loadMonsterStatistics=[&]{++monsters;return regional_test::statistics();};}
 void restore(const XeenSaveSnapshot &s,const XeenSaveState::Preflight &custom={}){
  XeenSaveState::restoreBeforeGameplay(s,resources,party,camera,flags,world,[&](auto &w,const auto &p,const auto &c,const auto &f){
   ++preflights;check(!XeenSaveState::canCapture(p,c,w),"unpublished candidate was saveable");
   if(custom)custom(w,p,c,f);
  });
 }
 void bind(){flow=std::make_unique<XeenEncounterFlow>(programState,world,party,camera,flags,[]{return 0;},XeenJourneyRestoreTag{});
  check(flow->prepareJourneyFrame(flow->ticket(),[]{}),"restore frame preparation");
  check(flow->presentJourney(flow->ticket()),"restore frame publication");}
 XeenSaveSnapshot capture(){return XeenSaveState::capture(resources.signature,party,camera,flags,world);}
};
XeenSaveSnapshot currentSnapshot(){static const auto value=regional_test::snapshot();return value;}
void liveStatusRoundTrip(){
 // Load the existing v14 shape, inject only transient statuses, and compare the
 // complete current serializer (including RNG/time), rather than selected fields.
 const auto existing=XeenSaveFormat::encode(currentSnapshot());
 CurrentDestination plain;plain.restore(XeenSaveFormat::decode(existing));plain.bind();
 const auto canonical=XeenSaveFormat::encode(plain.capture());
 for(auto status:{XeenActorStatus::Sleep,XeenActorStatus::BeastMaster,XeenActorStatus::Hypnotize,XeenActorStatus::DragonSleep}) {
  CurrentDestination live;live.restore(XeenSaveFormat::decode(existing));
  const auto hp=live.world.sessionState().monsters(23)[0].hp;
  XeenMonsterTestAccess::set(live.world,23,0,hp,false,status);live.bind();
  const auto saved=XeenSaveFormat::encode(live.capture());
  check(saved==canonical,"Live control status must canonicalize without RNG/time or byte changes");
  CurrentDestination restored;restored.restore(XeenSaveFormat::decode(saved));restored.bind();
  check(restored.world.sessionState().monsters(23)[0].status==XeenActorStatus::Physical &&
   XeenSaveFormat::encode(restored.capture())==saved,"Status load must reset exactly without replay");
 }
 for(auto status:{XeenActorStatus::Unsupported,static_cast<XeenActorStatus>(255)}) {
  CurrentDestination bad;bad.restore(XeenSaveFormat::decode(existing));
  XeenMonsterTestAccess::set(bad.world,23,0,bad.world.sessionState().monsters(23)[0].hp,false,status);
  bool refused=false;try {bad.bind();(void)bad.capture();}catch(const std::exception &){refused=true;}
  check(refused,"Invalid/unsupported live status must not be silently canonicalized");
 }
}
void fullRestoration(){
 auto s=currentSnapshot();s.questItems[17]=3;s.questFlags[2]=true;s.gameFlags[7]=true;
 s.food=27;s.journey->context->year=611;s.journey->context->day=0;
 s.journey->context->minutes=299;s.journey->context->newDay=true;s.journey->context->rested=true;
 s.journey->context->effects[XeenPartyEffect::Levitate]=3;s.journey->context->lightAndResistances[1]=9;
 for(auto &pair:s.journey->supplements) {
  pair.inputs.resistances->fireTemporary=7;pair.inputs.resistances->energyTemporary=8;
  pair.inputs.resistances->magicTemporary=9;
 }
 s.characters[29].conditions[2]=255;s.characters[29].conditions[7]=128;
 save_test::setRemovedObjects(s,{{23,0}});save_test::setDisabledEvents(s,{{23,0},{23,1}});
 s.characters[29].currentSp=-50;s.characters[29].miscellaneous[8]={231,12,255,7};
 for(unsigned owner=0;owner<30;++owner)for(unsigned skill=0;skill<18;++skill)
  s.characters[owner].skills[skill]=static_cast<std::uint8_t>(owner*18+skill);
 s.map(23).explored.emplace();(*s.map(23).explored)[0]=1;(*s.map(23).explored)[31]=128;
 const auto expected=s;
 CurrentDestination f;f.party.questItems.increment(0);f.flags.set(1);f.world.disableObject({23,0});
 f.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(s)));f.bind();sameSnapshot(f.capture(),expected);
 check(f.party.firstSerializedCount==6 && f.party.effectiveSerializedCount==6,"current membership metadata");
 for(unsigned i=0;i<6;++i)check(&f.party.party.member(f.party.roster,i)==&f.party.roster.at(kXeenCombatOwners[i]),"restored membership lost roster ownership");
 check(f.world.sessionState().disabledObjectCount()==1 && f.world.sessionState().disabledEventCount()==2,"independent overlay identities changed");
 const auto evt=regional_test::events(23);
 check(f.world.effectiveEvent({23,0},evt.records[0]).opcode==0 && f.world.effectiveEvent({23,2},evt.records[2]).opcode==0x12,"Event overlay expanded by cell");
 const auto maps=f.maps,objects=f.objects,events=f.events;f.world.discardMapCache();f.world.map(23);f.world.objectFile(23);f.resources.loadEvents(23);
 check(f.maps>maps && f.objects>objects && f.events>events,"restored providers did not reload");sameSnapshot(f.capture(),expected);
 auto copy=f.capture();copy.characters[18].currentHp=123;save_test::setDisabledEvents(copy,{});sameSnapshot(f.capture(),expected);sameSnapshot(s,expected);
 for(unsigned category=0;category<6;++category){auto value=currentSnapshot();
  switch(category){case 0:value.characters[29].currentHp=-32768;break;case 1:value.questItems[34]=0xffffffffU;break;
   case 2:value.questFlags[29]=true;break;case 3:value.gameFlags[255]=true;break;case 4:save_test::setRemovedObjects(value,{{23,0}});break;case 5:save_test::setDisabledEvents(value,{{23,1}});break;}
  CurrentDestination next;next.restore(value);next.bind();sameSnapshot(next.capture(),value);
 }
}
// Synthetic resources for the per-map tests: F3 stays the outdoor mainland;
// every other map is an indoor tile with three objects and `monsters` MOB
// monsters of type 8 at (i%16,1+i/16); MON type 0 copies type 8 so that a
// spawned script slot has hit points; every other map's EVT has four records.
XeenMap indoorMap(XeenMapIdentity id) {
 auto m=regional_test::map(id);if(id==XeenMapIdentity(23))return m;
 m.geometry.flags2=0;for(auto &c:m.geometry.cells){c.rawWord=0;c.rawAttributes=0;c.flags=0;c.geometry=XeenIndoorWalls{};}
 return m;
}
XeenObjectFile indoorObjects(XeenMapIdentity id,unsigned monsters) {
 auto o=regional_test::objects(id);if(id==XeenMapIdentity(23))return o;
 o.entities.objects.clear();
 for(int n:{0,2,3})o.entities.objects.push_back(n?XeenMapEntity{n,n,0,0,7}:XeenMapEntity{9,11,0,0,7});
 for(unsigned i=0;i<monsters;++i)o.entities.monsters.push_back({int(i%16),int(1+i/16),0,0,8});
 o.entities.wallItems=std::vector<XeenMapEntity>{{2,3,0,0,7},{4,5,1,0,8},{6,7,2,0,9}};
 return o;
}
XeenEventFile indoorEvents(XeenMapIdentity id) {
 if(id==XeenMapIdentity(23) || id==XeenMapIdentity(28))return regional_test::events(id);
 return XeenEventFile{id,"synthetic.evt",true,std::vector<XeenEventRecord>(4)};
}
void usePerMapResources(CurrentDestination &f,unsigned monsters) {
 f.mapProvider=indoorMap;f.objectProvider=[monsters](auto id){return indoorObjects(id,monsters);};
 f.resources.loadEvents=[&f](auto id){++f.events;return indoorEvents(id);};
 f.resources.loadMonsterStatistics=[&f]{++f.monsters;auto s=regional_test::statistics();s[0]=s[8];return s;};
}
// A map table saved as restore stages it: every MOB record Present at its MOB cell.
XeenSaveMonsterTable mobTable(unsigned monsters) {
 auto table=monsterTable(monsters);
 for(unsigned i=0;i<monsters;++i){table.slots[i].x=int(i%16);table.slots[i].y=int(1+i/16);}
 return table;
}
// M69 decision 5: capture, encode, decode, restore, capture again. Both the
// mainland and the city hold state in every table restore accepts: monster
// tables (a defeated, accounted record), moved and removed objects, indoor and
// outdoor cells (wall faces, the unlocked bit, the outdoor surface nibble), wall
// items, arbitrary Event opcodes, explored bits, the head cache and a dormant
// treasure item's source identity. mapEventRoundTrips produces these through Events.
void genericRoundTrip(){
 auto s=currentSnapshot();
 s.soundOptions={false,false,true,false};
 for(unsigned i=0;i<30;++i) {auto &c=s.characters[i];c.lloydMap=23+i;c.lloydX=i;c.lloydY=31-i;c.lloydSide=i%2;}
 auto &f3=s.map(23);
 auto &orc=f3.monsters->slots[2];orc.x=orc.y=-128;orc.lifecycle=XeenActorLifecycle::Defeated;orc.accounted=true;
 f3.objects={{0,-128,-128,false}};f3.events={{0,0},{1,0}};
 f3.cells={{17,0x0906,0,1,0}}; // Outdoor surface plus an independent wall-word layer.
 f3.explored=std::array<std::uint8_t,32>{};(*f3.explored)[9]=0x10;
 f3.tavernTips=14;
 s.journey->purse->gold=0x12345678;s.journey->purse->gems=0x87654321;
 auto &city=s.map(28);
 city.monsters=mobTable(4);
 auto &slime=city.monsters->slots[1];slime.x=slime.y=-128;slime.lifecycle=XeenActorLifecycle::Defeated;slime.accounted=true;
 city.objects={{1,4,5,true},{2,-128,-128,false}};
 // A grate on the east face of (5,5) and the west face of (6,5), both unlocked.
 city.cells={{5*16+5,0x0900,0x80,0,0},{5*16+6,0x0009,0x80,0,0}};
 city.wallItems={{0,-128,127},{2,9,10}};
 city.events={{763,42},{764,0}};
 // A map never entered can still own MOB/EVT changes; no monster staging.
 s.map(37).wallItems={{1,-1,-2}};s.map(37).events={{1,255}};
 city.explored=std::array<std::uint8_t,32>{};(*city.explored)[31]=0x80;
 city.tavernTips=0;
 s.headCache=XeenHeadCache{23,{}};s.headCache->overrides[9]=XeenHeadIndicators{1,2};
 const auto bytes=XeenSaveFormat::encode(s);
 CurrentDestination live;usePerMapResources(live,4);live.restore(XeenSaveFormat::decode(bytes));live.bind();
 check(XeenSoundOptions(live.party.soundOptions)==s.soundOptions,"Sound options restored exactly");
 const auto &state=live.world.sessionState();
 check(state.mapState(28)->cells.size()==2 && state.mapState(28)->cells[0].mask==2 && state.mapState(28)->cells[1].mask==8 &&
  state.mapState(28)->cells[0].unlocked && state.isObjectDisabled({28,2}) && !state.isObjectDisabled({28,1}) &&
  live.world.effectiveObject({28,1}).x==4 && live.world.effectiveObjectFlip({28,1}) && state.isEventDisabled({28,764}) &&
  state.isEventDisabled({23,1}) && state.isObjectDisabled({23,0}) && state.monsters(28).size()==4 &&
  state.accountedMonsters().count({28,1}) &&
  state.mapState(28)->originalMonsterCount==4 && *live.party.purse==*s.journey->purse &&
  live.world.treasureStaging().value()==XeenTreasureStaging{},
  "Restore distributes every table to its own map and the purse, granting no pending treasure");
 check(live.world.effectiveWallItem({28,0}).x==-128 && live.world.effectiveWallItem({28,2}).y==10 &&
  live.world.effectiveWallItem({37,1}).y==-2 && live.world.effectiveEvent({28,763},indoorEvents(28).records[763]).opcode==42 &&
  live.world.effectiveEvent({37,1},indoorEvents(37).records[1]).opcode==255 && !state.mapState(37)->monsters,
  "Restored wall items and arbitrary opcodes use logical map and original record identities");
 check(live.world.sampleCell(23,1,1)->cell->surfaceIndex==6 &&
  xeenGet<XeenOutdoorLayers>(live.world.sampleCell(23,1,1)->cell->geometry).top==9,
  "Outdoor v13 cells did not restore effective surface and wall-word layers");
 live.world.discardMapCache();
 check(live.world.tavernTipIndex(23)==14 && live.world.tavernTipIndex(28)==0 &&
  live.world.map(23).geometry.tavernTips==regional_test::map(23).geometry.tavernTips,
  "Restored tip cursors survive cache reload without modifying original DAT bytes");
 check(live.world.effectiveWallItem({28,2}).x==9 && live.world.objectFile(28).entities.wallItems[2].x==6,
  "Resource cache reload keeps effective wall-item positions and immutable MOB preimages");
 auto revisit=live.world.transitionCandidate();
 check(revisit->effectiveWallItem({37,1}).x==-1 && revisit->effectiveEvent({37,1},indoorEvents(37).records[1]).opcode==255,
  "Map revisit candidates retain wall-item and Event owners");
 const auto captured=live.capture();sameSnapshot(captured,s);
 const auto wire=XeenSaveFormat::encode(captured);check(wire==bytes,"Capture after restore writes the restored bytes");
 CurrentDestination again;usePerMapResources(again,4);again.restore(XeenSaveFormat::decode(wire));again.bind();
 check(XeenSaveFormat::encode(again.capture())==bytes,"Generic per-map round trip is exact");
 // Older builds saved Spawn slots past the MOB count (v0.2.0 Vertigo 46-51).
 // DOS never keeps them (writer 0x266e3..0x2671c, load 0x0f5be): drop them.
 auto older=s;auto &extra=older.map(28).monsters->slots;
 extra.push_back({0,0,XeenActorLifecycle::Unresolved,XeenActorStatus::Physical,false,-1});
 extra.push_back({7,7,XeenActorLifecycle::Present,XeenActorStatus::Physical,false,0});
 extra.push_back({-128,-128,XeenActorLifecycle::Defeated,XeenActorStatus::Physical,true,0});
 CurrentDestination dropped;usePerMapResources(dropped,4);dropped.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(older)));dropped.bind();
 check(dropped.world.sessionState().monsters(28).size()==4 && !dropped.world.sessionState().accountedMonsters().count({28,6}) &&
  XeenSaveFormat::encode(dropped.capture())==bytes,"Saved slots past the MOB count were not dropped on load");
 // DOS saves and reloads stacked cells (no cap outside movement, 0x1fc24).
 auto stacked=s;
 for(auto &slot:stacked.map(28).monsters->slots){slot.x=5;slot.y=5;slot.lifecycle=XeenActorLifecycle::Present;slot.accounted=false;}
 CurrentDestination four;usePerMapResources(four,4);four.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(stacked)));four.bind();
 check(XeenActorApproach::occupancy(four.world.sessionState().monsters(28))[5*32+5]==4 &&
  XeenSaveFormat::encode(four.capture())==XeenSaveFormat::encode(stacked),"A stacked cell of four did not round-trip exactly");
 // Pending treasure is running-program state: states that differ only in it
 // capture the same bytes, capture leaves it unchanged, and a later restore
 // into the same program neither grants nor clears the staged items.
 XeenTreasureStaging staged;staged.weapons[0]={0,30,0,0};staged.accessories[9]={0,1,0,0};staged.miscellaneous[0]={3,8,7,0};
 XeenMonsterTestAccess::stageTreasure(*live.flow,live.world,staged);
 check(XeenSaveFormat::encode(live.capture())==bytes && live.world.treasureStaging().value()==staged,
  "Pending treasure reached the save or changed on capture");
 // Map transitions keep it: a candidate stages on its own copy and an
 // abandoned candidate leaves the owner unchanged.
 {
  auto candidate=live.world.transitionCandidate();const auto revision=live.world.treasureStaging().revision();
  check(&candidate->treasureStaging()!=&live.world.treasureStaging() && candidate->treasureStaging().value()==staged,
   "Transition candidate does not stage on a copy");
  auto changed=staged;changed.gold=5;candidate->treasureStaging().publish(changed);candidate.reset();
  check(live.world.treasureStaging().value()==staged && live.world.treasureStaging().revision()==revision,
   "Abandoned transition changed pending treasure");
 }
 // A published candidate returns only its own staging changes, and only while
 // the live revision it copied is current; a stale copy never restores an
 // older value.
 {
  CurrentDestination transit;usePerMapResources(transit,4);transit.restore(XeenSaveFormat::decode(bytes));transit.bind();
  auto &w=transit.world;XeenMonsterTestAccess::stageTreasure(*transit.flow,w,staged);
  auto unchanged=w.transitionCandidate();const auto revision=w.treasureStaging().revision();
  w.checkTransitionTreasure(*unchanged);w.publishTransition(*unchanged);
  check(w.treasureStaging().value()==staged && w.treasureStaging().revision()==revision,"An unchanged transition copy published pending treasure");
  auto stale=w.transitionCandidate();auto later=staged;later.gold=20;w.treasureStaging().publish(later);
  rejects([&]{w.checkTransitionTreasure(*stale);});
  w.publishTransition(*stale);
  check(w.treasureStaging().value()==later,"A stale unchanged transition copy restored older pending treasure");
  auto moved=later;moved.armor[5]={0,4,0,0};
  auto changed=w.transitionCandidate();changed->treasureStaging().publish(moved);
  w.checkTransitionTreasure(*changed);w.publishTransition(*changed);
  check(w.treasureStaging().value()==moved,"A changed transition copy was not published");
 }
 // A write between restore and the flow's binding, even one that restores its
 // value, leaves the restoration stale when the world joins the program.
 {
  CurrentDestination touched;usePerMapResources(touched,4);touched.restore(XeenSaveFormat::decode(bytes));
  ++touched.party.food;--touched.party.food;
  bool refused=false;try{touched.bind();}catch(const std::logic_error &){refused=true;}
  check(refused,"A write between restore and flow binding was adopted");
 }
 // AlterMap deliberately writes just one selected cell, without mirroring.
 auto oneSided=s;oneSided.map(28).cells.pop_back();
 CurrentDestination single;usePerMapResources(single,4);single.restore(oneSided);single.bind();
 check(XeenSaveFormat::encode(single.capture())==XeenSaveFormat::encode(oneSided),"One-sided Event wall failed v13 round trip");
 auto parking=s;parking.map(28).monsters->slots[2].x=100;parking.map(28).monsters->slots[2].y=100;
 CurrentDestination parked;usePerMapResources(parked,4);
 parked.objectProvider=[](auto id){auto o=indoorObjects(id,4);if(id==XeenMapIdentity(28))o.entities.monsters[2].x=o.entities.monsters[2].y=100;return o;};
 parked.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(parking)));parked.bind();
 check(XeenSaveFormat::encode(parked.capture())==XeenSaveFormat::encode(parking),"Original Spawn parking coordinates failed exact v13 round trip");
}
// Restore an opcode whose source resource is None, execute the replacement,
// suspend for its reward, then Remove its cell. Publication checks must retain
// the effective dispatch after deletion and across every presentation resume.
void replacedOpcodePublication(){
    mmodern::XeenProgramState programState;
 auto initial=currentSnapshot();initial.map(23).events={{0,0x2c}};
 auto source=regional_test::events(23);
 source.records.clear();
 const auto add=[&](unsigned line,unsigned opcode,std::vector<std::uint8_t> parameters={}) {
  auto r=record(9,11,line,opcode,std::move(parameters));r.direction=4;source.records.push_back(std::move(r));
 };
 add(0,0,{70,37,0,1});add(1,0x0e);add(2,0x12);
 // An unreachable, valid NPC keeps the existing mainland interaction context.
 add(99,0x05,{0,0,0,1,0});
 CurrentDestination live;
 live.resources.loadEvents=[&](auto id){return id==XeenMapIdentity(23)?source:regional_test::events(id);};
 live.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(initial)));
 XeenEventSystem events([&](auto id){return XeenEventScript(live.resources.loadEvents(id));},regional_test::texts);
 const auto display=font();IndexedFrame image;image.width=320;image.height=200;image.pixels.resize(64000);
 std::uint64_t now=0,cycle=0;
 XeenEventFlow flow(programState,live.world,events,live.party,live.camera,live.flags,display,
  [&](auto){return XeenEventFlow::Composition{image,false};},{},[&]{return now;},{},nullptr,
  [&](auto,auto){return XeenEventFlow::Composition{image,false};},nullptr,
  [&](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{image,false};});
 flow.dialogText=&dos_test::text();unsigned failures=0;
 flow.reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r)){
  ++failures;std::cerr<<"Replacement publication: "<<e->message<<'\n';}};
 const auto present=[&](const IndexedFrame &frame){if(frame.presentation())flow.framePresented(frame.presentation());};
 present(flow.frame());flow.beginCycle(++cycle);
 present(flow.handle(InteractionAction{},flow.displayedInput(),flow.frame().presentation()));
 for(unsigned pulse=0;pulse<1000 && !flow.canSave();++pulse) {
  now+=125;flow.beginCycle(++cycle);
  if(const auto frame=flow.updatePresentation())present(*frame);
  present(flow.handle(AcknowledgeAction{},flow.displayedInput(),flow.frame().presentation()));
 }
 check(!failures && !flow.blocksGameplay() && live.world.sessionState().isEventDisabled({23,0}) &&
  live.world.sessionState().mapState(23)->eventOverrides.empty(),
  "A replaced opcode must publish through the normal reward and Remove owners");
 unsigned delivered=0;for(auto id:live.party.party.activeRosterIds())
  for(const auto &item:live.party.roster.at(id).miscellaneous)if(item.id==37)++delivered;
 check(delivered==1,"The replaced GiveEnchanted opcode must grant exactly once across suspension and deletion");
}
// Each producer runs through the normal detached Event/publication flow,
// leaves the indoor map, returns, then uses the current serializer/restore:
// MoveWallObj, called MakeNothingHere, AlterMap direction 4 through a neighbour,
// indoor AlterCellFlag, nonzero AlterEvent, Spawn, outdoor AlterCellFlag/AlterMap
// and MoveObj (a move and a same-position flip).
void mapEventRoundTrips(){
    mmodern::XeenProgramState programState;
 for(unsigned operation=0;operation<10;++operation) {
  const bool outdoor=operation==7 || operation==8;
  const XeenMapIdentity active=outdoor?23:28,other=outdoor?28:23;
  auto initial=currentSnapshot();initial.camera={active,9,11,XeenDirection::North};
  initial.map(28).monsters=mobTable(4);
  if(operation==5) {auto &a=initial.map(28).monsters->slots[0];a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
  auto city=XeenEventFile{active,"synthetic.evt",true,{}};
  const auto add=[&](unsigned x,unsigned y,unsigned line,unsigned op,std::vector<std::uint8_t> p={}) {
   auto r=record(x,y,line,op,std::move(p));r.direction=4;city.records.push_back(r);
  };
  switch(operation) {
  case 0:add(9,11,0,34,{2,13,13});break;
  case 1:add(9,11,0,25,{100,100,0});add(100,100,0,46);add(100,100,1,34,{2,13,13});add(100,100,2,26);break;
  case 2:add(9,11,0,19,{25,5,4,8});break;
  case 3:add(9,11,0,35,{25,5,6});break;
  case 4:add(9,11,0,24,{3,34});break;
  case 5:add(9,11,0,16,{0,7,11,0});break;
  case 6:add(9,11,0,16,{7,5,6,0});break;
  case 7:add(9,11,0,35,{3,4,6});add(9,11,1,19,{17,4,0,8});break;
  case 8:add(9,11,0,16,{22,5,6,0});break;
  // MOB record 1 moves from (2,2); record 2 is already at (3,3), so it flips.
  case 9:add(9,11,0,11,{1,4,5});add(9,11,1,11,{2,3,3});break;
  }
  // Called deletion clears the physical suffix: that case leaves/returns in
  // a separate candidate below; all others use actual travel publication.
  if(operation==1)add(9,11,1,18);else add(9,11,operation==7 || operation==9?2:1,7,{std::uint8_t(other.number),9,11});
  add(9,11,3,0,{2,12,12});
  if(outdoor)add(9,11,99,5,{0,0,0,1,0});
  auto mainland=regional_test::events(other);mainland.records.clear();
  auto travel=record(9,11,0,7,{std::uint8_t(active.number),9,11});travel.direction=4;mainland.records.push_back(travel);
  mainland.records.push_back(record(9,11,99,5,{0,0,0,1,0}));
  const auto configure=[&](CurrentDestination &f) {
   usePerMapResources(f,4);
   f.mapProvider=[](auto id){auto m=id==XeenMapIdentity(24)?regional_test::map(id):indoorMap(id);
    if(id==XeenMapIdentity(28))m.geometry.neighbors=std::array<std::uint16_t,4>{30,29,0,0};
    if(id==XeenMapIdentity(30))m.geometry.neighbors[1]=31;
    if(id==XeenMapIdentity(29))m.geometry.neighbors[0]=31;
    if(id==XeenMapIdentity(23))m.geometry.neighbors[1]=24;return m;};
   f.resources.loadEvents=[&](auto id){return id==active?city:id==other?mainland:indoorEvents(id);};
   f.resources.loadRegionalText=[](auto id){auto t=regional_test::texts(id);if(id==XeenMapIdentity(23))t.strings.resize(44);return t;};
  };
  CurrentDestination live;configure(live);live.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(initial)));
  if(operation==5) {
   XeenMonsterTestAccess::set(live.world,28,0,0,false,XeenActorStatus::DragonSleep);
   auto spawned=live.world.transitionCandidate();spawned->applySpawn(28,0,7,11,0);
   check(spawned->sessionState().monsters(28)[0].status==XeenActorStatus::Physical,
    "Spawn must reset status before any map reload");
  }
  XeenEventSystem events([&](auto id){return XeenEventScript(live.resources.loadEvents(id));},live.resources.loadRegionalText);
  const auto display=font();IndexedFrame image;image.width=320;image.height=200;image.pixels.resize(64000);
  std::uint64_t now=0,cycle=0;
  XeenEventFlow flow(programState,live.world,events,live.party,live.camera,live.flags,display,
   [&](auto){return XeenEventFlow::Composition{image,false};},{},[&]{return now;},{},nullptr,
   [&](auto,auto){return XeenEventFlow::Composition{image,false};},nullptr,
   [&](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{image,false};});
  flow.dialogText=&dos_test::text();unsigned failures=0;
  flow.reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r)){
   ++failures;std::cerr<<"Map round trip "<<operation<<": "<<e->message<<'\n';}};
  const auto present=[&](const IndexedFrame &frame){if(frame.presentation())flow.framePresented(frame.presentation());};
  const auto act=[&] {
   present(flow.frame());flow.beginCycle(++cycle);
   present(flow.handle(InteractionAction{},flow.displayedInput(),flow.frame().presentation()));
   for(unsigned n=0;n<1000 && !flow.canSave();++n) {now+=125;flow.beginCycle(++cycle);
    if(const auto frame=flow.updatePresentation())present(*frame);
    present(flow.handle(AcknowledgeAction{},flow.displayedInput(),flow.frame().presentation()));}
   check(!failures && flow.canSave(),"Persistent Event failed publication or quiet capture");
  };
  act();
  if(operation!=1) {check(live.camera.mapId==other,"Event did not leave map");act();}
  check(live.camera.mapId==active,"Event did not return to map");
  auto revisit=live.world.transitionCandidate();revisit->leaveMap(active);revisit->discardMapCache();
  const auto snapshot=live.capture();const auto bytes=XeenSaveFormat::encode(snapshot);
  const auto *map=snapshot.find(28);check(map,"Persistent Event lost logical owner");
  if(operation<2)check(revisit->effectiveWallItem({28,2}).x==13 && map->wallItems.size()==1,"Wall item lost after leave/return");
  if(operation==1)check(map->events.size()==3 && !revisit->isEventDisabled({28,1}),"Called deletion did not retain physical-only identities");
  if(operation==2)check(revisit->sampleCell(28,25,5)->cell->rawWord==0x8888 && snapshot.find(29)->cells.size()==1,"Wall change lost physical owner");
  if(operation==3)check(revisit->sampleCell(28,25,5)->cell->surfaceIndex==6 && snapshot.find(29)->cells.size()==1,"Surface change lost physical owner");
  if(operation==4)check(revisit->effectiveEvent({28,2},city.records[2]).opcode==34,"Nonzero replacement lost after leave/return");
  if(operation==5)check(map->monsters->slots[0].lifecycle==XeenActorLifecycle::Present && live.world.sessionState().monsters(28)[0].status==XeenActorStatus::Physical,"Spawn must reset retained control status");
  if(operation==6)check(map->monsters->slots.size()==4 && revisit->sessionState().monsters(28).size()==4,
   "Spawn past the MOB count materialized or persisted a slot");
  if(operation==7)check(revisit->sampleCell(23,3,4)->cell->surfaceIndex==6 &&
   revisit->sampleCell(24,1,4)->cell->rawWord==1 && !snapshot.find(24),"Outdoor surface or visit-only neighbour lifetime failed v13 capture");
  if(operation==8)check(snapshot.find(23)->monsters->slots.size()==snapshot.find(23)->monsters->originalCount,
   "Outdoor Spawn past the MOB count materialized or persisted a slot");
  if(operation==9)check(map->objects==std::vector<XeenSaveObjectOverride>{{1,4,5,false},{2,3,3,true}} &&
   revisit->effectiveObject({28,1}).x==4 && revisit->effectiveObjectFlip({28,2}),"MoveObj position or flip lost after leave/return");
  auto neutral=snapshot;neutral.maps=initial.maps;neutral.camera=initial.camera;
  auto expected=initial;
  // Part 0: teleport 0x27112 clears Space. Only the non-travel called deletion keeps Wait.
  expected.journey->context->minutes+=operation==1?1:0;
  expected.journey->context->ctr24=(expected.journey->context->ctr24+(operation==1?1:0))%24;
  check(XeenSaveFormat::encode(neutral)==XeenSaveFormat::encode(expected),"Map Event changed more than its completed Space scheduling or consumed Journey RNG");
  CurrentDestination loaded;configure(loaded);loaded.restore(XeenSaveFormat::decode(bytes));loaded.bind();
  check(XeenSaveFormat::encode(loaded.capture())==bytes,"Persistent Event did not round-trip exactly through v13");
  if(operation==9)check(loaded.world.effectiveObject({28,1}).y==5 && !loaded.world.effectiveObjectFlip({28,1}) &&
   loaded.world.effectiveObject({28,2}).x==3 && loaded.world.effectiveObjectFlip({28,2}),"Restored MoveObj position or flip differs");
 }
}
// Remove only writes the (-128,-128) position (scripts.cpp cmdRemove), so a
// later MoveObj on the same object shows it again; one position per record.
void removeThenMoveRoundTrip(){
    mmodern::XeenProgramState programState;
 const auto script=[](XeenMapIdentity id,std::vector<XeenEventRecord> records) {
  XeenEventFile file{id,"synthetic.evt",true,{}};
  for(auto &r:records){r.direction=4;file.records.push_back(r);}
  return file;
 };
 // Remove runs on the mainland (the city preflight refuses it); MoveObj runs
 // in the city, from a save holding the removed object.
 const auto mainland=script(23,{record(9,11,0,14),record(9,11,99,5,{0,0,0,1,0})});
 const auto city=script(28,{record(9,11,0,11,{0,13,13})});
 const auto configure=[&](CurrentDestination &f) {
  usePerMapResources(f,4);
  f.resources.loadEvents=[&](auto id){return id==XeenMapIdentity(23)?mainland:id==XeenMapIdentity(28)?city:indoorEvents(id);};
 };
 // One manual Event at the camera cell, captured at the quiet boundary while
 // its flow still holds the capture authority.
 const auto interact=[&](CurrentDestination &f) {
  XeenEventSystem events([&](auto id){return XeenEventScript(f.resources.loadEvents(id));},f.resources.loadRegionalText);
  const auto display=font();IndexedFrame image;image.width=320;image.height=200;image.pixels.resize(64000);
  std::uint64_t now=0,cycle=0;
  XeenEventFlow flow(programState,f.world,events,f.party,f.camera,f.flags,display,
   [&](auto){return XeenEventFlow::Composition{image,false};},{},[&]{return now;},{},nullptr,
   [&](auto,auto){return XeenEventFlow::Composition{image,false};},nullptr,
   [&](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{image,false};});
  flow.dialogText=&dos_test::text();unsigned failures=0;
  flow.reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r)){
   ++failures;std::cerr<<"Remove then MoveObj: "<<e->message<<'\n';}};
  const auto present=[&](const IndexedFrame &frame){if(frame.presentation())flow.framePresented(frame.presentation());};
  present(flow.frame());flow.beginCycle(++cycle);
  present(flow.handle(InteractionAction{},flow.displayedInput(),flow.frame().presentation()));
  for(unsigned n=0;n<1000 && !flow.canSave();++n) {now+=125;flow.beginCycle(++cycle);
   if(const auto frame=flow.updatePresentation())present(*frame);
   present(flow.handle(AcknowledgeAction{},flow.displayedInput(),flow.frame().presentation()));}
  check(!failures && flow.canSave(),"Remove/MoveObj failed publication or quiet capture");
  return f.capture();
 };
 const auto sameObjects=[](const XeenWorld &a,const XeenWorld &b,XeenMapIdentity id) {
  const auto *x=a.sessionState().mapState(id),*y=b.sessionState().mapState(id);
  return x && y && x->removedObjects==y->removedObjects && x->objectOverrides==y->objectOverrides;
 };
 // Remove after an in-place MoveObj flip: the removed position replaces the
 // moved one, and the flip stays in memory only.
 auto initial=currentSnapshot();initial.camera={23,9,11,XeenDirection::North};
 initial.map(23).objects={{0,9,11,true}};
 const XeenObjectIdentity statue{23,0};
 CurrentDestination live;configure(live);live.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(initial)));
 const auto removed=interact(live);const auto *tile=removed.find(23);
 check(live.world.isObjectDisabled(statue) && live.world.sessionState().mapState(23)->objectOverrides.empty() &&
  live.world.effectiveObjectFlip(statue) && tile && tile->objects.size()==1 && tile->objects[0].x==-128 && tile->objects[0].y==-128,
  "Remove did not replace the moved position with (-128,-128) or lost the flip in memory");
 CurrentDestination loadedRemoved;configure(loadedRemoved);loadedRemoved.restore(removed);loadedRemoved.bind();
 check(sameObjects(loadedRemoved.world,live.world,23) &&
  XeenSaveFormat::encode(loadedRemoved.capture())==XeenSaveFormat::encode(removed),"Removed object did not round-trip exactly");
 // MoveObj on an object a loaded save holds as removed.
 auto later=currentSnapshot();later.camera={28,9,11,XeenDirection::North};
 later.map(28).monsters=mobTable(4);later.map(28).objects={{0,-128,-128,false}};
 const XeenObjectIdentity id{28,0};
 CurrentDestination moving;configure(moving);moving.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(later)));
 check(moving.world.isObjectDisabled(id),"Saved removed object was not restored hidden");
 const auto moved=interact(moving);const auto bytes=XeenSaveFormat::encode(moved);
 const auto object=moving.world.effectiveObject(id);
 check(!moving.world.isObjectDisabled(id) && object.x==13 && object.y==13 && !moving.world.effectiveObjectFlip(id) &&
  moving.world.selectObject({28,13,13,XeenDirection::North})==id,"MoveObj did not show the removed object at its new position");
 check(moved.find(28)->objects.size()==1 && moved.find(28)->objects[0].x==13 && moved.find(28)->objects[0].y==13,
  "Capture dropped the moved position");
 CurrentDestination loaded;configure(loaded);loaded.restore(XeenSaveFormat::decode(bytes));loaded.bind();
 check(!loaded.world.isObjectDisabled(id) && loaded.world.effectiveObject(id).x==13 &&
  sameObjects(loaded.world,moving.world,28) &&
  XeenSaveFormat::encode(loaded.capture())==bytes,"Remove then MoveObj did not round-trip exactly");
}
// M70 Part C: every travel form reaches the normal quiet capture boundary.
void travelRoundTrips() {
    mmodern::XeenProgramState programState;
 for(unsigned form=0;form<20;++form) {
  const bool lateRefused=form>=16,publishedPrefix=lateRefused && form%2;
  const bool refused=form>=12,continued=lateRefused || form%2;
  const bool mirror=(form>=6 && form<12) || (form>=14 && form<16),run=(form>=2 && form<4) || (form>=8 && form<12);
  const bool same=(form>=4 && form<6) || (form>=10 && form<12) || form>=18;
  const XeenMapIdentity destination=refused && !lateRefused?99:same?28:23;
  auto initial=currentSnapshot();initial.camera={28,9,11,XeenDirection::West};initial.map(28).monsters=mobTable(4);
  XeenEventFile source{28,"synthetic.evt",true,{}};
  const auto add=[&](unsigned x,unsigned y,unsigned line,unsigned op,Bytes args={}) {
   auto r=record(x,y,line,op,std::move(args));r.direction=4;source.records.push_back(r);
  };
  unsigned entry=0;
  if(refused && !lateRefused) {add(9,11,0,20,{100,5,0,0,0});add(9,11,1,34,{0,13,13});entry=2;}
  if(mirror) {add(9,11,entry,21,{0,std::uint8_t(entry+2),0,0});add(9,11,entry+1,18);entry+=2;}
  add(9,11,entry,continued?31:7,mirror?Bytes{0}:Bytes{std::uint8_t(destination.number),std::uint8_t(run?255:10),11});
  // Continuing at the destination writes exactly once; Exit never runs it.
  XeenEventFile target{destination,"synthetic.evt",true,{}};
  // A destination suspension must retain that destination's complete script,
  // rather than comparing it to the source Event after Continue.
  auto message=record(10,11,0,1,{0});message.direction=4;target.records.push_back(message);
  auto prompt=record(10,11,1,21,{1,3,0,0});prompt.direction=4;target.records.push_back(prompt);
  auto wrong=record(10,11,2,18);wrong.direction=4;target.records.push_back(wrong);
  auto write=lateRefused && !publishedPrefix?record(10,11,3,7,{99,1,1}):
   record(10,11,3,12,publishedPrefix?Bytes{0,0,65,1}:Bytes{0,0,21,99});
  write.direction=4;target.records.push_back(write);
  auto exit=lateRefused && publishedPrefix?record(10,11,4,7,{99,1,1}):record(10,11,4,18);
  exit.direction=4;target.records.push_back(exit);
  if(same)source.records.insert(source.records.end(),target.records.begin(),target.records.end());
  const auto configure=[&](CurrentDestination &f) {
   usePerMapResources(f,4);
   f.mapProvider=[](auto id){if(id==XeenMapIdentity(99))throw std::runtime_error("Absent fixture DAT");auto map=indoorMap(id);map.geometry.runX=10;map.geometry.runY=11;
    if(id==XeenMapIdentity(28))map.geometry.neighbors=std::array<std::uint16_t,4>{30,29,0,0};
    if(id==XeenMapIdentity(30))map.geometry.neighbors[1]=31;
    if(id==XeenMapIdentity(29))map.geometry.neighbors[0]=31;return map;};
   f.resources.loadEvents=[&](auto id){return id==XeenMapIdentity(28)?source:id==destination?target:indoorEvents(id);};
   f.resources.loadRegionalText=[&](auto id){auto t=regional_test::texts(id);t.strings.resize(id==XeenMapIdentity(23)?44:64);t.strings[1]="Continue";
    t.mirrors={{"Travel",std::uint8_t(destination.number),run?0:10,run?0:11,form>=10 && form<12?2:-1}};return t;};
  };
  CurrentDestination live;configure(live);live.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(initial)));
  XeenMonsterTestAccess::set(live.world,28,0,3,false);
  const auto display=font();IndexedFrame image{320,200,Bytes(64000)};std::uint64_t now=0,cycle=0;
  const XeenWorld *composedWorld=nullptr;XeenCamera composedCamera;unsigned destinationSuspensions=0;
  bool wrongDestinationComposition=false,earlyTravelPublication=false;
  const auto compose=[&](const XeenWorld &world,const XeenCamera &camera) {
   composedWorld=&world;composedCamera=camera;return XeenEventFlow::Composition{image,false};
  };
  XeenEventSystem events([&](auto id){return XeenEventScript(live.resources.loadEvents(id));},live.resources.loadRegionalText);
  XeenEventFlow flow(programState,live.world,events,live.party,live.camera,live.flags,display,
   [&](auto){return compose(live.world,live.camera);},{},[&]{return now;},{},nullptr,
   [&](auto,auto){return compose(live.world,live.camera);},nullptr,
   [&](auto &world,const auto &,const auto &camera,auto,auto){return compose(world,camera);});
  flow.dialogText=&dos_test::text();unsigned failures=0;
  bool destinationWord=false;
  flow.reportManual=[&](const auto &r){destinationWord=false;
   if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&r)) {
    if(p->request.source.x==10 && p->request.source.y==11 &&
       (p->request.source.line==0 || p->request.kind==XeenPresentationKind::WordInput)) {
     ++destinationSuspensions;destinationWord=p->request.kind==XeenPresentationKind::WordInput;
     wrongDestinationComposition|=!composedWorld || composedWorld==&live.world || !xeen_state::sameCamera(composedCamera,p->state.workingCamera);
     earlyTravelPublication|=!xeen_state::sameCamera(live.camera,initial.camera);
    }
   }
   if(const auto *e=std::get_if<XeenEventExecutionError>(&r)) {
    ++failures;std::cerr<<"Travel form "<<form<<": "<<e->message<<'\n';}};
  const auto present=[&](const IndexedFrame &frame){if(frame.presentation()){flow.framePresented(frame.presentation());flow.completeInputHandoff(frame.presentation());}};
  const auto send=[&](const PlayerAction &action){present(flow.frame());flow.beginCycle(++cycle);present(flow.handle(action,flow.displayedInput(),flow.frame().presentation()));};
  send(InteractionAction{});
  check(!wrongDestinationComposition,"Continue suspension composed the source instead of the destination candidate");
  if(mirror) {send(TextInputAction{"travel"});send(DialogKeyAction{13});}
  if(continued && (!refused || lateRefused)) {
   present(flow.refresh(true));
   check(composedWorld && composedWorld!=&live.world && composedCamera.mapId==destination &&
    composedCamera.x==10 && composedCamera.y==11,"Destination input refresh composed the source");
  }
  for(unsigned n=0;n<1000 && !flow.canSave();++n) {
   now+=125;flow.beginCycle(++cycle);if(const auto f=flow.updatePresentation())present(*f);
   if(destinationWord){send(TextInputAction{"continue"});send(DialogKeyAction{13});}
   else send(AcknowledgeAction{});
  }
  check(failures==unsigned(refused) && flow.canSave(),"Travel did not settle through the normal Journey publication boundary");
  check(!wrongDestinationComposition && !earlyTravelPublication,"Destination suspension composition or deferred publication changed");
  if(refused) {
   auto expected=initial;if(publishedPrefix)++expected.food;
   // Part 0: an invalid destination rejected before teleport clearing leaves Space as Wait.
   if(!lateRefused && !mirror){expected.journey->context->minutes+=1;expected.journey->context->ctr24=(expected.journey->context->ctr24+1)%24;}
   check(destinationSuspensions==(lateRefused?2u:0u),"Late refusal must follow destination message and input");
   check(XeenSaveFormat::encode(live.capture())==XeenSaveFormat::encode(expected),
    "Refused travel leaked a candidate or lost/replayed its published food prefix");
   const auto bytes=XeenSaveFormat::encode(live.capture());CurrentDestination restored;configure(restored);
   restored.restore(XeenSaveFormat::decode(bytes));restored.bind();
   check(XeenSaveFormat::encode(restored.capture())==bytes,"Refused travel v13 round trip replayed an Event");continue;
  }
  check(live.camera.mapId==destination && live.camera.x==10 && live.camera.y==11 &&
   live.camera.direction==(form>=10?XeenDirection::South:XeenDirection::West),"Travel camera/facing");
  check(destinationSuspensions==(continued?2u:0u),"Continue must display both destination message and input; Exit must display neither");
  check(live.party.questItems.at(17)==initial.questItems.at(17)+unsigned(continued),
   "Continue must execute the destination counter exactly once; Exit must never execute it");
  if(same)check(live.world.sessionState().monsters(28)[0].hp==3,"Same-map teleport reloaded monster HP");
  const auto snapshot=live.capture();const auto bytes=XeenSaveFormat::encode(snapshot);
  auto expectedClock=*initial.journey->context;
  // Part 0: teleport 0x27112 and map-load 0xf590 clear the current key.
  check(snapshot.journey->random==initial.journey->random && snapshot.journey->context==expectedClock &&
   snapshot.journey->purse==initial.journey->purse,"Travel changed time, rewards or RNG");
  CurrentDestination restored;configure(restored);restored.restore(XeenSaveFormat::decode(bytes));restored.bind();
  check(XeenSaveFormat::encode(restored.capture())==bytes,"Travel v13 restore replayed Event, reward, time or draw");
 }
}

// M70 Part B: the value operations publish through the city's or the
// mainland's detached candidate, then round-trip exactly through
// v13 without replaying any Event, reward, time or draw.
void valueRoundTrips(){
    mmodern::XeenProgramState programState;
 for(unsigned operation=0;operation<8;++operation) {
  const XeenMapIdentity active=operation==5 || operation==7?23:28;
  auto initial=currentSnapshot();initial.camera={active,9,11,XeenDirection::North};
  initial.map(28).monsters=mobTable(4);
  auto city=XeenEventFile{active,"synthetic.evt",true,{}};
  const auto add=[&](unsigned line,unsigned op,std::vector<std::uint8_t> p={}) {
   auto r=record(9,11,line,op,std::move(p));r.direction=4;city.records.push_back(r);
  };
  switch(operation) {
  case 0:add(0,15,{0});add(1,12,{0,0,47,5});add(2,12,{0,0,15,20});add(3,12,{0,0,18,3});add(4,12,{0,0,37,7});add(5,18);break;
  case 1:add(0,27,{74,1});add(1,27,{76,42});add(2,12,{0,0,65,10});add(3,12,{0,0,35,5,0});add(4,18);break;
  case 2:add(0,23,{4,1,5});add(1,20,{101,15,0,106,5,0,0,0});add(2,18);add(5,20,{100,10,0,0,0,0,0});add(6,18);break;
  case 3:add(0,12,{0,0,76,1});add(1,18);break;
  case 4:add(0,12,{0,0,21,90});add(1,12,{21,90,0,0});add(2,18);break;
  // Any bound mainland record runs through the same detached candidate as the city.
  case 5:add(0,12,{0,0,11,5});add(1,10,{11,5,3});add(2,12,{0,0,11,1});add(3,18);break;
  // Skills whose effects are not implemented are still granted and saved as bytes.
  // The mainland uses the same quest byte rule: 0 - 1 = 255 and 255 + 1 = 0.
  case 7:add(0,12,{21,90});add(1,12,{0,0,21,91});add(2,18);initial.questItems[8]=0;initial.questItems[9]=255;break;
  case 6:{add(0,15,{0});unsigned line=1;for(std::uint8_t skill:{1,6,8,9,14,15,16,17})add(line++,12,{0,0,13,skill});add(line,18);break;}
  }
  const auto configure=[&](CurrentDestination &f) {
   usePerMapResources(f,4);
   f.mapProvider=[](auto id){auto m=id==XeenMapIdentity(24)?regional_test::map(id):indoorMap(id);
    if(id==XeenMapIdentity(28))m.geometry.neighbors=std::array<std::uint16_t,4>{30,29,0,0};
    if(id==XeenMapIdentity(30))m.geometry.neighbors[1]=31;
    if(id==XeenMapIdentity(29))m.geometry.neighbors[0]=31;
    if(id==XeenMapIdentity(23))m.geometry.neighbors[1]=24;return m;};
   f.resources.loadEvents=[&](auto id){return id==active?city:indoorEvents(id);};
   f.resources.loadRegionalText=[](auto id){auto t=regional_test::texts(id);if(id==XeenMapIdentity(23))t.strings.resize(44);return t;};
  };
  CurrentDestination live;configure(live);live.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(initial)));
  const auto &before=initial;
  XeenEventSystem events([&](auto id){return XeenEventScript(live.resources.loadEvents(id));},live.resources.loadRegionalText);
  const auto display=font();IndexedFrame image;image.width=320;image.height=200;image.pixels.resize(64000);
  std::uint64_t now=0,cycle=0;
  XeenEventFlow flow(programState,live.world,events,live.party,live.camera,live.flags,display,
   [&](auto){return XeenEventFlow::Composition{image,false};},{},[&]{return now;},{},nullptr,
   [&](auto,auto){return XeenEventFlow::Composition{image,false};},nullptr,
   [&](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{image,false};});
  flow.dialogText=&dos_test::text();unsigned failures=0;
  flow.reportManual=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r)){
   ++failures;std::cerr<<"Value round trip "<<operation<<": "<<e->message<<'\n';}};
  const auto present=[&](const IndexedFrame &frame){if(frame.presentation())flow.framePresented(frame.presentation());};
  present(flow.frame());flow.beginCycle(++cycle);
  present(flow.handle(InteractionAction{},flow.displayedInput(),flow.frame().presentation()));
  for(unsigned n=0;n<1000 && !flow.canSave();++n) {now+=125;flow.beginCycle(++cycle);
   if(const auto frame=flow.updatePresentation())present(*frame);
   present(flow.handle(AcknowledgeAction{},flow.displayedInput(),flow.frame().presentation()));}
  check(!failures && flow.canSave(),"Value Event failed publication or quiet capture");
  const auto snapshot=live.capture();const auto bytes=XeenSaveFormat::encode(snapshot);
  const auto &ids=snapshot.activeRosterIds;
  const auto first=ids.at(0);
  if(operation==0) for(const auto id:ids) {
   check(snapshot.characters[id].personality.permanent==std::min(255,before.characters[id].personality.permanent+5) &&
    snapshot.characters[id].awards[20]==(before.characters[id].awards[20]|1) &&
    snapshot.characters[id].conditions[3]==std::uint8_t(before.characters[id].conditions[3]+1),"Character values not published");
  }
  if(operation==1)check(snapshot.journey->context->effects[XeenPartyEffect::Levitate]==1 && snapshot.journey->context->effects[XeenPartyEffect::WalkOnWater]==1 &&
   snapshot.journey->context->day==42 && snapshot.food==std::uint16_t(before.food+10) &&
   snapshot.journey->purse->gems==before.journey->purse->gems+5,"Party values not published");
  if(operation==2)check(snapshot.journey->random->count>before.journey->random->count,"Random operations drew nothing");
  if(operation==3)check(snapshot.journey->context->day!=before.journey->context->day ||
   snapshot.journey->context->year!=before.journey->context->year,"Time advance not published");
  if(operation==4)check(snapshot.questItems==before.questItems,"Quest grant and take did not cancel");
  if(operation==5)check(snapshot.characters[first].temporaryLevel==before.characters[first].temporaryLevel+
   (before.characters[first].temporaryLevel+5<=5?5:6),("Mainland write or If3 test failed: "+
   std::to_string(before.characters[first].temporaryLevel)+" -> "+std::to_string(snapshot.characters[first].temporaryLevel)).c_str());
  if(operation==7)check(snapshot.questItems[8]==255 && snapshot.questItems[9]==0,"Mainland quest counters are not byte arithmetic");
  if(operation==6)for(const auto id:ids)for(unsigned skill:{1u,6u,8u,9u,14u,15u,16u,17u})
   check(snapshot.characters[id].skills[skill]==std::uint8_t(before.characters[id].skills[skill]+1),"Skill grant not published");
  CurrentDestination loaded;configure(loaded);loaded.restore(XeenSaveFormat::decode(bytes));loaded.bind();
  check(XeenSaveFormat::encode(loaded.capture())==bytes,"Value Event did not round-trip exactly through v13");
 }
}
// Decision 6 and the remaining producer-less cell states are refused; MOB/EVT
// override identities must bind to the current resource before publication.
void unsupportedMapState(){
 const auto good=currentSnapshot();
 for(unsigned mode=0;mode<6;++mode){
  auto s=good;const char *message="";
  switch(mode){
  case 0:s.map({XeenSide::Darkside,23}).explored.emplace();message="Darkside";break;
  case 1:{s.camera.mapId.side=XeenSide::Darkside;const auto table=s.map(23).monsters;s.map({XeenSide::Darkside,23}).monsters=table;message="Darkside";break;}
  case 2:s.map(28).wallItems={{3,1,1}};message="wall-item identity does not exist";break;
  case 3:s.map(37).events={{4,0x2a}};message="Event identity does not exist";break;
  case 4:s.map(28).cells={{17,0,0x40,0,0}};message="cell flag change is not supported yet";break;
  case 5:s.map(23).cells={{17,0x0901,0x80,2,0}};message="original resource mismatch";break; // Wrong outdoor preimage.
  }
  XeenSaveFormat::encode(s); // The format represents every one of them.
  CurrentDestination f;usePerMapResources(f,4);rejects([&]{f.restore(s);},message);
  check(!f.world.hasEncounterState(),"Rejected map state was published");
 }
}
// Decision 2 (review finding 2): the Warzone, Clouds 85, keeps state only for
// a visit. Leaving it drops its state and accounting, re-entry is fresh, a save
// never contains it and a save that does is refused before publication.
void warzoneLifetime(){
 const auto stats=[]{auto r=regional_test::statistics();r[0]=r[8];return r;};
 CurrentDestination f;usePerMapResources(f,4);f.restore(currentSnapshot());
 XeenMonsterTestAccess::removeObject(f.world,{85,0});
 f.bind();
 check(f.world.sessionState().isObjectDisabled({85,0}),"Warzone state lasts during the visit");
 const auto saved=f.capture();
 check(!saved.find(85) && saved.find(23),"A save never contains the Warzone");
 auto candidate=f.world.transitionCandidate();
 candidate->stageIndoorMonsters(candidate->objectFile(85),stats());
 candidate->disableObject({85,1});candidate->leaveMap(28);candidate->leaveMap(85);
 check(!candidate->sessionState().mapState(85) && candidate->sessionState().isObjectDisabled({85,0})==false,
  "Leaving the Warzone drops its state");
 candidate->stageIndoorMonsters(candidate->objectFile(85),stats());
 check(candidate->objectFile(85).entities.monsters.size()==4 && candidate->sessionState().monsters(85).empty() &&
  candidate->sessionState().mapState(85)->originalMonsterCount==0 && !candidate->sessionState().isObjectDisabled({85,1}),
  "Re-entering the Warzone clears the monster table after loading its resources");
 auto forged=currentSnapshot();forged.map(85).explored.emplace();XeenSaveFormat::encode(forged);
 CurrentDestination refused;usePerMapResources(refused,4);
 rejects([&]{refused.restore(forged);},"Warzone");
 check(!refused.world.hasEncounterState(),"A refused Warzone save published nothing");
}
// Risk "size and time": primary maps hold actors, objects and Events;
// neighboring physical tiles hold exploration. Every retained identity 1..128
// has state, except Warzone 85. Restore must stay under one second.
void allCloudsMaps(){
 constexpr unsigned kMonsters=20;
 auto s=currentSnapshot();
 for(unsigned number=1;number<=128;++number) {
  if(!xeenRetainsMapState(static_cast<std::uint16_t>(number)))continue;
  auto &m=s.map(static_cast<std::uint16_t>(number));
  if(number<100) {
   if(number!=23)m.monsters=mobTable(kMonsters);
   m.objects={{0,-128,-128,false}};m.events={{0,0}};
  }
  m.explored=std::array<std::uint8_t,32>{};(*m.explored)[number%32]=1;
 }
 const auto bytes=XeenSaveFormat::encode(s);
 CurrentDestination f;usePerMapResources(f,kMonsters);
 const auto snapshot=XeenSaveFormat::decode(bytes);
 const auto start=std::chrono::steady_clock::now();
 f.restore(snapshot);
 const auto elapsed=std::chrono::steady_clock::now()-start;
 f.bind();
 check(f.maps>=127 && f.world.sessionState().mapStates().size()==127 && !f.world.sessionState().mapState(85),
  "Restore loads every retained map and no Warzone state");
 check(XeenSaveFormat::encode(f.capture())==bytes,"All-maps restore and capture are exact");
 const auto milliseconds=std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
 std::cout<<"All 127 retained Clouds maps restored in "<<milliseconds<<" ms ("<<bytes.size()<<" bytes)\n";
#ifdef __OPTIMIZE__
 // Timing is a property of the optimized game build; -O0 builds only report it.
 check(elapsed<std::chrono::seconds(1),"Restoring every Clouds map takes a second or more");
#endif
}
void explicitItems(){
 auto empty=currentSnapshot();for(auto &c:empty.characters){c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};}
 CurrentDestination f;distinctiveInitialItems(f.party.roster);
 std::vector<std::uint8_t> displayRoster(30*354),displayParty(812);
 displayRoster[311]=17;displayRoster[37]=23;displayRoster[56]=1;displayParty[618]=36;
 unsigned initialReads=0;f.resources.loadInitialParty=[&]{++initialReads;return XeenPartyLoader().loadFromResources(displayRoster,displayParty);};
 f.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(empty)),[&](auto &,const auto &p,const auto &,const auto &){
  for(unsigned i=0;i<30;++i)remove_test::checkSameCharacter(p.roster.at(i),empty.characters[i]);});
 f.bind();check(initialReads==1,"display data not reloaded exactly once");
 check(f.party.food==empty.food && f.party.roster.at(0).originalDetails() &&
  f.party.roster.at(0).originalDetails()->resistances[0][0]==17 &&
  f.party.roster.at(0).originalDetails()->birthDay==23 && f.party.roster.at(0).skills[17]==empty.characters[0].skills[17],
  "restored display fields do not come from original resources");sameSnapshot(f.capture(),empty);
}
void invalidResourceAndState(){
 for(unsigned mode=0;mode<28;++mode){
  CurrentDestination f;auto s=currentSnapshot();f.world.disableObject({23,0});f.party.questItems.increment(5);f.party.questFlags.set(7);f.flags.set(255);
  const auto before=remove_test::partySnapshot(f.party);const auto camera=f.camera;const auto flags=f.flags.values();
  const auto *map=&f.world.map(23);f.world.objectFile(23);const auto maps=f.world.cachedMapCount(),objects=f.world.cachedObjectFileCount();
  XeenSaveState::Preflight preflight;
  switch(mode){
   case 0:s.resources.clouds.size++;break;case 1:s.resources.clouds.crc32^=1;break;case 2:s.resources.darkside.reset();break;
   case 3:s.resources.darkside->size++;break;case 4:s.resources.darkside->crc32^=1;break;
   case 5:f.resources.loadMonsterStatistics={};break;
   case 6:f.resources.loadMonsterStatistics=[]{return std::vector<XeenMonsterRecord>{};};break;
   case 7:f.mapProvider=[](auto)->XeenMap{throw std::runtime_error("missing map");};break;
   case 8:f.mapProvider=[](auto id){auto m=regional_test::map(id);m.geometry.id=22;return m;};break;
   case 9:f.mapProvider=[](auto id){auto m=regional_test::map(id);m.geometry.flags2=0;return m;};break;
   case 10:f.objectProvider=[](auto id){auto m=regional_test::objects(id);m.resourcePresent=false;return m;};break;
   case 11:f.objectProvider=[](auto id){auto m=regional_test::objects(id);m.entities.monsters.pop_back();return m;};break;
   case 12:f.resources.loadEvents={};break;
   case 13:f.resources.loadEvents=[](auto id){auto e=regional_test::events(id);e.mapId=22;return e;};break;
   case 14:save_test::setRemovedObjects(s,{{23,1}});break;case 15:save_test::setDisabledEvents(s,{{23,170}});break;
   case 16:s.activeRosterIds={30};break;case 17:s.activeRosterIds={24};break;
   case 18:s.characters[18].race=static_cast<XeenRace>(5);break;case 19:s.characters[18].characterClass=static_cast<XeenCharacterClass>(10);break;
   case 20:s.characters[29].rosterId=0;break;case 21:s.camera.mapId.side=XeenSide::Darkside;break;case 22:s.camera.x=-1;break;
   case 23:preflight=[](auto &,const auto &,const auto &,const auto &){throw std::runtime_error("late presentation failure");};break;
   case 24:s.characters[0].permanentLevel=std::numeric_limits<int>::max();break;
   case 25:s.characters[0].intellect.permanent=-1;break;
   case 26:s.journey->schema=8;break;case 27:s.journey.reset();break;
  }
  rejects([&]{f.restore(s,preflight);});
  check(remove_test::partySnapshot(f.party)==before && sameCamera(f.camera,camera) && f.flags.values()==flags,"failed current restore published owners");
  check(&f.world.map(23)==map && f.world.cachedMapCount()==maps && f.world.cachedObjectFileCount()==objects && f.world.isObjectDisabled({23,0}),"failed current restore changed destination cache/overlay");
  check(!f.world.hasEncounterState() && !f.party.encounterContext && !f.party.roster.combatMarked(),"failed current restore published encounter");
  if(mode<5 || mode>=26)check(f.monsters==0 && f.events==0 && f.preflights==0,"incompatible save invoked providers");
 }
 CurrentDestination f;auto s=currentSnapshot();rejects([&]{XeenSaveState::restoreBeforeGameplay(s,f.resources,f.party,f.camera,f.flags,f.world,{});},"providers");
 f.world.disableObject({23,0});
 rejects([&]{f.world.restoreSessionState({{23,0}},{{23,170}},regional_test::events);});
 check(f.world.isObjectDisabled({23,0}) && f.world.sessionState().disabledEventCount()==0,"owner overlay validation partially published");
 rejects([&]{f.world.restoreSessionState({{23,0},{23,0}},{},{});});
 check(f.world.isObjectDisabled({23,0}),"duplicate overlay changed state");
}

} // namespace

int main() {
	try {
        const auto run=[](const char *name,auto function) {try {function();}catch(const std::exception &e){throw std::runtime_error(std::string(name)+": "+e.what());}};
		run("liveStatusRoundTrip",liveStatusRoundTrip); run("fullRestoration",fullRestoration); run("explicitItems",explicitItems); run("invalidResourceAndState",invalidResourceAndState); run("characterPreflight",characterPreflight); run("mutationPolicies",mutationPolicies); run("journeyBorrowRefusal",journeyBorrowRefusal);
		run("genericRoundTrip",genericRoundTrip); run("replacedOpcodePublication",replacedOpcodePublication); run("mapEventRoundTrips",mapEventRoundTrips); run("removeThenMoveRoundTrip",removeThenMoveRoundTrip); run("travelRoundTrips",travelRoundTrips); run("valueRoundTrips",valueRoundTrips); run("unsupportedMapState",unsupportedMapState); run("warzoneLifetime",warzoneLifetime); run("allCloudsMaps",allCloudsMaps);
		std::cout << "Current save state: atomic preparation, resource identities, rules and owner/flow lifetimes passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n'; return 1;
	}
}
