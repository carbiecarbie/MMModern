// Test-only canonical field dump. Keep this visitor in step with XeenSaveSnapshot
// and its nested persisted values when extending the save reader. Field paths
// are the cross-build audit contract; do not rename them for cosmetic reasons.
#include "XeenSaveTestSupport.h"
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <type_traits>

using namespace mmodern;
namespace {
struct Dump {
    std::map<std::string, std::string> fields;
    template<class T> void value(const std::string &p, const T &v) {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>, "Missing dump visitor");
        if constexpr (std::is_enum_v<T>) value(p, static_cast<std::underlying_type_t<T>>(v));
        else fields[p] = std::to_string(v);
    }
    void value(const std::string &p, const std::string &v) {
        // Byte hex preserves DOS text without locale/JSON escaping ambiguity.
        std::string hex = "hex:";
        for (unsigned char c : v) { hex += "0123456789abcdef"[c >> 4]; hex += "0123456789abcdef"[c & 15]; }
        fields[p] = hex;
    }
    void value(const std::string &p, const XeenMutableString &v) { value(p, std::string(v)); }
    template<class T> void value(const std::string &p, const XeenMutable<T> &v) { value(p, static_cast<T>(v)); }
    template<class T> void optional(const std::string &p, const T &v) {
        value(p + ".present", bool(v)); if (v) value(p + ".value", *v);
    }
    template<class T> void value(const std::string &p, const std::optional<T> &v) { optional(p, v); }
    template<class T> void value(const std::string &p, const XeenMutableOptional<T> &v) { optional(p, v); }
    template<class T> void sequence(const std::string &p, const T &v) {
        value(p + ".size", v.size());
        for (std::size_t i = 0; i < v.size(); ++i) value(p + "[" + std::to_string(i) + "]", v[i]);
    }
    template<class T, std::size_t N> void value(const std::string &p, const std::array<T,N> &v) { sequence(p, v); }
    template<class T, std::size_t N> void value(const std::string &p, const XeenMutableArray<T,N> &v) { sequence(p, v); }
    template<class T> void value(const std::string &p, const std::vector<T> &v) { sequence(p, v); }
#define F(member) value(p + "." #member, v.member)
#define V(type, body) void value(const std::string &p, const type &v) { body }
    V(XeenArchiveFingerprint, F(size); F(crc32);)
    V(XeenSaveResourceSignature, F(clouds); F(darkside);)
    V(XeenMapIdentity, F(side); F(number);)
    V(XeenCamera, F(mapId); F(x); F(y); F(direction);)
    V(XeenMonsterIdentity, F(mapId); F(recordIndex);)
    V(XeenObjectIdentity, F(mapId); F(recordIndex);)
    V(XeenEventIdentity, F(mapId); F(recordIndex);)
    V(XeenAttributeValue, F(permanent); F(temporary);)
    V(XeenItem, F(material); F(id); F(state); F(frame);)
    V(XeenCharacter,
        F(rosterId); F(name); F(sex); F(race); F(characterClass);
        F(intellect); F(personality); F(endurance); F(permanentLevel); F(temporaryLevel);
        F(temporaryAge); F(skills);
        value(p+".maxStatSkills.astrologer",bool(v.skills[2]));
        value(p+".maxStatSkills.bodybuilder",bool(v.skills[3]));
        value(p+".maxStatSkills.prayerMaster",bool(v.skills[12]));
        value(p+".maxStatSkills.prestidigitation",bool(v.skills[13])); F(currentSpell); F(quickOption); F(hasSpells);
        F(learnedSpells); F(weapons); F(armor); F(accessories); F(miscellaneous);
        F(currentHp); F(currentSp); F(conditions); F(birthYear); F(awards);)
    V(XeenCombatResistances,
        F(coldPermanent); F(coldTemporary); F(electricalPermanent); F(electricalTemporary);
        F(firePermanent); F(fireTemporary); F(energyPermanent); F(energyTemporary);
        F(magicPermanent); F(magicTemporary);)
    V(XeenCombatInputs, F(might); F(speed); F(accuracy); F(temporaryAc); F(experience);
        F(luck); F(resistances); F(poisonResistance);)
    V(XeenSaveCombatSupplement, F(owner); F(inputs);)
    V(XeenGameplayContext, F(profile); F(difficulty); F(ctr24); F(day); F(year); F(minutes);
        F(effects); F(lightAndResistances); F(rested); F(newDay);)
    V(XeenJourneyRandomState, F(algorithm); F(state); F(count);)
    V(XeenMonsterTreasureItem, F(source); F(item);)
    V(XeenMonsterTreasure, F(gold); F(gems); F(pendingMask); F(pendingGold); F(weapons); F(armor); F(miscellaneous);)
    V(XeenHeadIndicators, F(left); F(right);)
    V(XeenHeadCache, F(mapId); F(overrides);)
    V(XeenWorldFlags, F(values);)
    V(XeenMerchantWares, F(records);)
    V(XeenBankBalances, F(gold); F(gems);)
    V(XeenServiceEconomy, F(wares); F(bank);)
    V(XeenBarrierOverride, F(tile); F(cell); F(mask); F(originalWord); F(originalAttributes); F(walls); F(unlocked);)
    V(XeenObjectOverride, F(id); F(x); F(y); F(flipped);)
    V(XeenExploredTile, F(mapId); F(cells);)
    V(XeenSaveJourneyActor, F(id); F(x); F(y); F(hp); F(activated); F(lifecycle); F(status); F(accounted); F(spawnedType);)
    V(XeenSaveJourney, F(entry); F(schema); F(content); F(context); F(supplements); F(skeletonSeed);
        F(random); F(initializedMap); F(originalActorCount); F(actors); F(vertigoActors);
        F(cityOriginalActorCount); F(treasure); F(worldFlags); F(serviceEconomy);)
    V(XeenSaveSnapshot, F(resources); F(name); F(food); F(camera); F(activeRosterIds); F(characters);
        F(questItems); F(questFlags); F(gameFlags); F(disabledObjects); F(disabledEvents);
        F(barriers); F(objectOverrides); F(exploredTiles); F(headCache); F(journey);)
#undef V
#undef F
};
// Per-map view (M69): the live per-map table's values under paths keyed by map
// identity and record, independent of the save layout. Absent and empty
// entries are equivalent, so only present values are written. This build
// projects the v12 layout exactly as restore distributes it: the Journey
// actors are the entry map's table, the city actors map 28's table, and every
// overlay belongs to its own identity's map or physical tile.
void perMap(Dump &d, const XeenSaveSnapshot &s) {
    d.value("view.perMap", 1); // Marks a build whose dump writes this view.
    const auto key = [](XeenMapIdentity id) {
        return "maps[" + std::to_string(unsigned(XeenSide(id.side))) + ":" + std::to_string(unsigned(id.number)) + "]";
    };
    const auto monsters = [&](XeenMapIdentity id, const std::vector<XeenSaveJourneyActor> &actors, std::size_t original) {
        const auto p = key(id) + ".monsters";
        d.value(p + ".present", true); d.value(p + ".size", actors.size()); d.value(p + ".originalCount", original);
        for (const auto &a : actors) {
            const auto m = p + "[" + std::to_string(std::size_t(a.id.recordIndex)) + "]";
            d.value(m + ".x", a.x); d.value(m + ".y", a.y); d.value(m + ".hp", a.hp); d.value(m + ".activated", a.activated);
            d.value(m + ".lifecycle", a.lifecycle); d.value(m + ".status", a.status); d.value(m + ".accounted", a.accounted);
            d.value(m + ".spawnedType", a.spawnedType);
        }
    };
    if (s.journey) {
        monsters(s.journey->initializedMap, s.journey->actors, s.journey->originalActorCount);
        if (s.journey->vertigoActors) monsters(28, *s.journey->vertigoActors, s.journey->cityOriginalActorCount);
    }
    for (const auto &id : s.disabledObjects)
        d.value(key(id.mapId) + ".objects[" + std::to_string(std::size_t(id.recordIndex)) + "].removed", true);
    for (const auto &v : s.objectOverrides) {
        const auto o = key(v.id.mapId) + ".objects[" + std::to_string(std::size_t(v.id.recordIndex)) + "]";
        d.value(o + ".x", v.x); d.value(o + ".y", v.y); d.value(o + ".flipped", v.flipped);
    }
    for (const auto &id : s.disabledEvents) // A disabled record is an opcode-0 override.
        d.value(key(id.mapId) + ".events[" + std::to_string(std::size_t(id.recordIndex)) + "].opcode", 0);
    for (const auto &v : s.barriers) {
        const auto c = key(v.tile) + ".cells[" + std::to_string(unsigned(v.cell)) + "]";
        d.value(c + ".mask", v.mask); d.value(c + ".originalWord", v.originalWord);
        d.value(c + ".originalAttributes", v.originalAttributes); d.value(c + ".walls", v.walls); d.value(c + ".unlocked", v.unlocked);
    }
    for (const auto &tile : s.exploredTiles) d.value(key(tile.mapId) + ".explored", tile.cells);
}
Dump decoded(const save_test::Bytes &wire) {
    auto snapshot = XeenSaveFormat::decode(wire); // Validates length, version and CRC.
    if (XeenSaveFormat::encode(snapshot) != wire) throw std::runtime_error("Noncanonical decode/encode");
    Dump d; d.value("snapshot", snapshot); perMap(d, snapshot);
    auto little = [&](std::size_t at, unsigned n) {
        std::uint32_t v = 0;
        for (unsigned i = 0; i < n; ++i) v |= std::uint32_t(wire.at(at+i)) << (8*i);
        return v;
    };
    d.value("envelope.version", little(8,2));
    d.value("envelope.payload_bytes", little(12,4));
    d.value("envelope.crc32", little(16,4));
    d.value("envelope.total_bytes", wire.size());
    return d;
}
void selfTest() {
    auto s = save_test::sample();
    s.name = "Audit";
    auto bytes = XeenSaveFormat::encode(s);
    const auto a = decoded(bytes).fields;
    save_test::check(a.at("snapshot.characters[0].name").size() == 36, "DOS string bytes lost");
    s.characters[29].awards[127] = 7;
    s.characters[29].skills[17]=255;
    s.exploredTiles={{23,{}}};s.exploredTiles[0].cells[31]=128;
    s.journey->random->count = 0xffffffffffffffffULL;
    s.journey->serviceEconomy->bank.gold = 123;
    auto &orc=s.journey->actors[2];orc.hp=0;orc.x=orc.y=-128;orc.activated=false;orc.accounted=true;orc.lifecycle=XeenActorLifecycle::Defeated;
    s.journey->treasure->miscellaneous[0]={2,{9,15,8,0}};
    const auto b = decoded(XeenSaveFormat::encode(s)).fields;
    save_test::check(b.at("snapshot.characters[29].skills[17]")=="255", "skill omitted");
    save_test::check(b.at("snapshot.exploredTiles[0].cells[31]")=="128", "exploration omitted");
    save_test::check(b.at("snapshot.characters[29].awards[127]") == "7", "award omitted");
    save_test::check(b.at("snapshot.journey.value.random.value.count") == "18446744073709551615", "uint64 truncated");
    save_test::check(b.at("snapshot.journey.value.serviceEconomy.value.bank.gold") == "123", "bank omitted");
    const std::string misc="snapshot.journey.value.treasure.value.miscellaneous";
    save_test::check(b.at(misc+"[0].source")=="2" && b.at(misc+"[0].item.material")=="9" &&
        b.at(misc+"[0].item.id")=="15" && b.at(misc+"[0].item.state")=="8" && b.at(misc+"[9].item.frame")=="0","Pending Misc omitted");
    // The per-map view keys the moved fields by map identity and record.
    save_test::check(b.at("maps[0:23].monsters[2].lifecycle")==std::to_string(unsigned(XeenActorLifecycle::Defeated)) &&
        b.at("maps[0:23].monsters[2].accounted")=="1" && b.at("maps[0:23].monsters.present")=="1" &&
        b.at("maps[0:23].monsters.originalCount")==std::to_string(s.journey->originalActorCount) &&
        b.at("maps[0:23].explored[31]")=="128","Per-map monster/exploration view omitted");
    save_test::check(b.at("maps[0:9999].objects[4294967295].removed")=="1" &&
        b.at("maps[0:3].events[1].opcode")=="0" && b.count("maps[0:28].monsters.present")==bool(s.journey->vertigoActors),
        "Per-map overlay view omitted or invented a city table");
    bytes.back() ^= 1;
    save_test::rejects([&] { decoded(bytes); });
    std::cout << "Canonical dump, DOS bytes, awards, RNG, bank and CRC rejection passed\n";
}
}
int main(int argc, char **argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--self-test") { selfTest(); return 0; }
        if (argc != 2) throw std::runtime_error("usage: mmodern_save_dump SAVE | --self-test");
        std::ifstream in(argv[1], std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open save");
        save_test::Bytes wire{std::istreambuf_iterator<char>(in), {}};
        auto d = decoded(wire);
        for (const auto &[key, value] : d.fields) std::cout << key << '\t' << value << '\n';
    } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
