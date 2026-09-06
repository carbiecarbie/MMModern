#include "games/xeen/XeenInstallationDetector.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace mmodern {
namespace {

std::string asciiLower(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

std::filesystem::path findFile(const std::filesystem::path &root, const char *wantedName) {
	std::error_code error;
	for (const std::filesystem::directory_entry &entry :
			std::filesystem::directory_iterator(root, error)) {
		if (error)
			break;

		if (entry.is_regular_file(error) &&
				asciiLower(entry.path().filename().string()) == wantedName)
			return entry.path();
	}

	return {};
}

} // namespace

std::optional<GameInstallation> XeenInstallationDetector::detect(
		const std::filesystem::path &root) const {
	std::error_code error;
	if (!std::filesystem::is_directory(root, error))
		return std::nullopt;

	GameInstallation installation;
	installation.root = std::filesystem::absolute(root, error);
	if (error)
		installation.root = root;

	installation.xeenArchive = findFile(root, "xeen.cc");
	installation.darkArchive = findFile(root, "dark.cc");

	if (!installation.hasXeen() && !installation.hasDarkside())
		return std::nullopt;

	if (installation.hasXeen() && installation.hasDarkside())
		installation.edition = GameEdition::WorldOfXeen;
	else if (installation.hasXeen())
		installation.edition = GameEdition::CloudsOfXeen;
	else
		installation.edition = GameEdition::DarksideOfXeen;

	return installation;
}

} // namespace mmodern
