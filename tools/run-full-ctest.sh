#!/usr/bin/env bash
# Runs the complete CTest suite of an already configured and built Linux
# directory, writes the full output to a log and publishes a short summary
# only when the run has finished: the POSIX twin of tools/run-full-ctest.ps1,
# writing the same summary.txt. See tools/AGENTS.md.
#
# Parallel jobs: explicit -j/--jobs (1..256), otherwise half of nproc (at
# least 1, at most 8). The PowerShell runner's sandbox check and job
# preference file are Windows-only. CMake marks the real-clock and
# native-window tests RUN_SERIAL and locks the tests that share a fixed path.
#
# --sync-from <checkout> first turns this clone into a copy of another
# working tree, as the local WSL build does with the Windows checkout
# (docs/dependencies.md, "Linux build under WSL"): it fetches that
# checkout's HEAD, checks it out detached, removes untracked files that are
# not ignored (build directories stay) and applies the checkout's complete
# diff against HEAD, untracked files included. The diff is taken through a
# temporary index and object directory, so the other checkout is only read.
# .claude/ (agent settings, hidden by the Windows user's global excludes) is
# left out. The clone's own diff must then hash the same. The sync then
# rebuilds the default targets of the build directory before the suite runs;
# --sync-only stops after the sync, so other targets can be built first.
# The summary records the synced HEAD and the SHA-256 of that diff; without
# --sync-from, this checkout's own HEAD and diff, which after a sync are the
# same values. Every local change in this clone is discarded by --sync-from.
#
# Example:
#   bash tools/run-full-ctest.sh --build-dir build-linux-rel \
#     --sync-from /mnt/d/Projetos/MModern/mmodern
set -u -o pipefail

# The whole script is one function, so bash has read all of it before
# --sync-from replaces this file.
main() {
usage() {
	echo "usage: $0 --build-dir <dir> [-j <jobs>] [--ctest <ctest>] [--sync-from <checkout> [--sync-only]]" >&2
	exit 2
}

build_dir=''
jobs=0
ctest_cmd=ctest
sync_from=''
sync_only=0
while [ $# -gt 0 ]; do
	case "$1" in
		--build-dir) [ $# -ge 2 ] || usage; build_dir=$2; shift 2 ;;
		-j|--jobs) [ $# -ge 2 ] || usage; jobs=$2; shift 2 ;;
		--ctest) [ $# -ge 2 ] || usage; ctest_cmd=$2; shift 2 ;;
		--sync-from) [ $# -ge 2 ] || usage; sync_from=$2; shift 2 ;;
		--sync-only) sync_only=1; shift ;;
		*) usage ;;
	esac
done
[ -n "$build_dir" ] || usage
[ "$sync_only" -eq 0 ] || [ -n "$sync_from" ] || usage
case "$jobs" in
	''|*[!0-9]*) echo "Jobs must be an integer from 1 to 256." >&2; exit 2 ;;
esac
if [ "$jobs" -gt 256 ]; then
	echo "Jobs must be an integer from 1 to 256." >&2
	exit 2
fi

