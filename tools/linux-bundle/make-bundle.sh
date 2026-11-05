#!/usr/bin/env bash
# Builds the Linux test bundle of M113: mmodern for x86-64 Linux, built on an
# old distribution (the Ubuntu 22.04 container of
# .github/workflows/linux-bundle.yml) so that it runs on most current ones.
# It is a tester's bundle, not a release package: no original data, no saves.
#
# The bundle holds mmodern linked with -static-libgcc -static-libstdc++ (as
# the Windows executable is) without debug information, the shared
# libraries it needs that a desktop distribution may lack, a start script that
# finds them, the GPL notice with the source commit, the copyright files of the
# bundled libraries and the tester's guide. Its SDL2 is the release that
# build-sdl2.sh installed in <sdl2-prefix>, since Ubuntu 22.04's is too old.
#
# Libraries left to the system: glibc, the graphics driver stack (GL, EGL,
# DRM, GBM), the X11/XCB and Wayland client libraries, the sound servers'
# client libraries (ALSA, PulseAudio, PipeWire) and the D-Bus, systemd and
# zlib libraries every desktop has. They must match the running system, so
# copying an older build of them would do harm. SDL2 loads its X11, Wayland
# and sound backends from the system at run time.
#
# Usage: make-bundle.sh <scummvm-source> <scummvm-build> <sdl2-prefix> <output-dir> <commit>
set -euo pipefail
scummvm_source=$1
scummvm_build=$2
sdl_prefix=$(cd "$3" && pwd -P)
out=$4
commit=$5
repo=$(cd "$(dirname "$0")/../.." && pwd -P)
here="$repo/tools/linux-bundle"

case "$commit" in
	[0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f][0-9a-f]*) ;;
	*) echo "The commit must be a hexadecimal SHA, not '$commit'." >&2; exit 2 ;;
esac
mkdir -p "$out"
out=$(cd "$out" && pwd -P)
build="$out/build"
name="mmodern-linux-test-${commit:0:12}"
stage="$out/$name"
if [ -e "$stage" ] || [ -e "$stage.tar.gz" ]; then
	echo "$stage or its archive already exists." >&2
	exit 1
fi

# The flags of build-rel without -g (assertions kept), plus the static GCC
# runtime, the pinned SDL2 and no run-time search path: mmodern.sh finds lib/.
# Until then the pinned SDL2 is found through LD_LIBRARY_PATH. The bundle
# would strip the debug information anyway, and GCC 11 spends over an hour on
# single units to produce it, beyond the job's time limit (maintainer,
# 2026-11-05).
export LD_LIBRARY_PATH="$sdl_prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cmake -S "$repo" -B "$build" -G Ninja \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	"-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O2" \
	"-DCMAKE_EXE_LINKER_FLAGS=-static-libgcc -static-libstdc++" \
	"-DCMAKE_PREFIX_PATH=$sdl_prefix" \
	-DCMAKE_SKIP_RPATH=ON \
	"-DSCUMMVM_SOURCE_DIR=$scummvm_source" \
	"-DSCUMMVM_BUILD_DIR=$scummvm_build" \
	-DMMODERN_ORIGINAL_DATA_TESTS=OFF
# GCC 11 needs up to about 3.5 GB for some units, so the job count follows
# the available memory as well as the processors.
memory_kib=$(awk '/^MemAvailable:/ { print $2 }' /proc/meminfo)
jobs=$(( memory_kib / (3584 * 1024) ))
[ "$jobs" -le "$(nproc)" ] || jobs=$(nproc)
[ "$jobs" -ge 1 ] || jobs=1
echo "Building with $jobs jobs ($(( memory_kib / 1024 )) MiB available, $(nproc) processors)."
cmake --build "$build" --target mmodern --parallel "$jobs"

mkdir -p "$stage/bin" "$stage/lib" "$stage/licenses"
objcopy --strip-debug "$build/mmodern" "$stage/bin/mmodern"
if ldd "$stage/bin/mmodern" | grep -E 'libstdc\+\+|libgcc_s'; then
	echo "mmodern still needs the shared GCC runtime." >&2
	exit 1
fi
if readelf -d "$stage/bin/mmodern" | grep -E 'RPATH|RUNPATH'; then
	echo "mmodern has a run-time search path." >&2
	exit 1
