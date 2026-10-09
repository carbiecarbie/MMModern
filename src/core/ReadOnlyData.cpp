#include "core/ReadOnlyData.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>
namespace mmodern {
namespace {
std::uint64_t add(std::uint64_t a,std::uint64_t b) {
 if(b>std::numeric_limits<std::uint64_t>::max()-a) throw std::runtime_error("Read-only source offset overflow");
 return a+b;
}
}
ReadOnlyDataFile ReadOnlyDataFile::plain(const std::filesystem::path &path) {
 ReadOnlyDataFile file; file.physicalPath=std::filesystem::weakly_canonical(path);
 file.logicalSize=std::filesystem::file_size(path); file.physicalSize=file.logicalSize; return file;
}
std::unique_ptr<ReadOnlyDataStream> ReadOnlyDataFile::open() const { return std::make_unique<ReadOnlyDataStream>(*this); }
std::string ReadOnlyDataFile::identity() const {
 return physicalPath.u8string()+"|"+std::to_string(static_cast<unsigned>(transport))+"|"+
  std::to_string(trackBase)+"|"+std::to_string(logicalOffset)+"|"+std::to_string(logicalSize);
}
ReadOnlyDataStream::ReadOnlyDataStream(const ReadOnlyDataFile &file):
 _input(file.physicalPath,std::ios::binary),_size(file.logicalSize),_trackBase(file.trackBase),
 _logicalOffset(file.logicalOffset),_raw(file.transport==ReadOnlyDataFile::Transport::Mode1Raw),_origin(file.identity()) {
 if(!_input) throw std::runtime_error("Cannot open read-only source: "+_origin);
 _input.seekg(0,std::ios::end); const auto physicalSize=_input.tellg();
 if(physicalSize<0) throw std::runtime_error("Cannot size read-only source: "+_origin);
 if(file.physicalSize && static_cast<std::uint64_t>(physicalSize)!=file.physicalSize) throw std::runtime_error("Read-only source size changed: "+_origin);
 const auto logicalEnd=add(_logicalOffset,_size);
 std::uint64_t physicalEnd=logicalEnd;
 if(_raw && logicalEnd) {
  const auto last=logicalEnd-1;
  if(last/2048>(std::numeric_limits<std::uint64_t>::max()-_trackBase)/2352) throw std::runtime_error("Raw source extent overflow");
  physicalEnd=add(add(_trackBase,(last/2048)*2352),2352);
 }
 if(physicalEnd>static_cast<std::uint64_t>(physicalSize)) throw std::runtime_error("Truncated read-only source extent: "+_origin);
}
bool ReadOnlyDataStream::seek(std::uint64_t offset) noexcept {
 if(offset>_size) return false;
 _position=offset; return true;
}
std::size_t ReadOnlyDataStream::read(void *destination,std::size_t count) {
 count=static_cast<std::size_t>(std::min<std::uint64_t>(count,_size-_position));
 auto *out=static_cast<char *>(destination); std::size_t done=0;
 while(done<count) {
  const auto logical=add(_logicalOffset,_position); std::uint64_t physical=logical;
  std::size_t chunk=count-done;
  if(_raw) {
   const auto sector=add(_trackBase,(logical/2048)*2352);
   if(sector>static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) throw std::runtime_error("Raw-sector seek offset overflow");
   std::array<unsigned char,16> header{};
   _input.clear(); _input.seekg(static_cast<std::streamoff>(sector));
   if(!_input.read(reinterpret_cast<char *>(header.data()),header.size())) throw std::runtime_error("Short raw-sector header: "+_origin);
   if(header[0]!=0 || header[11]!=0 || header[15]!=1 ||
     !std::all_of(header.begin()+1,header.begin()+11,[](unsigned char value){return value==255;}))
    throw std::runtime_error("Invalid MODE1/2352 sync or mode: "+_origin);
   physical=add(sector,16+logical%2048); chunk=std::min<std::size_t>(chunk,2048-logical%2048);
  }
  if(physical>static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) throw std::runtime_error("Source seek offset overflow");
  _input.clear(); _input.seekg(static_cast<std::streamoff>(physical));
  if(!_input.read(out+done,static_cast<std::streamsize>(chunk))) throw std::runtime_error("Short read-only source read: "+_origin);
  done+=chunk; _position+=chunk;
 }
 return done;
}
}