repo=$(cd "$(dirname "$0")/.." && pwd -P)
case "$build_dir" in
	/*) ;;
	*) build_dir="$PWD/$build_dir" ;;
esac
if [ ! -f "$build_dir/CTestTestfile.cmake" ]; then
	echo "No CTest configuration in $build_dir" >&2
	exit 1
fi
build_dir=$(cd "$build_dir" && pwd -P)

# Writes the complete diff of a working tree against its HEAD, untracked
# files included, without touching that tree's index or object store. Full
# blob names keep it independent of each repository's abbreviation length.
tree_diff() {
	local dir=$1 scratch objects
	scratch=$(mktemp -d) || return 1
	objects=$(git -C "$dir" rev-parse --path-format=absolute --git-path objects) || { rm -rf "$scratch"; return 1; }
	mkdir "$scratch/objects"
	(
		export GIT_INDEX_FILE="$scratch/index"
		export GIT_OBJECT_DIRECTORY="$scratch/objects"
		export GIT_ALTERNATE_OBJECT_DIRECTORIES="$objects"
		git -C "$dir" read-tree HEAD &&
			git -C "$dir" -c core.safecrlf=false add -A -- . ':(exclude).claude' &&
			git -C "$dir" diff --cached --binary --full-index --no-color --no-ext-diff HEAD
	)
	local code=$?
	rm -rf "$scratch"
	return $code
}

if ! command -v sha256sum >/dev/null; then
	echo "sha256sum is required." >&2
	exit 1
fi

synced_head=''
synced_sha=''
if [ -n "$sync_from" ]; then
	source_dir=$(cd "$sync_from" && pwd -P) || exit 1
	if [ "$source_dir" = "$repo" ]; then
		echo "--sync-from names this checkout itself." >&2
		exit 1
	fi
	patch=$(mktemp)
	synced_head=$(git -C "$source_dir" rev-parse HEAD) &&
		tree_diff "$source_dir" > "$patch" &&
		git -C "$repo" fetch --quiet --no-tags "$source_dir" HEAD &&
		[ "$(git -C "$repo" rev-parse FETCH_HEAD)" = "$synced_head" ] &&
		git -C "$repo" checkout --quiet --force --detach FETCH_HEAD &&
		git -C "$repo" clean -fdq &&
		{ [ ! -s "$patch" ] || git -C "$repo" apply --binary --whitespace=nowarn "$patch"; }
	if [ $? -ne 0 ]; then
		rm -f "$patch"
		echo "Could not sync $repo from $source_dir." >&2
		exit 1
	fi
	synced_sha=$(sha256sum < "$patch" | cut -d' ' -f1)
	rm -f "$patch"
	own_sha=$(tree_diff "$repo" | sha256sum | cut -d' ' -f1) ||
		{ echo "Could not hash the diff of $repo." >&2; exit 1; }
	if [ "$own_sha" != "$synced_sha" ]; then
		echo "After the sync this clone's diff hashes $own_sha, not $synced_sha." >&2
		exit 1
	fi
	echo "Synced $repo to $synced_head with diff SHA-256 $synced_sha."
	[ "$sync_only" -eq 0 ] || exit 0
	if ! cmake --build "$build_dir"; then
		echo "The build of $build_dir failed after the sync." >&2
		exit 1
	fi
else
	synced_head=$(git -C "$repo" rev-parse HEAD) || exit 1
	synced_sha=$(tree_diff "$repo" | sha256sum | cut -d' ' -f1) ||
		{ echo "Could not hash the diff of $repo." >&2; exit 1; }
fi

if [ "$jobs" -eq 0 ]; then
	jobs=$(( $(nproc) / 2 ))
	[ "$jobs" -ge 1 ] || jobs=1
	[ "$jobs" -le 8 ] || jobs=8
fi
ctest_exe=$(command -v "$ctest_cmd") || { echo "ctest not found: $ctest_cmd" >&2; exit 1; }

run_id="$(date +%Y%m%d-%H%M%S)-$(head -c 4 /dev/urandom | od -An -tx1 | tr -d ' \n')"
result_dir="$build_dir/ctest-results/$run_id"
mkdir -p "$result_dir" || exit 1
log="$result_dir/full.log"
summary="$result_dir/summary.txt"
ctest_args=(--test-dir "$build_dir" --no-tests=error --output-on-failure --parallel "$jobs")

echo "Running full suite with $jobs parallel jobs. Results: $result_dir"
started=$(date --iso-8601=seconds)
start_ns=$(date +%s%N)
"$ctest_exe" "${ctest_args[@]}" > "$log" 2>&1
code=$?
runner_error=''
[ -s "$log" ] || runner_error='ctest wrote no output'
# As in the PowerShell runner, a runner error is never a passing run.
[ -z "$runner_error" ] || [ "$code" -ne 0 ] || code=1
duration=$(awk -v s="$start_ns" -v e="$(date +%s%N)" 'BEGIN { printf "%.1f", (e - s) / 1e9 }')

{
	echo "Started: $started"
	echo "Finished: $(date --iso-8601=seconds)"
	echo "Executable: $ctest_exe"
	echo "Arguments: ${ctest_args[*]}"
	echo "Build directory: $build_dir"
	echo "Exit code: $code"
	echo "Parallel jobs: $jobs"
	echo "Duration seconds: $duration"
	echo "Runner error: $runner_error"
	echo "Synced HEAD: $synced_head"
	echo "Synced diff SHA-256: $synced_sha"
	echo "Full output and failure details: $log"
	echo
	grep -E '^[0-9]+% tests passed|^Total Test time|^[[:space:]]*[0-9]+[[:space:]]+-[[:space:]]+|No tests were found|CMake Error|CTest Error' "$log"
} > "$summary.tmp"
mv "$summary.tmp" "$summary"
echo "Finished. Exit code: $code. Summary: $summary"
exit $code
}

main "$@"
