#ifndef MMODERN_GAMES_XEEN_XEEN_GAME_FLAGS_H
#define MMODERN_GAMES_XEEN_XEEN_GAME_FLAGS_H

#include <array>
#include "games/xeen/XeenMapIdentity.h"
#include <cstddef>
#include "games/xeen/XeenGameplayBorrow.h"
#include "games/xeen/XeenMutation.h"

namespace mmodern {

class XeenGameFlags {
public:
	static constexpr std::size_t kCount = 256;
	static constexpr std::size_t kStorageCount = 2 * kCount;
	using Storage = std::array<bool, kStorageCount>;

	XeenGameFlags() = default;
	XeenGameFlags(const XeenGameFlags &) = default;
	XeenGameFlags &operator=(const XeenGameFlags &other) noexcept { XeenMutationWatch::write(this);_flags=other._flags;return *this; }
	explicit XeenGameFlags(Storage flags);
	explicit XeenGameFlags(std::array<bool,kCount> flags) {for(std::size_t i=0;i<kCount;++i)_flags[i]=flags[i];}

	// Single-index helpers select the Clouds bank (explicit Clouds rules only).
	bool isSet(int index) const;
	bool isSet(XeenSide side, int index) const;
	void set(XeenSide side, int index);
	void clear(XeenSide side, int index);
	void set(int index);
	void clear(int index);

	const XeenMutableArray<bool,kStorageCount> &values() const { return _flags; }

private:
	friend class XeenWorld;
	friend class XeenRestoreGuard;
	friend class XeenSaveState;
	XeenGameplayBorrowOwner _gameplayBorrow;
	static std::size_t checkedIndex(int index);
	static std::size_t checkedIndex(XeenSide side, int index);

	XeenMutableArray<bool,kStorageCount> _flags{};
};

} // namespace mmodern

#endif
