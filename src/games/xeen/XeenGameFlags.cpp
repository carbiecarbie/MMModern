#include "games/xeen/XeenGameFlags.h"

#include <stdexcept>
#include <utility>

namespace mmodern {

XeenGameFlags::XeenGameFlags(Storage flags) : _flags(std::move(flags)) {
}

std::size_t XeenGameFlags::checkedIndex(int index) {
	if (index < 0 || index >= static_cast<int>(kCount))
		throw std::out_of_range("indice de game flag de Clouds fora de 0..255");
	return static_cast<std::size_t>(index);
}

bool XeenGameFlags::isSet(int index) const {
	return _flags[checkedIndex(index)];
}

void XeenGameFlags::set(int index) {
	_flags[checkedIndex(index)] = true;
}

void XeenGameFlags::clear(int index) {
	_flags[checkedIndex(index)] = false;
}

} // namespace mmodern
