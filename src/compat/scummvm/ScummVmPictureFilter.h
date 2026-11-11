#ifndef MMODERN_COMPAT_SCUMMVM_PICTURE_FILTER_H
#define MMODERN_COMPAT_SCUMMVM_PICTURE_FILTER_H

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mmodern {

// M116 opt-in picture filter (approved deviation dev-picture-filter): the
// pinned ScummVM scalers, created directly without the plugin manager.
class ScummVmPictureFilter {
public:
	ScummVmPictureFilter();
	~ScummVmPictureFilter();
	ScummVmPictureFilter(const ScummVmPictureFilter &) = delete;
	ScummVmPictureFilter &operator=(const ScummVmPictureFilter &) = delete;

	// The factor of a scaler name ("advmame2x" ... "pm2x"); 0 for an unknown name.
	static int factor(std::string_view scaler);
	// Scales a width x height ARGB8888 frame into factor * width x factor * height.
	// The frame's edge pixels are repeated into the border the scalers read.
	// Returns false (output untouched) for an unknown name or an empty frame.
	bool apply(std::string_view scaler, const std::uint32_t *frame, int width, int height,
		std::vector<std::uint32_t> &output);

private:
	struct State;
	std::unique_ptr<State> _state;
};

} // namespace mmodern

#endif
