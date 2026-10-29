#ifndef MMODERN_XEEN_GAMEPLAY_CONTEXT_FORMAT_H
#define MMODERN_XEEN_GAMEPLAY_CONTEXT_FORMAT_H
#include "games/xeen/XeenGameplayContext.h"
#include <vector>
namespace mmodern {
class XeenGameplayContextFormat {
public:
	// Explicit opt-in; no shop/economy interpretation and no diagnostic admission here.
	static XeenGameplayContext parse(const std::vector<std::uint8_t> &pty);
	static void writeDos(std::vector<std::uint8_t> &pty, const XeenGameplayContext &);
};
}
#endif
