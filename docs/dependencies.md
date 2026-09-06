# MMModern dependencies

MMModern is an independent SDL application. It does not use ScummVM's SDL
backend or instantiate its engine. The initial compatibility target reuses only
the Xeen archive/sprite code and the Common, Graphics, and Image libraries from
a separately configured ScummVM build.

Required build inputs:

- MSYS2 UCRT64 C++ toolchain
- CMake
- SDL2
- zlib
- the sibling `scummvm-master` source tree
- the minimal sibling `build-xeen-probe-sdl` build tree

`cmake/ScummVmXeen.cmake` contains the temporary MinGW object filtering needed
because `SpriteResource` keeps its stream decoder and engine-specific path
loader in the same source file. No file in the ScummVM tree is modified.

ScummVM is licensed under GPLv3. MMModern must remain compatible with the
license of the reused code. Original Might & Magic game data is not part of the
MMModern source or build output.
