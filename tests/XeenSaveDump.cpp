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
        F(lloydMap); F(lloydX); F(lloydY); F(lloydSide); F(currentHp); F(currentSp); F(conditions); F(birthYear); F(awards);)
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
    V(XeenPurse, F(gold); F(gems);)
    V(XeenHeadIndicators, F(left); F(right);)
    V(XeenHeadCache, F(mapId); F(overrides);)
    V(XeenWorldFlags, F(values);)
    V(XeenMerchantWares, F(records);)
    V(XeenBankBalances, F(gold); F(gems);)
    V(XeenServiceEconomy, F(wares); F(bank);)
    V(XeenSaveMonster, F(x); F(y); F(lifecycle); F(status); F(accounted); F(spawnedType);)
    V(XeenSaveMonsterTable, F(originalCount); F(slots);)
    V(XeenSaveObjectOverride, F(record); F(x); F(y); F(flipped);)
    V(XeenSaveWallItemOverride, F(record); F(x); F(y);)
    V(XeenSaveCellOverride, F(cell); F(word); F(attributes); F(originalWord); F(originalAttributes);)
    V(XeenSaveEventOverride, F(record); F(opcode);)
    V(XeenSaveMapState, F(id); F(monsters); F(objects); F(wallItems); F(cells); F(events); F(explored); F(tavernTips);)
    V(XeenSaveJourney, F(entry); F(schema); F(content); F(context); F(supplements); F(skeletonSeed);
        F(random);
        // v17 saves the purse alone under the earlier `treasure` path, so audits
        // compare its gold/gems with older builds; pending treasure is not saved.
        value(p + ".treasure", v.purse);
        F(worldFlags); F(serviceEconomy);)
    V(XeenSoundOptions, F(efx); F(music); F(text); F(speech);)
    V(XeenSaveSnapshot, F(soundOptions); F(resources); F(name); F(food); F(camera); F(activeRosterIds); F(characters);
        F(questItems); F(questFlags); F(gameFlags); F(headCache); F(journey); F(maps);)
