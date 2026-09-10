#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenMaterialNames.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenItemCatalog.h"
#include "games/xeen/XeenPartyLoader.h"

#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;
namespace fs = std::filesystem;
namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

std::vector<std::uint8_t> readFile(const fs::path &path) {
	std::ifstream input(path, std::ios::binary);
	if (!input)
		throw std::runtime_error("cannot read reference file: " + path.u8string());
	return {std::istreambuf_iterator<char>(input), {}};
}

std::string sha256(const std::vector<std::uint8_t> &bytes) {
	BCRYPT_ALG_HANDLE algorithm = nullptr;
	BCRYPT_HASH_HANDLE hash = nullptr;
	DWORD objectLength = 0, hashLength = 0, received = 0;
	std::vector<std::uint8_t> object;
	std::vector<std::uint8_t> digest;
	auto cleanup = [&] {
		if (hash) BCryptDestroyHash(hash);
		if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
	};
	try {
		check(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM,
			nullptr, 0) >= 0, "cannot open SHA-256 provider");
		check(BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
			reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &received, 0) >= 0,
			"cannot query SHA-256 object length");
		check(BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH,
			reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength), &received, 0) >= 0,
			"cannot query SHA-256 digest length");
		object.resize(objectLength);
		digest.resize(hashLength);
		check(BCryptCreateHash(algorithm, &hash, object.data(), objectLength,
			nullptr, 0, 0) >= 0, "cannot create SHA-256 hash");
		check(bytes.size() <= static_cast<std::size_t>(std::numeric_limits<ULONG>::max()),
			"reference file is too large for SHA-256 check");
		check(BCryptHashData(hash, const_cast<PUCHAR>(bytes.data()),
			static_cast<ULONG>(bytes.size()), 0) >= 0, "cannot update SHA-256 hash");
		check(BCryptFinishHash(hash, digest.data(), hashLength, 0) >= 0,
			"cannot finish SHA-256 hash");
		cleanup();
	} catch (...) {
		cleanup();
		throw;
	}
	std::ostringstream out;
	out << std::hex << std::setfill('0');
	for (const auto byte : digest)
		out << std::setw(2) << static_cast<unsigned>(byte);
	return out.str();
}

bool sameItem(const XeenItem &a, const XeenItem &b) {
	return a.material == b.material && a.id == b.id &&
		a.state == b.state && a.frame == b.frame;
}

void checkAllItems(const XeenRoster &actual, const XeenRoster &expected) {
	for (std::size_t owner = 0; owner < XeenRoster::kCharacterCount; ++owner) {
		const auto &a = actual.at(owner);
		const auto &e = expected.at(owner);
		const XeenItemCategory *ac[] = {&a.weapons, &a.armor, &a.accessories, &a.miscellaneous};
		const XeenItemCategory *ec[] = {&e.weapons, &e.armor, &e.accessories, &e.miscellaneous};
		for (std::size_t category = 0; category < 4; ++category)
			for (std::size_t slot = 0; slot < 9; ++slot)
				check(sameItem((*ac[category])[slot], (*ec[category])[slot]),
					"catalog description mutated original owner item bytes");
	}
}

struct SmokeContext {
	std::vector<std::uint8_t> materialBytes;
	XeenItemCatalog catalog;
};

