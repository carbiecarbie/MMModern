#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include "compat/scummvm/ScummVmPictureFilter.h"
#include "graphics/pixelformat.h"
#include "graphics/scaler/edge.h"
#include "graphics/scaler/hq.h"
#include "graphics/scaler/pm.h"
#include "graphics/scaler/sai.h"
#include "graphics/scaler/scalebit.h"

#include <algorithm>

namespace mmodern {
namespace {

enum class Kind { AdvMame, Hq, Edge, Sai, SuperSai, SuperEagle, Pm };
struct Entry { std::string_view name; Kind kind; int factor; };
// The pin's factors; extraPixels() is at most 4 (AdvMame).
constexpr Entry entries[] = {
	{"advmame2x", Kind::AdvMame, 2}, {"advmame3x", Kind::AdvMame, 3}, {"advmame4x", Kind::AdvMame, 4},
	{"hq2x", Kind::Hq, 2}, {"hq3x", Kind::Hq, 3}, {"edge2x", Kind::Edge, 2}, {"edge3x", Kind::Edge, 3},
	{"sai2x", Kind::Sai, 2}, {"supersai2x", Kind::SuperSai, 2}, {"supereagle2x", Kind::SuperEagle, 2},
	{"pm2x", Kind::Pm, 2}};
constexpr int border = 4;

const Entry *find(std::string_view name) {
	for (const auto &entry : entries)
		if (entry.name == name) return &entry;
	return nullptr;
}

Scaler *create(Kind kind, const Graphics::PixelFormat &format) {
	switch (kind) {
	case Kind::AdvMame: return new AdvMameScaler(format);
	case Kind::Hq: return new HQScaler(format);
	case Kind::Edge: return new EdgeScaler(format);
	case Kind::Sai: return new SAIScaler(format);
	case Kind::SuperSai: return new SuperSAIScaler(format);
	case Kind::SuperEagle: return new SuperEagleScaler(format);
	case Kind::Pm: return new PMScaler(format);
	}
	return nullptr;
}

} // namespace

struct ScummVmPictureFilter::State {
	const Entry *entry = nullptr;
	std::unique_ptr<Scaler> scaler;
	std::vector<std::uint32_t> padded;
};

ScummVmPictureFilter::ScummVmPictureFilter() : _state(new State) {}
ScummVmPictureFilter::~ScummVmPictureFilter() = default;

int ScummVmPictureFilter::factor(std::string_view scaler) {
	const auto *entry = find(scaler);
	return entry ? entry->factor : 0;
}

bool ScummVmPictureFilter::apply(std::string_view name, const std::uint32_t *frame, int width, int height,
		std::vector<std::uint32_t> &output) {
	const auto *entry = find(name);
	if (!entry || !frame || width <= 0 || height <= 0) return false;
	auto &state = *_state;
	if (state.entry != entry) {
		// SDL_PIXELFORMAT_ARGB8888.
		const Graphics::PixelFormat argb(4, 8, 8, 8, 8, 16, 8, 0, 24);
		state.scaler.reset(create(entry->kind, argb));
		state.scaler->setFactor(entry->factor);
		state.entry = entry;
	}
	const int pitch = width + 2 * border;
	state.padded.resize(static_cast<std::size_t>(pitch) * (height + 2 * border));
	for (int y = -border; y < height + border; ++y) {
		const auto *row = frame + static_cast<std::size_t>(std::clamp(y, 0, height - 1)) * width;
		auto *out = state.padded.data() + static_cast<std::size_t>(y + border) * pitch;
		std::fill(out, out + border, row[0]);
		std::copy(row, row + width, out + border);
		std::fill(out + border + width, out + pitch, row[width - 1]);
	}
	const int f = entry->factor;
	output.resize(static_cast<std::size_t>(width) * f * height * f);
	const auto *origin = state.padded.data() + static_cast<std::size_t>(border) * pitch + border;
	state.scaler->scale(reinterpret_cast<const uint8 *>(origin), pitch * 4,
		reinterpret_cast<uint8 *>(output.data()), width * f * 4, width, height, 0, 0);
	return true;
}

} // namespace mmodern
