#!/usr/bin/env bash
# Installs the pinned Ubuntu package set of a Linux CI job and checks it, as
# the MSYS2 job does for .github/ci/msys2-packages.txt. Run as root inside
# the job's container (pinned by digest in the workflow).
#
# The list file names a snapshot.ubuntu.com timestamp and every package the
# container holds afterwards with its exact version; the lines marked
# "install" are the ones asked for. apt reads only that snapshot, so the same
# image and list always resolve to the same packages, and the job fails when
# the installed set differs from the list in any package or version.
#
# A job runs three commands:
#   key <list>                  prints the package cache key: the release,
#                               the list, this script and the image's own
#                               installed set
#   fetch <list> <cache-dir>    unless <cache-dir> already holds the mirror
#                               of this list, downloads the snapshot's signed
#                               indexes and the needed packages into it as a
#                               local mirror, which the job caches
#   install <list> <cache-dir>  installs from that mirror only, offline, and
#                               compares the installed set with the list
# The mirror keeps the snapshot's InRelease files as published, so the
# install checks them against the Ubuntu archive key and every index and
# package against the hashes they sign, as it would read the snapshot
# itself, and asks for the exact versions of the list. A cache hit saves
# only the downloads: a changed mirror fails the install.
#
# fetch stops as soon as any index or package cannot be downloaded, naming
# the snapshot service, instead of resolving against incomplete indexes.
# apt retries each file three times; the update and the package download are
# each tried three times, 30 and 60 seconds apart, and each update attempt is
# limited to two minutes.
#
# The base image has no CA certificates and the snapshot service redirects to
# HTTPS, so fetch reaches the service with the CA file named by
# MMODERN_CI_CA_FILE (the hosted runner's bundle, mounted read-only).
#
# Usage: linux-apt.sh key <packages.txt>
#        linux-apt.sh fetch|install <packages.txt> <cache-dir>
set -euo pipefail

usage() {
	echo "usage: $0 key <packages.txt> | fetch|install <packages.txt> <cache-dir>" >&2
	exit 2
}
[ $# -ge 2 ] || usage
mode=$1
list=$2
case "$mode" in
	key) [ $# -eq 2 ] || usage ;;
	fetch|install) [ $# -eq 3 ] || usage; mirror=$3/mirror ;;
	*) usage ;;
esac

ts=$(awk '$1 == "snapshot" { print $2 }' "$list")
case "$ts" in
	[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]T[0-9][0-9][0-9][0-9][0-9][0-9]Z) ;;
	*) echo "::error::$list names no snapshot timestamp." >&2; exit 1 ;;
esac
. /etc/os-release
expected_release=$(awk '$1 == "release" { print $2 }' "$list")
if [ "$VERSION_CODENAME" != "$expected_release" ]; then
	echo "::error::The container is $VERSION_CODENAME, but $list is for $expected_release." >&2
	exit 1
fi

if [ "$mode" = key ]; then
	list_sum=$(cat "$list" "$0" | sha256sum | cut -c1-16)
	image_sum=$(sha256sum < /var/lib/dpkg/status | cut -c1-16)
	echo "apt-$VERSION_CODENAME-$list_sum-$image_sum"
	exit 0
fi

snapshot=https://snapshot.ubuntu.com/ubuntu/$ts/
stamp="$snapshot $(sha256sum < "$list" | cut -d' ' -f1)"
# shellcheck disable=SC2207 # one word per package=version
packages=($(awk '$3 == "install" { print $1 "=" $2 }' "$list"))

