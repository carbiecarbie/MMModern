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
};

class XeenItemCatalog {
public:
	XeenItemCatalog();
	static XeenItemCatalog unavailable();

	XeenCatalogAvailability availability() const { return _availability; }
	XeenMaterialAvailability materialAvailability() const { return _materialAvailability; }
	XeenItemDescription describe(XeenInventoryCategory category,
		const XeenItem &item) const;

private:
	friend struct XeenItemCatalogLoadResult;
	friend XeenItemCatalog loadXeenItemCatalogWithMaterials(
		const XeenMaterialNameParseResult &materials);

	XeenCatalogAvailability _availability = XeenCatalogAvailability::Available;
	XeenMaterialAvailability _materialAvailability = XeenMaterialAvailability::Missing;
	std::shared_ptr<const std::array<std::string, 131>> _materials;
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
	const XeenMaterialResourceReader &reader);

// Smoke-only source-provenance helper. It parses only the reviewed bounded
// catalog block and compares every token with the embedded generated tables.
bool xeenVerifyItemCatalogReferenceBlock(const std::vector<std::uint8_t> &constants,
	std::string &diagnostic);

} // namespace mmodern

#endif
