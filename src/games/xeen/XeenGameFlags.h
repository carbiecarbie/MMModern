#ifndef MMODERN_GAMES_XEEN_XEEN_GAME_FLAGS_H
#define MMODERN_GAMES_XEEN_XEEN_GAME_FLAGS_H

#include <array>
#include <cstddef>

namespace mmodern {

class XeenGameFlags {
public:
	static constexpr std::size_t kCount = 256;
	using Storage = std::array<bool, kCount>;

	XeenGameFlags() = default;
	explicit XeenGameFlags(Storage flags);

	bool isSet(int index) const;
	void set(int index);
	void clear(int index);

	const Storage &values() const { return _flags; }

private:
	static std::size_t checkedIndex(int index);

	Storage _flags{};
};

} // namespace mmodern

#endif
