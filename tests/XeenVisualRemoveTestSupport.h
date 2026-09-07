#ifndef MMODERN_VISUAL_REMOVE_TEST_SUPPORT_H
#define MMODERN_VISUAL_REMOVE_TEST_SUPPORT_H
#include "XeenRemoveTestSupport.h"
#include "app/XeenEventFlow.h"
#include <filesystem>
#include <fstream>

namespace visual_remove_test {
using namespace mmodern;
using remove_test::check;
inline void save(const IndexedFrame &f, const std::filesystem::path &path) {
	check(f.width == 320 && f.height == 200, "native frame dimensions");
	std::ofstream out(path, std::ios::binary);
	auto u16=[&](unsigned n){out.put(n&255);out.put((n>>8)&255);};
	auto u32=[&](unsigned n){u16(n&65535);u16(n>>16);};
	out.put('B');out.put('M');u32(54+320*200*3);u32(0);u32(54);
	u32(40);u32(320);u32(200);u16(1);u16(24);u32(0);u32(320*200*3);u32(0);u32(0);u32(0);u32(0);
	for(int y=199;y>=0;--y)for(int x=0;x<320;++x){auto p=f.pixels[y*320+x]*3;out.put(f.palette[p+2]);out.put(f.palette[p+1]);out.put(f.palette[p]);}
	check(bool(out),"frame output failed");
}
// Only the original interpreter's initial-line hook is used by the real
// checkpoint. Result handling/composition is the same acceptManual as gameplay.
inline IndexedFrame checkpoint(XeenEventFlow &flow, XeenEventExecutionStepResult result,
		XeenCamera &camera, XeenGameFlags &flags) {
	if (const auto *e = std::get_if<XeenEventExecutionError>(&result)) return flow.acceptManual(*e);
	if (const auto *s = std::get_if<XeenEventExecutionSuspended>(&result)) return flow.acceptManual(*s);
	const auto &done=std::get<XeenEventExecutionCompleted>(result);
	const bool changed=camera.mapId!=done.finalCamera.mapId || camera.x!=done.finalCamera.x ||
		camera.y!=done.finalCamera.y || camera.direction!=done.finalCamera.direction;
	const bool flagChanged=flags.values()!=done.finalGameFlags.values();
	camera=done.finalCamera; flags=done.finalGameFlags;
	return flow.acceptManual(XeenManualEventCompleted{done.instructionCount,changed,flagChanged});
}
}
#endif
