#ifndef MMODERN_TOOLS_XEEN_DOS_ARCHIVE_H
#define MMODERN_TOOLS_XEEN_DOS_ARCHIVE_H
#include <cstdint>
#include <string>
#include <vector>
namespace mmodern::developer {
using DosBytes = std::vector<std::uint8_t>;
struct DosMember { std::string name; std::uint16_t id; DosBytes bytes; };
using DosMembers = std::vector<DosMember>;
std::uint16_t dosMemberId(const std::string &);
std::string dosMapName(unsigned id, const char *extension);
DosMembers readDosArchive(const DosBytes &);
DosBytes writeDosArchive(const DosMembers &);
// Retains the template's index order. DAT identities come from their payload.
void nameDosMembers(DosMembers &);
}
#endif
