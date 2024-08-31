#!/usr/bin/env bash

set -e

[[ "$ARCADIA" ]] || {
    ARCADIA="$(readlink -f "$BASH_SOURCE")"
    ARCADIA="${SCRIPT_PATH%/alice*}"
}

[[ -d "$ARCADIA" ]] || { echo -e "\e[31;1m[ FAIL ] Failed to autodetect the ARCADIA root, set it via environment, like this:\n export ARCADIA=~/arcadia\e[0m"; exit 1; }

pushd "$(dirname $0)" > /dev/null

echo "Checking out VINS..."
pushd "$ARCADIA" > /dev/null
ya make -j0 --checkout "alice/vins"
popd

echo "Building convert_vins tool..."
ya make -r --checkout

echo "Starting convert_vins tool..."
./convert_vins
