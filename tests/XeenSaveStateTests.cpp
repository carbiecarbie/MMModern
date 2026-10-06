#include "XeenRegionalTestSupport.h"
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

	XeenSaveSnapshot capture() const {
        XeenSaveSnapshot s;s.resources=signature;s.camera=camera;s.activeRosterIds=party.party.activeRosterIds();
        s.characters=party.roster.characters();s.questItems=party.questItems.counts();s.questFlags=party.questFlags.values();s.gameFlags=flags.values();
        const auto &state=world.sessionState();s.disabledObjects.assign(state.disabledObjects().begin(),state.disabledObjects().end());
        s.disabledEvents.assign(state.disabledEvents().begin(),state.disabledEvents().end());return s;
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
		case 5: c.personality = {minimum + 1, 0}; c.conditions[4] = 255; break;
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
		value.maxStatSkills = {true, true, true, true}; value.hasSpells = true;
		for (unsigned material = 0; material < 256; ++material) {
			value.weapons[0] = {static_cast<std::uint8_t>(material), 0, 0, 1};
			XeenCharacterRules::validateForUse(value, {610});
			check(XeenCharacterRules::maxHp(value, {610}) >= 0 &&
				XeenCharacterRules::maxSp(value, {610}) >= 0, "safe preflight changed defined rules");
		}
	}
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
}



void journeyBorrowRefusal() {
	Fixture f;
	XeenEventSystem events([&](XeenMapIdentity id) { return XeenEventScript(f.loadEvents(id)); },
		[](XeenMapIdentity id) { return XeenEventTextFile{id, "synthetic.txt", true, {}}; });
	auto textFont = font();
	XeenEventFlow flow(f.world, events, f.party, f.camera, f.flags, textFont,
		[&](std::uint64_t) { return XeenEventFlow::Composition{Fixture::compose(f.world, f.party, f.camera, f.flags), false}; });
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
 void bind(){flow=std::make_unique<XeenEncounterFlow>(world,party,camera,flags,[]{return 0;},XeenJourneyRestoreTag{});
  check(flow->prepareJourneyFrame(flow->ticket(),[]{}),"restore frame preparation");
  check(flow->presentJourney(flow->ticket()),"restore frame publication");}
 XeenSaveSnapshot capture(){return XeenSaveState::capture(resources.signature,party,camera,flags,world);}
};
XeenSaveSnapshot currentSnapshot(){static const auto value=regional_test::snapshot();return value;}
void fullRestoration(){
 auto s=currentSnapshot();s.questItems[17]=3;s.questFlags[2]=true;s.gameFlags[7]=true;
 s.food=27;s.journey->context->year=611;s.journey->context->day=0;
 s.journey->context->minutes=299;s.journey->context->newDay=true;s.journey->context->rested=true;
 s.journey->context->effects[0]=3;s.journey->context->lightAndResistances[1]=9;
 for(auto &pair:s.journey->supplements) {
  pair.inputs.resistances->fireTemporary=7;pair.inputs.resistances->energyTemporary=8;
  pair.inputs.resistances->magicTemporary=9;
 }
 s.characters[29].conditions[2]=255;s.characters[29].conditions[7]=128;
 s.disabledObjects={{23,0}};s.disabledEvents={{23,0},{23,1}};
 s.characters[29].currentSp=-50;s.characters[29].miscellaneous[8]={231,12,255,7};
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
 auto copy=f.capture();copy.characters[18].currentHp=123;copy.disabledEvents.clear();sameSnapshot(f.capture(),expected);sameSnapshot(s,expected);
 for(unsigned category=0;category<6;++category){auto value=currentSnapshot();
  switch(category){case 0:value.characters[29].currentHp=-32768;break;case 1:value.questItems[34]=0xffffffffU;break;
   case 2:value.questFlags[29]=true;break;case 3:value.gameFlags[255]=true;break;case 4:value.disabledObjects={{23,0}};break;case 5:value.disabledEvents={{23,1}};break;}
  CurrentDestination next;next.restore(value);next.bind();sameSnapshot(next.capture(),value);
 }
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
  f.party.roster.at(0).originalDetails()->birthDay==23 && f.party.roster.at(0).originalDetails()->skills[17]==1,
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
   case 14:s.disabledObjects={{23,1}};break;case 15:s.disabledEvents={{23,170}};break;
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
		fullRestoration(); explicitItems(); invalidResourceAndState(); characterPreflight(); mutationPolicies(); journeyBorrowRefusal();
		std::cout << "Current save state: atomic preparation, resource identities, rules and owner/flow lifetimes passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n'; return 1;
	}
}
