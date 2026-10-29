#ifndef MMODERN_TOOLS_XEEN_DOS_EXPORT_H
#define MMODERN_TOOLS_XEEN_DOS_EXPORT_H
#include "core/GameInstallation.h"
#include "tools/XeenDosArchive.h"
#include "games/xeen/XeenSaveSnapshot.h"
namespace mmodern::developer {
inline constexpr unsigned kDosExportNameLimit=15; // Maintainer decision, 2026-10-29 (M97 fallback).
bool validDosExportName(const std::string &) noexcept;
DosBytes dosExportNam(const std::string &);
struct DosExportPair {DosMembers clouds, darkside;};
DosExportPair dosNewGame(const GameInstallation &);
// Restores through the gameplay owners before any byte projection.
DosExportPair projectDosSave(const GameInstallation &,const XeenSaveSnapshot &,const std::string &name);
std::string dosProjectionSummary();
void exportDosSave(const GameInstallation &,const std::filesystem::path &input,
	const std::filesystem::path &output,unsigned slot,const std::string &name,bool overwrite=false);
}
#endif