write_sources() {
	rm -f /etc/apt/sources.list /etc/apt/sources.list.d/*
	cat > /etc/apt/sources.list.d/snapshot.sources <<EOF
Types: deb
URIs: $1
Suites: $VERSION_CODENAME $VERSION_CODENAME-updates $VERSION_CODENAME-security
Components: main universe
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF
}

# attempt <what> <command...>: runs the command up to three times.
attempt() {
	local what=$1 n
	shift
	for n in 1 2 3; do
		if "$@"; then
			return 0
		fi
		if [ "$n" -lt 3 ]; then
			echo "Attempt $n to download the $what failed; trying again in $((n * 30)) s."
			sleep $((n * 30))
		fi
	done
	echo "::error::snapshot.ubuntu.com unavailable: apt could not download" \
		"the $what of $snapshot in 3 attempts." >&2
	exit 1
}

if [ "$mode" = fetch ]; then
	if [ -f "$mirror/mmodern-apt.txt" ] && [ "$(cat "$mirror/mmodern-apt.txt")" = "$stamp" ]; then
		echo "$mirror holds the packages of $list; nothing to download."
		exit 0
	fi
	rm -rf "$mirror"
	work=$(mktemp -d)
	# apt downloads as the _apt user into partial/.
	chmod 755 "$work"
	mkdir -p "$work/lists/partial" "$work/archives/partial"
	write_sources "$snapshot"
	online=(-o Acquire::Retries=3 -o Acquire::http::Timeout=30 -o Acquire::https::Timeout=30
		-o Acquire::Check-Valid-Until=false -o APT::Update::Error-Mode=any
		-o "Dir::State::Lists=$work/lists" -o "Dir::Cache::archives=$work/archives")
	if [ -n "${MMODERN_CI_CA_FILE:-}" ]; then
		online+=(-o "Acquire::https::CAInfo=$MMODERN_CI_CA_FILE")
	fi
	echo "apt reads $snapshot ($VERSION_CODENAME)."
	attempt 'package indexes' timeout 120 apt-get "${online[@]}" update
	apt-get "${online[@]}" install -y --no-install-recommends --print-uris \
		"${packages[@]}" > "$work/print-uris.txt"
	attempt 'packages' apt-get "${online[@]}" install -y --no-install-recommends \
		--download-only "${packages[@]}"

	# The mirror: each InRelease as published, each package index
	# uncompressed (InRelease signs that form too), each package at its pool
	# path.
	prefix="snapshot.ubuntu.com_ubuntu_${ts}_"
	for file in "$work/lists/$prefix"*; do
		name=${file##*/}
		path=${name#"$prefix"}
		path=${path//_//}
		case "$path" in
			dists/*/InRelease) target=$mirror/$path; mkdir -p "${target%/*}"; cp "$file" "$target" ;;
			dists/*/Packages|dists/*/Packages.*)
				target=$mirror/${path%/Packages*}/Packages
				mkdir -p "${target%/*}"
				/usr/lib/apt/apt-helper cat-file "$file" > "$target" ;;
			*) echo "::error::Unexpected index file $name." >&2; exit 1 ;;
		esac
	done
	count=0
	while read -r uri file _; do
		uri=${uri//\'/}
		path=${uri#"$snapshot"}
		path=$(printf '%b' "${path//%/\\x}")
		mkdir -p "$mirror/${path%/*}"
		mv "$work/archives/$file" "$mirror/$path"
		count=$((count + 1))
	done < <(grep "^'" "$work/print-uris.txt")
	echo "$stamp" > "$mirror/mmodern-apt.txt"
	rm -rf "$work"
	echo "Stored the indexes and $count packages of $snapshot in $mirror."
	exit 0
fi

if [ ! -f "$mirror/mmodern-apt.txt" ] || [ "$(cat "$mirror/mmodern-apt.txt")" != "$stamp" ]; then
	echo "::error::$mirror does not hold the packages of $list; run fetch first." >&2
	exit 1
fi
# copy: reads the mirror like a server: apt skips the compressed index forms
# the mirror lacks (Ign) and takes the uncompressed one. file: would answer
# for the uncompressed form under a compressed name and fail its hash check.
write_sources "copy:$mirror/"
offline=(-o Acquire::Check-Valid-Until=false -o APT::Update::Error-Mode=any
	-o Acquire::By-Hash=no)
echo "apt reads the mirror of $snapshot in $mirror ($VERSION_CODENAME)."
apt-get "${offline[@]}" update
apt-get "${offline[@]}" install -y --no-install-recommends "${packages[@]}"

awk '$1 != "snapshot" && $1 != "release" && $1 !~ /^#/ && NF { print $1, $2 }' "$list" | sort > /tmp/linux-packages.expected
dpkg-query -W -f='${binary:Package} ${Version}\n' | sort > /tmp/linux-packages.actual
if ! cmp -s /tmp/linux-packages.expected /tmp/linux-packages.actual; then
	echo 'Only expected (<), only installed (>):'
	diff /tmp/linux-packages.expected /tmp/linux-packages.actual || true
	echo "::error::The installed packages differ from $list."
	exit 1
fi
echo "All $(wc -l < /tmp/linux-packages.actual) installed packages match $list."
