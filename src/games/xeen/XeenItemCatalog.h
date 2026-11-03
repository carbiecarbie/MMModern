#ifndef MMODERN_GAMES_XEEN_XEEN_ITEM_CATALOG_H
#define MMODERN_GAMES_XEEN_XEEN_ITEM_CATALOG_H

#include "formats/xeen/XeenMaterialNames.h"
#include "games/xeen/XeenCharacter.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mmodern {

class XeenAssetSource;

enum class XeenInventoryCategory : std::uint8_t {
	Weapons = 0,
	Armor = 1,
	Accessories = 2,
	Miscellaneous = 3
};

enum class XeenCatalogAvailability {
	Available,
	Unavailable
};

enum class XeenItemCounterKind {
	EquipmentCounter,
	Charges
};

enum class XeenUnsupportedItemField : std::uint8_t {
	Category = 1,
	Id = 2,
	Material = 4,
	Counter = 8
};

struct XeenUnsupportedItemFields {
	std::uint8_t mask = 0;

	bool contains(XeenUnsupportedItemField field) const {
		return (mask & static_cast<std::uint8_t>(field)) != 0;
	}
	void add(XeenUnsupportedItemField field) {
		mask |= static_cast<std::uint8_t>(field);
	}
	bool empty() const { return mask == 0; }
};

struct XeenItemDescription {
	std::string displayName;
	XeenItem raw;
	bool empty = false;
	bool broken = false;
	bool cursed = false;
	bool equipped = false;
	std::uint8_t counter = 0;
	XeenItemCounterKind counterKind = XeenItemCounterKind::EquipmentCounter;
	XeenUnsupportedItemFields unsupportedFields;
	XeenCatalogAvailability catalogAvailability = XeenCatalogAvailability::Available;
	XeenMaterialAvailability materialAvailability = XeenMaterialAvailability::Missing;
	bool truncated = false;
	// End of the "broken " and "cursed " prefixes within displayName.
	std::size_t brokenEnd = 0, cursedEnd = 0;
	// Russian equipment (M110 Part C) puts the words after the name: they
	// start at this index, in line color before it.
	std::size_t decorationBegin = 0;
	bool russianOrder = false;
	// The pin's getFullDescription: "broken" in color 32, "cursed" in color
	// 09, the rest in the given color (the list line's, 4 in the Discard
	// prompt, 15 in the other item prompts; DOS shows the same).
	std::string coloredName(unsigned lineColor) const;
};

// Item names read from the Russian program (M110 Part B); the English names
// are the build-time catalog. Russian weapons stop at id 34: neither program
// names ids 35-40, which keep the catalog's names.
struct XeenItemNames {
	std::vector<std::string> weapons, armor, accessories, misc, special, bonus;
	// The Russian program's words and order (Part C): weapons, armor and
	// accessories read name, broken, cursed, then material and suffix
	// (RU 0x3c1bb..0x3c4bf against CD 0x3cd00..0x3d004).
	std::string broken, cursed, of;
	bool russian = false;
};

class XeenItemCatalog {
public:
	XeenItemCatalog();
	static XeenItemCatalog unavailable();

	XeenCatalogAvailability availability() const { return _availability; }
	XeenMaterialAvailability materialAvailability() const { return _materialAvailability; }
	XeenItemDescription describe(XeenInventoryCategory category,
		const XeenItem &item) const;
	// Party::giveTreasure and the equip/proficiency refusals print the base
	// item/container name, without the inventory description's material,
	// power or condition decorations.
	std::string foundName(XeenInventoryCategory,const XeenItem &) const;
	void setNames(std::shared_ptr<const XeenItemNames> names) { _names = std::move(names); }

private:
	friend struct XeenItemCatalogLoadResult;
	friend XeenItemCatalog loadXeenItemCatalogWithMaterials(
		const XeenMaterialNameParseResult &materials);

	XeenCatalogAvailability _availability = XeenCatalogAvailability::Available;
	XeenMaterialAvailability _materialAvailability = XeenMaterialAvailability::Missing;
	std::shared_ptr<const std::array<std::string, 131>> _materials;
	std::shared_ptr<const XeenItemNames> _names;
	enum class Names { Weapons, Armor, Accessories, Misc, Special, Bonus };
	std::string_view name(Names table, unsigned index) const;
};

struct XeenItemCatalogLoadResult {
	XeenItemCatalog catalog;
	std::string diagnostic;
};

using XeenMaterialResourceReader =
	std::function<std::optional<std::vector<std::uint8_t>>() >;

XeenItemCatalog loadXeenItemCatalogWithMaterials(
	const XeenMaterialNameParseResult &materials);
XeenItemCatalogLoadResult loadXeenItemCatalog(XeenAssetSource &assets);
XeenItemCatalogLoadResult loadXeenItemCatalogFromReader(
	const XeenMaterialResourceReader &reader, XeenLanguage language = XeenLanguage::English);

// Smoke-only source-provenance helper. It parses only the reviewed bounded
// catalog block and compares every token with the embedded generated tables.
bool xeenVerifyItemCatalogReferenceBlock(const std::vector<std::uint8_t> &constants,
	std::string &diagnostic);

} // namespace mmodern

#endif
