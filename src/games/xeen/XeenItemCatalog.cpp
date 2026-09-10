#include "games/xeen/XeenItemCatalog.h"

#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/detail/XeenBoundedText.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <string_view>

#include "XeenItemCatalogEnglish.inc"

namespace mmodern {
namespace {

using namespace generated_item_catalog;
constexpr std::size_t kDescriptionLimit = 192;
constexpr std::string_view kExpectedRevision =
	"6814ee9ba54582f5b5adcffab49efbbd8f589edd";

static_assert(kSchema == 1, "unexpected generated item catalog schema");
static_assert(kLanguage == 7, "unexpected generated item catalog language");
static_assert(kSourceRevision == kExpectedRevision,
	"unexpected generated item catalog revision");
static_assert(kBonusNames.size() == 7);
static_assert(kWeaponNames.size() == 41);
static_assert(kArmorNames.size() == 14);
static_assert(kAccessoryNames.size() == 11);
static_assert(kMiscNames.size() == 22);
static_assert(kSpecialNames.size() == 74);

std::string trim(std::string_view value) {
	std::size_t first = 0;
	while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
		++first;
	std::size_t last = value.size();
	while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
		--last;
	return std::string(value.substr(first, last - first));
}

std::string decoration(std::string_view value, std::string_view directive) {
	if (value.substr(0, directive.size()) == directive)
		value.remove_prefix(directive.size());
	return trim(value);
}

std::string numeric(const char *label, unsigned value) {
	return std::string("[") + label + ' ' + std::to_string(value) + ']';
}

void appendUnknownPair(detail::XeenBoundedText &text, const char *firstLabel,
		unsigned first, const char *secondLabel, unsigned second) {
	text.append(std::string("[") + firstLabel + ' ' + std::to_string(first) +
		", " + secondLabel + ' ' + std::to_string(second) + ']');
}

template<std::size_t N>
std::string_view at(const std::array<std::string_view, N> &values, unsigned index) {
	return index < values.size() ? values[index] : std::string_view{};
}

XeenItemDescription baseDescription(const XeenItemCatalog &catalog, XeenItem item) {
	XeenItemDescription result;
	result.raw = item;
	result.empty = item.id == 0;
	result.broken = (item.state & 128) != 0;
	result.cursed = (item.state & 64) != 0;
	result.counter = item.state & 63;
	result.catalogAvailability = catalog.availability();
	result.materialAvailability = catalog.materialAvailability();
	return result;
}

template<std::size_t N>
bool readCStringArray(const std::vector<std::uint8_t> &bytes, std::size_t &position,
		const std::array<std::string_view, N> &expected, std::string &diagnostic) {
	if (position + 4 > bytes.size() || bytes[position] != 0 || bytes[position + 1] != 0 ||
			bytes[position + 2] != 0 || bytes[position + 3] != N) {
		diagnostic = "catalog array count tag mismatch";
		return false;
	}
	position += 4;
	for (std::size_t i = 0; i < N; ++i) {
		const std::size_t start = position;
		while (position < bytes.size() && bytes[position] != 0)
			++position;
		if (position == bytes.size()) {
			diagnostic = "truncated catalog token";
			return false;
		}
		const std::string_view actual(reinterpret_cast<const char *>(bytes.data() + start),
			position - start);
		if (actual != expected[i]) {
			diagnostic = "generated catalog token differs at entry " + std::to_string(i);
			return false;
		}
		++position;
	}
	return true;
}

bool readScalar(const std::vector<std::uint8_t> &bytes, std::size_t &position,
		std::string_view expected, std::string &diagnostic) {
	const std::size_t start = position;
	while (position < bytes.size() && bytes[position] != 0)
		++position;
	if (position == bytes.size()) {
		diagnostic = "truncated catalog scalar";
		return false;
	}
	const std::string_view actual(reinterpret_cast<const char *>(bytes.data() + start),
		position - start);
	++position;
	if (actual != expected) {
		diagnostic = "generated catalog scalar differs";
		return false;
	}
	return true;
}

} // namespace

XeenItemCatalog::XeenItemCatalog() = default;

XeenItemCatalog XeenItemCatalog::unavailable() {
	XeenItemCatalog result;
	result._availability = XeenCatalogAvailability::Unavailable;
	return result;
}

XeenItemDescription XeenItemCatalog::describe(XeenInventoryCategory category,
		const XeenItem &item) const {
	XeenItemDescription result = baseDescription(*this, item);
	const unsigned rawCategory = static_cast<unsigned>(category);
	const bool categoryValid = rawCategory <= static_cast<unsigned>(XeenInventoryCategory::Miscellaneous);
	result.counterKind = category == XeenInventoryCategory::Miscellaneous ?
		XeenItemCounterKind::Charges : XeenItemCounterKind::EquipmentCounter;
	if (result.empty) {
		result.displayName = "Empty";
		result.broken = false;
		result.cursed = false;
		return result;
	}

	result.equipped = categoryValid && category != XeenInventoryCategory::Miscellaneous && item.frame != 0;
	if (!categoryValid) {
		result.unsupportedFields.add(XeenUnsupportedItemField::Category);
		detail::XeenBoundedText text(kDescriptionLimit);
		text.append("Unknown item");
		text.append(numeric("category", rawCategory));
		text.capitalizeFirstAsciiLetter();
		result.displayName = text.value();
		result.truncated = text.truncated();
		return result;
	}
	if (_availability == XeenCatalogAvailability::Unavailable) {
		detail::XeenBoundedText text(kDescriptionLimit);
		text.append("Item catalog unavailable");
		appendUnknownPair(text, "ID", item.id, "material", item.material);
		result.displayName = text.value();
		result.truncated = text.truncated();
		return result;
	}

	detail::XeenBoundedText text(kDescriptionLimit);
	if (result.broken)
		text.append(decoration(kItemBroken, "\f32"));
	if (result.cursed)
		text.append(decoration(kItemCursed, "\f09"));

	if (category == XeenInventoryCategory::Miscellaneous) {
		const bool containerKnown = item.material >= 1 && item.material <= 11;
		const bool specialKnown = item.id >= 1 && item.id <= 73;
		if (!containerKnown)
			result.unsupportedFields.add(XeenUnsupportedItemField::Material);
		if (!specialKnown)
			result.unsupportedFields.add(XeenUnsupportedItemField::Id);
		if (containerKnown)
			text.append(trim(kMiscNames[item.material]));
		if (!result.broken && !result.cursed && specialKnown) {
			if (containerKnown)
				text.append(trim(kItemOf));
			text.append(trim(kSpecialNames[item.id]));
		}
		if (!containerKnown && (result.broken || result.cursed || !specialKnown))
			text.append("Unknown item");
		if (!containerKnown)
			text.append(numeric("unknown container", item.material));
		if (!specialKnown)
			text.append(numeric("unknown special", item.id));
	} else {
		std::string_view base;
		unsigned maximumId = 0;
		switch (category) {
		case XeenInventoryCategory::Weapons:
			base = at(kWeaponNames, item.id); maximumId = 40; break;
		case XeenInventoryCategory::Armor:
			base = at(kArmorNames, item.id); maximumId = 13; break;
		case XeenInventoryCategory::Accessories:
			base = at(kAccessoryNames, item.id); maximumId = 10; break;
		case XeenInventoryCategory::Miscellaneous:
			break;
		}
		const bool baseKnown = item.id >= 1 && item.id <= maximumId;
		if (!baseKnown)
			result.unsupportedFields.add(XeenUnsupportedItemField::Id);
		const bool materialInRange = item.material <= 130;
		if (!materialInRange)
			result.unsupportedFields.add(XeenUnsupportedItemField::Material);
		if (!result.broken && !result.cursed && item.material != 0 && materialInRange &&
				_materialAvailability == XeenMaterialAvailability::Ready && _materials)
			text.append((*_materials)[item.material]);
		if (baseKnown)
			text.append(trim(base));
		else
			text.append("Unknown item");
		if (!result.broken && !result.cursed && item.material != 0) {
			if (!materialInRange)
				text.append(numeric("unknown material", item.material));
			else if (_materialAvailability != XeenMaterialAvailability::Ready || !_materials)
				text.append(numeric("material unavailable", item.material));
		}
		if (!baseKnown)
			text.append(numeric("unknown ID", item.id));
		if (category == XeenInventoryCategory::Weapons && result.counter != 0) {
			if (result.counter <= 6)
				text.append(trim(kBonusNames[result.counter]));
			else {
				result.unsupportedFields.add(XeenUnsupportedItemField::Counter);
				text.append(numeric("unknown suffix", result.counter));
			}
		}
	}
	text.capitalizeFirstAsciiLetter();
	result.displayName = text.value();
	result.truncated = text.truncated();
	return result;
}

XeenItemCatalog loadXeenItemCatalogWithMaterials(
		const XeenMaterialNameParseResult &materials) {
	XeenItemCatalog result;
	result._materialAvailability = materials.availability;
	if (materials.ready())
		result._materials = std::make_shared<const std::array<std::string, 131>>(materials.names);
	return result;
}

XeenItemCatalogLoadResult loadXeenItemCatalogFromReader(
		const XeenMaterialResourceReader &reader) {
	try {
		const auto bytes = reader();
		if (!bytes) {
			XeenMaterialNameParseResult missing;
			missing.availability = XeenMaterialAvailability::Missing;
			return {loadXeenItemCatalogWithMaterials(missing),
				"Material names unavailable: DARK.CC/mae.xen is missing"};
		}
		auto parsed = parseXeenMaterialNames(*bytes);
		if (parsed.ready())
			return {loadXeenItemCatalogWithMaterials(parsed), {}};
		return {loadXeenItemCatalogWithMaterials(parsed),
			"Material names malformed: " + parsed.diagnostic};
	} catch (const std::exception &error) {
		XeenMaterialNameParseResult failed;
		failed.availability = XeenMaterialAvailability::ReadError;
		detail::XeenBoundedText diagnostic(kDescriptionLimit);
		diagnostic.append("Material names unavailable: read error:");
		std::string sanitized;
		for (const unsigned char c : std::string_view(error.what()))
			sanitized.push_back(c >= 32 && c <= 126 ? static_cast<char>(c) : '?');
		diagnostic.append(sanitized);
		return {loadXeenItemCatalogWithMaterials(failed), diagnostic.value()};
	}
}

XeenItemCatalogLoadResult loadXeenItemCatalog(XeenAssetSource &assets) {
	return loadXeenItemCatalogFromReader([&assets] {
		return assets.readItemMaterialNamesFromDarkArchive();
	});
}

bool xeenVerifyItemCatalogReferenceBlock(const std::vector<std::uint8_t> &constants,
		std::string &diagnostic) {
	constexpr std::size_t kStart = 20680;
	constexpr std::size_t kEnd = 22438;
	if (constants.size() < kEnd) {
		diagnostic = "CONSTANTS_7 is too short for the catalog block";
		return false;
	}
	std::size_t position = kStart;
	if (!readScalar(constants, position, kItemBroken, diagnostic) ||
			!readScalar(constants, position, kItemCursed, diagnostic) ||
			!readScalar(constants, position, kItemOf, diagnostic) ||
			!readCStringArray(constants, position, kBonusNames, diagnostic) ||
			!readCStringArray(constants, position, kWeaponNames, diagnostic) ||
			!readCStringArray(constants, position, kArmorNames, diagnostic) ||
			!readCStringArray(constants, position, kAccessoryNames, diagnostic) ||
			!readCStringArray(constants, position, kMiscNames, diagnostic) ||
			!readCStringArray(constants, position, kSpecialNames, diagnostic))
		return false;
	if (position != kEnd) {
		diagnostic = "CONSTANTS_7 catalog block end offset differs";
		return false;
	}
	diagnostic.clear();
	return true;
}

} // namespace mmodern
