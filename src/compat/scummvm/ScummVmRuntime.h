#ifndef MMODERN_COMPAT_SCUMMVM_RUNTIME_H
#define MMODERN_COMPAT_SCUMMVM_RUNTIME_H

#include "core/GameInstallation.h"

#include <memory>

namespace mmodern {

class ScummVmRuntime {
public:
	explicit ScummVmRuntime(const GameInstallation &installation);
	~ScummVmRuntime();

	ScummVmRuntime(const ScummVmRuntime &) = delete;
	ScummVmRuntime &operator=(const ScummVmRuntime &) = delete;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
};

} // namespace mmodern

#endif
