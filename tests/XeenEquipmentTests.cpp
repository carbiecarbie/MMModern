#include "XeenEquipmentTestSupport.h"
#include "games/xeen/XeenCharacterRules.h"
#include <iostream>
#include <limits>
#include <string_view>

using namespace equipment_test;
using Category = XeenInventoryCategory;
using Operation = XeenEquipmentOperation;
using Status = XeenEquipmentStatus;
namespace {
unsigned operations = 0;
XeenPartyState fixture() {
	XeenPartyState p;
	p.party = XeenParty::fromRosterIds({0, 1, 0});
	p.firstSerializedCount = 4; p.effectiveSerializedCount = 3;
	p.diagnostics = {"synthetic alias", "preserve diagnostic"};
	p.questItems.increment(17); p.questFlags.set(2);
	for (unsigned i = 0; i < 30; ++i) {
		auto &c = p.roster.at(i);
		c.rosterId = static_cast<std::uint8_t>(i);
		c.name = "Synthetic owner with allocated name " + std::to_string(i);
		c.characterClass = XeenCharacterClass::Paladin;
		c.birthYear = 592; c.permanentLevel = 1;
		c.intellect.permanent = 11; c.personality.permanent = 13; c.endurance.permanent = 19;
		c.hasSpells = true; c.currentHp = 16; c.currentSp = 6;
		if (i) {
			c.intellect.temporary = -static_cast<int>(i); c.temporaryAge = i;
			c.sex = XeenSex::YesPlease; c.maxStatSkills.astrologer = true;
			c.conditions[i % 16] = static_cast<std::uint8_t>(i);
			for (auto *array : {&c.weapons, &c.armor, &c.accessories, &c.miscellaneous})
				for (unsigned slot = 0; slot < 9; ++slot)
					(*array)[slot] = {255, static_cast<std::uint8_t>(slot % 2 ? 0 : i),
						static_cast<std::uint8_t>(slot + i), static_cast<std::uint8_t>(200 + slot)};
		}
	}
	// Unrelated active and inactive owners are deliberately unsafe for rules.
	p.roster.at(1).race = static_cast<XeenRace>(255);
	p.roster.at(29).characterClass = static_cast<XeenCharacterClass>(255);
	return p;
}

XeenEquipmentResult attempt(XeenPartyState &p, Category cat, unsigned slot,
		Operation op, Status status, unsigned target = 0, std::size_t active = 0) {
	const auto original = p;
	auto expected = original;
	const auto *ownerAddress = &p.roster.at(0);
	if (status == Status::Success)
		items(expected.roster.at(expected.party.activeRosterIds().at(active)), cat)[slot].frame = static_cast<std::uint8_t>(target);
	const auto r = xeenSetEquipment(p, active, cat, slot, op);
	++operations;
	if (r.status != status) throw std::runtime_error("unexpected equipment status at operation " + std::to_string(operations) +
		": actual " + std::to_string(static_cast<int>(r.status)) + " expected " + std::to_string(static_cast<int>(status)));
	sameParty(p, expected);
	check(ownerAddress == &p.roster.at(0), "owner address changed");
	const bool evaluated = status == Status::Success || status == Status::NoChange;
	check(r.modeled.has_value() == evaluated && r.afterItem.has_value() == evaluated, "modeled/after availability");
	check(r.conflict.has_value() == (status == Status::Conflict), "conflict availability");
	check(r.matchingFrameCount.has_value() == (status == Status::RingLimit || status == Status::MedalLimit), "count availability");
	if (r.selection) {
		check(r.owner && r.beforeItem, "selection requires owner and before item");
		auto owner = original.roster.at(*r.owner);
		check(sameItem(*r.beforeItem, items(owner, r.selection->category)[r.selection->physicalSlot]), "before item differs from original");
	}
	if (evaluated) {
		check(r.operation == op && r.owner == p.party.activeRosterIds()[active] && r.selection &&
			r.selection->category == cat && r.selection->physicalSlot == slot && r.beforeItem,
			"evaluated selection facts");
		check(sameItem(*r.afterItem, items(expected.roster.at(*r.owner), cat)[slot]), "after item facts");
		auto before = *r.afterItem;
		before.frame = r.beforeItem->frame;
		check(sameItem(before, *r.beforeItem), "before/after changed non-frame byte");
	}
	return r;
}
void values(const XeenEquipmentValues &v, int intellect, int personality, int endurance, int hp, int sp) {
	check(v.intellect == intellect && v.personality == personality && v.endurance == endurance &&
		v.maxHp == hp && v.maxSp == sp, "independent modeled-value expectation");
}

struct Subtype { Category category; unsigned id; unsigned frame; };
constexpr Subtype subtypes[]{
	{Category::Weapons,1,1}, {Category::Weapons,17,1}, {Category::Weapons,18,13},
	{Category::Weapons,29,13}, {Category::Weapons,30,4}, {Category::Weapons,33,4}, {Category::Weapons,34,13},
	{Category::Armor,1,3}, {Category::Armor,7,3}, {Category::Armor,8,2}, {Category::Armor,9,5},
	{Category::Armor,10,9}, {Category::Armor,11,10}, {Category::Armor,12,10}, {Category::Armor,13,6},
	{Category::Accessories,1,8}, {Category::Accessories,2,12}, {Category::Accessories,3,7},
	{Category::Accessories,7,7}, {Category::Accessories,8,11}, {Category::Accessories,10,11}
};

void proficiency() {
	// Independently authored forbidden class membership, not masks or bit tests.
	constexpr const char *weapons[]{
		"3468","3468","3468","3468","3468","3468","","34","2345789","2345789","2345789",
		"3","4","4","4","4","34","348","348","348","348","34568","348","","4",
		"2345789","3468","3468","3468","348","348","348","348",""
	};
	constexpr const char *armor[]{"","48","478","4678","245678","23456789","23456789","2468"};
	constexpr unsigned weaponFrames[]{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,13,13,13,13,13,13,13,13,13,13,13,13,4,4,4,4,13};
	constexpr unsigned armorFrames[]{3,3,3,3,3,3,3,2,5,9,10,10,6};
	constexpr unsigned accessoryFrames[]{8,12,7,7,7,7,7,11,11,11};
	for (unsigned c = 0; c < 10; ++c) for (Category cat : {Category::Weapons, Category::Armor, Category::Accessories}) {
		const unsigned end = cat == Category::Weapons ? 34 : cat == Category::Armor ? 13 : 10;
		for (unsigned id = 1; id <= end; ++id) {
			auto p = fixture(); auto &owner = p.roster.at(0);
			owner.characterClass = static_cast<XeenCharacterClass>(c);
			items(owner, cat)[4] = {255, static_cast<std::uint8_t>(id), 63, 0};
			const std::string_view forbidden = cat == Category::Weapons ? weapons[id-1] :
				cat == Category::Armor && id <= 8 ? armor[id-1] : "";
			const bool denied = forbidden.find(static_cast<char>('0' + c)) != std::string_view::npos;
			const unsigned frame = cat == Category::Weapons ? weaponFrames[id-1] : cat == Category::Armor ? armorFrames[id-1] : accessoryFrames[id-1];
			attempt(p, cat, 4, Operation::Equip, denied ? Status::NotProficient : Status::Success, frame);
		}
	}
}

void structural() {
	auto p = fixture();
	p.party = XeenParty::fromRosterIds({});
	auto r = attempt(p, static_cast<Category>(255), 99, static_cast<Operation>(99), Status::EmptyParty);
	check(!r.owner && !r.operation && !r.selection && !r.beforeItem, "invalid facts look available");
	p = fixture();
	attempt(p, Category::Weapons, 0, Operation::Equip, Status::InvalidParticipant, 0, std::numeric_limits<std::size_t>::max());
	p.roster.at(0).rosterId = 29;
	r = attempt(p, static_cast<Category>(255), 99, static_cast<Operation>(99), Status::InvalidOwner);
	check(!r.owner, "invalid owner exposed");
	p = fixture();
	attempt(p, static_cast<Category>(255), 99, static_cast<Operation>(99), Status::InvalidOperation);
	for (Category cat : {Category::Miscellaneous, static_cast<Category>(255)})
		attempt(p, cat, 99, Operation::Equip, Status::InvalidCategory);
	attempt(p, Category::Weapons, 9, Operation::Equip, Status::InvalidSlot);
	const auto before = p;
	check(xeenSetEquipment(p, 0, Category::Weapons, std::numeric_limits<std::size_t>::max(), Operation::Remove).status == Status::InvalidSlot, "large physical slot");
	sameParty(p, before);
	p.roster.at(0).weapons[0] = {105,0,255,13};
	attempt(p, Category::Weapons, 0, Operation::Equip, Status::EmptySource);
	attempt(p, Category::Weapons, 0, Operation::Remove, Status::EmptySource);
	for (const auto &ids : {std::vector<std::uint8_t>{30}, std::vector<std::uint8_t>{255}, std::vector<std::uint8_t>{0,0,0,0,0,0,0}}) {
		bool rejected = false;
		try { static_cast<void>(XeenParty::fromRosterIds(ids)); } catch (const std::invalid_argument &) { rejected = true; }
		check(rejected, "factory accepted impossible membership");
	}
}

void framesAndConflicts() {
	for (const auto &s : subtypes) for (unsigned slot : {0U,4U,8U}) {
		auto p = fixture();
		items(p.roster.at(0), s.category)[slot] = {131, static_cast<std::uint8_t>(s.id), 7, 255};
		attempt(p, s.category, slot, Operation::Equip, Status::Success, s.frame, 2);
		check(&p.party.member(p.roster,0) == &p.party.member(p.roster,2), "alias ownership");
		attempt(p, s.category, slot, Operation::Remove, Status::Success, 0);
		attempt(p, s.category, slot, Operation::Remove, Status::NoChange);
	}
	for (const auto &s : subtypes) {
		if (s.frame == 7 || s.frame == 8) continue;
		for (unsigned blocker = 0; blocker < 9; ++blocker) for (unsigned state : {0U,64U,128U,192U}) for (unsigned id : {0U,255U}) {
			auto p = fixture(); auto &a = items(p.roster.at(0), s.category);
			a[blocker] = {255, static_cast<std::uint8_t>(id), static_cast<std::uint8_t>(state), static_cast<std::uint8_t>(s.frame)};
			const unsigned selected = (blocker+1)%9;
			a[selected] = {0, static_cast<std::uint8_t>(s.id),0,0};
			const auto r = attempt(p,s.category,selected,Operation::Equip,Status::Conflict);
			check(r.conflict->category == s.category && r.conflict->physicalSlot == blocker, "first raw conflict");
		}
		auto p = fixture(); items(p.roster.at(0),s.category)[4] = {0,static_cast<std::uint8_t>(s.id),0,static_cast<std::uint8_t>(s.frame)};
		attempt(p,s.category,4,Operation::Equip,Status::Conflict); // Self participates.
	}
	// All positions, both melee frames, single ascending OR scan and same-before-cross.
	for (unsigned first = 0; first < 8; ++first) for (unsigned frame : {1U,13U}) {
		auto p = fixture(); auto &c = p.roster.at(0);
		c.weapons[first] = {0,0,0,static_cast<std::uint8_t>(frame)};
		c.weapons[first+1] = {0,255,192,static_cast<std::uint8_t>(frame == 1 ? 13 : 1)};
		c.weapons[8].id = 34; c.armor[0] = {0,0,0,2};
		const auto r = attempt(p,Category::Weapons,8,Operation::Equip,Status::Conflict);
		check(r.conflict->category == Category::Weapons && r.conflict->physicalSlot == first, "melee scan precedence");
	}
	for (unsigned blocker = 0; blocker < 9; ++blocker) for (bool shield : {false,true}) {
		auto p = fixture(); auto &c = p.roster.at(0);
		const Category selected = shield ? Category::Armor : Category::Weapons;
		const Category other = shield ? Category::Weapons : Category::Armor;
		items(c,selected)[4] = {0,static_cast<std::uint8_t>(shield?8:34),0,0};
		items(c,other)[blocker] = {255,0,192,static_cast<std::uint8_t>(shield?13:2)};
		auto r = attempt(p,selected,4,Operation::Equip,Status::Conflict);
		check(r.conflict->category == other && r.conflict->physicalSlot == blocker,"cross-category blocker");
		items(c,selected)[8].frame = shield ? 2 : 1;
		r = attempt(p,selected,4,Operation::Equip,Status::Conflict);
		check(r.conflict->category == selected && r.conflict->physicalSlot == 8,"same before cross");
	}
	// No global normalization; exact unrelated frames, body armor and legal coexistence.
	for (unsigned frame = 0; frame <= 255; ++frame) {
		auto p = fixture(); auto &c=p.roster.at(0);
		c.weapons[4] = {0,1,0,0}; c.weapons[0] = {255,0,192,static_cast<std::uint8_t>(frame)};
		c.armor[0] = {0,8,0,2}; c.armor[8] = {0,255,0,3};
		attempt(p,Category::Weapons,4,Operation::Equip,frame==1||frame==13?Status::Conflict:Status::Success,1);
	}
	auto p=fixture(); auto &c=p.roster.at(0);
	c.weapons[0]={0,1,0,1}; c.weapons[8]={0,30,0,0}; c.armor[0]={0,8,0,2};
	attempt(p,Category::Weapons,8,Operation::Equip,Status::Success,4);
	c.armor[0].frame=0;
	attempt(p,Category::Armor,0,Operation::Equip,Status::Success,2);
	c.characterClass=XeenCharacterClass::Cleric; c.weapons[8]={0,12,0,0};
	attempt(p,Category::Weapons,8,Operation::Equip,Status::NotProficient);
}

void capacityAndUnmaskedPrecedence() {
	for (unsigned frame : {7U,8U}) for (unsigned count=0;count<=9;++count) {
		auto p=fixture(); auto &a=p.roster.at(0).accessories;
		for (unsigned i=0;i<count;++i) a[i]={255,static_cast<std::uint8_t>(i%2?255:0),192,static_cast<std::uint8_t>(frame)};
		a[8].id=frame==8?1:5;
		const auto r=attempt(p,Category::Accessories,8,Operation::Equip,
			count>=2?(frame==8?Status::RingLimit:Status::MedalLimit):Status::Success,frame);
		if(count>=2) check(r.matchingFrameCount==count,"actual raw capacity count");
	}
	for(unsigned frame:{7U,8U}) {
		auto p=fixture(); auto &c=p.roster.at(0);
		c.accessories[4]={0,static_cast<std::uint8_t>(frame==8?1:5),0,static_cast<std::uint8_t>(frame)};
		attempt(p,Category::Accessories,4,Operation::Equip,Status::NoChange);
		c.accessories[0]={255,0,192,static_cast<std::uint8_t>(frame)};
		attempt(p,Category::Accessories,4,Operation::Equip,frame==8?Status::RingLimit:Status::MedalLimit);
	}
	for(const auto &s:subtypes) {
		if(s.category==Category::Weapons || (s.category==Category::Armor && s.id<=8)) continue;
		auto p=fixture(); auto &c=p.roster.at(0);
		c.characterClass=static_cast<XeenCharacterClass>(255);
		auto &a=items(c,s.category); a[8]={0,static_cast<std::uint8_t>(s.id),0,0};
		a[0].frame=static_cast<std::uint8_t>(s.frame); a[1].frame=static_cast<std::uint8_t>(s.frame);
		attempt(p,s.category,8,Operation::Equip,s.frame==8?Status::RingLimit:s.frame==7?Status::MedalLimit:Status::Conflict);
		a[0].frame=a[1].frame=0;
		attempt(p,s.category,8,Operation::Equip,Status::UnsafeRules);
	}
}

void removeAndEligibility() {
	for(Category cat:{Category::Weapons,Category::Armor,Category::Accessories}) for(unsigned id=1;id<=255;++id) {
		auto p=fixture(); auto &item=items(p.roster.at(0),cat)[4];
		item={255,static_cast<std::uint8_t>(id),191,255};
		attempt(p,cat,4,Operation::Remove,Status::Success,0);
		attempt(p,cat,4,Operation::Remove,Status::NoChange);
		for(unsigned frame:{0U,255U}) {
			item.state=255; item.frame=static_cast<std::uint8_t>(frame);
			attempt(p,cat,4,Operation::Remove,Status::Cursed);
		}
		const unsigned max=cat==Category::Weapons?34:cat==Category::Armor?13:10;
		if(id>max) attempt(p,cat,4,Operation::Equip,Status::UnsupportedItem);
	}
	for(const auto &s:subtypes) for(unsigned state:{64U,128U,192U}) {
		auto p=fixture(); items(p.roster.at(0),s.category)[4]={105,static_cast<std::uint8_t>(s.id),static_cast<std::uint8_t>(state),0};
		const auto r=attempt(p,s.category,4,Operation::Equip,Status::Success,s.frame);
		values(r.modeled->before,11,13,19,12,2); values(r.modeled->after,11,13,19,12,2);
	}
	for(unsigned condition=0;condition<=16;++condition) for(int current:{-32768,-12,0,32767}) for(unsigned race=0;race<5;++race) {
		auto p=fixture(); auto &c=p.roster.at(0);
		if(condition<16)c.conditions[condition]=255;
		c.currentHp=static_cast<std::int16_t>(current); c.currentSp=static_cast<std::int16_t>(current);
		c.race=static_cast<XeenRace>(race); c.sex=static_cast<XeenSex>(condition%3);
		c.armor[4]={0,10,0,0};
		attempt(p,Category::Armor,4,Operation::Equip,Status::Success,9);
		attempt(p,Category::Armor,4,Operation::Remove,Status::Success,0);
	}
}

void modeledConsequences() {
	// All produced frames, all modeled material ranges and neighboring boundaries.
	struct Material { unsigned id; int intellect,personality,hp,sp; };
	constexpr Material materials[]{
		{68,11,13,12,2},{69,13,13,12,2},{70,14,13,12,2},{71,16,13,12,2},{72,19,13,12,2},
		{73,23,13,12,2},{74,28,13,12,2},{75,34,13,12,2},{76,41,13,12,2},
		{77,11,15,12,2},{78,11,16,12,2},{79,11,18,12,3},{80,11,21,12,4},
		{81,11,25,12,4},{82,11,30,12,5},{83,11,36,12,5},{84,11,43,12,6},{85,11,13,12,2},
		{104,11,13,12,2},{105,11,13,16,2},{106,11,13,18,2},{107,11,13,22,2},{108,11,13,32,2},{109,11,13,62,2},
		{110,11,13,12,6},{111,11,13,12,10},{112,11,13,12,14},{113,11,13,12,18},{114,11,13,12,22},{115,11,13,12,27},{116,11,13,12,2}
	};
	for(const auto &s:subtypes) for(const auto &m:materials) {
		auto p=fixture(); auto &c=p.roster.at(0);
		items(c,s.category)[4]={static_cast<std::uint8_t>(m.id),static_cast<std::uint8_t>(s.id),63,0};
		auto r=attempt(p,s.category,4,Operation::Equip,Status::Success,s.frame);
		values(r.modeled->before,11,13,19,12,2); values(r.modeled->after,m.intellect,m.personality,19,m.hp,m.sp);
		r=attempt(p,s.category,4,Operation::Remove,Status::Success,0);
		values(r.modeled->before,m.intellect,m.personality,19,m.hp,m.sp); values(r.modeled->after,11,13,19,12,2);
	}
	for(bool intellect:{false,true}) {
		auto p=fixture(); auto &c=p.roster.at(0);
		c.characterClass=intellect?XeenCharacterClass::Sorcerer:XeenCharacterClass::Cleric;
		c.personality.permanent=11; c.armor[4]={static_cast<std::uint8_t>(intellect?69:77),10,0,0};
		auto r=attempt(p,Category::Armor,4,Operation::Equip,Status::Success,9);
		values(r.modeled->before,11,11,19,intellect?8:9,3);
		values(r.modeled->after,intellect?13:11,intellect?11:13,19,intellect?8:9,4);
	}
	for(bool spells:{false,true}) for(unsigned state:{0U,64U,128U,192U}) for(int current:{-32768,-12,0,16,32767}) {
		auto p=fixture(); auto &c=p.roster.at(0); c.hasSpells=spells;
		c.currentHp=static_cast<std::int16_t>(current); c.currentSp=static_cast<std::int16_t>(current);
		c.weapons[4]={110,12,static_cast<std::uint8_t>(state),0};
		const auto r=attempt(p,Category::Weapons,4,Operation::Equip,Status::Success,1);
		values(r.modeled->after,11,13,19,12,spells?(state?2:6):0);
		attempt(p,Category::Weapons,4,Operation::Remove,state&64?Status::Cursed:Status::Success,0);
	}
	auto p=fixture(); auto &c=p.roster.at(0);
	c.weapons[8]={69,0,0,255}; c.miscellaneous.fill({105,255,0,1});
	c.armor[4]={105,10,0,0};
	auto r=attempt(p,Category::Armor,4,Operation::Equip,Status::Success,9);
	values(r.modeled->before,13,13,19,12,2); values(r.modeled->after,13,13,19,16,2);
	r=attempt(p,Category::Armor,4,Operation::Remove,Status::Success,0);
	values(r.modeled->after,13,13,19,12,2);
}

void numericalSafety() {
	const int hi=std::numeric_limits<int>::max(), lo=std::numeric_limits<int>::min();
	for(unsigned scenario=0;scenario<9;++scenario) {
		auto p=fixture(); auto &c=p.roster.at(0); c.armor[4]={69,10,0,0};
		switch(scenario) {
		case 0: c.race=static_cast<XeenRace>(255); break;
		case 1: c.characterClass=static_cast<XeenCharacterClass>(255); break;
		case 2: c.permanentLevel=hi; break; // HP multiplication
		case 3: c.permanentLevel=hi; c.temporaryLevel=1; break;
		case 4: c.intellect.permanent=hi; c.intellect.temporary=1; break;
		case 5: c.temporaryAge=hi; break;
		case 6: c.endurance.permanent=lo; c.endurance.temporary=-1; break;
		case 7: c.characterClass=XeenCharacterClass::Druid; c.permanentLevel=60000000;
			c.endurance.permanent=0; c.intellect.permanent=c.personality.permanent=250; break; // SP sum
		case 8: c.characterClass=XeenCharacterClass::Sorcerer; c.permanentLevel=100000000;
			c.endurance.permanent=0; c.intellect.permanent=250; break; // SP multiplication
		}
		attempt(p,Category::Armor,4,Operation::Equip,Status::UnsafeRules);
		attempt(p,Category::Armor,4,Operation::Remove,Status::UnsafeRules); // zero-frame NoChange preflight
		c.armor[4].state=64;
		attempt(p,Category::Armor,4,Operation::Remove,Status::Cursed);
	}
	// Safe original -> unsafe candidate; and unsafe original -> safe candidate.
	for(bool repair:{false,true}) {
		auto p=fixture(); auto &c=p.roster.at(0); c.intellect.permanent=hi-1;
		c.armor[4]={69,10,0,static_cast<std::uint8_t>(repair?9:0)};
		if(!repair) XeenCharacterRules::validateForUse(c,{610});
		else { auto safe=c; safe.armor[4].frame=0; XeenCharacterRules::validateForUse(safe,{610}); }
		attempt(p,Category::Armor,4,repair?Operation::Remove:Operation::Equip,Status::UnsafeRules);
	}
	// Bonus is added before negative condition arithmetic, which precedes clamp.
	for(unsigned material:{69U,77U}) {
		auto p=fixture(); auto &c=p.roster.at(0);
		(material==69?c.intellect:c.personality).permanent=lo;
		c.conditions[static_cast<unsigned>(XeenCondition::Weak)]=1;
		c.armor[4]={static_cast<std::uint8_t>(material),10,0,9};
		XeenCharacterRules::validateForUse(c,{610});
		attempt(p,Category::Armor,4,Operation::Remove,Status::UnsafeRules);
	}
	// Safe product, overflowing direct HP addition only after Equip.
	auto p=fixture(); auto &c=p.roster.at(0); c.permanentLevel=178956970;
	c.armor[4]={109,10,0,0}; XeenCharacterRules::validateForUse(c,{610});
	attempt(p,Category::Armor,4,Operation::Equip,Status::UnsafeRules);
	p=fixture(); p.roster.at(0).characterClass=static_cast<XeenCharacterClass>(255);
	p.roster.at(0).weapons[4]={0,7,0,1};
	attempt(p,Category::Weapons,4,Operation::Equip,Status::UnsafeRules); // mask zero still proficiency branch
	p=fixture(); p.roster.at(0).race=static_cast<XeenRace>(255);
	p.roster.at(0).accessories[4]={0,1,0,8};
	attempt(p,Category::Accessories,4,Operation::Equip,Status::UnsafeRules); // direct NoChange preflight
	// Safe HP and SP products; direct SP addition exceeds INT_MAX only in candidate.
	p=fixture(); auto &sp=p.roster.at(0);
	sp.characterClass=XeenCharacterClass::Sorcerer; sp.permanentLevel=93368854;
	sp.endurance.permanent=0; sp.intellect.permanent=250;
	sp.armor[4]={115,10,0,0}; XeenCharacterRules::validateForUse(sp,{610});
	attempt(p,Category::Armor,4,Operation::Equip,Status::UnsafeRules);
	// An INT bonus alone can raise the SP multiplier beyond its safe range.
	p=fixture(); auto &indirect=p.roster.at(0);
	indirect.characterClass=XeenCharacterClass::Sorcerer; indirect.permanentLevel=550000000;
	indirect.endurance.permanent=0; indirect.armor[4]={69,10,0,0};
	XeenCharacterRules::validateForUse(indirect,{610});
	attempt(p,Category::Armor,4,Operation::Equip,Status::UnsafeRules);
}
} // namespace
int main() {
	try {
		proficiency(); structural(); framesAndConflicts(); capacityAndUnmaskedPrecedence();
		removeAndEligibility(); modeledConsequences(); numericalSafety();
		std::cout << "Xeen equipment: " << operations << " checked operations; full party preservation and independent domain/numeric expectations OK\n";
		return 0;
	} catch(const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
