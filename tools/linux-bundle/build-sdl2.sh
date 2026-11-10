#!/usr/bin/env bash
# Builds the SDL2 release that the Linux AppImage ships
# (.github/workflows/linux-appimage.yml). Ubuntu 22.04's SDL2 2.0.20 lacks
# SDL_GetDefaultAudioInfo (it needs 2.24 or later), so the bundle job builds
# the version the Windows build and CI use from the pinned upstream release
# tarball, after checking its SHA-256 (maintainer, 2026-11-04).
#
# SDL2 is configured with its X11, Wayland, PulseAudio, PipeWire and ALSA
# backends, each loaded at run time from the running system as distribution
# builds do; the script fails when one of them, OpenGL/EGL or the Linux
# joystick driver (game controllers, M115) is not enabled. The -dev
# packages they need are pinned in
# .github/ci/linux-bundle-packages.txt. The installed prefix gets
# SOURCE-SDL2.txt, which make-appimage.sh copies into the AppImage's SOURCE.txt.
#
# Usage: build-sdl2.sh <work-dir> <install-prefix>
set -euo pipefail
version=2.32.10
sha256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
url="https://github.com/libsdl-org/SDL/releases/download/release-$version/SDL2-$version.tar.gz"
work=$1
prefix=$2

mkdir -p "$work"
work=$(cd "$work" && pwd -P)
tarball="$work/SDL2-$version.tar.gz"
curl -fsSL --retry 3 -o "$tarball" "$url"
echo "$sha256  $tarball" | sha256sum -c -
rm -rf "$work/src" "$work/build"
mkdir "$work/src"
tar -C "$work/src" -xzf "$tarball"
source="$work/src/SDL2-$version"

cmake -S "$source" -B "$work/build" -G Ninja \
	-DCMAKE_BUILD_TYPE=Release \
	"-DCMAKE_INSTALL_PREFIX=$prefix" \
	-DCMAKE_INSTALL_LIBDIR=lib \
	-DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF \
	-DSDL_X11=ON -DSDL_WAYLAND=ON \
	-DSDL_PULSEAUDIO=ON -DSDL_PIPEWIRE=ON -DSDL_ALSA=ON
cmake --build "$work/build"
cmake --install "$work/build"

# SDL2's configure step silently drops a backend whose packages are missing.
config="$prefix/include/SDL2/SDL_config.h"
missing=0
for define in \
	SDL_VIDEO_DRIVER_X11 SDL_VIDEO_DRIVER_X11_DYNAMIC \
	SDL_VIDEO_DRIVER_WAYLAND SDL_VIDEO_DRIVER_WAYLAND_DYNAMIC \
	SDL_VIDEO_OPENGL SDL_VIDEO_OPENGL_GLX SDL_VIDEO_OPENGL_EGL SDL_VIDEO_RENDER_OGL \
	SDL_AUDIO_DRIVER_PULSEAUDIO SDL_AUDIO_DRIVER_PULSEAUDIO_DYNAMIC \
	SDL_AUDIO_DRIVER_PIPEWIRE SDL_AUDIO_DRIVER_PIPEWIRE_DYNAMIC \
	SDL_AUDIO_DRIVER_ALSA SDL_AUDIO_DRIVER_ALSA_DYNAMIC \
	SDL_JOYSTICK_LINUX
do
	line=$(grep -E "^#define $define( |$)" "$config" || true)
	if [ -z "$line" ]; then
		echo "SDL2 was configured without $define." >&2
		missing=1
	else
		echo "$line"
	fi
done
[ "$missing" = 0 ] || exit 1
[ "$("$prefix/bin/sdl2-config" --version)" = "$version" ]

cat > "$prefix/SOURCE-SDL2.txt" <<EOF
SDL2 $version, built from the upstream release tarball
  $url
  SHA-256 $sha256
EOF
echo "SDL2 $version installed in $prefix"
