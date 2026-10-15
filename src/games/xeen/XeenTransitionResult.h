#ifndef MMODERN_XEEN_TRANSITION_RESULT_H
#define MMODERN_XEEN_TRANSITION_RESULT_H
#include "games/xeen/XeenNavigation.h"
#include <string>
namespace mmodern {
// Admission is distinct from integrity. Unavailable travel is an ordinary
// refusal: discard unpublished candidate values, retaining published prefixes.
struct XeenTransitionResult {
    enum class Status { Available, Unavailable };
    Status status = Status::Unavailable;
    XeenCamera destination;
    std::string reason;
    explicit operator bool() const noexcept { return status == Status::Available; }
};
}
#endif
