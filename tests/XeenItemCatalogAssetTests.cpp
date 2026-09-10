#include "SyntheticXeenArchive.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenItemCatalog.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mmodern;
using namespace sprite_test;
namespace fs = std::filesystem;
namespace {

void check(bool value, const char *message) {
	if (!value)
		throw std::runtime_error(message);
}

Bytes validMaterials() {
	Bytes bytes{0};
	for (unsigned index = 1; index <= 130; ++index) {
		const std::string name = "material" + std::to_string(index);
		bytes.insert(bytes.end(), name.begin(), name.end());
		bytes.push_back(0);
	}
	return bytes;
}

GameInstallation installation(const fs::path &directory) {
	GameInstallation result;
	result.root = directory;
	result.xeenArchive = directory / "xeen.cc";
	result.darkArchive = directory / "dark.cc";
	result.edition = GameEdition::WorldOfXeen;
	return result;
}

void checkFallbacks(const XeenItemCatalog &catalog,
		XeenMaterialAvailability expectedAvailability) {
	const auto material = catalog.describe(
		XeenInventoryCategory::Armor, {38, 10, 0, 0});
	check(material.materialAvailability == expectedAvailability &&
		material.displayName == "Boots [material unavailable 38]",
		"unavailable material table leaked partial material names");
	check(catalog.describe(XeenInventoryCategory::Weapons, {0, 12, 0, 0}).displayName ==
		"Dagger", "material failure hid built-in equipment names");
	check(catalog.describe(XeenInventoryCategory::Miscellaneous, {10, 37, 1, 0}).displayName ==
		"Potion of antidotes", "material failure hid built-in miscellaneous names");
}

} // namespace

int main() {
	const auto directory = fs::temp_directory_path() /
		("mmodern-item-catalog-assets-" + std::to_string(
			std::chrono::steady_clock::now().time_since_epoch().count()));
	try {
		fs::create_directories(directory);
		struct Cleanup {
			fs::path path;
			~Cleanup() { std::error_code error; fs::remove_all(path, error); }
		} cleanup{directory};

		const auto currentInstallation = installation(directory);
		archive(currentInstallation.xeenArchive, {{"dummy", {1}}});
		const Bytes clouds{1, 2, 3, 4};

		archive(currentInstallation.darkArchive,
			{{"clouds.dat", clouds}, {"mae.xen", validMaterials()}});
		{
			XeenAssetSource assets(currentInstallation);
			const auto loaded = loadXeenItemCatalog(assets);
			check(loaded.catalog.materialAvailability() == XeenMaterialAvailability::Ready &&
				loaded.diagnostic.empty(), "valid production archive material read failed");
			check(loaded.catalog.describe(XeenInventoryCategory::Armor,
				{38, 10, 0, 0}).displayName == "Material38 boots",
				"valid production archive materials were not published");
		}

		archive(currentInstallation.darkArchive,
			{{"clouds.dat", clouds}, {"mae.xen", {0}}});
		{
			XeenAssetSource assets(currentInstallation);
			const auto loaded = loadXeenItemCatalog(assets);
			check(loaded.catalog.materialAvailability() == XeenMaterialAvailability::Malformed &&
				loaded.diagnostic.find("malformed") != std::string::npos,
				"structurally malformed production member was not typed");
			checkFallbacks(loaded.catalog, XeenMaterialAvailability::Malformed);
		}

		archive(currentInstallation.darkArchive,
			{{"clouds.dat", clouds}, {"mae.xen", validMaterials()}});
		const auto completeSize = fs::file_size(currentInstallation.darkArchive);
		fs::resize_file(currentInstallation.darkArchive, completeSize - 10);
		{
			XeenAssetSource assets(currentInstallation);
			check(assets.readCloudsVisualMetadataFromDarkArchive() ==
				std::optional<Bytes>(clouds),
				"catalog-specific recovery changed existing clouds.dat behavior");
			const auto loaded = loadXeenItemCatalog(assets);
			check(loaded.catalog.materialAvailability() == XeenMaterialAvailability::ReadError &&
				loaded.diagnostic.find("truncated") != std::string::npos,
				"truncated indexed payload did not return a typed read error");
			checkFallbacks(loaded.catalog, XeenMaterialAvailability::ReadError);
		}

		archive(currentInstallation.darkArchive, {{"clouds.dat", clouds}});
		{
			XeenAssetSource assets(currentInstallation);
			const auto loaded = loadXeenItemCatalog(assets);
			check(loaded.catalog.materialAvailability() == XeenMaterialAvailability::Missing,
				"missing production archive member was not typed");
			checkFallbacks(loaded.catalog, XeenMaterialAvailability::Missing);
		}

		std::cout << "Xeen item catalog production asset-path tests passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::error_code ignored;
		fs::remove_all(directory, ignored);
		std::cerr << error.what() << '\n';
		return 1;
	}
}
