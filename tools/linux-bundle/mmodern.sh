#!/bin/sh
# Starts MMModern from the Linux test bundle with the bundle's libraries.
# Usage: ./mmodern.sh <game-folder>, or any other mmodern command line.
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P) || exit 1
LD_LIBRARY_PATH="$here/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LD_LIBRARY_PATH
exec "$here/bin/mmodern" "$@"