fi
sdl_library=$(ldd "$stage/bin/mmodern" | awk '$1 ~ /^libSDL2-2\.0\.so/ { print $3 }')
case "$sdl_library" in
	"$sdl_prefix"/lib/*) ;;
	*) echo "mmodern does not use the pinned SDL2 (found '$sdl_library')." >&2; exit 1 ;;
esac

system_library() {
	case "$1" in
		linux-vdso.so*|ld-linux*|libc.so*|libm.so*|libdl.so*|libpthread.so*|librt.so*|libresolv.so*|libutil.so*) return 0 ;;
		libGL.so*|libGLX*|libGLdispatch*|libOpenGL*|libEGL*|libdrm*|libgbm*|libvulkan*) return 0 ;;
		libX11.so*|libX11-xcb.so*|libxcb*|libwayland-*) return 0 ;;
		libasound.so*|libpulse*|libdbus-1.so*|libsystemd.so*|libudev.so*|libz.so*) return 0 ;;
		libgcc_s.so*|libstdc++.so*) return 0 ;;
	esac
	return 1
}

: > "$out/bundled.txt"
: > "$out/system.txt"
ldd "$stage/bin/mmodern" | awk '$2 == "=>" && $3 ~ /^\// { print $1, $3 }' | sort -u |
while read -r soname path; do
	if system_library "$soname"; then
		echo "$soname" >> "$out/system.txt"
		continue
	fi
	cp -L "$path" "$stage/lib/$soname"
	if [ "$path" = "$sdl_library" ]; then
		echo "$soname SDL2 $("$sdl_prefix/bin/sdl2-config" --version) (upstream release, see below)" >> "$out/bundled.txt"
		cp "$sdl_prefix/share/licenses/SDL2/LICENSE.txt" "$stage/licenses/SDL2.LICENSE.txt"
		continue
	fi
	package=$(dpkg-query -S "$(readlink -f "$path")" 2>/dev/null | head -n 1 | cut -d: -f1) ||
		package=$(dpkg-query -S "$path" | head -n 1 | cut -d: -f1)
	version=$(dpkg-query -W -f='${Version}' "$package")
	echo "$soname $package $version" >> "$out/bundled.txt"
	doc="/usr/share/doc/${package%%:*}/copyright"
	if [ ! -f "$doc" ]; then
		echo "$package has no copyright file ($doc), which the bundle must carry." >&2
		exit 1
	fi
	cp "$doc" "$stage/licenses/${package%%:*}.copyright"
done
if [ ! -f "$stage/lib/$(basename "$sdl_library")" ]; then
	echo "The pinned SDL2 is not in the bundle." >&2
	exit 1
fi
for library in "$stage"/lib/*; do
	if readelf -d "$library" | grep -E 'RPATH|RUNPATH'; then
		echo "$(basename "$library") has a run-time search path." >&2
		exit 1
	fi
done
if LD_LIBRARY_PATH="$stage/lib" ldd "$stage/bin/mmodern" | grep 'not found'; then
	echo "A library of the bundle is missing." >&2
	exit 1
fi

install -m 0755 "$here/mmodern.sh" "$stage/mmodern.sh"
cp "$here/GUIDE.txt" "$stage/GUIDE.txt"
cp "$repo/LICENSE" "$stage/LICENSE.txt"
. /etc/os-release
{
	echo "MMModern Linux test bundle (not a release)"
	echo
	echo "MMModern is free software under the GNU General Public License,"
	echo "version 3 or (at your option) any later version; see LICENSE.txt."
	echo "Its complete source is the commit below of"
	echo "https://github.com/carbiecarbie/MMModern; the reused ScummVM code is"
	echo "the pinned commit of https://github.com/scummvm/scummvm.git."
	echo
	echo "MMModern commit: $commit"
	echo "ScummVM commit: $(git -C "$scummvm_source" rev-parse HEAD)"
	echo "Built on: $PRETTY_NAME, $(gcc --version | head -n 1)"
	echo "Linked statically: libgcc, libstdc++ (GCC Runtime Library Exception)"
	echo
	echo "Bundled libraries (lib/); their copyright files are in licenses/."
	echo "The sources of the Ubuntu packages are the Ubuntu source packages of"
	echo "these versions:"
	sed 's/^/  /' "$out/bundled.txt"
	echo
	cat "$sdl_prefix/SOURCE-SDL2.txt"
	echo
	echo "Taken from the system: $(tr '\n' ' ' < "$out/system.txt")"
} > "$stage/SOURCE.txt"

# Without the build's LD_LIBRARY_PATH, so only lib/ can provide SDL2.
env -u LD_LIBRARY_PATH "$stage/mmodern.sh" --version
tar -C "$out" -czf "$out/$name.tar.gz" "$name"
(cd "$out" && sha256sum "$name.tar.gz" | tee "$name.tar.gz.sha256")
echo "Bundle: $out/$name.tar.gz"
cat "$stage/SOURCE.txt"
