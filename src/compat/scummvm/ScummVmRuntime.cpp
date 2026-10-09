#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include "compat/scummvm/ScummVmRuntime.h"
#include "common/archive.h"
#include "common/stream.h"
#include <cstdio>
#include <limits>
#include <vector>
namespace MM {class MMEngine;MMEngine *g_engine=nullptr;}
namespace mmodern {
namespace {
class SourceReadStream final:public Common::SeekableReadStream {
public:
 explicit SourceReadStream(const ReadOnlyDataFile &file):_input(file.open()){}
 bool err() const override{return _error;}
 void clearErr() override{_error=false;}
 bool eos() const override{return _input->pos()>=_input->size();}
 uint32 read(void *data,uint32 count) override {
  try{return static_cast<uint32>(_input->read(data,count));}catch(...){_error=true;throw;}
 }
 int64 pos() const override{return static_cast<int64>(_input->pos());}
 int64 size() const override{return static_cast<int64>(_input->size());}
 bool seek(int64 offset,int whence=SEEK_SET) override {
  const auto base=whence==SEEK_SET?0:whence==SEEK_CUR?pos():whence==SEEK_END?size():-1;
  if(base<0 || (offset>0&&base>std::numeric_limits<int64>::max()-offset) || offset< -base)return false;
  return _input->seek(static_cast<std::uint64_t>(base+offset));
 }
private:std::unique_ptr<ReadOnlyDataStream> _input;bool _error=false;
};
struct Entry{Common::String name;ReadOnlyDataFile file;};
class SourceArchive final:public Common::Archive {
public:
 void add(const char *name,const ReadOnlyDataFile &file){_entries.push_back({Common::String(name),file});}
 bool hasFile(const Common::Path &path) const override{return find(path)!=nullptr;}
 int listMembers(Common::ArchiveMemberList &list) const override {
  for(const auto &entry:_entries)list.push_back(Common::ArchiveMemberPtr(new Common::GenericArchiveMember(Common::Path(entry.name,Common::Path::kNoSeparator),*this)));
  return static_cast<int>(_entries.size());
 }
 const Common::ArchiveMemberPtr getMember(const Common::Path &path) const override {
  return hasFile(path)?Common::ArchiveMemberPtr(new Common::GenericArchiveMember(path,*this)):Common::ArchiveMemberPtr();
 }
 Common::SeekableReadStream *createReadStreamForMember(const Common::Path &path) const override {
  const auto *entry=find(path);return entry?new SourceReadStream(entry->file):nullptr;
 }
private:
 const Entry *find(const Common::Path &path) const {
  auto name=path.baseName();name.toLowercase();for(const auto &entry:_entries)if(entry.name==name)return &entry;return nullptr;
 }
 std::vector<Entry> _entries;
};
}
struct ScummVmRuntime::Impl {
 SourceArchive files;
 explicit Impl(const GameInstallation &installation){
  if(installation.hasXeen())files.add("xeen.cc",archiveDataFile(installation,XeenArchiveRole::Clouds));
  if(installation.hasDarkside())files.add("dark.cc",archiveDataFile(installation,XeenArchiveRole::Darkside));
  if(installation.introData)files.add("intro.cc",*installation.introData);
  SearchMan.add("mmodern_input",&files,100,false);
 }
 ~Impl(){SearchMan.remove("mmodern_input");}
};
ScummVmRuntime::ScummVmRuntime(const GameInstallation &installation):_impl(new Impl(installation)){}
ScummVmRuntime::~ScummVmRuntime()=default;
}
