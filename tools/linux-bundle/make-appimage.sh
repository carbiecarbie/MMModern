#!/usr/bin/env bash
# Build the x86-64 AppImage in the pinned Ubuntu 22.04 CI container.
# System libraries retain M113's selection: glibc, graphics drivers, X11/XCB,
# Wayland, sound-server clients, D-Bus, systemd and zlib belong to the host.
# SDL2 is built by build-sdl2.sh, with dynamically loaded host backends.
#
# Usage: make-appimage.sh <scummvm-source> <scummvm-build> <sdl2-prefix> <output-dir> <commit>
# Local fallback only: append --from-build <build-linux-rel> to package an
# existing build, configured with the same static GCC runtime and RPATH below.
set -euo pipefail
[ $# = 5 ] || [ $# = 7 ] || { echo "Expected five arguments, optionally --from-build <build>." >&2; exit 2; }
scummvm_source=$(realpath "$1")
scummvm_build=$(realpath "$2")
sdl_prefix=$(realpath "$3")
commit=$5
repo=$(cd "$(dirname "$0")/../.." && pwd -P)
here="$repo/tools/linux-bundle"
[[ "$commit" =~ ^[0-9a-f]{40}$ ]] || { echo 'The commit must be a full hexadecimal SHA.' >&2; exit 2; }
mkdir -p "$4"
out=$(realpath "$4")
build="$out/build"
if [ $# = 7 ]; then
    [ "$6" = --from-build ] || exit 2
    build=$(realpath "$7")
else
    # DT_RPATH deliberately provides transitive lookup for vorbisfile's own
    # dependencies. DT_RUNPATH alone would not find those in usr/lib, and
    # libraries themselves must have no search path.
    cmake -S "$repo" -B "$build" -G Ninja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        '-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O2' \
        '-DCMAKE_EXE_LINKER_FLAGS=-static-libgcc -static-libstdc++ -Wl,--disable-new-dtags' \
        "-DCMAKE_PREFIX_PATH=$sdl_prefix" \
        -DCMAKE_SKIP_RPATH=OFF \
        -DCMAKE_SKIP_INSTALL_RPATH=OFF \
        -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
        '-DCMAKE_INSTALL_RPATH=$ORIGIN/../lib' \
        "-DSCUMMVM_SOURCE_DIR=$scummvm_source" \
        "-DSCUMMVM_BUILD_DIR=$scummvm_build" \
        -DMMODERN_ORIGINAL_DATA_TESTS=OFF
    memory_kib=$(awk '/^MemAvailable:/ { print $2 }' /proc/meminfo)
    jobs=$(( memory_kib / (3584 * 1024) ))
    [ "$jobs" -le "$(nproc)" ] || jobs=$(nproc)
    [ "$jobs" -ge 1 ] || jobs=1
    echo "Building with $jobs jobs ($(( memory_kib / 1024 )) MiB available)."
    cmake --build "$build" --target mmodern --parallel "$jobs"
fi

stage="$out/MMModern.AppDir"
[ ! -e "$stage" ] || { echo "$stage already exists." >&2; exit 1; }
doc="$stage/usr/share/doc/mmodern"
mkdir -p "$stage/usr/bin" "$stage/usr/lib" "$doc/licenses" \
    "$stage/usr/share/applications" "$stage/usr/share/icons/hicolor/512x512/apps"
objcopy --strip-debug "$build/mmodern" "$stage/usr/bin/mmodern"
dynamic=$(readelf -d "$stage/usr/bin/mmodern")
paths=$(printf '%s\n' "$dynamic" | sed -n '/(RPATH)\|(RUNPATH)/s/.*\[\(.*\)\].*/\1/p')
[ "$paths" = '$ORIGIN/../lib' ] && [[ "$dynamic" == *'(RPATH)'* ]] || {
    echo 'mmodern must have exactly the transitive $ORIGIN/../lib RPATH.' >&2; exit 1;
}
# Inspect before staging with the ELF loader's explicit search argument;
# no environment variable is changed, including in AppRun or its children.
loader=$(readelf -l "$build/mmodern" | sed -n 's/.*interpreter: \(.*\)]/\1/p')
"$loader" --library-path "$sdl_prefix/lib" --list "$build/mmodern" > "$out/linked.txt"
if grep -E 'libstdc\+\+|libgcc_s|not found' "$out/linked.txt"; then
    echo 'mmodern needs a missing library or the shared GCC runtime.' >&2; exit 1
fi
[ "$("$sdl_prefix/bin/sdl2-config" --version)" = 2.32.10 ]
sdl_library=$(awk '$1 ~ /^libSDL2-2\.0\.so/ { print $3 }' "$out/linked.txt")
case "$sdl_library" in
    "$sdl_prefix"/lib/*) ;;
    *) echo "mmodern does not use the pinned SDL2 ($sdl_library)." >&2; exit 1 ;;
esac
system_library() {
    case "$1" in
        linux-vdso.so*|ld-linux*|libc.so*|libm.so*|libmvec.so*|libdl.so*|libpthread.so*|librt.so*|libresolv.so*|libutil.so*) return 0 ;;
        libGL.so*|libGLX*|libGLdispatch*|libOpenGL*|libEGL*|libdrm*|libgbm*|libvulkan*) return 0 ;;
        libX11.so*|libX11-xcb.so*|libxcb*|libwayland-*) return 0 ;;
        libasound.so*|libpulse*|libdbus-1.so*|libsystemd.so*|libudev.so*|libz.so*) return 0 ;;
        libgcc_s.so*|libstdc++.so*) return 0 ;;
    esac
    return 1
}
: > "$out/bundled.txt"
: > "$out/system.txt"
awk '$2 == "=>" && $3 ~ /^\// { print $1, $3 }' "$out/linked.txt" | sort -u |
while read -r soname path; do
    if system_library "$soname"; then
        echo "$soname" >> "$out/system.txt"
        continue
    fi
    cp -L "$path" "$stage/usr/lib/$soname"
    if [ "$path" = "$sdl_library" ]; then
        echo "$soname SDL2 2.32.10 (upstream release, see below)" >> "$out/bundled.txt"
        cp "$sdl_prefix/share/licenses/SDL2/LICENSE.txt" "$doc/licenses/SDL2.LICENSE.txt"
        continue
    fi
    package=$(dpkg-query -S "$(readlink -f "$path")" 2>/dev/null | head -n 1 | cut -d: -f1) ||
        package=$(dpkg-query -S "$path" | head -n 1 | cut -d: -f1)
    version=$(dpkg-query -W -f='${Version}' "$package")
    echo "$soname $package $version" >> "$out/bundled.txt"
    cp "/usr/share/doc/${package%%:*}/copyright" "$doc/licenses/${package%%:*}.copyright"
done
[ -f "$stage/usr/lib/$(basename "$sdl_library")" ]
for library in "$stage"/usr/lib/*; do
    if readelf -d "$library" | grep -E 'RPATH|RUNPATH'; then
        echo "$(basename "$library") has a run-time search path." >&2; exit 1
    fi
done

install -m 0755 "$here/AppRun" "$stage/AppRun"
install -m 0644 "$here/mmodern.desktop" "$stage/usr/share/applications/mmodern.desktop"
install -m 0644 "$here/mmodern.png" "$stage/usr/share/icons/hicolor/512x512/apps/mmodern.png"
ln -s usr/share/applications/mmodern.desktop "$stage/mmodern.desktop"
ln -s usr/share/icons/hicolor/512x512/apps/mmodern.png "$stage/mmodern.png"
ln -s mmodern.png "$stage/.DirIcon"
desktop-file-validate "$stage/mmodern.desktop"
cp "$repo/LICENSE" "$doc/LICENSE.txt"
cp "$here"/licenses/* "$doc/licenses/"
gcc_notice="/usr/share/doc/gcc-$(gcc -dumpversion | cut -d. -f1)-base/copyright"
cp "$gcc_notice" "$doc/licenses/GCC.copyright"

# Official versioned release assets, never continuous. --runtime-file prevents
# appimagetool's own default network download of an unpinned runtime.
tool_version=1.9.1
tool_sha256=ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0
runtime_version=20251108
runtime_sha256=2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d
tool_url="https://github.com/AppImage/appimagetool/releases/download/$tool_version/appimagetool-x86_64.AppImage"
runtime_url="https://github.com/AppImage/type2-runtime/releases/download/$runtime_version/runtime-x86_64"
fetch() {
    local url=$1 dest=$2 sha=$3
    [ -f "$dest" ] || curl -fsSL --retry 3 "$url" -o "$dest"
    echo "$sha  $dest" | sha256sum -c -
}
mkdir -p "$out/tools"
fetch "$tool_url" "$out/tools/appimagetool.AppImage" "$tool_sha256"
fetch "$runtime_url" "$out/tools/runtime-x86_64" "$runtime_sha256"
chmod +x "$out/tools/appimagetool.AppImage"

# The staged binary must resolve all its libraries without any loader hint.
env -u LD_LIBRARY_PATH ldd "$stage/usr/bin/mmodern" > "$out/staged-linked.txt"
if grep -E 'not found|libstdc\+\+|libgcc_s' "$out/staged-linked.txt"; then
    echo 'A staged library is missing or the shared GCC runtime is required.' >&2; exit 1
fi
staged_sdl=$(awk '$1 ~ /^libSDL2-2\.0\.so/ { print $3 }' "$out/staged-linked.txt")
[ "$(realpath "$staged_sdl")" = "$stage/usr/lib/$(basename "$sdl_library")" ] || {
    echo 'The staged executable did not resolve the bundled SDL2.' >&2; exit 1;
}
version=$(env -u LD_LIBRARY_PATH "$stage/AppRun" --version)
[[ "$version" =~ ^MMModern\ ([0-9]+\.[0-9]+\.[0-9]+)$ ]] || { echo "Invalid version: $version" >&2; exit 1; }
version=${BASH_REMATCH[1]}
image="$out/MMModern-$version-x86_64.AppImage"
[ ! -e "$image" ] || { echo "$image already exists." >&2; exit 1; }
. /etc/os-release
{
    echo "MMModern $version AppImage (x86-64)"
    echo 'MMModern: GPL-3.0-or-later; see LICENSE.txt.'
    echo 'CRT shader adapted from zfast_crt_standard, copyright 2017 Greg Hogan (SoltanGris42), GPL-2.0-or-later.'
    echo 'Shader source: https://github.com/libretro/glsl-shaders/blob/a35069864b4bf30f8c8e740d57206543fb7e745e/crt/shaders/zfast_crt.glsl'
    echo "Complete MMModern source: https://github.com/carbiecarbie/MMModern/tree/$commit"
    echo "MMModern commit: $commit (local builds may contain uncommitted changes)"
    echo "ScummVM source: https://github.com/scummvm/scummvm/tree/$(git -C "$scummvm_source" rev-parse HEAD)"
    echo "Built on: $PRETTY_NAME; $(gcc --version | head -n 1)"
    echo 'Statically linked GCC libgcc/libstdc++: GPL-3.0 with GCC Runtime Library Exception 3.1; see licenses/GCC.copyright.'
    echo 'Bundled libraries (usr/lib); copyright notices in licenses/; source packages at https://packages.ubuntu.com/:'
    sed 's/^/  /' "$out/bundled.txt"
    cat "$sdl_prefix/SOURCE-SDL2.txt"
    echo 'SDL2 license: zlib; see licenses/SDL2.LICENSE.txt.'
    echo "System libraries: $(tr '\n' ' ' < "$out/system.txt")"
    echo
    echo "Build tool: appimagetool $tool_version, MIT (not included in the payload)"
    echo "  $tool_url"
    echo "  SHA-256 $tool_sha256"
    echo "Embedded type-2 runtime $runtime_version, MIT"
    echo "  $runtime_url"
    echo "  SHA-256 $runtime_sha256"
    echo "Runtime source and dependency build recipes: https://github.com/AppImage/type2-runtime/tree/$runtime_version"
    echo 'Runtime static dependencies: musl libc (MIT and BSD notices), libfuse (LGPL-2.1),'
    echo '  squashfuse (BSD-2-Clause), zstd (BSD-3-Clause), zlib (zlib), mimalloc (MIT).'
    echo 'Their complete notices and upstream source locations are in licenses/.'
    echo 'Notice sources (license texts also shipped in licenses/):'
    echo '  https://git.musl-libc.org/cgit/musl/tree/COPYRIGHT?h=v1.2.5'
    echo '  https://github.com/libfuse/libfuse/blob/fuse-3.15.0/LGPL2.txt'
    echo '  https://github.com/vasi/squashfuse/blob/0.5.2/LICENSE'
    echo '  https://github.com/facebook/zstd/blob/v1.5.7/LICENSE'
    echo '  https://github.com/madler/zlib/blob/v1.3.1/LICENSE'
    echo '  https://github.com/microsoft/mimalloc/blob/v3.1.5/LICENSE'
    echo 'Project icon: MMModern-owned artwork, GPL-3.0-or-later; SVG in tools/linux-bundle/mmodern.svg.'
} > "$doc/SOURCE.txt"

ARCH=x86_64 "$out/tools/appimagetool.AppImage" --appimage-extract-and-run \
    --runtime-file "$out/tools/runtime-x86_64" "$stage" "$image"
# Exercise the actual downloadable artifact in a FUSE-less CI container.
env -u LD_LIBRARY_PATH "$image" --appimage-extract-and-run --version
mkdir "$out/empty-game"
env -u LD_LIBRARY_PATH "$image" --appimage-extract-and-run --diagnostics "$out/empty-game" > "$out/diagnostics.txt"
grep -E '^game.status: unusable:' "$out/diagnostics.txt"
(cd "$out" && sha256sum "$(basename "$image")" | tee "$(basename "$image").sha256")
echo "AppImage: $image"
