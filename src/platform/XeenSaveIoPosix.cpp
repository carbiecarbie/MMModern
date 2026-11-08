#include "core/PathText.h"
#include "platform/XeenSaveIo.h"
#include "platform/XeenUserFolders.h"
#include "platform/XeenFileOperations.h"
#include "formats/xeen/XeenSaveFormat.h"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <mutex>
#include <sstream>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#if defined(__APPLE__)
#include <sys/mount.h>
#else
#include <sys/statfs.h>
#include <sys/sysmacros.h>
#endif
#include <unistd.h>
namespace mmodern::saveio {
namespace {
std::runtime_error error(const char *operation) {
 const int code=errno;
 return std::runtime_error(std::string(operation)+" failed (POSIX error "+std::to_string(code)+")");
}
struct Handle {
 int value=-1;
 ~Handle(){if(value>=0)::close(value);}
 void close(){const int fd=value;value=-1;if(::close(fd)<0)throw error("Close save file");}
};
Path directory(const Path &path) {
 char *value=::realpath(path.c_str(),nullptr);
 if(!value)throw error("Resolve save directory");
 const Path result(value);std::free(value);
 struct stat st{};
 if(::stat(result.c_str(),&st)<0)throw error("Inspect save directory");
 if(!S_ISDIR(st.st_mode))throw std::runtime_error("Save parent directory must already exist");
 return result;
}
#if defined(__APPLE__)
void requireLocal(const struct statfs &fs,const struct stat &) {
 if((fs.f_flags&MNT_LOCAL)==0)throw std::runtime_error("Save path must be on a local filesystem; set XDG_DATA_HOME to a local disk for saves");
}
#else
void requireLocal(const struct statfs &fs,const struct stat &st) {
 bool remote=remoteType(static_cast<unsigned long>(fs.f_type));
 if(static_cast<unsigned long>(fs.f_type)==0x65735546) {
  std::ifstream input("/proc/self/mountinfo");if(!input)throw error("Read save mount information");
  const std::string text((std::istreambuf_iterator<char>(input)),{});
  if(input.bad())throw error("Read save mount information");
  remote=remoteFuseMount(text,major(st.st_dev),minor(st.st_dev));
 }
 if(remote)throw std::runtime_error("Save path must be on a local filesystem; set XDG_DATA_HOME to a local disk for saves");
}
#endif
void local(const Path &path) {
 struct statfs fs{};if(::statfs(path.c_str(),&fs)<0)throw error("Inspect save filesystem");
 struct stat st{};if(::stat(path.c_str(),&st)<0)throw error("Inspect save directory");
 requireLocal(fs,st);
}
void local(int directory) {
 struct statfs fs{};if(::fstatfs(directory,&fs)<0)throw error("Inspect save filesystem");
 struct stat st{};if(::fstat(directory,&st)<0)throw error("Inspect save directory");
 requireLocal(fs,st);
}

}
bool remoteType(unsigned long type) {
 switch(type) {
 case 0x6969: case 0x517b: case 0xff534d42: case 0xfe534d42: // NFS, SMB, CIFS, SMB2
 case 0x5346414f: case 0x6b414653: case 0x73757245: case 0x00c36400: // AFS, Coda, Ceph
 case 0x564c: case 0x01021997: case 0x786f4256: return true; // NCP, 9P, vboxsf
 default:return false;
 }
}
bool remoteFuseMount(const std::string &text,unsigned deviceMajor,unsigned deviceMinor) {
 std::istringstream lines(text);std::string line;
 const auto device=std::to_string(deviceMajor)+":"+std::to_string(deviceMinor);
 while(std::getline(lines,line)) {
  std::istringstream fields(line);std::string id,parent,dev,field;
  if(!(fields>>id>>parent>>dev) || dev!=device)continue;
  while(fields>>field && field!="-"){}
  if(!(fields>>field))continue;
  for(const auto *name:{"sshfs","rclone","gvfsd-fuse","s3fs","davfs","curlftpfs"})
   if(field==name || field==std::string("fuse.")+name)return true;
 }
 return false;
}
bool ordinary(const Path &path,bool absent) {
 struct stat st{};
 if(::lstat(path.c_str(),&st)<0) {
  if(absent && errno==ENOENT)return false;
  throw error("Inspect save file");
 }
 if(!S_ISREG(st.st_mode))throw std::runtime_error("Save target must be an ordinary local file");
 return true;
}
Path resolve(const Path &path,const Path &protectedRoot) {
 if(path.empty())throw std::runtime_error("Save path is empty");
 const auto absolute=std::filesystem::absolute(path).lexically_normal();
 auto extension=absolute.extension().native();
 for(auto &c:extension)if(c>='A' && c<='Z')c+=32;
 if(extension!=".mmsave")throw std::runtime_error("Save path requires a .mmsave extension");
 const auto parent=directory(absolute.parent_path());local(parent);
 struct stat root{};
 if(::stat(protectedRoot.c_str(),&root)<0) {
  if(errno!=ENOENT && errno!=ENOTDIR)throw error("Inspect protected directory");
 } else {
  if(!S_ISDIR(root.st_mode))throw std::runtime_error("Protected save root must be a directory");
  // Compare identity, including aliases, rather than path spelling. A bind mount
  // of a subdirectory beneath a protected root is not detected (maintainer
  // accepted 2026-11-04; creating such a mount requires root). Do not inspect
  // mountinfo for this check; remote classification is separate.
  for(auto ancestor=parent;;ancestor=ancestor.parent_path()) {
   struct stat st{};if(::stat(ancestor.c_str(),&st)<0)throw error("Inspect save ancestor");
   if(st.st_dev==root.st_dev && st.st_ino==root.st_ino)
    throw std::runtime_error("Cannot save inside the commercial game installation");
   if(ancestor==ancestor.root_path())break;
  }
 }
 const auto result=parent/absolute.filename();ordinary(result,true);return result;
}
namespace {
std::vector<std::uint8_t> readFile(Handle &file) {
 struct stat st{};if(::fstat(file.value,&st)<0)throw error("Read save size");
 if(!S_ISREG(st.st_mode))throw std::runtime_error("Save target must be an ordinary local file");
 if(st.st_size<0 || std::uint64_t(st.st_size)>XeenSaveFormat::kMaximumSize)throw std::runtime_error("Save file is oversized");
 std::vector<std::uint8_t> bytes(st.st_size);std::size_t offset=0;
 while(offset<bytes.size()) {
  const auto count=::read(file.value,bytes.data()+offset,bytes.size()-offset);
  if(count<0){if(errno==EINTR)continue;throw error("Read save file");}
  if(!count)throw std::runtime_error("Save file was truncated while reading");
  offset+=count;
 }
 file.close();return bytes;
}
}
std::vector<std::uint8_t> read(const Path &path) {
 Handle file{::open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC)};
 if(file.value<0)throw error("Open save file");
 return readFile(file);
}

