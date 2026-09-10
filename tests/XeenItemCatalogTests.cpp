#include "formats/xeen/XeenMaterialNames.h"
#include "games/xeen/XeenItemCatalog.h"
#include "games/xeen/detail/XeenBoundedText.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;
namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

bool sameItem(const XeenItem &a, const XeenItem &b) {
	return a.material == b.material && a.id == b.id &&
		a.state == b.state && a.frame == b.frame;
}

std::vector<std::vector<std::uint8_t>> materialTokens() {
	std::vector<std::vector<std::uint8_t>> result(131);
	for (std::size_t i = 1; i < result.size(); ++i) {
		const std::string value = "material" + std::to_string(i);
		result[i].assign(value.begin(), value.end());
	}
	auto set = [&](std::size_t index, const std::string &value) {
		result[index].assign(value.begin(), value.end());
	};
	set(5, "100%metal");
	result[6] = {'a', 1, 0x80, '%', 'b'};
	set(38, "leather");
	set(42, "silver");
	set(77, "burning");
	return result;
}

std::vector<std::uint8_t> encode(
		const std::vector<std::vector<std::uint8_t>> &entries, bool finalNul = true) {
	std::vector<std::uint8_t> bytes;
	for (std::size_t i = 0; i < entries.size(); ++i) {
		bytes.insert(bytes.end(), entries[i].begin(), entries[i].end());
		if (finalNul || i + 1 != entries.size())
			bytes.push_back(0);
	}
	return bytes;
}

XeenItemCatalog syntheticCatalog() {
	const auto parsed = parseXeenMaterialNames(encode(materialTokens()));
	check(parsed.ready(), "synthetic material fixture must parse");
	return loadXeenItemCatalogWithMaterials(parsed);
}

void expectName(const XeenItemCatalog &catalog, XeenInventoryCategory category,
		XeenItem item, const char *expected) {
	const XeenItem before = item;
	const auto description = catalog.describe(category, item);
	check(description.displayName == expected, expected);
	check(sameItem(item, before) && sameItem(description.raw, before),
		"description changed or failed to retain raw item bytes");
	check(description.displayName.size() <= 192, "description exceeded its byte limit");
}

void testRepresentativeNames() {
	const auto catalog = syntheticCatalog();
	expectName(catalog, XeenInventoryCategory::Weapons, {0, 12, 0, 0}, "Dagger");
	expectName(catalog, XeenInventoryCategory::Armor, {38, 10, 0, 9}, "Leather boots");
	expectName(catalog, XeenInventoryCategory::Accessories, {42, 1, 0, 8}, "Silver ring");
	expectName(catalog, XeenInventoryCategory::Miscellaneous, {10, 37, 1, 0},
		"Potion of antidotes");
	expectName(catalog, XeenInventoryCategory::Weapons, {77, 12, 1, 0},
		"Burning dagger Dragon Slayer");
	expectName(catalog, XeenInventoryCategory::Weapons, {77, 12, 193, 1},
		"Broken cursed dagger Dragon Slayer");
	expectName(catalog, XeenInventoryCategory::Miscellaneous, {10, 37, 129, 7},
		"Broken potion");
	const auto hiddenEquipment = catalog.describe(XeenInventoryCategory::Weapons,
		{77, 12, 193, 1});
	check(hiddenEquipment.displayName.find("Burning") == std::string::npos,
		"suppressed material name leaked");
	const auto hiddenMisc = catalog.describe(XeenInventoryCategory::Miscellaneous,
		{10, 37, 129, 7});
	check(hiddenMisc.displayName.find("antidotes") == std::string::npos,
		"suppressed special name leaked");
}

