#ifndef MMODERN_XEEN_TEMPLE_PRICE_STATE_H
#define MMODERN_XEEN_TEMPLE_PRICE_STATE_H
#include "games/xeen/XeenTempleHeal.h"
namespace mmodern {
class XeenTemplePriceState {
public:
    XeenTemplePriceState() = default;
    XeenTemplePriceState(const XeenTemplePriceState &) = delete;
    XeenTemplePriceState &operator=(const XeenTemplePriceState &) = delete;
    const XeenTempleSurcharges &row() const noexcept { return _row; }
    std::uint64_t revision() const noexcept { return _revision; }
    XeenTempleSurcharges prepare(XeenMapIdentity) const;
    void checkPublication(std::uint64_t base) const;
    void checkVisit(const XeenTempleSurcharges &,std::uint64_t revision) const;
private:
    friend class XeenEncounterFlow;
    friend struct XeenTemplePriceStateTestAccess;
    // Caller checks capacity and base revision before acquiring the Service lease.
    void publish(const XeenTempleSurcharges &row) noexcept { _row=row;++_revision; }
    XeenTempleSurcharges _row{};
    std::uint64_t _revision=0;
};
}
#endif
