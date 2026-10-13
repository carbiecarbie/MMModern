#ifndef MMODERN_CORE_PRESENTATION_CLOCK_H
#define MMODERN_CORE_PRESENTATION_CLOCK_H
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>

namespace mmodern {
// Counter conversion only. Flows retain their independent reset points and work.
// DOS INT 1Ah: DAT 0x440cd..0x44144; draw3d: 0x10bcb/0x11873..0x1187b.
struct PresentationClock {
    using Source = std::function<std::uint64_t()>; // Absolute monotonic nanoseconds.
    static constexpr std::uint64_t scale = 10000000000000ULL;
    static constexpr std::uint64_t biosRate = 182065; // 18.2065 Hz, not 50 ms.
    static std::uint64_t checkedAdd(std::uint64_t a,std::uint64_t b) {
        if(b>std::numeric_limits<std::uint64_t>::max()-a)throw std::overflow_error("Presentation clock exhausted");
        return a+b;
    }
    static std::uint64_t fromMilliseconds(std::uint64_t ms) {
        if(ms>std::numeric_limits<std::uint64_t>::max()/1000000)throw std::overflow_error("Presentation clock exhausted");
        return ms*1000000;
    }
    static std::uint64_t tick(std::uint64_t ns) noexcept {
        return (ns/scale)*biosRate+(ns%scale)*biosRate/scale;
    }
    static std::uint64_t boundary(std::uint64_t tick) {
        const auto whole=tick/biosRate, fraction=tick%biosRate;
        if(whole>std::numeric_limits<std::uint64_t>::max()/scale)throw std::overflow_error("Presentation clock exhausted");
        return checkedAdd(whole*scale,(fraction*scale+biosRate-1)/biosRate);
    }
    // Reset samples the current integer counter, without aligning the reset.
    static std::uint64_t after(std::uint64_t ns,unsigned ticks) {
        return boundary(checkedAdd(tick(ns),ticks));
    }
    static std::uint64_t ceilMilliseconds(std::uint64_t ns) noexcept {
        return ns/1000000+(ns%1000000!=0);
    }
    static std::uint64_t afterMilliseconds(std::uint64_t ms,unsigned ticks) {
        return ceilMilliseconds(after(fromMilliseconds(ms),ticks));
    }
    // Separate VGA mode-13 retrace timeline. No BIOS or host-monitor conversion.
    static std::uint64_t retraceAfter(std::uint64_t ns) {
        const auto second=ns/1000000000, remainder=ns%1000000000;
        const auto phase=remainder*70/1000000000+1;
        return checkedAdd(second*1000000000,(phase*1000000000+69)/70);
    }
    static std::uint64_t retraceAfterMilliseconds(std::uint64_t ms) {
        return ceilMilliseconds(retraceAfter(fromMilliseconds(ms)));
    }
};
}
#endif