void testDomainsAndFallbacks() {
	const auto catalog = syntheticCatalog();
	expectName(catalog, XeenInventoryCategory::Weapons, {0, 1, 0, 0}, "Long sword");
	expectName(catalog, XeenInventoryCategory::Weapons, {0, 40, 0, 0}, "Elder LongBow");
	expectName(catalog, XeenInventoryCategory::Armor, {0, 1, 0, 0}, "Robes");
	expectName(catalog, XeenInventoryCategory::Armor, {0, 13, 0, 0}, "Gauntlets");
	expectName(catalog, XeenInventoryCategory::Accessories, {0, 1, 0, 0}, "Ring");
	expectName(catalog, XeenInventoryCategory::Accessories, {0, 10, 0, 0}, "Amulet");

	for (const auto example : std::array<std::pair<XeenInventoryCategory, std::uint8_t>, 3>{{
		{XeenInventoryCategory::Weapons, 41},
		{XeenInventoryCategory::Armor, 14},
		{XeenInventoryCategory::Accessories, 11}}}) {
		const auto d = catalog.describe(example.first, {0, example.second, 0, 1});
		check(d.unsupportedFields.contains(XeenUnsupportedItemField::Id) && d.equipped,
			"first unsupported equipment ID was not typed/equipped safely");
	}

	expectName(catalog, XeenInventoryCategory::Weapons, {130, 12, 0, 0},
		"Material130 dagger");
	for (const std::uint8_t material : {std::uint8_t(131), std::uint8_t(255)}) {
		const auto d = catalog.describe(XeenInventoryCategory::Weapons, {material, 12, 0, 0});
		check(d.displayName == "Dagger [unknown material " + std::to_string(material) + "]" &&
			d.unsupportedFields.contains(XeenUnsupportedItemField::Material),
			"out-of-range equipment material fallback differs");
	}
	expectName(catalog, XeenInventoryCategory::Weapons, {77, 41, 0, 0},
		"Burning Unknown item [unknown ID 41]");

	expectName(catalog, XeenInventoryCategory::Miscellaneous, {1, 1, 0, 0},
		"Rod of light");
	expectName(catalog, XeenInventoryCategory::Miscellaneous, {11, 73, 63, 255},
		"Scroll of the GODS!");
	for (const std::uint8_t material : {std::uint8_t(0), std::uint8_t(12), std::uint8_t(21),
			std::uint8_t(22), std::uint8_t(255)}) {
		const auto d = catalog.describe(XeenInventoryCategory::Miscellaneous, {material, 37, 0, 0});
		check(d.unsupportedFields.contains(XeenUnsupportedItemField::Material),
			"unsupported miscellaneous container was not typed");
		check(d.displayName.find("bogus") == std::string::npos,
			"upstream placeholder was advertised");
	}
	for (const std::uint8_t id : {std::uint8_t(74), std::uint8_t(255)}) {
		const auto d = catalog.describe(XeenInventoryCategory::Miscellaneous, {10, id, 0, 0});
		check(d.displayName == "Potion [unknown special " + std::to_string(id) + "]" &&
			d.unsupportedFields.contains(XeenUnsupportedItemField::Id),
			"unknown misc special fallback differs");
		check(d.displayName.find(" of ") == std::string::npos, "orphan ITEM_OF emitted");
	}

	for (const std::uint8_t counter : {std::uint8_t(0), std::uint8_t(1), std::uint8_t(6),
			std::uint8_t(7), std::uint8_t(63)}) {
		const auto d = catalog.describe(XeenInventoryCategory::Weapons, {0, 12, counter, 0});
		check(d.counter == counter, "weapon counter not retained");
		check(d.unsupportedFields.contains(XeenUnsupportedItemField::Counter) == (counter >= 7),
			"weapon suffix support typing differs");
	}
}

