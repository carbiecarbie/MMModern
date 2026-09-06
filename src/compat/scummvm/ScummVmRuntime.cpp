#define FORBIDDEN_SYMBOL_ALLOW_ALL

#include "compat/scummvm/ScummVmRuntime.h"

#include "common/archive.h"
#include "common/stream.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

// The reused sprite decoder consults this global only for engine-specific
// loading and drawing modes. MMModern uses its stream loader and normal draw.
namespace MM {
class MMEngine;
MMEngine *g_engine = nullptr;
}

namespace mmodern {
namespace {

class HostFileReadStream final : public Common::SeekableReadStream {
public:
	explicit HostFileReadStream(const std::filesystem::path &path) :
		_input(path, std::ios::binary), _position(0), _size(-1), _error(false) {
		if (!_input) {
			_error = true;
			return;
		}

		_input.seekg(0, std::ios::end);
		const std::streamoff length = _input.tellg();
		if (length < 0) {
			_error = true;
			return;
		}

		_size = static_cast<int64>(length);
		_input.seekg(0, std::ios::beg);
		if (!_input)
			_error = true;
	}

	bool isOpen() const {
		return _input.is_open() && !_error;
	}

	bool err() const override {
		return _error;
	}

	void clearErr() override {
		_error = false;
		_input.clear();
	}

	bool eos() const override {
		return _size >= 0 && _position >= _size;
	}

	uint32 read(void *dataPtr, uint32 dataSize) override {
		if (_error || dataSize == 0)
			return 0;

		_input.read(static_cast<char *>(dataPtr), static_cast<std::streamsize>(dataSize));
		const std::streamsize count = _input.gcount();
		if (_input.bad())
			_error = true;
		_position += static_cast<int64>(count);
		return static_cast<uint32>(count);
	}

	int64 pos() const override {
		return _error ? -1 : _position;
	}

	int64 size() const override {
		return _error ? -1 : _size;
	}

	bool seek(int64 offset, int whence = SEEK_SET) override {
		int64 base = 0;
		switch (whence) {
		case SEEK_SET:
			break;
		case SEEK_CUR:
			base = _position;
			break;
		case SEEK_END:
			base = _size;
			break;
		default:
			return false;
		}

		const int64 target = base + offset;
		if (_size < 0 || target < 0)
			return false;

		_input.clear();
		_input.seekg(static_cast<std::streamoff>(target), std::ios::beg);
		if (!_input) {
			_error = true;
			return false;
		}

		_position = target;
		_error = false;
		return true;
	}

private:
	std::ifstream _input;
	int64 _position;
	int64 _size;
	bool _error;
};

struct HostFileEntry {
	Common::String logicalName;
	std::filesystem::path physicalPath;
};

class HostFilesArchive final : public Common::Archive {
public:
	void add(const char *logicalName, const std::filesystem::path &physicalPath) {
		_entries.push_back({ Common::String(logicalName), physicalPath });
		_entries.back().logicalName.toLowercase();
	}

	bool hasFile(const Common::Path &path) const override {
		return find(path) != nullptr;
	}

	int listMembers(Common::ArchiveMemberList &list) const override {
		for (const HostFileEntry &entry : _entries) {
			list.push_back(Common::ArchiveMemberPtr(new Common::GenericArchiveMember(
				Common::Path(entry.logicalName, Common::Path::kNoSeparator), *this)));
		}
		return static_cast<int>(_entries.size());
	}

	const Common::ArchiveMemberPtr getMember(const Common::Path &path) const override {
		if (!hasFile(path))
			return Common::ArchiveMemberPtr();
		return Common::ArchiveMemberPtr(new Common::GenericArchiveMember(path, *this));
	}

	Common::SeekableReadStream *createReadStreamForMember(
			const Common::Path &path) const override {
		const HostFileEntry *entry = find(path);
		if (!entry)
			return nullptr;

		HostFileReadStream *stream = new HostFileReadStream(entry->physicalPath);
		if (!stream->isOpen()) {
			delete stream;
			return nullptr;
		}
		return stream;
	}

private:
	const HostFileEntry *find(const Common::Path &path) const {
		Common::String name = path.baseName();
		name.toLowercase();
		for (const HostFileEntry &entry : _entries) {
			if (entry.logicalName == name)
				return &entry;
		}
		return nullptr;
	}

	std::vector<HostFileEntry> _entries;
};

} // namespace

struct ScummVmRuntime::Impl {
	HostFilesArchive files;
	bool registered = false;

	explicit Impl(const GameInstallation &installation) {
		if (installation.hasXeen())
			files.add("xeen.cc", installation.xeenArchive);
		if (installation.hasDarkside())
			files.add("dark.cc", installation.darkArchive);

		SearchMan.add("mmodern_input", &files, 100, false);
		registered = true;
	}

	~Impl() {
		if (registered)
			SearchMan.remove("mmodern_input");
	}
};

ScummVmRuntime::ScummVmRuntime(const GameInstallation &installation) :
	_impl(new Impl(installation)) {
}

ScummVmRuntime::~ScummVmRuntime() = default;

} // namespace mmodern
