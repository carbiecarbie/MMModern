#ifndef MMODERN_XEEN_GAMEPLAY_BORROW_H
#define MMODERN_XEEN_GAMEPLAY_BORROW_H
#include <cstdint>
#include <memory>
namespace mmodern {
// Runtime borrow identity belongs to the owner lifetime, never to copied values.
// A lease retains only this control block, so releasing an old borrower never
// dereferences a destroyed owner or clears a new owner at the same address.
// Destruction permanently retires the retained block; dispatch checks it without
// dereferencing owner storage. A copied/reconstructed owner starts a new block.
class XeenGameplayBorrowOwner {
public:
	XeenGameplayBorrowOwner() = default;
	~XeenGameplayBorrowOwner() { if (state) state->alive = false; }
	XeenGameplayBorrowOwner(const XeenGameplayBorrowOwner &) noexcept {}
	XeenGameplayBorrowOwner &operator=(const XeenGameplayBorrowOwner &) noexcept { return *this; }
private:
	friend class XeenWorld;
	friend class XeenRestoreGuard;
	friend class XeenSaveState;
	struct State { std::size_t references = 0; std::uint64_t revision = 0; bool alive = true; };
	mutable std::shared_ptr<State> state;
	std::shared_ptr<State> retain() const {
		if (!state) state = std::make_shared<State>();
		return state;
	}
	bool borrowed() const noexcept { return state && state->references; }
};
}
#endif