void write(const Path &path,const std::vector<std::uint8_t> &bytes,
 const std::function<void(const std::vector<std::uint8_t> &)> &validatePrevious,const XeenSaveFile::Fault &fault) {
 using Operation=XeenSaveFile::Operation;
 static std::mutex mutex;std::lock_guard<std::mutex> lock(mutex);
 const auto parent=directory(path.parent_path());
 if(parent!=path.parent_path())throw std::runtime_error("Save parent directory identity changed");
 Handle directoryHandle{::open(parent.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC)};
 if(directoryHandle.value<0)throw error("Open save directory");
 while(::flock(directoryHandle.value,LOCK_EX)<0)if(errno!=EINTR)throw error("Acquire save writer lock");
 local(directoryHandle.value);
 const auto leaf=path.filename();
 const auto validateTarget=[&] {
  struct stat st{};
  if(::fstatat(directoryHandle.value,leaf.c_str(),&st,AT_SYMLINK_NOFOLLOW)<0) {
   if(errno==ENOENT)return;
   throw error("Inspect save file");
  }
  if(!S_ISREG(st.st_mode))throw std::runtime_error("Save target must be an ordinary local file");
  Handle previous{::openat(directoryHandle.value,leaf.c_str(),O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC)};
  if(previous.value<0)throw error("Open save file");
  validatePrevious(readFile(previous));
 };
 validateTarget();
 const auto fails=[&](Operation op){return fault && fault(op);};
 const auto require=[&](Operation op,const char *message){if(fails(op))throw std::runtime_error(message);};
 static std::atomic<unsigned long long> sequence{0};Path temporary,temporaryLeaf;Handle file;bool created=false,published=false;
 try {
  require(Operation::Open,"Injected temporary open failure");
  for(unsigned attempt=0;attempt<100;++attempt) {
   temporary=path;temporary+=".tmp-"+std::to_string(::getpid())+"-"+std::to_string(++sequence);
   temporaryLeaf=temporary.filename();
   file.value=::openat(directoryHandle.value,temporaryLeaf.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0666);
   if(file.value>=0){created=true;break;}
   if(errno!=EEXIST)throw error("Create temporary save");
  }
  if(!created)throw std::runtime_error("Cannot create a unique temporary save");
  std::size_t offset=0;
  while(offset<bytes.size()) {
   require(Operation::Write,"Injected write failure");
   const auto requested=std::min<std::size_t>(65536,bytes.size()-offset);
   const bool shortWrite=fails(Operation::ShortWrite);
   const auto count=::write(file.value,bytes.data()+offset,shortWrite?requested/2:requested);
   if(count<0){if(errno==EINTR)continue;throw error("Write save file");}
   if(shortWrite)throw std::runtime_error("Short save write");
   if(!count)throw std::runtime_error("Short save write");
   offset+=count;
  }
  require(Operation::Flush,"Injected flush failure");
  while(::fsync(file.value)<0)if(errno!=EINTR)throw error("Flush save file");
  file.close();require(Operation::Close,"Injected close failure");
  require(Operation::Replace,"Injected replacement failure");
  require(Operation::Revalidate,"Injected re-validation failure");validateTarget();
  if(::renameat(directoryHandle.value,temporaryLeaf.c_str(),directoryHandle.value,leaf.c_str())<0)throw error("Publish save file");
  published=true;
  require(Operation::Publish,"Publish save file: injected failure");
  xeenSyncDirectory(directoryHandle.value);
 }catch(const std::exception &failure) {
  std::string message=failure.what();
  if(file.value>=0){try{file.close();}catch(const std::exception &e){message+=std::string("; ")+e.what();}}
  if(created && !published && (fails(Operation::Cleanup) || ::unlinkat(directoryHandle.value,temporaryLeaf.c_str(),0)<0))
   message+="; temporary cleanup failed: "+xeenPathText(temporary);
  throw std::runtime_error(message);
 }
}
Path createSlotDirectory(const std::function<Path(const Path &)> &check,const std::function<std::string()> &dataId,const std::optional<Path> &override) {
 Path base;
 std::vector<Path> components;
 if(override){base=*override;components={"MMModern","Saves"};}
 else {base=xeenUserFolder(false);components={"Saves"};}
 if(base.empty() || !base.is_absolute())throw std::runtime_error("User save base must be an absolute existing directory");
 // Validate the nearest existing ancestor before creating a missing XDG base.
 // This prevents even an empty MMModern folder from appearing in protected data.
 std::vector<Path> missing;struct stat existing{};
 while(!override && ::lstat(base.c_str(),&existing)<0) {
  if(errno!=ENOENT && errno!=ENOTDIR)throw error("Inspect user save base");
  missing.push_back(base.filename());base=base.parent_path();
 }
 base=check(base);
 const auto id=dataId();
 for(auto it=missing.rbegin();it!=missing.rend();++it) {
  const auto child=base/(*it);
  if(::mkdir(child.c_str(),0700)<0 && errno!=EEXIST)throw error("Create user save base");
  base=check(child);
 }
 components.push_back(id);
 for(const auto &component:components) {
  const auto child=base/component;struct stat st{};
  if(::lstat(child.c_str(),&st)==0)base=check(child);
  else {
   if(errno!=ENOENT && errno!=ENOTDIR)throw error("Inspect managed save directory");
   if(::mkdir(child.c_str(),0700)<0 && errno!=EEXIST)throw error("Create managed save directory");
   base=check(child);
  }
 }
 return base;
}
}
