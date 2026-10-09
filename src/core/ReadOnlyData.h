#ifndef MMODERN_CORE_READ_ONLY_DATA_H
#define MMODERN_CORE_READ_ONLY_DATA_H
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
namespace mmodern {
// A logical, bounded file. MODE1 transport exposes only the 2048-byte payload.
struct ReadOnlyDataFile;
class ReadOnlyDataStream {
public:
 explicit ReadOnlyDataStream(const ReadOnlyDataFile &file);
 std::uint64_t size() const noexcept { return _size; }
 std::uint64_t pos() const noexcept { return _position; }
 bool seek(std::uint64_t offset) noexcept;
 std::size_t read(void *destination, std::size_t count);
private:
 std::ifstream _input;
 std::uint64_t _size=0, _position=0, _trackBase=0, _logicalOffset=0;
 bool _raw=false;
 std::string _origin;
};
struct ReadOnlyDataFile {
 enum class Transport { Plain, Mode1Raw };
 std::filesystem::path physicalPath;
 Transport transport=Transport::Plain;
 std::uint64_t trackBase=0, logicalOffset=0, logicalSize=0, physicalSize=0;
 static ReadOnlyDataFile plain(const std::filesystem::path &path);
 std::unique_ptr<ReadOnlyDataStream> open() const;
 std::string identity() const;
};
}
#endif
