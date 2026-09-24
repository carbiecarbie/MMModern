#ifndef MMODERN_GAMES_XEEN_XEEN_GAME_FLAGS_H
#define MMODERN_GAMES_XEEN_XEEN_GAME_FLAGS_H

#include <array>
#include <cstddef>
#include "games/xeen/XeenGameplayBorrow.h"
#include "games/xeen/XeenMutation.h"

namespace mmodern {

class XeenGameFlags {
public:
	static constexpr std::size_t kCount = 256;
	using Storage = std::array<bool, kCount>;

	XeenGameFlags() = default;
	XeenGameFlags(const XeenGameFlags &) = default;
	XeenGameFlags &operator=(const XeenGameFlags &other) noexcept { XeenMutationWatch::write(this);_flags=other._flags;return *this; }
	explicit XeenGameFlags(Storage flags);

	bool isSet(int index) const;
	void set(int index);
	void clear(int index);

	const XeenMutableArray<bool,kCount> &values() const { return _flags; }

private:
	friend class XeenWorld;
	friend class XeenRestoreGuard;
	friend class XeenSaveState;
	XeenGameplayBorrowOwner _gameplayBorrow;
	static std::size_t checkedIndex(int index);

	XeenMutableArray<bool,kCount> _flags{};
};

} // namespace mmodern

#endif