#undef V
#undef F
};
// Per-map view (M69): the live per-map table's values under paths keyed by map
// identity and record, independent of the save layout. Absent and empty
// entries are equivalent, so only present values are written. v13 stores the
// table itself; the view keeps the v12 build's paths (a removed object, the
// barrier faces and unlocked bit of a cell) so both builds compare. HP and
// wake state are no longer saved (decision 8).
void perMap(Dump &d, const XeenSaveSnapshot &s) {
    d.value("view.perMap", 1); // Marks a build whose dump writes this view.
    const auto key = [](XeenMapIdentity id) {
        return "maps[" + std::to_string(unsigned(XeenSide(id.side))) + ":" + std::to_string(unsigned(id.number)) + "]";
    };
    for (const auto &m : s.maps) {
        if (m.tavernTips) d.value(key(m.id) + ".tavernTips", *m.tavernTips);
        if (m.monsters) {
            const auto p = key(m.id) + ".monsters";
            d.value(p + ".present", true); d.value(p + ".size", m.monsters->slots.size());
            d.value(p + ".originalCount", std::size_t(m.monsters->originalCount));
            for (std::size_t i = 0; i < m.monsters->slots.size(); ++i) {
                const auto &a = m.monsters->slots[i];
                const auto n = p + "[" + std::to_string(i) + "]";
                d.value(n + ".x", a.x); d.value(n + ".y", a.y); d.value(n + ".lifecycle", a.lifecycle);
                d.value(n + ".status", a.status); d.value(n + ".accounted", a.accounted); d.value(n + ".spawnedType", a.spawnedType);
            }
        }
        for (const auto &v : m.objects) {
            const auto o = key(m.id) + ".objects[" + std::to_string(unsigned(v.record)) + "]";
            if (v.x == -128 && v.y == -128) d.value(o + ".removed", true);
            else { d.value(o + ".x", v.x); d.value(o + ".y", v.y); d.value(o + ".flipped", v.flipped); }
        }
        for (const auto &v : m.wallItems) {
            const auto o = key(m.id) + ".wallItems[" + std::to_string(unsigned(v.record)) + "]";
            d.value(o + ".x", v.x); d.value(o + ".y", v.y);
        }
        for (const auto &v : m.events)
            d.value(key(m.id) + ".events[" + std::to_string(unsigned(v.record)) + "].opcode", v.opcode);
        for (const auto &v : m.cells) {
            const auto c = key(m.id) + ".cells[" + std::to_string(unsigned(v.cell)) + "]";
            d.value(c + ".word", v.word); d.value(c + ".attributes", v.attributes);
            d.value(c + ".originalWord", v.originalWord); d.value(c + ".originalAttributes", v.originalAttributes);
            // The changed faces and unlocked bit, as the v12 build's view wrote them.
            std::uint8_t mask = 0; std::array<std::uint8_t, 4> walls{};
            for (unsigned f = 0; f < 4; ++f) {
                const auto shift = 12 - 4 * f;
                const auto wall = (v.word >> shift) & 15u;
                if (wall != ((v.originalWord >> shift) & 15u)) { mask |= 1u << f; walls[f] = std::uint8_t(wall); }
            }
            d.value(c + ".mask", mask); d.value(c + ".walls", walls); d.value(c + ".unlocked", v.attributes != v.originalAttributes);
        }
        if (m.explored) d.value(key(m.id) + ".explored", *m.explored);
    }
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
    s.characters[29].lloydMap=23;s.characters[29].lloydX=9;s.characters[29].lloydY=11;s.characters[29].lloydSide=1;
    s.map(23).explored.emplace();(*s.map(23).explored)[31]=128;
    s.map(23).tavernTips=14;s.map(28).tavernTips=0;
    s.journey->random->count = 0xffffffffffffffffULL;
    s.journey->serviceEconomy->bank.gold = 123;
    auto &orc=s.map(23).monsters->slots[2];orc.x=orc.y=-128;orc.accounted=true;orc.lifecycle=XeenActorLifecycle::Defeated;
    s.journey->purse->gold=0xfedcba98u;s.journey->purse->gems=7;
    s.map(28).cells.push_back({255,0x1999,0x80,0x9999,0});
    const auto b = decoded(XeenSaveFormat::encode(s)).fields;
    save_test::check(b.at("maps[0:23].tavernTips")=="14" && b.at("maps[0:28].tavernTips")=="0" &&
        b.at("snapshot.maps[1].tavernTips.present")=="0", "Tip cursor dump omitted zero, absence or map identity");
    save_test::check(b.at("snapshot.characters[29].lloydMap")=="23" && b.at("snapshot.characters[29].lloydX")=="9" &&
        b.at("snapshot.characters[29].lloydY")=="11" && b.at("snapshot.characters[29].lloydSide")=="1","Beacon dump incomplete");
    save_test::check(b.at("snapshot.characters[29].skills[17]")=="255", "skill omitted");
    save_test::check(b.at("snapshot.characters[29].awards[127]") == "7", "award omitted");
    save_test::check(b.at("snapshot.journey.value.random.value.count") == "18446744073709551615", "uint64 truncated");
    save_test::check(b.at("snapshot.journey.value.serviceEconomy.value.bank.gold") == "123", "bank omitted");
    const std::string purse="snapshot.journey.value.treasure.value";
    save_test::check(b.at(purse+".gold")=="4275878552" && b.at(purse+".gems")=="7" &&
        !b.count(purse+".pendingGold") && !b.count("snapshot.journey.value.treasureSources[0].recordIndex"),"Purse omitted or pending treasure dumped");
    // The per-map view keys the table by map identity and record.
    save_test::check(b.at("maps[0:23].monsters[2].lifecycle")==std::to_string(unsigned(XeenActorLifecycle::Defeated)) &&
        b.at("maps[0:23].monsters[2].accounted")=="1" && b.at("maps[0:23].monsters.present")=="1" &&
        b.at("maps[0:23].monsters.originalCount")==std::to_string(s.map(23).monsters->originalCount) &&
        b.at("maps[0:23].explored[31]")=="128" && !b.count("maps[0:23].monsters[2].hp"),"Per-map monster/exploration view omitted");
    save_test::check(b.at("maps[0:9999].objects[65535].removed")=="1" &&
        b.at("maps[0:3].events[1].opcode")=="0" && !b.count("maps[0:28].monsters.present") &&
        b.at("maps[0:28].cells[255].mask")=="1" && b.at("maps[0:28].cells[255].walls[0]")=="1" &&
        b.at("maps[0:28].cells[255].unlocked")=="1","Per-map overlay view omitted or invented a city table");
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
