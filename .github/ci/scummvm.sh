#!/usr/bin/env bash
# Configures the pinned ScummVM checkout, or builds the four artifacts
# MMModern links, exactly as docs/dependencies.md documents. Run from an
# MSYS2 UCRT64 shell. .github/workflows/ci.yml hashes this file into the key
# of its ScummVM build cache, so a changed command never restores a stale
# build. Change it and docs/dependencies.md together.
# Usage: scummvm.sh configure|build <scummvm-source> <scummvm-build>
set -eu
step=$1
source=$2
build=$3
case "$step" in
	configure)
		mkdir -p "$build"
		cd "$build"
		if [ "$(uname -s)" = Linux ]; then
			"$source/configure" \
				--backend=sdl \
				--disable-all-engines \
				--disable-detection-full \
				--disable-ogg --disable-vorbis
		else
		SDL_CONFIG=/ucrt64/bin/sdl2-config \
			"$source/configure" \
			--backend=sdl \
			--disable-all-engines \
			--disable-detection-full
		fi
		;;
	build)
		cd "$build"
		make -j4 \
			image/libimage.a \
			graphics/libgraphics.a \
			common/formats/po_parser.o \
			common/libcommon.a
		;;
	*)
		echo "usage: $0 configure|build <scummvm-source> <scummvm-build>" >&2
		exit 2
		;;
esac
