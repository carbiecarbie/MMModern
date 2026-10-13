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
Dump decoded(const save_test::Bytes &wire) {
    auto snapshot = XeenSaveFormat::decode(wire); // Validates length, version and CRC.
    if (XeenSaveFormat::encode(snapshot) != wire) throw std::runtime_error("Noncanonical decode/encode");
    Dump d; d.value("snapshot", snapshot);
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
