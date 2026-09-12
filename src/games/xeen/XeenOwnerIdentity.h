#ifndef MMODERN_XEEN_OWNER_IDENTITY_H
#define MMODERN_XEEN_OWNER_IDENTITY_H
#include <atomic>
#include <cstdint>
#include <limits>
#include <stdexcept>
namespace mmodern {
inline std::uint64_t xeenNextOwnerIdentity() {
	static std::atomic<std::uint64_t> next{1};
	auto value = next.load(std::memory_order_relaxed);
	for (;;) {
		if (value == std::numeric_limits<std::uint64_t>::max())
			throw std::overflow_error("party owner identity exhausted");
		if (next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) return value;
	}
}
}
#endif
