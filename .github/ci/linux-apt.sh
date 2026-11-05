#!/usr/bin/env bash
# Installs the pinned Ubuntu package set of a Linux CI job and checks it, as
# the MSYS2 job does for .github/ci/msys2-packages.txt. Run as root inside
# the job's container (pinned by digest in .github/workflows/ci.yml).
#
# The list file names a snapshot.ubuntu.com timestamp and every package the
# container holds afterwards with its exact version; the lines marked
# "install" are the ones asked for. apt reads only that snapshot, so the same
# image and list always resolve to the same packages, and the job fails when
# the installed set differs from the list in any package or version.
#
# The base image has no CA certificates and the snapshot service redirects to
# HTTPS, so apt verifies the service with the CA file named by
# MMODERN_CI_CA_FILE (the hosted runner's bundle, mounted read-only) until
# the pinned ca-certificates package is installed. apt still verifies every
# index against the Ubuntu archive key.
#
# Usage: linux-apt.sh <packages.txt>
set -euo pipefail
list=$1

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

rm -f /etc/apt/sources.list /etc/apt/sources.list.d/*
cat > /etc/apt/sources.list.d/snapshot.sources <<EOF
Types: deb
URIs: https://snapshot.ubuntu.com/ubuntu/$ts/
Suites: $VERSION_CODENAME $VERSION_CODENAME-updates $VERSION_CODENAME-security
Components: main universe
Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
EOF
apt_options=(-o Acquire::Retries=3 -o Acquire::Check-Valid-Until=false)
if [ -n "${MMODERN_CI_CA_FILE:-}" ]; then
	apt_options+=(-o "Acquire::https::CAInfo=$MMODERN_CI_CA_FILE")
fi
echo "apt reads https://snapshot.ubuntu.com/ubuntu/$ts/ ($VERSION_CODENAME)."
apt-get "${apt_options[@]}" update
# shellcheck disable=SC2046 # one word per package=version
apt-get "${apt_options[@]}" install -y --no-install-recommends \
	$(awk '$3 == "install" { print $1 "=" $2 }' "$list")

awk '$1 != "snapshot" && $1 != "release" && $1 !~ /^#/ && NF { print $1, $2 }' "$list" | sort > /tmp/linux-packages.expected
dpkg-query -W -f='${binary:Package} ${Version}\n' | sort > /tmp/linux-packages.actual
if ! cmp -s /tmp/linux-packages.expected /tmp/linux-packages.actual; then
	echo 'Only expected (<), only installed (>):'
	diff /tmp/linux-packages.expected /tmp/linux-packages.actual || true
	echo "::error::The installed packages differ from $list."
	exit 1
fi
echo "All $(wc -l < /tmp/linux-packages.actual) installed packages match $list."