void testStateFrameAndRawBytes() {
	const auto catalog = syntheticCatalog();
	for (unsigned value = 0; value < 256; ++value) {
		XeenItem weapon{0, 12, static_cast<std::uint8_t>(value),
			static_cast<std::uint8_t>(255 - value)};
		const XeenItem weaponBefore = weapon;
		const auto wd = catalog.describe(XeenInventoryCategory::Weapons, weapon);
		check(sameItem(weapon, weaponBefore) && sameItem(wd.raw, weaponBefore),
			"state/frame sweep mutated weapon bytes");
		check(wd.counter == (value & 63) && wd.cursed == ((value & 64) != 0) &&
			wd.broken == ((value & 128) != 0) && wd.equipped == (value != 255),
			"weapon state/frame interpretation differs");

		XeenItem misc{10, 37, static_cast<std::uint8_t>(255 - value),
			static_cast<std::uint8_t>(value)};
		const XeenItem miscBefore = misc;
		const auto md = catalog.describe(XeenInventoryCategory::Miscellaneous, misc);
		check(sameItem(misc, miscBefore) && sameItem(md.raw, miscBefore) && !md.equipped &&
			md.counterKind == XeenItemCounterKind::Charges && md.counter == ((255 - value) & 63),
			"miscellaneous state/frame interpretation differs");
	}

	for (const auto category : {XeenInventoryCategory::Weapons, XeenInventoryCategory::Armor,
			XeenInventoryCategory::Accessories}) {
		check(!catalog.describe(category, {0, 1, 0, 0}).equipped,
			"zero equipment frame marked equipped");
		check(catalog.describe(category, {0, 1, 0, 255}).equipped,
			"nonzero equipment frame not marked equipped");
	}

	const XeenItem opaqueEmpty{255, 0, 255, 255};
	for (unsigned category = 0; category < 256; ++category) {
		const auto d = catalog.describe(static_cast<XeenInventoryCategory>(category), opaqueEmpty);
		check(d.displayName == "Empty" && d.empty && !d.broken && !d.cursed && !d.equipped &&
			sameItem(d.raw, opaqueEmpty), "ID-zero precedence/raw preservation differs");
		if (category == static_cast<unsigned>(XeenInventoryCategory::Miscellaneous))
			check(d.counterKind == XeenItemCounterKind::Charges,
				"empty miscellaneous counter was not typed as charges");
	}
	const auto invalid = catalog.describe(static_cast<XeenInventoryCategory>(255), {1, 1, 255, 255});
	check(invalid.displayName == "Unknown item [category 255]" && !invalid.equipped &&
		invalid.unsupportedFields.contains(XeenUnsupportedItemField::Category),
		"invalid category was not bounded");

	XeenItemCategory records{};
	records[0] = {77, 12, 193, 255};
	records[4] = {255, 0, 255, 255};
	const auto before = records;
	for (const auto &item : records)
		(void)catalog.describe(XeenInventoryCategory::Weapons, item);
	for (std::size_t i = 0; i < records.size(); ++i)
		check(sameItem(records[i], before[i]), "array inspection changed storage");
}

void testAvailability() {
	XeenItemCatalog missing;
	const auto materialZero = missing.describe(XeenInventoryCategory::Weapons, {0, 12, 0, 0});
	check(materialZero.displayName == "Dagger", "missing materials blocked material-zero equipment");
	const auto needsMaterial = missing.describe(XeenInventoryCategory::Armor, {38, 10, 0, 0});
	check(needsMaterial.displayName == "Boots [material unavailable 38]" &&
		needsMaterial.materialAvailability == XeenMaterialAvailability::Missing,
		"missing material fallback differs");
	check(missing.describe(XeenInventoryCategory::Miscellaneous, {10, 37, 1, 0}).displayName ==
		"Potion of antidotes", "missing materials blocked miscellaneous composition");
	check(missing.describe(XeenInventoryCategory::Armor, {38, 10, 128, 0}).displayName ==
		"Broken boots", "suppression did not avoid unavailable-material fallback");

	const auto unavailable = XeenItemCatalog::unavailable().describe(
		XeenInventoryCategory::Weapons, {0, 12, 0, 1});
	check(unavailable.displayName == "Item catalog unavailable [ID 12, material 0]" &&
		unavailable.catalogAvailability == XeenCatalogAvailability::Unavailable && unavailable.equipped,
		"unavailable built-in catalog fallback differs");
	const auto unavailableEmpty = XeenItemCatalog::unavailable().describe(
		XeenInventoryCategory::Weapons, {9, 0, 255, 255});
	check(unavailableEmpty.displayName == "Empty" && unavailableEmpty.empty,
		"unavailable catalog blocked empty/raw inspection");

	const auto absent = loadXeenItemCatalogFromReader([] {
		return std::optional<std::vector<std::uint8_t>>{};
	});
	check(absent.catalog.materialAvailability() == XeenMaterialAvailability::Missing &&
		absent.diagnostic.find("missing") != std::string::npos,
		"absent resource was not distinguished");
	const auto malformed = loadXeenItemCatalogFromReader([] {
		return std::optional<std::vector<std::uint8_t>>(std::vector<std::uint8_t>{});
	});
	check(malformed.catalog.materialAvailability() == XeenMaterialAvailability::Malformed &&
		malformed.diagnostic.find("malformed") != std::string::npos,
		"present-empty resource was not malformed");
	const auto readError = loadXeenItemCatalogFromReader([]() -> std::optional<std::vector<std::uint8_t>> {
		throw std::runtime_error("synthetic read failure\x01");
	});
	check(readError.catalog.materialAvailability() == XeenMaterialAvailability::ReadError &&
		readError.diagnostic.find("synthetic read failure?") != std::string::npos &&
		readError.diagnostic.size() <= 192,
		"resource read-error fallback differs");
}

