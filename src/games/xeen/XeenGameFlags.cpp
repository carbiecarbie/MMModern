#include "games/xeen/XeenGameFlags.h"

#include <stdexcept>
#include <utility>

namespace mmodern {

XeenGameFlags::XeenGameFlags(Storage flags) : _flags(std::move(flags)) {
}

std::size_t XeenGameFlags::checkedIndex(int index) {
	if (index < 0 || index >= static_cast<int>(kCount))
		throw std::out_of_range("Game flag index outside 0..255");
	return static_cast<std::size_t>(index);
}

std::size_t XeenGameFlags::checkedIndex(XeenSide side, int index) {
	if (side!=XeenSide::Clouds && side!=XeenSide::Darkside) throw std::out_of_range("Invalid game-flag side");
	return static_cast<unsigned>(side)*kCount+checkedIndex(index);
}
bool XeenGameFlags::isSet(XeenSide side, int index) const {return _flags[checkedIndex(side,index)];}
void XeenGameFlags::set(XeenSide side, int index) {const auto slot=checkedIndex(side,index);XeenMutationWatch::write(this);_flags[slot]=true;}
void XeenGameFlags::clear(XeenSide side, int index) {const auto slot=checkedIndex(side,index);XeenMutationWatch::write(this);_flags[slot]=false;}

bool XeenGameFlags::isSet(int index) const {
	return _flags[checkedIndex(index)];
}

void XeenGameFlags::set(int index) {
	const auto slot = checkedIndex(index);
	XeenMutationWatch::write(this);
	_flags[slot] = true;
}

void XeenGameFlags::clear(int index) {
	const auto slot = checkedIndex(index);
	XeenMutationWatch::write(this);
	_flags[slot] = false;
}

} // namespace mmodern