SmokeContext runCatalogSmoke(const fs::path &game) {
	const auto installation = XeenInstallationDetector().detect(game);
	check(installation && installation->hasXeen(), "Clouds installation unavailable");
	XeenAssetSource assets(*installation);
	const auto loaded = loadXeenItemCatalog(assets);
	check(loaded.catalog.availability() == XeenCatalogAvailability::Available,
		"built-in item catalog unavailable");
	check(loaded.catalog.materialAvailability() == XeenMaterialAvailability::Ready,
		loaded.diagnostic.empty() ? "commercial material names unavailable" : loaded.diagnostic.c_str());
	const auto materialBytes = assets.readItemMaterialNamesFromDarkArchive();
	check(materialBytes.has_value(), "DARK.CC/mae.xen disappeared after catalog loading");
	check(materialBytes->size() <= 8192 && parseXeenMaterialNames(*materialBytes).ready(),
		"runtime material stream is not structurally supported");

	const auto party = XeenPartyLoader().loadInitialCloudsParty(assets);
	const auto before = party.roster;
	struct Anchor {
		std::size_t owner;
		XeenInventoryCategory category;
		std::size_t slot;
		XeenItem expected;
		const char *name;
		bool equipped;
	};
	const std::array<Anchor, 3> anchors{{
		{11, XeenInventoryCategory::Weapons, 0, {0, 12, 0, 1}, "Dagger", true},
		{0, XeenInventoryCategory::Armor, 3, {38, 10, 0, 9}, "Leather boots", true},
		{11, XeenInventoryCategory::Accessories, 1, {42, 1, 0, 8}, "Silver ring", true}
	}};
	for (const auto &anchor : anchors) {
		const auto &character = party.roster.at(anchor.owner);
		const XeenItemCategory *category = anchor.category == XeenInventoryCategory::Weapons ?
			&character.weapons : anchor.category == XeenInventoryCategory::Armor ?
			&character.armor : &character.accessories;
		const XeenItem actual = category->at(anchor.slot);
		check(sameItem(actual, anchor.expected), "original equipment anchor bytes changed");
		const auto description = loaded.catalog.describe(anchor.category, actual);
		check(description.displayName == anchor.name && description.equipped == anchor.equipped &&
			description.unsupportedFields.empty() && sameItem(description.raw, actual),
			"original equipment anchor description changed");
	}
	checkAllItems(party.roster, before);
	std::cout << "Catalog consumer anchors: Dagger, Leather boots, Silver ring; exact bytes/equipped state PASS\n";
	std::cout << "Runtime DARK.CC/mae.xen: " << materialBytes->size()
		<< " bytes, 131 structurally valid entries, SHA-256 " << sha256(*materialBytes) << '\n';
	return {*materialBytes, loaded.catalog};
}

void runReferenceVerification(const fs::path &reference,
		const std::vector<std::uint8_t> &runtimeMaterial) {
	const auto constants = readFile(reference / "devtools/create_mm/files/xeen/CONSTANTS_7");
	const auto material = readFile(reference / "devtools/create_mm/files/xeen/mae.cld");
	check(constants.size() == 35065, "pinned CONSTANTS_7 size changed");
	check(sha256(constants) == "a3022d378e7570a56332f30128942afe02ae2bfdef70c307b2de997eb07a9e77",
		"pinned CONSTANTS_7 SHA-256 changed");
	check(material.size() == 1093, "pinned mae.cld size changed");
	check(sha256(material) == "78f3ec8421fd46619a4aba63dca1042c914b0f3437d91ce37a47f81676bd4156",
		"pinned mae.cld SHA-256 changed");
	check(runtimeMaterial == material,
		"runtime DARK.CC/mae.xen differs from pinned mae.cld certification evidence");
	std::string diagnostic;
	check(xeenVerifyItemCatalogReferenceBlock(constants, diagnostic),
		diagnostic.empty() ? "catalog reference block verification failed" : diagnostic.c_str());
	std::cout << "Reference verification: CONSTANTS_7 [20680,22438), six count tags, 169 entries, three strings PASS\n";
	std::cout << "Reference verification: runtime mae.xen equals pinned mae.cld; certified installation PASS\n";
}

} // namespace

int main(int argc, char **argv) {
	try {
		if (argc == 2) {
			(void)runCatalogSmoke(argv[1]);
			return 0;
		}
		if (argc == 4 && std::string(argv[1]) == "--verify-reference") {
			const auto context = runCatalogSmoke(argv[2]);
			runReferenceVerification(argv[3], context.materialBytes);
			return 0;
		}
		std::cerr << "Usage: mmodern_item_catalog_smoke <game directory>\n"
			"   or: mmodern_item_catalog_smoke --verify-reference <game directory> <pinned ScummVM checkout>\n";
		return 2;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