void expectMalformed(const std::vector<std::uint8_t> &bytes, const char *message) {
	const auto result = parseXeenMaterialNames(bytes);
	check(result.availability == XeenMaterialAvailability::Malformed, message);
	for (const auto &name : result.names)
		check(name.empty(), "malformed parser partially published material names");
}

void testMaterialParser() {
	auto entries = materialTokens();
	const auto valid = parseXeenMaterialNames(encode(entries));
	check(valid.ready() && valid.names[38] == "leather" && valid.names[42] == "silver" &&
		valid.names[5] == "100%metal" && valid.names[6] == "a??%b",
		"valid material parsing/sanitation differs");

	auto fewer = entries; fewer.pop_back();
	expectMalformed(encode(fewer), "130 entries accepted");
	auto extra = entries; extra.push_back({'x'});
	expectMalformed(encode(extra), "132 entries accepted");
	expectMalformed(encode(entries, false), "missing final terminator accepted");
	auto trailing = encode(entries); trailing.push_back('x');
	expectMalformed(trailing, "trailing bytes accepted");
	for (const auto rawZero : std::vector<std::vector<std::uint8_t>>{
			{'x'}, {' '}, {'\t'}, {1}, {0x80}}) {
		auto nonemptyZero = entries; nonemptyZero[0] = rawZero;
		expectMalformed(encode(nonemptyZero),
			"structurally nonempty entry zero accepted after normalization");
	}
	auto emptyRequired = entries; emptyRequired[1].clear();
	expectMalformed(encode(emptyRequired), "empty required entry accepted");
	auto whitespace = entries; whitespace[1] = {' ', '\t', '\r', '\n'};
	expectMalformed(encode(whitespace), "whitespace-only required entry accepted");
	auto length63 = entries; length63[1].assign(63, 'x');
	check(parseXeenMaterialNames(encode(length63)).ready(), "63-byte entry rejected");
	auto length64 = entries; length64[1].assign(64, 'x');
	expectMalformed(encode(length64), "64-byte entry accepted");

	std::vector<std::vector<std::uint8_t>> envelope(131);
	envelope[1].assign(63, 'x');
	for (std::size_t i = 2; i < envelope.size(); ++i)
		envelope[i].assign(62, 'x');
	const auto exact = encode(envelope);
	check(exact.size() == 8192 && parseXeenMaterialNames(exact).ready(),
		"valid 8,192-byte envelope rejected");
	auto tooLarge = exact; tooLarge.push_back(0);
	check(tooLarge.size() == 8193, "invalid envelope fixture size");
	expectMalformed(tooLarge, "8,193-byte resource accepted");
}

void testBoundedText() {
	detail::XeenBoundedText text(192);
	text.append(std::string(150, 'a'));
	text.append(std::string(100, 'b'));
	text.capitalizeFirstAsciiLetter();
	check(text.truncated() && text.value().size() == 192 &&
		text.value().substr(189) == "..." && text.value().front() == 'A',
		"synthetic long-token builder did not truncate deterministically");
}

} // namespace

int main() {
	try {
		testRepresentativeNames();
		testDomainsAndFallbacks();
		testStateFrameAndRawBytes();
		testAvailability();
		testMaterialParser();
		testBoundedText();
		std::cout << "Xeen item catalog tests passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
