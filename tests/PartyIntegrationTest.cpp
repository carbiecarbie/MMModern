#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenItemTransfer.h"
#include "XeenEquipmentTestSupport.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace mmodern;

namespace {

struct ExpectedCharacter {
	std::uint8_t rosterId;
	const char *name;
	XeenCharacterClass characterClass;
	std::int16_t currentHp;
	int maxHp;
	std::int16_t currentSp;
	int maxSp;
	const char *portrait;
};

constexpr std::array<ExpectedCharacter, 6> kExpected = {{
	{0,  "Arturius", XeenCharacterClass::Paladin,  12, 12, 2, 2, "char01.fac"},
	{18, "Tyro",     XeenCharacterClass::Knight,   16, 16, 0, 0, "char19.fac"},
	{14, "Badger",   XeenCharacterClass::Ranger,   12, 12, 2, 2, "char15.fac"},
	{11, "Zippo",    XeenCharacterClass::Robber,   10, 10, 0, 0, "char12.fac"},
	{1,  "Rebecca",  XeenCharacterClass::Cleric,    7,  7, 7, 7, "char02.fac"},
	{6,  "Seymour",  XeenCharacterClass::Sorcerer,  5,  5, 9, 9, "char07.fac"}
}};

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

void equipmentControls(XeenAssetSource &assets) {
	using Category = XeenInventoryCategory;
	using Operation = XeenEquipmentOperation;
	using Status = XeenEquipmentStatus;
	using equipment_test::sameParty;
	using equipment_test::sameItem;
	using equipment_test::items;
	unsigned operations = 0;
	const auto fresh = [&] {
		auto p = XeenPartyLoader().loadInitialCloudsParty(assets);
		check(p.party.activeRosterIds() == std::vector<std::uint8_t>({0,18,14,11,1,6}), "equipment original membership prerequisite");
		return p;
	};
	const auto step = [&](XeenPartyState &p, XeenPartyState &expected, std::size_t active,
			Category cat, unsigned slot, Operation op, Status status, unsigned frame) {
		const auto owner = expected.party.activeRosterIds().at(active);
		const auto beforeItem = items(expected.roster.at(owner),cat)[slot];
		if(status == Status::Success) items(expected.roster.at(owner),cat)[slot].frame = static_cast<std::uint8_t>(frame);
		const auto r = xeenSetEquipment(p,active,cat,slot,op);
		++operations;
		check(r.status == status && r.owner == owner && r.operation == op && r.selection &&
			r.selection->category == cat && r.selection->physicalSlot == slot && r.beforeItem &&
			sameItem(*r.beforeItem,beforeItem), "original equipment result/selection");
		sameParty(p,expected);
		if(status==Status::Success || status==Status::NoChange) {
			check(r.modeled && r.afterItem && sameItem(*r.afterItem,items(expected.roster.at(owner),cat)[slot]), "original after facts");
			const auto &before = r.modeled->before, &after = r.modeled->after;
			check(before.intellect==after.intellect && before.personality==after.personality &&
				before.endurance==after.endurance && before.maxHp==after.maxHp && before.maxSp==after.maxSp,
				"unmodeled original equipment invented a modeled benefit");
		}
		return r;
	};
	{
		auto p=fresh(); const auto &zippo=p.roster.at(11);
		check(zippo.characterClass==XeenCharacterClass::Robber && sameItem(zippo.weapons[0],{0,12,0,1}) &&
			sameItem(zippo.weapons[1],{0,12,0,0}),"original Zippo dagger prerequisites");
		auto expected=p;
		const auto r=step(p,expected,3,Category::Weapons,1,Operation::Equip,Status::Conflict,0);
		check(r.conflict && r.conflict->category==Category::Weapons && r.conflict->physicalSlot==0,"original dagger blocker");
		step(p,expected,3,Category::Weapons,0,Operation::Remove,Status::Success,0);
		step(p,expected,3,Category::Weapons,1,Operation::Equip,Status::Success,1);
	}
	{
		auto p=fresh(); check(sameItem(p.roster.at(0).armor[3],{38,10,0,9}),"original Arturius boots prerequisite");
		auto expected=p;
		step(p,expected,0,Category::Armor,3,Operation::Remove,Status::Success,0);
		step(p,expected,0,Category::Armor,3,Operation::Equip,Status::Success,9);
	}
	{
		auto p=fresh(); const auto &a=p.roster.at(11).accessories;
		check(sameItem(a[1],{42,1,0,8}) && sameItem(a[0],{38,2,0,12}),"original Zippo ring/belt prerequisites");
		unsigned count=0; for(const auto &item:a) if(item.frame==8) ++count;
		check(count==1,"original Zippo raw ring count");
		auto expected=p;
		step(p,expected,3,Category::Accessories,1,Operation::Equip,Status::NoChange,8);
		step(p,expected,3,Category::Accessories,1,Operation::Remove,Status::Success,0);
		step(p,expected,3,Category::Accessories,1,Operation::Equip,Status::Success,8);
	}
	{
		auto p=fresh(); const auto &badger=p.roster.at(14);
		check(badger.characterClass==XeenCharacterClass::Ranger && sameItem(badger.weapons[0],{0,8,0,1}) &&
			sameItem(badger.weapons[1],{0,30,0,4}),"original Badger bow/melee prerequisites");
		auto expected=p;
		step(p,expected,2,Category::Weapons,1,Operation::Remove,Status::Success,0);
		step(p,expected,2,Category::Weapons,1,Operation::Equip,Status::Success,4);
	}
	for(bool direct:{false,true}) {
		auto p=fresh(); check(sameItem(p.roster.at(1).accessories[1],{42,5,0,8}),"original Rebecca anomalous charm prerequisite");
		auto expected=p;
		if(!direct)step(p,expected,4,Category::Accessories,1,Operation::Remove,Status::Success,0);
		step(p,expected,4,Category::Accessories,1,Operation::Equip,Status::Success,7);
	}
	{
		auto p=fresh(); const auto &source=p.roster.at(11), &destination=p.roster.at(1);
		check(source.characterClass==XeenCharacterClass::Robber && destination.characterClass==XeenCharacterClass::Cleric &&
			sameItem(source.weapons[0],{0,12,0,1}) && sameItem(source.weapons[1],{0,12,0,0}) &&
			sameItem(destination.weapons[0],{0,15,0,1}),"original transfer/proficiency prerequisites");
		for(unsigned slot=2;slot<9;++slot)check(sameItem(source.weapons[slot],{}),"original dagger source tail");
		for(unsigned slot=1;slot<9;++slot)check(sameItem(destination.weapons[slot],{}),"original cleric destination tail");
		auto expected=p;
		expected.roster.at(11).weapons[1]={}; expected.roster.at(1).weapons[1]={0,12,0,0};
		const auto transfer=xeenTransferItem(p,3,4,Category::Weapons,1);
		check(transfer.status==XeenTransferStatus::Success && transfer.sourceOwner==11 && transfer.destinationOwner==1 &&
			transfer.destinationSlot==1,"original bounded M24 transfer");
		sameParty(p,expected);
		step(p,expected,4,Category::Weapons,1,Operation::Equip,Status::NotProficient,0);
	}
	std::cout << "Original equipment foundation: " << operations << " operations across 7 fresh controls; dagger, boots, ring, bow, charm and M24 proficiency contrast OK\n";
}

} // namespace

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: mmodern_party_smoke <game directory>\n";
		return 1;
	}
	try {
		const auto installation = XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasXeen(), "Clouds installation unavailable");
		XeenAssetSource assets(*installation);
		const auto rosterBytes = assets.readInitialResource("maze.chr");
		const auto partyBytes = assets.readInitialResource("maze.pty");
		check(rosterBytes.size() == 10620, "unexpected real maze.chr size");
		check(partyBytes.size() == 812, "unexpected real maze.pty size");
		const XeenPartyState state = XeenPartyLoader().loadFromResources(rosterBytes, partyBytes);
		unsigned occupied = 0, miscellaneousOccupied = 0;
		const unsigned offsets[]{166, 202, 238, 274};
		for (unsigned i = 0; i < 30; ++i) {
			const auto &c = state.roster.at(i);
			const XeenItemCategory *categories[]{&c.weapons, &c.armor, &c.accessories, &c.miscellaneous};
			for (unsigned category = 0; category < 4; ++category)
				for (unsigned slot = 0; slot < 9; ++slot) {
					const auto offset = i * 354 + offsets[category] + slot * 4;
					const auto &item = categories[category]->at(slot);
					check(item.material == rosterBytes.at(offset) && item.id == rosterBytes.at(offset + 1) &&
						item.state == rosterBytes.at(offset + 2) && item.frame == rosterBytes.at(offset + 3),
						"loaded original items differ from actual CHR resource bytes");
					occupied += item.id != 0;
					miscellaneousOccupied += category == 3 && item.id != 0;
				}
		}
		check(occupied == 35 && miscellaneousOccupied == 0, "supplied installation item population differs from recorded evidence");
		std::cout << "All 1080 original item slots match CHR bytes: " << occupied
			<< " occupied, " << miscellaneousOccupied << " miscellaneous (nonzero misc tested synthetically)\n";
		check(state.party.size() == kExpected.size(), "unexpected real active party size");
		const XeenCharacterRulesContext rulesContext{kCloudsInitialYear};
		check(rulesContext.currentYear == 610, "unexpected Clouds initial year");
		const auto placements = CloudsUiComposer::buildPortraitPlacements(state);
		const auto hpPlacements = CloudsUiComposer::buildHpPlacements(state, rulesContext);
		check(placements.size() == kExpected.size(), "unexpected real portrait count");
		check(hpPlacements.size() == kExpected.size(), "unexpected real HP indicator count");
		constexpr std::array<int, 6> kExpectedHpX = {13, 50, 86, 122, 158, 194};
		for (std::size_t i = 0; i < kExpected.size(); ++i) {
			const XeenCharacter &character = state.party.member(state.roster, i);
			const ExpectedCharacter &expected = kExpected[i];
			check(character.rosterId == expected.rosterId && character.name == expected.name &&
				character.characterClass == expected.characterClass && character.currentLevel() == 1 &&
				character.currentHp == expected.currentHp &&
				XeenCharacterRules::maxHp(character, rulesContext) == expected.maxHp &&
				character.currentSp == expected.currentSp &&
				XeenCharacterRules::maxSp(character, rulesContext) == expected.maxSp &&
				character.worstCondition() == XeenCondition::Good,
				"real initial character HP/SP differs from expected Clouds roster");
			check(placements[i].resourceName == expected.portrait && placements[i].frame == 0,
				"real initial portrait order/frame differs from expected Clouds party");
			check(hpPlacements[i].partySlot == i &&
				hpPlacements[i].rosterId == expected.rosterId &&
				hpPlacements[i].frame == 0 && hpPlacements[i].x == kExpectedHpX[i] &&
				hpPlacements[i].y == 182,
				"real initial HP indicator order/frame/position differs from expected Clouds party");
		}
		std::cout << "Real Clouds party: current/max HP/SP and portrait order OK\n";
		equipmentControls(assets);
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
