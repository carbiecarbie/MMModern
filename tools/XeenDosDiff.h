#ifndef MMODERN_TOOLS_XEEN_DOS_DIFF_H
#define MMODERN_TOOLS_XEEN_DOS_DIFF_H
#include "tools/XeenDosExport.h"
#include <ostream>
namespace mmodern::developer {
// Reports every changed payload byte. Returns false for invalid markers or,
// when requested, differences outside Save As name and play time.
bool diffDosPairs(const DosExportPair &, const DosExportPair &, bool resaveCheck, std::ostream &);
DosExportPair readDosPair(const std::filesystem::path &, const std::string &slot);
}
#endif
